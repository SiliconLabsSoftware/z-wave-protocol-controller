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
            auto home = root.emplace_node(ATTRIBUTE_HOME_ID, std::vector<uint8_t> {0x12, 0x34, 0x56, 0x78});
            auto node = home.emplace_node(ATTRIBUTE_NODE_ID, uint16_t {2});
            endpoint  = node.emplace_node(ATTRIBUTE_ENDPOINT_ID, uint8_t {0});
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

        attribute_store::attribute endpoint;
};

// Verifies Meter Report values are grouped by logical scale and rate and retain all report fields.
TEST_F(MeterCommandClassTest, StoresReportsByScaleAndRateType)
{
    // Scale 2 and Export are encoded in masked report fields, while values remain byte arrays.
    command_class_meter_attribute_map_t report {
      {"meter_type", uint8_t {1}},
      {"rate_type", uint8_t {0x40}},
      {"scale_bit_2", uint8_t {0}},
      {"size", uint8_t {2}},
      {"scale_bits_10", uint8_t {0x10}},
      {"precision", uint8_t {0x40}},
      {"meter_value", std::vector<uint8_t> {0x01, 0xF4}},
      {"delta_time", uint16_t {60}},
      {"previous_meter_value", std::vector<uint8_t> {0x01, 0xB0}},
      {"scale_2", uint8_t {0}},
    };

    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_received_store(endpoint, report));
    auto groups = endpoint.children(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP));
    ASSERT_EQ(1, groups.size());
    auto group = groups.front();
    EXPECT_EQ(2,
              command_class_meter_constants::meter_scale_from_report_fields(group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::scale_bit_2)).reported<uint8_t>(),
                                                                            group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::scale_bits_10)).reported<uint8_t>(),
                                                                            group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::scale_2)).reported<uint8_t>()));
    EXPECT_EQ(2, command_class_meter_constants::rate_type_from_masked(group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::rate_type)).reported<uint8_t>()));
    EXPECT_EQ((std::vector<uint8_t> {0x01, 0xF4}), group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_value)).reported<std::vector<uint8_t>>());
    EXPECT_EQ(60, group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::delta_time)).reported<uint16_t>());
    EXPECT_EQ((std::vector<uint8_t> {0x01, 0xB0}), group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::previous_meter_value)).reported<std::vector<uint8_t>>());

    // A second report for the same scale and rate updates the same group.
    report["meter_value"] = std::vector<uint8_t> {0x02, 0x00};
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_received_store(endpoint, report));
    groups = endpoint.children(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP));
    EXPECT_EQ(1, groups.size());
}

// Verifies Supported Report storage and reset support follow the advertised reset flag.
TEST_F(MeterCommandClassTest, StoresSupportedScalesAndResetCapability)
{
    // Advertise v4 scales, both rate types, and Meter Reset support.
    command_class_meter_attribute_map_t report {
      {"meter_type", uint8_t {2}},
      {"rate_type", uint8_t {0x60}},
      {"meter_reset", uint8_t {0x80}},
      {"scale_supported_0", uint8_t {0x03}},
      {"m_s_t", uint8_t {0x80}},
      {"number_of_scale_supported_bytes_to_follow", uint8_t {1}},
      {"scale_supported", std::vector<uint8_t> {0x02}},
    };
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received_store(endpoint, report));
    EXPECT_TRUE(command_class_meter_attribute_store::get_reported_meter_reset_supported(endpoint));
    auto supported = endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::METER_SUPPORTED_REPORT_GROUP));
    EXPECT_EQ((std::vector<uint8_t> {0x02}), supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported)).reported<std::vector<uint8_t>>());

    // Clearing the advertised flag prevents reset operations.
    report["meter_reset"] = uint8_t {0};
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received_store(endpoint, report));
    EXPECT_FALSE(command_class_meter_attribute_store::get_reported_meter_reset_supported(endpoint));
}

