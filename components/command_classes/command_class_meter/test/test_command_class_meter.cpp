/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of the Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include <gtest/gtest.h>

#include <array>
#include <functional>

#include <nlohmann/json.hpp>

#include "attribute.hpp"
#include "attribute_store.h"
#include "attribute_store_fixt.h"
#include "command_class_meter.hpp"
#include "command_class_meter_attribute_store.hpp"
#include "command_class_meter_constants.hpp"
#include "command_class_meter_mqtt.hpp"
#include "zwave_frame_generator.hpp"

using namespace zwave_command_class;
using namespace zwave_command_class::command_class_meter_types;

namespace
{
    constexpr uint8_t METER_COMMAND_CLASS_ID            = 0x32;
    constexpr uint8_t METER_GET_COMMAND_ID              = 0x01;
    constexpr uint8_t METER_REPORT_COMMAND_ID           = 0x02;
    constexpr uint8_t METER_RESET_COMMAND_ID            = 0x05;
    constexpr uint8_t METER_SUPPORTED_REPORT_COMMAND_ID = 0x04;
    constexpr uint8_t METER_CC_VERSION_1                = 1;
    constexpr uint8_t METER_CC_VERSION_2                = 2;
    constexpr uint8_t METER_CC_VERSION_3                = 3;
    constexpr uint8_t METER_CC_VERSION_4                = 4;
    constexpr uint8_t METER_CC_VERSION_5                = 5;
    constexpr uint8_t METER_CC_VERSION_6                = 6;
    constexpr uint16_t TEST_NODE_ID                     = 2;
    constexpr uint8_t TEST_ENDPOINT_ID                  = 0;
    constexpr std::array<uint8_t, 4> TEST_HOME_ID {{0x12, 0x34, 0x56, 0x78}};

    constexpr uint8_t TEST_METER_TYPE_ELECTRIC        = 1;
    constexpr uint8_t TEST_METER_TYPE_DEFAULT         = 0;
    constexpr uint8_t TEST_METER_RESET_UNSUPPORTED    = 0;
    constexpr uint8_t TEST_SUPPORTED_SCALE_0          = 0x01;
    constexpr uint8_t TEST_MORE_SCALE_BYTES_FLAG      = 0x80;
    constexpr uint8_t TEST_SCALE_EXTENSION_BYTE_COUNT = 1;
    constexpr uint8_t TEST_RATE_TYPE_IMPORT           = 1;

    // Logical Meter Type and Rate Type values used by the test scenarios.
    constexpr uint8_t TEST_METER_TYPE_GAS              = 2;
    constexpr uint8_t TEST_METER_TYPE_WATER            = 3;
    constexpr uint8_t TEST_RATE_TYPE_DEFAULT           = 0;
    constexpr uint8_t TEST_RATE_TYPE_NO_SUPPORT_MASKED = 0;
    constexpr uint8_t TEST_RATE_TYPE_EXPORT            = 2;
    constexpr uint8_t TEST_LOGICAL_SCALE_DEFAULT       = 0;
    constexpr uint8_t TEST_LOGICAL_SCALE_ONE           = 1;
    constexpr uint8_t TEST_LOGICAL_SCALE_THREE         = 3;
    constexpr uint8_t TEST_LOGICAL_SCALE_FOUR          = 4;
    constexpr uint8_t TEST_LOGICAL_SCALE_SIX           = 6;

    // Meter Report property bytes: Export occupies bits 6:5; scale 2 occupies bits 4:3; precision 2 occupies bits 7:5.
    constexpr uint8_t TEST_RATE_TYPE_IMPORT_MASKED   = 0x20;
    constexpr uint8_t TEST_RATE_TYPE_EXPORT_MASKED   = 0x40;
    constexpr uint8_t TEST_RATE_TYPE_BOTH_MASKED     = 0x60;
    constexpr uint8_t TEST_SCALE_BITS_10_FOR_SCALE_2 = 0x10;
    constexpr uint8_t TEST_PRECISION_2_MASKED        = 0x40;
    constexpr uint8_t TEST_SCALE_BIT_2_CLEAR         = 0;
    constexpr uint8_t TEST_SCALE_BITS_10_CLEAR       = 0;
    constexpr uint8_t TEST_SCALE_2_CLEAR             = 0;
    constexpr uint8_t TEST_PRECISION_ZERO            = 0;

    // Example readings are unsigned big-endian values; precision 2 means 500 is displayed as 5.00 and 432 as 4.32.
    constexpr uint8_t TEST_READING_500_MSB     = 0x01;
    constexpr uint8_t TEST_READING_500_LSB     = 0xF4;
    constexpr uint8_t TEST_READING_432_MSB     = 0x01;
    constexpr uint8_t TEST_READING_432_LSB     = 0xB0;
    constexpr uint8_t TEST_READING_42          = 0x2A;
    constexpr uint8_t TEST_READING_ONE         = 0x01;
    constexpr uint8_t TEST_DELTA_TIME_SECONDS  = 60;
    constexpr uint16_t TEST_DELTA_TIME_DEFAULT = 0;

    // Named test sizes and encodings used in malformed-type and report-group cases.
    constexpr uint8_t TEST_METER_VALUE_SIZE_TWO_BYTES                  = 2;
    constexpr uint8_t TEST_METER_VALUE_SIZE_THREE_BYTES                = 3;
    constexpr uint8_t TEST_INVALID_TYPED_VALUE                         = 9;
    constexpr uint8_t TEST_RESET_SUPPORTED_MASKED                      = 0x80;
    constexpr uint8_t TEST_KEY_FIELD_CLEAR                             = 0;
    constexpr uint8_t TEST_SCALE_ZERO_SUPPORTED_MASKED                 = 0x01;
    constexpr uint8_t TEST_SCALE_ZERO_AND_ONE_SUPPORTED_MASKED         = 0x03;
    constexpr uint8_t TEST_MST_CLEAR                                   = 0;
    constexpr uint8_t TEST_MST_SET                                     = 0x80;
    constexpr uint8_t TEST_SCALE_EXTENSION_SCALE_SEVEN                 = 0x01;
    constexpr uint8_t TEST_SCALE_EXTENSION_SCALE_EIGHT                 = 0x02;
    constexpr uint8_t TEST_METER_VALUE_SIZE_ONE_BYTE                   = 1;
    constexpr uint8_t TEST_METER_VALUE_SIZE_FOUR_BYTES                 = 4;
    constexpr uint16_t TEST_MQTT_SCALE_OUT_OF_RANGE                    = 256;
    constexpr uint8_t TEST_MQTT_VALUE_TOO_LONG_BYTES                   = 5;
    constexpr int16_t TEST_NEGATIVE_SCALE                              = -1;
    constexpr uint8_t TEST_READING_TWO                                 = 2;
    constexpr uint8_t TEST_READING_THREE                               = 3;
    constexpr uint8_t TEST_READING_FOUR                                = 4;
    constexpr uint8_t TEST_READING_FIVE                                = 5;
    constexpr uint8_t TEST_READING_ZERO                                = 0;
    constexpr uint8_t TEST_FRAME_BUFFER_SIZE                           = 16;
    constexpr uint8_t TEST_METER_GET_BUFFER_SIZE                       = 8;
    constexpr uint8_t TEST_FIRST_COMMAND_PAYLOAD_OFFSET                = 2;
    constexpr uint8_t TEST_SECOND_COMMAND_PAYLOAD_OFFSET               = 3;
    constexpr uint8_t TEST_METER_RESET_FIRST_VALUE_OFFSET              = 4;
    constexpr uint8_t TEST_METER_RESET_SECOND_VALUE_OFFSET             = 5;
    constexpr uint8_t TEST_METER_RESET_SCALE_2_OFFSET                  = 6;
    constexpr uint8_t TEST_METER_GET_FRAME_LENGTH                      = 4;
    constexpr uint8_t TEST_NO_ARGUMENT_COMMAND_FRAME_LENGTH            = 2;
    constexpr uint8_t TEST_METER_GET_V2_PROPERTIES_SCALE_THREE         = 0x18;
    constexpr uint8_t TEST_METER_GET_V3_PROPERTIES_SCALE_SIX           = 0x30;
    constexpr uint8_t TEST_METER_GET_V2_V3_FRAME_LENGTH                = 3;
    constexpr uint8_t TEST_METER_RESET_FRAME_LENGTH                    = 7;
    constexpr uint8_t TEST_METER_GET_V6_PROPERTIES                     = 0xB8;
    constexpr uint8_t TEST_METER_GET_V6_SCALE_2                        = 1;
    constexpr uint8_t TEST_METER_RESET_V6_PROPERTIES_1                 = 0xC3;
    constexpr uint8_t TEST_METER_RESET_V6_PROPERTIES_2                 = 0x5A;
    constexpr uint8_t TEST_METER_REPORT_V6_PROPERTIES_1                = 0xC1;
    constexpr uint8_t TEST_METER_REPORT_V6_PROPERTIES_2                = 0x5A;
    constexpr uint8_t TEST_METER_REPORT_V6_DELTA_TIME_MSB              = 0;
    constexpr uint8_t TEST_METER_REPORT_V6_DELTA_TIME_LSB              = 0x3C;
    constexpr uint8_t TEST_METER_SCALE_2_FOR_SCALE_EIGHT               = 1;
    constexpr uint8_t TEST_METER_REPORT_V1_PROPERTIES_1                = 0x01;
    constexpr uint8_t TEST_METER_REPORT_V1_PROPERTIES_2                = 0x01;
    constexpr uint8_t TEST_SUPPORTED_REPORT_V4_PROPERTIES_1            = 0xE2;
    constexpr uint8_t TEST_SUPPORTED_REPORT_V4_PROPERTIES_2            = 0x86;
    constexpr uint8_t TEST_SUPPORTED_REPORT_V2_PROPERTIES_1            = 0x82;
    constexpr uint8_t TEST_SUPPORTED_REPORT_V2_SCALE_MASK              = 0x05;
    constexpr uint8_t TEST_SUPPORTED_REPORT_IMPORT_PROPERTIES_1        = 0x20;
    constexpr uint8_t TEST_SUPPORTED_REPORT_SCALE_ZERO_MASK            = 0x01;
    constexpr uint8_t TEST_SUPPORTED_REPORT_SCALES_ONE_AND_TWO_MASK    = 0x06;
    constexpr uint8_t TEST_SUPPORTED_REPORT_EXTENSION_SCALE_SEVEN_MASK = 0x01;
    constexpr uint8_t TEST_SUPPORTED_REPORT_MST_CLEAR                  = 0;
    constexpr uint8_t TEST_METER_VALUE_UPDATED_MSB                     = 0x02;
    constexpr uint8_t TEST_METER_VALUE_UPDATED_LSB                     = 0x00;
    constexpr uint8_t TEST_PRECISION_TWO_DECIMAL_PLACES                = 2;
    constexpr uint8_t TEST_LOGICAL_SCALE_EIGHT                         = 8;
    constexpr uint8_t TEST_LOGICAL_SCALE_TWO                           = 2;

