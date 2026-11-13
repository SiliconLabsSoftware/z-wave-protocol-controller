/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include "attribute.hpp"
#include "attribute_resolver.h"
#include "attribute_resolver.hpp"
#include "attribute_store_defined_attribute_types.h"
#include "attribute_store_fixt.h"
#include "attribute_store_type_registration.h"
#include "command_class_user_credential.hpp"
#include "command_class_user_credential_core.hpp"
#include "command_class_user_credential_events.hpp"
#include "command_class_user_credential_rules.hpp"
#include "command_class_user_credential_types.hpp"
#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "component_connector_types.hpp"
#include "zwave_frame_generator.hpp"
#include "zwave_frame_parser.hpp"
#include "ZW_classcmd.h"

using namespace zwave_command_class;
using namespace zwave_command_class::command_class_user_credential_types;

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}

namespace user_credential_test
{
    constexpr uint8_t USER_CREDENTIAL_CC                   = COMMAND_CLASS_USER_CREDENTIAL;
    constexpr uint8_t USER_CODE_CC                         = COMMAND_CLASS_USER_CODE;
    constexpr uint8_t VERSION_1                            = 1;
    constexpr uint8_t MAX_CONNECTOR_EVENT_DRAIN_PASSES     = 8;
    constexpr uint8_t COMMAND_INDEX                        = 1;
    constexpr uint8_t FRAME_USER_CAPABILITIES_REPORT       = 0x02;
    constexpr uint8_t FRAME_CREDENTIAL_CAPABILITIES_REPORT = 0x04;
    constexpr uint8_t FRAME_USER_SET                       = 0x05;
    constexpr uint8_t FRAME_USER_GET                       = 0x06;
    constexpr uint8_t FRAME_USER_REPORT                    = 0x07;
    constexpr uint8_t FRAME_CREDENTIAL_SET                 = 0x0A;
    constexpr uint8_t FRAME_CREDENTIAL_GET                 = 0x0B;
    constexpr uint8_t FRAME_CREDENTIAL_REPORT              = 0x0C;
    constexpr uint8_t FRAME_CREDENTIAL_LEARN_START         = 0x0F;
    constexpr uint8_t FRAME_CREDENTIAL_LEARN_CANCEL        = 0x10;
    constexpr uint8_t FRAME_ASSOCIATION_SET                = 0x12;
    constexpr uint8_t FRAME_ALL_USERS_CHECKSUM_GET         = 0x14;
    constexpr uint8_t FRAME_ALL_USERS_CHECKSUM_REPORT      = 0x15;
    constexpr uint8_t FRAME_USER_CHECKSUM_GET              = 0x16;
    constexpr uint8_t FRAME_CREDENTIAL_CHECKSUM_GET        = 0x18;
    constexpr uint8_t FRAME_ADMIN_PIN_SET                  = 0x1A;
    constexpr uint8_t FRAME_ADMIN_PIN_GET                  = 0x1B;
    constexpr uint8_t OPERATION_ADD                        = 0;
    constexpr uint8_t OPERATION_MODIFY                     = 1;
    constexpr uint8_t OPERATION_DELETE                     = 2;
    constexpr uint8_t OPERATION_UNDEFINED                  = 3;
    constexpr uint8_t USER_TYPE_GENERAL                    = 0x00;
    constexpr uint8_t USER_TYPE_UNDEFINED                  = 0x02;
    constexpr uint8_t USER_TYPE_PROGRAMMING                = 0x03;
    constexpr uint8_t USER_TYPE_EXPIRING                   = 0x07;
    constexpr uint8_t USER_TYPE_REMOTE_ONLY                = 0x09;
    constexpr uint8_t USER_ACTIVE                          = 1;
    constexpr uint8_t USER_STATE_UNDEFINED                 = 2;
    constexpr uint8_t CREDENTIAL_RULE_UNDEFINED            = 0x00;
    constexpr uint8_t CREDENTIAL_RULE_SINGLE               = 0x01;
    constexpr uint8_t CREDENTIAL_RULE_DUAL                 = 0x02;
    constexpr uint8_t CREDENTIAL_RULE_TRIPLE               = 0x03;
    constexpr uint8_t CREDENTIAL_RULE_ABOVE_TRIPLE         = 0x04;
    constexpr uint8_t CREDENTIAL_TYPE_NONE                 = 0x00;
    constexpr uint8_t CREDENTIAL_TYPE_PIN                  = 0x01;
    constexpr uint8_t CREDENTIAL_TYPE_PASSWORD             = 0x02;
    constexpr uint8_t ENCODING_ASCII                       = 0;
    constexpr uint8_t ENCODING_EXTENDED_ASCII              = 1;
    constexpr uint8_t ENCODING_UTF_16                      = 2;
    constexpr uint8_t ENCODING_UNDEFINED                   = 3;
    constexpr uint8_t ASCII_SUPPORT                        = 0x04;
    constexpr uint8_t EXTENDED_ASCII_SUPPORT               = 0x08;
    constexpr uint8_t UTF_16_SUPPORT                       = 0x10;
    constexpr uint8_t USER_CHECKSUM_SUPPORT                = 0x20;
    constexpr uint8_t ALL_USERS_CHECKSUM_SUPPORT           = 0x40;
    constexpr uint8_t USER_SCHEDULE_SUPPORT                = 0x80;
    constexpr uint8_t ADMIN_DEACTIVATION_SUPPORT           = 0x20;
    constexpr uint8_t ADMIN_CODE_SUPPORT                   = 0x40;
    constexpr uint8_t CREDENTIAL_CHECKSUM_SUPPORT          = 0x80;
    constexpr uint8_t SINGLE_RULE_BIT                      = 0x02;
    constexpr uint8_t DUAL_RULE_BIT                        = 0x04;
    constexpr uint8_t TRIPLE_RULE_BIT                      = 0x08;
    constexpr uint8_t GENERAL_USER_BIT                     = 0x01;
    constexpr uint8_t PROGRAMMING_TO_EXPIRING_USER_BITS    = 0xF8;
    constexpr uint16_t NO_USERS                            = 0;
    constexpr uint16_t TWO_USERS                           = 2;
    constexpr uint16_t USER_IDENTIFIER_1                   = 1;
    constexpr uint16_t USER_IDENTIFIER_2                   = 2;
    constexpr uint16_t USER_IDENTIFIER_OUT_OF_RANGE        = 99;
    constexpr uint16_t SLOT_0                              = 0;
    constexpr uint16_t SLOT_1                              = 1;
    constexpr uint16_t SLOT_2                              = 2;
    constexpr uint16_t SLOT_OUT_OF_RANGE                   = 99;
    constexpr uint16_t TEN_SLOTS                           = 10;
    constexpr uint8_t PIN_MIN                              = 4;
    constexpr uint8_t PIN_MAX                              = 10;
    constexpr uint8_t USER_NAME_MAX                        = 16;
    constexpr uint8_t BIT_MASK_LENGTH                      = 1;
    constexpr uint8_t DIGIT_1                              = 0x31;
    constexpr uint8_t DIGIT_2                              = 0x32;
    constexpr uint8_t DIGIT_3                              = 0x33;
    constexpr uint8_t DIGIT_4                              = 0x34;
    constexpr uint8_t NON_DIGIT                            = 0x41;
    constexpr uint8_t REPORT_RESPONSE_TO_GET               = 0x04;
    constexpr uint16_t CHECKSUM_NONE                       = 0;
    constexpr uint16_t CHECKSUM_PRESENT                    = 0x1234;
    constexpr uint16_t CHECKSUM_OTHER                      = 0x5678;
    constexpr uint16_t EXPIRING_MINUTES                    = 30;
    constexpr uint8_t LEARN_TIMEOUT                        = 10;
    constexpr uint8_t FRAME_BUFFER_SIZE                    = 64;
    constexpr uint8_t TEST_NODE_ID                         = 1;
    constexpr uint8_t TEST_ENDPOINT_ID                     = 0;
    constexpr std::array<uint8_t, 4> TEST_HOME_ID {0x01, 0x02, 0x03, 0x04};

    const std::vector<uint8_t> FOUR_DIGIT_PIN {DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4};

    template<typename Type> attribute_store_type_t type_of(Type type)
    {
        return static_cast<attribute_store_type_t>(type);
    }

    uint8_t msb(uint16_t value)
    {
        return static_cast<uint8_t>(value >> 8);
    }

    uint8_t lsb(uint16_t value)
    {
        return static_cast<uint8_t>(value & 0xFF);
    }

    const auto USER_CAP_GET_GROUP      = type_of(user_capabilities_get_group_attributes_t::USER_CAPABILITIES_GET_GROUP);
    const auto USER_CAP_REPORT_GROUP   = type_of(user_capabilities_report_group_attributes_t::USER_CAPABILITIES_REPORT_GROUP);
    const auto CRED_CAP_GET_GROUP      = type_of(credential_capabilities_get_group_attributes_t::CREDENTIAL_CAPABILITIES_GET_GROUP);
    const auto CRED_CAP_REPORT_GROUP   = type_of(credential_capabilities_report_group_attributes_t::CREDENTIAL_CAPABILITIES_REPORT_GROUP);
    const auto CHECKSUM_GET_GROUP      = type_of(all_users_checksum_get_group_attributes_t::ALL_USERS_CHECKSUM_GET_GROUP);
    const auto CHECKSUM_REPORT_GROUP   = type_of(all_users_checksum_report_group_attributes_t::ALL_USERS_CHECKSUM_REPORT_GROUP);
    const auto USER_GET_GROUP          = type_of(user_get_group_attributes_t::USER_GET_GROUP);
    const auto USER_SET_GROUP          = type_of(user_set_group_attributes_t::USER_SET_GROUP);
    const auto USER_REPORT_GROUP       = type_of(user_report_group_attributes_t::USER_REPORT_GROUP);
    const auto CREDENTIAL_GET_GROUP    = type_of(credential_get_group_attributes_t::CREDENTIAL_GET_GROUP);
    const auto CREDENTIAL_SET_GROUP    = type_of(credential_set_group_attributes_t::CREDENTIAL_SET_GROUP);
    const auto CREDENTIAL_REPORT_GROUP = type_of(credential_report_group_attributes_t::CREDENTIAL_REPORT_GROUP);
    const auto LEARN_START_GROUP       = type_of(credential_learn_start_group_attributes_t::CREDENTIAL_LEARN_START_GROUP);
    const auto LEARN_CANCEL_GROUP      = type_of(credential_learn_cancel_group_attributes_t::CREDENTIAL_LEARN_CANCEL_GROUP);
    const auto ASSOCIATION_SET_GROUP   = type_of(user_credential_association_set_group_attributes_t::USER_CREDENTIAL_ASSOCIATION_SET_GROUP);
    const auto USER_CHECKSUM_GET_GROUP = type_of(user_checksum_get_group_attributes_t::USER_CHECKSUM_GET_GROUP);
    const auto CRED_CHECKSUM_GET_GROUP = type_of(credential_checksum_get_group_attributes_t::CREDENTIAL_CHECKSUM_GET_GROUP);
    const auto ADMIN_SET_GROUP         = type_of(admin_pin_code_set_group_attributes_t::ADMIN_PIN_CODE_SET_GROUP);
    const auto ADMIN_GET_GROUP         = type_of(admin_pin_code_get_group_attributes_t::ADMIN_PIN_CODE_GET_GROUP);

    void register_byte_array_attributes()
    {
        const auto add = [](attribute_store_type_t type, const char *name, attribute_store_type_t parent) {
            if (!attribute_store_is_type_registered(type)) {
                attribute_store_register_type(type, name, parent, BYTE_ARRAY_STORAGE_TYPE);
            }
        };
        add(type_of(user_capabilities_report_group_attributes_t::supported_credential_rules_bit_mask), "rules", USER_CAP_REPORT_GROUP);
        add(type_of(user_capabilities_report_group_attributes_t::supported_user_types_bit_mask), "user_types", USER_CAP_REPORT_GROUP);
        add(type_of(user_set_group_attributes_t::user_name), "user_name", USER_SET_GROUP);
        add(type_of(user_report_group_attributes_t::user_name), "user_name_report", USER_REPORT_GROUP);
        add(type_of(credential_capabilities_report_group_attributes_t::vg1_credential_type), "types", CRED_CAP_REPORT_GROUP);
        add(type_of(credential_capabilities_report_group_attributes_t::vg3_number_of_supported_credential_slots_per_credential_type), "slots", CRED_CAP_REPORT_GROUP);
        add(type_of(credential_capabilities_report_group_attributes_t::vg4_min_length_of_credential_data_per_credential_type), "min", CRED_CAP_REPORT_GROUP);
        add(type_of(credential_capabilities_report_group_attributes_t::vg5_max_length_of_credential_data_per_credential_type), "max", CRED_CAP_REPORT_GROUP);
        add(type_of(credential_set_group_attributes_t::credential_data), "credential_data", CREDENTIAL_SET_GROUP);
        add(type_of(credential_report_group_attributes_t::credential_data), "credential_data_report", CREDENTIAL_REPORT_GROUP);
        add(type_of(admin_pin_code_set_group_attributes_t::admin_code), "admin_code", ADMIN_SET_GROUP);
        add(type_of(admin_pin_code_report_group_attributes_t::admin_code), "admin_code_report", type_of(admin_pin_code_report_group_attributes_t::ADMIN_PIN_CODE_REPORT_GROUP));
    }

    std::vector<uint8_t> user_capabilities(uint16_t users, uint8_t rules, uint8_t properties, uint8_t type_mask)
    {
        return {USER_CREDENTIAL_CC,              // Command Class
                FRAME_USER_CAPABILITIES_REPORT,  // Command
                msb(users),
                lsb(users),       // Number of Supported User Unique Identifiers
                rules,            // Supported Credential Rules Bit Mask
                USER_NAME_MAX,    // Max Length of User Name
                properties,       // User Schedule, checksum, and name encoding bits
                BIT_MASK_LENGTH,  // Supported User Types Bit Mask Length
                type_mask};       // Supported User Types Bit Mask
    }

    std::vector<uint8_t> general_user_capabilities(uint8_t properties = ASCII_SUPPORT)
    {
        return user_capabilities(TWO_USERS, SINGLE_RULE_BIT, properties, GENERAL_USER_BIT);
    }

    std::vector<uint8_t> pin_credential_capabilities(uint8_t properties = 0)
    {
        constexpr uint8_t ONE_CREDENTIAL_TYPE = 1;
        return {USER_CREDENTIAL_CC,                    // Command Class
                FRAME_CREDENTIAL_CAPABILITIES_REPORT,  // Command
                properties,                            // Checksum, Admin Code, and Admin Code Deactivation bits
                ONE_CREDENTIAL_TYPE,                   // Number of Supported Credential Types
                CREDENTIAL_TYPE_PIN,                   // Credential Type
                0,                                     // Credential Learn support for that type
                msb(TEN_SLOTS),
                lsb(TEN_SLOTS),  // Number of Supported Credential Slots
                PIN_MIN,         // Min Length of Credential Data
                PIN_MAX,         // Max Length of Credential Data
                0,               // Credential Learn recommended timeout
                0,               // Credential Learn number of steps
                0};              // Maximum Credential Hash Length
    }

    std::vector<uint8_t> all_users_checksum_report(uint16_t checksum)
    {
        return {USER_CREDENTIAL_CC, FRAME_ALL_USERS_CHECKSUM_REPORT, msb(checksum), lsb(checksum)};
    }