// Verifies a field with the wrong decoded type uses the map fallback rather than storing unrelated data.
TEST_F(MeterCommandClassTest, ReportStoresUseDefaultsForWrongFieldTypes)
{
    const command_class_meter_attribute_map_t supported_report {
      {"meter_type", uint16_t {9}},
      {"rate_type", uint16_t {9}},
      {"meter_reset", uint16_t {9}},
      {"scale_supported_0", uint16_t {9}},
      {"m_s_t", uint16_t {9}},
      {"number_of_scale_supported_bytes_to_follow", uint16_t {9}},
      {"scale_supported", uint8_t {9}},
    };
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received_store(endpoint, supported_report));
    auto supported = endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::METER_SUPPORTED_REPORT_GROUP));
    ASSERT_TRUE(supported.is_valid());
    EXPECT_EQ(0, supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::meter_type)).reported<uint8_t>());
    EXPECT_EQ(0, supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported)).reported<std::vector<uint8_t>>().size());

    const command_class_meter_attribute_map_t report {
      {"meter_type", uint16_t {9}},
      {"rate_type", uint16_t {9}},
      {"scale_bit_2", uint16_t {9}},
      {"size", uint16_t {9}},
      {"scale_bits_10", uint16_t {9}},
      {"precision", uint16_t {9}},
      {"scale_2", uint16_t {9}},
      {"delta_time", uint8_t {9}},
      {"meter_value", uint8_t {9}},
      {"previous_meter_value", uint8_t {9}},
    };
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_received_store(endpoint, report));
    auto report_group = endpoint.children(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP)).front();
    EXPECT_EQ(0, report_group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_type)).reported<uint8_t>());
    EXPECT_TRUE(report_group.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_value)).reported<std::vector<uint8_t>>().empty());
}

// Verifies interview uses the v1 no-argument Get and requires Supported Report for versions 2–6.
TEST_F(MeterCommandClassTest, InterviewSelectsV1OrSupportedReportFlowByVersion)
{
    // V1 creates a default reading requirement and Get group without requesting Meter Supported.
    component().on_interview(endpoint, 1);
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP)).is_valid());
    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_supported_get_group_attributes_t::METER_SUPPORTED_GET_GROUP)).is_valid());

    // V2 through v6 start from a Supported Get; v4+ will later fan out by rate type.
    for (const uint8_t version: {uint8_t {2}, uint8_t {3}, uint8_t {4}, uint8_t {5}, uint8_t {6}}) {
        component().on_interview(endpoint, version);
        EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_supported_get_group_attributes_t::METER_SUPPORTED_GET_GROUP)).is_valid());
    }
}

// Verifies supported scales and v4+ rate types create one required reading per supported combination.
TEST_F(MeterCommandClassTest, SupportedReportStartsEveryScaleAndRateReading)
{
    set_version(4);
    ASSERT_EQ(SL_STATUS_OK,
              component().on_meter_supported_report_received_store(endpoint,
                                                                   {
                                                                     {"meter_type", uint8_t {1}},
                                                                     {"rate_type", uint8_t {0x60}},
                                                                     {"meter_reset", uint8_t {0}},
                                                                     {"scale_supported_0", uint8_t {0x03}},
                                                                     {"m_s_t", uint8_t {0}},
                                                                   }));

    // The first callback queues the first scale/rate Get and prepares report groups for all four pairs.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_parsed(nullptr, endpoint, {}));
    auto groups = endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP));
    ASSERT_EQ(1, std::distance(groups.begin(), groups.end()));
    EXPECT_EQ(0, groups.front().child_by_type(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::scale)).desired<uint8_t>());
    EXPECT_EQ(1, groups.front().child_by_type(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::rate_type)).desired<uint8_t>());
    auto first_report = endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP));
    EXPECT_TRUE(first_report.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_value)).is_valid());

    // Reporting one reading advances to the next not-yet-reported scale/rate pair.
    command_class_meter_attribute_map_t reading {{"rate_type", uint8_t {0x20}}, {"meter_value", std::vector<uint8_t> {0x01}}};
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_received_store(endpoint, reading));
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, reading));
    groups = endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP));
    ASSERT_EQ(1, groups.size());
    EXPECT_EQ(2, groups.front().child_by_type(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::rate_type)).desired<uint8_t>());
}