    // Return the Meter Report lookup key as the implementation stores it: three masked scale fields and a masked rate.
    std::array<uint8_t, 4> meter_report_key(uint8_t logical_scale, uint8_t logical_rate_type)
    {
        uint8_t scale_bit_2   = 0;
        uint8_t scale_bits_10 = 0;
        uint8_t scale_2       = 0;
        command_class_meter_constants::encode_scale_key_fields(logical_scale, scale_bit_2, scale_bits_10, scale_2);
        return {scale_bit_2, scale_bits_10, scale_2, command_class_meter_constants::rate_type_to_masked(logical_rate_type)};
    }
}  // namespace

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}

class TestableMeterMqtt : public command_class_meter_mqtt
{
    public:
        using zwave_command_class_base::mqtt_command_handler;
};

class MeterCommandClassTest : public ::testing::Test
{
    protected:
        static command_class_meter &component()
        {
            static command_class_meter instance;
            return instance;
        }

        static TestableMeterMqtt &mqtt()
        {
            static TestableMeterMqtt instance;
            return instance;
        }

        void SetUp() override
        {
            ASSERT_EQ(SL_STATUS_OK, attribute_store_init());
            auto root = attribute_store::attribute::root();
            // Give the fixture a stable, named home/node/endpoint identity for MQTT topics and attribute paths.
            auto home = root.emplace_node(ATTRIBUTE_HOME_ID, std::vector<uint8_t>(TEST_HOME_ID.begin(), TEST_HOME_ID.end()));
            auto node = home.emplace_node(ATTRIBUTE_NODE_ID, TEST_NODE_ID);
            endpoint  = node.emplace_node(ATTRIBUTE_ENDPOINT_ID, TEST_ENDPOINT_ID);
            (void)component();
            (void)mqtt();
        }

        void TearDown() override
        {
            attribute_store_teardown();
        }

        void set_version(uint8_t version)
        {
            endpoint.emplace_node(static_cast<attribute_store_type_t>(command_class_meter_attributes_t::supported_version)).set_reported(version);
        }

        template<typename AttributeEnum, typename Value> void desired(attribute_store::attribute group, AttributeEnum type, Value value)
        {
            group.emplace_node(static_cast<attribute_store_type_t>(type)).set_desired(value);
        }

        void verify_interview_start(uint8_t version, bool expects_report_group, bool expects_supported_get_group)
        {
            // Start the command-class interview at the requested version.
            component().on_interview(endpoint, version);
            // Version 1 schedules a direct reading; later versions request Supported Report first.
            EXPECT_EQ(expects_report_group, endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP)).is_valid());
            EXPECT_EQ(expects_supported_get_group, endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_supported_get_group_attributes_t::METER_SUPPORTED_GET_GROUP)).is_valid());
        }

        void verify_meter_get_peer_format(uint8_t version)
        {
            // Start with the extended scale and Export rate used by v4 and newer peers.
            uint8_t data[TEST_METER_GET_BUFFER_SIZE] {};
            uint16_t length = 0;
            zwave_frame_generator generator {METER_COMMAND_CLASS_ID};
            auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP));
            command_class_meter_core::get_requested_args args {group, &generator};

            // Choose a scale that exists in this peer version; v1 ignores scale and rate fields entirely.
            if (version == METER_CC_VERSION_2) {
                desired(group, meter_get_group_attributes_t::scale, uint8_t {TEST_LOGICAL_SCALE_THREE});  // Scale 3 is the highest v2 scale.
            } else if (version == METER_CC_VERSION_3) {
                desired(group, meter_get_group_attributes_t::scale, uint8_t {TEST_LOGICAL_SCALE_SIX});  // Scale 6 is defined by v3.
            } else {
                desired(group, meter_get_group_attributes_t::scale, uint8_t {TEST_LOGICAL_SCALE_EIGHT});
            }
            if (version >= METER_CC_VERSION_4) {
                desired(group, meter_get_group_attributes_t::rate_type, uint8_t {TEST_RATE_TYPE_EXPORT});
            }

            // Assemble using the selected peer version so the assertions check that version's specified format.
            set_version(version);
            generator.initialize_frame(METER_GET_COMMAND_ID, data, &length);
            ASSERT_EQ(SL_STATUS_OK, component().on_meter_get_requested_assemble_frame(args, data, &length));

            // Version 1 defines Meter Get with no payload fields.
            if (version == METER_CC_VERSION_1) {
                EXPECT_EQ(TEST_NO_ARGUMENT_COMMAND_FRAME_LENGTH, length);
                return;
            }

            // Versions 2 and 3 encode only Scale; Rate Type and Scale 2 are not present in these formats.
            if (version == METER_CC_VERSION_2) {
                EXPECT_EQ(TEST_METER_GET_V2_V3_FRAME_LENGTH, length);
                EXPECT_EQ(TEST_METER_GET_V2_PROPERTIES_SCALE_THREE, data[TEST_FIRST_COMMAND_PAYLOAD_OFFSET]);
                return;
            }
            if (version == METER_CC_VERSION_3) {
                EXPECT_EQ(TEST_METER_GET_V2_V3_FRAME_LENGTH, length);
                EXPECT_EQ(TEST_METER_GET_V3_PROPERTIES_SCALE_SIX, data[TEST_FIRST_COMMAND_PAYLOAD_OFFSET]);
                return;
            }

            // Versions 4 through 6 encode Export and extended scale 8, including the Scale 2 byte.
            EXPECT_EQ(TEST_METER_GET_FRAME_LENGTH, length);                                    // Header, properties, and Scale 2 byte.
            EXPECT_EQ(TEST_METER_GET_V6_PROPERTIES, data[TEST_FIRST_COMMAND_PAYLOAD_OFFSET]);  // Export and scale 8 fields.
            EXPECT_EQ(TEST_METER_GET_V6_SCALE_2, data[TEST_SECOND_COMMAND_PAYLOAD_OFFSET]);    // Scale 2 extends the scale to 8.
        }

        void verify_meter_reset_peer_format(uint8_t version)
        {
            uint8_t data[TEST_FRAME_BUFFER_SIZE] {};
            uint16_t length = 0;
            zwave_frame_generator generator {METER_COMMAND_CLASS_ID};

            // Versions 2 through 5 use the original no-argument Meter Reset format.
            if (version < METER_CC_VERSION_6) {
                set_version(version);
                generator.initialize_frame(METER_RESET_COMMAND_ID, data, &length);
                command_class_meter_core::set_requested_args args {endpoint.emplace_node(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::METER_RESET_GROUP)), &generator};
                ASSERT_EQ(SL_STATUS_OK, component().on_meter_reset_requested_assemble_frame(args, data, &length));
                EXPECT_EQ(TEST_NO_ARGUMENT_COMMAND_FRAME_LENGTH, length);
                return;
            }

            // Version 6 adds fields to Meter Reset; this value length matches its declared Size.
            auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::METER_RESET_GROUP));
            command_class_meter_core::set_requested_args args {group, &generator};
            set_version(version);
            desired(group, meter_reset_group_attributes_t::meter_type, uint8_t {TEST_METER_TYPE_WATER});
            desired(group, meter_reset_group_attributes_t::rate_type, uint8_t {TEST_RATE_TYPE_EXPORT});
            desired(group, meter_reset_group_attributes_t::size, uint8_t {TEST_METER_VALUE_SIZE_TWO_BYTES});
            desired(group, meter_reset_group_attributes_t::precision, uint8_t {TEST_PRECISION_TWO_DECIMAL_PLACES});
            desired(group, meter_reset_group_attributes_t::meter_value, std::vector<uint8_t> {TEST_READING_500_MSB, TEST_READING_500_LSB});
            desired(group, meter_reset_group_attributes_t::scale_bits_10, uint8_t {TEST_LOGICAL_SCALE_EIGHT});

            // Assemble and verify the current reset payload for this peer that supports the command ID.
            generator.initialize_frame(METER_RESET_COMMAND_ID, data, &length);
            ASSERT_EQ(SL_STATUS_OK, component().on_meter_reset_requested_assemble_frame(args, data, &length));
            EXPECT_EQ(TEST_METER_RESET_FRAME_LENGTH, length);                                       // Header, two property bytes, two value bytes, and Scale 2.
            EXPECT_EQ(TEST_METER_RESET_V6_PROPERTIES_1, data[TEST_FIRST_COMMAND_PAYLOAD_OFFSET]);   // Water, Export, and scale high bit.
            EXPECT_EQ(TEST_METER_RESET_V6_PROPERTIES_2, data[TEST_SECOND_COMMAND_PAYLOAD_OFFSET]);  // Precision, scale low bits, and Size=2.
            EXPECT_EQ(TEST_READING_500_MSB, data[TEST_METER_RESET_FIRST_VALUE_OFFSET]);             // Most-significant byte of 500.
            EXPECT_EQ(TEST_READING_500_LSB, data[TEST_METER_RESET_SECOND_VALUE_OFFSET]);            // Least-significant byte of 500.
            EXPECT_EQ(TEST_METER_GET_V6_SCALE_2, data[TEST_METER_RESET_SCALE_2_OFFSET]);            // Scale 2 completes extended scale 8.
        }

        attribute_store::attribute endpoint;
};

