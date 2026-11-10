/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************/
#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "attribute.hpp"
#include "attribute_store_fixt.h"
#include "command_class_barrier_operator.hpp"
#include "command_class_barrier_operator_core.hpp"
#include "command_class_barrier_operator_constants.hpp"
#include "command_class_barrier_operator_mqtt.hpp"
#include "command_class_barrier_operator_types.hpp"
#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "component_connector_types.hpp"
#include "zwave_frame_generator.hpp"
#include "zwave_frame_parser.hpp"

using namespace zwave_command_class;
using namespace zwave_command_class::command_class_barrier_operator_types;

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}

namespace barrier_operator_test
{
    using namespace command_class_barrier_operator_constants;

    constexpr uint8_t BARRIER_OPERATOR_SET_COMMAND      = 0x01;
    constexpr uint8_t BARRIER_OPERATOR_GET_COMMAND      = 0x02;
    constexpr uint8_t BARRIER_OPERATOR_REPORT_COMMAND   = 0x03;
    constexpr uint8_t SIGNAL_SUPPORTED_GET_COMMAND      = 0x04;
    constexpr uint8_t SIGNAL_SUPPORTED_REPORT_COMMAND   = 0x05;
    constexpr uint8_t EVENT_SIGNAL_SET_COMMAND          = 0x06;
    constexpr uint8_t EVENT_SIGNALING_GET_COMMAND       = 0x07;
    constexpr uint8_t EVENT_SIGNALING_REPORT_COMMAND    = 0x08;
    constexpr uint8_t TARGET_VALUE_CLOSE                = BARRIER_OPERATOR_TARGET_CLOSE;
    constexpr uint8_t TARGET_VALUE_OPEN                 = BARRIER_OPERATOR_TARGET_OPEN;
    constexpr uint8_t RESERVED_TARGET_VALUE             = BARRIER_OPERATOR_TARGET_RESERVED;
    constexpr uint8_t SUBSYSTEM_AUDIBLE                 = BARRIER_OPERATOR_SUBSYSTEM_AUDIBLE;
    constexpr uint8_t SUBSYSTEM_VISUAL                  = BARRIER_OPERATOR_SUBSYSTEM_VISUAL;
    constexpr uint8_t RESERVED_SUBSYSTEM_TYPE           = BARRIER_OPERATOR_SUBSYSTEM_RESERVED;
    constexpr uint8_t SUBSYSTEM_OFF                     = BARRIER_OPERATOR_SUBSYSTEM_OFF;
    constexpr uint8_t SUBSYSTEM_ON                      = BARRIER_OPERATOR_SUBSYSTEM_ON;
    constexpr uint8_t SUPPORTED_AUDIBLE_AND_VISUAL      = BARRIER_OPERATOR_SUBSYSTEM_AUDIBLE | BARRIER_OPERATOR_SUBSYSTEM_VISUAL;
    constexpr uint8_t BARRIER_STATE_OPEN                = BARRIER_OPERATOR_TARGET_OPEN;
    constexpr uint8_t RESERVED_BARRIER_STATE            = BARRIER_OPERATOR_STATE_RESERVED;
    constexpr uint8_t RESERVED_SUBSYSTEM_STATE          = BARRIER_OPERATOR_SUBSYSTEM_STATE_RESERVED;
    constexpr uint8_t UNSUPPORTED_SUBSYSTEM_TYPE        = RESERVED_SUBSYSTEM_TYPE;
    constexpr uint8_t UNKNOWN_COMMAND_ID                = 0xFF;
    constexpr size_t EXPECTED_SUPPORTED_SUBSYSTEM_COUNT = 2;
    constexpr size_t EMPTY_GET_FRAME_SIZE               = 2;
    constexpr size_t BARRIER_SET_FRAME_SIZE             = 3;
    constexpr size_t EVENT_SIGNAL_SET_FRAME_SIZE        = 4;
    constexpr size_t EVENT_SIGNAL_GET_FRAME_SIZE        = 3;
    constexpr size_t BARRIER_REPORT_FRAME_SIZE          = 3;
    constexpr size_t SIGNAL_SUPPORTED_REPORT_FRAME_SIZE = 3;
    constexpr size_t EVENT_SIGNAL_REPORT_FRAME_SIZE     = 4;
    constexpr size_t FRAME_CAPACITY                     = 8;
    constexpr uint16_t TEST_NODE_ID                     = 2;
    constexpr uint8_t TEST_ENDPOINT_ID                  = 0;
    constexpr std::array<uint8_t, 4> TEST_HOME_ID {0x00, 0x00, 0x00, 0x00};

    const auto SET_GROUP                     = static_cast<attribute_store_type_t>(barrier_operator_set_group_attributes_t::BARRIER_OPERATOR_SET_GROUP);
    const auto TARGET_VALUE                  = static_cast<attribute_store_type_t>(barrier_operator_set_group_attributes_t::target_value);
    const auto GET_GROUP                     = static_cast<attribute_store_type_t>(barrier_operator_get_group_attributes_t::BARRIER_OPERATOR_GET_GROUP);
    const auto SIGNAL_SUPPORTED_GET_GROUP    = static_cast<attribute_store_type_t>(barrier_operator_signal_supported_get_group_attributes_t::BARRIER_OPERATOR_SIGNAL_SUPPORTED_GET_GROUP);
    const auto REPORT_GROUP                  = static_cast<attribute_store_type_t>(barrier_operator_report_group_attributes_t::BARRIER_OPERATOR_REPORT_GROUP);
    const auto REPORT_STATE                  = static_cast<attribute_store_type_t>(barrier_operator_report_group_attributes_t::state);
    const auto SIGNAL_SUPPORTED_REPORT_GROUP = static_cast<attribute_store_type_t>(barrier_operator_signal_supported_report_group_attributes_t::BARRIER_OPERATOR_SIGNAL_SUPPORTED_REPORT_GROUP);
    const auto SIGNAL_SUPPORTED_REPORT_MASK  = static_cast<attribute_store_type_t>(barrier_operator_signal_supported_report_group_attributes_t::bit_mask);
    const auto EVENT_SIGNAL_SET_GROUP        = static_cast<attribute_store_type_t>(barrier_operator_event_signal_set_group_attributes_t::BARRIER_OPERATOR_EVENT_SIGNAL_SET_GROUP);
    const auto EVENT_SIGNAL_SET_TYPE         = static_cast<attribute_store_type_t>(barrier_operator_event_signal_set_group_attributes_t::subsystem_type);
    const auto EVENT_SIGNAL_SET_STATE        = static_cast<attribute_store_type_t>(barrier_operator_event_signal_set_group_attributes_t::subsystem_state);
    const auto EVENT_SIGNALING_GET_GROUP     = static_cast<attribute_store_type_t>(barrier_operator_event_signaling_get_group_attributes_t::BARRIER_OPERATOR_EVENT_SIGNALING_GET_GROUP);
    const auto EVENT_SIGNALING_GET_TYPE      = static_cast<attribute_store_type_t>(barrier_operator_event_signaling_get_group_attributes_t::subsystem_type);
    const auto EVENT_SIGNALING_REPORT_GROUP  = static_cast<attribute_store_type_t>(barrier_operator_event_signaling_report_group_attributes_t::BARRIER_OPERATOR_EVENT_SIGNALING_REPORT_GROUP);
    const auto EVENT_SIGNALING_REPORT_TYPE   = static_cast<attribute_store_type_t>(barrier_operator_event_signaling_report_group_attributes_t::subsystem_type);
    const auto EVENT_SIGNALING_REPORT_STATE  = static_cast<attribute_store_type_t>(barrier_operator_event_signaling_report_group_attributes_t::subsystem_state);

    class BarrierOperatorTest : public ::testing::Test
    {
        protected:
            static command_class_barrier_operator &component()
            {
                static command_class_barrier_operator instance;
                return instance;
            }