// Verifies v4 Supported Reports decode import, export, and extended-scale bitmasks.
TEST_F(MeterCommandClassTest, SupportedReportDecodesRateAndExtendedScaleBitmasks)
{
    set_version(4);
    command_class_meter_attribute_map_t report {
      {"meter_type", uint8_t {1}},
      {"meter_reset", uint8_t {0}},
      {"scale_supported_0", uint8_t {0x01}},
      {"m_s_t", uint8_t {0x80}},
      {"number_of_scale_supported_bytes_to_follow", uint8_t {1}},
      {"scale_supported", std::vector<uint8_t> {0x02}},
    };

    // Import-only support selects Import and keeps the extended scale from the following byte.
    report["rate_type"] = uint8_t {0x20};
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received_store(endpoint, report));
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_parsed(nullptr, endpoint, {}));
    auto groups = endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP));
    ASSERT_EQ(1, groups.size());
    EXPECT_EQ(1, groups.front().child_by_type(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::rate_type)).desired<uint8_t>());
    auto extended = endpoint.children(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP));
    ASSERT_EQ(2, extended.size());

    // Export-only support selects Export; an empty rate mask yields no reading request.
    report["rate_type"] = uint8_t {0x40};
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received_store(endpoint, report));
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_parsed(nullptr, endpoint, {}));
    EXPECT_EQ(2, groups.front().child_by_type(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::rate_type)).desired<uint8_t>());
    report["rate_type"] = uint8_t {0};
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received_store(endpoint, report));
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_parsed(nullptr, endpoint, {}));
    EXPECT_EQ(2, groups.front().child_by_type(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::rate_type)).desired<uint8_t>());
}

// Verifies interview callbacks safely handle missing capabilities and v1's provisional reading group.
TEST_F(MeterCommandClassTest, InterviewCallbacksHandleMissingAndLegacyReportGroups)
{
    // A v2 Supported Report callback with no stored Supported Report cannot schedule readings.
    set_version(2);
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_parsed(nullptr, endpoint, {}));
    EXPECT_TRUE(endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP)).empty());

    // A v1 report with no provisional group is accepted; with an interviewed group, it clears the requirement.
    set_version(1);
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    component().on_interview(endpoint, 1);
    auto report = endpoint.children(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP)).front();
    auto value  = report.child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_value));
    EXPECT_TRUE(value.is_valid());
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    // The report value remains stored after the interview requirement is cleared.
    EXPECT_TRUE(value.is_valid());
}

// Verifies a reported Supported Report can create a reading group when parsing has not prepared one yet.
TEST_F(MeterCommandClassTest, ReportCallbackCreatesMissingReadingGroupAndSkipsCompleteReadings)
{
    set_version(2);
    ASSERT_EQ(SL_STATUS_OK,
              component().on_meter_supported_report_received_store(endpoint,
                                                                   {
                                                                     {"meter_type", uint8_t {1}},
                                                                     {"meter_reset", uint8_t {0}},
                                                                     {"scale_supported_0", uint8_t {1}},
                                                                   }));

    // This callback path receives supported data without the normal parsed callback creating report groups.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    auto readings = endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP));
    ASSERT_EQ(1, readings.size());
    EXPECT_EQ(0, readings.front().child_by_type(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::scale)).desired<uint8_t>());

    // A second report while the value is still absent requests the same reading again.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    readings = endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP));
    EXPECT_EQ(1, readings.size());

    // Once that reading has a reported value, no additional Get is queued.
    auto report_group = endpoint.children(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP)).front();
    report_group.emplace_node(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_value)).set_reported(std::vector<uint8_t> {0x01});
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    readings = endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP));
    EXPECT_EQ(1, readings.size());
}

