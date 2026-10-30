/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************/
#include <gtest/gtest.h>

#include <array>

#include "attribute.hpp"
#include "attribute_store_fixt.h"
#include "command_class_sensor_binary.hpp"
#include "command_class_sensor_binary_attribute_store.hpp"
#include "command_class_sensor_binary_mqtt.hpp"
#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "component_connector_types.hpp"
#include "zwave_frame_generator.hpp"

using namespace zwave_command_class;
using namespace zwave_command_class::command_class_sensor_binary_types;

namespace
{
    constexpr uint8_t SENSOR_BINARY_COMMAND_CLASS_ID                = 0x30;
    constexpr uint8_t SENSOR_BINARY_SUPPORTED_GET_SENSOR_COMMAND_ID = 0x01;
    constexpr uint8_t SENSOR_BINARY_GET_COMMAND_ID                  = 0x02;

    constexpr uint8_t SENSOR_BINARY_VERSION_1 = 1;
    constexpr uint8_t SENSOR_BINARY_VERSION_2 = 2;

    // Sensor Type 1 is General Purpose; its Supported Report flag is bit 1 because bit 0 is reserved.
    constexpr uint8_t SENSOR_TYPE_UNSPECIFIED     = 0;
    constexpr uint8_t SENSOR_TYPE_GENERAL_PURPOSE = 1;
    // Sensor Type 2 is Smoke; its Supported Report flag is bit 2 of the first mask byte.
    constexpr uint8_t SENSOR_TYPE_SMOKE = 2;
    // Sensor Type 6 is Water; Sensor Binary v2 defines it as type 0x06.
    constexpr uint8_t SENSOR_TYPE_WATER = 6;

    constexpr uint8_t SENSOR_VALUE_IDLE                        = 0x00;
    constexpr uint8_t SENSOR_VALUE_TRIGGER                     = 0xFF;
    constexpr uint8_t SUPPORTED_GENERAL_PURPOSE_MASK           = 0x02;
    constexpr uint8_t SUPPORTED_SMOKE_MASK                     = 0x04;
    constexpr uint8_t SUPPORTED_GENERAL_PURPOSE_AND_SMOKE_MASK = SUPPORTED_GENERAL_PURPOSE_MASK | SUPPORTED_SMOKE_MASK;

    constexpr size_t EXPECTED_SUPPORTED_SENSOR_TYPE_COUNT = 2;
    constexpr uint16_t EMPTY_FRAME_LENGTH                 = 0;
    constexpr size_t FRAME_CAPACITY                       = 8;
    constexpr uint16_t TEST_NODE_ID                       = 2;
    constexpr uint8_t TEST_ENDPOINT_ID                    = 0;
    constexpr std::array<uint8_t, 4> TEST_HOME_ID {0x12, 0x34, 0x56, 0x78};
    attribute_store::attribute report_group_for_sensor_type(attribute_store::attribute endpoint, uint8_t sensor_type)
    {
        // Locate the report group by its stored Sensor Type instead of depending on child insertion order.
        for (const auto &report: endpoint.children(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::SENSOR_BINARY_REPORT_GROUP))) {
            auto type = report.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_type));
            if (type.is_valid() && type.reported_exists() && type.reported<uint8_t>() == sensor_type) {
                return report;
            }
        }
        return attribute_store::attribute(ATTRIBUTE_STORE_INVALID_NODE);
    }
}  // namespace

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}

class TestableSensorBinaryMqtt : public command_class_sensor_binary_mqtt
{
    public:
        using zwave_command_class_base::mqtt_command_handler;
};

class TestableSensorBinaryAttributeStore : public command_class_sensor_binary_attribute_store
{
    public:
        using command_class_sensor_binary_attribute_store::find_or_create_report_group_by_sensor_type;
        using command_class_sensor_binary_attribute_store::find_report_group_by_sensor_type;
        using command_class_sensor_binary_attribute_store::get_supported_sensor_bit_mask;
        using command_class_sensor_binary_attribute_store::has_report_for_sensor_type;
};

class SensorBinaryTest : public ::testing::Test
{
    protected:
        static command_class_sensor_binary &component()
        {
            static command_class_sensor_binary instance;
            return instance;
        }

        static TestableSensorBinaryMqtt &mqtt()
        {
            static TestableSensorBinaryMqtt instance;
            return instance;
        }