// Verifies v4 Meter Report values are grouped by logical scale and rate and retain all report fields.
TEST_F(MeterCommandClassTest, StoresV4ReportsByScaleAndRateType)
{
    // Build a v4 Report for Electric/Export, scale 2, a two-byte value, one-minute delta, and a previous reading.
    command_class_meter_attribute_map_t report {
      {"meter_type", uint8_t {TEST_METER_TYPE_ELECTRIC}},                                           // Meter Type 1 selects an Electric meter.
      {"rate_type", uint8_t {TEST_RATE_TYPE_EXPORT_MASKED}},                                        // Masked rate bits select Export.
      {"scale_bit_2", uint8_t {TEST_SCALE_BIT_2_CLEAR}},                                            // The high scale bit is clear.
      {"size", uint8_t {TEST_METER_VALUE_SIZE_TWO_BYTES}},                                          // Meter Value and Previous Meter Value each contain two bytes.
      {"scale_bits_10", uint8_t {TEST_SCALE_BITS_10_FOR_SCALE_2}},                                  // Low scale bits encode logical scale 2.
      {"precision", uint8_t {TEST_PRECISION_2_MASKED}},                                             // Masked precision bits encode two decimal places.
      {"meter_value", std::vector<uint8_t> {TEST_READING_500_MSB, TEST_READING_500_LSB}},           // 0x01F4 is 500, displayed as 5.00.
      {"delta_time", uint16_t {TEST_DELTA_TIME_SECONDS}},                                           // The previous reading is 60 seconds earlier.
      {"previous_meter_value", std::vector<uint8_t> {TEST_READING_432_MSB, TEST_READING_432_LSB}},  // 0x01B0 is 432, displayed as 4.32.
      {"scale_2", uint8_t {TEST_SCALE_2_CLEAR}},                                                    // No extended-scale bit is needed for scale 2.
    };

    // Store the first report and confirm its decoded key and value fields are retained.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_received_store(endpoint, report));
    auto groups = endpoint.children(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP));
    ASSERT_EQ(1, groups.size());  // One report group is created for this Meter Report.
    auto group = groups.front();
    EXPECT_EQ(TEST_LOGICAL_SCALE_TWO,
              command_class_meter_constants::meter_scale_from_report_fields(group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::scale_bit_2)).reported<uint8_t>(),
                                                                            group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::scale_bits_10)).reported<uint8_t>(),
                                                                            group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::scale_2)).reported<uint8_t>()));
    EXPECT_EQ(TEST_RATE_TYPE_EXPORT, command_class_meter_constants::rate_type_from_masked(group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::rate_type)).reported<uint8_t>()));
    EXPECT_EQ((std::vector<uint8_t> {TEST_READING_500_MSB, TEST_READING_500_LSB}), group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_value)).reported<std::vector<uint8_t>>());           // Current value remains 5.00.
    EXPECT_EQ(TEST_DELTA_TIME_SECONDS, group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::delta_time)).reported<uint16_t>());                                                                    // Delta Time remains 60 seconds.
    EXPECT_EQ((std::vector<uint8_t> {TEST_READING_432_MSB, TEST_READING_432_LSB}), group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::previous_meter_value)).reported<std::vector<uint8_t>>());  // Previous value remains 4.32.

    // A second report for the same scale and rate updates the same group.
    report["meter_value"] = std::vector<uint8_t> {TEST_METER_VALUE_UPDATED_MSB, TEST_METER_VALUE_UPDATED_LSB};  // Change current reading to 512 (5.12) for the update check.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_received_store(endpoint, report));
    groups = endpoint.children(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP));
    EXPECT_EQ(1, groups.size());  // The second report updates the existing scale/rate group.
}

// Verifies the v6 Meter Report wire layout preserves masked fields, both values, Delta Time, and Scale 2.
TEST_F(MeterCommandClassTest, ParsesV6MeterReportWireFields)
{
    // This v6 Report encodes Electric/Export, scale 8, precision 2, a value of 5.00, a 60-second delta, and previous value 4.32.
    constexpr uint8_t meter_command_class = METER_COMMAND_CLASS_ID;
    constexpr uint8_t meter_report        = METER_REPORT_COMMAND_ID;
    const std::array<uint8_t, 11> frame {{
      meter_command_class,
      meter_report,
      TEST_METER_REPORT_V6_PROPERTIES_1,  // Electric meter type, Export rate, and high scale bit.
      TEST_METER_REPORT_V6_PROPERTIES_2,  // Precision 2, low scale bits, and two-byte value Size.
      TEST_READING_500_MSB,
      TEST_READING_500_LSB,  // Meter Value = 500, displayed as 5.00.
      TEST_METER_REPORT_V6_DELTA_TIME_MSB,
      TEST_METER_REPORT_V6_DELTA_TIME_LSB,  // Delta Time = 60 seconds.
      TEST_READING_432_MSB,
      TEST_READING_432_LSB,               // Previous Meter Value = 432, displayed as 4.32.
      TEST_METER_SCALE_2_FOR_SCALE_EIGHT  // Scale 2 byte selects scale 8.
    }};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_meter_core::report_received_args args {endpoint, parser, nullptr};

    // Parse through the generated frame parser so the assertions cover wire decoding as well as storage.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_received(args));
    auto groups = endpoint.children(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP));
    ASSERT_EQ(1, groups.size());  // The v6 frame creates one report group.
    const auto group = groups.front();
    EXPECT_EQ(TEST_METER_TYPE_ELECTRIC, group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_type)).reported<uint8_t>());
    EXPECT_EQ(TEST_RATE_TYPE_EXPORT, command_class_meter_constants::rate_type_from_masked(group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::rate_type)).reported<uint8_t>()));
    EXPECT_EQ(TEST_LOGICAL_SCALE_EIGHT,
              command_class_meter_constants::meter_scale_from_report_fields(group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::scale_bit_2)).reported<uint8_t>(),
                                                                            group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::scale_bits_10)).reported<uint8_t>(),
                                                                            group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::scale_2)).reported<uint8_t>()));
    EXPECT_EQ((std::vector<uint8_t> {TEST_READING_500_MSB, TEST_READING_500_LSB}), group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_value)).reported<std::vector<uint8_t>>());           // Parsed value stays 5.00.
    EXPECT_EQ(TEST_DELTA_TIME_SECONDS, group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::delta_time)).reported<uint16_t>());                                                                    // Parsed delta is 60 seconds.
    EXPECT_EQ((std::vector<uint8_t> {TEST_READING_432_MSB, TEST_READING_432_LSB}), group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::previous_meter_value)).reported<std::vector<uint8_t>>());  // Parsed previous value stays 4.32.
}

// Verifies the v1 Meter Report prefix parses without the fields introduced by later versions.
TEST_F(MeterCommandClassTest, ParsesV1MeterReportPrefix)
{
    // Version 1 carries the two property bytes and a one-byte meter value only.
    constexpr uint8_t meter_command_class = METER_COMMAND_CLASS_ID;
    constexpr uint8_t meter_report        = METER_REPORT_COMMAND_ID;
    const std::array<uint8_t, 5> frame {{
      meter_command_class,
      meter_report,
      TEST_METER_REPORT_V1_PROPERTIES_1,  // Electric meter type, default rate, and scale 0.
      TEST_METER_REPORT_V1_PROPERTIES_2,  // Precision 0, scale 0, and one-byte value Size.
      TEST_READING_42                     // Meter Value = 42.
    }};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_meter_core::report_received_args args {endpoint, parser, nullptr};

    // The legacy frame still produces its reading even though Delta Time and Scale 2 are absent.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_received(args));
    const auto reports = endpoint.children(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP));
    ASSERT_EQ(1, reports.size());                                                                                                                                                                            // The v1 frame creates one report group.
    EXPECT_EQ((std::vector<uint8_t> {TEST_READING_42}), reports.front().child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_value)).reported<std::vector<uint8_t>>());  // The one-byte reading remains 42.
    EXPECT_EQ(TEST_DELTA_TIME_DEFAULT, reports.front().child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::delta_time)).reported<uint16_t>());                                // Missing v1 Delta Time decodes to its zero default.
}