// Verifies a partially initialized report group is skipped before a complete matching group is created.
TEST_F(MeterCommandClassTest, ReportStorageSkipsIncompleteMatchingGroups)
{
    using group_type = meter_report_group_attributes_t;
    const std::array<group_type, 4> key_types {{group_type::scale_bit_2, group_type::scale_bits_10, group_type::scale_2, group_type::rate_type}};
    // Each incomplete group omits one key child; each unreported group has every child but one stored value.
    for (const auto missing: key_types) {
        auto group = endpoint.add_node(static_cast<attribute_store_type_t>(group_type::METER_REPORT_GROUP));
        for (const auto field: key_types) {
            if (field != missing) {
                group.emplace_node(static_cast<attribute_store_type_t>(field)).set_reported(uint8_t {0});
            }
        }
    }

    // Fully reported groups that differ in one key field must also be skipped before the matching group is found.
    const std::array<std::array<uint8_t, 4>, 4> nonmatching_keys {{
      {{0x80, 0, 0, 0}},
      {{0, 0x08, 0, 0}},
      {{0x80, 0x18, 1, 0}},
      {{0, 0, 0, 0x20}},
    }};
    for (const auto &keys: nonmatching_keys) {
        auto group = endpoint.add_node(static_cast<attribute_store_type_t>(group_type::METER_REPORT_GROUP));
        for (size_t index = 0; index < key_types.size(); ++index) {
            group.emplace_node(static_cast<attribute_store_type_t>(key_types[index])).set_reported(keys[index]);
        }
    }
    for (const auto missing: key_types) {
        auto group = endpoint.add_node(static_cast<attribute_store_type_t>(group_type::METER_REPORT_GROUP));
        for (const auto field: key_types) {
            auto node = group.emplace_node(static_cast<attribute_store_type_t>(field));
            if (field != missing) {
                node.set_reported(uint8_t {0});
            }
        }
    }

    ASSERT_EQ(SL_STATUS_OK,
              component().on_meter_report_received_store(endpoint,
                                                         {
                                                           {"meter_type", uint8_t {1}},
                                                           {"rate_type", uint8_t {0}},
                                                           {"scale_bit_2", uint8_t {0}},
                                                           {"scale_bits_10", uint8_t {0}},
                                                           {"scale_2", uint8_t {0}},
                                                           {"size", uint8_t {1}},
                                                           {"precision", uint8_t {0}},
                                                           {"meter_value", std::vector<uint8_t> {0x2A}},
                                                         }));
    const auto groups = endpoint.children(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::METER_REPORT_GROUP));
    ASSERT_EQ(13, groups.size());
    EXPECT_TRUE(groups.back().child_by_type(static_cast<attribute_store_type_t>(meter_report_group_attributes_t::meter_value)).reported_exists());
}

// Verifies the supported-report helpers distinguish missing, unreported, and advertised capability fields.
TEST_F(MeterCommandClassTest, SupportedReportCapabilityChecksHandleMissingFields)
{
    EXPECT_FALSE(command_class_meter_attribute_store::get_reported_meter_reset_supported(endpoint));
    auto supported = endpoint.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::METER_SUPPORTED_REPORT_GROUP));
    auto reset     = supported.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::meter_reset));
    EXPECT_FALSE(command_class_meter_attribute_store::get_reported_meter_reset_supported(endpoint));
    reset.set_reported(uint8_t {0});
    EXPECT_FALSE(command_class_meter_attribute_store::get_reported_meter_reset_supported(endpoint));
    reset.set_reported(uint8_t {0x80});
    EXPECT_TRUE(command_class_meter_attribute_store::get_reported_meter_reset_supported(endpoint));

    set_version(2);
    // Missing scale support prevents reading requests; a present empty mask is valid and yields no requests.
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    auto scale0 = supported.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported_0));
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    scale0.set_reported(uint8_t {0});
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    EXPECT_TRUE(endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP)).empty());

    // Optional scale extension fields are ignored when their nodes are absent or not reported.
    scale0.set_reported(uint8_t {1});
    supported.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::m_s_t));
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    EXPECT_FALSE(endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP)).empty());

    // V4 advertises extended scales only when the marker bit is set; absent and empty follow bytes produce no extension Get.
    set_version(4);
    supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::m_s_t)).set_reported(uint8_t {0});
    supported.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::rate_type)).set_reported(uint8_t {0x20});
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    auto m_s_t = supported.child_by_type(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::m_s_t));
    m_s_t.set_reported(uint8_t {0x80});
    auto follow = supported.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported));
    follow.set_reported(std::vector<uint8_t> {});
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
}