            void SetUp() override
            {
                ASSERT_EQ(SL_STATUS_OK, attribute_store_init());
                auto home                    = attribute_store::attribute::root().emplace_node(ATTRIBUTE_HOME_ID, std::vector<uint8_t>(TEST_HOME_ID.begin(), TEST_HOME_ID.end()));
                static uint16_t next_node_id = TEST_NODE_ID;
                node_id                      = next_node_id++;
                auto node                    = home.emplace_node(ATTRIBUTE_NODE_ID, node_id);
                endpoint                     = node.emplace_node(ATTRIBUTE_ENDPOINT_ID, TEST_ENDPOINT_ID);
                (void)component();
                register_signal_mask_attribute();
            }

            void TearDown() override
            {
                attribute_store_teardown();
            }

            void set_version()
            {
                endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(BARRIER_OPERATOR_CC_ID)).set_reported(BARRIER_OPERATOR_VERSION_1);
            }

            void start_interview()
            {
                endpoint.emplace_node(ATTRIBUTE_ZWAVE_NIF, std::vector<uint8_t> {BARRIER_OPERATOR_CC_ID});
                set_version();
                component_connector connector;
                const component_connector_interview_done_payload_t payload {static_cast<attribute_store_node_t>(endpoint), SL_STATUS_OK};
                auto status = connector.fire_event_async(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_INTERVIEW_DONE), payload);
                connector.run();
                ASSERT_EQ(SL_STATUS_OK, status.get());
            }

            static void register_signal_mask_attribute()
            {
                if (!attribute_store_is_type_registered(SIGNAL_SUPPORTED_REPORT_MASK)) {
                    attribute_store_register_type(SIGNAL_SUPPORTED_REPORT_MASK, "bit_mask", SIGNAL_SUPPORTED_REPORT_GROUP, BYTE_ARRAY_STORAGE_TYPE);
                }
            }

            sl_status_t assemble_set(attribute_store::attribute group, uint8_t *data, uint16_t *length)
            {
                zwave_frame_generator generator {BARRIER_OPERATOR_CC_ID};
                generator.initialize_frame(BARRIER_OPERATOR_SET_COMMAND, data, length);
                command_class_barrier_operator_core::set_requested_args args {group, &generator};
                return component().on_barrier_operator_set_requested(args, data, length);
            }

            sl_status_t assemble_get(attribute_store::attribute group, uint8_t command, uint8_t *data, uint16_t *length)
            {
                zwave_frame_generator generator {BARRIER_OPERATOR_CC_ID};
                generator.initialize_frame(command, data, length);
                command_class_barrier_operator_core::get_requested_args args {group, &generator};
                if (command == BARRIER_OPERATOR_GET_COMMAND) {
                    return component().on_barrier_operator_get_requested(args, data, length);
                }
                if (command == SIGNAL_SUPPORTED_GET_COMMAND) {
                    return component().on_barrier_operator_signal_supported_get_requested(args, data, length);
                }
                return component().on_barrier_operator_event_signaling_get_requested(args, data, length);
            }

            template<size_t N> sl_status_t receive_report(const std::array<uint8_t, N> &frame)
            {
                zwave_controller_connection_info_t connection_info {};
                connection_info.remote.node_id     = node_id;
                connection_info.remote.endpoint_id = TEST_ENDPOINT_ID;
                return component().control_handler(&connection_info, frame.data(), static_cast<uint16_t>(frame.size()));
            }

            attribute_store::attribute endpoint;
            uint16_t node_id = 0;
    };
}  // namespace barrier_operator_test

using namespace barrier_operator_test;

// Spec 2026B Barrier Operator v1 interview (CC:0066.01.02.11.001, CC:0066.01.04.11.001).
// Interview starts status and signaling-capability Gets and requires both reported values.
TEST_F(BarrierOperatorTest, InterviewStartsStatusAndCapabilitiesGets)
{
    component().on_interview(endpoint, BARRIER_OPERATOR_VERSION_1);

    EXPECT_TRUE(endpoint.child_by_type(GET_GROUP).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(SIGNAL_SUPPORTED_GET_GROUP).is_valid());

    auto status_report = endpoint.child_by_type(REPORT_GROUP);
    ASSERT_TRUE(status_report.is_valid());
    EXPECT_TRUE(status_report.child_by_type(REPORT_STATE).is_valid());

    auto capabilities_report = endpoint.child_by_type(SIGNAL_SUPPORTED_REPORT_GROUP);
    ASSERT_TRUE(capabilities_report.is_valid());
    EXPECT_TRUE(capabilities_report.child_by_type(SIGNAL_SUPPORTED_REPORT_MASK).is_valid());
}

// Spec 2026B Barrier Operator Set v1 (CC:0066.01.01.11.001).
// Set encodes the requested barrier target in its required payload byte.
TEST_F(BarrierOperatorTest, SetAssemblesTargetValue)
{
    set_version();
    auto group = endpoint.emplace_node(SET_GROUP);
    group.emplace_node(TARGET_VALUE).set_desired(TARGET_VALUE_OPEN);

    std::array<uint8_t, FRAME_CAPACITY> frame {};
    uint16_t length = 0;
    ASSERT_EQ(SL_STATUS_OK, assemble_set(group, frame.data(), &length));

    const std::array<uint8_t, BARRIER_SET_FRAME_SIZE> expected {BARRIER_OPERATOR_CC_ID, BARRIER_OPERATOR_SET_COMMAND, TARGET_VALUE_OPEN};
    const std::array<uint8_t, BARRIER_SET_FRAME_SIZE> actual {frame[0], frame[1], frame[2]};
    EXPECT_EQ(expected, actual);
    EXPECT_EQ(expected.size(), length);
}

// Spec 2026B Barrier Operator Set v1 (CC:0066.01.01.11.001).
// Set encodes CLOSE as the specified zero-valued target byte.
TEST_F(BarrierOperatorTest, SetAssemblesCloseTargetValue)
{
    set_version();
    auto group = endpoint.emplace_node(SET_GROUP);
    group.emplace_node(TARGET_VALUE).set_desired(TARGET_VALUE_CLOSE);

    std::array<uint8_t, FRAME_CAPACITY> frame {};
    uint16_t length = 0;
    ASSERT_EQ(SL_STATUS_OK, assemble_set(group, frame.data(), &length));

    EXPECT_EQ((std::array<uint8_t, BARRIER_SET_FRAME_SIZE> {BARRIER_OPERATOR_CC_ID, BARRIER_OPERATOR_SET_COMMAND, TARGET_VALUE_CLOSE}), (std::array<uint8_t, BARRIER_SET_FRAME_SIZE> {frame[0], frame[1], frame[2]}));
    EXPECT_EQ(BARRIER_SET_FRAME_SIZE, length);
}

// Spec 2026B Barrier Operator Set v1 required payload (CC:0066.01.01.11.001).
// Frame assembly waits until the required target value has a desired value.
TEST_F(BarrierOperatorTest, SetWaitsForTargetValue)
{
    auto group = endpoint.emplace_node(SET_GROUP);
    group.emplace_node(TARGET_VALUE);
    std::array<uint8_t, FRAME_CAPACITY> frame {};
    uint16_t length = 0;

    EXPECT_EQ(SL_STATUS_NOT_READY, assemble_set(group, frame.data(), &length));
}

// Spec 2026B Barrier Operator Set v1 (CC:0066.01.01.11.003).
// A reserved target value cannot be emitted by the controller.
TEST_F(BarrierOperatorTest, SetRejectsReservedTargetValue)
{
    auto group = endpoint.emplace_node(SET_GROUP);
    group.emplace_node(TARGET_VALUE).set_desired(RESERVED_TARGET_VALUE);
    std::array<uint8_t, FRAME_CAPACITY> frame {};
    uint16_t length = 0;

    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, assemble_set(group, frame.data(), &length));
}