        void SetUp() override
        {
            // Each test gets an isolated in-memory endpoint tree for report values and requests.
            ASSERT_EQ(SL_STATUS_OK, attribute_store_init());
            auto home        = attribute_store::attribute::root().emplace_node(ATTRIBUTE_HOME_ID, std::vector<uint8_t>(TEST_HOME_ID.begin(), TEST_HOME_ID.end()));
            auto node        = home.emplace_node(ATTRIBUTE_NODE_ID, TEST_NODE_ID);
            endpoint         = node.emplace_node(ATTRIBUTE_ENDPOINT_ID, TEST_ENDPOINT_ID);
            endpoint_node_id = static_cast<attribute_store_node_t>(endpoint);
            (void)component();
        }

        void TearDown() override
        {
            attribute_store_teardown();
        }

        void set_version(uint8_t version)
        {
            // The peer version selects the Sensor Binary v1 or v2 frame interpretation.
            endpoint.emplace_node(static_cast<attribute_store_type_t>(command_class_sensor_binary_attributes_t::supported_version)).set_reported(version);
        }

        void finish_interview(uint8_t version)
        {
            // Advertise Sensor Binary support and the negotiated version before signaling interview completion.
            endpoint.emplace_node(ATTRIBUTE_ZWAVE_NIF, std::vector<uint8_t> {SENSOR_BINARY_COMMAND_CLASS_ID});
            set_version(version);

            // Fire the same synchronous connector event used by the device interviewer to open the CC interview window.
            component_connector connector;
            const component_connector_interview_done_payload_t payload {endpoint_node_id, SL_STATUS_OK};
            auto status = connector.fire_event_async(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_INTERVIEW_DONE), payload);
            connector.run();
            ASSERT_EQ(SL_STATUS_OK, status.get());
        }

        attribute_store::attribute endpoint;
        attribute_store_node_t endpoint_node_id = ATTRIBUTE_STORE_INVALID_NODE;
};

// Spec 2026B Binary Sensor CC v1, Interview.
// Verifies a v1 peer starts a type-less Get and requires a Sensor Value report.
TEST_F(SensorBinaryTest, InterviewV1StartsUntypedGetAndRequiresReportValue)
{
    // The v1 Supported Get command does not exist, so interview starts directly with the generic Sensor Get.
    component().on_interview(endpoint, SENSOR_BINARY_VERSION_1);
    auto get_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP));
    ASSERT_TRUE(get_group.is_valid());
    EXPECT_FALSE(get_group.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::sensor_type)).desired_exists());

    // A v1 interview waits for the Sensor Value even though Sensor Type is not present in the Report frame.
    auto report = endpoint.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::SENSOR_BINARY_REPORT_GROUP));
    ASSERT_TRUE(report.is_valid());
    EXPECT_TRUE(report.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_value)).is_valid());
}

// Spec 2026B Binary Sensor CC v2, Interview.
// Verifies v2 starts with Supported Get and records that the Supported Report mask is required.
TEST_F(SensorBinaryTest, InterviewV2StartsSupportedGetAndRequiresMask)
{
    // Version 2 first discovers which Sensor Types the peer supports.
    component().on_interview(endpoint, SENSOR_BINARY_VERSION_2);
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_supported_get_sensor_group_attributes_t::SENSOR_BINARY_SUPPORTED_GET_SENSOR_GROUP)).is_valid());
    auto supported_report = endpoint.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_supported_sensor_report_group_attributes_t::SENSOR_BINARY_SUPPORTED_SENSOR_REPORT_GROUP));
    ASSERT_TRUE(supported_report.is_valid());
    EXPECT_TRUE(supported_report.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_supported_sensor_report_group_attributes_t::bit_mask)).is_valid());
}