// Verifies v4+ Supported Report wire decoding honors rate, reset, M.S.T., and the advertised extension byte.
TEST_F(MeterCommandClassTest, ParsesV4SupportedReportWireFields)
{
    // This v4 Supported Report advertises Gas, Import+Export, Reset support, scales 1 and 2, then scale 7 in one extension byte.
    constexpr uint8_t meter_command_class    = METER_COMMAND_CLASS_ID;
    constexpr uint8_t meter_supported_report = METER_SUPPORTED_REPORT_COMMAND_ID;
    const std::array<uint8_t, 6> frame {{
      meter_command_class,
      meter_supported_report,
      TEST_SUPPORTED_REPORT_V4_PROPERTIES_1,            // Reset supported, both rates, and Gas meter type.
      TEST_SUPPORTED_REPORT_V4_PROPERTIES_2,            // M.S.T. set; first bitmap advertises scales 1 and 2.
      TEST_SCALE_EXTENSION_BYTE_COUNT,                  // One additional scale bitmap byte follows.
      TEST_SUPPORTED_REPORT_EXTENSION_SCALE_SEVEN_MASK  // Extension bitmap bit 0 advertises scale 7.
    }};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_meter_core::report_received_args args {endpoint, parser, nullptr};

    // Parse the raw command and verify each relevant supported-capability field.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received(args));
    const auto supported = endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::METER_SUPPORTED_REPORT_GROUP));
    ASSERT_TRUE(supported.is_valid());
    EXPECT_EQ(TEST_METER_TYPE_GAS, supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::meter_type)).reported<uint8_t>());
    EXPECT_EQ(TEST_RATE_TYPE_BOTH_MASKED, supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::rate_type)).reported<uint8_t>());
    EXPECT_TRUE(command_class_meter_attribute_store::get_reported_meter_reset_supported(endpoint));
    EXPECT_EQ(TEST_SUPPORTED_REPORT_SCALES_ONE_AND_TWO_MASK, supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported_0)).reported<uint8_t>());
    EXPECT_EQ((std::vector<uint8_t> {TEST_SUPPORTED_REPORT_EXTENSION_SCALE_SEVEN_MASK}), supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported)).reported<std::vector<uint8_t>>());
}

// Verifies v4 Supported Report omits the count and extension bytes when M.S.T. is clear.
TEST_F(MeterCommandClassTest, ParsesV4SupportedReportWithoutScaleExtension)
{
    // The second property byte advertises scale 0 and clears M.S.T., so the frame ends after that byte.
    constexpr uint8_t meter_command_class    = METER_COMMAND_CLASS_ID;
    constexpr uint8_t meter_supported_report = METER_SUPPORTED_REPORT_COMMAND_ID;
    const std::array<uint8_t, 4> frame {{
      meter_command_class,
      meter_supported_report,
      TEST_SUPPORTED_REPORT_IMPORT_PROPERTIES_1,  // Import is supported; Reset is not advertised.
      TEST_SUPPORTED_REPORT_SCALE_ZERO_MASK       // Scale 0 supported, with M.S.T. clear and no extension bytes.
    }};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_meter_core::report_received_args args {endpoint, parser, nullptr};

    // Parsing the v4 prefix must preserve Import and scale 0 without inventing extension support.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received(args));
    const auto supported = endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::METER_SUPPORTED_REPORT_GROUP));
    ASSERT_TRUE(supported.is_valid());
    EXPECT_EQ(TEST_RATE_TYPE_IMPORT_MASKED, supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::rate_type)).reported<uint8_t>());
    EXPECT_EQ(TEST_SUPPORTED_REPORT_SCALE_ZERO_MASK, supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported_0)).reported<uint8_t>());
    EXPECT_EQ(TEST_SUPPORTED_REPORT_MST_CLEAR, supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::m_s_t)).reported<uint8_t>());
    EXPECT_TRUE(supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported)).reported<std::vector<uint8_t>>().empty());
}

// Verifies the v2 Supported Report prefix parses when v4 rate and scale-extension fields are absent.
TEST_F(MeterCommandClassTest, ParsesV2SupportedReportPrefix)
{
    // Version 2 carries only Meter Reset/Meter Type and the low four supported-scale bits.
    constexpr uint8_t meter_command_class    = METER_COMMAND_CLASS_ID;
    constexpr uint8_t meter_supported_report = METER_SUPPORTED_REPORT_COMMAND_ID;
    const std::array<uint8_t, 4> frame {{
      meter_command_class,
      meter_supported_report,
      TEST_SUPPORTED_REPORT_V2_PROPERTIES_1,  // Reset supported and Gas meter type.
      TEST_SUPPORTED_REPORT_V2_SCALE_MASK     // Legacy bitmap advertises scales 0 and 2.
    }};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_meter_core::report_received_args args {endpoint, parser, nullptr};

    // The short legacy report stores its capabilities without treating missing extension bytes as supported.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received(args));
    const auto supported = endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::METER_SUPPORTED_REPORT_GROUP));
    ASSERT_TRUE(supported.is_valid());
    EXPECT_EQ(TEST_METER_TYPE_GAS, supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::meter_type)).reported<uint8_t>());
    EXPECT_TRUE(command_class_meter_attribute_store::get_reported_meter_reset_supported(endpoint));
    EXPECT_EQ(TEST_SUPPORTED_REPORT_V2_SCALE_MASK, supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported_0)).reported<uint8_t>());
    EXPECT_TRUE(supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported)).reported<std::vector<uint8_t>>().empty());
}

// Verifies v4 Supported Report storage retains scale extensions and the advertised reset capability.
TEST_F(MeterCommandClassTest, StoresV4SupportedScalesAndResetCapability)
{
    // Advertise v4 scales, both rate types, and Meter Reset support.
    command_class_meter_attribute_map_t report {
      {"meter_type", uint8_t {TEST_METER_TYPE_GAS}},                                             // Store a Gas meter's capabilities.
      {"rate_type", uint8_t {TEST_RATE_TYPE_BOTH_MASKED}},                                       // Advertise both Import and Export rates.
      {"meter_reset", uint8_t {TEST_RESET_SUPPORTED_MASKED}},                                    // Set the Meter Reset supported flag.
      {"scale_supported_0", uint8_t {TEST_SCALE_ZERO_AND_ONE_SUPPORTED_MASKED}},                 // Advertise scales 0 and 1 in bitmap byte 1.
      {"m_s_t", uint8_t {TEST_MST_SET}},                                                         // Indicate that an extended scale bitmap follows.
      {"number_of_scale_supported_bytes_to_follow", uint8_t {TEST_SCALE_EXTENSION_BYTE_COUNT}},  // Include one extended bitmap byte.
      {"scale_supported", std::vector<uint8_t> {TEST_SCALE_EXTENSION_SCALE_EIGHT}},              // Its bit 1 advertises logical scale 8.
    };
    // Store the advertised capabilities and verify the extension and reset fields.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received_store(endpoint, report));
    EXPECT_TRUE(command_class_meter_attribute_store::get_reported_meter_reset_supported(endpoint));
    auto supported = endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::METER_SUPPORTED_REPORT_GROUP));
    EXPECT_EQ((std::vector<uint8_t> {TEST_SCALE_EXTENSION_SCALE_EIGHT}), supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported)).reported<std::vector<uint8_t>>());

    // Clearing the advertised flag prevents reset operations.
    report["meter_reset"] = uint8_t {TEST_METER_RESET_UNSUPPORTED};  // Clear the reset-supported flag.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received_store(endpoint, report));
    EXPECT_FALSE(command_class_meter_attribute_store::get_reported_meter_reset_supported(endpoint));
}

// Verifies a field with the wrong decoded type uses the map fallback rather than storing unrelated data.
TEST_F(MeterCommandClassTest, ReportStoresUseDefaultsForWrongDecodedTypes)
{
    // Supply types that cannot be decoded as Supported Report fields and verify the safe defaults.
    const command_class_meter_attribute_map_t supported_report {
      {"meter_type", uint16_t {TEST_INVALID_TYPED_VALUE}},                                 // Wrong type: meter type must be one byte.
      {"rate_type", uint16_t {TEST_INVALID_TYPED_VALUE}},                                  // Wrong type: rate mask must be one byte.
      {"meter_reset", uint16_t {TEST_INVALID_TYPED_VALUE}},                                // Wrong type: reset flag must be one byte.
      {"scale_supported_0", uint16_t {TEST_INVALID_TYPED_VALUE}},                          // Wrong type: base scale mask must be one byte.
      {"m_s_t", uint16_t {TEST_INVALID_TYPED_VALUE}},                                      // Wrong type: M.S.T. flag must be one byte.
      {"number_of_scale_supported_bytes_to_follow", uint16_t {TEST_INVALID_TYPED_VALUE}},  // Wrong type: extension count must be one byte.
      {"scale_supported", uint8_t {TEST_INVALID_TYPED_VALUE}},                             // Wrong type: extended bitmap must be a byte vector.
    };
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received_store(endpoint, supported_report));
    auto supported = endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::METER_SUPPORTED_REPORT_GROUP));
    ASSERT_TRUE(supported.is_valid());
    EXPECT_EQ(TEST_METER_TYPE_DEFAULT, supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::meter_type)).reported<uint8_t>());     // Invalid Meter Type falls back to the undefined zero value.
    EXPECT_TRUE(supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported)).reported<std::vector<uint8_t>>().empty());  // Wrongly typed extension data stores no supported scales.

    // Repeat the malformed-type check for Meter Report fields and verify the reading remains empty.
    const command_class_meter_attribute_map_t report {
      {"meter_type", uint16_t {TEST_INVALID_TYPED_VALUE}},           // Wrong type: Meter Type must be one byte.
      {"rate_type", uint16_t {TEST_INVALID_TYPED_VALUE}},            // Wrong type: rate mask must be one byte.
      {"scale_bit_2", uint16_t {TEST_INVALID_TYPED_VALUE}},          // Wrong type: high scale field must be one byte.
      {"size", uint16_t {TEST_INVALID_TYPED_VALUE}},                 // Wrong type: value size must be one byte.
      {"scale_bits_10", uint16_t {TEST_INVALID_TYPED_VALUE}},        // Wrong type: low scale field must be one byte.
      {"precision", uint16_t {TEST_INVALID_TYPED_VALUE}},            // Wrong type: precision field must be one byte.
      {"scale_2", uint16_t {TEST_INVALID_TYPED_VALUE}},              // Wrong type: extended scale field must be one byte.
      {"delta_time", uint8_t {TEST_INVALID_TYPED_VALUE}},            // Wrong type: Delta Time must be two bytes.
      {"meter_value", uint8_t {TEST_INVALID_TYPED_VALUE}},           // Wrong type: Meter Value must be a byte vector.
      {"previous_meter_value", uint8_t {TEST_INVALID_TYPED_VALUE}},  // Wrong type: Previous Meter Value must be a byte vector.
    };
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_received_store(endpoint, report));
    auto report_group = endpoint.children(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP)).front();
    EXPECT_EQ(TEST_METER_TYPE_DEFAULT, report_group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_type)).reported<uint8_t>());  // Invalid Meter Type falls back to the undefined zero value.
    EXPECT_TRUE(report_group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_value)).reported<std::vector<uint8_t>>().empty());
}