// Spec 2026B Barrier Operator MQTT Set payload.
// MQTT stores the requested target value before asking the resolver to send it.
TEST_F(BarrierOperatorTest, MqttSetStoresDesiredTargetValue)
{
    auto endpoint_node = endpoint;
    auto &mqtt         = static_cast<command_class_barrier_operator_mqtt &>(component());
    ASSERT_EQ(SL_STATUS_OK, mqtt.mqtt_on_barrier_operator_set_command(endpoint_node, R"({"target_value":"0xFF"})"));

    auto group = endpoint.child_by_type(SET_GROUP);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(TARGET_VALUE_OPEN, group.child_by_type(TARGET_VALUE).desired<uint8_t>());
}

// Spec 2026B Barrier Operator v1 interview (CC:0066.01.05.11.001, CC:0066.01.05.11.002).
// Capabilities discover supported signaling subsystems and the interview queries their states in order.
TEST_F(BarrierOperatorTest, CapabilityReportQueuesReportsForAdvertisedSubsystems)
{
    start_interview();

    // The mask's low two bits advertise subsystem types 1 (audible) and 2 (visual), in that order.
    ASSERT_EQ(SL_STATUS_OK, receive_report(std::array<uint8_t, SIGNAL_SUPPORTED_REPORT_FRAME_SIZE> {BARRIER_OPERATOR_CC_ID, SIGNAL_SUPPORTED_REPORT_COMMAND, SUPPORTED_AUDIBLE_AND_VISUAL}));
    auto get_group = endpoint.child_by_type(EVENT_SIGNALING_GET_GROUP);
    ASSERT_TRUE(get_group.is_valid());
    EXPECT_EQ(SUBSYSTEM_AUDIBLE, get_group.child_by_type(EVENT_SIGNALING_GET_TYPE).desired<uint8_t>());

    auto audible_report = component().find_report_group_by_subsystem_type(endpoint, SUBSYSTEM_AUDIBLE);
    auto visual_report  = component().find_report_group_by_subsystem_type(endpoint, SUBSYSTEM_VISUAL);
    ASSERT_TRUE(audible_report.is_valid());
    ASSERT_TRUE(visual_report.is_valid());
    EXPECT_TRUE(audible_report.child_by_type(EVENT_SIGNALING_REPORT_STATE).is_valid());
    EXPECT_TRUE(visual_report.child_by_type(EVENT_SIGNALING_REPORT_STATE).is_valid());
    EXPECT_EQ(EXPECTED_SUPPORTED_SUBSYSTEM_COUNT, endpoint.children(EVENT_SIGNALING_REPORT_GROUP).size());
    EXPECT_EQ(SUBSYSTEM_AUDIBLE, audible_report.child_by_type(EVENT_SIGNALING_REPORT_TYPE).reported<uint8_t>());
    EXPECT_EQ(SUBSYSTEM_VISUAL, visual_report.child_by_type(EVENT_SIGNALING_REPORT_TYPE).reported<uint8_t>());

    // The Audible report satisfies its requirement and schedules the next supported subsystem.
    ASSERT_EQ(SL_STATUS_OK, receive_report(std::array<uint8_t, EVENT_SIGNAL_REPORT_FRAME_SIZE> {BARRIER_OPERATOR_CC_ID, EVENT_SIGNALING_REPORT_COMMAND, SUBSYSTEM_AUDIBLE, SUBSYSTEM_OFF}));
    EXPECT_EQ(SUBSYSTEM_VISUAL, get_group.child_by_type(EVENT_SIGNALING_GET_TYPE).desired<uint8_t>());

    // The Visual report satisfies the final discovered requirement; the Barrier status remains required.
    ASSERT_EQ(SL_STATUS_OK, receive_report(std::array<uint8_t, EVENT_SIGNAL_REPORT_FRAME_SIZE> {BARRIER_OPERATOR_CC_ID, EVENT_SIGNALING_REPORT_COMMAND, SUBSYSTEM_VISUAL, SUBSYSTEM_ON}));
    EXPECT_TRUE(component().has_report_for_subsystem_type(endpoint, SUBSYSTEM_AUDIBLE));
    EXPECT_TRUE(component().has_report_for_subsystem_type(endpoint, SUBSYSTEM_VISUAL));

    // The primary status report is the final interview requirement and closes the interview window.
    ASSERT_EQ(SL_STATUS_OK, receive_report(std::array<uint8_t, BARRIER_REPORT_FRAME_SIZE> {BARRIER_OPERATOR_CC_ID, BARRIER_OPERATOR_REPORT_COMMAND, BARRIER_STATE_OPEN}));
    auto status_group = endpoint.child_by_type(REPORT_GROUP);
    ASSERT_EQ(BARRIER_STATE_OPEN, status_group.child_by_type(REPORT_STATE).reported<uint8_t>());
}

// Spec 2026B Barrier Operator v1 (CC:0066.01.01.11.003).
// MQTT Set rejects reserved target values before creating a request group.
TEST_F(BarrierOperatorTest, MqttSetRejectsReservedTargetValue)
{
    auto endpoint_node = endpoint;
    auto &mqtt         = static_cast<command_class_barrier_operator_mqtt &>(component());
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, mqtt.mqtt_on_barrier_operator_set_command(endpoint_node, R"({"target_value":"0x01"})"));
    EXPECT_FALSE(endpoint.child_by_type(SET_GROUP).is_valid());
}

// Spec 2026B Barrier Operator v1 Get commands.
// Both status and capability Gets have no command-specific payload bytes.
TEST_F(BarrierOperatorTest, GetCommandsAssembleWithoutPayload)
{
    std::array<uint8_t, FRAME_CAPACITY> frame {};
    uint16_t length = 0;
    auto status_get = endpoint.emplace_node(GET_GROUP);
    ASSERT_EQ(SL_STATUS_OK, assemble_get(status_get, BARRIER_OPERATOR_GET_COMMAND, frame.data(), &length));
    EXPECT_EQ((std::array<uint8_t, 2> {BARRIER_OPERATOR_CC_ID, BARRIER_OPERATOR_GET_COMMAND}), (std::array<uint8_t, 2> {frame[0], frame[1]}));
    EXPECT_EQ(EMPTY_GET_FRAME_SIZE, length);

    auto capabilities_get = endpoint.emplace_node(SIGNAL_SUPPORTED_GET_GROUP);
    ASSERT_EQ(SL_STATUS_OK, assemble_get(capabilities_get, SIGNAL_SUPPORTED_GET_COMMAND, frame.data(), &length));
    EXPECT_EQ((std::array<uint8_t, 2> {BARRIER_OPERATOR_CC_ID, SIGNAL_SUPPORTED_GET_COMMAND}), (std::array<uint8_t, 2> {frame[0], frame[1]}));
    EXPECT_EQ(EMPTY_GET_FRAME_SIZE, length);
}

// Spec 2026B Barrier Operator Event Signal Set v1 (CC:0066.01.06.11.002, CC:0066.01.06.11.003).
// Event Signal Set serializes subsystem type and requested on/off state after its command identifier.
TEST_F(BarrierOperatorTest, EventSignalSetAssemblesSubsystemAndState)
{
    auto group = endpoint.emplace_node(EVENT_SIGNAL_SET_GROUP);
    group.emplace_node(EVENT_SIGNAL_SET_TYPE).set_desired(SUBSYSTEM_VISUAL);
    group.emplace_node(EVENT_SIGNAL_SET_STATE).set_desired(SUBSYSTEM_ON);
    std::array<uint8_t, FRAME_CAPACITY> frame {};
    uint16_t length = 0;
    zwave_frame_generator generator {BARRIER_OPERATOR_CC_ID};
    generator.initialize_frame(EVENT_SIGNAL_SET_COMMAND, frame.data(), &length);
    command_class_barrier_operator_core::set_requested_args args {group, &generator};

    ASSERT_EQ(SL_STATUS_OK, component().on_barrier_operator_event_signal_set_requested(args, frame.data(), &length));
    EXPECT_EQ((std::array<uint8_t, EVENT_SIGNAL_SET_FRAME_SIZE> {BARRIER_OPERATOR_CC_ID, EVENT_SIGNAL_SET_COMMAND, SUBSYSTEM_VISUAL, SUBSYSTEM_ON}), (std::array<uint8_t, EVENT_SIGNAL_SET_FRAME_SIZE> {frame[0], frame[1], frame[2], frame[3]}));
    EXPECT_EQ(EVENT_SIGNAL_SET_FRAME_SIZE, length);
}

