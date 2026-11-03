/******************************************************************************
 * # License
 * <b>Copyright 2026 Silicon Laboratories Inc. www.silabs.com</b>
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of the Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and governed by the
 * sections of the MSLA applicable to Source Code.
 ******************************************************************************/
#include <gtest/gtest.h>

#include <array>
#include <vector>

#include "attribute.hpp"
#include "attribute_store_fixt.h"
#include "command_class_battery.hpp"
#include "command_class_battery_core.hpp"
#include "command_class_battery_types.hpp"
#include "zwave_frame_parser.hpp"

using namespace zwave_command_class;

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}
using namespace zwave_command_class::command_class_battery_types;

class TestableBatteryMqtt : public command_class_battery_mqtt
{
    public:
        using zwave_command_class_base::mqtt_command_handler;
};

namespace
{
    constexpr uint8_t BATTERY_CC_ID                 = 0x80;  // COMMAND_CLASS_BATTERY
    constexpr uint8_t BATTERY_GET_COMMAND           = 0x02;  // BATTERY_GET
    constexpr uint8_t BATTERY_REPORT_COMMAND        = 0x03;  // BATTERY_REPORT
    constexpr uint8_t BATTERY_HEALTH_GET_COMMAND    = 0x04;  // BATTERY_HEALTH_GET, added in version 2
    constexpr uint8_t BATTERY_HEALTH_REPORT_COMMAND = 0x05;  // BATTERY_HEALTH_REPORT, added in version 2
    constexpr uint8_t BATTERY_VERSION_1             = 1;
    constexpr uint8_t BATTERY_VERSION_2             = 2;
    constexpr uint8_t BATTERY_VERSION_3             = 3;
    constexpr uint8_t TEST_LEVEL                    = 72;  // 72%, inside the 0x00..0x64 percentage range
    constexpr uint8_t TEST_MAXIMUM_CAPACITY         = 80;  // 80%, inside the 0x00..0x64 capacity range
    // Properties 1 bits. Each status field is stored as its mask, so the byte is their sum.
    constexpr uint8_t REPLACE_RECHARGE_BIT = 0x01;  // bits 0-1, value 1: replace or recharge
    constexpr uint8_t LOW_FLUID_BIT        = 0x04;  // bit 2
    constexpr uint8_t OVERHEATING_BIT      = 0x08;  // bit 3
    constexpr uint8_t RECHARGEABLE_BIT     = 0x20;  // bit 5
    constexpr uint8_t TEST_PROPERTIES_1    = static_cast<uint8_t>(REPLACE_RECHARGE_BIT | LOW_FLUID_BIT | OVERHEATING_BIT | RECHARGEABLE_BIT);
    constexpr uint8_t DISCONNECTED_BIT     = 0x01;  // Properties 2 bit 0
    constexpr uint8_t LOW_TEMPERATURE_BIT  = 0x02;  // Properties 2 bit 1. The field value is 1 when this bit is set.
    // Health Report properties: Size is the low 3 bits, Scale is bits 3-4, Precision is bits 5-7.
    constexpr uint8_t HEALTH_SIZE_TWO_BYTES      = 0x02;
    constexpr uint8_t HEALTH_SCALE_CELSIUS_SHIFT = 3;
    constexpr uint8_t HEALTH_SCALE_CELSIUS       = static_cast<uint8_t>(2 << HEALTH_SCALE_CELSIUS_SHIFT);
    constexpr uint8_t HEALTH_PRECISION_NONE      = 0x00;
    constexpr uint8_t TEST_HEALTH_PROPERTIES     = static_cast<uint8_t>(HEALTH_SIZE_TWO_BYTES | HEALTH_SCALE_CELSIUS | HEALTH_PRECISION_NONE);
    constexpr uint8_t TEST_TEMPERATURE_MSB       = 0x00;  // 0x0019 is 25 with precision 0
    constexpr uint8_t TEST_TEMPERATURE_LSB       = 0x19;
    constexpr uint8_t HEALTH_GET_PENDING         = 1;
    constexpr uint8_t HEALTH_GET_DONE            = 0;
    constexpr uint16_t TEST_NODE_ID              = 2;  // Fixture node, not a protocol constant
    constexpr uint8_t TEST_ENDPOINT_ID           = 0;  // Root endpoint
    constexpr size_t FRAME_BUFFER_SIZE           = 8;
    // Fixture home ID. The four bytes only identify the RAM tree.
    constexpr std::array<uint8_t, 4> TEST_HOME_ID {0x12, 0x34, 0x56, 0x78};