    std::vector<uint8_t> user_report(uint16_t user_identifier, uint16_t next_user_identifier)
    {
        return {USER_CREDENTIAL_CC,
                FRAME_USER_REPORT,
                REPORT_RESPONSE_TO_GET,
                msb(next_user_identifier),
                lsb(next_user_identifier),
                0,  // User Modifier Type: DNE
                0,
                0,  // User Modifier Node ID
                msb(user_identifier),
                lsb(user_identifier),
                USER_TYPE_GENERAL,
                USER_ACTIVE,
                CREDENTIAL_RULE_SINGLE,
                0,
                0,  // Expiring Timeout Minutes
                ENCODING_ASCII,
                0};  // User Name Length
    }

    std::vector<uint8_t> credential_report(uint16_t user_identifier, uint8_t credential_type, uint16_t credential_slot, uint8_t next_credential_type, uint16_t next_credential_slot)
    {
        std::vector<uint8_t> report {USER_CREDENTIAL_CC,
                                     FRAME_CREDENTIAL_REPORT,
                                     REPORT_RESPONSE_TO_GET,
                                     msb(user_identifier),
                                     lsb(user_identifier),
                                     credential_type,
                                     msb(credential_slot),
                                     lsb(credential_slot),
                                     0,  // Credential Properties
                                     static_cast<uint8_t>(FOUR_DIGIT_PIN.size())};
        report.insert(report.end(), FOUR_DIGIT_PIN.begin(), FOUR_DIGIT_PIN.end());
        report.insert(report.end(),
                      {0,  // Credential Modifier Type: DNE
                       0,
                       0,  // Credential Modifier Node ID
                       next_credential_type,
                       msb(next_credential_slot),
                       lsb(next_credential_slot)});
        return report;
    }

    template<typename AttributeEnum, typename Value> attribute_store::attribute desire(attribute_store::attribute group, AttributeEnum type, Value value)
    {
        auto node = group.emplace_node(type_of(type));
        node.set_desired(value);
        return node;
    }

    template<typename Value, typename AttributeEnum> Value reported(attribute_store::attribute group, AttributeEnum type)
    {
        return group.child_by_type(type_of(type)).template reported<Value>();
    }

    template<typename AttributeEnum, typename Value> size_t groups_reporting(attribute_store::attribute endpoint, attribute_store_type_t group_type, AttributeEnum attribute_type, Value value)
    {
        size_t count = 0;
        for (auto group: endpoint.children(group_type)) {
            auto attribute = group.child_by_type(type_of(attribute_type));
            if (attribute.is_valid() && attribute.reported_exists() && attribute.template reported<Value>() == value) {
                ++count;
            }
        }
        return count;
    }

    template<typename AttributeEnum> size_t groups_without_reported(attribute_store::attribute endpoint, attribute_store_type_t group_type, AttributeEnum attribute_type)
    {
        size_t count = 0;
        for (auto group: endpoint.children(group_type)) {
            auto attribute = group.child_by_type(type_of(attribute_type));
            if (!attribute.is_valid() || !attribute.reported_exists()) {
                ++count;
            }
        }
        return count;
    }

    using namespace user_credential_test;
    using rules = command_class_user_credential_rules;

    class UserCredentialCommandClassTest : public ::testing::Test
    {
        protected:
            static command_class_user_credential &component()
            {
                static command_class_user_credential instance;
                return instance;
            }

            void SetUp() override
            {
                ASSERT_EQ(SL_STATUS_OK, attribute_store_init());
                auto home = attribute_store::attribute::root().emplace_node(ATTRIBUTE_HOME_ID, std::vector<uint8_t>(TEST_HOME_ID.begin(), TEST_HOME_ID.end()));
                auto node = home.emplace_node(ATTRIBUTE_NODE_ID, TEST_NODE_ID);
                endpoint  = node.emplace_node(ATTRIBUTE_ENDPOINT_ID, TEST_ENDPOINT_ID);
                (void)component();
                register_byte_array_attributes();
            }

            // Interview state is kept per endpoint node id, and node ids restart after teardown.
            void TearDown() override
            {
                component_connector connector;
                const component_connector_cc_interview_action_payload_t cancel {static_cast<attribute_store_node_t>(endpoint), component_connector_cc_interview_action_t::cancel, nullptr};
                EXPECT_EQ(SL_STATUS_OK, run_until_done(connector.fire_event_async(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_CC_INTERVIEW_ACTION_REQUESTED), cancel)));
                attribute_resolver_teardown();
                attribute_store_teardown();
            }

            // Earlier reports queue a user-count event on the same connector queue. Drain until this event completes.
            static sl_status_t run_until_done(std::future<sl_status_t> status)
            {
                component_connector connector;
                while (status.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
                    connector.run();
                }
                return status.get();
            }