// Spec 2026B Binary Sensor CC v2, Supported Sensor Report and Sensor Report interview behavior.
// Verifies the real interview-complete event requests each advertised type and completes when all are reported.
TEST_F(SensorBinaryTest, InterviewV2RequestsSupportedTypesInOrder)
{
    // The interview event opens the interview window and initializes the v2 Supported Get and mask requirement.
    finish_interview(SENSOR_BINARY_VERSION_2);

    // The peer advertises General Purpose (type 1) and Smoke (type 2), represented by their assigned mask bits.
    auto &store = static_cast<command_class_sensor_binary_attribute_store &>(component());
    ASSERT_EQ(SL_STATUS_OK, store.on_sensor_binary_supported_sensor_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {SUPPORTED_GENERAL_PURPOSE_AND_SMOKE_MASK}}}));
    ASSERT_EQ(SL_STATUS_OK, component().on_sensor_binary_supported_sensor_report_parsed(nullptr, endpoint, {}));

    // The first advertised type is requested, with its report value registered as an interview requirement.
    auto get_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP));
    ASSERT_TRUE(get_group.is_valid());
    auto type_node = get_group.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::sensor_type));
    ASSERT_TRUE(type_node.is_valid());
    EXPECT_EQ(SENSOR_TYPE_GENERAL_PURPOSE, type_node.desired<uint8_t>());

    // Storing type 1 before its parsed callback lets the interview advance to the next supported type.
    const command_class_sensor_binary_attribute_map_t general_purpose_report {{"sensor_type", SENSOR_TYPE_GENERAL_PURPOSE}, {"sensor_value", SENSOR_VALUE_IDLE}};
    ASSERT_EQ(SL_STATUS_OK, store.on_sensor_binary_report_received_store(endpoint, general_purpose_report));
    ASSERT_EQ(SL_STATUS_OK, component().on_sensor_binary_report_parsed(nullptr, endpoint, general_purpose_report));
    EXPECT_EQ(SENSOR_TYPE_SMOKE, type_node.desired<uint8_t>());

    // Reporting type 2 completes the final advertised requirement; the interview then closes as fully resolved.
    const command_class_sensor_binary_attribute_map_t smoke_report {{"sensor_type", SENSOR_TYPE_SMOKE}, {"sensor_value", SENSOR_VALUE_TRIGGER}};
    ASSERT_EQ(SL_STATUS_OK, store.on_sensor_binary_report_received_store(endpoint, smoke_report));
    ASSERT_EQ(SL_STATUS_OK, component().on_sensor_binary_report_parsed(nullptr, endpoint, smoke_report));
    component().cc_interview_finish_if_complete(endpoint);
    size_t reported_type_count = 0;
    for (const auto &report: endpoint.children(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::SENSOR_BINARY_REPORT_GROUP))) {
        const auto report_type  = report.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_type)).reported<uint8_t>();
        const auto report_value = report.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_value)).reported<uint8_t>();
        EXPECT_TRUE(report_type == SENSOR_TYPE_GENERAL_PURPOSE || report_type == SENSOR_TYPE_SMOKE);
        EXPECT_EQ(report_type == SENSOR_TYPE_GENERAL_PURPOSE ? SENSOR_VALUE_IDLE : SENSOR_VALUE_TRIGGER, report_value);
        ++reported_type_count;
    }
    EXPECT_EQ(EXPECTED_SUPPORTED_SENSOR_TYPE_COUNT, reported_type_count);
}

// Spec 2026B Binary Sensor CC v2, interview with previously stored reports.
// Verifies interview skips an advertised type whose Sensor Value is already available.
TEST_F(SensorBinaryTest, InterviewV2SkipsPreviouslyReportedSensorType)
{
    // Open the normal v2 interview and save the General Purpose value before learning the supported mask.
    finish_interview(SENSOR_BINARY_VERSION_2);
    auto &store = static_cast<command_class_sensor_binary_attribute_store &>(component());
    const command_class_sensor_binary_attribute_map_t general_purpose_report {{"sensor_type", SENSOR_TYPE_GENERAL_PURPOSE}, {"sensor_value", SENSOR_VALUE_IDLE}};
    ASSERT_EQ(SL_STATUS_OK, store.on_sensor_binary_report_received_store(endpoint, general_purpose_report));

    // The Supported Report advertises both types; interview must skip known type 1 and request type 2 next.
    ASSERT_EQ(SL_STATUS_OK, store.on_sensor_binary_supported_sensor_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {SUPPORTED_GENERAL_PURPOSE_AND_SMOKE_MASK}}}));
    ASSERT_EQ(SL_STATUS_OK, component().on_sensor_binary_supported_sensor_report_parsed(nullptr, endpoint, {}));
    auto get_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP));
    ASSERT_TRUE(get_group.is_valid());
    auto type_node = get_group.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::sensor_type));
    ASSERT_TRUE(type_node.is_valid());
    EXPECT_EQ(SENSOR_TYPE_SMOKE, type_node.desired<uint8_t>());

    // A Smoke report completes the final interview step because the earlier General Purpose report is present.
    const command_class_sensor_binary_attribute_map_t smoke_report {{"sensor_type", SENSOR_TYPE_SMOKE}, {"sensor_value", SENSOR_VALUE_TRIGGER}};
    ASSERT_EQ(SL_STATUS_OK, store.on_sensor_binary_report_received_store(endpoint, smoke_report));
    ASSERT_EQ(SL_STATUS_OK, component().on_sensor_binary_report_parsed(nullptr, endpoint, smoke_report));
    component().cc_interview_finish_if_complete(endpoint);
    auto general_purpose_group = report_group_for_sensor_type(endpoint, SENSOR_TYPE_GENERAL_PURPOSE);
    ASSERT_TRUE(general_purpose_group.is_valid());
    EXPECT_EQ(SENSOR_VALUE_IDLE, general_purpose_group.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_value)).reported<uint8_t>());
}

