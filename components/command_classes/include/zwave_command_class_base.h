/******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef ZWAVE_COMMAND_CLASS_BASE_H
#define ZWAVE_COMMAND_CLASS_BASE_H

#ifdef __cplusplus

#include "zwave_command_class_base_definitions.h"

// Z-Wave includes
#include "zwave_generic_types.h"               // zwave_command_class_t
#include "zwave_controller_connection_info.h"  // zwave_controller_connection_info_t

#include "zpc_mqtt.hpp"

// ZPC includes
#include "attribute.hpp"

// Cpp includes
#include <string>
#include <vector>
#include <map>

// Generator & parsers
#include "zwave_frame_parser.hpp"                   // zwave_frame_parser
#include "zwave_frame_generator.hpp"                // zwave_frame_generator
#include "zwave_frame_generator_standalone.hpp"     // zwave_frame_generator_standalone
#include "zpc_attribute_store_type_registration.h"  // attribute_schema_t
#include "attribute_resolver.hpp"                   // group_resolution_options

namespace zwave_command_class
{

    using attribute_list_registration_t = std::vector<attribute_schema_t>;

    class zwave_command_class_base
    {
        public:
            // Arguments when a REPORT is received
            struct report_received_args {
                    // Endpoint node associated with the report
                    attribute_store::attribute endpoint_node;
                    // The frame parser to extract the data from the frame
                    zwave_frame_parser &report_frame_parser;
                    // Used in some cases to match a node with a session
                    const zwave_controller_connection_info_t *connection_info;
            };
            // Arguments when a GET is received
            struct get_received_args {
                    // Endpoint node associated with the get
                    attribute_store::attribute endpoint_node;
                    // Get frame to parse
                    zwave_frame_parser &received_get_frame;
                    // Frame generator to generate the response
                    // If you return SL_STATUS_OK, the top level handler will send the frame
                    // If you return SL_STATUS_IN_PROGRESS, you need to send the frame yourself
                    // Otherwise the contents of this is ignored
                    zwave_frame_generator_standalone *const response_report_frame;
                    // Used in some cases to send the response in the get handler
                    const zwave_controller_connection_info_t *connection_info;
            };
            // Arguments when a SET is received
            struct set_received_args {
                    // Endpoint node associated with the set
                    attribute_store::attribute endpoint_node;
                    // Set frame to parse
                    zwave_frame_parser &received_set_frame;
            };

            // Arguments when a SET is requested
            struct set_requested_args {
                    // One of the node in "triggered_by" in the data model
                    attribute_store::attribute node;
                    // The frame generator to generate the frame
                    zwave_frame_generator *const set_frame_generator;
            };
            // Arguments when a GET is requested
            struct get_requested_args {
                    // One of the node in "triggered_by" in the data model
                    attribute_store::attribute node;
                    // The frame generator to generate the frame
                    zwave_frame_generator *const get_frame_generator;
            };

            static std::map<int, int> supported_command_class_versions;

            virtual ~zwave_command_class_base() = default;

            virtual bool is_supported_on_node(attribute_store::attribute endpoint_node) const;

            /**
             * @brief Register the attributes for this command class
             *
             * @param attributes The list of attributes to register for this command class
             *
             * @return sl_status_t SL_STATUS_OK if successful, SL_STATUS_FAIL otherwise
             */
            static sl_status_t register_attribute_types(const attribute_list_registration_t &attributes);

            /**
             * @brief Register the command class to MQTT
             *
             * @note This function is called automatically by the generated constructor
             */
            virtual void mqtt_register();

            void mqtt_register_command_handler(void);

            /**
             * @brief Publish the supported MQTT commands for this command class
             *
             * @note This function is called automatically after the on_interview() callback
             *
             * @param endpoint_node The endpoint node to publish the commands for
             */
            virtual void mqtt_publish_supported_commands(const attribute_store::attribute &endpoint_node);

            /**
             * @brief Called when interview for the command class is done
             *
             * This function is used in the implementation to trigger all the gets we need to
             * have the current status of the end device.
             *
             * @param endpoint_node The endpoint node that was interviewed
             * @param supported_version The version of the command class supported by the endpoint
             */
            virtual void on_interview(attribute_store::attribute endpoint_node, uint8_t supported_version);

            /**
             * @brief This is the function which will be executed when a Report frame of
             * a given Command Class is received.
             *
             * The handler MUST return a \ref sl_status_t status code.
             *
             * @param connection Info about the connection properties of this frame.
             * @param frame_data The data payload of this frame.
             * @param frame_length The length of this frame.
             *
             * @returns SL_STATUS_OK    The command was handled by the command handler.
             *                          Supervision will return SUCCESS in this case
             * @returns SL_STATUS_FAIL  The command handler was unable to parse the command
             *                          or was busy carring another operation.
             *                          Supervision Command Class returns FAIL.
             * @returns SL_STATUS_IN_PROGRESS   The command handler is processing the command
             *                                  Supervision Command Class returns WORKING.
             * @returns SL_STATUS_NOT_SUPPORTED The command handler does not support this
             *                                  Command or Command Class.
             *                                  Supervision Command Class returns NO_SUPPORT.
             * @returns Any other status: Supervision Command Class returns FAIL.
             */
            virtual sl_status_t control_handler(const zwave_controller_connection_info_t *connection_info, const uint8_t *frame_data, uint16_t frame_length);

            /**
             * @brief This is the function which will be executed when a Set/Get frame of
             * a given Command Class is received.
             *
             * The handler MUST return a \ref sl_status_t status code.
             *
             * @param connection Info about the connection properties of this frame.
             * @param frame_data The data payload of this frame.
             * @param frame_length The length of this frame.
             *
             * @returns SL_STATUS_OK    The command was handled by the command handler.
             *                          Supervision will return SUCCESS in this case
             * @returns SL_STATUS_FAIL  The command handler was unable to parse the command
             *                          or was busy carring another operation.
             *                          Supervision Command Class returns FAIL.
             * @returns SL_STATUS_IN_PROGRESS   The command handler is processing the command
             *                                  Supervision Command Class returns WORKING.
             * @returns SL_STATUS_NOT_SUPPORTED The command handler does not support this
             *                                  Command or Command Class.
             *                                  Supervision Command Class returns NO_SUPPORT.
             * @returns Any other status: Supervision Command Class returns FAIL.
             */
            virtual sl_status_t support_handler(const zwave_controller_connection_info_t *connection_info, const uint8_t *frame_data, uint16_t frame_length);

            /**
             * @brief Returns true if the command class has a control handler
             */
            virtual bool has_control_handler() const;
            /**
             * @brief Returns true if the command class has a support handler
             */
            virtual bool has_support_handler() const;

            /**
             * @brief Get the ID of the command class
             */
            zwave_command_class_t id() const;
            /**
             * @brief Get the minimal encapsulation scheme supported by the SUPPORT handler
             *
             * @note Doesn't apply to the control handler
             */
            zwave_controller_encapsulation_scheme_t supported_handler_minimal_scheme() const;
            /**
             * @brief Get the human readable name of the command class
             */
            std::string display_name() const;
            /**
             * @brief Get the implementations comments
             */
            std::string comments() const;
            /**
             * @brief Get the implementation supported version
             */
            uint8_t supported_version() const;
            /**
             * @brief Get if the command class uses manual security validation
             * Use manual-security filtering for incoming frames
             * If set to true, the command class dispatch handler will send frames to the
             * handler without validating their security level.
             * If set to false, the command class handler can assume that the frame has
             * been received at an approved security level.
             */
            bool manual_security_validation() const;

            /**
             * @brief Get the name of the command class used in MQTT
             *
             * E.g. : {prefix}/{mqtt_namespace}/[Attributes|Command]
             *
             * @return The name of the command class used in MQTT
             */
            const std::string &mqtt_class_namespace() const;

            /**
             * @brief Get the version of the command class supported from endpoint node
             *
             * @param node Any node that has an endpoint parent (or the endpoint itself)
             *
             * @return The version of the command class supported by the node (0 if not supported)
             */
            uint8_t endpoint_supported_version(const attribute_store::attribute &node) const;

            /**
             * @brief Returns true if the endpoint reports support for this command class.
             *
             * Checks the endpoint-specific S2 and S0 Commands Supported report lists
             * (not the Version CC). Use this when deciding whether to run interview
             * or create attributes for a given endpoint.
             *
             * @param endpoint_node The endpoint node (ATTRIBUTE_ENDPOINT_ID).
             * @return true if this CC is in the endpoint's S2 or S0 reported list.
             */
            bool endpoint_supports_command_class(const attribute_store::attribute &endpoint_node) const;

            /**
             * @brief Returns true if endpoint_node is the root endpoint (ep0) of a
             *        device that has at least one individual sub-endpoint (ep1, ep2, …).
             *
             * On such devices the root endpoint may advertise a CC for aggregation
             * purposes without implementing it directly (spec CC:0060.02). Any CC
             * whose GET might go unanswered on ep0 can use this to reduce retries.
             *
             * ep0 may have a reported endpoint ID of 0; sub-endpoints use reported IDs > 0.
             *
             * @param endpoint_node The endpoint node passed to on_interview().
             * @return true if this is ep0 of a multi-endpoint device.
             */
            static bool is_root_of_multi_endpoint_device(const attribute_store::attribute &endpoint_node);

        protected:
            /**
             * @brief Constructor
             *
             * @param properties The properties of the command class (name, id, ...)
             * @param attributes The list of attributes to register for this command class
             *
             */
            zwave_command_class_base(command_class_properties properties, const attribute_list_registration_t &attributes, const std::string &mqtt_cc_name);

            /**
             * @brief Function to handle the interview process
             *
             * Called when interview completes for an endpoint.
             * Will call on_interview if the command class is supported by the endpoint.
             *
             * @param endpoint_node The endpoint node that completed interview
             * @return sl_status_t SL_STATUS_OK on success
             */
            sl_status_t interview(attribute_store_node_t endpoint_node);

            // Force interview for the command class even if it is not supported by the node
            bool force_interview_for_cc = false;

            /**
             * @brief Returns the resolution options appropriate for the endpoint currently being interviewed.
             *
             * Computed by interview() before on_interview() is called. Use this in on_interview()
             * when calling start_group_resolution() so that topology-aware retry behaviour
             * (e.g. fewer retries on ep0 of a multi-endpoint device) is applied automatically
             * without the CC needing to know about the device topology.
             */
            const group_resolution_options &interview_resolution_options() const;

            /**
             * @brief MQTT command handler
             *
             * This function is called when a command is received from MQTT.
             *
             * @param endpoint_node The endpoint node that received the command
             * @param command_name The name of the command
             * @param payload The payload of the command
             */
            virtual void mqtt_command_handler(attribute_store::attribute endpoint_node, const std::string &command_name, const std::string &payload);

            /**
             * @brief Called when node information is received
             *
             * @param node_id The id of the node that sent the node information
             * @param node_info The node information
             */
            static void on_node_information(zwave_node_id_t node_id, const zwave_node_info_t *node_info);

            // Command class properties (name, version, ...)
            const command_class_properties properties;
            // Retry options resolved in interview() before on_interview() is called
            group_resolution_options m_interview_resolution_options;
            // Command class name used in MQTT
            const std::string mqtt_command_class_namespace;
            // Frame Helpers
            zwave_frame_generator m_frame_generator;

            std::map<std::string, std::function<void(attribute_store::attribute &endpoint_node, std::string)>> mqtt_callback_map;
    };
}  // namespace zwave_command_class

#endif  // __cplusplus

#endif  // ZWAVE_COMMAND_CLASS_BASE_H