// Verifies the v1 interview creates a direct Meter Report requirement rather than a Supported Get.
TEST_F(MeterCommandClassTest, InterviewV1StartsWithMeterReport)
{
    verify_interview_start(METER_CC_VERSION_1, true, false);
}

// Verifies the v2 interview begins by requesting the Supported Report.
TEST_F(MeterCommandClassTest, InterviewV2StartsWithSupportedGet)
{
    verify_interview_start(METER_CC_VERSION_2, false, true);
}

// Verifies the v3 interview begins by requesting the Supported Report.
TEST_F(MeterCommandClassTest, InterviewV3StartsWithSupportedGet)
{
    verify_interview_start(METER_CC_VERSION_3, false, true);
}

// Verifies the v4 interview begins by requesting the Supported Report.
TEST_F(MeterCommandClassTest, InterviewV4StartsWithSupportedGet)
{
    verify_interview_start(METER_CC_VERSION_4, false, true);
}

// Verifies the v5 interview begins by requesting the Supported Report.
TEST_F(MeterCommandClassTest, InterviewV5StartsWithSupportedGet)
{
    verify_interview_start(METER_CC_VERSION_5, false, true);
}

// Verifies the v6 interview begins by requesting the Supported Report.
TEST_F(MeterCommandClassTest, InterviewV6StartsWithSupportedGet)
{
    verify_interview_start(METER_CC_VERSION_6, false, true);
}

// Verifies v4 Supported Report scales and rate types create one required reading per supported pair.
TEST_F(MeterCommandClassTest, SupportedReportV4StartsEveryScaleAndRateReading)
{
    // Advertise two scales and both rate types so the scheduler must prepare each supported pair.
    set_version(METER_CC_VERSION_4);
    ASSERT_EQ(SL_STATUS_OK,
              component().on_meter_supported_report_received_store(endpoint,
                                                                   {
                                                                     {"meter_type", uint8_t {TEST_METER_TYPE_ELECTRIC}},                         // Use the Electric meter type.
                                                                     {"rate_type", uint8_t {TEST_RATE_TYPE_BOTH_MASKED}},                        // Advertise Import and Export.
                                                                     {"meter_reset", uint8_t {TEST_METER_RESET_UNSUPPORTED}},                    // Reset is not supported.
                                                                     {"scale_supported_0", uint8_t {TEST_SCALE_ZERO_AND_ONE_SUPPORTED_MASKED}},  // Advertise scales 0 and 1.
                                                                     {"m_s_t", uint8_t {TEST_MST_CLEAR}},                                        // No extended scale bitmap follows.
                                                                   }));

    // The first callback queues the first scale/rate Get and prepares report groups for all four pairs.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_parsed(nullptr, endpoint, {}));
    auto groups = endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP));
    ASSERT_EQ(1, std::distance(groups.begin(), groups.end()));                                                                                                         // Exactly one Supported Get request is scheduled.
    EXPECT_EQ(TEST_LOGICAL_SCALE_DEFAULT, groups.front().child_by_type(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::scale)).desired<uint8_t>());  // The request uses default scale 0.
    EXPECT_EQ(TEST_RATE_TYPE_IMPORT, groups.front().child_by_type(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::rate_type)).desired<uint8_t>());   // Import is requested first.
    auto first_report = endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP));
    EXPECT_TRUE(first_report.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_value)).is_valid());

    // Reporting one reading advances to the next not-yet-reported scale/rate pair.
    command_class_meter_attribute_map_t reading {
      {"rate_type", uint8_t {TEST_RATE_TYPE_IMPORT_MASKED}},    // The completed report is for Import.
      {"meter_value", std::vector<uint8_t> {TEST_READING_ONE}}  // A one-byte value marks this reading complete.
    };
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_received_store(endpoint, reading));
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, reading));
    groups = endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP));
    ASSERT_EQ(1, groups.size());                                                                                                                                      // Import support schedules exactly one Meter Get.
    EXPECT_EQ(TEST_RATE_TYPE_EXPORT, groups.front().child_by_type(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::rate_type)).desired<uint8_t>());  // Export is the next requested rate.
}

// Verifies v4 Supported Report decoding covers import, export, and extended-scale bitmasks.
TEST_F(MeterCommandClassTest, SupportedReportV4DecodesRateAndExtendedScaleBitmasks)
{
    // Start with one legacy scale and one scale bit in the extension byte.
    set_version(METER_CC_VERSION_4);
    command_class_meter_attribute_map_t report {
      {"meter_type", uint8_t {TEST_METER_TYPE_ELECTRIC}},                                        // Advertise the Electric meter type.
      {"meter_reset", uint8_t {TEST_METER_RESET_UNSUPPORTED}},                                   // This report does not advertise Meter Reset.
      {"scale_supported_0", uint8_t {TEST_SUPPORTED_SCALE_0}},                                   // The first scale bitmap supports logical scale 0.
      {"m_s_t", uint8_t {TEST_MORE_SCALE_BYTES_FLAG}},                                           // M.S.T. announces that scale bitmap bytes follow.
      {"number_of_scale_supported_bytes_to_follow", uint8_t {TEST_SCALE_EXTENSION_BYTE_COUNT}},  // Exactly one extension byte follows.
      {"scale_supported", std::vector<uint8_t> {TEST_SCALE_EXTENSION_SCALE_EIGHT}},              // Extension bit 1 advertises logical scale 8.
    };

    // Import-only support selects Import and keeps the extended scale from the following byte.
    report["rate_type"] = uint8_t {TEST_RATE_TYPE_IMPORT_MASKED};  // Advertise Import only.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received_store(endpoint, report));
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_parsed(nullptr, endpoint, {}));
    auto groups = endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP));
    ASSERT_EQ(1, groups.size());                                                                                                                                      // The base scale creates one report group.
    EXPECT_EQ(TEST_RATE_TYPE_IMPORT, groups.front().child_by_type(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::rate_type)).desired<uint8_t>());  // Import is the requested rate.
    auto extended = endpoint.children(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP));
    ASSERT_EQ(2, extended.size());  // The two advertised extended scales each create a Meter Report group.

    // Export-only support selects Export; an empty rate mask yields no reading request.
    report["rate_type"] = uint8_t {TEST_RATE_TYPE_EXPORT_MASKED};  // Replace Import with Export support.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received_store(endpoint, report));
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_parsed(nullptr, endpoint, {}));
    EXPECT_EQ(TEST_RATE_TYPE_EXPORT, groups.front().child_by_type(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::rate_type)).desired<uint8_t>());  // Export becomes the requested rate.
    report["rate_type"] = uint8_t {TEST_RATE_TYPE_NO_SUPPORT_MASKED};                                                                                                 // Clear both advertised rate bits.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received_store(endpoint, report));
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_parsed(nullptr, endpoint, {}));
    EXPECT_EQ(TEST_RATE_TYPE_EXPORT, groups.front().child_by_type(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::rate_type)).desired<uint8_t>());  // No new request replaces the already queued Export request.
}

// Verifies v2 Supported Report callback does not schedule readings without stored capabilities.
TEST_F(MeterCommandClassTest, SupportedReportCallbackV2HandlesMissingCapabilities)
{
    // A v2 callback with no Supported Report data must not invent reading requests.
    set_version(METER_CC_VERSION_2);
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_parsed(nullptr, endpoint, {}));
    EXPECT_TRUE(endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP)).empty());
}

// Verifies v1 Meter Report callback handles a missing provisional group and clears an interview requirement.
TEST_F(MeterCommandClassTest, MeterReportCallbackV1HandlesProvisionalGroup)
{
    // Simulate the initial provisional group and let the v1 callback clear its interview requirement.
    set_version(METER_CC_VERSION_1);
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    component().on_interview(endpoint, METER_CC_VERSION_1);
    auto report = endpoint.children(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP)).front();
    auto value  = report.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_value));
    EXPECT_TRUE(value.is_valid());
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    // The report value remains stored after the interview requirement is cleared.
    EXPECT_TRUE(value.is_valid());
}