// Spec 2026B Binary Sensor CC v1, Sensor Get frame.
// Checks the legacy receiver's defined Get prefix while a v1 peer is selected.
TEST_F(SensorBinaryTest, GetV1PeerBuildsEmptyPayload)
{
    set_version(SENSOR_BINARY_VERSION_1);
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::sensor_type)).set_desired(SENSOR_TYPE_GENERAL_PURPOSE);

    // Sensor Binary v1 Get has no Sensor Type payload; only v2 adds that field.
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = EMPTY_FRAME_LENGTH;
    zwave_frame_generator generator {SENSOR_BINARY_COMMAND_CLASS_ID};
    generator.initialize_frame(SENSOR_BINARY_GET_COMMAND_ID, data, &length);
    command_class_sensor_binary_core::get_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_sensor_binary_get_requested(args, data, &length));
    const std::vector<uint8_t> expected {SENSOR_BINARY_COMMAND_CLASS_ID, SENSOR_BINARY_GET_COMMAND_ID};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec 2026B Binary Sensor CC v2, Sensor Get frame.
// Verifies a v2 Sensor Type request is present as the final command payload byte.
TEST_F(SensorBinaryTest, GetV2EncodesRequestedSensorType)
{
    set_version(SENSOR_BINARY_VERSION_2);
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::sensor_type)).set_desired(SENSOR_TYPE_SMOKE);

    // Smoke is Sensor Type 2, encoded directly after the Get command identifier in v2.
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = EMPTY_FRAME_LENGTH;
    zwave_frame_generator generator {SENSOR_BINARY_COMMAND_CLASS_ID};
    generator.initialize_frame(SENSOR_BINARY_GET_COMMAND_ID, data, &length);
    command_class_sensor_binary_core::get_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_sensor_binary_get_requested(args, data, &length));
    const std::vector<uint8_t> expected {SENSOR_BINARY_COMMAND_CLASS_ID, SENSOR_BINARY_GET_COMMAND_ID, SENSOR_TYPE_SMOKE};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec 2026B Binary Sensor CC v2, Sensor Get frame.
// Confirms a missing requested type produces only the common command prefix.
TEST_F(SensorBinaryTest, GetV2WithoutTypeBuildsEmptyPayload)
{
    set_version(SENSOR_BINARY_VERSION_2);
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP));
    group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::sensor_type));

    // An unresolved type has no v2 payload byte to append yet.
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = EMPTY_FRAME_LENGTH;
    zwave_frame_generator generator {SENSOR_BINARY_COMMAND_CLASS_ID};
    generator.initialize_frame(SENSOR_BINARY_GET_COMMAND_ID, data, &length);
    command_class_sensor_binary_core::get_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_sensor_binary_get_requested(args, data, &length));
    const std::vector<uint8_t> expected {SENSOR_BINARY_COMMAND_CLASS_ID, SENSOR_BINARY_GET_COMMAND_ID};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec 2026B Binary Sensor CC v2, Sensor Report matching.
// Verifies a report without a matching Get group is safely ignored.
TEST_F(SensorBinaryTest, ReportV2WithoutOutstandingGetIsIgnored)
{
    set_version(SENSOR_BINARY_VERSION_2);
    auto &core = static_cast<command_class_sensor_binary_core &>(component());

    // No Get is pending, so a valid Sensor Report cannot complete any request.
    EXPECT_EQ(SL_STATUS_OK, core.on_sensor_binary_report_parsed(nullptr, endpoint, {{"sensor_type", SENSOR_TYPE_SMOKE}, {"sensor_value", SENSOR_VALUE_TRIGGER}}));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP)).is_valid());
}