    const auto REPORT_GROUP     = static_cast<attribute_store_type_t>(battery_report_group_attributes_t::BATTERY_REPORT_GROUP);
    const auto BATTERY_LEVEL    = static_cast<attribute_store_type_t>(battery_report_group_attributes_t::battery_level);
    const auto GET_GROUP        = static_cast<attribute_store_type_t>(battery_get_group_attributes_t::BATTERY_GET_GROUP);
    const auto HEALTH_GET_GROUP = static_cast<attribute_store_type_t>(battery_health_get_group_attributes_t::BATTERY_HEALTH_GET_GROUP);
}  // namespace

class BatteryCommandClassTest : public ::testing::Test
{
    protected:
        static TestableBatteryMqtt &mqtt()
        {
            static TestableBatteryMqtt instance;
            return instance;
        }

        static command_class_battery &component()
        {
            static command_class_battery instance;
            return instance;
        }

        void SetUp() override
        {
            ASSERT_EQ(SL_STATUS_OK, attribute_store_init());
            auto home = attribute_store::attribute::root().emplace_node(ATTRIBUTE_HOME_ID, std::vector<uint8_t>(TEST_HOME_ID.begin(), TEST_HOME_ID.end()));
            auto node = home.emplace_node(ATTRIBUTE_NODE_ID, TEST_NODE_ID);
            endpoint  = node.emplace_node(ATTRIBUTE_ENDPOINT_ID, TEST_ENDPOINT_ID);
            (void)component();
            (void)mqtt();
        }

        void TearDown() override
        {
            attribute_store_teardown();
        }

        void set_peer_version(uint8_t version)
        {
            endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_BATTERY)).set_reported(version);
        }

        sl_status_t assemble_get(attribute_store::attribute group, uint8_t command, uint8_t *frame, uint16_t *frame_length)
        {
            zwave_frame_generator generator {BATTERY_CC_ID};
            generator.initialize_frame(command, frame, frame_length);
            command_class_battery_core::get_requested_args args {group, &generator};
            if (command == BATTERY_HEALTH_GET_COMMAND) {
                return component().on_battery_health_get_requested(args, frame, frame_length);
            }
            return component().on_battery_get_requested(args, frame, frame_length);
        }

        attribute_store::attribute endpoint;
};

// Spec 2026B Battery CC v1, Battery Level (CC:0080.01.03.11.002).
// A v1 Report is Command Class, BATTERY_REPORT, and one percentage byte. That percentage is stored.
TEST_F(BatteryCommandClassTest, ReportV1StoresBatteryLevel)
{
    set_peer_version(BATTERY_VERSION_1);
    const std::array<uint8_t, 3> report {BATTERY_CC_ID, BATTERY_REPORT_COMMAND, TEST_LEVEL};
    zwave_frame_parser parser {report.data(), static_cast<uint16_t>(report.size())};
    command_class_battery_core::report_received_args args {endpoint, parser, nullptr};

    ASSERT_EQ(SL_STATUS_OK, component().on_battery_report_received(args));

    const auto group = endpoint.child_by_type(REPORT_GROUP);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(TEST_LEVEL, group.child_by_type(BATTERY_LEVEL).reported<uint8_t>());
}

// Spec 2026B Battery CC v2 Report, incoming longer frame (CC:0000.00.00.11.002, CC:0080.01.03.11.002).
// The v2 frame keeps the v1 Battery Level and appends Properties 1 and Properties 2. The level is stored.
TEST_F(BatteryCommandClassTest, ReportV2StoresBatteryLevelPrefix)
{
    set_peer_version(BATTERY_VERSION_2);
    // Properties 1 is recharge, low fluid, overheating, and rechargeable. Properties 2 is Disconnected only.
    const std::array<uint8_t, 5> frame {BATTERY_CC_ID, BATTERY_REPORT_COMMAND, TEST_LEVEL, TEST_PROPERTIES_1, DISCONNECTED_BIT};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_battery_core::report_received_args args {endpoint, parser, nullptr};

    ASSERT_EQ(SL_STATUS_OK, component().on_battery_report_received(args));

    const auto group = endpoint.child_by_type(REPORT_GROUP);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(TEST_LEVEL, group.child_by_type(BATTERY_LEVEL).reported<uint8_t>());
}