// Verifies v2 Report callback creates a reading group from capabilities and skips complete readings.
TEST_F(MeterCommandClassTest, MeterReportCallbackV2CreatesMissingReadingGroupAndSkipsCompleteReadings)
{
    // Exercise creation, retry, and completion of the one supported v2 reading.
    set_version(METER_CC_VERSION_2);
    ASSERT_EQ(SL_STATUS_OK,
              component().on_meter_supported_report_received_store(endpoint,
                                                                   {
                                                                     {"meter_type", uint8_t {TEST_METER_TYPE_ELECTRIC}},                 // Advertise an Electric meter.
                                                                     {"meter_reset", uint8_t {TEST_METER_RESET_UNSUPPORTED}},            // Reset is not supported.
                                                                     {"scale_supported_0", uint8_t {TEST_SCALE_ZERO_SUPPORTED_MASKED}},  // Only scale 0 is supported.
                                                                   }));

    // This callback path receives supported data without the normal parsed callback creating report groups.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    auto readings = endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP));
    ASSERT_EQ(1, readings.size());                                                                                                                                       // Only the supported default scale is requested.
    EXPECT_EQ(TEST_LOGICAL_SCALE_DEFAULT, readings.front().child_by_type(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::scale)).desired<uint8_t>());  // The default supported scale is logical scale 0.

    // A second report while the value is still absent requests the same reading again.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    readings = endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP));
    EXPECT_EQ(1, readings.size());  // Reprocessing leaves the existing request in place.

    // Once that reading has a reported value, no additional Get is queued.
    auto report_group = endpoint.children(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP)).front();
    report_group.emplace_node(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_value)).set_reported(std::vector<uint8_t> {TEST_READING_ONE});  // A non-empty one-byte value marks the report complete.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    readings = endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP));
    EXPECT_EQ(1, readings.size());  // The existing request remains the only request.
}

// Verifies v4 Report storage skips incomplete groups before finding a complete scale/rate match.
TEST_F(MeterCommandClassTest, MeterReportStorageV4SkipsIncompleteMatchingGroups)
{
    // Build report groups with missing key children so lookup must ignore incomplete candidates.
    using group_type = meter_report_group_attributes_t;
    const std::array<group_type, 4> key_types {{group_type::scale_bit_2, group_type::scale_bits_10, group_type::scale_2, group_type::rate_type}};  // Three scale fields plus the rate form the lookup key.
    // Each incomplete group omits one key child; each unreported group has every child but one stored value.
    for (const auto missing: key_types) {
        auto group = endpoint.add_node(static_cast<attribute_store_type_t>(group_type::METER_REPORT_GROUP));
        for (const auto field: key_types) {
            if (field != missing) {
                group.emplace_node(static_cast<attribute_store_type_t>(field)).set_reported(uint8_t {TEST_KEY_FIELD_CLEAR});  // Present key field with its clear bitmap value.
            }
        }
    }

    // Fully reported groups that differ in one key field must also be skipped before the matching group is found.
    // Add complete groups whose scale or rate differs from the incoming report's key.
    const std::array<std::array<uint8_t, 4>, 4> nonmatching_keys {{
      meter_report_key(TEST_LOGICAL_SCALE_FOUR, TEST_RATE_TYPE_DEFAULT),    // Scale 4, default rate.
      meter_report_key(TEST_LOGICAL_SCALE_ONE, TEST_RATE_TYPE_DEFAULT),     // Scale 1, default rate.
      meter_report_key(TEST_LOGICAL_SCALE_EIGHT, TEST_RATE_TYPE_DEFAULT),   // Extended scale 8, default rate.
      meter_report_key(TEST_LOGICAL_SCALE_DEFAULT, TEST_RATE_TYPE_IMPORT),  // Scale 0, Import rate.
    }};
    for (const auto &keys: nonmatching_keys) {
        auto group = endpoint.add_node(static_cast<attribute_store_type_t>(group_type::METER_REPORT_GROUP));
        for (size_t index = 0; index < key_types.size(); ++index) {
            group.emplace_node(static_cast<attribute_store_type_t>(key_types[index])).set_reported(keys[index]);
        }
    }
    // Present but unreported key children are invalid matches as well.
    for (const auto missing: key_types) {
        auto group = endpoint.add_node(static_cast<attribute_store_type_t>(group_type::METER_REPORT_GROUP));
        for (const auto field: key_types) {
            auto node = group.emplace_node(static_cast<attribute_store_type_t>(field));
            if (field != missing) {
                node.set_reported(uint8_t {TEST_KEY_FIELD_CLEAR});  // Mark all present key fields as reported with clear values.
            }
        }
    }

    // Store a report matching scale zero and the default rate after all invalid candidate groups.
    ASSERT_EQ(SL_STATUS_OK,
              component().on_meter_report_received_store(endpoint,
                                                         {
                                                           {"meter_type", uint8_t {TEST_METER_TYPE_ELECTRIC}},       // Report a reading for an Electric meter.
                                                           {"rate_type", uint8_t {TEST_RATE_TYPE_DEFAULT}},          // Use the default rate.
                                                           {"scale_bit_2", uint8_t {TEST_SCALE_BIT_2_CLEAR}},        // Clear the high scale bit.
                                                           {"scale_bits_10", uint8_t {TEST_SCALE_BITS_10_CLEAR}},    // Clear the low scale bits for logical scale 0.
                                                           {"scale_2", uint8_t {TEST_SCALE_2_CLEAR}},                // No extended scale for logical scale 0.
                                                           {"size", uint8_t {TEST_METER_VALUE_SIZE_ONE_BYTE}},       // Meter Value contains one byte.
                                                           {"precision", uint8_t {TEST_PRECISION_ZERO}},             // No decimal places are applied.
                                                           {"meter_value", std::vector<uint8_t> {TEST_READING_42}},  // Reading value is 42.
                                                         }));
    const auto groups = endpoint.children(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP));
    ASSERT_EQ(13, groups.size());  // 4 missing-key + 4 different-key + 4 unreported-key groups + 1 newly stored match.
    EXPECT_TRUE(groups.back().child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_value)).reported_exists());
}

// Verifies v2 Supported Report capability checks distinguish missing, unreported, and advertised fields.
TEST_F(MeterCommandClassTest, SupportedReportCapabilityChecksV2)
{
    // Check reset capability while its attribute is absent, unreported, cleared, and set.
    EXPECT_FALSE(command_class_meter_attribute_store::get_reported_meter_reset_supported(endpoint));
    auto supported = endpoint.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::METER_SUPPORTED_REPORT_GROUP));
    auto reset     = supported.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::meter_reset));
    EXPECT_FALSE(command_class_meter_attribute_store::get_reported_meter_reset_supported(endpoint));
    reset.set_reported(uint8_t {TEST_METER_RESET_UNSUPPORTED});  // Report the flag as cleared.
    EXPECT_FALSE(command_class_meter_attribute_store::get_reported_meter_reset_supported(endpoint));
    reset.set_reported(uint8_t {TEST_RESET_SUPPORTED_MASKED});  // Set the masked reset-supported bit.
    EXPECT_TRUE(command_class_meter_attribute_store::get_reported_meter_reset_supported(endpoint));

    // Then distinguish a missing scale mask from an empty or non-empty reported mask.
    set_version(METER_CC_VERSION_2);
    // Missing scale support prevents reading requests; a present empty mask is valid and yields no requests.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    auto scale0 = supported.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported_0));
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    scale0.set_reported(uint8_t {TEST_RATE_TYPE_NO_SUPPORT_MASKED});  // Report an empty scale mask.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    EXPECT_TRUE(endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP)).empty());

    // A reported v2 scale mask schedules the supported default-scale reading.
    scale0.set_reported(uint8_t {TEST_SCALE_ZERO_SUPPORTED_MASKED});  // Advertise support for scale 0.
    supported.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::m_s_t));
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    EXPECT_FALSE(endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP)).empty());
}

// Verifies v4 only prepares extended-scale reads when M.S.T. and a following scale bit are advertised.
TEST_F(MeterCommandClassTest, SupportedReportCapabilityChecksV4ScaleExtension)
{
    set_version(METER_CC_VERSION_4);
    auto supported = endpoint.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::METER_SUPPORTED_REPORT_GROUP));
    supported.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported_0)).set_reported(uint8_t {TEST_SCALE_ZERO_SUPPORTED_MASKED});  // Advertise scale 0.
    supported.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::rate_type)).set_reported(uint8_t {TEST_RATE_TYPE_IMPORT_MASKED});              // Advertise Import.
    auto marker = supported.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::m_s_t));
    marker.set_reported(uint8_t {TEST_MST_CLEAR});  // No extended scale byte is advertised initially.

    // With M.S.T. clear, only the first scale byte contributes a reading group.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_parsed(nullptr, endpoint, {}));
    auto reports = endpoint.children(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP));
    ASSERT_EQ(1, reports.size());  // Scale 0 creates one reading request.

    // M.S.T. set with one following byte advertises scale 7 and creates its required report group.
    marker.set_reported(uint8_t {TEST_MST_SET});                                                                                                                                                                // Announce that an extended scale bitmap follows.
    supported.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::number_of_scale_supported_bytes_to_follow)).set_reported(uint8_t {TEST_SCALE_EXTENSION_BYTE_COUNT});  // Provide one extension bitmap byte.
    supported.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported)).set_reported(std::vector<uint8_t> {TEST_SCALE_EXTENSION_SCALE_SEVEN});              // Extension bit 0 advertises scale 7.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_parsed(nullptr, endpoint, {}));
    reports = endpoint.children(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP));
    EXPECT_EQ(2, reports.size());  // Scale 0 and the newly advertised scale 7 each have a request.
}