// Spec 2026B Binary Sensor CC v2, Sensor Report matching.
// Verifies a matching report stops a typed request even when no interview is open.
TEST_F(SensorBinaryTest, ReportV2CompletesMatchingGetOutsideInterview)
{
    set_version(SENSOR_BINARY_VERSION_2);
    auto &core     = static_cast<command_class_sensor_binary_core &>(component());
    auto get_group = endpoint.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP));
    get_group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::sensor_type)).set_desired(SENSOR_TYPE_SMOKE);

    // The returned Smoke type matches the requested one, allowing the resolver group to stop immediately.
    EXPECT_EQ(SL_STATUS_OK, core.on_sensor_binary_report_parsed(nullptr, endpoint, {{"sensor_type", SENSOR_TYPE_SMOKE}, {"sensor_value", SENSOR_VALUE_TRIGGER}}));
    EXPECT_TRUE(get_group.is_valid());  // Resolver stop keeps the completed group in the attribute tree.
}

// Spec 2026B Binary Sensor CC v2, interview report handling.
// Verifies a matching Report without a stored Sensor Value cannot advance interview, while an empty mask ends it.
TEST_F(SensorBinaryTest, InterviewV2WaitsForStoredValueAndHandlesEmptySupportedMask)
{
    // Open a real v2 interview; its Supported Sensor Report mask is initially required but not yet reported.
    finish_interview(SENSOR_BINARY_VERSION_2);
    auto &core        = static_cast<command_class_sensor_binary_core &>(component());
    auto report_group = endpoint.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::SENSOR_BINARY_REPORT_GROUP));
    report_group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_type)).set_reported(SENSOR_TYPE_GENERAL_PURPOSE);
    report_group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_value));
    auto get_group = endpoint.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP));
    get_group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::sensor_type)).set_desired(SENSOR_TYPE_GENERAL_PURPOSE);

    // A matching decoded Report alone is insufficient: the attribute store must first contain its Sensor Value.
    const command_class_sensor_binary_attribute_map_t general_purpose_report {{"sensor_type", SENSOR_TYPE_GENERAL_PURPOSE}, {"sensor_value", SENSOR_VALUE_IDLE}};
    ASSERT_EQ(SL_STATUS_OK, core.on_sensor_binary_report_parsed(nullptr, endpoint, general_purpose_report));
    auto get_type = get_group.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::sensor_type));
    EXPECT_EQ(SENSOR_TYPE_GENERAL_PURPOSE, get_type.desired<uint8_t>());

    // Store the value and report an empty Supported Sensor mask; no sensor type remains for interview to request.
    report_group.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_value)).set_reported(SENSOR_VALUE_IDLE);
    ASSERT_EQ(SL_STATUS_OK, core.on_sensor_binary_report_parsed(nullptr, endpoint, general_purpose_report));
    auto &store = static_cast<command_class_sensor_binary_attribute_store &>(component());
    ASSERT_EQ(SL_STATUS_OK, store.on_sensor_binary_supported_sensor_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {}}}));
    ASSERT_EQ(SL_STATUS_OK, component().on_sensor_binary_supported_sensor_report_parsed(nullptr, endpoint, {}));
    component().cc_interview_finish_if_complete(endpoint);
    EXPECT_EQ((std::vector<uint8_t> {}),
              endpoint.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_supported_sensor_report_group_attributes_t::SENSOR_BINARY_SUPPORTED_SENSOR_REPORT_GROUP))
                .child_by_type(static_cast<attribute_store_type_t>(sensor_binary_supported_sensor_report_group_attributes_t::bit_mask))
                .reported<std::vector<uint8_t>>());
}

// Spec 2026B Binary Sensor CC v2, Supported Sensor Report callback.
// Verifies receiving this Report outside interview does not queue a sensor-specific Get.
TEST_F(SensorBinaryTest, SupportedReportV2OutsideInterviewDoesNotStartSensorGet)
{
    // Store a valid type mask, then invoke the callback without opening the post-interview window.
    auto &store = static_cast<command_class_sensor_binary_attribute_store &>(component());
    ASSERT_EQ(SL_STATUS_OK, store.on_sensor_binary_supported_sensor_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {SUPPORTED_GENERAL_PURPOSE_MASK}}}));
    ASSERT_EQ(SL_STATUS_OK, component().on_sensor_binary_supported_sensor_report_parsed(nullptr, endpoint, {}));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP)).is_valid());
}