// Spec 2026B Barrier Operator Event Signal Set v1 (CC:0066.01.06.11.003).
// Reserved subsystem states cannot be assembled into an outgoing Set frame.
TEST_F(BarrierOperatorTest, EventSignalSetRejectsReservedState)
{
    auto group = endpoint.emplace_node(EVENT_SIGNAL_SET_GROUP);
    group.emplace_node(EVENT_SIGNAL_SET_TYPE).set_desired(SUBSYSTEM_AUDIBLE);
    group.emplace_node(EVENT_SIGNAL_SET_STATE).set_desired(RESERVED_SUBSYSTEM_STATE);
    std::array<uint8_t, FRAME_CAPACITY> frame {};
    uint16_t length = 0;
    zwave_frame_generator generator {BARRIER_OPERATOR_CC_ID};
    generator.initialize_frame(EVENT_SIGNAL_SET_COMMAND, frame.data(), &length);
    command_class_barrier_operator_core::set_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, component().on_barrier_operator_event_signal_set_requested(args, frame.data(), &length));
}

// Spec 2026B Barrier Operator Event Signaling Get v1 (CC:0066.01.07.11.001).
// The Get includes the selected subsystem type as its single payload byte.
TEST_F(BarrierOperatorTest, EventSignalingGetAssemblesSubsystemType)
{
    auto group = endpoint.emplace_node(EVENT_SIGNALING_GET_GROUP);
    group.emplace_node(EVENT_SIGNALING_GET_TYPE).set_desired(SUBSYSTEM_AUDIBLE);
    std::array<uint8_t, FRAME_CAPACITY> frame {};
    uint16_t length = 0;
    ASSERT_EQ(SL_STATUS_OK, assemble_get(group, EVENT_SIGNALING_GET_COMMAND, frame.data(), &length));

    EXPECT_EQ((std::array<uint8_t, EVENT_SIGNAL_GET_FRAME_SIZE> {BARRIER_OPERATOR_CC_ID, EVENT_SIGNALING_GET_COMMAND, SUBSYSTEM_AUDIBLE}), (std::array<uint8_t, EVENT_SIGNAL_GET_FRAME_SIZE> {frame[0], frame[1], frame[2]}));
    EXPECT_EQ(EVENT_SIGNAL_GET_FRAME_SIZE, length);
}

// Spec 2026B Barrier Operator Event Signaling Get v1 (CC:0066.01.07.11.001).
// A reserved subsystem type cannot be sent in the Get field.
TEST_F(BarrierOperatorTest, EventSignalingGetRejectsReservedSubsystemType)
{
    auto group = endpoint.emplace_node(EVENT_SIGNALING_GET_GROUP);
    group.emplace_node(EVENT_SIGNALING_GET_TYPE).set_desired(RESERVED_SUBSYSTEM_TYPE);
    std::array<uint8_t, FRAME_CAPACITY> frame {};
    uint16_t length = 0;
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, assemble_get(group, EVENT_SIGNALING_GET_COMMAND, frame.data(), &length));
}

// Spec 2026B Barrier Operator Event Signaling Get v1 required payload (CC:0066.01.07.11.001).
// Frame assembly waits until the requested subsystem type is set.
TEST_F(BarrierOperatorTest, EventSignalingGetWaitsForSubsystemType)
{
    auto group = endpoint.emplace_node(EVENT_SIGNALING_GET_GROUP);
    group.emplace_node(EVENT_SIGNALING_GET_TYPE);
    std::array<uint8_t, FRAME_CAPACITY> frame {};
    uint16_t length = 0;

    EXPECT_EQ(SL_STATUS_NOT_READY, assemble_get(group, EVENT_SIGNALING_GET_COMMAND, frame.data(), &length));
}

// Barrier Operator Event Signal Set accepts the supported Visual subsystem with OFF.
// This exercises the valid second subsystem and second state encoding.
TEST_F(BarrierOperatorTest, EventSignalSetAssemblesVisualOff)
{
    set_version();
    auto group = endpoint.emplace_node(EVENT_SIGNAL_SET_GROUP);
    group.emplace_node(EVENT_SIGNAL_SET_TYPE).set_desired(SUBSYSTEM_VISUAL);
    group.emplace_node(EVENT_SIGNAL_SET_STATE).set_desired(SUBSYSTEM_OFF);
    std::array<uint8_t, FRAME_CAPACITY> frame {};
    uint16_t length = 0;
    zwave_frame_generator generator {BARRIER_OPERATOR_CC_ID};
    generator.initialize_frame(EVENT_SIGNAL_SET_COMMAND, frame.data(), &length);
    command_class_barrier_operator_core::set_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_barrier_operator_event_signal_set_requested(args, frame.data(), &length));
    const std::array<uint8_t, EVENT_SIGNAL_SET_FRAME_SIZE> expected {BARRIER_OPERATOR_CC_ID, EVENT_SIGNAL_SET_COMMAND, SUBSYSTEM_VISUAL, SUBSYSTEM_OFF};
    EXPECT_EQ(expected, (std::array<uint8_t, EVENT_SIGNAL_SET_FRAME_SIZE> {frame[0], frame[1], frame[2], frame[3]}));
    EXPECT_EQ(expected.size(), length);
}

// Spec 2026B Barrier Operator v1 signaling capabilities (CC:0066.01.05.11.002).
// A one-byte zero mask advertises no signaling subsystem and schedules no subsystem Get.
TEST_F(BarrierOperatorTest, EmptyCapabilitiesDoNotQueueSubsystemGet)
{
    start_interview();
    ASSERT_EQ(SL_STATUS_OK, receive_report(std::array<uint8_t, SIGNAL_SUPPORTED_REPORT_FRAME_SIZE> {BARRIER_OPERATOR_CC_ID, SIGNAL_SUPPORTED_REPORT_COMMAND, SUBSYSTEM_OFF}));
    EXPECT_FALSE(endpoint.child_by_type(EVENT_SIGNALING_GET_GROUP).is_valid());
}

// Spec 2026B Barrier Operator v1 signaling capabilities (CC:0066.01.05.11.001).
// A mask with only Visual supported requests type 2 and does not invent a request for Audible.
TEST_F(BarrierOperatorTest, VisualOnlyCapabilitiesRequestVisualSubsystem)
{
    start_interview();
    constexpr uint8_t SUPPORTED_VISUAL_ONLY = 0x02;  // Bit 1 maps to subsystem type 2 (Visual).
    ASSERT_EQ(SL_STATUS_OK, receive_report(std::array<uint8_t, SIGNAL_SUPPORTED_REPORT_FRAME_SIZE> {BARRIER_OPERATOR_CC_ID, SIGNAL_SUPPORTED_REPORT_COMMAND, SUPPORTED_VISUAL_ONLY}));

    auto get_group = endpoint.child_by_type(EVENT_SIGNALING_GET_GROUP);
    ASSERT_TRUE(get_group.is_valid());
    EXPECT_EQ(SUBSYSTEM_VISUAL, get_group.child_by_type(EVENT_SIGNALING_GET_TYPE).desired<uint8_t>());
    EXPECT_FALSE(component().find_report_group_by_subsystem_type(endpoint, SUBSYSTEM_AUDIBLE).is_valid());
}