// Spec 2026B Battery CC v3 Report (CC:0080.03.00.21.001, CC:0080.01.03.11.002).
// Version 3 adds Low Temperature Status in Properties 2 and leaves Battery Level unchanged. The level is stored.
TEST_F(BatteryCommandClassTest, ReportV3StoresBatteryLevelPrefix)
{
    set_peer_version(BATTERY_VERSION_3);
    // Properties 2 bit 1 is Low Temperature Status. CC:0080.03.03.11.001 defines that field as 1 when charging stopped.
    const std::array<uint8_t, 5> frame {BATTERY_CC_ID, BATTERY_REPORT_COMMAND, TEST_LEVEL, TEST_PROPERTIES_1, LOW_TEMPERATURE_BIT};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_battery_core::report_received_args args {endpoint, parser, nullptr};

    ASSERT_EQ(SL_STATUS_OK, component().on_battery_report_received(args));

    const auto group = endpoint.child_by_type(REPORT_GROUP);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(TEST_LEVEL, group.child_by_type(BATTERY_LEVEL).reported<uint8_t>());
}

// Spec 2026B Battery CC v2, Battery Health Report (CC:0080.02.05.11.002).
// A Health Report with an in-range Maximum Capacity completes the outstanding Health Get.
TEST_F(BatteryCommandClassTest, HealthReportV2CompletesHealthGet)
{
    set_peer_version(BATTERY_VERSION_2);
    auto pending_group = endpoint.emplace_node(HEALTH_GET_GROUP);
    pending_group.set_desired(HEALTH_GET_PENDING);
    // Capacity, then size 2 / Celsius / precision 0, then temperature 0x0019 (25).
    const std::array<uint8_t, 6> frame {BATTERY_CC_ID, BATTERY_HEALTH_REPORT_COMMAND, TEST_MAXIMUM_CAPACITY, TEST_HEALTH_PROPERTIES, TEST_TEMPERATURE_MSB, TEST_TEMPERATURE_LSB};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_battery_core::report_received_args args {endpoint, parser, nullptr};

    ASSERT_EQ(SL_STATUS_OK, component().on_battery_health_report_received(args));

    EXPECT_EQ(HEALTH_GET_DONE, pending_group.reported<uint8_t>());
}

// Spec 2026B Battery CC v1, Battery Get (CC:0000.00.00.11.002).
// A version 1 peer receives Command Class and BATTERY_GET, with no command data.
TEST_F(BatteryCommandClassTest, GetV1PeerKeepsTwoByteFrame)
{
    set_peer_version(BATTERY_VERSION_1);
    auto group = endpoint.emplace_node(GET_GROUP);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    ASSERT_EQ(SL_STATUS_OK, assemble_get(group, BATTERY_GET_COMMAND, frame, &frame_length));

    const std::vector<uint8_t> expected {BATTERY_CC_ID, BATTERY_GET_COMMAND};
    EXPECT_EQ(expected, std::vector<uint8_t>(frame, frame + frame_length));
}

// Spec 2026B Battery CC v2, Battery Get (CC:0000.00.00.11.002).
// Version 2 does not add bytes to Battery Get, so a version 2 peer receives the same two-byte frame.
TEST_F(BatteryCommandClassTest, GetV2PeerKeepsTwoByteFrame)
{
    set_peer_version(BATTERY_VERSION_2);
    auto group = endpoint.emplace_node(GET_GROUP);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    ASSERT_EQ(SL_STATUS_OK, assemble_get(group, BATTERY_GET_COMMAND, frame, &frame_length));

    const std::vector<uint8_t> expected {BATTERY_CC_ID, BATTERY_GET_COMMAND};
    EXPECT_EQ(expected, std::vector<uint8_t>(frame, frame + frame_length));
}

// Spec 2026B Battery CC v3, Battery Get (CC:0080.03.00.21.001, CC:0000.00.00.11.002).
// Version 3 leaves Battery Get unchanged, so a version 3 peer receives the same two-byte frame.
TEST_F(BatteryCommandClassTest, GetV3PeerKeepsTwoByteFrame)
{
    set_peer_version(BATTERY_VERSION_3);
    auto group = endpoint.emplace_node(GET_GROUP);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    ASSERT_EQ(SL_STATUS_OK, assemble_get(group, BATTERY_GET_COMMAND, frame, &frame_length));

    const std::vector<uint8_t> expected {BATTERY_CC_ID, BATTERY_GET_COMMAND};
    EXPECT_EQ(expected, std::vector<uint8_t>(frame, frame + frame_length));
}