// Verifies v4 reading requests wait until the supported rate capability is reported.
TEST_F(MeterCommandClassTest, ReadingRequestsV4WaitForReportedRateCapability)
{
    set_version(METER_CC_VERSION_4);
    auto supported = endpoint.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::METER_SUPPORTED_REPORT_GROUP));
    supported.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported_0)).set_reported(uint8_t {TEST_SCALE_ZERO_SUPPORTED_MASKED});  // Advertise scale 0.

    // A supported group without its reset flag does not advertise reset support.
    EXPECT_FALSE(command_class_meter_attribute_store::get_reported_meter_reset_supported(endpoint));
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    EXPECT_TRUE(endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP)).empty());

    // Creating the rate attribute alone is insufficient; its reported capability is required.
    auto rate = supported.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::rate_type));
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    EXPECT_TRUE(endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP)).empty());

    rate.set_reported(uint8_t {TEST_RATE_TYPE_IMPORT_MASKED});  // Report Import as supported.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    const auto requests = endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP));
    ASSERT_EQ(1, requests.size());                                                                                                                                       // The supported default scale schedules one request.
    EXPECT_EQ(TEST_LOGICAL_SCALE_DEFAULT, requests.front().child_by_type(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::scale)).desired<uint8_t>());  // Request default scale 0.
    EXPECT_EQ(TEST_RATE_TYPE_IMPORT, requests.front().child_by_type(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::rate_type)).desired<uint8_t>());   // Request Import.
}

// Verifies the v1 peer receives the controller's current Meter Get encoding.
TEST_F(MeterCommandClassTest, MeterGetV1PeerUsesControllerVersionFormat)
{
    verify_meter_get_peer_format(METER_CC_VERSION_1);
}

// Verifies the v2 peer receives the controller's current Meter Get encoding.
TEST_F(MeterCommandClassTest, MeterGetV2PeerUsesControllerVersionFormat)
{
    verify_meter_get_peer_format(METER_CC_VERSION_2);
}

// Verifies the v3 peer receives the controller's current Meter Get encoding.
TEST_F(MeterCommandClassTest, MeterGetV3PeerUsesControllerVersionFormat)
{
    verify_meter_get_peer_format(METER_CC_VERSION_3);
}

// Verifies the v4 peer receives the controller's current Meter Get encoding.
TEST_F(MeterCommandClassTest, MeterGetV4PeerUsesControllerVersionFormat)
{
    verify_meter_get_peer_format(METER_CC_VERSION_4);
}

// Verifies the v5 peer receives the controller's current Meter Get encoding.
TEST_F(MeterCommandClassTest, MeterGetV5PeerUsesControllerVersionFormat)
{
    verify_meter_get_peer_format(METER_CC_VERSION_5);
}

// Verifies the v6 peer receives the controller's current Meter Get encoding.
TEST_F(MeterCommandClassTest, MeterGetV6PeerUsesControllerVersionFormat)
{
    verify_meter_get_peer_format(METER_CC_VERSION_6);
}

// Verifies v6 Meter Reset assembly requires each field added to its payload.
TEST_F(MeterCommandClassTest, MeterResetV6RequiresEveryPayloadField)
{
    using group_type = meter_reset_group_attributes_t;
    const std::array<std::function<void(attribute_store::attribute)>, 6> setters {{
      [this](auto group) { desired(group, group_type::meter_type, uint8_t {TEST_METER_TYPE_WATER}); },                                     // Water meter type.
      [this](auto group) { desired(group, group_type::rate_type, uint8_t {TEST_RATE_TYPE_EXPORT}); },                                      // Export rate.
      [this](auto group) { desired(group, group_type::size, uint8_t {TEST_METER_VALUE_SIZE_TWO_BYTES}); },                                 // Size declares a two-byte Meter Value.
      [this](auto group) { desired(group, group_type::precision, uint8_t {TEST_PRECISION_TWO_DECIMAL_PLACES}); },                          // Two decimal places.
      [this](auto group) { desired(group, group_type::meter_value, std::vector<uint8_t> {TEST_READING_500_MSB, TEST_READING_500_LSB}); },  // Value 500 in big-endian order.
      [this](auto group) { desired(group, group_type::scale_bits_10, uint8_t {TEST_LOGICAL_SCALE_EIGHT}); },                               // Low scale bits for the extended scale request.
    }};

    set_version(METER_CC_VERSION_6);
    uint8_t data[TEST_FRAME_BUFFER_SIZE] {};
    uint16_t length = 0;
    zwave_frame_generator generator {METER_COMMAND_CLASS_ID};
    for (size_t missing = 0; missing < setters.size(); ++missing) {
        auto group = endpoint.add_node(static_cast<attribute_store_type_t>(group_type::METER_RESET_GROUP));
        for (size_t field = 0; field < setters.size(); ++field) {
            if (field != missing) {
                setters[field](group);
            }
        }
        generator.initialize_frame(METER_RESET_COMMAND_ID, data, &length);
        command_class_meter_core::set_requested_args args {group, &generator};
        EXPECT_EQ(SL_STATUS_NOT_READY, component().on_meter_reset_requested_assemble_frame(args, data, &length)) << "missing field index " << missing;
    }
}

// Verifies the new Meter Reset command ID is unavailable before version 2.
TEST_F(MeterCommandClassTest, MeterResetV1CommandIdIsUnavailable)
{
    uint8_t data[TEST_FRAME_BUFFER_SIZE] {};
    uint16_t length = 0;
    zwave_frame_generator generator {METER_COMMAND_CLASS_ID};
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::METER_RESET_GROUP));
    command_class_meter_core::set_requested_args args {group, &generator};

    // Meter Reset was introduced in v2, so a v1 peer must not receive its command ID.
    set_version(METER_CC_VERSION_1);
    generator.initialize_frame(METER_RESET_COMMAND_ID, data, &length);
    EXPECT_EQ(SL_STATUS_FAIL, component().on_meter_reset_requested_assemble_frame(args, data, &length));
}

// Verifies v6 Meter Reset rejects a Size value that disagrees with the Meter Value byte count.
TEST_F(MeterCommandClassTest, MeterResetV6RequiresSizeToMatchMeterValue)
{
    // Prepare every required v6 field, but advertise one value byte for the two-byte Meter Value.
    set_version(METER_CC_VERSION_6);
    uint8_t data[16] {};
    uint16_t length = 0;
    zwave_frame_generator generator {METER_COMMAND_CLASS_ID};
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::METER_RESET_GROUP));
    desired(group, meter_reset_group_attributes_t::meter_type, uint8_t {TEST_METER_TYPE_WATER});                                     // Water meter type.
    desired(group, meter_reset_group_attributes_t::rate_type, uint8_t {TEST_RATE_TYPE_EXPORT});                                      // Export rate.
    desired(group, meter_reset_group_attributes_t::size, uint8_t {TEST_METER_VALUE_SIZE_ONE_BYTE});                                  // Deliberately says one byte.
    desired(group, meter_reset_group_attributes_t::precision, uint8_t {TEST_PRECISION_TWO_DECIMAL_PLACES});                          // Two decimal places.
    desired(group, meter_reset_group_attributes_t::meter_value, std::vector<uint8_t> {TEST_READING_500_MSB, TEST_READING_500_LSB});  // Deliberately contains two bytes for mismatch coverage.
    desired(group, meter_reset_group_attributes_t::scale_bits_10, uint8_t {TEST_LOGICAL_SCALE_EIGHT});                               // Scale 8 for this v6 reset.

    // The size field specifies the number of Meter Value bytes in the v6 reset payload.
    generator.initialize_frame(METER_RESET_COMMAND_ID, data, &length);
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, component().on_meter_reset_requested_assemble_frame({group, &generator}, data, &length));
}

// Verifies the v2 peer receives the controller's current Meter Reset encoding.
TEST_F(MeterCommandClassTest, MeterResetV2PeerUsesControllerVersionFormat)
{
    verify_meter_reset_peer_format(METER_CC_VERSION_2);
}

// Verifies the v3 peer receives the controller's current Meter Reset encoding.
TEST_F(MeterCommandClassTest, MeterResetV3PeerUsesControllerVersionFormat)
{
    verify_meter_reset_peer_format(METER_CC_VERSION_3);
}

// Verifies the v4 peer receives the controller's current Meter Reset encoding.
TEST_F(MeterCommandClassTest, MeterResetV4PeerUsesControllerVersionFormat)
{
    verify_meter_reset_peer_format(METER_CC_VERSION_4);
}

// Verifies the v5 peer receives the controller's current Meter Reset encoding.
TEST_F(MeterCommandClassTest, MeterResetV5PeerUsesControllerVersionFormat)
{
    verify_meter_reset_peer_format(METER_CC_VERSION_5);
}

// Verifies the v6 peer receives the controller's current Meter Reset encoding.
TEST_F(MeterCommandClassTest, MeterResetV6PeerUsesControllerVersionFormat)
{
    verify_meter_reset_peer_format(METER_CC_VERSION_6);
}