// Barrier Operator reports received outside interview do not start interview-only subsystem Gets.
// The parsed callbacks store unsolicited reports but reserve interview work for an open interview.
TEST_F(BarrierOperatorTest, ReportsOutsideInterviewDoNotQueueSubsystemGets)
{
    auto &store = static_cast<command_class_barrier_operator_attribute_store &>(component());
    const command_class_barrier_operator_attribute_map_t capabilities {{"bit_mask", std::vector<uint8_t> {SUPPORTED_AUDIBLE_AND_VISUAL}}};
    ASSERT_EQ(SL_STATUS_OK, store.on_barrier_operator_signal_supported_report_received_store(endpoint, capabilities));
    ASSERT_EQ(SL_STATUS_OK, component().on_barrier_operator_signal_supported_report_parsed(nullptr, endpoint, capabilities));
    EXPECT_FALSE(endpoint.child_by_type(EVENT_SIGNALING_GET_GROUP).is_valid());

    const command_class_barrier_operator_attribute_map_t signal_report {{"subsystem_type", SUBSYSTEM_AUDIBLE}, {"subsystem_state", SUBSYSTEM_OFF}};
    ASSERT_EQ(SL_STATUS_OK, store.on_barrier_operator_event_signaling_report_received_store(endpoint, signal_report));
    ASSERT_EQ(SL_STATUS_OK, component().on_barrier_operator_event_signaling_report_parsed(nullptr, endpoint, signal_report));
    EXPECT_FALSE(endpoint.child_by_type(EVENT_SIGNALING_GET_GROUP).is_valid());
}

// Spec 2026B Barrier Operator v1 interview and Event Signaling Report behavior.
// A subsystem report received ahead of its turn is stored and skipped when the interview advances.
TEST_F(BarrierOperatorTest, UnsolicitedSubsystemReportIsStoredAndSkipped)
{
    start_interview();
    ASSERT_EQ(SL_STATUS_OK, receive_report(std::array<uint8_t, SIGNAL_SUPPORTED_REPORT_FRAME_SIZE> {BARRIER_OPERATOR_CC_ID, SIGNAL_SUPPORTED_REPORT_COMMAND, SUPPORTED_AUDIBLE_AND_VISUAL}));
    auto get_group = endpoint.child_by_type(EVENT_SIGNALING_GET_GROUP);
    ASSERT_TRUE(get_group.is_valid());
    auto get_type = get_group.child_by_type(EVENT_SIGNALING_GET_TYPE);
    ASSERT_EQ(SUBSYSTEM_AUDIBLE, get_type.desired<uint8_t>());

    // Visual arrives before Audible; it satisfies Visual's report requirement without advancing the active Audible request.
    ASSERT_EQ(SL_STATUS_OK, receive_report(std::array<uint8_t, EVENT_SIGNAL_REPORT_FRAME_SIZE> {BARRIER_OPERATOR_CC_ID, EVENT_SIGNALING_REPORT_COMMAND, SUBSYSTEM_VISUAL, SUBSYSTEM_ON}));
    EXPECT_EQ(SUBSYSTEM_AUDIBLE, get_type.desired<uint8_t>());

    // The Audible response advances past Visual because the Visual state was already reported.
    ASSERT_EQ(SL_STATUS_OK, receive_report(std::array<uint8_t, EVENT_SIGNAL_REPORT_FRAME_SIZE> {BARRIER_OPERATOR_CC_ID, EVENT_SIGNALING_REPORT_COMMAND, SUBSYSTEM_AUDIBLE, SUBSYSTEM_OFF}));
    EXPECT_TRUE(component().has_report_for_subsystem_type(endpoint, SUBSYSTEM_AUDIBLE));
    EXPECT_TRUE(component().has_report_for_subsystem_type(endpoint, SUBSYSTEM_VISUAL));

    // The remaining Barrier Report completes the known interview requirements.
    ASSERT_EQ(SL_STATUS_OK, receive_report(std::array<uint8_t, BARRIER_REPORT_FRAME_SIZE> {BARRIER_OPERATOR_CC_ID, BARRIER_OPERATOR_REPORT_COMMAND, BARRIER_STATE_OPEN}));
}

// Barrier Operator capability storage helper behavior.
// An absent or unreported bitmask means no subsystem support has been discovered yet.
TEST_F(BarrierOperatorTest, SupportedSubsystemsAreEmptyUntilCapabilitiesArrive)
{
    EXPECT_TRUE(component().get_supported_subsystems(endpoint).empty());
    auto report_group = endpoint.emplace_node(SIGNAL_SUPPORTED_REPORT_GROUP);
    report_group.emplace_node(SIGNAL_SUPPORTED_REPORT_MASK);
    EXPECT_TRUE(component().get_supported_subsystems(endpoint).empty());
}

// A subsystem is unresolved if its report group does not exist or has no reported state.
TEST_F(BarrierOperatorTest, SubsystemReportRequiresReportedState)
{
    EXPECT_FALSE(component().has_report_for_subsystem_type(endpoint, SUBSYSTEM_AUDIBLE));
    auto report_group = endpoint.emplace_node(EVENT_SIGNALING_REPORT_GROUP);
    report_group.emplace_node(EVENT_SIGNALING_REPORT_TYPE).set_reported(SUBSYSTEM_AUDIBLE);
    EXPECT_FALSE(component().has_report_for_subsystem_type(endpoint, SUBSYSTEM_AUDIBLE));
}

// Capability bitmaps longer than one byte are accepted and ignored for v1's known subsystem IDs.
// Reserved high bits do not cause the controller to invent Event Signaling Gets.
TEST_F(BarrierOperatorTest, CapabilityReportIgnoresUnknownV1SubsystemBits)
{
    start_interview();
    constexpr uint8_t UNKNOWN_SUBSYSTEM_BITS = 0xFC;  // Bits 2..7 correspond to subsystem types outside v1's known types.
    ASSERT_EQ(SL_STATUS_OK, receive_report(std::array<uint8_t, SIGNAL_SUPPORTED_REPORT_FRAME_SIZE + 1> {BARRIER_OPERATOR_CC_ID, SIGNAL_SUPPORTED_REPORT_COMMAND, SUBSYSTEM_OFF, UNKNOWN_SUBSYSTEM_BITS}));
    EXPECT_FALSE(endpoint.child_by_type(EVENT_SIGNALING_GET_GROUP).is_valid());
    EXPECT_TRUE(component().get_supported_subsystems(endpoint).size() > 1);
}

// Capability discovery requires a one-byte bitmap containing the Visual support bit.
// A longer report can advertise Visual in its first byte and still queues its report.
TEST_F(BarrierOperatorTest, CapabilityReportUsesFirstBitmapByteForVisual)
{
    start_interview();
    constexpr uint8_t VISUAL_BIT_MASK = 0x02;
    ASSERT_EQ(SL_STATUS_OK, receive_report(std::array<uint8_t, SIGNAL_SUPPORTED_REPORT_FRAME_SIZE + 1> {BARRIER_OPERATOR_CC_ID, SIGNAL_SUPPORTED_REPORT_COMMAND, VISUAL_BIT_MASK, 0x00}));
    auto get_group = endpoint.child_by_type(EVENT_SIGNALING_GET_GROUP);
    ASSERT_TRUE(get_group.is_valid());
    EXPECT_EQ(SUBSYSTEM_VISUAL, get_group.child_by_type(EVENT_SIGNALING_GET_TYPE).desired<uint8_t>());
}

// Capability interview skips a subsystem report that was already stored before the mask arrived.
TEST_F(BarrierOperatorTest, CapabilityReportSkipsPreviouslyReportedSubsystem)
{
    start_interview();
    auto &store = static_cast<command_class_barrier_operator_attribute_store &>(component());
    ASSERT_EQ(SL_STATUS_OK, store.on_barrier_operator_event_signaling_report_received_store(endpoint, {{"subsystem_type", SUBSYSTEM_AUDIBLE}, {"subsystem_state", SUBSYSTEM_OFF}}));
    ASSERT_TRUE(component().has_report_for_subsystem_type(endpoint, SUBSYSTEM_AUDIBLE));
    ASSERT_EQ(SL_STATUS_OK, receive_report(std::array<uint8_t, SIGNAL_SUPPORTED_REPORT_FRAME_SIZE> {BARRIER_OPERATOR_CC_ID, SIGNAL_SUPPORTED_REPORT_COMMAND, SUPPORTED_AUDIBLE_AND_VISUAL}));
    auto get_group = endpoint.child_by_type(EVENT_SIGNALING_GET_GROUP);
    ASSERT_TRUE(get_group.is_valid());
    EXPECT_EQ(SUBSYSTEM_VISUAL, get_group.child_by_type(EVENT_SIGNALING_GET_TYPE).desired<uint8_t>());
}