// Spec 2026B Battery CC v2, Battery Health Get (CC:0000.00.00.11.002).
// Health Get exists in version 2 and has no command data. A version 2 peer receives the two-byte command.
TEST_F(BatteryCommandClassTest, HealthGetV2PeerKeepsTwoByteFrame)
{
    set_peer_version(BATTERY_VERSION_2);
    auto group = endpoint.emplace_node(HEALTH_GET_GROUP);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    ASSERT_EQ(SL_STATUS_OK, assemble_get(group, BATTERY_HEALTH_GET_COMMAND, frame, &frame_length));

    const std::vector<uint8_t> expected {BATTERY_CC_ID, BATTERY_HEALTH_GET_COMMAND};
    EXPECT_EQ(expected, std::vector<uint8_t>(frame, frame + frame_length));
}

// Spec 2026B Battery CC v3, Battery Health Get (CC:0080.03.00.21.001, CC:0000.00.00.11.002).
// Version 3 does not change Health Get, so a version 3 peer receives the same two-byte command.
TEST_F(BatteryCommandClassTest, HealthGetV3PeerKeepsTwoByteFrame)
{
    set_peer_version(BATTERY_VERSION_3);
    auto group = endpoint.emplace_node(HEALTH_GET_GROUP);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    ASSERT_EQ(SL_STATUS_OK, assemble_get(group, BATTERY_HEALTH_GET_COMMAND, frame, &frame_length));

    const std::vector<uint8_t> expected {BATTERY_CC_ID, BATTERY_HEALTH_GET_COMMAND};
    EXPECT_EQ(expected, std::vector<uint8_t>(frame, frame + frame_length));
}

// Spec 2026B Battery CC interview (CL:0080.01.21.01.1).
// The mandatory interview is Battery Get and its Report. Version 1 has no Health Get command.
TEST_F(BatteryCommandClassTest, InterviewVersionOneRequestsBatteryGet)
{
    component().on_interview(endpoint, BATTERY_VERSION_1);

    EXPECT_TRUE(endpoint.child_by_type(GET_GROUP).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(REPORT_GROUP).child_by_type(BATTERY_LEVEL).is_valid());
    EXPECT_FALSE(endpoint.child_by_type(HEALTH_GET_GROUP).is_valid());
}

// Spec 2026B Battery CC interview (CL:0080.01.21.01.1).
// The interview figure is still Battery Get. Version 2 does not add Health Get to that interview.
TEST_F(BatteryCommandClassTest, InterviewVersionTwoRequestsBatteryGet)
{
    component().on_interview(endpoint, BATTERY_VERSION_2);

    EXPECT_TRUE(endpoint.child_by_type(GET_GROUP).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(REPORT_GROUP).child_by_type(BATTERY_LEVEL).is_valid());
    EXPECT_FALSE(endpoint.child_by_type(HEALTH_GET_GROUP).is_valid());
}

// Spec 2026B Battery CC v3 interview (CC:0080.03.00.21.001, CL:0080.01.21.01.1).
// Version 3 leaves the interview command unchanged, so interview still requests Battery Get only.
TEST_F(BatteryCommandClassTest, InterviewVersionThreeRequestsBatteryGet)
{
    component().on_interview(endpoint, BATTERY_VERSION_3);

    EXPECT_TRUE(endpoint.child_by_type(GET_GROUP).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(REPORT_GROUP).child_by_type(BATTERY_LEVEL).is_valid());
    EXPECT_FALSE(endpoint.child_by_type(HEALTH_GET_GROUP).is_valid());
}

// The registered BatteryGet name creates the Battery Get resolution group.
// The specification does not define this MQTT command name.
TEST_F(BatteryCommandClassTest, RegisteredBatteryGetDispatches)
{
    mqtt().mqtt_command_handler(endpoint, "BatteryGet", "{}");

    EXPECT_TRUE(endpoint.child_by_type(GET_GROUP).is_valid());
    EXPECT_FALSE(endpoint.child_by_type(HEALTH_GET_GROUP).is_valid());
}

// The registered BatteryHealthGet name creates the Health Get resolution group.
// The specification does not define this MQTT command name.
TEST_F(BatteryCommandClassTest, RegisteredBatteryHealthGetDispatches)
{
    mqtt().mqtt_command_handler(endpoint, "BatteryHealthGet", "{}");

    EXPECT_TRUE(endpoint.child_by_type(HEALTH_GET_GROUP).is_valid());
    EXPECT_FALSE(endpoint.child_by_type(GET_GROUP).is_valid());
}