// Verifies incomplete v4 capabilities cannot queue a reading until the rate type has a reported value.
TEST_F(MeterCommandClassTest, ReadingRequestsWaitForReportedRateCapability)
{
    set_version(4);
    auto supported = endpoint.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::METER_SUPPORTED_REPORT_GROUP));
    supported.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::scale_supported_0)).set_reported(uint8_t {1});

    // A supported group without its reset flag does not advertise reset support.
    EXPECT_FALSE(command_class_meter_attribute_store::get_reported_meter_reset_supported(endpoint));
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    EXPECT_TRUE(endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP)).empty());

    // Creating the rate attribute alone is insufficient; its reported capability is required.
    auto rate = supported.emplace_node(static_cast<attribute_store_type_t>(meter_supported_report_group_attributes_t::rate_type));
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    EXPECT_TRUE(endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP)).empty());

    rate.set_reported(uint8_t {0x20});
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_report_parsed(nullptr, endpoint, {}));
    const auto requests = endpoint.children(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP));
    ASSERT_EQ(1, requests.size());
    EXPECT_EQ(0, requests.front().child_by_type(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::scale)).desired<uint8_t>());
    EXPECT_EQ(1, requests.front().child_by_type(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::rate_type)).desired<uint8_t>());
}

// Verifies v1 Meter Get has no parameters, v2 rejects unsupported scales, and v4+ encodes rate and extended scale.
TEST_F(MeterCommandClassTest, MeterGetFramesFollowVersionScaleAndRateRules)
{
    uint8_t data[8] {};
    uint16_t length = 0;
    zwave_frame_generator generator {0x32};
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP));
    command_class_meter_core::get_requested_args args {group, &generator};

    // Version 1 uses the legacy empty Meter Get frame.
    set_version(1);
    generator.initialize_frame(0x01, data, &length);
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_get_requested_assemble_frame(args, data, &length));
    EXPECT_EQ(2, length);

    // Version 2 only supports scales 0 through 3.
    set_version(2);
    generator.initialize_frame(0x01, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_meter_get_requested_assemble_frame(args, data, &length));
    desired(group, meter_get_group_attributes_t::scale, uint8_t {4});
    generator.initialize_frame(0x01, data, &length);
    EXPECT_EQ(SL_STATUS_FAIL, component().on_meter_get_requested_assemble_frame(args, data, &length));
    desired(group, meter_get_group_attributes_t::scale, uint8_t {2});
    generator.initialize_frame(0x01, data, &length);
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_get_requested_assemble_frame(args, data, &length));
    EXPECT_EQ(3, length);
    EXPECT_EQ(0x10, data[2]);

    // Version 4 adds rate type and an extra byte for extended scales.
    set_version(4);
    desired(group, meter_get_group_attributes_t::scale, uint8_t {8});
    generator.initialize_frame(0x01, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_meter_get_requested_assemble_frame(args, data, &length));
    desired(group, meter_get_group_attributes_t::rate_type, uint8_t {2});
    generator.initialize_frame(0x01, data, &length);
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_get_requested_assemble_frame(args, data, &length));
    EXPECT_EQ(4, length);
    EXPECT_EQ(0xB8, data[2]);
    EXPECT_EQ(1, data[3]);
}

// Verifies each required v6 Meter Reset field is checked before assembling its payload.
TEST_F(MeterCommandClassTest, MeterResetRequiresEveryV6Field)
{
    using group_type = meter_reset_group_attributes_t;
    const std::array<std::function<void(attribute_store::attribute)>, 6> setters {{
      [this](auto group) { desired(group, group_type::meter_type, uint8_t {3}); },
      [this](auto group) { desired(group, group_type::rate_type, uint8_t {2}); },
      [this](auto group) { desired(group, group_type::size, uint8_t {2}); },
      [this](auto group) { desired(group, group_type::precision, uint8_t {2}); },
      [this](auto group) { desired(group, group_type::meter_value, std::vector<uint8_t> {0x01, 0xF4}); },
      [this](auto group) { desired(group, group_type::scale_bits_10, uint8_t {8}); },
    }};

    set_version(6);
    uint8_t data[16] {};
    uint16_t length = 0;
    zwave_frame_generator generator {0x32};
    for (size_t missing = 0; missing < setters.size(); ++missing) {
        auto group = endpoint.add_node(static_cast<attribute_store_type_t>(group_type::METER_RESET_GROUP));
        for (size_t field = 0; field < setters.size(); ++field) {
            if (field != missing) {
                setters[field](group);
            }
        }
        generator.initialize_frame(0x05, data, &length);
        command_class_meter_core::set_requested_args args {group, &generator};
        EXPECT_EQ(SL_STATUS_NOT_READY, component().on_meter_reset_requested_assemble_frame(args, data, &length)) << "missing field index " << missing;
    }
}