// Interview requires report attributes only for subsystem types advertised by the device.
// If neither known type is supported, no per-subsystem requirements are created.
TEST_F(BarrierOperatorTest, CapabilityReportDoesNotRequireUnknownSubsystemReports)
{
    start_interview();
    constexpr uint8_t UNKNOWN_SUBSYSTEM_BITS = 0xFC;
    ASSERT_EQ(SL_STATUS_OK, receive_report(std::array<uint8_t, SIGNAL_SUPPORTED_REPORT_FRAME_SIZE> {BARRIER_OPERATOR_CC_ID, SIGNAL_SUPPORTED_REPORT_COMMAND, UNKNOWN_SUBSYSTEM_BITS}));
    EXPECT_TRUE(endpoint.children(EVENT_SIGNALING_REPORT_GROUP).empty());
    EXPECT_FALSE(endpoint.child_by_type(EVENT_SIGNALING_GET_GROUP).is_valid());
}

// Discovery stores a valid Event Signaling Report before its capabilities bitmap arrives.
// Later capability processing skips the already resolved subsystem.
TEST_F(BarrierOperatorTest, SubsystemReportBeforeCapabilitiesIsStored)
{
    start_interview();
    ASSERT_EQ(SL_STATUS_OK, receive_report(std::array<uint8_t, EVENT_SIGNAL_REPORT_FRAME_SIZE> {BARRIER_OPERATOR_CC_ID, EVENT_SIGNALING_REPORT_COMMAND, SUBSYSTEM_AUDIBLE, SUBSYSTEM_ON}));
    EXPECT_TRUE(component().has_report_for_subsystem_type(endpoint, SUBSYSTEM_AUDIBLE));
    ASSERT_EQ(SL_STATUS_OK, receive_report(std::array<uint8_t, SIGNAL_SUPPORTED_REPORT_FRAME_SIZE> {BARRIER_OPERATOR_CC_ID, SIGNAL_SUPPORTED_REPORT_COMMAND, SUPPORTED_AUDIBLE_AND_VISUAL}));
    auto get_group = endpoint.child_by_type(EVENT_SIGNALING_GET_GROUP);
    ASSERT_TRUE(get_group.is_valid());
    EXPECT_EQ(SUBSYSTEM_VISUAL, get_group.child_by_type(EVENT_SIGNALING_GET_TYPE).desired<uint8_t>());
}

// Signal Supported Report accepts the full 32-byte protocol maximum.
TEST_F(BarrierOperatorTest, AttributeStoreAcceptsMaximumCapabilitiesMask)
{
    auto &store                                     = static_cast<command_class_barrier_operator_attribute_store &>(component());
    constexpr size_t MAXIMUM_CAPABILITIES_MASK_SIZE = 32;
    EXPECT_EQ(SL_STATUS_OK, store.on_barrier_operator_signal_supported_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t>(MAXIMUM_CAPABILITIES_MASK_SIZE, 0)}}));
    EXPECT_EQ(MAXIMUM_CAPABILITIES_MASK_SIZE, component().get_supported_subsystems(endpoint).size());
}

// A complete Barrier Report stores the reported position, including the CLOSED endpoint value.
TEST_F(BarrierOperatorTest, AttributeStoreAcceptsClosedBarrierReport)
{
    auto &store = static_cast<command_class_barrier_operator_attribute_store &>(component());
    EXPECT_EQ(SL_STATUS_OK, store.on_barrier_operator_report_received_store(endpoint, {{"state", TARGET_VALUE_CLOSE}}));
    auto report_group = endpoint.child_by_type(REPORT_GROUP);
    ASSERT_TRUE(report_group.is_valid());
    EXPECT_EQ(TARGET_VALUE_CLOSE, report_group.child_by_type(REPORT_STATE).reported<uint8_t>());
}

// A second valid report for the same subsystem updates its existing report group.
TEST_F(BarrierOperatorTest, EventSignalingReportUpdatesExistingGroup)
{
    auto &store = static_cast<command_class_barrier_operator_attribute_store &>(component());
    ASSERT_EQ(SL_STATUS_OK, store.on_barrier_operator_event_signaling_report_received_store(endpoint, {{"subsystem_type", SUBSYSTEM_AUDIBLE}, {"subsystem_state", SUBSYSTEM_OFF}}));
    auto first_group = component().find_report_group_by_subsystem_type(endpoint, SUBSYSTEM_AUDIBLE);
    ASSERT_TRUE(first_group.is_valid());
    ASSERT_EQ(SL_STATUS_OK, store.on_barrier_operator_event_signaling_report_received_store(endpoint, {{"subsystem_type", SUBSYSTEM_AUDIBLE}, {"subsystem_state", SUBSYSTEM_ON}}));
    auto groups = endpoint.children(EVENT_SIGNALING_REPORT_GROUP);
    ASSERT_EQ(1U, groups.size());
    EXPECT_EQ(SUBSYSTEM_ON, groups.front().child_by_type(EVENT_SIGNALING_REPORT_STATE).reported<uint8_t>());
}

// Event Signaling Get rejects a subsystem type that is outside the supported v1 type IDs.
TEST_F(BarrierOperatorTest, EventSignalingGetRejectsUnknownSubsystemType)
{
    set_version();
    auto group = endpoint.emplace_node(EVENT_SIGNALING_GET_GROUP);
    group.emplace_node(EVENT_SIGNALING_GET_TYPE).set_desired(UNSUPPORTED_SUBSYSTEM_TYPE);
    std::array<uint8_t, FRAME_CAPACITY> frame {};
    uint16_t length = 0;
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, assemble_get(group, EVENT_SIGNALING_GET_COMMAND, frame.data(), &length));
}

// Event Signal Set requires the subsystem type before a frame can be assembled.
// The state alone cannot form a v1 payload.
TEST_F(BarrierOperatorTest, EventSignalSetWaitsForSubsystemType)
{
    set_version();
    auto group = endpoint.emplace_node(EVENT_SIGNAL_SET_GROUP);
    group.emplace_node(EVENT_SIGNAL_SET_STATE).set_desired(SUBSYSTEM_OFF);
    std::array<uint8_t, FRAME_CAPACITY> frame {};
    uint16_t length = 0;
    zwave_frame_generator generator {BARRIER_OPERATOR_CC_ID};
    generator.initialize_frame(EVENT_SIGNAL_SET_COMMAND, frame.data(), &length);
    command_class_barrier_operator_core::set_requested_args args {group, &generator};
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_barrier_operator_event_signal_set_requested(args, frame.data(), &length));
}

// Event Signal Set requires the state before a frame can be assembled.
// The subsystem type alone cannot form a v1 payload.
TEST_F(BarrierOperatorTest, EventSignalSetWaitsForSubsystemState)
{
    set_version();
    auto group = endpoint.emplace_node(EVENT_SIGNAL_SET_GROUP);
    group.emplace_node(EVENT_SIGNAL_SET_TYPE).set_desired(SUBSYSTEM_AUDIBLE);
    std::array<uint8_t, FRAME_CAPACITY> frame {};
    uint16_t length = 0;
    zwave_frame_generator generator {BARRIER_OPERATOR_CC_ID};
    generator.initialize_frame(EVENT_SIGNAL_SET_COMMAND, frame.data(), &length);
    command_class_barrier_operator_core::set_requested_args args {group, &generator};
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_barrier_operator_event_signal_set_requested(args, frame.data(), &length));
}

// Event Signal Set rejects a subsystem type outside the two defined v1 types.
TEST_F(BarrierOperatorTest, EventSignalSetRejectsUnknownSubsystemType)
{
    set_version();
    auto group = endpoint.emplace_node(EVENT_SIGNAL_SET_GROUP);
    group.emplace_node(EVENT_SIGNAL_SET_TYPE).set_desired(UNSUPPORTED_SUBSYSTEM_TYPE);
    group.emplace_node(EVENT_SIGNAL_SET_STATE).set_desired(SUBSYSTEM_OFF);
    std::array<uint8_t, FRAME_CAPACITY> frame {};
    uint16_t length = 0;
    zwave_frame_generator generator {BARRIER_OPERATOR_CC_ID};
    generator.initialize_frame(EVENT_SIGNAL_SET_COMMAND, frame.data(), &length);
    command_class_barrier_operator_core::set_requested_args args {group, &generator};
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, component().on_barrier_operator_event_signal_set_requested(args, frame.data(), &length));
}