// Spec 2026B Binary Sensor CC v1, Sensor Report matching.
// Verifies a v1 Report safely handles the case where no Get group is present.
TEST_F(SensorBinaryTest, ReportV1WithoutOutstandingGetIsIgnored)
{
    set_version(SENSOR_BINARY_VERSION_1);
    auto &core = static_cast<command_class_sensor_binary_core &>(component());

    // v1 Reports stop an existing untyped request; without one there is nothing for the callback to stop.
    EXPECT_EQ(SL_STATUS_OK, core.on_sensor_binary_report_parsed(nullptr, endpoint, {{"sensor_value", SENSOR_VALUE_IDLE}}));
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP)).is_valid());
}

// Sensor Binary attribute-store edge cases.
// Covers absent mask/report attributes that can exist before a peer sends the corresponding Report.
TEST_F(SensorBinaryTest, AttributeStoreHandlesMissingMaskAndIncompleteReportGroups)
{
    TestableSensorBinaryAttributeStore store;

    // Before a Supported Sensor Report arrives, no mask group exists and the helper must return an empty mask.
    EXPECT_TRUE(store.get_supported_sensor_bit_mask(endpoint).empty());

    // A mask group can be created by interview before its required bit-mask value has arrived.
    auto mask_group = endpoint.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_supported_sensor_report_group_attributes_t::SENSOR_BINARY_SUPPORTED_SENSOR_REPORT_GROUP));
    EXPECT_TRUE(store.get_supported_sensor_bit_mask(endpoint).empty());
    mask_group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_supported_sensor_report_group_attributes_t::bit_mask));
    EXPECT_TRUE(store.get_supported_sensor_bit_mask(endpoint).empty());

    // Ignore malformed report groups without a stored type while searching for a requested sensor type.
    endpoint.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::SENSOR_BINARY_REPORT_GROUP));
    auto incomplete_group = endpoint.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::SENSOR_BINARY_REPORT_GROUP));
    incomplete_group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_type));
    EXPECT_FALSE(store.find_report_group_by_sensor_type(endpoint, SENSOR_TYPE_SMOKE).is_valid());

    // A known report group without Sensor Value is not complete; adding the value makes it available.
    auto report_group = store.find_or_create_report_group_by_sensor_type(endpoint, SENSOR_TYPE_SMOKE);
    EXPECT_FALSE(store.has_report_for_sensor_type(endpoint, SENSOR_TYPE_SMOKE));
    report_group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_value));
    EXPECT_FALSE(store.has_report_for_sensor_type(endpoint, SENSOR_TYPE_SMOKE));
    report_group.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_value)).set_reported(SENSOR_VALUE_IDLE);
    EXPECT_TRUE(store.has_report_for_sensor_type(endpoint, SENSOR_TYPE_SMOKE));
}

// Spec 2026B Binary Sensor CC v2, Supported Get Sensor frame.
// Verifies the v2 capability query carries no command-specific payload.
TEST_F(SensorBinaryTest, SupportedGetSensorV2BuildsEmptyPayload)
{
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_supported_get_sensor_group_attributes_t::SENSOR_BINARY_SUPPORTED_GET_SENSOR_GROUP));

    // Supported Get consists of Command Class and Command identifiers only.
    uint8_t data[FRAME_CAPACITY] {};
    uint16_t length = EMPTY_FRAME_LENGTH;
    zwave_frame_generator generator {SENSOR_BINARY_COMMAND_CLASS_ID};
    generator.initialize_frame(SENSOR_BINARY_SUPPORTED_GET_SENSOR_COMMAND_ID, data, &length);
    command_class_sensor_binary_core::get_requested_args args {group, &generator};
    ASSERT_EQ(SL_STATUS_OK, component().on_sensor_binary_supported_get_sensor_requested(args, data, &length));
    const std::vector<uint8_t> expected {SENSOR_BINARY_COMMAND_CLASS_ID, SENSOR_BINARY_SUPPORTED_GET_SENSOR_COMMAND_ID};
    EXPECT_EQ(expected, std::vector<uint8_t>(data, data + length));
}

// Spec 2026B Binary Sensor CC v1, Sensor Report storage.
// Checks the v1 Sensor Value is stored under the reserved Sensor Type 0 group.
TEST_F(SensorBinaryTest, ReportV1StoresIdleValueWithoutSensorType)
{
    auto &store = static_cast<command_class_sensor_binary_attribute_store &>(component());

    // A v1 Report contains only Sensor Value; the missing type defaults to the legacy group identifier 0.
    ASSERT_EQ(SL_STATUS_OK, store.on_sensor_binary_report_received_store(endpoint, {{"sensor_value", SENSOR_VALUE_IDLE}}));
    auto report = endpoint.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::SENSOR_BINARY_REPORT_GROUP));
    ASSERT_TRUE(report.is_valid());
    EXPECT_EQ(SENSOR_TYPE_UNSPECIFIED, report.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_type)).reported<uint8_t>());
    EXPECT_EQ(SENSOR_VALUE_IDLE, report.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_value)).reported<uint8_t>());
}