// Verifies v1 MQTT Get accepts its empty command payload and newer command IDs are unavailable.
TEST_F(MeterCommandClassTest, MqttCommandsFollowV1Availability)
{
    // A v1 peer supports Meter Get, while Supported Get and Meter Reset are newer command IDs.
    set_version(METER_CC_VERSION_1);
    EXPECT_EQ(SL_STATUS_FAIL, component().mqtt_on_meter_supported_get_command(endpoint, "{}"));
    EXPECT_EQ(SL_STATUS_FAIL, component().mqtt_on_meter_reset_command(endpoint, "{}"));
    EXPECT_EQ(SL_STATUS_OK, component().mqtt_on_meter_get_command(endpoint, "not-json"));
}

// Verifies v2 MQTT Get validates scale and Reset requires its advertised capability.
TEST_F(MeterCommandClassTest, MqttCommandsFollowV2FieldsAndCapabilities)
{
    // Check missing, malformed, out-of-range, and valid scale values for v2 Meter Get.
    set_version(METER_CC_VERSION_2);
    const auto missing_scale   = nlohmann::json::object();                                  // No scale member is supplied.
    const auto text_scale      = nlohmann::json {{"scale", "electric"}};                    // Scale must be numeric, not a label.
    const auto negative_scale  = nlohmann::json {{"scale", TEST_NEGATIVE_SCALE}};           // Scale cannot be negative.
    const auto oversized_scale = nlohmann::json {{"scale", TEST_MQTT_SCALE_OUT_OF_RANGE}};  // Scale must fit its protocol field.
    const auto valid_scale     = nlohmann::json {{"scale", TEST_LOGICAL_SCALE_TWO}};        // Scale 2 is a valid v2 scale.
    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_get_command(endpoint, missing_scale.dump()));
    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_get_command(endpoint, text_scale.dump()));
    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_get_command(endpoint, negative_scale.dump()));
    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_get_command(endpoint, oversized_scale.dump()));
    EXPECT_EQ(SL_STATUS_OK, component().mqtt_on_meter_get_command(endpoint, valid_scale.dump()));
    // Supported Get is available in v2; Reset additionally requires the device to advertise support.
    EXPECT_EQ(SL_STATUS_OK, component().mqtt_on_meter_supported_get_command(endpoint, "{}"));
    EXPECT_EQ(SL_STATUS_FAIL, component().mqtt_on_meter_reset_command(endpoint, "{}"));
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received_store(endpoint, {{"meter_reset", uint8_t {TEST_RESET_SUPPORTED_MASKED}}}));  // Advertise Reset support.
    EXPECT_EQ(SL_STATUS_OK, component().mqtt_on_meter_reset_command(endpoint, "{}"));
}

// Verifies v4 MQTT Get requires a valid Import or Export rate type.
TEST_F(MeterCommandClassTest, MqttMeterGetV4RequiresSupportedRateType)
{
    // Require a numeric Import or Export rate type alongside a valid scale.
    set_version(METER_CC_VERSION_4);
    const auto missing_rate   = nlohmann::json {{"scale", TEST_LOGICAL_SCALE_DEFAULT}};                                               // V4 requires Rate Type.
    const auto text_rate      = nlohmann::json {{"scale", TEST_LOGICAL_SCALE_DEFAULT}, {"rate_type", "import"}};                      // Rate Type must be numeric.
    const auto oversized_rate = nlohmann::json {{"scale", TEST_LOGICAL_SCALE_DEFAULT}, {"rate_type", TEST_MQTT_SCALE_OUT_OF_RANGE}};  // Rate Type must fit its two-bit field.
    const auto valid_import   = nlohmann::json {{"scale", TEST_LOGICAL_SCALE_DEFAULT}, {"rate_type", TEST_RATE_TYPE_IMPORT}};         // Import is a supported logical rate value.
    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_get_command(endpoint, missing_rate.dump()));
    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_get_command(endpoint, text_rate.dump()));
    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_get_command(endpoint, oversized_rate.dump()));
    EXPECT_EQ(SL_STATUS_OK, component().mqtt_on_meter_get_command(endpoint, valid_import.dump()));
}

// Verifies v6 MQTT Meter Reset validates value length and stores Size with its payload.
TEST_F(MeterCommandClassTest, MqttMeterResetV6ChecksValueLengthAndStoresPayload)
{
    // Enable Reset, then check that every v6 JSON field is required.
    set_version(METER_CC_VERSION_6);
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received_store(endpoint, {{"meter_reset", uint8_t {TEST_RESET_SUPPORTED_MASKED}}}));  // Advertise Reset support.

    // Use Electric/Import, default scale, precision 2, and a one-byte reading as the valid reset payload.
    const auto complete = nlohmann::json {{"meter_type", TEST_METER_TYPE_ELECTRIC}, {"rate_type", TEST_RATE_TYPE_IMPORT}, {"scale", TEST_LOGICAL_SCALE_DEFAULT}, {"precision", TEST_PRECISION_TWO_DECIMAL_PLACES}, {"meter_value", std::array<uint8_t, 1> {TEST_READING_ONE}}};
    for (const auto *field: {"meter_type", "rate_type", "scale", "precision", "meter_value"}) {
        auto payload = complete;
        payload.erase(field);
        EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_reset_command(endpoint, payload.dump())) << field;
    }
    // Change one field at a time to a string, which is the wrong JSON type for these numeric fields and byte values.
    auto wrong_meter_type                    = complete;
    wrong_meter_type["meter_type"]           = "electric";
    auto wrong_rate_type                     = complete;
    wrong_rate_type["rate_type"]             = "import";
    auto wrong_scale                         = complete;
    wrong_scale["scale"]                     = "kWh";
    auto wrong_precision                     = complete;
    wrong_precision["precision"]             = "high";
    auto wrong_meter_value_element           = complete;
    wrong_meter_value_element["meter_value"] = std::array<std::string, 1> {"one"};
    const std::array<nlohmann::json, 5> wrong_types {{wrong_meter_type, wrong_rate_type, wrong_scale, wrong_precision, wrong_meter_value_element}};
    for (const auto &payload: wrong_types) {
        EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_reset_command(endpoint, payload.dump()));
    }

    // Meter Reset Size is restricted by the spec to 1, 2, or 4 bytes; reject lengths 0, 3, and 5.
    auto empty_value                = complete;
    empty_value["meter_value"]      = std::array<uint8_t, 0> {};
    auto three_byte_value           = complete;
    three_byte_value["meter_value"] = std::array<uint8_t, TEST_METER_VALUE_SIZE_THREE_BYTES> {TEST_READING_ONE, TEST_READING_TWO, TEST_READING_THREE};
    auto overlong_value             = complete;
    overlong_value["meter_value"]   = std::array<uint8_t, TEST_MQTT_VALUE_TOO_LONG_BYTES> {TEST_READING_ONE, TEST_READING_TWO, TEST_READING_THREE, TEST_READING_FOUR, TEST_READING_FIVE};
    auto four_byte_value            = complete;
    four_byte_value["meter_value"]  = std::array<uint8_t, TEST_METER_VALUE_SIZE_FOUR_BYTES> {TEST_READING_ZERO, TEST_READING_ZERO, TEST_READING_ZERO, TEST_READING_FIVE};
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, component().mqtt_on_meter_reset_command(endpoint, empty_value.dump()));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, component().mqtt_on_meter_reset_command(endpoint, three_byte_value.dump()));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, component().mqtt_on_meter_reset_command(endpoint, overlong_value.dump()));
    EXPECT_EQ(SL_STATUS_OK, component().mqtt_on_meter_reset_command(endpoint, four_byte_value.dump()));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::METER_RESET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(TEST_METER_VALUE_SIZE_FOUR_BYTES, group.child_by_type(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::size)).desired<uint8_t>());                                                                                         // Size records four value bytes.
    EXPECT_EQ((std::vector<uint8_t> {TEST_READING_ZERO, TEST_READING_ZERO, TEST_READING_ZERO, TEST_READING_FIVE}), group.child_by_type(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::meter_value)).desired<std::vector<uint8_t>>());  // The value bytes are preserved in order.
    // Confirm a request with no required payload fields is still rejected.
    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_reset_command(endpoint, "{}"));
}

// Verifies v1 Meter Get MQTT route dispatch creates its request group.
TEST_F(MeterCommandClassTest, RegisteredMqttMeterGetV1Dispatches)
{
    // Dispatch the registered Meter Get route and verify it creates a v1 request group.
    set_version(METER_CC_VERSION_1);
    mqtt().mqtt_command_handler(endpoint, "MeterGet", "{}");
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP)).is_valid());
}

// Verifies v2 Supported Get and Reset MQTT routes dispatch to their request groups.
TEST_F(MeterCommandClassTest, RegisteredMqttSupportedGetAndResetV2Dispatch)
{
    // Dispatch Supported Get, then advertise reset support before dispatching Meter Reset.
    set_version(METER_CC_VERSION_2);
    mqtt().mqtt_command_handler(endpoint, "MeterSupportedGet", "{}");
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_supported_get_group_attributes_t::METER_SUPPORTED_GET_GROUP)).is_valid());

    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received_store(endpoint, {{"meter_reset", uint8_t {TEST_RESET_SUPPORTED_MASKED}}}));  // Advertise Meter Reset support before dispatching the route.
    mqtt().mqtt_command_handler(endpoint, "MeterReset", "{}");
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::METER_RESET_GROUP)).is_valid());
    mqtt().mqtt_command_handler(endpoint, "MeterReset", "{}");
    mqtt().mqtt_command_handler(endpoint, "MeterSupportedGet", "{}");
}