// A report group lacking a subsystem type cannot satisfy a typed Event Signaling Get request.
// The lookup continues and returns the separately stored, correctly typed group.
TEST_F(BarrierOperatorTest, SubsystemLookupSkipsGroupWithoutType)
{
    auto malformed_group = endpoint.emplace_node(EVENT_SIGNALING_REPORT_GROUP);
    malformed_group.emplace_node(EVENT_SIGNALING_REPORT_STATE).set_reported(SUBSYSTEM_OFF);
    auto &store = static_cast<command_class_barrier_operator_attribute_store &>(component());
    ASSERT_EQ(SL_STATUS_OK, store.on_barrier_operator_event_signaling_report_received_store(endpoint, {{"subsystem_type", SUBSYSTEM_AUDIBLE}, {"subsystem_state", SUBSYSTEM_ON}}));
    auto matching_group = component().find_report_group_by_subsystem_type(endpoint, SUBSYSTEM_AUDIBLE);
    ASSERT_TRUE(matching_group.is_valid());
    EXPECT_EQ(SUBSYSTEM_AUDIBLE, matching_group.child_by_type(EVENT_SIGNALING_REPORT_TYPE).reported<uint8_t>());
}

// A valid report-group type without a reported value is skipped during typed subsystem lookup.
TEST_F(BarrierOperatorTest, SubsystemLookupSkipsUnreportedType)
{
    auto incomplete_group = endpoint.emplace_node(EVENT_SIGNALING_REPORT_GROUP);
    incomplete_group.emplace_node(EVENT_SIGNALING_REPORT_TYPE);
    auto &store = static_cast<command_class_barrier_operator_attribute_store &>(component());
    ASSERT_EQ(SL_STATUS_OK, store.on_barrier_operator_event_signaling_report_received_store(endpoint, {{"subsystem_type", SUBSYSTEM_AUDIBLE}, {"subsystem_state", SUBSYSTEM_OFF}}));
    auto matching_group = component().find_report_group_by_subsystem_type(endpoint, SUBSYSTEM_AUDIBLE);
    ASSERT_TRUE(matching_group.is_valid());
    EXPECT_TRUE(matching_group.child_by_type(EVENT_SIGNALING_REPORT_TYPE).reported_exists());
}

// Report lookup returns an existing matching group instead of adding a duplicate.
TEST_F(BarrierOperatorTest, SubsystemReportUsesExistingGroup)
{
    auto existing_group = endpoint.emplace_node(EVENT_SIGNALING_REPORT_GROUP);
    existing_group.emplace_node(EVENT_SIGNALING_REPORT_TYPE).set_reported(SUBSYSTEM_AUDIBLE);
    auto &store = static_cast<command_class_barrier_operator_attribute_store &>(component());
    ASSERT_EQ(SL_STATUS_OK, store.on_barrier_operator_event_signaling_report_received_store(endpoint, {{"subsystem_type", SUBSYSTEM_AUDIBLE}, {"subsystem_state", SUBSYSTEM_ON}}));
    EXPECT_EQ(1U, endpoint.children(EVENT_SIGNALING_REPORT_GROUP).size());
    EXPECT_EQ(SUBSYSTEM_ON, existing_group.child_by_type(EVENT_SIGNALING_REPORT_STATE).reported<uint8_t>());
}

// A valid reported subsystem type without a state value is not treated as a complete report.
TEST_F(BarrierOperatorTest, SubsystemTypeWithoutStateDoesNotCompleteReport)
{
    auto report_group = endpoint.emplace_node(EVENT_SIGNALING_REPORT_GROUP);
    report_group.emplace_node(EVENT_SIGNALING_REPORT_TYPE).set_reported(SUBSYSTEM_AUDIBLE);
    EXPECT_FALSE(component().has_report_for_subsystem_type(endpoint, SUBSYSTEM_AUDIBLE));
}

// An unsupported advertised subsystem is skipped while discovery proceeds to a later known type.
// The v1 controller recognizes type 1 and 2, so this is represented by an unknown bit preceding type 2.
TEST_F(BarrierOperatorTest, CapabilityDiscoverySkipsUnknownTypeBeforeVisual)
{
    start_interview();
    constexpr uint8_t VISUAL_AND_UNKNOWN_FIRST = 0x06;  // Bits 1 and 2; type 3 is unknown, type 2 is known.
    ASSERT_EQ(SL_STATUS_OK, receive_report(std::array<uint8_t, SIGNAL_SUPPORTED_REPORT_FRAME_SIZE> {BARRIER_OPERATOR_CC_ID, SIGNAL_SUPPORTED_REPORT_COMMAND, VISUAL_AND_UNKNOWN_FIRST}));
    auto get_group = endpoint.child_by_type(EVENT_SIGNALING_GET_GROUP);
    ASSERT_TRUE(get_group.is_valid());
    EXPECT_EQ(SUBSYSTEM_VISUAL, get_group.child_by_type(EVENT_SIGNALING_GET_TYPE).desired<uint8_t>());
}

// MQTT Set rejects non-object JSON payloads before reading its required Target Value.
TEST_F(BarrierOperatorTest, MqttSetRejectsNonObjectPayload)
{
    auto endpoint_node = endpoint;
    auto &mqtt         = static_cast<command_class_barrier_operator_mqtt &>(component());
    EXPECT_NE(SL_STATUS_OK, mqtt.mqtt_on_barrier_operator_set_command(endpoint_node, "null"));
    EXPECT_FALSE(endpoint.child_by_type(SET_GROUP).is_valid());
}

// MQTT Set rejects a well-formed object that omits the required Target Value.
TEST_F(BarrierOperatorTest, MqttSetRejectsMissingTargetValue)
{
    auto endpoint_node = endpoint;
    auto &mqtt         = static_cast<command_class_barrier_operator_mqtt &>(component());
    EXPECT_NE(SL_STATUS_OK, mqtt.mqtt_on_barrier_operator_set_command(endpoint_node, "{}"));
    EXPECT_FALSE(endpoint.child_by_type(SET_GROUP).is_valid());
}

// MQTT Event Signal Set rejects a well-formed object missing either required field.
TEST_F(BarrierOperatorTest, MqttEventSignalSetRejectsMissingRequiredField)
{
    auto endpoint_node = endpoint;
    auto &mqtt         = static_cast<command_class_barrier_operator_mqtt &>(component());
    EXPECT_NE(SL_STATUS_OK, mqtt.mqtt_on_barrier_operator_event_signal_set_command(endpoint_node, R"({"subsystem_type":"0x01"})"));
    EXPECT_FALSE(endpoint.child_by_type(EVENT_SIGNAL_SET_GROUP).is_valid());
}

// MQTT Event Signaling Get rejects payloads that are not JSON objects before parsing type.
TEST_F(BarrierOperatorTest, MqttEventSignalingGetRejectsNonObjectPayload)
{
    auto endpoint_node = endpoint;
    auto &mqtt         = static_cast<command_class_barrier_operator_mqtt &>(component());
    EXPECT_NE(SL_STATUS_OK, mqtt.mqtt_on_barrier_operator_event_signaling_get_command(endpoint_node, "null"));
    EXPECT_FALSE(endpoint.child_by_type(EVENT_SIGNALING_GET_GROUP).is_valid());
}