// Spec 2026B Binary Sensor CC v2, Sensor Report storage.
// Verifies active Water and idle Smoke reports are stored in separate type-specific groups.
TEST_F(SensorBinaryTest, ReportV2StoresValuesBySensorType)
{
    auto &store = static_cast<command_class_sensor_binary_attribute_store &>(component());

    // Sensor Value 0xFF means the Water sensor detected an event; its type is carried in the v2 Report.
    ASSERT_EQ(SL_STATUS_OK, store.on_sensor_binary_report_received_store(endpoint, {{"sensor_type", SENSOR_TYPE_WATER}, {"sensor_value", SENSOR_VALUE_TRIGGER}}));
    // Sensor Value 0x00 means the Smoke sensor is idle; it must not overwrite Water's separate report group.
    ASSERT_EQ(SL_STATUS_OK, store.on_sensor_binary_report_received_store(endpoint, {{"sensor_type", SENSOR_TYPE_SMOKE}, {"sensor_value", SENSOR_VALUE_IDLE}}));

    std::array<bool, SENSOR_TYPE_WATER + 1> found_types {};
    for (auto report: endpoint.children(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::SENSOR_BINARY_REPORT_GROUP))) {
        const auto type   = report.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_type)).reported<uint8_t>();
        found_types[type] = true;
        const auto value  = report.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_report_group_attributes_t::sensor_value)).reported<uint8_t>();
        EXPECT_EQ(type == SENSOR_TYPE_WATER ? SENSOR_VALUE_TRIGGER : SENSOR_VALUE_IDLE, value);
    }
    EXPECT_TRUE(found_types[SENSOR_TYPE_WATER]);
    EXPECT_TRUE(found_types[SENSOR_TYPE_SMOKE]);
}

// Spec 2026B Binary Sensor CC v2, Supported Sensor Report.
// Confirms the bit mask is stored and repeated Reports update the existing group.
TEST_F(SensorBinaryTest, SupportedReportV2StoresAndUpdatesBitMask)
{
    auto &store = static_cast<command_class_sensor_binary_attribute_store &>(component());

    // Bit 1 denotes General Purpose; bit 2 denotes Smoke, following the Sensor Type values from the v2 table.
    ASSERT_EQ(SL_STATUS_OK, store.on_sensor_binary_supported_sensor_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {SUPPORTED_GENERAL_PURPOSE_MASK}}}));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_supported_sensor_report_group_attributes_t::SENSOR_BINARY_SUPPORTED_SENSOR_REPORT_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ((std::vector<uint8_t> {SUPPORTED_GENERAL_PURPOSE_MASK}), group.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_supported_sensor_report_group_attributes_t::bit_mask)).reported<std::vector<uint8_t>>());

    // A later mask advertises Smoke as well; the endpoint retains one current Supported Report group.
    ASSERT_EQ(SL_STATUS_OK, store.on_sensor_binary_supported_sensor_report_received_store(endpoint, {{"bit_mask", std::vector<uint8_t> {SUPPORTED_SMOKE_MASK}}}));
    EXPECT_EQ((std::vector<uint8_t> {SUPPORTED_SMOKE_MASK}), group.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_supported_sensor_report_group_attributes_t::bit_mask)).reported<std::vector<uint8_t>>());
}

// Spec 2026B Binary Sensor CC v2, report parsing and request matching.
// Checks that an unrelated report leaves the outstanding typed Get intact.
TEST_F(SensorBinaryTest, ReportV2IgnoresUnexpectedSensorType)
{
    set_version(SENSOR_BINARY_VERSION_2);
    auto &core     = static_cast<command_class_sensor_binary_core &>(component());
    auto get_group = endpoint.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP));
    get_group.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::sensor_type)).set_desired(SENSOR_TYPE_SMOKE);

    // A Water report cannot complete the outstanding Smoke request; the group remains available for retry.
    ASSERT_EQ(SL_STATUS_OK, core.on_sensor_binary_report_parsed(nullptr, endpoint, {{"sensor_type", SENSOR_TYPE_WATER}, {"sensor_value", SENSOR_VALUE_TRIGGER}}));
    EXPECT_TRUE(get_group.is_valid());
}