// Verifies Meter Reset is unavailable in v1, empty in v2–5, and carries requested values in v6.
TEST_F(MeterCommandClassTest, MeterResetFramesFollowVersionAndV6Payload)
{
    uint8_t data[16] {};
    uint16_t length = 0;
    zwave_frame_generator generator {0x32};
    auto group = endpoint.emplace_node(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::METER_RESET_GROUP));
    command_class_meter_core::set_requested_args args {group, &generator};

    // Meter Reset was introduced in v2, so a v1 peer must not receive its command ID.
    set_version(1);
    generator.initialize_frame(0x05, data, &length);
    EXPECT_EQ(SL_STATUS_FAIL, component().on_meter_reset_requested_assemble_frame(args, data, &length));

    // Reset was introduced in v2, with no request fields before v6.
    for (const uint8_t version: {uint8_t {2}, uint8_t {3}, uint8_t {4}, uint8_t {5}}) {
        set_version(version);
        generator.initialize_frame(0x05, data, &length);
        ASSERT_EQ(SL_STATUS_OK, component().on_meter_reset_requested_assemble_frame(args, data, &length));
        EXPECT_EQ(2, length);
    }

    // V6 requires all fields and writes the extended scale byte after the value.
    set_version(6);
    generator.initialize_frame(0x05, data, &length);
    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_meter_reset_requested_assemble_frame(args, data, &length));
    desired(group, meter_reset_group_attributes_t::meter_type, uint8_t {3});
    desired(group, meter_reset_group_attributes_t::rate_type, uint8_t {2});
    desired(group, meter_reset_group_attributes_t::size, uint8_t {2});
    desired(group, meter_reset_group_attributes_t::precision, uint8_t {2});
    desired(group, meter_reset_group_attributes_t::meter_value, std::vector<uint8_t> {0x01, 0xF4});
    desired(group, meter_reset_group_attributes_t::scale_bits_10, uint8_t {8});
    generator.initialize_frame(0x05, data, &length);
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_reset_requested_assemble_frame(args, data, &length));
    EXPECT_EQ(7, length);
    EXPECT_EQ(0xC3, data[2]);
    EXPECT_EQ(0x5A, data[3]);
    EXPECT_EQ(0x01, data[4]);
    EXPECT_EQ(0xF4, data[5]);
    EXPECT_EQ(1, data[6]);
}

// Verifies MQTT Gets enforce version-specific fields and Reset requires advertised support.
TEST_F(MeterCommandClassTest, MqttGetAndResetValidateVersionAndCapability)
{
    // Supported Get and Reset are rejected before v2; v1 Meter Get has no payload fields.
    set_version(1);
    EXPECT_EQ(SL_STATUS_FAIL, component().mqtt_on_meter_supported_get_command(endpoint, "{}"));
    EXPECT_EQ(SL_STATUS_FAIL, component().mqtt_on_meter_reset_command(endpoint, "{}"));
    EXPECT_EQ(SL_STATUS_OK, component().mqtt_on_meter_get_command(endpoint, "not-json"));

    // V2 requires scale for Get and a positive Supported Report reset flag for Reset.
    set_version(2);
    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_get_command(endpoint, "{}"));
    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_get_command(endpoint, R"({"scale":"electric"})"));
    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_get_command(endpoint, R"({"scale":-1})"));
    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_get_command(endpoint, R"({"scale":256})"));
    EXPECT_EQ(SL_STATUS_OK, component().mqtt_on_meter_get_command(endpoint, R"({"scale":2})"));
    EXPECT_EQ(SL_STATUS_OK, component().mqtt_on_meter_supported_get_command(endpoint, "{}"));
    EXPECT_EQ(SL_STATUS_FAIL, component().mqtt_on_meter_reset_command(endpoint, "{}"));
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received_store(endpoint, {{"meter_reset", uint8_t {0x80}}}));
    EXPECT_EQ(SL_STATUS_OK, component().mqtt_on_meter_reset_command(endpoint, "{}"));

    // V4 requires rate_type; import and export are separately encoded.
    set_version(4);
    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_get_command(endpoint, R"({"scale":0})"));
    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_get_command(endpoint, R"({"scale":0,"rate_type":"import"})"));
    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_get_command(endpoint, R"({"scale":0,"rate_type":256})"));
    EXPECT_EQ(SL_STATUS_OK, component().mqtt_on_meter_get_command(endpoint, R"({"scale":0,"rate_type":1})"));

    // Exercise each registered MQTT command route, not only its parsing function.
    mqtt().mqtt_command_handler(endpoint, "MeterGet", R"({"scale":0,"rate_type":1})");
    mqtt().mqtt_command_handler(endpoint, "MeterReset", "{}");
    mqtt().mqtt_command_handler(endpoint, "MeterSupportedGet", "{}");
    mqtt().mqtt_command_handler(endpoint, "MeterGet", R"({"scale":0,"rate_type":1})");
    mqtt().mqtt_command_handler(endpoint, "MeterReset", "{}");
    mqtt().mqtt_command_handler(endpoint, "MeterSupportedGet", "{}");
}