            void set_peer_version(attribute_store::attribute target, uint8_t version)
            {
                target.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_USER_CREDENTIAL)).set_reported(version);
            }

            void advertise(attribute_store::attribute target, std::vector<uint8_t> command_classes)
            {
                target.emplace_node(ATTRIBUTE_ZWAVE_NIF, command_classes);
            }

            void open_interview(attribute_store::attribute target)
            {
                component_connector connector;
                const component_connector_interview_done_payload_t payload {static_cast<attribute_store_node_t>(target), SL_STATUS_OK};
                ASSERT_EQ(SL_STATUS_OK, run_until_done(connector.fire_event_async(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_INTERVIEW_DONE), payload)));
            }

            // A version 1 interview with no checksum support reaches the user walk after the two capability reports.
            void start_user_walk(uint8_t user_properties = ASCII_SUPPORT)
            {
                set_peer_version(endpoint, VERSION_1);
                advertise(endpoint, {USER_CREDENTIAL_CC});
                open_interview(endpoint);
                ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities(user_properties)));
                ASSERT_EQ(SL_STATUS_OK, receive(pin_credential_capabilities()));
            }

            void store_pin_and_user_capabilities(uint8_t credential_properties = 0)
            {
                ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));
                ASSERT_EQ(SL_STATUS_OK, receive(pin_credential_capabilities(credential_properties)));
            }

            sl_status_t receive(const std::vector<uint8_t> &frame)
            {
                return receive(frame, endpoint);
            }

            sl_status_t receive(const std::vector<uint8_t> &frame, attribute_store::attribute target)
            {
                zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
                command_class_user_credential_core::report_received_args args {target, parser, nullptr};
                switch (frame[COMMAND_INDEX]) {
                    case FRAME_USER_CAPABILITIES_REPORT:
                        return component().on_user_capabilities_report_received(args);
                    case FRAME_CREDENTIAL_CAPABILITIES_REPORT:
                        return component().on_credential_capabilities_report_received(args);
                    case FRAME_USER_REPORT:
                        return component().on_user_report_received(args);
                    case FRAME_CREDENTIAL_REPORT:
                        return component().on_credential_report_received(args);
                    case FRAME_ALL_USERS_CHECKSUM_REPORT:
                        return component().on_all_users_checksum_report_received(args);
                    default:
                        return SL_STATUS_FAIL;
                }
            }

            sl_status_t assemble_set(attribute_store::attribute group, uint8_t command)
            {
                zwave_frame_generator generator {USER_CREDENTIAL_CC};
                frame_length = 0;
                generator.initialize_frame(command, frame, &frame_length);
                command_class_user_credential_core::set_requested_args args {group, &generator};
                switch (command) {
                    case FRAME_USER_SET:
                        return component().on_user_set_requested(args, frame, &frame_length);
                    case FRAME_CREDENTIAL_SET:
                        return component().on_credential_set_requested(args, frame, &frame_length);
                    case FRAME_ASSOCIATION_SET:
                        return component().on_user_credential_association_set_requested(args, frame, &frame_length);
                    case FRAME_ADMIN_PIN_SET:
                        return component().on_admin_pin_code_set_requested(args, frame, &frame_length);
                    default:
                        return SL_STATUS_FAIL;
                }
            }

            sl_status_t assemble_get(attribute_store::attribute group, uint8_t command)
            {
                zwave_frame_generator generator {USER_CREDENTIAL_CC};
                frame_length = 0;
                generator.initialize_frame(command, frame, &frame_length);
                command_class_user_credential_core::get_requested_args args {group, &generator};
                switch (command) {
                    case FRAME_USER_GET:
                        return component().on_user_get_requested(args, frame, &frame_length);
                    case FRAME_CREDENTIAL_GET:
                        return component().on_credential_get_requested(args, frame, &frame_length);
                    case FRAME_ALL_USERS_CHECKSUM_GET:
                        return component().on_all_users_checksum_get_requested(args, frame, &frame_length);
                    case FRAME_USER_CHECKSUM_GET:
                        return component().on_user_checksum_get_requested(args, frame, &frame_length);
                    case FRAME_CREDENTIAL_CHECKSUM_GET:
                        return component().on_credential_checksum_get_requested(args, frame, &frame_length);
                    case FRAME_ADMIN_PIN_GET:
                        return component().on_admin_pin_code_get_requested(args, frame, &frame_length);
                    default:
                        return SL_STATUS_FAIL;
                }
            }

            sl_status_t run_set_rule(attribute_store_type_t group_type, attribute_store::attribute group)
            {
                auto rule = attribute_resolver::set_function(group_type);
                if (rule == nullptr) {
                    return SL_STATUS_NOT_FOUND;
                }
                frame_length = FRAME_BUFFER_SIZE;
                return rule(group, frame, &frame_length);
            }

            attribute_store::attribute complete_user_set(uint8_t user_type, uint16_t timeout)
            {
                auto group = endpoint.emplace_node(USER_SET_GROUP);
                desire(group, user_set_group_attributes_t::operation_type, OPERATION_ADD);
                desire(group, user_set_group_attributes_t::user_unique_identifier, USER_IDENTIFIER_1);
                desire(group, user_set_group_attributes_t::user_type, user_type);
                desire(group, user_set_group_attributes_t::user_active_state, USER_ACTIVE);
                desire(group, user_set_group_attributes_t::credential_rule, CREDENTIAL_RULE_SINGLE);
                desire(group, user_set_group_attributes_t::expiring_timeout_minutes, timeout);
                desire(group, user_set_group_attributes_t::user_name_encoding, ENCODING_ASCII);
                desire(group, user_set_group_attributes_t::user_name_length, static_cast<uint8_t>(0));
                return group;
            }

            attribute_store::attribute complete_credential_set(uint8_t type, const std::vector<uint8_t> &data)
            {
                auto group = endpoint.emplace_node(CREDENTIAL_SET_GROUP);
                desire(group, credential_set_group_attributes_t::user_unique_identifier, USER_IDENTIFIER_1);
                desire(group, credential_set_group_attributes_t::credential_type, type);
                desire(group, credential_set_group_attributes_t::credential_slot, SLOT_1);
                desire(group, credential_set_group_attributes_t::operation_type, OPERATION_ADD);
                desire(group, credential_set_group_attributes_t::credential_length, static_cast<uint8_t>(data.size()));
                desire(group, credential_set_group_attributes_t::credential_data, data);
                return group;
            }

            bool pin_set_is_valid(uint16_t user, uint8_t type, uint16_t slot, uint8_t operation, const std::vector<uint8_t> &data)
            {
                return rules::credential_set_is_valid(endpoint, user, type, slot, operation, static_cast<uint8_t>(data.size()), data);
            }

            bool user_add_is_valid(uint16_t user, uint8_t user_type, uint8_t state, uint8_t rule, uint16_t timeout, uint8_t encoding, const std::vector<uint8_t> &name)
            {
                return rules::user_set_is_valid(endpoint, OPERATION_ADD, user, user_type, state, rule, timeout, encoding, static_cast<uint8_t>(name.size()), name);
            }

            attribute_store::attribute endpoint;
            uint8_t frame[FRAME_BUFFER_SIZE] {};
            uint16_t frame_length = 0;
    };

    // Spec 2026B checksum of 0x0000 means the node has no users (CL:0083.01.22.03.1, CC:0083.01.15.11.010).
    // A zero All Users Checksum skips the database walk.
    TEST_F(UserCredentialCommandClassTest, ZeroAllUsersChecksumSkipsUserWalk)
    {
        start_user_walk(static_cast<uint8_t>(ASCII_SUPPORT | ALL_USERS_CHECKSUM_SUPPORT));
        ASSERT_TRUE(endpoint.child_by_type(CHECKSUM_GET_GROUP).is_valid());

        ASSERT_EQ(SL_STATUS_OK, receive(all_users_checksum_report(CHECKSUM_NONE)));

        EXPECT_FALSE(endpoint.child_by_type(USER_GET_GROUP).is_valid());
    }

    // Spec 2026B non-zero All Users Checksum starts the user and credential walk (CL:0083.01.22.03.1).
    // A non-zero All Users Checksum starts the user walk.
    TEST_F(UserCredentialCommandClassTest, NonZeroAllUsersChecksumStartsUserWalk)
    {
        start_user_walk(static_cast<uint8_t>(ASCII_SUPPORT | ALL_USERS_CHECKSUM_SUPPORT));

        ASSERT_EQ(SL_STATUS_OK, receive(all_users_checksum_report(CHECKSUM_PRESENT)));

        EXPECT_TRUE(endpoint.child_by_type(USER_GET_GROUP).is_valid());
    }

    // Spec 2026B zero supported users means User Codes are in use (CL:0083.01.21.00.4, CC:0083.02.02.21.002).
    // Both user-management command classes and zero users make User Credential inactive after the user count.
    TEST_F(UserCredentialCommandClassTest, BothCommandClassesAndZeroUsersStopAfterUserCount)
    {
        advertise(endpoint, {USER_CREDENTIAL_CC, USER_CODE_CC});
        open_interview(endpoint);

        ASSERT_EQ(SL_STATUS_OK, receive(user_capabilities(NO_USERS, SINGLE_RULE_BIT, ASCII_SUPPORT, GENERAL_USER_BIT)));

        EXPECT_FALSE(endpoint.child_by_type(CRED_CAP_GET_GROUP).is_valid());
        EXPECT_EQ(NO_USERS, reported<uint16_t>(endpoint.child_by_type(USER_CAP_REPORT_GROUP), user_capabilities_report_group_attributes_t::number_of_supported_user_unique_identifiers));
    }

    // Spec 2026B a non-zero user count keeps User Credential in control (CL:0083.01.21.00.4).
    // The Library requirement describes this cross-CC handoff; this test checks that the interview stays open.
    // User Code plus a non-zero user count keeps the User Credential interview going.
    TEST_F(UserCredentialCommandClassTest, BothCommandClassesAndUsersContinuePastTheUserCount)
    {
        advertise(endpoint, {USER_CREDENTIAL_CC, USER_CODE_CC});
        open_interview(endpoint);

        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));

        EXPECT_TRUE(endpoint.child_by_type(CRED_CAP_GET_GROUP).is_valid());
    }

    // The specification defines no requirement for this handoff. ZPC publishes the reported count so User Code can apply CL:0083.01.21.00.4.
    // User Capabilities publishes the supported user count for User Code.
    TEST_F(UserCredentialCommandClassTest, UserCapabilitiesReportPublishesTheSupportedUserCount)
    {
        auto published = std::make_shared<uint16_t>(0);
        component_connector connector;
        connector.connect_typed<command_class_user_credential_events_t, user_credential_supported_user_count_payload_t>(command_class_user_credential_events_t::COMMAND_CLASS_USER_CREDENTIAL_SUPPORTED_USER_COUNT, [published](const user_credential_supported_user_count_payload_t &payload) {
            *published = payload.supported_user_count;
            return SL_STATUS_OK;
        });

        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));
        for (uint8_t drain = 0; drain < MAX_CONNECTOR_EVENT_DRAIN_PASSES && *published == 0; ++drain) {
            connector.run();
        }

        EXPECT_EQ(TWO_USERS, *published);
    }

    // Spec 2026B User Schedule support hands the user to Active Schedule (CC:0083.02.02.11.001, CC:0083.01.02.11.009).
    // User Schedule support publishes the Active Schedule handoff for each interviewed user.
    TEST_F(UserCredentialCommandClassTest, UserReportPublishesScheduleHandoff)
    {
        start_user_walk(static_cast<uint8_t>(ASCII_SUPPORT | USER_SCHEDULE_SUPPORT));
        auto schedule_seen = std::make_shared<bool>(false);
        component_connector connector;
        connector.connect_typed<command_class_user_credential_events_t, user_credential_interview_user_schedule_payload_t>(command_class_user_credential_events_t::COMMAND_CLASS_USER_CREDENTIAL_INTERVIEW_USER_SCHEDULE,
                                                                                                                           [schedule_seen](const user_credential_interview_user_schedule_payload_t &payload) {
                                                                                                                               *schedule_seen = payload.user_unique_identifier == USER_IDENTIFIER_1 && payload.target_command_class == USER_CREDENTIAL_CC;
                                                                                                                               return SL_STATUS_OK;
                                                                                                                           });

        ASSERT_EQ(SL_STATUS_OK, receive(user_report(USER_IDENTIFIER_1, 0)));
        for (uint8_t drain = 0; drain < MAX_CONNECTOR_EVENT_DRAIN_PASSES && !*schedule_seen; ++drain) {
            connector.run();
        }

        EXPECT_TRUE(*schedule_seen);
    }

    // Spec 2026B User Schedule handoff applies to an actual user (CC:0083.02.02.11.001, CC:0083.01.02.11.009).
    // The no-user identifier 0 does not publish an Active Schedule handoff.
    TEST_F(UserCredentialCommandClassTest, UserIdentifierZeroDoesNotPublishScheduleHandoff)
    {
        start_user_walk(static_cast<uint8_t>(ASCII_SUPPORT | USER_SCHEDULE_SUPPORT));
        auto schedule_seen = std::make_shared<bool>(false);
        component_connector connector;
        connector.connect_typed<command_class_user_credential_events_t, user_credential_interview_user_schedule_payload_t>(command_class_user_credential_events_t::COMMAND_CLASS_USER_CREDENTIAL_INTERVIEW_USER_SCHEDULE, [schedule_seen](const user_credential_interview_user_schedule_payload_t &) {
            *schedule_seen = true;
            return SL_STATUS_OK;
        });

        ASSERT_EQ(SL_STATUS_OK, receive(user_report(0, 0)));
        for (uint8_t drain = 0; drain < MAX_CONNECTOR_EVENT_DRAIN_PASSES; ++drain) {
            connector.run();
        }

        EXPECT_FALSE(*schedule_seen);
        EXPECT_FALSE(endpoint.child_by_type(CREDENTIAL_GET_GROUP).is_valid());
    }

    // Spec 2026B Credential Get with a user and type 0, slot 0 returns that user's first credential (CC:0083.01.0B.11.005).
    // Each User Report is followed by Credential Get for that user.
    TEST_F(UserCredentialCommandClassTest, UserReportRequestsTheFirstCredentialOfThatUser)
    {
        start_user_walk();

        ASSERT_EQ(SL_STATUS_OK, receive(user_report(USER_IDENTIFIER_1, 0)));

        const auto get = endpoint.child_by_type(CREDENTIAL_GET_GROUP);
        ASSERT_TRUE(get.is_valid());
        EXPECT_EQ(USER_IDENTIFIER_1, get.child_by_type(type_of(credential_get_group_attributes_t::user_unique_identifier)).desired<uint16_t>());
    }

    // Spec 2026B a User Get for identifier 0 with no users returns identifier 0 (CC:0083.01.06.11.005).
    // User identifier 0 means there is no user, so the walk does not ask for a credential.
    TEST_F(UserCredentialCommandClassTest, UserIdentifierZeroDoesNotRequestACredential)
    {
        start_user_walk();

        ASSERT_EQ(SL_STATUS_OK, receive(user_report(0, 0)));

        EXPECT_FALSE(endpoint.child_by_type(CREDENTIAL_GET_GROUP).is_valid());
    }

    // Spec 2026B a non-zero Next Credential continues the credential walk (CC:0083.01.0C.11.010).
    // A credential report whose next slot is set continues the walk at that slot.
    TEST_F(UserCredentialCommandClassTest, CredentialReportContinuesAtNextSlot)
    {
        start_user_walk();
        ASSERT_EQ(SL_STATUS_OK, receive(user_report(USER_IDENTIFIER_1, USER_IDENTIFIER_2)));

        ASSERT_EQ(SL_STATUS_OK, receive(credential_report(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_1, CREDENTIAL_TYPE_PIN, SLOT_2)));

        EXPECT_EQ(SLOT_2, endpoint.child_by_type(CREDENTIAL_GET_GROUP).child_by_type(type_of(credential_get_group_attributes_t::credential_slot)).desired<uint16_t>());
    }

    // Spec 2026B Next Credential 0 ends that user, and Next User continues the walk (CC:0083.01.0C.11.008, CC:0083.01.07.11.000).
    // A credential report with no further credential continues at the next user.
    TEST_F(UserCredentialCommandClassTest, CredentialReportWithNoNextCredentialRequestsTheNextUser)
    {
        start_user_walk();
        ASSERT_EQ(SL_STATUS_OK, receive(user_report(USER_IDENTIFIER_1, USER_IDENTIFIER_2)));

        ASSERT_EQ(SL_STATUS_OK, receive(credential_report(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_1, CREDENTIAL_TYPE_NONE, SLOT_0)));

        EXPECT_EQ(USER_IDENTIFIER_2, endpoint.child_by_type(USER_GET_GROUP).child_by_type(type_of(user_get_group_attributes_t::user_unique_identifier)).desired<uint16_t>());
    }

    // Spec 2026B credential slots are unique per credential type, so each slot is stored (CC:0083.01.0A.11.005).
    // A user's second credential is stored next to the first one.
    TEST_F(UserCredentialCommandClassTest, SecondCredentialOfAUserKeepsTheFirst)
    {
        start_user_walk();
        ASSERT_EQ(SL_STATUS_OK, receive(user_report(USER_IDENTIFIER_1, 0)));
        ASSERT_EQ(SL_STATUS_OK, receive(credential_report(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_1, CREDENTIAL_TYPE_PIN, SLOT_2)));

        ASSERT_EQ(SL_STATUS_OK, receive(credential_report(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_2, CREDENTIAL_TYPE_NONE, SLOT_0)));

        EXPECT_EQ(1u, groups_reporting(endpoint, CREDENTIAL_REPORT_GROUP, credential_report_group_attributes_t::credential_slot, SLOT_1));
        EXPECT_EQ(1u, groups_reporting(endpoint, CREDENTIAL_REPORT_GROUP, credential_report_group_attributes_t::credential_slot, SLOT_2));
    }

    // Spec 2026B credential slots are unique per credential type, so each slot is stored (CC:0083.01.0A.11.005).
    // The report for a user's second credential answers the group the walk asked for, so the interview can finish.
    TEST_F(UserCredentialCommandClassTest, SecondCredentialOfAUserAnswersTheRequestedGroup)
    {
        start_user_walk();
        ASSERT_EQ(SL_STATUS_OK, receive(user_report(USER_IDENTIFIER_1, 0)));
        ASSERT_EQ(SL_STATUS_OK, receive(credential_report(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_1, CREDENTIAL_TYPE_PIN, SLOT_2)));

        ASSERT_EQ(SL_STATUS_OK, receive(credential_report(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_2, CREDENTIAL_TYPE_NONE, SLOT_0)));

        EXPECT_EQ(0u, groups_without_reported(endpoint, CREDENTIAL_REPORT_GROUP, credential_report_group_attributes_t::next_credential_type));
    }

    // Spec 2026B a new interview reads the users again (CL:0083.01.21.01.1).
    // The Library requirement defines the repeated interview behavior; this test checks that a new response answers the new request.
    // A user stored by an earlier interview does not take the report the new walk asked for.
    TEST_F(UserCredentialCommandClassTest, ReinterviewWalkAnswersTheRequestedUserGroup)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(user_report(USER_IDENTIFIER_1, 0)));
        start_user_walk();

        ASSERT_EQ(SL_STATUS_OK, receive(user_report(USER_IDENTIFIER_1, 0)));

        EXPECT_EQ(1u, groups_reporting(endpoint, USER_REPORT_GROUP, user_report_group_attributes_t::user_unique_identifier, USER_IDENTIFIER_1));
        EXPECT_EQ(0u, groups_without_reported(endpoint, USER_REPORT_GROUP, user_report_group_attributes_t::next_user_unique_identifier));
    }

    // Spec 2026B the user walk replaces the previously read users (CL:0083.01.22.03.1).
    // The Library requirement describes the user walk; this test checks that its new results replace stale users.
    // A new user walk drops the users stored by an earlier interview.
    TEST_F(UserCredentialCommandClassTest, ReinterviewForgetsUsersThatAreNotReportedAgain)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(user_report(USER_IDENTIFIER_2, 0)));

        start_user_walk();

        EXPECT_EQ(0u, groups_reporting(endpoint, USER_REPORT_GROUP, user_report_group_attributes_t::user_unique_identifier, USER_IDENTIFIER_2));
    }

    // Spec 2026B the user walk replaces the previously read credentials (CL:0083.01.22.03.1).
    // The Library requirement describes the user walk; this test checks that its new results replace stale credentials.
    // A new user walk drops the credentials stored by an earlier interview.
    TEST_F(UserCredentialCommandClassTest, ReinterviewForgetsStoredCredentials)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(credential_report(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_1, CREDENTIAL_TYPE_NONE, SLOT_0)));

        start_user_walk();

        EXPECT_FALSE(endpoint.child_by_type(CREDENTIAL_REPORT_GROUP).is_valid());
    }

    // The specification defines no requirement for a report received outside an interview. The controller stores it and does not continue the walk.
    // Database reports that arrive without an open interview do not continue the user walk.
    TEST_F(UserCredentialCommandClassTest, DatabaseReportsOutsideTheInterviewSendNoGets)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(all_users_checksum_report(CHECKSUM_PRESENT)));
        ASSERT_EQ(SL_STATUS_OK, receive(user_report(USER_IDENTIFIER_1, USER_IDENTIFIER_2)));
        EXPECT_FALSE(endpoint.child_by_type(CREDENTIAL_GET_GROUP).is_valid());
        ASSERT_EQ(SL_STATUS_OK, receive(credential_report(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_1, CREDENTIAL_TYPE_PIN, SLOT_2)));

        EXPECT_FALSE(endpoint.child_by_type(USER_GET_GROUP).child_by_type(type_of(user_get_group_attributes_t::user_unique_identifier)).is_valid());
        EXPECT_FALSE(endpoint.child_by_type(CREDENTIAL_GET_GROUP).child_by_type(type_of(credential_get_group_attributes_t::credential_slot)).is_valid());
    }

    // Spec 2026B All Users Checksum (CC:0083.01.15.11.000).
    // An All Users Checksum Report outside the interview is still stored.
    TEST_F(UserCredentialCommandClassTest, AllUsersChecksumReportOutsideTheInterviewIsStored)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(all_users_checksum_report(CHECKSUM_PRESENT)));

        EXPECT_EQ(CHECKSUM_PRESENT, reported<uint16_t>(endpoint.child_by_type(CHECKSUM_REPORT_GROUP), all_users_checksum_report_group_attributes_t::all_users_checksum));
    }

    // Spec 2026B a User Report for an existing user updates that user (CC:0083.01.07.11.007).
    // A second report for a stored user updates that user's group.
    TEST_F(UserCredentialCommandClassTest, UserReportForAStoredUserUpdatesItsGroup)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(user_report(USER_IDENTIFIER_1, 0)));

        ASSERT_EQ(SL_STATUS_OK, receive(user_report(USER_IDENTIFIER_1, USER_IDENTIFIER_2)));

        EXPECT_EQ(1u, groups_reporting(endpoint, USER_REPORT_GROUP, user_report_group_attributes_t::user_unique_identifier, USER_IDENTIFIER_1));
        EXPECT_EQ(1u, groups_reporting(endpoint, USER_REPORT_GROUP, user_report_group_attributes_t::next_user_unique_identifier, USER_IDENTIFIER_2));
    }

    // Spec 2026B each User Unique Identifier is a distinct user (CC:0083.01.05.11.004).
    // A report for a different user adds a group.
    TEST_F(UserCredentialCommandClassTest, UserReportForAnotherUserAddsAGroup)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(user_report(USER_IDENTIFIER_1, 0)));

        ASSERT_EQ(SL_STATUS_OK, receive(user_report(USER_IDENTIFIER_2, 0)));

        EXPECT_EQ(1u, groups_reporting(endpoint, USER_REPORT_GROUP, user_report_group_attributes_t::user_unique_identifier, USER_IDENTIFIER_2));
        EXPECT_EQ(1u, groups_reporting(endpoint, USER_REPORT_GROUP, user_report_group_attributes_t::user_unique_identifier, USER_IDENTIFIER_1));
    }

    // Spec 2026B a credential is identified by its user, type, and slot (CC:0083.01.0B.11.003).
    // A credential report for another slot of the same user adds a group.
    TEST_F(UserCredentialCommandClassTest, CredentialReportForAnotherSlotOfTheSameUserAddsAGroup)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(credential_report(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_1, CREDENTIAL_TYPE_NONE, SLOT_0)));

        ASSERT_EQ(SL_STATUS_OK, receive(credential_report(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_2, CREDENTIAL_TYPE_NONE, SLOT_0)));

        EXPECT_EQ(2u, groups_reporting(endpoint, CREDENTIAL_REPORT_GROUP, credential_report_group_attributes_t::user_unique_identifier, USER_IDENTIFIER_1));
    }

    // Spec 2026B a report for a credential already at that user, type, and slot updates it (CC:0083.01.0C.11.013).
    // A credential report for a stored credential updates that credential's group.
    TEST_F(UserCredentialCommandClassTest, CredentialReportForAStoredCredentialUpdatesItsGroup)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(credential_report(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_1, CREDENTIAL_TYPE_NONE, SLOT_0)));

        ASSERT_EQ(SL_STATUS_OK, receive(credential_report(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_1, CREDENTIAL_TYPE_PIN, SLOT_2)));

        EXPECT_EQ(1u, groups_reporting(endpoint, CREDENTIAL_REPORT_GROUP, credential_report_group_attributes_t::credential_slot, SLOT_1));
        EXPECT_EQ(1u, groups_reporting(endpoint, CREDENTIAL_REPORT_GROUP, credential_report_group_attributes_t::next_credential_slot, SLOT_2));
    }

    // Spec 2026B Credential Learn Steps Remaining (CC:0083.01.11.11.003).
    // Credential Learn Report fields are stored.
    TEST_F(UserCredentialCommandClassTest, CredentialLearnReportIsStored)
    {
        constexpr uint8_t STEPS_REMAINING = 2;
        command_class_user_credential_attribute_map_t learn;
        learn.insert({"credential_learn_status", static_cast<uint8_t>(1)});
        learn.insert({"user_unique_identifier", USER_IDENTIFIER_1});
        learn.insert({"credential_type", CREDENTIAL_TYPE_PIN});
        learn.insert({"credential_slot", SLOT_1});
        learn.insert({"credential_learn_steps_remaining", STEPS_REMAINING});

        ASSERT_EQ(SL_STATUS_OK, component().on_credential_learn_report_received_store(endpoint, learn));

        const auto group = endpoint.child_by_type(type_of(credential_learn_report_group_attributes_t::CREDENTIAL_LEARN_REPORT_GROUP));
        EXPECT_EQ(STEPS_REMAINING, reported<uint8_t>(group, credential_learn_report_group_attributes_t::credential_learn_steps_remaining));
    }

    // Spec 2026B User Credential Association status (CC:0083.01.13.11.000).
    // User Credential Association Report fields are stored.
    TEST_F(UserCredentialCommandClassTest, AssociationReportIsStored)
    {
        command_class_user_credential_attribute_map_t association;
        association.insert({"credential_type", CREDENTIAL_TYPE_PIN});
        association.insert({"credential_slot", SLOT_1});
        association.insert({"destination_user_unique_identifier", USER_IDENTIFIER_2});
        association.insert({"user_credential_association_status", static_cast<uint8_t>(0)});

        ASSERT_EQ(SL_STATUS_OK, component().on_user_credential_association_report_received_store(endpoint, association));

        const auto group = endpoint.child_by_type(type_of(user_credential_association_report_group_attributes_t::USER_CREDENTIAL_ASSOCIATION_REPORT_GROUP));
        EXPECT_EQ(USER_IDENTIFIER_2, reported<uint16_t>(group, user_credential_association_report_group_attributes_t::destination_user_unique_identifier));
    }

    // Spec 2026B the User Checksum is calculated per User Unique Identifier (CC:0083.01.17.11.000).
    // User Checksum Reports are stored per user.
    TEST_F(UserCredentialCommandClassTest, UserChecksumReportIsStoredPerUser)
    {
        command_class_user_credential_attribute_map_t first;
        first.insert({"user_unique_identifier", USER_IDENTIFIER_1});
        first.insert({"user_checksum", CHECKSUM_PRESENT});
        command_class_user_credential_attribute_map_t second;
        second.insert({"user_unique_identifier", USER_IDENTIFIER_2});
        second.insert({"user_checksum", CHECKSUM_OTHER});

        ASSERT_EQ(SL_STATUS_OK, component().on_user_checksum_report_received_store(endpoint, first));
        ASSERT_EQ(SL_STATUS_OK, component().on_user_checksum_report_received_store(endpoint, second));

        const auto group_type = type_of(user_checksum_report_group_attributes_t::USER_CHECKSUM_REPORT_GROUP);
        EXPECT_EQ(1u, groups_reporting(endpoint, group_type, user_checksum_report_group_attributes_t::user_checksum, CHECKSUM_PRESENT));
        EXPECT_EQ(1u, groups_reporting(endpoint, group_type, user_checksum_report_group_attributes_t::user_checksum, CHECKSUM_OTHER));
    }

    // Spec 2026B the Credential Checksum is calculated per Credential Type (CC:0083.01.19.11.000).
    // Credential Checksum Reports are stored per credential type.
    TEST_F(UserCredentialCommandClassTest, CredentialChecksumReportIsStoredPerType)
    {
        command_class_user_credential_attribute_map_t pin;
        pin.insert({"credential_type", CREDENTIAL_TYPE_PIN});
        pin.insert({"credential_checksum", CHECKSUM_PRESENT});
        command_class_user_credential_attribute_map_t password;
        password.insert({"credential_type", CREDENTIAL_TYPE_PASSWORD});
        password.insert({"credential_checksum", CHECKSUM_OTHER});

        ASSERT_EQ(SL_STATUS_OK, component().on_credential_checksum_report_received_store(endpoint, pin));
        ASSERT_EQ(SL_STATUS_OK, component().on_credential_checksum_report_received_store(endpoint, password));

        const auto group_type = type_of(credential_checksum_report_group_attributes_t::CREDENTIAL_CHECKSUM_REPORT_GROUP);
        EXPECT_EQ(1u, groups_reporting(endpoint, group_type, credential_checksum_report_group_attributes_t::credential_checksum, CHECKSUM_PRESENT));
        EXPECT_EQ(1u, groups_reporting(endpoint, group_type, credential_checksum_report_group_attributes_t::credential_checksum, CHECKSUM_OTHER));
    }

    // Spec 2026B Admin PIN Code Report carries the Admin Code (CC:0083.01.1C.11.007).
    // Admin PIN Code Report fields are stored.
    TEST_F(UserCredentialCommandClassTest, AdminPinCodeReportIsStored)
    {
        command_class_user_credential_attribute_map_t admin;
        admin.insert({"admin_pin_code_length", PIN_MIN});
        admin.insert({"admin_pin_code_operation_result", static_cast<uint8_t>(0)});
        admin.insert({"admin_code", FOUR_DIGIT_PIN});

        ASSERT_EQ(SL_STATUS_OK, component().on_admin_pin_code_report_received_store(endpoint, admin));

        const auto group = endpoint.child_by_type(type_of(admin_pin_code_report_group_attributes_t::ADMIN_PIN_CODE_REPORT_GROUP));
        EXPECT_EQ(FOUR_DIGIT_PIN, reported<std::vector<uint8_t>>(group, admin_pin_code_report_group_attributes_t::admin_code));
    }

    // Spec 2026B the credential type blocks follow Number of Supported Credential Types (CC:0083.01.04.11.012).
    // A Credential Capabilities Report without type lists stores no list.
    TEST_F(UserCredentialCommandClassTest, CredentialCapabilitiesWithoutListsStoreNoList)
    {
        ASSERT_EQ(SL_STATUS_OK, component().on_credential_capabilities_report_received_store(endpoint, {}));

        EXPECT_FALSE(endpoint.child_by_type(CRED_CAP_REPORT_GROUP).child_by_type(type_of(credential_capabilities_report_group_attributes_t::vg1_credential_type)).is_valid());
    }

    // The specification defines no requirement for which Node Info frame carries the advertisement. ZPC accepts the secure Node Info.
    // A secure Node Info frame is enough for the controller to see User Code.
    TEST_F(UserCredentialCommandClassTest, SecureNodeInfoAdvertisesUserCode)
    {
        endpoint.emplace_node(ATTRIBUTE_ZWAVE_SECURE_NIF, std::vector<uint8_t> {USER_CODE_CC});

        EXPECT_TRUE(rules::endpoint_advertises_command_class(endpoint, USER_CODE_CC));
        EXPECT_FALSE(rules::endpoint_advertises_command_class(endpoint, USER_CREDENTIAL_CC));
    }

    // Spec 2026B an Expiring user has a non-zero timeout (CL:0083.01.31.05.1, CC:0083.01.05.11.042).
    // User Set carries an expiring user when the timeout is non-zero.
    TEST_F(UserCredentialCommandClassTest, UserSetSendsExpiringUser)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(user_capabilities(TWO_USERS, SINGLE_RULE_BIT, ASCII_SUPPORT, PROGRAMMING_TO_EXPIRING_USER_BITS)));
        auto group = complete_user_set(USER_TYPE_EXPIRING, EXPIRING_MINUTES);

        ASSERT_EQ(SL_STATUS_OK, assemble_set(group, FRAME_USER_SET));

        EXPECT_EQ(FRAME_USER_SET, frame[COMMAND_INDEX]);
    }

    // Spec 2026B every user type except Expiring has a zero timeout (CC:0083.01.05.11.053).
    // A non-expiring user with a timeout is not a valid User Set.
    TEST_F(UserCredentialCommandClassTest, UserSetRejectsTimeoutOnGeneralUser)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));
        auto group = complete_user_set(USER_TYPE_GENERAL, EXPIRING_MINUTES);

        EXPECT_EQ(SL_STATUS_FAIL, assemble_set(group, FRAME_USER_SET));
    }

    // Spec 2026B an Expiring user has a non-zero timeout (CC:0083.01.05.11.042).
    // An expiring user without a timeout is not a valid User Set.
    TEST_F(UserCredentialCommandClassTest, UserSetRejectsExpiringUserWithoutTimeout)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(user_capabilities(TWO_USERS, SINGLE_RULE_BIT, ASCII_SUPPORT, PROGRAMMING_TO_EXPIRING_USER_BITS)));

        EXPECT_FALSE(user_add_is_valid(USER_IDENTIFIER_1, USER_TYPE_EXPIRING, USER_ACTIVE, CREDENTIAL_RULE_SINGLE, 0, ENCODING_ASCII, {}));
    }

    // Spec 2026B delete-all uses user identifier 0 (CL:0083.01.31.13.1, CC:0083.01.05.11.009).
    // Delete-all is a User Set with operation Delete and user identifier 0.
    TEST_F(UserCredentialCommandClassTest, UserSetAcceptsDeleteAll)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));

        EXPECT_TRUE(rules::user_set_is_valid(endpoint, OPERATION_DELETE, 0, 0, 0, 0, 0, 0, 0, {}));
    }

    // Spec 2026B deleting one user (CL:0083.01.31.11.1).
    // The detailed delete workflow is defined by the Library requirement; this test checks ZPC's command validation.
    // Deleting one user does not check the user fields.
    TEST_F(UserCredentialCommandClassTest, UserSetAcceptsDeleteOfOneUser)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));

        EXPECT_TRUE(rules::user_set_is_valid(endpoint, OPERATION_DELETE, USER_IDENTIFIER_1, 0, 0, 0, 0, 0, 0, {}));
    }

    // Spec 2026B operation types above Delete are reserved (CC:0083.01.05.11.003).
    // An operation type above Delete is not sent.
    TEST_F(UserCredentialCommandClassTest, UserSetRejectsOperationAboveDelete)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));

        EXPECT_FALSE(rules::user_set_is_valid(endpoint, OPERATION_UNDEFINED, USER_IDENTIFIER_1, USER_TYPE_GENERAL, USER_ACTIVE, CREDENTIAL_RULE_SINGLE, 0, ENCODING_ASCII, 0, {}));
    }

    // Spec 2026B user active states above Active are reserved (CC:0083.01.05.11.033).
    // A user active state above Active is not sent.
    TEST_F(UserCredentialCommandClassTest, UserSetRejectsActiveStateAboveActive)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));

        EXPECT_FALSE(user_add_is_valid(USER_IDENTIFIER_1, USER_TYPE_GENERAL, USER_STATE_UNDEFINED, CREDENTIAL_RULE_SINGLE, 0, ENCODING_ASCII, {}));
    }

    // Spec 2026B a user identifier above the advertised maximum is ignored (CC:0083.01.05.11.014).
    // A user identifier above the supported count is not sent.
    TEST_F(UserCredentialCommandClassTest, UserSetRejectsIdentifierAboveTheSupportedCount)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));

        EXPECT_FALSE(user_add_is_valid(USER_IDENTIFIER_OUT_OF_RANGE, USER_TYPE_GENERAL, USER_ACTIVE, CREDENTIAL_RULE_SINGLE, 0, ENCODING_ASCII, {}));
    }

    // Spec 2026B a credential rule the node did not advertise is ignored (CC:0083.01.05.11.039).
    // A credential rule the peer did not advertise is not sent.
    TEST_F(UserCredentialCommandClassTest, UserSetRejectsUnsupportedCredentialRule)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));

        EXPECT_FALSE(user_add_is_valid(USER_IDENTIFIER_1, USER_TYPE_GENERAL, USER_ACTIVE, CREDENTIAL_RULE_DUAL, 0, ENCODING_ASCII, {}));
    }

    // Spec 2026B the user name matches its encoding and length (CC:0083.01.05.11.049).
    // The user name length must match the name bytes.
    TEST_F(UserCredentialCommandClassTest, UserSetRejectsNameLengthThatDoesNotMatchTheName)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));

        EXPECT_FALSE(rules::user_set_is_valid(endpoint, OPERATION_ADD, USER_IDENTIFIER_1, USER_TYPE_GENERAL, USER_ACTIVE, CREDENTIAL_RULE_SINGLE, 0, ENCODING_ASCII, 1, {}));
    }

    // Spec 2026B a UTF-16 name fits an even length (CC:0083.01.05.11.049, CC:0083.01.02.11.019).
    // A UTF-16 name with an even length is accepted.
    TEST_F(UserCredentialCommandClassTest, UserSetAcceptsAnEvenLengthUtf16Name)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities(static_cast<uint8_t>(ASCII_SUPPORT | UTF_16_SUPPORT))));

        EXPECT_TRUE(user_add_is_valid(USER_IDENTIFIER_1, USER_TYPE_GENERAL, USER_ACTIVE, CREDENTIAL_RULE_SINGLE, 0, ENCODING_UTF_16, {0x00, DIGIT_1}));
    }

    // Spec 2026B a UTF-16 name fits an even length (CC:0083.01.05.11.049).
    // A UTF-16 name with an odd length is refused.
    TEST_F(UserCredentialCommandClassTest, UserSetRejectsAnOddLengthUtf16Name)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities(static_cast<uint8_t>(ASCII_SUPPORT | UTF_16_SUPPORT))));

        EXPECT_FALSE(user_add_is_valid(USER_IDENTIFIER_1, USER_TYPE_GENERAL, USER_ACTIVE, CREDENTIAL_RULE_SINGLE, 0, ENCODING_UTF_16, {0x00}));
    }

    // Spec 2026B a user name is not longer than the reported maximum (CC:0083.01.02.11.008).
    // A user name longer than the advertised maximum is refused.
    TEST_F(UserCredentialCommandClassTest, UserSetRejectsNameLongerThanTheMaximum)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));

        EXPECT_FALSE(user_add_is_valid(USER_IDENTIFIER_1, USER_TYPE_GENERAL, USER_ACTIVE, CREDENTIAL_RULE_SINGLE, 0, ENCODING_ASCII, std::vector<uint8_t>(USER_NAME_MAX + 1, DIGIT_1)));
    }

    // Spec 2026B user types outside the defined set are reserved (CC:0083.01.05.11.021).
    // A user type value the specification does not define is refused.
    TEST_F(UserCredentialCommandClassTest, UndefinedUserTypeIsRejected)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));

        EXPECT_FALSE(rules::user_type_is_supported(endpoint, USER_TYPE_UNDEFINED));
    }

    // Spec 2026B a user type the node did not advertise is ignored (CC:0083.01.05.11.030).
    // A defined user type outside the advertised mask is refused.
    TEST_F(UserCredentialCommandClassTest, UserTypeOutsideTheMaskIsRejected)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(user_capabilities(TWO_USERS, SINGLE_RULE_BIT, ASCII_SUPPORT, PROGRAMMING_TO_EXPIRING_USER_BITS)));

        EXPECT_FALSE(rules::user_type_is_supported(endpoint, USER_TYPE_REMOTE_ONLY));
    }

    // Spec 2026B a supported user type has its bit set (CC:0083.01.02.11.022).
    // Each user type with its bit in the mask is supported.
    TEST_F(UserCredentialCommandClassTest, UserTypesListedInTheMaskAreSupported)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(user_capabilities(TWO_USERS, SINGLE_RULE_BIT, ASCII_SUPPORT, PROGRAMMING_TO_EXPIRING_USER_BITS)));

        for (uint8_t user_type = USER_TYPE_PROGRAMMING; user_type <= USER_TYPE_EXPIRING; ++user_type) {
            EXPECT_TRUE(rules::user_type_is_supported(endpoint, user_type)) << "user type " << static_cast<int>(user_type);
        }
    }

    // The specification defines no requirement for a command sent before User Capabilities are stored. ZPC allows it.
    // Without User Capabilities, a general user with a single rule at any identifier is allowed.
    TEST_F(UserCredentialCommandClassTest, MissingUserCapabilitiesDoNotRefuseAUser)
    {
        EXPECT_TRUE(rules::user_type_is_supported(endpoint, USER_TYPE_GENERAL));
        EXPECT_TRUE(rules::credential_rule_is_supported(endpoint, CREDENTIAL_RULE_SINGLE));
        EXPECT_TRUE(rules::user_identifier_is_supported(endpoint, USER_IDENTIFIER_OUT_OF_RANGE, false));
    }

    // Spec 2026B credential rules outside Single to Triple are reserved (CC:0083.01.05.11.038).
    // Credential rules outside Single to Triple are refused.
    TEST_F(UserCredentialCommandClassTest, CredentialRulesOutsideSingleToTripleAreRejected)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(user_capabilities(TWO_USERS, static_cast<uint8_t>(SINGLE_RULE_BIT | DUAL_RULE_BIT | TRIPLE_RULE_BIT), ASCII_SUPPORT, GENERAL_USER_BIT)));

        EXPECT_FALSE(rules::credential_rule_is_supported(endpoint, CREDENTIAL_RULE_UNDEFINED));
        EXPECT_FALSE(rules::credential_rule_is_supported(endpoint, CREDENTIAL_RULE_ABOVE_TRIPLE));
    }

    // Spec 2026B credential rule bits (CC:0083.01.02.11.003).
    // Dual and Triple rules are supported when their bits are set.
    TEST_F(UserCredentialCommandClassTest, DualAndTripleRulesFollowTheMask)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(user_capabilities(TWO_USERS, static_cast<uint8_t>(SINGLE_RULE_BIT | DUAL_RULE_BIT | TRIPLE_RULE_BIT), ASCII_SUPPORT, GENERAL_USER_BIT)));

        EXPECT_TRUE(rules::credential_rule_is_supported(endpoint, CREDENTIAL_RULE_DUAL));
        EXPECT_TRUE(rules::credential_rule_is_supported(endpoint, CREDENTIAL_RULE_TRIPLE));
    }

    // Spec 2026B an advertised name encoding is accepted (CC:0083.01.02.11.025).
    // Extended ASCII is supported when its bit is set.
    TEST_F(UserCredentialCommandClassTest, ExtendedAsciiIsSupportedWhenAdvertised)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities(static_cast<uint8_t>(ASCII_SUPPORT | EXTENDED_ASCII_SUPPORT))));

        EXPECT_TRUE(rules::user_name_encoding_is_supported(endpoint, ENCODING_EXTENDED_ASCII));
    }

    // Spec 2026B name encodings outside the defined set are reserved (CC:0083.01.05.11.045).
    // A name encoding the specification does not define is refused.
    TEST_F(UserCredentialCommandClassTest, UndefinedNameEncodingIsRejected)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));

        EXPECT_FALSE(rules::user_name_encoding_is_supported(endpoint, ENCODING_UNDEFINED));
    }

    // Spec 2026B a user type the node did not advertise is ignored (CC:0083.01.05.11.030).
    // User Set refuses a defined type that is absent from the advertised type mask.
    TEST_F(UserCredentialCommandClassTest, UserSetRejectsUnadvertisedUserType)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));

        EXPECT_FALSE(user_add_is_valid(USER_IDENTIFIER_1, USER_TYPE_PROGRAMMING, USER_ACTIVE, CREDENTIAL_RULE_SINGLE, 0, ENCODING_ASCII, {}));
    }

    // Spec 2026B a name encoding not advertised by the node is ignored (CC:0083.01.02.11.025).
    // User Set refuses Extended ASCII when only ASCII support is advertised.
    TEST_F(UserCredentialCommandClassTest, UserSetRejectsUnadvertisedNameEncoding)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities(ASCII_SUPPORT)));

        EXPECT_FALSE(user_add_is_valid(USER_IDENTIFIER_1, USER_TYPE_GENERAL, USER_ACTIVE, CREDENTIAL_RULE_SINGLE, 0, ENCODING_EXTENDED_ASCII, {}));
    }

    // Spec 2026B user identifier 0 is invalid except for a delete (CC:0083.01.05.11.006).
    // User Checksum Get with user identifier 0 is not sent.
    TEST_F(UserCredentialCommandClassTest, UserChecksumGetRejectsIdentifierZero)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities(static_cast<uint8_t>(ASCII_SUPPORT | USER_CHECKSUM_SUPPORT))));

        EXPECT_FALSE(rules::user_checksum_get_is_supported(endpoint, 0));
    }

    // Spec 2026B a PIN is ASCII digits (CL:0083.01.31.14.1, CC:0083.01.0A.11.020).
    // PIN Credential Set accepts four digits inside the advertised length.
    TEST_F(UserCredentialCommandClassTest, CredentialSetSendsPin)
    {
        store_pin_and_user_capabilities();
        auto group = complete_credential_set(CREDENTIAL_TYPE_PIN, FOUR_DIGIT_PIN);

        ASSERT_EQ(SL_STATUS_OK, assemble_set(group, FRAME_CREDENTIAL_SET));

        EXPECT_EQ(FRAME_CREDENTIAL_SET, frame[COMMAND_INDEX]);
        EXPECT_EQ(DIGIT_1, frame[frame_length - FOUR_DIGIT_PIN.size()]);
    }

    // Spec 2026B a PIN is ASCII digits (CC:0083.01.0A.11.020).
    // A PIN byte outside 0-9 is rejected.
    TEST_F(UserCredentialCommandClassTest, CredentialSetRejectsNonDigitPin)
    {
        store_pin_and_user_capabilities();
        auto group = complete_credential_set(CREDENTIAL_TYPE_PIN, {NON_DIGIT, DIGIT_2, DIGIT_3, DIGIT_4});

        EXPECT_EQ(SL_STATUS_FAIL, assemble_set(group, FRAME_CREDENTIAL_SET));
    }

    // Spec 2026B operation types above Delete are reserved (CC:0083.01.0A.11.009).
    // An operation type above Delete is not sent.
    TEST_F(UserCredentialCommandClassTest, CredentialSetRejectsOperationAboveDelete)
    {
        store_pin_and_user_capabilities();

        EXPECT_FALSE(pin_set_is_valid(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_1, OPERATION_UNDEFINED, FOUR_DIGIT_PIN));
    }

    // Spec 2026B the credential data length matches the credential data (CC:0083.01.0A.11.014).
    // The credential length must match the data bytes.
    TEST_F(UserCredentialCommandClassTest, CredentialSetRejectsLengthThatDoesNotMatchTheData)
    {
        store_pin_and_user_capabilities();

        EXPECT_FALSE(rules::credential_set_is_valid(endpoint, USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_1, OPERATION_ADD, 2, FOUR_DIGIT_PIN));
    }

    // Spec 2026B deleting every credential of one type uses user 0 and slot 0 (CL:0083.01.31.25.1).
    // The Library requirement defines this delete form; the assertion documents ZPC's validation of the required identifiers.
    // Delete-all for one credential type uses user 0 and slot 0.
    TEST_F(UserCredentialCommandClassTest, CredentialSetAcceptsDeleteAllOfOneType)
    {
        store_pin_and_user_capabilities();

        EXPECT_TRUE(pin_set_is_valid(0, CREDENTIAL_TYPE_PIN, SLOT_0, OPERATION_DELETE, {}));
    }

    // Spec 2026B credential type 0 is reserved (CC:0083.01.0A.11.001).
    // Delete-all needs a non-zero credential type.
    TEST_F(UserCredentialCommandClassTest, CredentialSetDeleteAllRejectsTypeZero)
    {
        store_pin_and_user_capabilities();

        EXPECT_FALSE(pin_set_is_valid(0, CREDENTIAL_TYPE_NONE, SLOT_0, OPERATION_DELETE, {}));
    }

    // Spec 2026B credential data may be omitted when deleting (CC:0083.01.0A.13.016).
    // Delete-all carries no credential data.
    TEST_F(UserCredentialCommandClassTest, CredentialSetDeleteAllRejectsData)
    {
        store_pin_and_user_capabilities();

        EXPECT_FALSE(pin_set_is_valid(0, CREDENTIAL_TYPE_PIN, SLOT_0, OPERATION_DELETE, {DIGIT_1}));
    }

    // Spec 2026B a user identifier above the advertised maximum is ignored (CC:0083.01.05.11.014).
    // A user identifier above the supported count is not sent.
    TEST_F(UserCredentialCommandClassTest, CredentialSetRejectsUserAboveTheSupportedCount)
    {
        store_pin_and_user_capabilities();

        EXPECT_FALSE(pin_set_is_valid(USER_IDENTIFIER_OUT_OF_RANGE, CREDENTIAL_TYPE_PIN, SLOT_1, OPERATION_ADD, FOUR_DIGIT_PIN));
    }

    // Spec 2026B credential type 0 is reserved (CC:0083.01.0A.11.001).
    // Credential type 0 is not a credential.
    TEST_F(UserCredentialCommandClassTest, CredentialSetRejectsTypeZero)
    {
        store_pin_and_user_capabilities();

        EXPECT_FALSE(pin_set_is_valid(USER_IDENTIFIER_1, CREDENTIAL_TYPE_NONE, SLOT_1, OPERATION_ADD, FOUR_DIGIT_PIN));
    }

    // Spec 2026B a credential type the node did not advertise is ignored (CC:0083.01.0A.11.002).
    // A credential type missing from Credential Capabilities is not sent.
    TEST_F(UserCredentialCommandClassTest, CredentialSetRejectsUnlistedType)
    {
        store_pin_and_user_capabilities();

        EXPECT_FALSE(pin_set_is_valid(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PASSWORD, SLOT_1, OPERATION_ADD, FOUR_DIGIT_PIN));
    }

    // Spec 2026B slot 0 is ignored for an operation other than Delete (CC:0083.01.0A.11.005).
    // Slot 0 is only valid for delete-all.
    TEST_F(UserCredentialCommandClassTest, CredentialSetRejectsSlotZero)
    {
        store_pin_and_user_capabilities();

        EXPECT_FALSE(pin_set_is_valid(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_0, OPERATION_ADD, FOUR_DIGIT_PIN));
    }

    // Spec 2026B deleting one credential (CL:0083.01.31.17.1).
    // The Library requirement defines this delete form; the assertion documents ZPC's validation of the required identifiers.
    // Deleting one credential carries no data.
    TEST_F(UserCredentialCommandClassTest, CredentialSetAcceptsDeleteOfOneSlot)
    {
        store_pin_and_user_capabilities();

        EXPECT_TRUE(pin_set_is_valid(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_1, OPERATION_DELETE, {}));
    }

    // Spec 2026B credential data may be omitted when deleting (CC:0083.01.0A.13.016).
    // Delete of one credential with data is refused.
    TEST_F(UserCredentialCommandClassTest, CredentialSetRejectsDataOnDelete)
    {
        store_pin_and_user_capabilities();

        EXPECT_FALSE(pin_set_is_valid(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_1, OPERATION_DELETE, {DIGIT_1}));
    }

    // Spec 2026B credential data is not shorter than the reported minimum (CC:0083.01.0A.11.015).
    // A PIN shorter than the advertised minimum is refused.
    TEST_F(UserCredentialCommandClassTest, CredentialSetRejectsPinShorterThanTheStoredMinimum)
    {
        store_pin_and_user_capabilities();

        EXPECT_FALSE(pin_set_is_valid(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_1, OPERATION_ADD, {DIGIT_1, DIGIT_2, DIGIT_3}));
    }

    // Spec 2026B credential data is not longer than the reported maximum (CC:0083.01.0A.11.014).
    // A PIN longer than the advertised maximum is refused.
    TEST_F(UserCredentialCommandClassTest, CredentialSetRejectsPinLongerThanTheStoredMaximum)
    {
        store_pin_and_user_capabilities();

        EXPECT_FALSE(pin_set_is_valid(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_1, OPERATION_ADD, std::vector<uint8_t>(PIN_MAX + 1, DIGIT_1)));
    }

    // Spec 2026B a PIN is 4 to 10 digits (CC:0083.01.00.21.014, CC:0083.01.00.41.016).
    // Without Credential Capabilities, a PIN uses the four to ten digit default.
    TEST_F(UserCredentialCommandClassTest, PinWithoutCapabilitiesUsesTheDefaultLength)
    {
        EXPECT_FALSE(pin_set_is_valid(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_1, OPERATION_ADD, {DIGIT_1, DIGIT_2, DIGIT_3}));
        EXPECT_TRUE(pin_set_is_valid(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_1, OPERATION_ADD, FOUR_DIGIT_PIN));
    }

    // The specification defines no default length for a non-PIN type, so ZPC accepts it until Credential Capabilities are stored.
    // Without Credential Capabilities, a non-PIN type accepts any length.
    TEST_F(UserCredentialCommandClassTest, NonPinTypeWithoutCapabilitiesAcceptsAnyLength)
    {
        EXPECT_TRUE(pin_set_is_valid(USER_IDENTIFIER_1, CREDENTIAL_TYPE_PASSWORD, SLOT_1, OPERATION_ADD, {DIGIT_1}));
    }

    // The specification defines no requirement for a missing slot list. ZPC allows the slot until the list is stored.
    // An empty stored slot list does not refuse a slot.
    TEST_F(UserCredentialCommandClassTest, EmptySlotListAllowsAnySlot)
    {
        store_pin_and_user_capabilities();
        endpoint.child_by_type(CRED_CAP_REPORT_GROUP).child_by_type(type_of(credential_capabilities_report_group_attributes_t::vg3_number_of_supported_credential_slots_per_credential_type)).set_reported(std::vector<uint8_t> {});

        EXPECT_TRUE(rules::credential_slot_is_supported(endpoint, CREDENTIAL_TYPE_PIN, SLOT_OUT_OF_RANGE, false));
    }

    // Spec 2026B user, type, and slot all zero return the first credential (CC:0083.01.0B.11.011).
    // Credential Get with type 0 and slot 0 asks for the first credential.
    TEST_F(UserCredentialCommandClassTest, CredentialGetAcceptsTheFirstCredential)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));

        EXPECT_TRUE(rules::credential_get_is_valid(endpoint, 0, CREDENTIAL_TYPE_NONE, SLOT_0));
    }

    // Spec 2026B a non-zero user, type, and slot return that credential (CC:0083.01.0B.11.003).
    // Credential Get for a stored PIN slot is accepted.
    TEST_F(UserCredentialCommandClassTest, CredentialGetAcceptsAStoredPinSlot)
    {
        store_pin_and_user_capabilities();

        EXPECT_TRUE(rules::credential_get_is_valid(endpoint, USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_1));
    }

    // Spec 2026B a user identifier above the advertised maximum is ignored (CC:0083.01.05.11.014).
    // Credential Get for a user above the supported count is refused.
    TEST_F(UserCredentialCommandClassTest, CredentialGetRejectsUserAboveTheSupportedCount)
    {
        store_pin_and_user_capabilities();

        EXPECT_FALSE(rules::credential_get_is_valid(endpoint, USER_IDENTIFIER_OUT_OF_RANGE, CREDENTIAL_TYPE_PIN, SLOT_1));
    }

    // Spec 2026B a credential type the node did not advertise is ignored (CC:0083.01.0A.11.002).
    // Credential Get for an unlisted type is refused.
    TEST_F(UserCredentialCommandClassTest, CredentialGetRejectsUnlistedType)
    {
        store_pin_and_user_capabilities();

        EXPECT_FALSE(rules::credential_get_is_valid(endpoint, USER_IDENTIFIER_1, CREDENTIAL_TYPE_PASSWORD, SLOT_1));
    }

    // Spec 2026B a slot the node did not advertise is ignored (CC:0083.01.0A.11.004).
    // Credential Get for a slot above the stored count is refused.
    TEST_F(UserCredentialCommandClassTest, CredentialGetRejectsSlotAboveTheStoredCount)
    {
        store_pin_and_user_capabilities();

        EXPECT_FALSE(rules::credential_get_is_valid(endpoint, USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_OUT_OF_RANGE));
    }

    // Spec 2026B Credential Learn Start operation type (CC:0083.01.0F.11.008).
    // Credential Learn Start is assembled by the resolver rule for a supported PIN slot.
    TEST_F(UserCredentialCommandClassTest, CredentialLearnStartBuildsFrame)
    {
        set_peer_version(endpoint, VERSION_1);
        advertise(endpoint, {USER_CREDENTIAL_CC});
        store_pin_and_user_capabilities();
        auto group = endpoint.emplace_node(LEARN_START_GROUP);
        desire(group, credential_learn_start_group_attributes_t::user_unique_identifier, USER_IDENTIFIER_1);
        desire(group, credential_learn_start_group_attributes_t::credential_type, CREDENTIAL_TYPE_PIN);
        desire(group, credential_learn_start_group_attributes_t::credential_slot, SLOT_1);
        desire(group, credential_learn_start_group_attributes_t::operation_type, OPERATION_ADD);
        desire(group, credential_learn_start_group_attributes_t::credential_learn_timeout, LEARN_TIMEOUT);

        ASSERT_EQ(SL_STATUS_OK, run_set_rule(LEARN_START_GROUP, group));

        EXPECT_EQ(FRAME_CREDENTIAL_LEARN_START, frame[COMMAND_INDEX]);
    }

    // The specification defines no requirement for a partially filled command. The controller waits until every field is present.
    // Credential Learn Start waits until every field has a desired value.
    TEST_F(UserCredentialCommandClassTest, CredentialLearnStartIsNotReadyUntilEveryFieldIsPresent)
    {
        set_peer_version(endpoint, VERSION_1);
        advertise(endpoint, {USER_CREDENTIAL_CC});
        store_pin_and_user_capabilities();
        auto group = endpoint.emplace_node(LEARN_START_GROUP);

        EXPECT_EQ(SL_STATUS_NOT_READY, run_set_rule(LEARN_START_GROUP, group));
        desire(group, credential_learn_start_group_attributes_t::user_unique_identifier, USER_IDENTIFIER_1);
        EXPECT_EQ(SL_STATUS_NOT_READY, run_set_rule(LEARN_START_GROUP, group));
        desire(group, credential_learn_start_group_attributes_t::credential_type, CREDENTIAL_TYPE_PIN);
        EXPECT_EQ(SL_STATUS_NOT_READY, run_set_rule(LEARN_START_GROUP, group));
        desire(group, credential_learn_start_group_attributes_t::credential_slot, SLOT_1);
        EXPECT_EQ(SL_STATUS_NOT_READY, run_set_rule(LEARN_START_GROUP, group));
        desire(group, credential_learn_start_group_attributes_t::operation_type, OPERATION_ADD);
        EXPECT_EQ(SL_STATUS_NOT_READY, run_set_rule(LEARN_START_GROUP, group));
    }

    // The specification defines no requirement for a command to a node that does not advertise the Command Class. ZPC does not send it.
    // Credential Learn Start is refused when User Credential is not advertised.
    TEST_F(UserCredentialCommandClassTest, CredentialLearnStartIsRefusedWhenUserCredentialIsNotAdvertised)
    {
        auto group = endpoint.emplace_node(LEARN_START_GROUP);

        EXPECT_NE(SL_STATUS_OK, run_set_rule(LEARN_START_GROUP, group));
    }

    // Spec 2026B a user identifier the node did not advertise is ignored (CC:0083.01.0F.11.005).
    // Credential Learn Start for a user above the supported count is refused.
    TEST_F(UserCredentialCommandClassTest, CredentialLearnStartAssemblyRejectsUserAboveTheSupportedCount)
    {
        set_peer_version(endpoint, VERSION_1);
        advertise(endpoint, {USER_CREDENTIAL_CC});
        store_pin_and_user_capabilities();
        auto group = endpoint.emplace_node(LEARN_START_GROUP);
        desire(group, credential_learn_start_group_attributes_t::user_unique_identifier, USER_IDENTIFIER_OUT_OF_RANGE);
        desire(group, credential_learn_start_group_attributes_t::credential_type, CREDENTIAL_TYPE_PIN);
        desire(group, credential_learn_start_group_attributes_t::credential_slot, SLOT_1);
        desire(group, credential_learn_start_group_attributes_t::operation_type, OPERATION_ADD);
        desire(group, credential_learn_start_group_attributes_t::credential_learn_timeout, LEARN_TIMEOUT);

        EXPECT_EQ(SL_STATUS_FAIL, run_set_rule(LEARN_START_GROUP, group));
    }

    // Spec 2026B Credential Learn Start operation types above Modify are reserved (CC:0083.01.0F.11.010).
    // Credential Learn Start accepts Add and Modify only.
    TEST_F(UserCredentialCommandClassTest, CredentialLearnStartRejectsDelete)
    {
        store_pin_and_user_capabilities();

        EXPECT_FALSE(rules::credential_learn_start_is_valid(endpoint, USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_1, OPERATION_DELETE));
    }

    // Spec 2026B a credential type the node did not advertise is ignored (CC:0083.01.0F.11.003).
    // Credential Learn Start for an unlisted type is refused.
    TEST_F(UserCredentialCommandClassTest, CredentialLearnStartRejectsUnlistedType)
    {
        store_pin_and_user_capabilities();

        EXPECT_FALSE(rules::credential_learn_start_is_valid(endpoint, USER_IDENTIFIER_1, CREDENTIAL_TYPE_PASSWORD, SLOT_1, OPERATION_ADD));
    }

    // Spec 2026B a slot the node did not advertise is ignored (CC:0083.01.0F.11.006).
    // Credential Learn Start for a slot above the stored count is refused.
    TEST_F(UserCredentialCommandClassTest, CredentialLearnStartRejectsSlotAboveTheStoredCount)
    {
        store_pin_and_user_capabilities();

        EXPECT_FALSE(rules::credential_learn_start_is_valid(endpoint, USER_IDENTIFIER_1, CREDENTIAL_TYPE_PIN, SLOT_OUT_OF_RANGE, OPERATION_MODIFY));
    }

    // Spec 2026B Credential Learn Cancel ends the learn (CC:0083.01.10.11.001).
    // Credential Learn Cancel is assembled by its resolver rule.
    TEST_F(UserCredentialCommandClassTest, CredentialLearnCancelBuildsFrame)
    {
        advertise(endpoint, {USER_CREDENTIAL_CC});
        auto group = endpoint.emplace_node(LEARN_CANCEL_GROUP);

        ASSERT_EQ(SL_STATUS_OK, run_set_rule(LEARN_CANCEL_GROUP, group));

        EXPECT_EQ(FRAME_CREDENTIAL_LEARN_CANCEL, frame[COMMAND_INDEX]);
    }

    // The specification defines no requirement for a command to a node that does not advertise the Command Class. ZPC does not send it.
    // Credential Learn Cancel is refused when User Credential is not advertised.
    TEST_F(UserCredentialCommandClassTest, CredentialLearnCancelIsRefusedWhenUserCredentialIsNotAdvertised)
    {
        auto group = endpoint.emplace_node(LEARN_CANCEL_GROUP);

        EXPECT_NE(SL_STATUS_OK, run_set_rule(LEARN_CANCEL_GROUP, group));
    }

    // Spec 2026B the destination user is a non-zero existing user (CC:0083.01.12.11.003).
    // Association Set to a user above the supported count is refused.
    TEST_F(UserCredentialCommandClassTest, AssociationSetRejectsDestinationAboveTheSupportedCount)
    {
        store_pin_and_user_capabilities();

        EXPECT_FALSE(rules::user_credential_association_set_is_valid(endpoint, CREDENTIAL_TYPE_PIN, SLOT_1, USER_IDENTIFIER_OUT_OF_RANGE));
    }

    // Spec 2026B the credential type is one the node supports (CC:0083.01.12.11.004).
    // Association Set for an unlisted type is refused.
    TEST_F(UserCredentialCommandClassTest, AssociationSetRejectsUnlistedType)
    {
        store_pin_and_user_capabilities();

        EXPECT_FALSE(rules::user_credential_association_set_is_valid(endpoint, CREDENTIAL_TYPE_PASSWORD, SLOT_1, USER_IDENTIFIER_2));
    }

    // Spec 2026B the credential slot holds a credential (CC:0083.01.12.11.005).
    // Association Set for a slot above the stored count is refused.
    TEST_F(UserCredentialCommandClassTest, AssociationSetRejectsSlotAboveTheStoredCount)
    {
        store_pin_and_user_capabilities();

        EXPECT_FALSE(rules::user_credential_association_set_is_valid(endpoint, CREDENTIAL_TYPE_PIN, SLOT_OUT_OF_RANGE, USER_IDENTIFIER_2));
    }

    // Spec 2026B Admin PIN Code Get without Admin Code support (CC:0083.01.1B.13.001).
    // Admin PIN Get is refused when Admin Code Support is clear.
    TEST_F(UserCredentialCommandClassTest, AdminPinGetIsRefusedWithoutAdminCodeSupport)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(pin_credential_capabilities()));

        EXPECT_EQ(SL_STATUS_FAIL, component().mqtt_on_admin_pin_code_get_command(endpoint, "{}"));
    }

    // Spec 2026B Admin PIN Code Get returns the Admin PIN Code Report (CC:0083.01.1B.11.001).
    // Admin PIN Get is sent when Admin Code Support is set.
    TEST_F(UserCredentialCommandClassTest, AdminPinGetIsSentWithAdminCodeSupport)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(pin_credential_capabilities(ADMIN_CODE_SUPPORT)));

        EXPECT_EQ(SL_STATUS_OK, component().mqtt_on_admin_pin_code_get_command(endpoint, "{}"));
    }

    // Spec 2026B length 0 deactivates the Admin Code only when deactivation is supported (CC:0083.01.1A.13.003).
    // Admin PIN deactivation requires the deactivation bit.
    TEST_F(UserCredentialCommandClassTest, AdminPinSetRejectsDeactivationWithoutSupport)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(pin_credential_capabilities(ADMIN_CODE_SUPPORT)));

        EXPECT_EQ(SL_STATUS_FAIL, component().mqtt_on_admin_pin_code_set_command(endpoint, "{\"admin_pin_code_length\":0}"));
    }

    // Spec 2026B the Admin Code is ASCII within the PIN length (CL:0083.01.31.32.1, CC:0083.01.1A.11.008).
    // Admin PIN Set stores a supported digit code.
    TEST_F(UserCredentialCommandClassTest, AdminPinSetStoresDigits)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(pin_credential_capabilities(static_cast<uint8_t>(ADMIN_CODE_SUPPORT | ADMIN_DEACTIVATION_SUPPORT))));

        ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_admin_pin_code_set_command(endpoint, "{\"admin_pin_code_length\":4,\"admin_code\":[49,50,51,52]}"));

        EXPECT_EQ(FOUR_DIGIT_PIN, endpoint.child_by_type(ADMIN_SET_GROUP).child_by_type(type_of(admin_pin_code_set_group_attributes_t::admin_code)).desired<std::vector<uint8_t>>());
    }

    // Spec 2026B length 0 deactivates the Admin Code (CC:0083.01.1A.11.006).
    // Admin PIN deactivation stores length 0 when the peer supports it.
    TEST_F(UserCredentialCommandClassTest, MqttAdminPinSetStoresDeactivation)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(pin_credential_capabilities(static_cast<uint8_t>(ADMIN_CODE_SUPPORT | ADMIN_DEACTIVATION_SUPPORT))));

        ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_admin_pin_code_set_command(endpoint, "{\"admin_pin_code_length\":0}"));

        EXPECT_EQ(0, endpoint.child_by_type(ADMIN_SET_GROUP).child_by_type(type_of(admin_pin_code_set_group_attributes_t::admin_pin_code_length)).desired<uint8_t>());
    }

    // Spec 2026B Admin PIN Code Set without Admin Code support (CC:0083.01.1A.13.002).
    // Admin PIN Set is refused when Admin Code Support is clear.
    TEST_F(UserCredentialCommandClassTest, AdminPinSetIsRefusedWithoutAdminCodeSupport)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(pin_credential_capabilities()));

        EXPECT_FALSE(rules::admin_pin_code_set_is_valid(endpoint, PIN_MIN, FOUR_DIGIT_PIN));
    }

    // Spec 2026B the Admin Code length matches the Admin Code (CC:0083.01.1A.11.007).
    // The Admin PIN length must match the code bytes.
    TEST_F(UserCredentialCommandClassTest, AdminPinSetRejectsLengthThatDoesNotMatchTheCode)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(pin_credential_capabilities(ADMIN_CODE_SUPPORT)));

        EXPECT_FALSE(rules::admin_pin_code_set_is_valid(endpoint, PIN_MIN, {DIGIT_1, DIGIT_2, DIGIT_3}));
    }

    // Spec 2026B an unsupported character is ignored (CC:0083.01.1A.11.010).
    // An Admin PIN byte outside 0-9 is refused.
    TEST_F(UserCredentialCommandClassTest, AdminPinSetRejectsNonDigit)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(pin_credential_capabilities(ADMIN_CODE_SUPPORT)));

        EXPECT_FALSE(rules::admin_pin_code_set_is_valid(endpoint, 1, {NON_DIGIT}));
    }

    // Spec 2026B the credential type complies with the credential type table (CC:0083.01.18.11.004).
    // Credential Checksum Get for an unlisted type is refused.
    TEST_F(UserCredentialCommandClassTest, CredentialChecksumGetRejectsUnlistedType)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(pin_credential_capabilities(CREDENTIAL_CHECKSUM_SUPPORT)));

        EXPECT_FALSE(rules::credential_checksum_get_is_supported(endpoint, CREDENTIAL_TYPE_PASSWORD));
    }

    // Spec 2026B User Get returns the User Report (CC:0083.01.06.11.000).
    // User Get waits for the identifier, then builds the frame.
    TEST_F(UserCredentialCommandClassTest, UserGetIsAssembledOnceTheIdentifierIsPresent)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));
        auto group = endpoint.emplace_node(USER_GET_GROUP);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_get(group, FRAME_USER_GET));

        desire(group, user_get_group_attributes_t::user_unique_identifier, USER_IDENTIFIER_1);

        EXPECT_EQ(SL_STATUS_OK, assemble_get(group, FRAME_USER_GET));
    }

    // Spec 2026B a user identifier above the advertised maximum is ignored (CC:0083.01.05.11.014).
    // User Get for an identifier above the supported count is refused.
    TEST_F(UserCredentialCommandClassTest, UserGetAssemblyRejectsIdentifierAboveTheSupportedCount)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));
        auto group = endpoint.emplace_node(USER_GET_GROUP);
        desire(group, user_get_group_attributes_t::user_unique_identifier, USER_IDENTIFIER_OUT_OF_RANGE);

        EXPECT_EQ(SL_STATUS_FAIL, assemble_get(group, FRAME_USER_GET));
    }

    // Spec 2026B a non-zero user, type, and slot return that credential (CC:0083.01.0B.11.003).
    // Credential Get waits for user, type, and slot, then builds the frame.
    TEST_F(UserCredentialCommandClassTest, CredentialGetIsAssembledOnceEveryFieldIsPresent)
    {
        store_pin_and_user_capabilities();
        auto group = endpoint.emplace_node(CREDENTIAL_GET_GROUP);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_get(group, FRAME_CREDENTIAL_GET));
        desire(group, credential_get_group_attributes_t::user_unique_identifier, USER_IDENTIFIER_1);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_get(group, FRAME_CREDENTIAL_GET));
        desire(group, credential_get_group_attributes_t::credential_type, CREDENTIAL_TYPE_PIN);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_get(group, FRAME_CREDENTIAL_GET));

        desire(group, credential_get_group_attributes_t::credential_slot, SLOT_1);

        EXPECT_EQ(SL_STATUS_OK, assemble_get(group, FRAME_CREDENTIAL_GET));
    }

    // Spec 2026B a user identifier above the advertised maximum is ignored (CC:0083.01.05.11.014).
    // Credential Get for a user above the supported count is refused.
    TEST_F(UserCredentialCommandClassTest, CredentialGetAssemblyRejectsUserAboveTheSupportedCount)
    {
        store_pin_and_user_capabilities();
        auto group = endpoint.emplace_node(CREDENTIAL_GET_GROUP);
        desire(group, credential_get_group_attributes_t::user_unique_identifier, USER_IDENTIFIER_OUT_OF_RANGE);
        desire(group, credential_get_group_attributes_t::credential_type, CREDENTIAL_TYPE_PIN);
        desire(group, credential_get_group_attributes_t::credential_slot, SLOT_1);

        EXPECT_EQ(SL_STATUS_FAIL, assemble_get(group, FRAME_CREDENTIAL_GET));
    }

    // Spec 2026B adding a user (CL:0083.01.31.05.1).
    // The Library requirement defines the use case; this test covers ZPC's partial-group assembly behavior.
    // User Set waits for every fixed field, then builds the frame with the name.
    TEST_F(UserCredentialCommandClassTest, UserSetIsAssembledOnceEveryFieldIsPresent)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));
        auto group = endpoint.emplace_node(USER_SET_GROUP);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_set(group, FRAME_USER_SET));
        desire(group, user_set_group_attributes_t::operation_type, OPERATION_ADD);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_set(group, FRAME_USER_SET));
        desire(group, user_set_group_attributes_t::user_unique_identifier, USER_IDENTIFIER_1);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_set(group, FRAME_USER_SET));
        desire(group, user_set_group_attributes_t::user_type, USER_TYPE_GENERAL);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_set(group, FRAME_USER_SET));
        desire(group, user_set_group_attributes_t::user_active_state, USER_ACTIVE);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_set(group, FRAME_USER_SET));
        desire(group, user_set_group_attributes_t::credential_rule, CREDENTIAL_RULE_SINGLE);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_set(group, FRAME_USER_SET));
        desire(group, user_set_group_attributes_t::expiring_timeout_minutes, static_cast<uint16_t>(0));
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_set(group, FRAME_USER_SET));
        desire(group, user_set_group_attributes_t::user_name_encoding, ENCODING_ASCII);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_set(group, FRAME_USER_SET));

        desire(group, user_set_group_attributes_t::user_name_length, static_cast<uint8_t>(FOUR_DIGIT_PIN.size()));
        desire(group, user_set_group_attributes_t::user_name, FOUR_DIGIT_PIN);

        EXPECT_EQ(SL_STATUS_OK, assemble_set(group, FRAME_USER_SET));
    }

    // Spec 2026B adding a credential (CL:0083.01.31.14.1).
    // The Library requirement defines the use case; this test covers ZPC's partial-group assembly behavior.
    // Credential Set waits for every fixed field, then builds the frame with the data.
    TEST_F(UserCredentialCommandClassTest, CredentialSetIsAssembledOnceEveryFieldIsPresent)
    {
        store_pin_and_user_capabilities();
        auto group = endpoint.emplace_node(CREDENTIAL_SET_GROUP);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_set(group, FRAME_CREDENTIAL_SET));
        desire(group, credential_set_group_attributes_t::user_unique_identifier, USER_IDENTIFIER_1);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_set(group, FRAME_CREDENTIAL_SET));
        desire(group, credential_set_group_attributes_t::credential_type, CREDENTIAL_TYPE_PIN);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_set(group, FRAME_CREDENTIAL_SET));
        desire(group, credential_set_group_attributes_t::credential_slot, SLOT_1);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_set(group, FRAME_CREDENTIAL_SET));
        desire(group, credential_set_group_attributes_t::operation_type, OPERATION_ADD);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_set(group, FRAME_CREDENTIAL_SET));

        desire(group, credential_set_group_attributes_t::credential_length, static_cast<uint8_t>(FOUR_DIGIT_PIN.size()));
        desire(group, credential_set_group_attributes_t::credential_data, FOUR_DIGIT_PIN);

        EXPECT_EQ(SL_STATUS_OK, assemble_set(group, FRAME_CREDENTIAL_SET));
    }

    // Spec 2026B association set returns the association report (CC:0083.01.12.11.000).
    // Association Set waits for type, slot, and destination, then builds the frame.
    TEST_F(UserCredentialCommandClassTest, AssociationSetIsAssembledOnceEveryFieldIsPresent)
    {
        store_pin_and_user_capabilities();
        auto group = endpoint.emplace_node(ASSOCIATION_SET_GROUP);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_set(group, FRAME_ASSOCIATION_SET));
        desire(group, user_credential_association_set_group_attributes_t::credential_type, CREDENTIAL_TYPE_PIN);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_set(group, FRAME_ASSOCIATION_SET));
        desire(group, user_credential_association_set_group_attributes_t::credential_slot, SLOT_1);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_set(group, FRAME_ASSOCIATION_SET));

        desire(group, user_credential_association_set_group_attributes_t::destination_user_unique_identifier, USER_IDENTIFIER_2);

        EXPECT_EQ(SL_STATUS_OK, assemble_set(group, FRAME_ASSOCIATION_SET));
    }

    // Spec 2026B the destination user is a non-zero existing user (CC:0083.01.12.11.003).
    // Association Set to a user above the supported count is not assembled.
    TEST_F(UserCredentialCommandClassTest, AssociationSetAssemblyRejectsDestinationAboveTheSupportedCount)
    {
        store_pin_and_user_capabilities();
        auto group = endpoint.emplace_node(ASSOCIATION_SET_GROUP);
        desire(group, user_credential_association_set_group_attributes_t::credential_type, CREDENTIAL_TYPE_PIN);
        desire(group, user_credential_association_set_group_attributes_t::credential_slot, SLOT_1);
        desire(group, user_credential_association_set_group_attributes_t::destination_user_unique_identifier, USER_IDENTIFIER_OUT_OF_RANGE);

        EXPECT_EQ(SL_STATUS_FAIL, assemble_set(group, FRAME_ASSOCIATION_SET));
    }

    // Spec 2026B User Checksum Get returns the User Checksum Report (CC:0083.01.16.11.001).
    // User Checksum Get waits for the identifier, then builds the frame.
    TEST_F(UserCredentialCommandClassTest, UserChecksumGetIsAssembledOnceTheIdentifierIsPresent)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities(static_cast<uint8_t>(ASCII_SUPPORT | USER_CHECKSUM_SUPPORT))));
        auto group = endpoint.emplace_node(USER_CHECKSUM_GET_GROUP);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_get(group, FRAME_USER_CHECKSUM_GET));

        desire(group, user_checksum_get_group_attributes_t::user_unique_identifier, USER_IDENTIFIER_1);

        EXPECT_EQ(SL_STATUS_OK, assemble_get(group, FRAME_USER_CHECKSUM_GET));
    }

    // Spec 2026B User Checksum Get is ignored without User Checksum support (CC:0083.01.16.11.000).
    // User Checksum Get is refused when User Checksum Support is clear.
    TEST_F(UserCredentialCommandClassTest, UserChecksumGetAssemblyIsRefusedWithoutSupport)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));
        auto group = endpoint.emplace_node(USER_CHECKSUM_GET_GROUP);
        desire(group, user_checksum_get_group_attributes_t::user_unique_identifier, USER_IDENTIFIER_1);

        EXPECT_EQ(SL_STATUS_FAIL, assemble_get(group, FRAME_USER_CHECKSUM_GET));
    }

    // Spec 2026B Credential Checksum Get returns the Credential Checksum Report (CC:0083.01.18.11.001).
    // Credential Checksum Get waits for the type, then builds the frame.
    TEST_F(UserCredentialCommandClassTest, CredentialChecksumGetIsAssembledOnceTheTypeIsPresent)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(pin_credential_capabilities(CREDENTIAL_CHECKSUM_SUPPORT)));
        auto group = endpoint.emplace_node(CRED_CHECKSUM_GET_GROUP);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_get(group, FRAME_CREDENTIAL_CHECKSUM_GET));

        desire(group, credential_checksum_get_group_attributes_t::credential_type, CREDENTIAL_TYPE_PIN);

        EXPECT_EQ(SL_STATUS_OK, assemble_get(group, FRAME_CREDENTIAL_CHECKSUM_GET));
    }

    // Spec 2026B Credential Checksum Get is ignored without Credential Checksum support (CC:0083.01.18.11.000).
    // Credential Checksum Get is refused when Credential Checksum Support is clear.
    TEST_F(UserCredentialCommandClassTest, CredentialChecksumGetAssemblyIsRefusedWithoutSupport)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(pin_credential_capabilities()));
        auto group = endpoint.emplace_node(CRED_CHECKSUM_GET_GROUP);
        desire(group, credential_checksum_get_group_attributes_t::credential_type, CREDENTIAL_TYPE_PIN);

        EXPECT_EQ(SL_STATUS_FAIL, assemble_get(group, FRAME_CREDENTIAL_CHECKSUM_GET));
    }

    // Spec 2026B length 0 deactivates and a length in range sets the Admin Code (CC:0083.01.1A.11.006, CC:0083.01.1A.11.016).
    // Admin PIN Set builds a deactivation frame and a frame that carries a code.
    TEST_F(UserCredentialCommandClassTest, AdminPinSetIsAssembledForDeactivationAndForACode)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(pin_credential_capabilities(static_cast<uint8_t>(ADMIN_CODE_SUPPORT | ADMIN_DEACTIVATION_SUPPORT))));
        auto group = endpoint.emplace_node(ADMIN_SET_GROUP);
        EXPECT_EQ(SL_STATUS_NOT_READY, assemble_set(group, FRAME_ADMIN_PIN_SET));
        auto length = desire(group, admin_pin_code_set_group_attributes_t::admin_pin_code_length, static_cast<uint8_t>(0));
        EXPECT_EQ(SL_STATUS_OK, assemble_set(group, FRAME_ADMIN_PIN_SET));

        desire(group, admin_pin_code_set_group_attributes_t::admin_code, FOUR_DIGIT_PIN);
        length.set_desired(static_cast<uint8_t>(FOUR_DIGIT_PIN.size()));

        EXPECT_EQ(SL_STATUS_OK, assemble_set(group, FRAME_ADMIN_PIN_SET));
    }

    // Spec 2026B Admin PIN Code Set without Admin Code support (CC:0083.01.1A.13.002).
    // Admin PIN Set is not assembled when Admin Code Support is clear.
    TEST_F(UserCredentialCommandClassTest, AdminPinSetAssemblyIsRefusedWithoutSupport)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(pin_credential_capabilities()));
        auto group = endpoint.emplace_node(ADMIN_SET_GROUP);
        desire(group, admin_pin_code_set_group_attributes_t::admin_pin_code_length, PIN_MIN);

        EXPECT_EQ(SL_STATUS_FAIL, assemble_set(group, FRAME_ADMIN_PIN_SET));
    }

    // Spec 2026B All Users Checksum Get is ignored without support (CC:0083.01.14.11.000).
    // All Users Checksum Get is not assembled when its support bit is clear.
    TEST_F(UserCredentialCommandClassTest, AllUsersChecksumGetAssemblyIsRefusedWithoutSupport)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));

        EXPECT_EQ(SL_STATUS_FAIL, assemble_get(endpoint.emplace_node(CHECKSUM_GET_GROUP), FRAME_ALL_USERS_CHECKSUM_GET));
    }

    // Spec 2026B value 1 means All Users Checksum is supported (CC:0083.01.02.11.011).
    // All Users Checksum Get is assembled when the peer advertises support.
    TEST_F(UserCredentialCommandClassTest, AllUsersChecksumGetAssemblySucceedsWithSupport)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities(static_cast<uint8_t>(ASCII_SUPPORT | ALL_USERS_CHECKSUM_SUPPORT))));

        EXPECT_EQ(SL_STATUS_OK, assemble_get(endpoint.emplace_node(CHECKSUM_GET_GROUP), FRAME_ALL_USERS_CHECKSUM_GET));
    }

    // Spec 2026B Admin PIN Code Get without Admin Code support (CC:0083.01.1B.13.001).
    // Admin PIN Get is not assembled when Admin Code Support is clear.
    TEST_F(UserCredentialCommandClassTest, AdminPinGetAssemblyIsRefusedWithoutSupport)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(pin_credential_capabilities()));

        EXPECT_EQ(SL_STATUS_FAIL, assemble_get(endpoint.emplace_node(ADMIN_GET_GROUP), FRAME_ADMIN_PIN_GET));
    }

    // Spec 2026B value 1 means Admin Code is supported (CC:0083.01.04.11.013).
    // Admin PIN Code Get is assembled when the peer advertises Admin Code support.
    TEST_F(UserCredentialCommandClassTest, AdminPinGetAssemblySucceedsWithSupport)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(pin_credential_capabilities(ADMIN_CODE_SUPPORT)));

        EXPECT_EQ(SL_STATUS_OK, assemble_get(endpoint.emplace_node(ADMIN_GET_GROUP), FRAME_ADMIN_PIN_GET));
    }

    // Spec 2026B value 0 means Admin Code is unsupported (CC:0083.01.04.11.014).
    // Interviewing a peer without Admin Code support does not request Admin PIN Code.
    TEST_F(UserCredentialCommandClassTest, InterviewSkipsAdminPinGetWithoutSupport)
    {
        start_user_walk();

        EXPECT_FALSE(endpoint.child_by_type(ADMIN_GET_GROUP).is_valid());
    }

    // Spec 2026B value 1 means Admin Code is supported (CC:0083.01.04.11.013).
    // Interviewing a peer with Admin Code support requests its Admin PIN Code.
    TEST_F(UserCredentialCommandClassTest, InterviewRequestsAdminPinGetWhenSupported)
    {
        set_peer_version(endpoint, VERSION_1);
        advertise(endpoint, {USER_CREDENTIAL_CC});
        open_interview(endpoint);
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));
        ASSERT_EQ(SL_STATUS_OK, receive(pin_credential_capabilities(ADMIN_CODE_SUPPORT)));

        EXPECT_TRUE(endpoint.child_by_type(ADMIN_GET_GROUP).is_valid());
    }

    // Spec 2026B adding a user (CL:0083.01.31.05.1).
    // The Library requirement defines the use case; this checks that the MQTT path uses the same valid User Set fields.
    // MQTT User Set stores a general user when the capabilities allow it.
    TEST_F(UserCredentialCommandClassTest, MqttUserSetStoresGeneralUser)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));
        const std::string payload = "{\"operation_type\":0,\"user_unique_identifier\":1,\"user_type\":0,\"user_active_state\":1,\"credential_rule\":1,\"expiring_timeout_minutes\":0,\"user_name_encoding\":0,\"user_name_length\":0,\"user_name\":[]}";

        ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_user_set_command(endpoint, payload));

        EXPECT_EQ(USER_IDENTIFIER_1, endpoint.child_by_type(USER_SET_GROUP).child_by_type(type_of(user_set_group_attributes_t::user_unique_identifier)).desired<uint16_t>());
    }

    // Spec 2026B operation types above Delete are reserved (CC:0083.01.05.11.003).
    // MQTT User Set with an operation above Delete is refused.
    TEST_F(UserCredentialCommandClassTest, MqttUserSetRejectsOperationAboveDelete)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));
        const std::string payload = "{\"operation_type\":3,\"user_unique_identifier\":1,\"user_type\":0,\"user_active_state\":1,\"credential_rule\":1,\"expiring_timeout_minutes\":0,\"user_name_encoding\":0,\"user_name_length\":0,\"user_name\":[]}";

        EXPECT_EQ(SL_STATUS_FAIL, component().mqtt_on_user_set_command(endpoint, payload));
    }

    // Spec 2026B a PIN is ASCII digits (CL:0083.01.31.14.1, CC:0083.01.0A.11.020).
    // MQTT Credential Set stores a supported PIN.
    TEST_F(UserCredentialCommandClassTest, MqttCredentialSetStoresAPin)
    {
        store_pin_and_user_capabilities();

        ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_credential_set_command(endpoint, "{\"user_unique_identifier\":1,\"credential_type\":1,\"credential_slot\":1,\"operation_type\":0,\"credential_length\":4,\"credential_data\":[49,50,51,52]}"));

        EXPECT_EQ(FOUR_DIGIT_PIN, endpoint.child_by_type(CREDENTIAL_SET_GROUP).child_by_type(type_of(credential_set_group_attributes_t::credential_data)).desired<std::vector<uint8_t>>());
    }

    // Spec 2026B a user identifier above the advertised maximum is ignored (CC:0083.01.05.11.014).
    // MQTT Credential Set for a user above the supported count is refused.
    TEST_F(UserCredentialCommandClassTest, MqttCredentialSetRejectsUserAboveTheSupportedCount)
    {
        store_pin_and_user_capabilities();

        EXPECT_EQ(SL_STATUS_FAIL, component().mqtt_on_credential_set_command(endpoint, "{\"user_unique_identifier\":99,\"credential_type\":1,\"credential_slot\":1,\"operation_type\":0,\"credential_length\":4,\"credential_data\":[49,50,51,52]}"));
    }

    // Spec 2026B All Users Checksum Get is ignored without support (CC:0083.01.14.11.000).
    // MQTT All Users Checksum Get is refused when its support bit is clear.
    TEST_F(UserCredentialCommandClassTest, MqttAllUsersChecksumGetIsRefusedWithoutSupport)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));

        EXPECT_EQ(SL_STATUS_FAIL, component().mqtt_on_all_users_checksum_get_command(endpoint, "{}"));
    }

    // Spec 2026B value 1 means All Users Checksum is supported (CC:0083.01.02.11.011).
    // MQTT All Users Checksum Get creates its request group when support is advertised.
    TEST_F(UserCredentialCommandClassTest, MqttAllUsersChecksumGetSucceedsWithSupport)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities(static_cast<uint8_t>(ASCII_SUPPORT | ALL_USERS_CHECKSUM_SUPPORT))));

        EXPECT_EQ(SL_STATUS_OK, component().mqtt_on_all_users_checksum_get_command(endpoint, "{}"));
        EXPECT_TRUE(endpoint.child_by_type(CHECKSUM_GET_GROUP).is_valid());
    }

    // Spec 2026B User Get, User Checksum Get, and Credential Checksum Get (CC:0083.01.06.11.003, CC:0083.01.16.11.001, CC:0083.01.18.11.004).
    // MQTT User Get and checksum Gets store the requested identifier when the peer supports them.
    TEST_F(UserCredentialCommandClassTest, MqttGetsStoreTheRequestedIdentifier)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities(static_cast<uint8_t>(ASCII_SUPPORT | USER_CHECKSUM_SUPPORT))));
        ASSERT_EQ(SL_STATUS_OK, receive(pin_credential_capabilities(CREDENTIAL_CHECKSUM_SUPPORT)));

        EXPECT_EQ(SL_STATUS_OK, component().mqtt_on_user_get_command(endpoint, "{\"user_unique_identifier\":1}"));
        EXPECT_EQ(SL_STATUS_OK, component().mqtt_on_user_checksum_get_command(endpoint, "{\"user_unique_identifier\":1}"));
        EXPECT_EQ(SL_STATUS_OK, component().mqtt_on_credential_checksum_get_command(endpoint, "{\"credential_type\":1}"));
    }

    // Spec 2026B a user identifier above the advertised maximum is ignored (CC:0083.01.05.11.014).
    // MQTT User Get for an identifier above the supported count is refused.
    TEST_F(UserCredentialCommandClassTest, MqttUserGetRejectsIdentifierAboveTheSupportedCount)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(general_user_capabilities()));

        EXPECT_EQ(SL_STATUS_FAIL, component().mqtt_on_user_get_command(endpoint, "{\"user_unique_identifier\":99}"));
    }

    // Spec 2026B checksum Gets are ignored without support (CC:0083.01.16.11.000, CC:0083.01.18.11.000).
    // MQTT checksum Gets are refused when their support bits are clear.
    TEST_F(UserCredentialCommandClassTest, MqttChecksumGetsAreRefusedWithoutSupport)
    {
        store_pin_and_user_capabilities();

        EXPECT_EQ(SL_STATUS_FAIL, component().mqtt_on_user_checksum_get_command(endpoint, "{\"user_unique_identifier\":1}"));
        EXPECT_EQ(SL_STATUS_FAIL, component().mqtt_on_credential_checksum_get_command(endpoint, "{\"credential_type\":1}"));
    }

    // Spec 2026B Credential Get, Credential Learn Start, and association (CC:0083.01.0B.11.003, CC:0083.01.0F.11.008, CC:0083.01.12.11.003).
    // MQTT Credential Get, Learn Start, and Association Set store a supported PIN slot.
    TEST_F(UserCredentialCommandClassTest, MqttCredentialCommandsStoreAPinSlot)
    {
        store_pin_and_user_capabilities();

        EXPECT_EQ(SL_STATUS_OK, component().mqtt_on_credential_get_command(endpoint, "{\"user_unique_identifier\":1,\"credential_type\":1,\"credential_slot\":1}"));
        EXPECT_EQ(SL_STATUS_OK, component().mqtt_on_credential_learn_start_command(endpoint, "{\"user_unique_identifier\":1,\"credential_type\":1,\"credential_slot\":1,\"operation_type\":0,\"credential_learn_timeout\":10}"));
        EXPECT_EQ(SL_STATUS_OK, component().mqtt_on_user_credential_association_set_command(endpoint, "{\"credential_type\":1,\"credential_slot\":1,\"destination_user_unique_identifier\":2}"));
    }

    // Spec 2026B a user identifier above the advertised maximum is ignored (CC:0083.01.05.11.014).
    // MQTT Credential Get and Association Set for a user above the supported count are refused.
    TEST_F(UserCredentialCommandClassTest, MqttCredentialCommandsRejectUserAboveTheSupportedCount)
    {
        store_pin_and_user_capabilities();

        EXPECT_EQ(SL_STATUS_FAIL, component().mqtt_on_credential_get_command(endpoint, "{\"user_unique_identifier\":99,\"credential_type\":1,\"credential_slot\":1}"));
        EXPECT_EQ(SL_STATUS_FAIL, component().mqtt_on_user_credential_association_set_command(endpoint, "{\"credential_type\":1,\"credential_slot\":1,\"destination_user_unique_identifier\":99}"));
    }

    // Spec 2026B Credential Learn Start operation types above Modify are reserved (CC:0083.01.0F.11.010).
    // MQTT Credential Learn Start with Delete is refused.
    TEST_F(UserCredentialCommandClassTest, MqttCredentialLearnStartRejectsDelete)
    {
        store_pin_and_user_capabilities();

        EXPECT_EQ(SL_STATUS_FAIL, component().mqtt_on_credential_learn_start_command(endpoint, "{\"user_unique_identifier\":1,\"credential_type\":1,\"credential_slot\":1,\"operation_type\":2,\"credential_learn_timeout\":10}"));
    }

    // The MQTT payload schema has no corresponding Z-Wave CC requirement; malformed required fields must be rejected.
    // An empty object omits the required User Set fields, so parsing fails before any desired state is created.
    TEST_F(UserCredentialCommandClassTest, MqttUserSetRejectsMissingFields)
    {
        EXPECT_NE(SL_STATUS_OK, component().mqtt_on_user_set_command(endpoint, "{}"));
        EXPECT_FALSE(endpoint.child_by_type(USER_SET_GROUP).is_valid());
    }

    // The MQTT payload schema has no corresponding Z-Wave CC requirement; malformed required fields must be rejected.
    // An empty object omits the required User Get identifier, so parsing fails before a Get group is created.
    TEST_F(UserCredentialCommandClassTest, MqttUserGetRejectsMissingIdentifier)
    {
        EXPECT_NE(SL_STATUS_OK, component().mqtt_on_user_get_command(endpoint, "{}"));
        EXPECT_FALSE(endpoint.child_by_type(USER_GET_GROUP).is_valid());
    }

    // The MQTT payload schema has no corresponding Z-Wave CC requirement; malformed required fields must be rejected.
    // An empty object omits the required Credential Set fields, so parsing fails before desired state is created.
    TEST_F(UserCredentialCommandClassTest, MqttCredentialSetRejectsMissingFields)
    {
        EXPECT_NE(SL_STATUS_OK, component().mqtt_on_credential_set_command(endpoint, "{}"));
        EXPECT_FALSE(endpoint.child_by_type(CREDENTIAL_SET_GROUP).is_valid());
    }

    // The MQTT payload schema has no corresponding Z-Wave CC requirement; malformed required fields must be rejected.
    // An empty object omits the required Credential Get fields, so parsing fails before a Get group is created.
    TEST_F(UserCredentialCommandClassTest, MqttCredentialGetRejectsMissingFields)
    {
        EXPECT_NE(SL_STATUS_OK, component().mqtt_on_credential_get_command(endpoint, "{}"));
        EXPECT_FALSE(endpoint.child_by_type(CREDENTIAL_GET_GROUP).is_valid());
    }

    // The MQTT payload schema has no corresponding Z-Wave CC requirement; malformed required fields must be rejected.
    // An empty object omits Credential Learn Start fields, so parsing fails before a request group is created.
    TEST_F(UserCredentialCommandClassTest, MqttCredentialLearnStartRejectsMissingFields)
    {
        EXPECT_NE(SL_STATUS_OK, component().mqtt_on_credential_learn_start_command(endpoint, "{}"));
        EXPECT_FALSE(endpoint.child_by_type(LEARN_START_GROUP).is_valid());
    }

    // The MQTT payload schema has no corresponding Z-Wave CC requirement; malformed required fields must be rejected.
    // An empty object omits Association Set fields, so parsing fails before desired state is created.
    TEST_F(UserCredentialCommandClassTest, MqttAssociationSetRejectsMissingFields)
    {
        EXPECT_NE(SL_STATUS_OK, component().mqtt_on_user_credential_association_set_command(endpoint, "{}"));
        EXPECT_FALSE(endpoint.child_by_type(ASSOCIATION_SET_GROUP).is_valid());
    }

    // The MQTT payload schema has no corresponding Z-Wave CC requirement; malformed required fields must be rejected.
    // An empty object omits the User Checksum Get identifier, so parsing fails before a Get group is created.
    TEST_F(UserCredentialCommandClassTest, MqttUserChecksumGetRejectsMissingIdentifier)
    {
        EXPECT_NE(SL_STATUS_OK, component().mqtt_on_user_checksum_get_command(endpoint, "{}"));
        EXPECT_FALSE(endpoint.child_by_type(type_of(user_checksum_get_group_attributes_t::USER_CHECKSUM_GET_GROUP)).is_valid());
    }

    // The MQTT payload schema has no corresponding Z-Wave CC requirement; malformed required fields must be rejected.
    // An empty object omits the Credential Checksum Get type, so parsing fails before a Get group is created.
    TEST_F(UserCredentialCommandClassTest, MqttCredentialChecksumGetRejectsMissingType)
    {
        EXPECT_NE(SL_STATUS_OK, component().mqtt_on_credential_checksum_get_command(endpoint, "{}"));
        EXPECT_FALSE(endpoint.child_by_type(type_of(credential_checksum_get_group_attributes_t::CREDENTIAL_CHECKSUM_GET_GROUP)).is_valid());
    }

    // The MQTT payload schema has no corresponding Z-Wave CC requirement; malformed required fields must be rejected.
    // An empty object omits the required Admin PIN length, so parsing fails before desired state is created.
    TEST_F(UserCredentialCommandClassTest, MqttAdminPinSetRejectsMissingLength)
    {
        EXPECT_NE(SL_STATUS_OK, component().mqtt_on_admin_pin_code_set_command(endpoint, "{}"));
        EXPECT_FALSE(endpoint.child_by_type(ADMIN_SET_GROUP).is_valid());
    }

    // Spec 2026B length 0 deactivates Admin Code when supported (CC:0083.01.1A.11.006).
    // Omitting the optional code for deactivation stores an empty desired code and succeeds.
    TEST_F(UserCredentialCommandClassTest, MqttAdminPinDeactivationAllowsOmittedCode)
    {
        ASSERT_EQ(SL_STATUS_OK, receive(pin_credential_capabilities(static_cast<uint8_t>(ADMIN_CODE_SUPPORT | ADMIN_DEACTIVATION_SUPPORT))));

        ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_admin_pin_code_set_command(endpoint, "{\"admin_pin_code_length\":0}"));

        EXPECT_TRUE(endpoint.child_by_type(ADMIN_SET_GROUP).child_by_type(type_of(admin_pin_code_set_group_attributes_t::admin_code)).desired<std::vector<uint8_t>>().empty());
    }

}  // namespace user_credential_test