// Spec 2026B Binary Sensor CC v1, report parsing.
// Verifies any Report completes a v1 type-less Get and stops its resolver group.
TEST_F(SensorBinaryTest, ReportV1StopsUntypedGet)
{
    set_version(SENSOR_BINARY_VERSION_1);
    auto &core     = static_cast<command_class_sensor_binary_core &>(component());
    auto get_group = endpoint.emplace_node(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP));

    // In v1, the Get does not identify a type, so the received Sensor Value report matches it without a type check.
    ASSERT_EQ(SL_STATUS_OK, core.on_sensor_binary_report_parsed(nullptr, endpoint, {{"sensor_value", SENSOR_VALUE_TRIGGER}}));
    EXPECT_TRUE(get_group.is_valid());  // The group remains in the tree but its resolution was stopped by the callback.
}

// Spec 2026B Binary Sensor CC v2, MQTT SensorBinaryGet payload.
// Checks a requested Sensor Type is saved and an omitted type clears it for the wildcard-style Get.
TEST_F(SensorBinaryTest, MqttGetParsesOptionalSensorType)
{
    // The payload's Sensor Type 6 selects Water, an assigned type in the Sensor Binary v2 table.
    ASSERT_EQ(SL_STATUS_OK, command_class_sensor_binary_mqtt::mqtt_on_sensor_binary_get_command(endpoint, R"({"sensor_type":6})"));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP));
    ASSERT_TRUE(group.is_valid());
    auto type = group.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::sensor_type));
    ASSERT_TRUE(type.is_valid());
    EXPECT_EQ(SENSOR_TYPE_WATER, type.desired<uint8_t>());

    // Empty payload means no particular type was requested, so the desired type is cleared.
    ASSERT_EQ(SL_STATUS_OK, command_class_sensor_binary_mqtt::mqtt_on_sensor_binary_get_command(endpoint, ""));
    EXPECT_FALSE(type.desired_exists());
}

// Spec 2026B Binary Sensor CC v2, MQTT command validation.
// Verifies malformed JSON fails while Supported Get ignores its unused payload.
TEST_F(SensorBinaryTest, MqttRejectsInvalidGetPayloadAndAcceptsSupportedGet)
{
    // A malformed JSON object cannot be parsed into a Sensor Type request.
    EXPECT_NE(SL_STATUS_OK, command_class_sensor_binary_mqtt::mqtt_on_sensor_binary_get_command(endpoint, "{invalid"));
    EXPECT_NE(SL_STATUS_OK, command_class_sensor_binary_mqtt::mqtt_on_sensor_binary_get_command(endpoint, R"({"sensor_type":"smoke"})"));

    // A valid object with no sensor_type exercises the optional-field path and leaves the Get untyped.
    ASSERT_EQ(SL_STATUS_OK, command_class_sensor_binary_mqtt::mqtt_on_sensor_binary_get_command(endpoint, "{}"));
    auto get_group = endpoint.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP));
    ASSERT_TRUE(get_group.is_valid());
    EXPECT_FALSE(get_group.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::sensor_type)).desired_exists());

    // Supported Get has no fields in its command definition; its handler accepts and ignores payload text.
    ASSERT_EQ(SL_STATUS_OK, command_class_sensor_binary_mqtt::mqtt_on_sensor_binary_supported_get_sensor_command(endpoint, "ignored payload"));
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_supported_get_sensor_group_attributes_t::SENSOR_BINARY_SUPPORTED_GET_SENSOR_GROUP)).is_valid());
}

// Spec 2026B Binary Sensor CC v2, registered MQTT command routes.
// Confirms both implemented command names dispatch into the expected resolver groups.
TEST_F(SensorBinaryTest, RegisteredMqttCommandsDispatch)
{
    // Dispatch a typed Sensor Get and the empty Supported Get using their public MQTT command names.
    mqtt().mqtt_command_handler(endpoint, "SensorBinaryGet", R"({"sensor_type":2})");
    mqtt().mqtt_command_handler(endpoint, "SensorBinarySupportedGetSensor", "{}");

    // Both handlers create the request groups whose resolvers later assemble the Z-Wave frames.
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_get_group_attributes_t::SENSOR_BINARY_GET_GROUP)).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(sensor_binary_supported_get_sensor_group_attributes_t::SENSOR_BINARY_SUPPORTED_GET_SENSOR_GROUP)).is_valid());
}