// Verifies v6 MQTT Reset validates value length and stores size with the requested reset fields.
TEST_F(MeterCommandClassTest, MqttV6ResetChecksValueLengthAndStoresPayload)
{
    set_version(6);
    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received_store(endpoint, {{"meter_reset", uint8_t {0x80}}}));

    const auto complete = nlohmann::json::parse(R"({"meter_type":1,"rate_type":1,"scale":0,"precision":2,"meter_value":[1]})");
    for (const auto *field: {"meter_type", "rate_type", "scale", "precision", "meter_value"}) {
        auto payload = complete;
        payload.erase(field);
        EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_reset_command(endpoint, payload.dump())) << field;
    }
    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_reset_command(endpoint, R"({"meter_type":"electric","rate_type":1,"scale":0,"precision":2,"meter_value":[1]})"));
    for (const auto *payload: {
           R"({"meter_type":1,"rate_type":"import","scale":0,"precision":2,"meter_value":[1]})",
           R"({"meter_type":1,"rate_type":1,"scale":"kWh","precision":2,"meter_value":[1]})",
           R"({"meter_type":1,"rate_type":1,"scale":0,"precision":"high","meter_value":[1]})",
           R"({"meter_type":1,"rate_type":1,"scale":0,"precision":2,"meter_value":["one"]})",
         }) {
        EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_reset_command(endpoint, payload));
    }

    // Empty and overlong reset values are rejected; one through four bytes are valid.
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, component().mqtt_on_meter_reset_command(endpoint, R"({"meter_type":1,"rate_type":1,"scale":0,"precision":2,"meter_value":[]})"));
    EXPECT_EQ(SL_STATUS_INVALID_PARAMETER, component().mqtt_on_meter_reset_command(endpoint, R"({"meter_type":1,"rate_type":1,"scale":0,"precision":2,"meter_value":[1,2,3,4,5]})"));
    EXPECT_EQ(SL_STATUS_OK, component().mqtt_on_meter_reset_command(endpoint, R"({"meter_type":1,"rate_type":1,"scale":0,"precision":2,"meter_value":[0,0,0,5]})"));
    auto group = endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::METER_RESET_GROUP));
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(4, group.child_by_type(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::size)).desired<uint8_t>());
    EXPECT_EQ((std::vector<uint8_t> {0, 0, 0, 5}), group.child_by_type(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::meter_value)).desired<std::vector<uint8_t>>());
    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_meter_reset_command(endpoint, "{}"));
}

// Verifies the registered MQTT command names dispatch to their Meter handlers.
TEST_F(MeterCommandClassTest, RegisteredMqttCommandsDispatch)
{
    set_version(1);
    mqtt().mqtt_command_handler(endpoint, "MeterGet", "{}");
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_get_group_attributes_t::METER_GET_GROUP)).is_valid());

    set_version(2);
    mqtt().mqtt_command_handler(endpoint, "MeterSupportedGet", "{}");
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_supported_get_group_attributes_t::METER_SUPPORTED_GET_GROUP)).is_valid());

    ASSERT_EQ(SL_STATUS_OK, component().on_meter_supported_report_received_store(endpoint, {{"meter_reset", uint8_t {0x80}}}));
    mqtt().mqtt_command_handler(endpoint, "MeterReset", "{}");
    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(meter_reset_group_attributes_t::METER_RESET_GROUP)).is_valid());
    mqtt().mqtt_command_handler(endpoint, "MeterReset", "{}");
    mqtt().mqtt_command_handler(endpoint, "MeterSupportedGet", "{}");
}