// Spec 2026B Barrier Operator v1 report validation (CC:0066.01.03.11.003, CC:0066.01.05.11.002).
// Reserved barrier positions and bitmask lengths outside 1..32 bytes are rejected.
TEST_F(BarrierOperatorTest, AttributeStoreRejectsReservedReportValues)
{
    auto &store = static_cast<command_class_barrier_operator_attribute_store &>(component());
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, store.on_barrier_operator_report_received_store(endpoint, {{"state", RESERVED_BARRIER_STATE}}));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, store.on_barrier_operator_report_received_store(endpoint, {}));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, store.on_barrier_operator_signal_supported_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {}}}));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, store.on_barrier_operator_signal_supported_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t>(33, 0)}}));
    EXPECT_FALSE(endpoint.child_by_type(REPORT_GROUP).is_valid());
    EXPECT_FALSE(endpoint.child_by_type(SIGNAL_SUPPORTED_REPORT_GROUP).is_valid());
}

// Spec 2026B Barrier Operator Event Signaling Report fields.
// Reserved subsystem types and states are rejected without creating a report group.
TEST_F(BarrierOperatorTest, AttributeStoreRejectsReservedEventReportValues)
{
    auto &store = static_cast<command_class_barrier_operator_attribute_store &>(component());
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, store.on_barrier_operator_event_signaling_report_received_store(endpoint, {{"subsystem_type", RESERVED_SUBSYSTEM_TYPE}, {"subsystem_state", SUBSYSTEM_ON}}));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, store.on_barrier_operator_event_signaling_report_received_store(endpoint, {{"subsystem_type", SUBSYSTEM_AUDIBLE}, {"subsystem_state", RESERVED_SUBSYSTEM_STATE}}));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, store.on_barrier_operator_event_signaling_report_received_store(endpoint, {{"subsystem_type", SUBSYSTEM_AUDIBLE}}));
    EXPECT_TRUE(endpoint.children(EVENT_SIGNALING_REPORT_GROUP).empty());
}

// Spec 2026B Barrier Operator v1 control handler command IDs.
// Unknown command identifiers are rejected by the generated dispatcher.
TEST_F(BarrierOperatorTest, ControlHandlerRejectsUnknownCommand)
{
    EXPECT_EQ(SL_STATUS_NOT_SUPPORTED, receive_report(std::array<uint8_t, EMPTY_GET_FRAME_SIZE> {BARRIER_OPERATOR_CC_ID, UNKNOWN_COMMAND_ID}));
}

// Spec 2026B Barrier Operator Report state field (CC:0066.01.03.11.001).
// A Report missing its required state byte fails parsing and does not create stored state.
TEST_F(BarrierOperatorTest, ReportWithoutStateByteIsRejected)
{
    const std::array<uint8_t, EMPTY_GET_FRAME_SIZE> truncated_report {BARRIER_OPERATOR_CC_ID, BARRIER_OPERATOR_REPORT_COMMAND};
    EXPECT_EQ(SL_STATUS_FAIL, receive_report(truncated_report));
    EXPECT_FALSE(endpoint.child_by_type(REPORT_GROUP).is_valid());
}

// Spec 2026B Barrier Operator signaling capability length (CC:0066.01.05.11.002).
// A Supported Report without its mandatory bitmask byte is rejected.
TEST_F(BarrierOperatorTest, SupportedReportWithoutMaskByteIsRejected)
{
    const std::array<uint8_t, EMPTY_GET_FRAME_SIZE> truncated_report {BARRIER_OPERATOR_CC_ID, SIGNAL_SUPPORTED_REPORT_COMMAND};
    EXPECT_EQ(SL_STATUS_FAIL, receive_report(truncated_report));
    EXPECT_FALSE(endpoint.child_by_type(SIGNAL_SUPPORTED_REPORT_GROUP).is_valid());
}

// Barrier Operator Event Signaling Report contains both a subsystem type and state.
// A truncated payload is rejected without storing an incomplete report group.
TEST_F(BarrierOperatorTest, EventSignalingReportWithoutStateByteIsRejected)
{
    constexpr size_t TRUNCATED_EVENT_REPORT_SIZE = 3;  // Class, command, and type; the state byte is absent.
    const std::array<uint8_t, TRUNCATED_EVENT_REPORT_SIZE> truncated_report {BARRIER_OPERATOR_CC_ID, EVENT_SIGNALING_REPORT_COMMAND, SUBSYSTEM_AUDIBLE};
    EXPECT_EQ(SL_STATUS_FAIL, receive_report(truncated_report));
    EXPECT_TRUE(endpoint.children(EVENT_SIGNALING_REPORT_GROUP).empty());
}

// Spec 2026B Barrier Operator MQTT commands.
// Command callbacks create resolver groups with the validated desired fields for each command.
TEST_F(BarrierOperatorTest, MqttCommandsCreateExpectedRequestGroups)
{
    auto endpoint_node = endpoint;
    auto &mqtt         = static_cast<command_class_barrier_operator_mqtt &>(component());
    ASSERT_EQ(SL_STATUS_OK, mqtt.mqtt_on_barrier_operator_get_command(endpoint_node, ""));
    ASSERT_EQ(SL_STATUS_OK, mqtt.mqtt_on_barrier_operator_signal_supported_get_command(endpoint_node, ""));
    ASSERT_EQ(SL_STATUS_OK, mqtt.mqtt_on_barrier_operator_event_signal_set_command(endpoint_node, R"({"subsystem_type":"0x02","subsystem_state":"0xFF"})"));
    ASSERT_EQ(SL_STATUS_OK, mqtt.mqtt_on_barrier_operator_event_signaling_get_command(endpoint_node, R"({"subsystem_type":"0x01"})"));

    EXPECT_TRUE(endpoint.child_by_type(GET_GROUP).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(SIGNAL_SUPPORTED_GET_GROUP).is_valid());
    auto set_group = endpoint.child_by_type(EVENT_SIGNAL_SET_GROUP);
    ASSERT_TRUE(set_group.is_valid());
    EXPECT_EQ(SUBSYSTEM_VISUAL, set_group.child_by_type(EVENT_SIGNAL_SET_TYPE).desired<uint8_t>());
    EXPECT_EQ(SUBSYSTEM_ON, set_group.child_by_type(EVENT_SIGNAL_SET_STATE).desired<uint8_t>());
    auto get_group = endpoint.child_by_type(EVENT_SIGNALING_GET_GROUP);
    ASSERT_TRUE(get_group.is_valid());
    EXPECT_EQ(SUBSYSTEM_AUDIBLE, get_group.child_by_type(EVENT_SIGNALING_GET_TYPE).desired<uint8_t>());
}

// Spec 2026B Barrier Operator MQTT Event Signal Set fields.
// Invalid subsystem state and malformed payloads are rejected without creating a command group.
TEST_F(BarrierOperatorTest, MqttEventSignalSetRejectsInvalidPayload)
{
    auto endpoint_node = endpoint;
    auto &mqtt         = static_cast<command_class_barrier_operator_mqtt &>(component());
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, mqtt.mqtt_on_barrier_operator_event_signal_set_command(endpoint_node, R"({"subsystem_type":"0x01","subsystem_state":"0x01"})"));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, mqtt.mqtt_on_barrier_operator_event_signal_set_command(endpoint_node, R"({"subsystem_type":"0x03","subsystem_state":"0xFF"})"));
    EXPECT_NE(SL_STATUS_OK, mqtt.mqtt_on_barrier_operator_event_signal_set_command(endpoint_node, "{invalid"));
    EXPECT_FALSE(endpoint.child_by_type(EVENT_SIGNAL_SET_GROUP).is_valid());
}

// Spec 2026B Barrier Operator Event Signaling Get subsystem type.
// MQTT rejects a reserved subsystem type without creating a resolver group.
TEST_F(BarrierOperatorTest, MqttEventSignalingGetRejectsReservedSubsystemType)
{
    auto endpoint_node = endpoint;
    auto &mqtt         = static_cast<command_class_barrier_operator_mqtt &>(component());
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, mqtt.mqtt_on_barrier_operator_event_signaling_get_command(endpoint_node, R"({"subsystem_type":"0x03"})"));
    EXPECT_FALSE(endpoint.child_by_type(EVENT_SIGNALING_GET_GROUP).is_valid());
}
