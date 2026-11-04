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
#include "command_class_configuration.hpp"
#include "command_class_configuration_constants.hpp"
#include "command_class_configuration_types.hpp"
#include "zwave_frame_parser.hpp"

using namespace zwave_command_class;

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}
using namespace zwave_command_class::command_class_configuration_types;
using namespace zwave_command_class::command_class_configuration_constants;

class TestableConfigurationMqtt : public command_class_configuration_mqtt
{
    public:
        using zwave_command_class_base::mqtt_command_handler;
};

namespace
{
    constexpr uint8_t CONFIGURATION_VERSION_1      = 1;
    constexpr uint8_t CONFIGURATION_VERSION_2      = 2;
    constexpr uint8_t CONFIGURATION_VERSION_3      = 3;
    constexpr uint8_t CONFIGURATION_VERSION_4      = 4;
    constexpr zwave_node_id_t TEST_NODE_ID         = 2;
    constexpr zwave_endpoint_id_t TEST_ENDPOINT_ID = 0;
    constexpr uint8_t TEST_PARAMETER_1             = 1;
    constexpr uint8_t TEST_PARAMETER_2             = 2;
    constexpr uint8_t TEST_PARAMETER_3             = 3;
    constexpr uint8_t TEST_SIZE_ONE_BYTE           = 1;
    constexpr uint8_t TEST_SIZE_TWO_BYTES          = 2;
    constexpr uint16_t TEST_PARAMETER_OFFSET       = 258;
    constexpr zwave_home_id_t TEST_HOME_ID         = 0;

    const auto PARAMETER_ID              = static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::PARAMETER_ID);
    const auto PARAMETER_SIZE            = static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::size);
    const auto PARAMETER_FORMAT          = static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::format);
    const auto PARAMETER_VALUE           = static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::value);
    const auto DISCOVERY_COMPLETE        = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_complete);
    const auto PROPERTIES_WALK_ACTIVE    = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::properties_walk_active);
    const auto DISCOVERY_REQUESTED       = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_descriptions_requested);
    const auto PARAMETER_SCAN_ACTIVE     = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_active);
    const auto PARAMETER_SCAN_NUMBER     = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_number);
    const auto PARAMETER_SCAN_SIZE_INDEX = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_size_index);
    const auto REPORT_GROUP              = static_cast<attribute_store_type_t>(configuration_report_group_attributes_t::CONFIGURATION_REPORT_GROUP);
    const auto SET_GROUP                 = static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::CONFIGURATION_SET_GROUP);
    const auto BULK_GET_GROUP            = static_cast<attribute_store_type_t>(configuration_bulk_get_group_attributes_t::CONFIGURATION_BULK_GET_GROUP);
    const auto PROPERTIES_GET_GROUP      = static_cast<attribute_store_type_t>(configuration_properties_get_group_attributes_t::CONFIGURATION_PROPERTIES_GET_GROUP);
    const auto GET_GROUP                 = static_cast<attribute_store_type_t>(configuration_get_group_attributes_t::CONFIGURATION_GET_GROUP);
    const auto NAME_GET_GROUP            = static_cast<attribute_store_type_t>(configuration_name_get_group_attributes_t::CONFIGURATION_NAME_GET_GROUP);
    const auto INFO_GET_GROUP            = static_cast<attribute_store_type_t>(configuration_info_get_group_attributes_t::CONFIGURATION_INFO_GET_GROUP);
    const auto DEFAULT_RESET_GROUP       = static_cast<attribute_store_type_t>(configuration_default_reset_group_attributes_t::CONFIGURATION_DEFAULT_RESET_GROUP);
    const auto BULK_SET_GROUP            = static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::CONFIGURATION_BULK_SET_GROUP);
}  // namespace

class ConfigurationCommandClassTest : public ::testing::Test
{
    protected:
        static TestableConfigurationMqtt &mqtt()
        {
            static TestableConfigurationMqtt instance;
            return instance;
        }

        static command_class_configuration &component()
        {
            static command_class_configuration instance;
            return instance;
        }

        static command_class_configuration_attribute_store &store()
        {
            return static_cast<command_class_configuration_attribute_store &>(component());
        }

        sl_status_t call_control_handler(const uint8_t *frame, uint16_t length)
        {
            zwave_controller_connection_info_t connection_info {};
            connection_info.remote.node_id     = TEST_NODE_ID;
            connection_info.remote.endpoint_id = TEST_ENDPOINT_ID;
            return component().control_handler(&connection_info, frame, length);
        }

        void call_default_probe_report(uint8_t parameter_number, int64_t value)
        {
            // A parsed Configuration Report reaches the probe only while its scan is active.
            auto scan_active = endpoint.child_by_type(PARAMETER_SCAN_ACTIVE);
            if (!scan_active.is_valid()) {
                endpoint.emplace_node(PARAMETER_SCAN_ACTIVE).set_reported<uint8_t>(1);
            }
            auto parameter_node = parameter(parameter_number);
            parameter_node.emplace_node(PARAMETER_VALUE).set_reported<int64_t>(value);
            command_class_configuration_attribute_map_t payload {{"parameter_number", parameter_number}, {"size", TEST_SIZE_ONE_BYTE}, {"value", static_cast<int32_t>(value)}};
            ASSERT_EQ(SL_STATUS_OK, call_report_parsed(std::move(payload)));
        }

        void call_start_default_bit_probe()
        {
            endpoint.emplace_node(PARAMETER_SCAN_ACTIVE).set_reported<uint8_t>(1);
            endpoint.emplace_node(PARAMETER_SCAN_NUMBER).set_reported<uint8_t>(0xFF);
            endpoint.emplace_node(PARAMETER_SCAN_SIZE_INDEX).set_reported<uint8_t>(0);
            endpoint.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase)).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::SIZE_PROBE));
            command_class_configuration::advance_parameter_scan(endpoint, true);
            auto phase = endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase));
            if (!phase.is_valid() || !phase.reported_exists() || phase.reported<uint8_t>() != static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS)) {
                endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).clear_reported();
            }
        }

        void call_advance_parameter_scan(bool found)
        {
            command_class_configuration::advance_parameter_scan(endpoint, found);
        }

        void call_request_next_description(uint16_t after_parameter)
        {
            endpoint.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_descriptions_active)).set_reported<uint8_t>(1);
            endpoint.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_phase)).set_reported(static_cast<uint8_t>(discovery_description_phase_t::NAME));
            endpoint.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_parameter)).set_reported(after_parameter);
            component().on_configuration_name_report_parsed(nullptr, endpoint, {{"parameter_number", after_parameter}, {"reports_to_follow", static_cast<uint8_t>(0)}});
        }

        sl_status_t call_properties_report_parsed(command_class_configuration_attribute_map_t payload)
        {
            return component().on_configuration_properties_report_parsed(nullptr, endpoint, std::move(payload));
        }

        sl_status_t call_report_parsed(command_class_configuration_attribute_map_t payload)
        {
            return component().on_configuration_report_parsed(nullptr, endpoint, std::move(payload));
        }

        sl_status_t call_name_report_parsed(command_class_configuration_attribute_map_t payload)
        {
            return component().on_configuration_name_report_parsed(nullptr, endpoint, std::move(payload));
        }

        sl_status_t call_info_report_parsed(command_class_configuration_attribute_map_t payload)
        {
            return component().on_configuration_info_report_parsed(nullptr, endpoint, std::move(payload));
        }

        void SetUp() override
        {
            ASSERT_EQ(SL_STATUS_OK, attribute_store_init());
            auto home = attribute_store::attribute::root().emplace_node(ATTRIBUTE_HOME_ID, TEST_HOME_ID);
            endpoint  = home.emplace_node(ATTRIBUTE_NODE_ID, TEST_NODE_ID).emplace_node(ATTRIBUTE_ENDPOINT_ID, TEST_ENDPOINT_ID);
            (void)component();
            (void)mqtt();
        }

        void TearDown() override
        {
            attribute_store_teardown();
        }

        attribute_store::attribute parameter(uint16_t number)
        {
            return store().emplace_parameter(endpoint, number);
        }

        attribute_store::attribute endpoint;
};

// Spec 2026B Configuration CC v1, signed parameter representation.
// Verifies encoding uses network byte order and decoding sign-extends signed values.
TEST_F(ConfigurationCommandClassTest, EncodesAndDecodesSignedParameterValues)
{
    EXPECT_EQ((std::vector<uint8_t> {0xFF}), command_class_configuration_attribute_store::encode_configuration_value(-1, TEST_SIZE_ONE_BYTE));
    EXPECT_EQ((std::vector<uint8_t> {0x01, 0x02}), command_class_configuration_attribute_store::encode_configuration_value(0x0102, TEST_SIZE_TWO_BYTES));
    EXPECT_EQ(-128, command_class_configuration_attribute_store::decode_configuration_value({0x80}, static_cast<uint8_t>(format::SIGNED_INTEGER)));
    EXPECT_EQ(128, command_class_configuration_attribute_store::decode_configuration_value({0x80}, static_cast<uint8_t>(format::UNSIGNED_INTEGER)));
    EXPECT_EQ(0, command_class_configuration_attribute_store::decode_configuration_value({}, static_cast<uint8_t>(format::SIGNED_INTEGER)));
    EXPECT_EQ(128, command_class_configuration_attribute_store::decode_configuration_value({0x80}, static_cast<uint8_t>(format::ENUMERATED)));
    EXPECT_EQ(128, command_class_configuration_attribute_store::decode_configuration_value({0x80}, static_cast<uint8_t>(format::BIT_FIELD)));
    EXPECT_EQ(-128, command_class_configuration_attribute_store::decode_configuration_value({0x80}, 0xFF));
    EXPECT_EQ(static_cast<int64_t>(0x8000000000000000ULL), command_class_configuration_attribute_store::decode_configuration_value({0x80, 0, 0, 0, 0, 0, 0, 0}, static_cast<uint8_t>(format::SIGNED_INTEGER)));
}

// Spec 2026B Configuration CC v1, Configuration Report.
// Verifies a one-byte signed report stores the parameter size, default format, and decoded value.
TEST_F(ConfigurationCommandClassTest, ReportV1StoresSignedParameterValue)
{
    const command_class_configuration_attribute_map_t report {{"parameter_number", TEST_PARAMETER_1}, {"size", TEST_SIZE_ONE_BYTE}, {"configuration_value", std::vector<uint8_t> {0x80}}};
    ASSERT_EQ(SL_STATUS_OK, store().on_configuration_report_received_store(endpoint, report));

    const auto parameter = endpoint.child_by_type_and_value(PARAMETER_ID, TEST_PARAMETER_1);
    ASSERT_TRUE(parameter.is_valid());
    EXPECT_EQ(TEST_SIZE_ONE_BYTE, parameter.child_by_type(PARAMETER_SIZE).reported<uint8_t>());
    EXPECT_EQ(static_cast<uint8_t>(format::SIGNED_INTEGER), parameter.child_by_type(PARAMETER_FORMAT).reported<uint8_t>());
    EXPECT_EQ(-128, parameter.child_by_type(PARAMETER_VALUE).reported<int64_t>());
}

// Spec 2026B Configuration CC v1, Configuration Report value-size handling.
// Verifies a mismatched value length stays in the report group and does not update parameter state.
TEST_F(ConfigurationCommandClassTest, ReportWithMismatchedSizeDoesNotCreateParameterState)
{
    const command_class_configuration_attribute_map_t report {{"parameter_number", TEST_PARAMETER_1}, {"size", TEST_SIZE_TWO_BYTES}, {"configuration_value", std::vector<uint8_t> {0x01}}};
    ASSERT_EQ(SL_STATUS_OK, store().on_configuration_report_received_store(endpoint, report));

    EXPECT_TRUE(endpoint.child_by_type(REPORT_GROUP).is_valid());
    EXPECT_FALSE(endpoint.child_by_type_and_value(PARAMETER_ID, TEST_PARAMETER_1).is_valid());
}

// Configuration Reports reuse an already-reported format when decoding a parameter update.
TEST_F(ConfigurationCommandClassTest, ReportPreservesReportedUnsignedFormat)
{
    auto known = parameter(TEST_PARAMETER_1);
    known.emplace_node(PARAMETER_FORMAT).set_reported(static_cast<uint8_t>(format::UNSIGNED_INTEGER));
    const command_class_configuration_attribute_map_t report {{"parameter_number", TEST_PARAMETER_1}, {"size", TEST_SIZE_ONE_BYTE}, {"configuration_value", std::vector<uint8_t> {0xFE}}};

    ASSERT_EQ(SL_STATUS_OK, store().on_configuration_report_received_store(endpoint, report));

    EXPECT_EQ(254, known.child_by_type(PARAMETER_VALUE).reported<int64_t>());
    EXPECT_EQ(static_cast<uint8_t>(format::UNSIGNED_INTEGER), known.child_by_type(PARAMETER_FORMAT).reported<uint8_t>());
}

// Spec 2026B Configuration CC v2, Configuration Bulk Report.
// Valid one-byte values create parameter records with signed values and inherited format defaults.
TEST_F(ConfigurationCommandClassTest, BulkReportStoresEachParameterValue)
{
    constexpr auto BULK_REPORT_GROUP    = static_cast<attribute_store_type_t>(configuration_bulk_report_group_attributes_t::CONFIGURATION_BULK_REPORT_GROUP);
    constexpr auto BULK_REPORT_OFFSET   = static_cast<attribute_store_type_t>(configuration_bulk_report_group_attributes_t::parameter_offset);
    constexpr auto BULK_REPORT_COUNT    = static_cast<attribute_store_type_t>(configuration_bulk_report_group_attributes_t::number_of_parameters);
    constexpr auto BULK_REPORT_SIZE     = static_cast<attribute_store_type_t>(configuration_bulk_report_group_attributes_t::size);
    constexpr uint16_t PARAMETER_OFFSET = 258;
    constexpr uint8_t PARAMETER_COUNT   = 2;
    constexpr uint8_t SIZE_ONE_BYTE     = 1;
    configuration_bulk_report_vg_t values;
    values.push_back({.parameter = {0xFE}});
    values.push_back({.parameter = {0x07}});
    const command_class_configuration_attribute_map_t report {
      {"parameter_offset", PARAMETER_OFFSET},
      {"number_of_parameters", PARAMETER_COUNT},
      {"size", SIZE_ONE_BYTE},
      {"vg", values},
    };

    ASSERT_EQ(SL_STATUS_OK, store().on_configuration_bulk_report_received_store(endpoint, report));

    const auto report_group = endpoint.child_by_type(BULK_REPORT_GROUP);
    ASSERT_TRUE(report_group.is_valid());
    EXPECT_EQ(PARAMETER_OFFSET, report_group.child_by_type(BULK_REPORT_OFFSET).reported<uint16_t>());
    EXPECT_EQ(PARAMETER_COUNT, report_group.child_by_type(BULK_REPORT_COUNT).reported<uint8_t>());
    EXPECT_EQ(SIZE_ONE_BYTE, report_group.child_by_type(BULK_REPORT_SIZE).reported<uint8_t>());
    EXPECT_EQ(-2, endpoint.child_by_type_and_value(PARAMETER_ID, PARAMETER_OFFSET).child_by_type(PARAMETER_VALUE).reported<int64_t>());
    EXPECT_EQ(7, endpoint.child_by_type_and_value(PARAMETER_ID, PARAMETER_OFFSET + 1).child_by_type(PARAMETER_VALUE).reported<int64_t>());
}

// CC:0070.02.09.11.001 — Bulk Reports decode values using the parameter's existing reported format.
TEST_F(ConfigurationCommandClassTest, BulkReportPreservesReportedUnsignedFormat)
{
    constexpr uint16_t PARAMETER_OFFSET = 258;
    constexpr uint8_t PARAMETER_COUNT   = 1;
    constexpr uint8_t SIZE_ONE_BYTE     = 1;
    constexpr auto VALUE_FORMAT         = static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::format);
    auto known_parameter                = parameter(PARAMETER_OFFSET);
    known_parameter.emplace_node(VALUE_FORMAT).set_reported(static_cast<uint8_t>(format::UNSIGNED_INTEGER));
    configuration_bulk_report_vg_t values {{.parameter = {0xFE}}};
    const command_class_configuration_attribute_map_t report {{"parameter_offset", PARAMETER_OFFSET}, {"number_of_parameters", PARAMETER_COUNT}, {"size", SIZE_ONE_BYTE}, {"vg", values}};

    ASSERT_EQ(SL_STATUS_OK, store().on_configuration_bulk_report_received_store(endpoint, report));

    EXPECT_EQ(254, known_parameter.child_by_type(PARAMETER_VALUE).reported<int64_t>());
    EXPECT_EQ(static_cast<uint8_t>(format::UNSIGNED_INTEGER), known_parameter.child_by_type(VALUE_FORMAT).reported<uint8_t>());
}

// Spec 2026B Configuration CC v3, Configuration Properties Report (CC:0070.03.0F.11.001).
// Verifies Properties Report metadata decodes signed bounds and normalizes optional flags.
TEST_F(ConfigurationCommandClassTest, PropertiesReportV3StoresSignedMetadata)
{
    const command_class_configuration_attribute_map_t report {
      {"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)},
      {"size", TEST_SIZE_ONE_BYTE},
      {"format", static_cast<uint8_t>(0x00)},
      {"min_value", std::vector<uint8_t> {0x80}},
      {"max_value", std::vector<uint8_t> {0x7F}},
      {"default_value", std::vector<uint8_t> {0x05}},
      {"next_parameter_number", static_cast<uint16_t>(TEST_PARAMETER_2)},
      {"readonly", static_cast<uint8_t>(1)},
      {"altering_capabilities", static_cast<uint8_t>(1)},
      {"advanced", static_cast<uint8_t>(1)},
      {"no_bulk_support", static_cast<uint8_t>(1)},
    };
    ASSERT_EQ(SL_STATUS_OK, store().on_configuration_properties_report_received_store(endpoint, report));

    const auto parameter = endpoint.child_by_type_and_value(PARAMETER_ID, TEST_PARAMETER_1);
    ASSERT_TRUE(parameter.is_valid());
    EXPECT_EQ(-128, parameter.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::min_value)).reported<int64_t>());
    EXPECT_EQ(127, parameter.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::max_value)).reported<int64_t>());
    EXPECT_EQ(5, parameter.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value)).reported<int64_t>());
    EXPECT_EQ(1, parameter.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::read_only)).reported<uint8_t>());
    EXPECT_EQ(1, parameter.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::altering_capabilities)).reported<uint8_t>());
    EXPECT_EQ(1, parameter.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::advanced)).reported<uint8_t>());
    EXPECT_EQ(1, parameter.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::no_bulk_support)).reported<uint8_t>());
}

// Spec 2026B Configuration CC v3, Configuration Properties Report.
// Missing optional flags remain absent instead of becoming false capability reports.
TEST_F(ConfigurationCommandClassTest, PropertiesReportWithoutOptionalFlagsLeavesThemAbsent)
{
    const command_class_configuration_attribute_map_t report {
      {"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)},
      {"size", TEST_SIZE_ONE_BYTE},
      {"format", static_cast<uint8_t>(format::SIGNED_INTEGER)},
      {"min_value", std::vector<uint8_t> {0}},
      {"max_value", std::vector<uint8_t> {127}},
      {"default_value", std::vector<uint8_t> {5}},
    };

    ASSERT_EQ(SL_STATUS_OK, store().on_configuration_properties_report_received_store(endpoint, report));

    const auto stored_parameter = endpoint.child_by_type_and_value(PARAMETER_ID, TEST_PARAMETER_1);
    EXPECT_FALSE(stored_parameter.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::read_only)).reported_exists());
    EXPECT_FALSE(stored_parameter.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::advanced)).reported_exists());
    EXPECT_FALSE(stored_parameter.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::no_bulk_support)).reported_exists());
}

// Spec 2026B Configuration CC v3, Configuration Name Report multipart handling.
// CC:0070.03.0B.11.002 / CC:0070.03.0B.11.003 — Name fragments append until reports-to-follow reaches zero.
TEST_F(ConfigurationCommandClassTest, NameReportAppendsFragments)
{
    const command_class_configuration_attribute_map_t first {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(1)}, {"name", std::vector<uint8_t> {'T', 'e'}}};
    const command_class_configuration_attribute_map_t last {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(0)}, {"name", std::vector<uint8_t> {'s', 't'}}};
    ASSERT_EQ(SL_STATUS_OK, store().on_configuration_name_report_received_store(endpoint, first));
    ASSERT_EQ(SL_STATUS_OK, store().on_configuration_name_report_received_store(endpoint, last));

    const auto parameter = endpoint.child_by_type_and_value(PARAMETER_ID, TEST_PARAMETER_1);
    ASSERT_TRUE(parameter.is_valid());
    EXPECT_EQ("Test", parameter.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::name)).reported<std::string>());
}

// Spec 2026B Configuration CC v3, Configuration Info Report.
// CC:0070.03.0D.11.002 / CC:0070.03.0D.11.003 — Info fragments append until reports-to-follow reaches zero.
TEST_F(ConfigurationCommandClassTest, InfoReportAppendsFragments)
{
    const command_class_configuration_attribute_map_t first {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(1)}, {"info", std::vector<uint8_t> {'O', 'n'}}};
    const command_class_configuration_attribute_map_t last {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(0)}, {"info", std::vector<uint8_t> {' ', 'o', 'f', 'f'}}};
    ASSERT_EQ(SL_STATUS_OK, store().on_configuration_info_report_received_store(endpoint, first));
    auto parameter_node = endpoint.child_by_type_and_value(PARAMETER_ID, TEST_PARAMETER_1);
    EXPECT_EQ("On", parameter_node.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::info)).desired<std::string>());

    ASSERT_EQ(SL_STATUS_OK, store().on_configuration_info_report_received_store(endpoint, last));

    EXPECT_EQ("On off", parameter_node.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::info)).reported<std::string>());
}

// A non-decreasing reports-to-follow count starts a new name sequence instead of appending stale fragments.
TEST_F(ConfigurationCommandClassTest, NameReportRestartsWhenFragmentSequenceDoesNotAdvance)
{
    const command_class_configuration_attribute_map_t first {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(2)}, {"name", std::vector<uint8_t> {'O', 'l', 'd'}}};
    const command_class_configuration_attribute_map_t restarted {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(2)}, {"name", std::vector<uint8_t> {'N', 'e', 'w'}}};

    ASSERT_EQ(SL_STATUS_OK, store().on_configuration_name_report_received_store(endpoint, first));
    ASSERT_EQ(SL_STATUS_OK, store().on_configuration_name_report_received_store(endpoint, restarted));

    EXPECT_EQ("New", parameter(TEST_PARAMETER_1).child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::name)).desired<std::string>());
}

// An Info fragment with a non-decreasing reports-to-follow count starts a fresh sequence.
TEST_F(ConfigurationCommandClassTest, InfoReportRestartsWhenFragmentSequenceDoesNotAdvance)
{
    const command_class_configuration_attribute_map_t first {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(2)}, {"info", std::vector<uint8_t> {'O', 'l', 'd'}}};
    const command_class_configuration_attribute_map_t restarted {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(2)}, {"info", std::vector<uint8_t> {'N', 'e', 'w'}}};

    ASSERT_EQ(SL_STATUS_OK, store().on_configuration_info_report_received_store(endpoint, first));
    ASSERT_EQ(SL_STATUS_OK, store().on_configuration_info_report_received_store(endpoint, restarted));

    EXPECT_EQ("New", parameter(TEST_PARAMETER_1).child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::info)).desired<std::string>());
}

// A continued Name fragment starts a new buffer when its prior partial text is missing.
TEST_F(ConfigurationCommandClassTest, NameReportContinuesWithoutPriorPartialText)
{
    const command_class_configuration_attribute_map_t fragment {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(1)}, {"name", std::vector<uint8_t> {'N', 'e', 'w'}}};
    endpoint.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::name_reports_to_follow)).set_reported<uint8_t>(2);
    parameter(TEST_PARAMETER_1);

    ASSERT_EQ(SL_STATUS_OK, store().on_configuration_name_report_received_store(endpoint, fragment));

    EXPECT_EQ("New", parameter(TEST_PARAMETER_1).child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::name)).desired<std::string>());
}

// A continued Info fragment starts a new buffer when its prior partial text is missing.
TEST_F(ConfigurationCommandClassTest, InfoReportContinuesWithoutPriorPartialText)
{
    const command_class_configuration_attribute_map_t fragment {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(1)}, {"info", std::vector<uint8_t> {'N', 'e', 'w'}}};
    endpoint.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::info_reports_to_follow)).set_reported<uint8_t>(2);
    parameter(TEST_PARAMETER_1);

    ASSERT_EQ(SL_STATUS_OK, store().on_configuration_info_report_received_store(endpoint, fragment));

    EXPECT_EQ("New", parameter(TEST_PARAMETER_1).child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::info)).desired<std::string>());
}

// A zero-size Properties Report stores the report metadata but does not create parameter capabilities.
TEST_F(ConfigurationCommandClassTest, PropertiesReportZeroSizeDoesNotCreateParameter)
{
    const command_class_configuration_attribute_map_t report {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"size", static_cast<uint8_t>(0)}};

    ASSERT_EQ(SL_STATUS_OK, store().on_configuration_properties_report_received_store(endpoint, report));

    EXPECT_TRUE(endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_properties_report_group_attributes_t::CONFIGURATION_PROPERTIES_REPORT_GROUP)).is_valid());
    EXPECT_FALSE(endpoint.child_by_type_and_value(PARAMETER_ID, TEST_PARAMETER_1).is_valid());
}

// Spec 2026B Configuration CC v1. Versions below 3 have no Properties Get, so interview marks discovery complete.
TEST_F(ConfigurationCommandClassTest, InterviewVersionOneCompletesWithoutPropertiesWalk)
{
    component().on_interview(endpoint, CONFIGURATION_VERSION_1);

    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
    EXPECT_EQ(0, endpoint.child_by_type(PROPERTIES_WALK_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(0, endpoint.child_by_type(DISCOVERY_REQUESTED).reported<uint8_t>());
}

// Spec 2026B Configuration CC v2. Version 2 still has no Properties Get, so interview marks discovery complete.
TEST_F(ConfigurationCommandClassTest, InterviewVersionTwoCompletesWithoutPropertiesWalk)
{
    component().on_interview(endpoint, CONFIGURATION_VERSION_2);

    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
    EXPECT_EQ(0, endpoint.child_by_type(PROPERTIES_WALK_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(0, endpoint.child_by_type(DISCOVERY_REQUESTED).reported<uint8_t>());
}

// Spec 2026B Configuration CC v3. Interview starts the Properties walk at parameter 0 and requests descriptions.
TEST_F(ConfigurationCommandClassTest, InterviewVersionThreeStartsPropertiesWalk)
{
    component().on_interview(endpoint, CONFIGURATION_VERSION_3);

    EXPECT_EQ(1, endpoint.child_by_type(PROPERTIES_WALK_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_REQUESTED).reported<uint8_t>());
    EXPECT_FALSE(endpoint.child_by_type(DISCOVERY_COMPLETE).reported_exists());
    EXPECT_TRUE(endpoint.child_by_type(PROPERTIES_GET_GROUP).is_valid());
}

// Spec 2026B Configuration CC v4. Interview uses the same Properties walk as version 3.
TEST_F(ConfigurationCommandClassTest, InterviewVersionFourStartsPropertiesWalk)
{
    component().on_interview(endpoint, CONFIGURATION_VERSION_4);

    EXPECT_EQ(1, endpoint.child_by_type(PROPERTIES_WALK_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_REQUESTED).reported<uint8_t>());
    EXPECT_FALSE(endpoint.child_by_type(DISCOVERY_COMPLETE).reported_exists());
    EXPECT_TRUE(endpoint.child_by_type(PROPERTIES_GET_GROUP).is_valid());
}

// Spec 2026B Configuration CC v3, Properties discovery.
// A new interview clears cached parameter sizes before walking the current supported list.
TEST_F(ConfigurationCommandClassTest, InterviewV3ClearsCachedParameterSizes)
{
    auto cached_parameter = parameter(TEST_PARAMETER_1);
    auto cached_size      = cached_parameter.emplace_node(PARAMETER_SIZE);
    cached_size.set_reported(TEST_SIZE_ONE_BYTE);
    endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_CONFIGURATION)).set_reported(CONFIGURATION_VERSION_3);

    component().on_interview(endpoint, CONFIGURATION_VERSION_3);

    EXPECT_FALSE(cached_size.reported_exists());
    EXPECT_EQ(1, endpoint.child_by_type(PROPERTIES_WALK_ACTIVE).reported<uint8_t>());
}

// A cached parameter without a size child is left intact during the v3 properties interview.
TEST_F(ConfigurationCommandClassTest, InterviewV3SkipsParametersWithoutSize)
{
    parameter(TEST_PARAMETER_1);

    component().on_interview(endpoint, CONFIGURATION_VERSION_3);

    EXPECT_EQ(1, endpoint.child_by_type(PROPERTIES_WALK_ACTIVE).reported<uint8_t>());
    EXPECT_FALSE(endpoint.child_by_type(DISCOVERY_COMPLETE).reported_exists());
}

// A desired-only CC version is not treated as a reported supported version.
TEST_F(ConfigurationCommandClassTest, DefaultProbeUsesReportedVersionOnly)
{
    endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_CONFIGURATION)).set_desired(CONFIGURATION_VERSION_4);
    parameter(TEST_PARAMETER_1).emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    parameter(TEST_PARAMETER_2).emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);

    call_start_default_bit_probe();

    EXPECT_EQ(1, endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported<uint8_t>());
}

// Spec 2026B Configuration CC v1, Configuration Set MQTT validation.
// Verifies missing size is rejected unless a size has been discovered, then desired wire bytes are stored.
TEST_F(ConfigurationCommandClassTest, MqttSetRequiresSizeAndStoresDesiredValue)
{
    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_set_command(endpoint, "{}"));
    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_set_command(endpoint, R"({"parameter_number":1,"value":7})"));
    parameter(TEST_PARAMETER_1).emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    ASSERT_EQ(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_set_command(endpoint, R"({"parameter_number":1,"value":7})"));

    const auto group = endpoint.child_by_type(SET_GROUP);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ((std::vector<uint8_t> {7}), group.child_by_type(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::configuration_value)).desired<std::vector<uint8_t>>());
}

// Spec 2026B Configuration CC v2, Configuration Bulk Get (CC:0070.02.04.11.001).
// Verifies zero-count bulk reads are rejected and a valid request stores its offset and count.
TEST_F(ConfigurationCommandClassTest, MqttBulkGetValidatesCountAndStoresRange)
{
    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_bulk_get_command(endpoint, "not-json"));
    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_bulk_get_command(endpoint, R"({"parameter_offset":258,"number_of_parameters":0})"));
    ASSERT_EQ(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_bulk_get_command(endpoint, R"({"parameter_offset":258,"number_of_parameters":2})"));

    const auto group = endpoint.child_by_type(BULK_GET_GROUP);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(TEST_PARAMETER_OFFSET, group.child_by_type(static_cast<attribute_store_type_t>(configuration_bulk_get_group_attributes_t::parameter_offset)).desired<uint16_t>());
    EXPECT_EQ(2, group.child_by_type(static_cast<attribute_store_type_t>(configuration_bulk_get_group_attributes_t::number_of_parameters)).desired<uint8_t>());
}

// Spec 2026B Configuration CC v3, Properties Get.
// A v3 capable endpoint starts the properties walk with parameter zero.
TEST_F(ConfigurationCommandClassTest, StartPropertiesWalkV3RequestsParameterZero)
{
    endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_CONFIGURATION)).set_reported(CONFIGURATION_VERSION_3);

    command_class_configuration::start_properties_walk(endpoint);

    EXPECT_EQ(1, endpoint.child_by_type(PROPERTIES_WALK_ACTIVE).reported<uint8_t>());
    const auto group = endpoint.child_by_type(PROPERTIES_GET_GROUP);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(0, group.child_by_type(static_cast<attribute_store_type_t>(configuration_properties_get_group_attributes_t::parameter_number)).desired<uint16_t>());
}

// Spec 2026B Configuration CC v1/v2, legacy parameter discovery.
// Properties discovery is marked complete without a Properties Get for versions below 3.
TEST_F(ConfigurationCommandClassTest, StartPropertiesWalkLegacyVersionCompletesWithoutRequest)
{
    endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_CONFIGURATION)).set_reported(CONFIGURATION_VERSION_2);

    command_class_configuration::start_properties_walk(endpoint);

    EXPECT_EQ(0, endpoint.child_by_type(PROPERTIES_WALK_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
    EXPECT_FALSE(endpoint.child_by_type(PROPERTIES_GET_GROUP).is_valid());
}

// Spec 2026B Configuration CC v1, Configuration parameter discovery behavior.
// Starting a scan probes parameter one using the smallest legal width, then requests its report.
TEST_F(ConfigurationCommandClassTest, StartParameterScanCreatesProbeRequests)
{
    command_class_configuration::start_parameter_scan(endpoint);

    EXPECT_EQ(1, endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(PARAMETER_SCAN_NUMBER).reported<uint8_t>());
    EXPECT_EQ(0, endpoint.child_by_type(PARAMETER_SCAN_SIZE_INDEX).reported<uint8_t>());
    EXPECT_TRUE(endpoint.child_by_type(SET_GROUP).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(GET_GROUP).is_valid());
}

// Spec 2026B Configuration CC v1, Configuration parameter discovery behavior.
// A successful size probe advances to the next parameter with a one-byte probe.
TEST_F(ConfigurationCommandClassTest, SuccessfulParameterProbeAdvancesScan)
{
    command_class_configuration::start_parameter_scan(endpoint);

    command_class_configuration::advance_parameter_scan(endpoint, true);

    EXPECT_EQ(2, endpoint.child_by_type(PARAMETER_SCAN_NUMBER).reported<uint8_t>());
    EXPECT_EQ(0, endpoint.child_by_type(PARAMETER_SCAN_SIZE_INDEX).reported<uint8_t>());
}

// A sparse parameter list advances to the lowest valid parameter above the cursor.
TEST_F(ConfigurationCommandClassTest, DescriptionCursorSkipsInvalidAndEarlierParameters)
{
    const auto add_sized_parameter = [this](uint16_t number, uint8_t size) {
        parameter(number).emplace_node(PARAMETER_SIZE).set_reported(size);
    };
    add_sized_parameter(1, TEST_SIZE_ONE_BYTE);
    add_sized_parameter(2, 3);  // Three-byte parameter widths are not valid in this CC.
    add_sized_parameter(3, TEST_SIZE_TWO_BYTES);
    add_sized_parameter(4, TEST_SIZE_ONE_BYTE);
    endpoint.emplace_node(PARAMETER_ID).set_desired<uint16_t>(5);  // An unreported parameter ID is skipped by discovery.
    call_request_next_description(1);

    EXPECT_EQ(static_cast<uint8_t>(discovery_description_phase_t::NAME), endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_phase)).reported<uint8_t>());
    EXPECT_EQ(3, endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_parameter)).reported<uint16_t>());
    EXPECT_EQ(3, endpoint.child_by_type(NAME_GET_GROUP).child_by_type(static_cast<attribute_store_type_t>(configuration_name_get_group_attributes_t::parameter_number)).desired<uint16_t>());
}

// Discovery selects the smallest valid parameter after the cursor even when records are unsorted.
TEST_F(ConfigurationCommandClassTest, DescriptionCursorKeepsSmallestCandidateAndSkipsMissingSize)
{
    auto later = parameter(TEST_PARAMETER_3);
    later.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    parameter(TEST_PARAMETER_2);  // A reported parameter without size metadata is not requestable.
    auto earlier = parameter(TEST_PARAMETER_1);
    earlier.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_TWO_BYTES);

    call_request_next_description(0);

    EXPECT_EQ(TEST_PARAMETER_1, endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_parameter)).reported<uint16_t>());
}

// A candidate with desired-only size metadata is skipped until its size is reported.
TEST_F(ConfigurationCommandClassTest, DescriptionCursorSkipsDesiredOnlySize)
{
    parameter(TEST_PARAMETER_1).emplace_node(PARAMETER_SIZE).set_desired(TEST_SIZE_ONE_BYTE);

    call_request_next_description(0);

    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
    EXPECT_FALSE(endpoint.child_by_type(NAME_GET_GROUP).is_valid());
}

// Description discovery ignores a parameter whose identifier exists only as desired state.
TEST_F(ConfigurationCommandClassTest, DescriptionCursorSkipsUnreportedParameterId)
{
    constexpr auto DISCOVERY_ACTIVE = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_descriptions_active);
    endpoint.emplace_node(PARAMETER_ID).set_desired<uint16_t>(TEST_PARAMETER_1);

    call_request_next_description(0);

    EXPECT_EQ(0, endpoint.child_by_type(DISCOVERY_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
}

// Spec 2026B Configuration CC v1, Configuration parameter discovery behavior.
// A failed size probe advances the probe width while keeping the current parameter number.
TEST_F(ConfigurationCommandClassTest, FailedParameterProbeTriesNextSize)
{
    command_class_configuration::start_parameter_scan(endpoint);

    command_class_configuration::advance_parameter_scan(endpoint, false);

    EXPECT_EQ(1, endpoint.child_by_type(PARAMETER_SCAN_NUMBER).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(PARAMETER_SCAN_SIZE_INDEX).reported<uint8_t>());
}

// An active scan with missing cursor metadata restarts at parameter one and the smallest width.
TEST_F(ConfigurationCommandClassTest, AdvanceScanInitializesMissingCursorMetadata)
{
    endpoint.emplace_node(PARAMETER_SCAN_ACTIVE).set_reported<uint8_t>(1);

    call_advance_parameter_scan(false);

    EXPECT_FALSE(endpoint.child_by_type(PARAMETER_SCAN_NUMBER).reported_exists());
    EXPECT_EQ(1, endpoint.child_by_type(PARAMETER_SCAN_SIZE_INDEX).reported<uint8_t>());
    EXPECT_TRUE(endpoint.child_by_type(SET_GROUP).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(GET_GROUP).is_valid());
}

// Inactive scans and non-size phases do not advance the size-probe cursor.
TEST_F(ConfigurationCommandClassTest, AdvanceScanRequiresAnActiveSizeProbe)
{
    call_advance_parameter_scan(false);
    endpoint.emplace_node(PARAMETER_SCAN_ACTIVE).set_desired<uint8_t>(1);
    call_advance_parameter_scan(false);
    endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase)).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS));

    call_advance_parameter_scan(false);

    EXPECT_FALSE(endpoint.child_by_type(PARAMETER_SCAN_NUMBER).reported_exists());
}

// Completing the last size probe completes discovery when no default-bit probe can start.
TEST_F(ConfigurationCommandClassTest, FinalSizeProbeCompletesDiscoveryWithoutDefaultProbe)
{
    endpoint.emplace_node(PARAMETER_SCAN_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(PARAMETER_SCAN_NUMBER).set_reported<uint8_t>(0xFF);
    endpoint.emplace_node(PARAMETER_SCAN_SIZE_INDEX).set_reported<uint8_t>(2);
    endpoint.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase)).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::SIZE_PROBE));

    call_advance_parameter_scan(false);

    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
}

// A reported inactive scan ignores additional size-probe results.
TEST_F(ConfigurationCommandClassTest, AdvanceScanIgnoresReportedInactiveState)
{
    endpoint.emplace_node(PARAMETER_SCAN_ACTIVE).set_reported<uint8_t>(0);

    call_advance_parameter_scan(true);

    EXPECT_FALSE(endpoint.child_by_type(PARAMETER_SCAN_NUMBER).reported_exists());
}

// A size probe with desired-only phase metadata uses the scan's default SIZE_PROBE phase.
TEST_F(ConfigurationCommandClassTest, AdvanceScanDefaultsUnreportedPhaseToSizeProbe)
{
    endpoint.emplace_node(PARAMETER_SCAN_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase)).set_desired(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS));

    call_advance_parameter_scan(false);

    EXPECT_EQ(1, endpoint.child_by_type(PARAMETER_SCAN_SIZE_INDEX).reported<uint8_t>());
}

// Failure of the largest size probe moves scanning to the next parameter and resets its size index.
TEST_F(ConfigurationCommandClassTest, FailedLargestSizeProbeAdvancesParameter)
{
    constexpr uint8_t LARGEST_SIZE_INDEX = 2;
    command_class_configuration::start_parameter_scan(endpoint);
    endpoint.child_by_type(PARAMETER_SCAN_SIZE_INDEX).set_reported(LARGEST_SIZE_INDEX);

    command_class_configuration::advance_parameter_scan(endpoint, false);

    EXPECT_EQ(2, endpoint.child_by_type(PARAMETER_SCAN_NUMBER).reported<uint8_t>());
    EXPECT_EQ(0, endpoint.child_by_type(PARAMETER_SCAN_SIZE_INDEX).reported<uint8_t>());
}

// Failing the final size probe at the last supported parameter completes the scan.
TEST_F(ConfigurationCommandClassTest, FailedFinalSizeProbeCompletesDiscovery)
{
    constexpr uint8_t LAST_PARAMETER     = 255;
    constexpr uint8_t LARGEST_SIZE_INDEX = 2;
    command_class_configuration::start_parameter_scan(endpoint);
    endpoint.child_by_type(PARAMETER_SCAN_NUMBER).set_reported(LAST_PARAMETER);
    endpoint.child_by_type(PARAMETER_SCAN_SIZE_INDEX).set_reported(LARGEST_SIZE_INDEX);

    call_advance_parameter_scan(false);

    EXPECT_EQ(0, endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
}

// Finishing the v1 scan with no known parameters ends discovery without starting default probing.
TEST_F(ConfigurationCommandClassTest, FinalSizeProbeWithoutParametersCompletesDiscovery)
{
    constexpr uint8_t LAST_PARAMETER     = 255;
    constexpr uint8_t LARGEST_SIZE_INDEX = 2;
    command_class_configuration::start_parameter_scan(endpoint);
    endpoint.child_by_type(PARAMETER_SCAN_NUMBER).set_reported(LAST_PARAMETER);
    endpoint.child_by_type(PARAMETER_SCAN_SIZE_INDEX).set_reported(LARGEST_SIZE_INDEX);

    command_class_configuration::advance_parameter_scan(endpoint, true);

    EXPECT_EQ(0, endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
}

// The optional default-bit probe requires at least two v1 parameters.
TEST_F(ConfigurationCommandClassTest, DefaultBitProbeSkipsSingleKnownParameter)
{
    parameter(TEST_PARAMETER_1).emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);

    call_start_default_bit_probe();

    EXPECT_FALSE(endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported_exists());
}

// The optional default-bit probe skips parameters outside the legacy 1..255 range.
TEST_F(ConfigurationCommandClassTest, DefaultBitProbeSkipsOutOfRangeParameters)
{
    constexpr uint16_t FIRST_OUT_OF_RANGE_PARAMETER  = 300;
    constexpr uint16_t SECOND_OUT_OF_RANGE_PARAMETER = 301;
    parameter(FIRST_OUT_OF_RANGE_PARAMETER).emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    parameter(SECOND_OUT_OF_RANGE_PARAMETER).emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);

    call_start_default_bit_probe();

    EXPECT_FALSE(endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported_exists());
}

// Default-bit probing also rejects an out-of-range second ID and a missing second size report.
TEST_F(ConfigurationCommandClassTest, DefaultBitProbeRequiresBothInRangeReportedSizes)
{
    constexpr uint16_t OUT_OF_RANGE_PARAMETER = 300;
    auto first                                = parameter(TEST_PARAMETER_1);
    first.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    auto second = parameter(OUT_OF_RANGE_PARAMETER);
    second.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);

    call_start_default_bit_probe();
    EXPECT_FALSE(endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported_exists());

    second = parameter(TEST_PARAMETER_2);
    second.emplace_node(PARAMETER_SIZE).set_desired(TEST_SIZE_ONE_BYTE);
    call_start_default_bit_probe();
    EXPECT_FALSE(endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported_exists());
}

// The optional default-bit probe requires a reported size for both known parameters.
TEST_F(ConfigurationCommandClassTest, DefaultBitProbeSkipsUnreportedSizes)
{
    parameter(TEST_PARAMETER_1).emplace_node(PARAMETER_SIZE).set_desired(TEST_SIZE_ONE_BYTE);
    parameter(TEST_PARAMETER_2).emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);

    call_start_default_bit_probe();

    EXPECT_FALSE(endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported_exists());
}

// A desired-only size is insufficient for issuing default-bit probes.
TEST_F(ConfigurationCommandClassTest, DefaultBitProbeSkipsDesiredSize)
{
    auto first = parameter(TEST_PARAMETER_1);
    first.emplace_node(PARAMETER_SIZE).set_desired(TEST_SIZE_ONE_BYTE);
    auto second = parameter(TEST_PARAMETER_2);
    second.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);

    call_start_default_bit_probe();

    EXPECT_FALSE(endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported_exists());
}

// Default-bit probing waits until both parameter sizes have reported values.
TEST_F(ConfigurationCommandClassTest, DefaultBitProbeRequiresReportedSizeForEachParameter)
{
    auto first       = parameter(TEST_PARAMETER_1);
    auto first_size  = first.emplace_node(PARAMETER_SIZE);
    auto second      = parameter(TEST_PARAMETER_2);
    auto second_size = second.emplace_node(PARAMETER_SIZE);

    first_size.set_desired(TEST_SIZE_ONE_BYTE);
    second_size.set_reported(TEST_SIZE_ONE_BYTE);
    call_start_default_bit_probe();
    EXPECT_FALSE(endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported_exists());

    first_size.set_reported(TEST_SIZE_ONE_BYTE);
    second_size.clear_reported();
    call_start_default_bit_probe();
    EXPECT_FALSE(endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported_exists());

    second_size.set_desired(TEST_SIZE_ONE_BYTE);
    call_start_default_bit_probe();
    EXPECT_FALSE(endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported_exists());
}

// A missing first parameter size prevents default-bit probe setup.
TEST_F(ConfigurationCommandClassTest, DefaultBitProbeRequiresFirstSizeNode)
{
    parameter(TEST_PARAMETER_1);
    parameter(TEST_PARAMETER_2).emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);

    call_start_default_bit_probe();

    EXPECT_FALSE(endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported_exists());
}

// Default-bit probing also requires a size node for the second candidate parameter.
TEST_F(ConfigurationCommandClassTest, DefaultBitProbeRequiresSecondSizeNode)
{
    parameter(TEST_PARAMETER_1).emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    parameter(TEST_PARAMETER_2);

    call_start_default_bit_probe();

    EXPECT_FALSE(endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported_exists());
}

// A Configuration Get give-up advances the parameter scan using the same failure path as a missed probe.
TEST_F(ConfigurationCommandClassTest, GetResolutionGiveUpAdvancesParameterScan)
{
    command_class_configuration::start_parameter_scan(endpoint);
    auto group = endpoint.child_by_type(GET_GROUP);

    command_class_configuration::on_configuration_get_resolution_give_up(group);

    EXPECT_EQ(1, endpoint.child_by_type(PARAMETER_SCAN_SIZE_INDEX).reported<uint8_t>());
}

// Spec 2026B Configuration CC v1, Configuration Report during parameter scanning.
// A matching size-probe report advances to the next parameter.
TEST_F(ConfigurationCommandClassTest, MatchingScanReportAdvancesParameter)
{
    endpoint.emplace_node(PARAMETER_SCAN_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(PARAMETER_SCAN_NUMBER).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PARAMETER_SCAN_SIZE_INDEX).set_reported<uint8_t>(0);

    ASSERT_EQ(SL_STATUS_OK, call_report_parsed({{"parameter_number", TEST_PARAMETER_1}, {"size", TEST_SIZE_ONE_BYTE}}));

    EXPECT_EQ(2, endpoint.child_by_type(PARAMETER_SCAN_NUMBER).reported<uint8_t>());
    EXPECT_EQ(0, endpoint.child_by_type(PARAMETER_SCAN_SIZE_INDEX).reported<uint8_t>());
}

// Configuration Reports for a different parameter do not advance the pending scan probe.
TEST_F(ConfigurationCommandClassTest, NonmatchingScanReportLeavesProbePending)
{
    endpoint.emplace_node(PARAMETER_SCAN_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(PARAMETER_SCAN_NUMBER).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PARAMETER_SCAN_SIZE_INDEX).set_reported<uint8_t>(0);

    ASSERT_EQ(SL_STATUS_OK, call_report_parsed({{"parameter_number", TEST_PARAMETER_2}, {"size", TEST_SIZE_ONE_BYTE}}));

    EXPECT_EQ(TEST_PARAMETER_1, endpoint.child_by_type(PARAMETER_SCAN_NUMBER).reported<uint8_t>());
}

// A Configuration Report stops only the pending Get group whose desired parameter matches.
TEST_F(ConfigurationCommandClassTest, ConfigurationReportStopsMatchingGetGroup)
{
    auto missing_parameter_group = endpoint.add_node(GET_GROUP);
    auto mismatch_group          = endpoint.add_node(GET_GROUP);
    mismatch_group.emplace_node(static_cast<attribute_store_type_t>(configuration_get_group_attributes_t::parameter_number)).set_desired(TEST_PARAMETER_2);
    auto matching_group = endpoint.add_node(GET_GROUP);
    matching_group.emplace_node(static_cast<attribute_store_type_t>(configuration_get_group_attributes_t::parameter_number)).set_desired(TEST_PARAMETER_1);

    ASSERT_EQ(SL_STATUS_OK, call_report_parsed({{"parameter_number", TEST_PARAMETER_1}}));

    EXPECT_FALSE(missing_parameter_group.reported_exists());
    EXPECT_FALSE(mismatch_group.reported_exists());
    EXPECT_EQ(0, matching_group.reported<uint8_t>());
}

// A report does not stop a Get group whose parameter number is only reported, not desired.
TEST_F(ConfigurationCommandClassTest, ConfigurationReportDoesNotMatchReportedOnlyGetGroup)
{
    auto group = endpoint.add_node(GET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_get_group_attributes_t::parameter_number)).set_reported(TEST_PARAMETER_1);

    ASSERT_EQ(SL_STATUS_OK, call_report_parsed({{"parameter_number", TEST_PARAMETER_1}}));

    EXPECT_FALSE(group.reported_exists());
}

// Incomplete and stale scan reports must not advance the current size probe.
TEST_F(ConfigurationCommandClassTest, ScanReportIgnoresInactiveAndIncompleteProbeState)
{
    const command_class_configuration_attribute_map_t matching_report {{"parameter_number", TEST_PARAMETER_1}, {"size", TEST_SIZE_ONE_BYTE}};
    ASSERT_EQ(SL_STATUS_OK, call_report_parsed(matching_report));

    endpoint.emplace_node(PARAMETER_SCAN_ACTIVE).set_reported<uint8_t>(1);
    ASSERT_EQ(SL_STATUS_OK, call_report_parsed(matching_report));
    endpoint.emplace_node(PARAMETER_SCAN_NUMBER).set_reported<uint8_t>(TEST_PARAMETER_1);
    ASSERT_EQ(SL_STATUS_OK, call_report_parsed(matching_report));
    endpoint.emplace_node(PARAMETER_SCAN_SIZE_INDEX).set_reported<uint8_t>(99);
    ASSERT_EQ(SL_STATUS_OK, call_report_parsed(matching_report));
    endpoint.child_by_type(PARAMETER_SCAN_SIZE_INDEX).set_reported<uint8_t>(0);
    const command_class_configuration_attribute_map_t wrong_size {{"parameter_number", TEST_PARAMETER_1}, {"size", TEST_SIZE_TWO_BYTES}};
    ASSERT_EQ(SL_STATUS_OK, call_report_parsed(wrong_size));

    EXPECT_EQ(TEST_PARAMETER_1, endpoint.child_by_type(PARAMETER_SCAN_NUMBER).reported<uint8_t>());
    EXPECT_EQ(0, endpoint.child_by_type(PARAMETER_SCAN_SIZE_INDEX).reported<uint8_t>());
}

// A late size Report after the scan has stopped must not advance the saved probe cursor.
TEST_F(ConfigurationCommandClassTest, ScanReportDoesNotAdvanceWhenScanIsReportedInactive)
{
    endpoint.emplace_node(PARAMETER_SCAN_ACTIVE).set_reported<uint8_t>(0);
    endpoint.emplace_node(PARAMETER_SCAN_NUMBER).set_reported<uint8_t>(TEST_PARAMETER_1);
    endpoint.emplace_node(PARAMETER_SCAN_SIZE_INDEX).set_reported<uint8_t>(0);

    ASSERT_EQ(SL_STATUS_OK, call_report_parsed({{"parameter_number", TEST_PARAMETER_1}, {"size", TEST_SIZE_ONE_BYTE}}));

    EXPECT_EQ(TEST_PARAMETER_1, endpoint.child_by_type(PARAMETER_SCAN_NUMBER).reported<uint8_t>());
    EXPECT_EQ(0, endpoint.child_by_type(PARAMETER_SCAN_SIZE_INDEX).reported<uint8_t>());
}

// Desired-only scan-active and scan-phase attributes cannot authorize scan advancement.
TEST_F(ConfigurationCommandClassTest, ScanReportRequiresReportedActiveAndPhase)
{
    const command_class_configuration_attribute_map_t matching_report {{"parameter_number", TEST_PARAMETER_1}, {"size", TEST_SIZE_ONE_BYTE}};
    endpoint.emplace_node(PARAMETER_SCAN_ACTIVE).set_desired<uint8_t>(1);
    endpoint.emplace_node(PARAMETER_SCAN_NUMBER).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PARAMETER_SCAN_SIZE_INDEX).set_reported<uint8_t>(0);
    ASSERT_EQ(SL_STATUS_OK, call_report_parsed(matching_report));
    EXPECT_FALSE(endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported_exists());

    endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase)).set_desired(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS));
    ASSERT_EQ(SL_STATUS_OK, call_report_parsed(matching_report));

    EXPECT_EQ(2, endpoint.child_by_type(PARAMETER_SCAN_NUMBER).reported<uint8_t>());
    EXPECT_EQ(0, endpoint.child_by_type(PARAMETER_SCAN_SIZE_INDEX).reported<uint8_t>());
}

// A report for a default-probe parameter with no stored record leaves probe progress unchanged.
TEST_F(ConfigurationCommandClassTest, DefaultProbeReportIgnoresUnknownParameter)
{
    endpoint.emplace_node(PARAMETER_SCAN_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase)).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS));
    endpoint.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first)).set_reported<uint8_t>(TEST_PARAMETER_1);
    endpoint.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second)).set_reported<uint8_t>(TEST_PARAMETER_2);
    endpoint.emplace_node(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining)).set_reported<uint8_t>(2);

    ASSERT_EQ(SL_STATUS_OK, call_report_parsed({{"parameter_number", TEST_PARAMETER_1}, {"size", TEST_SIZE_ONE_BYTE}}));

    EXPECT_EQ(2, endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining)).reported<uint8_t>());
}

// Default probe reports are ignored until all tracking attributes exist and the report matches a probe ID.
TEST_F(ConfigurationCommandClassTest, DefaultProbeReportRequiresCompleteTrackingState)
{
    constexpr auto SCAN_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase);
    constexpr auto PROBE_FIRST  = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first);
    constexpr auto PROBE_SECOND = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second);
    constexpr auto REMAINING    = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining);
    endpoint.emplace_node(SCAN_PHASE).set_desired(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS));
    call_default_probe_report(TEST_PARAMETER_1, 0);  // Desired-only phase state is not enough to consume a report.
    endpoint.child_by_type(SCAN_PHASE).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS));
    call_default_probe_report(TEST_PARAMETER_1, 0);  // No first probe ID is available yet.
    endpoint.emplace_node(PROBE_FIRST).set_desired(TEST_PARAMETER_1);
    call_default_probe_report(TEST_PARAMETER_1, 0);  // Desired-only probe IDs are ignored.
    endpoint.child_by_type(PROBE_FIRST).set_reported(TEST_PARAMETER_1);
    call_default_probe_report(TEST_PARAMETER_1, 0);  // The second probe ID is still absent.
    endpoint.emplace_node(PROBE_SECOND).set_desired(TEST_PARAMETER_2);
    call_default_probe_report(TEST_PARAMETER_1, 0);  // The second probe ID is not reported yet.
    endpoint.child_by_type(PROBE_SECOND).set_reported(TEST_PARAMETER_2);
    call_default_probe_report(TEST_PARAMETER_1, 0);  // The remaining-report count is absent.
    endpoint.emplace_node(REMAINING).set_desired<uint8_t>(2);
    call_default_probe_report(TEST_PARAMETER_1, 0);  // The count must be reported to advance state.
    endpoint.child_by_type(REMAINING).set_reported<uint8_t>(2);
    call_default_probe_report(TEST_PARAMETER_3, 0);

    EXPECT_EQ(2, endpoint.child_by_type(REMAINING).reported<uint8_t>());
    EXPECT_FALSE(parameter(TEST_PARAMETER_1).child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value)).reported_exists());
}

// A completed defaults phase stops when the first probe parameter has no reported size.
TEST_F(ConfigurationCommandClassTest, DefaultProbeStopsWhenFirstSizeIsMissing)
{
    constexpr auto SCAN_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase);
    constexpr auto PROBE_FIRST  = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first);
    constexpr auto PROBE_SECOND = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second);
    constexpr auto REMAINING    = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining);
    parameter(TEST_PARAMETER_1);
    auto second = parameter(TEST_PARAMETER_2);
    second.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    second.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value)).set_reported(0);
    endpoint.emplace_node(SCAN_PHASE).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS));
    endpoint.emplace_node(PROBE_FIRST).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PROBE_SECOND).set_reported(TEST_PARAMETER_2);
    endpoint.emplace_node(REMAINING).set_reported<uint8_t>(1);

    call_default_probe_report(TEST_PARAMETER_2, 0);

    EXPECT_EQ(0, endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
}

// A completed defaults phase stops when the second probe parameter has no reported size.
TEST_F(ConfigurationCommandClassTest, DefaultProbeStopsWhenSecondSizeIsMissing)
{
    constexpr auto SCAN_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase);
    constexpr auto PROBE_FIRST  = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first);
    constexpr auto PROBE_SECOND = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second);
    constexpr auto REMAINING    = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining);
    auto first                  = parameter(TEST_PARAMETER_1);
    first.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    parameter(TEST_PARAMETER_2);
    endpoint.emplace_node(SCAN_PHASE).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS));
    endpoint.emplace_node(PROBE_FIRST).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PROBE_SECOND).set_reported(TEST_PARAMETER_2);
    endpoint.emplace_node(REMAINING).set_reported<uint8_t>(1);

    call_default_probe_report(TEST_PARAMETER_1, 0);

    EXPECT_EQ(0, endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
}

// Default probing abandons its inference when a known parameter's size loses reported state.
TEST_F(ConfigurationCommandClassTest, DefaultProbeStopsWhenSizeIsOnlyDesired)
{
    constexpr auto SCAN_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase);
    constexpr auto PROBE_FIRST  = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first);
    constexpr auto PROBE_SECOND = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second);
    constexpr auto REMAINING    = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining);
    auto first                  = parameter(TEST_PARAMETER_1);
    first.emplace_node(PARAMETER_SIZE).set_desired(TEST_SIZE_ONE_BYTE);
    auto second = parameter(TEST_PARAMETER_2);
    second.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    second.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value)).set_reported(0);
    endpoint.emplace_node(SCAN_PHASE).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS));
    endpoint.emplace_node(PROBE_FIRST).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PROBE_SECOND).set_reported(TEST_PARAMETER_2);
    endpoint.emplace_node(REMAINING).set_reported<uint8_t>(1);

    call_default_probe_report(TEST_PARAMETER_2, 0);

    EXPECT_EQ(0, endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
}

// Default probing abandons reset inference when either probed value is not reported.
TEST_F(ConfigurationCommandClassTest, PartialResetProbeRequiresBothReportedValues)
{
    constexpr auto SCAN_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase);
    constexpr auto PROBE_FIRST  = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first);
    constexpr auto PROBE_SECOND = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second);
    constexpr auto REMAINING    = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining);
    auto first                  = parameter(TEST_PARAMETER_1);
    auto second                 = parameter(TEST_PARAMETER_2);
    first.emplace_node(PARAMETER_VALUE).set_desired<int64_t>(0);
    second.emplace_node(PARAMETER_VALUE).set_reported<int64_t>(0);
    first.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value)).set_reported<int64_t>(0);
    second.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value)).set_reported<int64_t>(0);
    endpoint.emplace_node(SCAN_PHASE).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_PARTIAL_RESET));
    endpoint.emplace_node(PROBE_FIRST).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PROBE_SECOND).set_reported(TEST_PARAMETER_2);
    endpoint.emplace_node(REMAINING).set_reported<uint8_t>(1);

    call_default_probe_report(TEST_PARAMETER_2, 0);

    EXPECT_EQ(0, endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::default_resets_every_parameter)).reported<uint8_t>());
}

// The partial-reset probe records false when the first cached value is only desired.
TEST_F(ConfigurationCommandClassTest, PartialResetProbeRequiresReportedFirstValue)
{
    constexpr auto SCAN_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase);
    constexpr auto PROBE_FIRST  = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first);
    constexpr auto PROBE_SECOND = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second);
    constexpr auto REMAINING    = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining);
    auto first                  = parameter(TEST_PARAMETER_1);
    auto second                 = parameter(TEST_PARAMETER_2);
    first.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    second.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    first.emplace_node(PARAMETER_VALUE).set_desired<int64_t>(0);
    second.emplace_node(PARAMETER_VALUE).set_reported<int64_t>(0);
    first.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value)).set_reported<int64_t>(0);
    second.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value)).set_reported<int64_t>(0);
    endpoint.emplace_node(SCAN_PHASE).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_PARTIAL_RESET));
    endpoint.emplace_node(PROBE_FIRST).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PROBE_SECOND).set_reported(TEST_PARAMETER_2);
    endpoint.emplace_node(REMAINING).set_reported<uint8_t>(1);

    call_default_probe_report(TEST_PARAMETER_2, 0);

    EXPECT_EQ(0, endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::default_resets_every_parameter)).reported<uint8_t>());
}

// A partial-reset probe remains pending until the second reported value arrives.
TEST_F(ConfigurationCommandClassTest, PartialResetProbeWaitsForSecondReportedValue)
{
    constexpr auto SCAN_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase);
    constexpr auto PROBE_FIRST  = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first);
    constexpr auto PROBE_SECOND = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second);
    constexpr auto REMAINING    = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining);
    auto first                  = parameter(TEST_PARAMETER_1);
    auto second                 = parameter(TEST_PARAMETER_2);
    first.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    second.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    first.emplace_node(PARAMETER_VALUE).set_reported<int64_t>(0);
    second.emplace_node(PARAMETER_VALUE).set_desired<int64_t>(0);
    first.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value)).set_reported<int64_t>(0);
    second.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value)).set_reported<int64_t>(0);
    endpoint.emplace_node(SCAN_PHASE).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_PARTIAL_RESET));
    endpoint.emplace_node(PROBE_FIRST).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PROBE_SECOND).set_reported(TEST_PARAMETER_2);
    endpoint.emplace_node(REMAINING).set_reported<uint8_t>(2);

    call_default_probe_report(TEST_PARAMETER_1, 0);

    EXPECT_FALSE(endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::default_resets_every_parameter)).reported_exists());
    EXPECT_EQ(1, endpoint.child_by_type(REMAINING).reported<uint8_t>());
}

// The partial-reset probe records false when the first cached default is absent.
TEST_F(ConfigurationCommandClassTest, PartialResetProbeRequiresFirstDefault)
{
    constexpr auto SCAN_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase);
    constexpr auto PROBE_FIRST  = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first);
    constexpr auto PROBE_SECOND = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second);
    constexpr auto REMAINING    = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining);
    auto first                  = parameter(TEST_PARAMETER_1);
    auto second                 = parameter(TEST_PARAMETER_2);
    first.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    second.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    first.emplace_node(PARAMETER_VALUE).set_reported<int64_t>(0);
    second.emplace_node(PARAMETER_VALUE).set_reported<int64_t>(0);
    second.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value)).set_reported<int64_t>(0);
    endpoint.emplace_node(SCAN_PHASE).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_PARTIAL_RESET));
    endpoint.emplace_node(PROBE_FIRST).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PROBE_SECOND).set_reported(TEST_PARAMETER_2);
    endpoint.emplace_node(REMAINING).set_reported<uint8_t>(1);

    call_default_probe_report(TEST_PARAMETER_2, 0);

    EXPECT_EQ(0, endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::default_resets_every_parameter)).reported<uint8_t>());
}

// The partial-reset probe records false when the second cached default is absent.
TEST_F(ConfigurationCommandClassTest, PartialResetProbeRequiresSecondDefault)
{
    constexpr auto SCAN_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase);
    constexpr auto PROBE_FIRST  = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first);
    constexpr auto PROBE_SECOND = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second);
    constexpr auto REMAINING    = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining);
    auto first                  = parameter(TEST_PARAMETER_1);
    auto second                 = parameter(TEST_PARAMETER_2);
    first.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    second.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    first.emplace_node(PARAMETER_VALUE).set_reported<int64_t>(0);
    second.emplace_node(PARAMETER_VALUE).set_reported<int64_t>(0);
    first.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value)).set_reported<int64_t>(0);
    endpoint.emplace_node(SCAN_PHASE).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_PARTIAL_RESET));
    endpoint.emplace_node(PROBE_FIRST).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PROBE_SECOND).set_reported(TEST_PARAMETER_2);
    endpoint.emplace_node(REMAINING).set_reported<uint8_t>(1);

    call_default_probe_report(TEST_PARAMETER_1, 0);

    EXPECT_EQ(0, endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::default_resets_every_parameter)).reported<uint8_t>());
}

// A missing cached value in a probe response is interpreted as zero while the second report remains pending.
TEST_F(ConfigurationCommandClassTest, DefaultProbeParsedReportDefaultsMissingValueToZero)
{
    constexpr auto SCAN_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase);
    constexpr auto PROBE_FIRST  = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first);
    constexpr auto PROBE_SECOND = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second);
    constexpr auto REMAINING    = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining);
    auto first                  = parameter(TEST_PARAMETER_1);
    parameter(TEST_PARAMETER_2);
    endpoint.emplace_node(PARAMETER_SCAN_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(SCAN_PHASE).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS));
    endpoint.emplace_node(PROBE_FIRST).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PROBE_SECOND).set_reported(TEST_PARAMETER_2);
    endpoint.emplace_node(REMAINING).set_reported<uint8_t>(2);

    ASSERT_EQ(SL_STATUS_OK, call_report_parsed({{"parameter_number", TEST_PARAMETER_1}, {"size", TEST_SIZE_ONE_BYTE}}));

    EXPECT_EQ(0, first.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value)).reported<int64_t>());
    EXPECT_EQ(1, endpoint.child_by_type(REMAINING).reported<uint8_t>());
}

// A desired-only cached value is not used as the actual value in a default-probe report.
TEST_F(ConfigurationCommandClassTest, DefaultProbeParsedReportIgnoresDesiredCachedValue)
{
    constexpr auto SCAN_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase);
    constexpr auto PROBE_FIRST  = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first);
    constexpr auto PROBE_SECOND = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second);
    constexpr auto REMAINING    = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining);
    auto first                  = parameter(TEST_PARAMETER_1);
    first.emplace_node(PARAMETER_VALUE).set_desired<int64_t>(9);
    parameter(TEST_PARAMETER_2);
    endpoint.emplace_node(PARAMETER_SCAN_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(SCAN_PHASE).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS));
    endpoint.emplace_node(PROBE_FIRST).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PROBE_SECOND).set_reported(TEST_PARAMETER_2);
    endpoint.emplace_node(REMAINING).set_reported<uint8_t>(2);

    ASSERT_EQ(SL_STATUS_OK, call_report_parsed({{"parameter_number", TEST_PARAMETER_1}, {"size", TEST_SIZE_ONE_BYTE}}));

    EXPECT_EQ(0, first.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value)).reported<int64_t>());
    EXPECT_EQ(1, endpoint.child_by_type(REMAINING).reported<uint8_t>());
}

// Spec 2026B Configuration CC v2, Configuration Bulk Set validation.
// Bulk Set allows unknown parameters but rejects parameters marked read-only or no-bulk.
TEST_F(ConfigurationCommandClassTest, BulkSetChecksKnownParameterRestrictions)
{
    constexpr auto READ_ONLY = static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::read_only);
    constexpr auto NO_BULK   = static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::no_bulk_support);
    auto known_parameter     = parameter(TEST_PARAMETER_1);
    EXPECT_TRUE(command_class_configuration::bulk_set_allowed(endpoint, TEST_PARAMETER_1, 1));
    known_parameter.emplace_node(READ_ONLY).set_reported<uint8_t>(0);
    known_parameter.emplace_node(NO_BULK).set_reported<uint8_t>(0);

    EXPECT_TRUE(command_class_configuration::bulk_set_allowed(endpoint, TEST_PARAMETER_1, 1));
    EXPECT_TRUE(command_class_configuration::bulk_set_allowed(endpoint, TEST_PARAMETER_2, 1));

    known_parameter.child_by_type(READ_ONLY).set_reported<uint8_t>(1);
    EXPECT_FALSE(command_class_configuration::bulk_set_allowed(endpoint, TEST_PARAMETER_1, 1));
    known_parameter.child_by_type(READ_ONLY).set_reported<uint8_t>(0);
    known_parameter.child_by_type(NO_BULK).clear_reported();
    EXPECT_TRUE(command_class_configuration::bulk_set_allowed(endpoint, TEST_PARAMETER_1, 1));
    known_parameter.child_by_type(NO_BULK).set_reported<uint8_t>(0);
    known_parameter.child_by_type(NO_BULK).set_reported<uint8_t>(1);
    EXPECT_FALSE(command_class_configuration::bulk_set_allowed(endpoint, TEST_PARAMETER_1, 1));
    EXPECT_FALSE(command_class_configuration::bulk_set_allowed(endpoint, 0xFFFF, 2));
}

// Bulk Set restriction flags are ignored until their reported values are available.
TEST_F(ConfigurationCommandClassTest, BulkSetAllowsDesiredOnlyRestrictionFlags)
{
    auto known = parameter(TEST_PARAMETER_1);
    known.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::read_only)).set_desired<uint8_t>(1);
    known.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::no_bulk_support)).set_desired<uint8_t>(1);

    EXPECT_TRUE(command_class_configuration::bulk_set_allowed(endpoint, TEST_PARAMETER_1, 1));
}

// Spec 2026B Configuration CC v1, Configuration Report refresh behavior.
// Default Reset clears cached values and requests refreshes only for v1 parameter numbers.
TEST_F(ConfigurationCommandClassTest, ClearReportedValuesRefreshesV1Parameters)
{
    auto first = parameter(TEST_PARAMETER_1);
    first.emplace_node(PARAMETER_VALUE).set_reported<int64_t>(7);
    auto later = parameter(300);
    later.emplace_node(PARAMETER_VALUE).set_reported<int64_t>(8);

    command_class_configuration::clear_reported_values_and_refresh(endpoint);

    EXPECT_FALSE(first.child_by_type(PARAMETER_VALUE).reported_exists());
    EXPECT_FALSE(later.child_by_type(PARAMETER_VALUE).reported_exists());
    EXPECT_TRUE(endpoint.child_by_type(GET_GROUP).is_valid());
}

// Default reset refresh ignores unreported IDs, clears cached values, and refreshes known v1 numbers.
TEST_F(ConfigurationCommandClassTest, ClearReportedValuesHandlesPartialParameterRecords)
{
    auto unreported = endpoint.emplace_node(PARAMETER_ID);
    unreported.emplace_node(PARAMETER_VALUE).set_reported<int64_t>(1);
    auto parameter_without_value = parameter(TEST_PARAMETER_1);
    auto out_of_range            = parameter(300);
    out_of_range.emplace_node(PARAMETER_VALUE).set_reported<int64_t>(2);

    command_class_configuration::clear_reported_values_and_refresh(endpoint);

    EXPECT_TRUE(unreported.child_by_type(PARAMETER_VALUE).reported_exists());
    EXPECT_FALSE(parameter_without_value.child_by_type(PARAMETER_VALUE).is_valid());
    EXPECT_FALSE(out_of_range.child_by_type(PARAMETER_VALUE).reported_exists());
    EXPECT_TRUE(endpoint.child_by_type(GET_GROUP).is_valid());
}

// Refresh clears values for 16-bit parameter IDs without issuing v1 Get requests.
TEST_F(ConfigurationCommandClassTest, ClearReportedValuesDoesNotGetExtendedParameterIds)
{
    constexpr uint16_t EXTENDED_PARAMETER_ID = 258;
    auto extended_parameter                  = parameter(EXTENDED_PARAMETER_ID);
    extended_parameter.emplace_node(PARAMETER_VALUE).set_reported<int64_t>(5);

    command_class_configuration::clear_reported_values_and_refresh(endpoint);

    EXPECT_FALSE(extended_parameter.child_by_type(PARAMETER_VALUE).reported_exists());
    EXPECT_FALSE(endpoint.child_by_type(GET_GROUP).is_valid());
}

// Spec 2026B Configuration CC v1, Configuration Get.
// MQTT Configuration Get validates its parameter and creates the matching resolver request.
TEST_F(ConfigurationCommandClassTest, MqttGetRejectsMissingParameterAndCreatesRequest)
{
    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_get_command(endpoint, "{}"));
    ASSERT_EQ(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_get_command(endpoint, R"({"parameter_number":2})"));
    EXPECT_EQ(2, endpoint.child_by_type(GET_GROUP).child_by_type(static_cast<attribute_store_type_t>(configuration_get_group_attributes_t::parameter_number)).desired<uint8_t>());
}

// Description getters reject payloads that omit the required parameter number.
TEST_F(ConfigurationCommandClassTest, MqttDescriptionGettersRejectMissingParameter)
{
    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_name_get_command(endpoint, "{}"));
    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_info_get_command(endpoint, "{}"));
    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_properties_get_command(endpoint, "{}"));
    EXPECT_FALSE(endpoint.child_by_type(NAME_GET_GROUP).is_valid());
    EXPECT_FALSE(endpoint.child_by_type(INFO_GET_GROUP).is_valid());
    EXPECT_FALSE(endpoint.child_by_type(PROPERTIES_GET_GROUP).is_valid());
}

// MQTT Set uses a reported size when omitted and validates the value unless defaulting.
TEST_F(ConfigurationCommandClassTest, MqttSetUsesReportedSizeAndValidatesValue)
{
    auto known = parameter(TEST_PARAMETER_1);
    known.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_set_command(endpoint, R"({"parameter_number":1,"default_flag":"bad"})"));
    EXPECT_FALSE(endpoint.child_by_type(SET_GROUP).is_valid());

    ASSERT_EQ(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_set_command(endpoint, R"({"parameter_number":1,"default_flag":1})"));
    EXPECT_TRUE(endpoint.child_by_type(SET_GROUP).is_valid());
}

// MQTT Set cannot infer a size from parameter metadata that is only desired, not reported.
TEST_F(ConfigurationCommandClassTest, MqttSetRejectsDesiredOnlySizeFallback)
{
    parameter(TEST_PARAMETER_1).emplace_node(PARAMETER_SIZE).set_desired(TEST_SIZE_ONE_BYTE);

    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_set_command(endpoint, R"({"parameter_number":1,"value":7})"));
    EXPECT_FALSE(endpoint.child_by_type(SET_GROUP).is_valid());
}

// Spec 2026B Configuration CC v3, Configuration Name Get.
// The name request accepts a 16-bit parameter number and stores it in the request group.
TEST_F(ConfigurationCommandClassTest, MqttNameGetCreatesRequest)
{
    ASSERT_EQ(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_name_get_command(endpoint, R"({"parameter_number":258})"));
    EXPECT_EQ(TEST_PARAMETER_OFFSET, endpoint.child_by_type(NAME_GET_GROUP).child_by_type(static_cast<attribute_store_type_t>(configuration_name_get_group_attributes_t::parameter_number)).desired<uint16_t>());
}

// Spec 2026B Configuration CC v3, Configuration Info Get.
// The MQTT Info Get request stores the requested parameter before resolution starts.
TEST_F(ConfigurationCommandClassTest, MqttInfoGetCreatesRequest)
{
    ASSERT_EQ(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_info_get_command(endpoint, R"({"parameter_number":2})"));
    EXPECT_EQ(2, endpoint.child_by_type(INFO_GET_GROUP).child_by_type(static_cast<attribute_store_type_t>(configuration_info_get_group_attributes_t::parameter_number)).desired<uint16_t>());
}

// Spec 2026B Configuration CC v3, Configuration Properties Get.
// The MQTT properties request stores a 16-bit parameter number in its group.
TEST_F(ConfigurationCommandClassTest, MqttPropertiesGetCreatesRequest)
{
    ASSERT_EQ(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_properties_get_command(endpoint, R"({"parameter_number":258})"));
    EXPECT_EQ(TEST_PARAMETER_OFFSET, endpoint.child_by_type(PROPERTIES_GET_GROUP).child_by_type(static_cast<attribute_store_type_t>(configuration_properties_get_group_attributes_t::parameter_number)).desired<uint16_t>());
}

// Spec 2026B Configuration CC v4, Configuration Default Reset.
// The MQTT reset request creates its command group without requiring a payload.
TEST_F(ConfigurationCommandClassTest, MqttDefaultResetCreatesRequest)
{
    ASSERT_EQ(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_default_reset_command(endpoint, "{}"));
    EXPECT_TRUE(endpoint.child_by_type(DEFAULT_RESET_GROUP).is_valid());
}

// Spec 2026B Configuration CC v3, Configuration Properties discovery.
// The MQTT discover command begins a properties walk on a version 3 endpoint.
TEST_F(ConfigurationCommandClassTest, MqttPropertiesDiscoverStartsWalk)
{
    endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_CONFIGURATION)).set_reported(CONFIGURATION_VERSION_3);

    ASSERT_EQ(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_properties_discover_command(endpoint, "{}"));

    EXPECT_EQ(1, endpoint.child_by_type(PROPERTIES_WALK_ACTIVE).reported<uint8_t>());
    EXPECT_TRUE(endpoint.child_by_type(PROPERTIES_GET_GROUP).is_valid());
}

// Spec 2026B Configuration CC v1, Configuration parameter scanning.
// The MQTT scan command begins discovery and schedules the first parameter probe.
TEST_F(ConfigurationCommandClassTest, MqttParameterScanStartsScan)
{
    ASSERT_EQ(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_parameter_scan_command(endpoint, "{}"));

    EXPECT_EQ(1, endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported<uint8_t>());
    EXPECT_TRUE(endpoint.child_by_type(SET_GROUP).is_valid());
}

// Spec 2026B Configuration CC v2, Configuration Bulk Set.
// An explicit size and one value per parameter are flattened into the desired request payload.
TEST_F(ConfigurationCommandClassTest, MqttBulkSetStoresValues)
{
    const auto payload = R"({"parameter_offset":258,"number_of_parameters":2,"properties1":{"size":1},"vg":[{"parameter":[7]},{"parameter":[8]}]})";

    ASSERT_EQ(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_bulk_set_command(endpoint, payload));

    const auto group = endpoint.child_by_type(BULK_SET_GROUP);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(TEST_PARAMETER_OFFSET, group.child_by_type(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::parameter_offset)).desired<uint16_t>());
    EXPECT_EQ((std::vector<uint8_t> {7, 8}), group.child_by_type(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::vg)).desired<std::vector<uint8_t>>());
}

// Spec 2026B Configuration CC v2, Configuration Bulk Set.
// When size is omitted, MQTT infers one common size from all parameter reports.
TEST_F(ConfigurationCommandClassTest, MqttBulkSetInfersCommonReportedSize)
{
    constexpr auto BULK_SET_SIZE   = static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::size);
    constexpr auto BULK_SET_VALUES = static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::vg);
    parameter(TEST_PARAMETER_OFFSET).emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    parameter(TEST_PARAMETER_OFFSET + 1).emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    const auto payload = R"({"parameter_offset":258,"number_of_parameters":2,"properties1":{},"vg":[{"parameter":[7]},{"parameter":[8]}]})";

    ASSERT_EQ(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_bulk_set_command(endpoint, payload));

    const auto group = endpoint.child_by_type(BULK_SET_GROUP);
    EXPECT_EQ(TEST_SIZE_ONE_BYTE, group.child_by_type(BULK_SET_SIZE).desired<uint8_t>());
    EXPECT_EQ((std::vector<uint8_t> {7, 8}), group.child_by_type(BULK_SET_VALUES).desired<std::vector<uint8_t>>());
}

// Spec 2026B Configuration CC v2, Configuration Bulk Set.
// The default flag omits value input and creates a zero-filled default payload.
TEST_F(ConfigurationCommandClassTest, MqttBulkSetDefaultFlagCreatesZeroPayload)
{
    constexpr auto BULK_SET_DEFAULT = static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::default_flag);
    constexpr auto BULK_SET_VALUES  = static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::vg);
    const auto payload              = R"({"parameter_offset":258,"number_of_parameters":2,"properties1":{"size":1,"default_flag":1}})";

    ASSERT_EQ(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_bulk_set_command(endpoint, payload));

    const auto group = endpoint.child_by_type(BULK_SET_GROUP);
    EXPECT_EQ(1, group.child_by_type(BULK_SET_DEFAULT).desired<uint8_t>());
    EXPECT_EQ((std::vector<uint8_t> {0, 0}), group.child_by_type(BULK_SET_VALUES).desired<std::vector<uint8_t>>());
}

// A malformed JSON body is rejected without creating a Bulk Set group.
TEST_F(ConfigurationCommandClassTest, MqttBulkSetRejectsMalformedPayload)
{
    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_bulk_set_command(endpoint, "not-json"));
    EXPECT_FALSE(endpoint.child_by_type(BULK_SET_GROUP).is_valid());
}

// CC:0070.02.07.11.002 — Bulk Set requires a parameter count in the range 1..255.
TEST_F(ConfigurationCommandClassTest, MqttBulkSetRejectsEmptyRange)
{
    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_bulk_set_command(endpoint, R"({"parameter_offset":1,"number_of_parameters":0,"properties1":{"size":1},"vg":[]})"));
    EXPECT_FALSE(endpoint.child_by_type(BULK_SET_GROUP).is_valid());
}

// Bulk Set without an explicit size is rejected when no reported parameter size exists.
TEST_F(ConfigurationCommandClassTest, MqttBulkSetRejectsMissingReportedSize)
{
    const auto payload = R"({"parameter_offset":258,"number_of_parameters":1,"properties1":{},"vg":[{"parameter":[7]}]})";
    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_bulk_set_command(endpoint, payload));
    EXPECT_FALSE(endpoint.child_by_type(BULK_SET_GROUP).is_valid());
}

// CC:0070.02.07.11.00C — Bulk Set cannot infer one common width when parameters have different reported sizes.
TEST_F(ConfigurationCommandClassTest, MqttBulkSetRejectsInconsistentReportedSizes)
{
    parameter(TEST_PARAMETER_OFFSET).emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    parameter(TEST_PARAMETER_OFFSET + 1).emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_TWO_BYTES);
    const auto payload = R"({"parameter_offset":258,"number_of_parameters":2,"properties1":{},"vg":[{"parameter":[7]},{"parameter":[8,9]}]})";
    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_bulk_set_command(endpoint, payload));
    EXPECT_FALSE(endpoint.child_by_type(BULK_SET_GROUP).is_valid());
}

// CC:0070.02.07.11.001 / CC:0070.02.07.11.002 — A Bulk Set range cannot overflow the 16-bit parameter space.
TEST_F(ConfigurationCommandClassTest, MqttBulkSetRejectsRangeOverflow)
{
    constexpr uint16_t LAST_PARAMETER = 0xFFFF;
    parameter(LAST_PARAMETER).emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    const auto overflow = R"({"parameter_offset":65535,"number_of_parameters":2,"properties1":{"default_flag":1}})";
    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_bulk_set_command(endpoint, overflow));
    EXPECT_FALSE(endpoint.child_by_type(BULK_SET_GROUP).is_valid());
}

// Desired-only parameter size metadata cannot supply the inferred Bulk Set width.
TEST_F(ConfigurationCommandClassTest, MqttBulkSetRejectsDesiredOnlySize)
{
    auto known = parameter(TEST_PARAMETER_1);
    known.emplace_node(PARAMETER_SIZE).set_desired(TEST_SIZE_ONE_BYTE);
    const auto not_reported = R"({"parameter_offset":1,"number_of_parameters":1,"properties1":{"default_flag":1}})";
    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_bulk_set_command(endpoint, not_reported));
    EXPECT_FALSE(endpoint.child_by_type(BULK_SET_GROUP).is_valid());
}

// CC:0070.02.07.11.00C — An invalid reported parameter size cannot be inferred for Bulk Set.
TEST_F(ConfigurationCommandClassTest, MqttBulkSetRejectsInvalidReportedWidth)
{
    auto known = parameter(TEST_PARAMETER_1);
    known.emplace_node(PARAMETER_SIZE).set_reported<uint8_t>(0);
    const auto invalid_width = R"({"parameter_offset":1,"number_of_parameters":1,"properties1":{"default_flag":1}})";
    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_bulk_set_command(endpoint, invalid_width));
    EXPECT_FALSE(endpoint.child_by_type(BULK_SET_GROUP).is_valid());
}

// CC:0070.04.0F.11.001 — A read-only parameter cannot be included in a Bulk Set request.
TEST_F(ConfigurationCommandClassTest, MqttBulkSetRejectsReadOnlyParameter)
{
    const auto payload = R"({"parameter_offset":1,"number_of_parameters":1,"properties1":{"size":1,"default_flag":1}})";
    auto known         = parameter(TEST_PARAMETER_1);
    known.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    auto read_only = known.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::read_only));
    read_only.set_reported<uint8_t>(1);
    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_bulk_set_command(endpoint, payload));
    EXPECT_FALSE(endpoint.child_by_type(BULK_SET_GROUP).is_valid());
}

// CC:0070.04.0F.11.007 — A parameter that does not support Bulk Commands cannot be included in Bulk Set.
TEST_F(ConfigurationCommandClassTest, MqttBulkSetRejectsNoBulkParameter)
{
    const auto payload = R"({"parameter_offset":1,"number_of_parameters":1,"properties1":{"size":1,"default_flag":1}})";
    auto known         = parameter(TEST_PARAMETER_1);
    known.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    known.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::no_bulk_support)).set_reported<uint8_t>(1);
    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_bulk_set_command(endpoint, payload));
    EXPECT_FALSE(endpoint.child_by_type(BULK_SET_GROUP).is_valid());
}

// CC:0070.02.07.11.00E — A Bulk Set request requires one value item for each parameter.
TEST_F(ConfigurationCommandClassTest, MqttBulkSetRejectsWrongValueCount)
{
    const auto wrong_count = R"({"parameter_offset":258,"number_of_parameters":2,"properties1":{"size":1},"vg":[{"parameter":[7]}]})";
    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_bulk_set_command(endpoint, wrong_count));
    EXPECT_FALSE(endpoint.child_by_type(BULK_SET_GROUP).is_valid());
}

// CC:0070.02.07.11.00E — Each Bulk Set value must match the request's advertised byte width.
TEST_F(ConfigurationCommandClassTest, MqttBulkSetRejectsWrongValueWidth)
{
    const auto wrong_width = R"({"parameter_offset":258,"number_of_parameters":1,"properties1":{"size":2},"vg":[{"parameter":[7]}]})";
    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_bulk_set_command(endpoint, wrong_width));
    EXPECT_FALSE(endpoint.child_by_type(BULK_SET_GROUP).is_valid());
}

// Configuration MQTT routes dispatch to the same request handlers as direct calls.
TEST_F(ConfigurationCommandClassTest, RegisteredMqttRoutesDispatchAllCommands)
{
    endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_CONFIGURATION)).set_reported(CONFIGURATION_VERSION_3);
    mqtt().mqtt_command_handler(endpoint, "ConfigurationBulkGet", R"({"parameter_offset":1,"number_of_parameters":1})");
    mqtt().mqtt_command_handler(endpoint, "ConfigurationBulkSet", R"({"parameter_offset":1,"number_of_parameters":1,"properties1":{"size":1},"vg":[{"parameter":[7]}]})");
    mqtt().mqtt_command_handler(endpoint, "ConfigurationGet", R"({"parameter_number":1})");
    mqtt().mqtt_command_handler(endpoint, "ConfigurationSet", R"({"parameter_number":1,"size":1,"value":7})");
    mqtt().mqtt_command_handler(endpoint, "ConfigurationNameGet", R"({"parameter_number":258})");
    mqtt().mqtt_command_handler(endpoint, "ConfigurationInfoGet", R"({"parameter_number":258})");
    mqtt().mqtt_command_handler(endpoint, "ConfigurationPropertiesGet", R"({"parameter_number":258})");
    mqtt().mqtt_command_handler(endpoint, "ConfigurationDefaultReset", "{}");
    mqtt().mqtt_command_handler(endpoint, "ConfigurationPropertiesDiscover", "{}");
    mqtt().mqtt_command_handler(endpoint, "ConfigurationParameterScan", "{}");

    EXPECT_TRUE(endpoint.child_by_type(BULK_GET_GROUP).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(BULK_SET_GROUP).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(GET_GROUP).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(SET_GROUP).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(NAME_GET_GROUP).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(INFO_GET_GROUP).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(PROPERTIES_GET_GROUP).is_valid());
    EXPECT_TRUE(endpoint.child_by_type(DEFAULT_RESET_GROUP).is_valid());
    EXPECT_EQ(1, endpoint.child_by_type(PROPERTIES_WALK_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported<uint8_t>());
}

// Spec 2026B Configuration CC v1, Configuration Report.
// The v1 wire report is parsed, stored, and passed through the Command Class report handler.
TEST_F(ConfigurationCommandClassTest, ConfigurationReportFrameStoresParameter)
{
    constexpr uint8_t COMMAND_CLASS_ID      = 0x70;
    constexpr uint8_t REPORT_COMMAND        = 0x06;
    constexpr uint8_t SIGNED_ONE_BYTE_VALUE = 0x80;
    const std::array<uint8_t, 6> frame {COMMAND_CLASS_ID, REPORT_COMMAND, TEST_PARAMETER_1, TEST_SIZE_ONE_BYTE, SIGNED_ONE_BYTE_VALUE, 0};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_configuration_core::report_received_args args {endpoint, parser, nullptr};

    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_report_received(args));

    const auto stored_parameter = endpoint.child_by_type_and_value(PARAMETER_ID, TEST_PARAMETER_1);
    ASSERT_TRUE(stored_parameter.is_valid());
    EXPECT_EQ(-128, stored_parameter.child_by_type(PARAMETER_VALUE).reported<int64_t>());
}

// Spec 2026B Configuration CC v3, Configuration Properties Report (CC:0070.03.0F.11.001).
// A terminal next-parameter value ends the properties walk and marks discovery complete.
TEST_F(ConfigurationCommandClassTest, PropertiesReportFrameEndsWalkAtTerminator)
{
    constexpr uint8_t COMMAND_CLASS_ID           = 0x70;
    constexpr uint8_t PROPERTIES_REPORT_COMMAND  = 0x0F;
    constexpr uint8_t ONE_BYTE_SIGNED_PROPERTIES = 0x01;
    constexpr uint8_t MINIMUM_SIGNED_VALUE       = 0x80;
    constexpr uint8_t MAXIMUM_SIGNED_VALUE       = 0x7F;
    constexpr uint8_t DEFAULT_VALUE              = 0x05;
    endpoint.emplace_node(PROPERTIES_WALK_ACTIVE).set_reported<uint8_t>(1);
    const std::array<uint8_t, 11> frame {COMMAND_CLASS_ID, PROPERTIES_REPORT_COMMAND, 0, TEST_PARAMETER_1, ONE_BYTE_SIGNED_PROPERTIES, MINIMUM_SIGNED_VALUE, MAXIMUM_SIGNED_VALUE, DEFAULT_VALUE, 0, 0, 0};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_configuration_core::report_received_args args {endpoint, parser, nullptr};

    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_properties_report_received(args));

    const auto stored_parameter = endpoint.child_by_type_and_value(PARAMETER_ID, TEST_PARAMETER_1);
    ASSERT_TRUE(stored_parameter.is_valid());
    EXPECT_EQ(-128, stored_parameter.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::min_value)).reported<int64_t>());
    EXPECT_EQ(127, stored_parameter.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::max_value)).reported<int64_t>());
    EXPECT_EQ(0, endpoint.child_by_type(PROPERTIES_WALK_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
}

// Spec 2026B Configuration CC v3, Properties Report.
// A non-terminating next parameter keeps the walk active and queues the next Properties Get.
TEST_F(ConfigurationCommandClassTest, PropertiesReportFrameRequestsNextParameter)
{
    constexpr uint8_t COMMAND_CLASS_ID           = 0x70;
    constexpr uint8_t PROPERTIES_REPORT_COMMAND  = 0x0F;
    constexpr uint8_t ONE_BYTE_SIGNED_PROPERTIES = 0x01;
    constexpr uint16_t NEXT_PARAMETER            = 2;
    endpoint.emplace_node(PROPERTIES_WALK_ACTIVE).set_reported<uint8_t>(1);
    const std::array<uint8_t, 10> frame {COMMAND_CLASS_ID, PROPERTIES_REPORT_COMMAND, 0, TEST_PARAMETER_1, ONE_BYTE_SIGNED_PROPERTIES, 0x80, 0x7F, 0x05, 0, NEXT_PARAMETER};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_configuration_core::report_received_args args {endpoint, parser, nullptr};

    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_properties_report_received(args));

    EXPECT_EQ(1, endpoint.child_by_type(PROPERTIES_WALK_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(NEXT_PARAMETER, endpoint.child_by_type(PROPERTIES_GET_GROUP).child_by_type(static_cast<attribute_store_type_t>(configuration_properties_get_group_attributes_t::parameter_number)).desired<uint16_t>());
}

// When parameter zero reports a zero terminator, the compatibility quirk reads the trailing next ID.
TEST_F(ConfigurationCommandClassTest, PropertiesReportParameterZeroUsesTrailingNextIdQuirk)
{
    constexpr uint8_t COMMAND_CLASS_ID           = 0x70;
    constexpr uint8_t PROPERTIES_REPORT_COMMAND  = 0x0F;
    constexpr uint8_t ONE_BYTE_SIGNED_PROPERTIES = 0x01;
    constexpr uint16_t QUIRK_NEXT_PARAMETER      = 0x0102;
    endpoint.emplace_node(PROPERTIES_WALK_ACTIVE).set_reported<uint8_t>(1);

    // The standard next-parameter field is zero; the final two compatibility bytes carry parameter 258.
    const std::array<uint8_t, 12> frame {COMMAND_CLASS_ID, PROPERTIES_REPORT_COMMAND, 0, 0, ONE_BYTE_SIGNED_PROPERTIES, 0x80, 0x7F, 0x05, 0, 0, 0x01, 0x02};

    ASSERT_EQ(SL_STATUS_OK, call_control_handler(frame.data(), static_cast<uint16_t>(frame.size())));

    EXPECT_EQ(QUIRK_NEXT_PARAMETER, endpoint.child_by_type(PROPERTIES_GET_GROUP).child_by_type(static_cast<attribute_store_type_t>(configuration_properties_get_group_attributes_t::parameter_number)).desired<uint16_t>());
}

// A terminal Properties Report starts the requested name-description phase for known parameters.
TEST_F(ConfigurationCommandClassTest, PropertiesReportTerminatorStartsRequestedDescriptions)
{
    constexpr uint8_t COMMAND_CLASS_ID           = 0x70;
    constexpr uint8_t PROPERTIES_REPORT_COMMAND  = 0x0F;
    constexpr uint8_t ONE_BYTE_SIGNED_PROPERTIES = 0x01;
    constexpr auto DESCRIPTION_PHASE             = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_phase);
    endpoint.emplace_node(PROPERTIES_WALK_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(DISCOVERY_REQUESTED).set_reported<uint8_t>(1);
    parameter(TEST_PARAMETER_1).emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    const std::array<uint8_t, 11> frame {COMMAND_CLASS_ID, PROPERTIES_REPORT_COMMAND, 0, TEST_PARAMETER_2, ONE_BYTE_SIGNED_PROPERTIES, 0, 0x7F, 0, 0, 0, 0};

    ASSERT_EQ(SL_STATUS_OK, call_control_handler(frame.data(), static_cast<uint16_t>(frame.size())));

    EXPECT_EQ(static_cast<uint8_t>(discovery_description_phase_t::NAME), endpoint.child_by_type(DESCRIPTION_PHASE).reported<uint8_t>());
    EXPECT_EQ(TEST_PARAMETER_1, endpoint.child_by_type(NAME_GET_GROUP).child_by_type(static_cast<attribute_store_type_t>(configuration_name_get_group_attributes_t::parameter_number)).desired<uint16_t>());
}

// A Properties Report received outside an active walk is stored without scheduling discovery work.
TEST_F(ConfigurationCommandClassTest, PropertiesReportOutsideWalkDoesNotQueueRequest)
{
    ASSERT_EQ(SL_STATUS_OK, call_properties_report_parsed({{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"next_parameter_number", static_cast<uint16_t>(0)}}));

    EXPECT_FALSE(endpoint.child_by_type(PROPERTIES_GET_GROUP).is_valid());
}

// A Properties Report outside an active, reported walk is stored without continuing discovery.
TEST_F(ConfigurationCommandClassTest, PropertiesReportIgnoresUnreportedAndInactiveWalkFlags)
{
    auto active = endpoint.emplace_node(PROPERTIES_WALK_ACTIVE);
    active.set_desired<uint8_t>(1);
    ASSERT_EQ(SL_STATUS_OK, call_properties_report_parsed({{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"next_parameter_number", static_cast<uint16_t>(TEST_PARAMETER_2)}}));
    EXPECT_FALSE(endpoint.child_by_type(PROPERTIES_GET_GROUP).is_valid());

    active.set_reported<uint8_t>(0);
    ASSERT_EQ(SL_STATUS_OK, call_properties_report_parsed({{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"next_parameter_number", static_cast<uint16_t>(TEST_PARAMETER_2)}}));
    EXPECT_FALSE(endpoint.child_by_type(PROPERTIES_GET_GROUP).is_valid());
}

// A terminal Properties Report completes an unrequested walk without starting description discovery.
TEST_F(ConfigurationCommandClassTest, PropertiesReportTerminatorWithoutDescriptionRequestCompletes)
{
    constexpr auto WALK_ACTIVE = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::properties_walk_active);
    endpoint.emplace_node(WALK_ACTIVE).set_reported<uint8_t>(1);

    ASSERT_EQ(SL_STATUS_OK, call_properties_report_parsed({{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"next_parameter_number", static_cast<uint16_t>(0)}}));

    EXPECT_EQ(0, endpoint.child_by_type(WALK_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
    EXPECT_FALSE(endpoint.child_by_type(NAME_GET_GROUP).is_valid());
}

// A reported zero description-request flag completes the properties walk without starting descriptions.
TEST_F(ConfigurationCommandClassTest, PropertiesReportTerminatorHonorsDisabledDescriptionRequest)
{
    constexpr auto WALK_ACTIVE           = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::properties_walk_active);
    constexpr auto DESCRIPTION_REQUESTED = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_descriptions_requested);
    endpoint.emplace_node(WALK_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(DESCRIPTION_REQUESTED).set_reported<uint8_t>(0);

    ASSERT_EQ(SL_STATUS_OK, call_properties_report_parsed({{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"next_parameter_number", static_cast<uint16_t>(0)}}));

    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
    EXPECT_FALSE(endpoint.child_by_type(NAME_GET_GROUP).is_valid());
}

// An unreported description-request flag does not initiate the optional name and info walk.
TEST_F(ConfigurationCommandClassTest, PropertiesReportTerminatorIgnoresDesiredDescriptionRequest)
{
    constexpr auto WALK_ACTIVE           = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::properties_walk_active);
    constexpr auto DESCRIPTION_REQUESTED = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_descriptions_requested);
    endpoint.emplace_node(WALK_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(DESCRIPTION_REQUESTED).set_desired<uint8_t>(1);

    ASSERT_EQ(SL_STATUS_OK, call_properties_report_parsed({{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"next_parameter_number", static_cast<uint16_t>(0)}}));

    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
    EXPECT_FALSE(endpoint.child_by_type(NAME_GET_GROUP).is_valid());
}

// When the properties list is empty, the description chain checks names, then info, then completes.
TEST_F(ConfigurationCommandClassTest, EmptyDescriptionChainChecksBothPhasesAndCompletes)
{
    endpoint.emplace_node(PROPERTIES_WALK_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(DISCOVERY_REQUESTED).set_reported<uint8_t>(1);

    ASSERT_EQ(SL_STATUS_OK, call_properties_report_parsed({{"parameter_number", static_cast<uint16_t>(0)}, {"next_parameter_number", static_cast<uint16_t>(0)}}));

    EXPECT_EQ(0, endpoint.child_by_type(PROPERTIES_WALK_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(0, endpoint.child_by_type(static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_descriptions_active)).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
}

// Multipart description reports advance discovery only when the active phase and parameter match.
TEST_F(ConfigurationCommandClassTest, DescriptionReportParserChecksActivePhaseAndParameter)
{
    constexpr auto DESCRIPTIONS_ACTIVE = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_descriptions_active);
    constexpr auto DESCRIPTION_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_phase);
    constexpr auto DESCRIPTION_PARAM   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_parameter);
    const command_class_configuration_attribute_map_t final_name {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(0)}};
    const command_class_configuration_attribute_map_t final_info {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(0)}};

    ASSERT_EQ(SL_STATUS_OK, call_name_report_parsed(final_name));
    ASSERT_EQ(SL_STATUS_OK, call_info_report_parsed(final_info));

    endpoint.emplace_node(DESCRIPTIONS_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(DESCRIPTION_PHASE).set_reported(static_cast<uint8_t>(discovery_description_phase_t::NAME));
    endpoint.emplace_node(DESCRIPTION_PARAM).set_reported<uint16_t>(TEST_PARAMETER_2);
    ASSERT_EQ(SL_STATUS_OK, call_name_report_parsed({{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(1)}}));
    EXPECT_FALSE(endpoint.child_by_type(NAME_GET_GROUP).is_valid());

    endpoint.child_by_type(DESCRIPTION_PARAM).set_reported<uint16_t>(TEST_PARAMETER_1);
    parameter(TEST_PARAMETER_1).emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    ASSERT_EQ(SL_STATUS_OK, call_name_report_parsed(final_name));
    EXPECT_EQ(static_cast<uint8_t>(discovery_description_phase_t::INFO), endpoint.child_by_type(DESCRIPTION_PHASE).reported<uint8_t>());
    EXPECT_TRUE(endpoint.child_by_type(INFO_GET_GROUP).is_valid());

    endpoint.child_by_type(DESCRIPTION_PHASE).set_reported(static_cast<uint8_t>(discovery_description_phase_t::INFO));
    ASSERT_EQ(SL_STATUS_OK, call_info_report_parsed(final_info));
    EXPECT_EQ(0, endpoint.child_by_type(DESCRIPTIONS_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
}

// Final name and info fragments do not advance discovery when their active phase or parameter is stale.
TEST_F(ConfigurationCommandClassTest, DescriptionReportParserIgnoresStaleFinalFragments)
{
    constexpr auto DESCRIPTIONS_ACTIVE = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_descriptions_active);
    constexpr auto DESCRIPTION_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_phase);
    constexpr auto DESCRIPTION_PARAM   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_parameter);
    const command_class_configuration_attribute_map_t name_done {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(0)}};
    const command_class_configuration_attribute_map_t info_done {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(0)}};

    endpoint.emplace_node(DESCRIPTIONS_ACTIVE).set_reported<uint8_t>(0);
    endpoint.emplace_node(DESCRIPTION_PHASE).set_reported(static_cast<uint8_t>(discovery_description_phase_t::NAME));
    endpoint.emplace_node(DESCRIPTION_PARAM).set_reported<uint16_t>(TEST_PARAMETER_1);
    ASSERT_EQ(SL_STATUS_OK, call_name_report_parsed(name_done));
    EXPECT_FALSE(endpoint.child_by_type(NAME_GET_GROUP).is_valid());

    endpoint.child_by_type(DESCRIPTIONS_ACTIVE).set_reported<uint8_t>(1);
    endpoint.child_by_type(DESCRIPTION_PHASE).set_reported(static_cast<uint8_t>(discovery_description_phase_t::INFO));
    ASSERT_EQ(SL_STATUS_OK, call_name_report_parsed(name_done));
    EXPECT_FALSE(endpoint.child_by_type(NAME_GET_GROUP).is_valid());

    endpoint.child_by_type(DESCRIPTION_PHASE).set_reported(static_cast<uint8_t>(discovery_description_phase_t::NAME));
    endpoint.child_by_type(DESCRIPTION_PARAM).set_reported<uint16_t>(TEST_PARAMETER_2);
    ASSERT_EQ(SL_STATUS_OK, call_name_report_parsed(name_done));
    EXPECT_FALSE(endpoint.child_by_type(NAME_GET_GROUP).is_valid());

    endpoint.child_by_type(DESCRIPTION_PHASE).set_reported(static_cast<uint8_t>(discovery_description_phase_t::INFO));
    endpoint.child_by_type(DESCRIPTION_PARAM).set_reported<uint16_t>(TEST_PARAMETER_1);
    ASSERT_EQ(SL_STATUS_OK, call_info_report_parsed(info_done));
    EXPECT_FALSE(endpoint.child_by_type(INFO_GET_GROUP).is_valid());
}

// CC:0070.03.0B.11.002 — A final Name Report is ignored for discovery without active controller state.
TEST_F(ConfigurationCommandClassTest, NameReportWithoutReportedTrackingStateDoesNotAdvance)
{
    constexpr auto DESCRIPTIONS_ACTIVE = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_descriptions_active);
    constexpr auto DESCRIPTION_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_phase);
    constexpr auto DESCRIPTION_PARAM   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_parameter);
    const command_class_configuration_attribute_map_t final_name {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(0)}};
    endpoint.emplace_node(DESCRIPTIONS_ACTIVE).set_desired<uint8_t>(1);
    endpoint.emplace_node(DESCRIPTION_PHASE).set_reported(static_cast<uint8_t>(discovery_description_phase_t::NAME));
    endpoint.emplace_node(DESCRIPTION_PARAM).set_reported<uint16_t>(TEST_PARAMETER_1);

    ASSERT_EQ(SL_STATUS_OK, call_name_report_parsed(final_name));

    EXPECT_FALSE(endpoint.child_by_type(NAME_GET_GROUP).is_valid());
}

// CC:0070.03.0B.11.002 — A final Name Report does not advance discovery without a description phase.
TEST_F(ConfigurationCommandClassTest, NameReportWithoutPhaseDoesNotAdvance)
{
    constexpr auto DESCRIPTIONS_ACTIVE = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_descriptions_active);
    constexpr auto DESCRIPTION_PARAM   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_parameter);
    const command_class_configuration_attribute_map_t final_name {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(0)}};
    endpoint.emplace_node(DESCRIPTIONS_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(DESCRIPTION_PARAM).set_reported<uint16_t>(TEST_PARAMETER_1);

    ASSERT_EQ(SL_STATUS_OK, call_name_report_parsed(final_name));

    EXPECT_FALSE(endpoint.child_by_type(NAME_GET_GROUP).is_valid());
    EXPECT_EQ(1, endpoint.child_by_type(DESCRIPTIONS_ACTIVE).reported<uint8_t>());
}

// CC:0070.03.0B.11.002 — A desired-only Name phase cannot advance report processing.
TEST_F(ConfigurationCommandClassTest, NameReportWithDesiredOnlyPhaseDoesNotAdvance)
{
    constexpr auto DESCRIPTIONS_ACTIVE = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_descriptions_active);
    constexpr auto DESCRIPTION_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_phase);
    constexpr auto DESCRIPTION_PARAM   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_parameter);
    const command_class_configuration_attribute_map_t final_name {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(0)}};
    endpoint.emplace_node(DESCRIPTIONS_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(DESCRIPTION_PHASE).set_desired(static_cast<uint8_t>(discovery_description_phase_t::NAME));
    endpoint.emplace_node(DESCRIPTION_PARAM).set_reported<uint16_t>(TEST_PARAMETER_1);

    ASSERT_EQ(SL_STATUS_OK, call_name_report_parsed(final_name));

    EXPECT_FALSE(endpoint.child_by_type(NAME_GET_GROUP).is_valid());
    EXPECT_EQ(1, endpoint.child_by_type(DESCRIPTIONS_ACTIVE).reported<uint8_t>());
}

// CC:0070.03.0D.11.002 — A final Info Report is ignored for discovery without a current parameter.
TEST_F(ConfigurationCommandClassTest, InfoReportWithoutCurrentParameterDoesNotAdvance)
{
    constexpr auto DESCRIPTIONS_ACTIVE = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_descriptions_active);
    constexpr auto DESCRIPTION_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_phase);
    const command_class_configuration_attribute_map_t final_info {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(0)}};
    endpoint.emplace_node(DESCRIPTIONS_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(DESCRIPTION_PHASE).set_reported(static_cast<uint8_t>(discovery_description_phase_t::INFO));

    ASSERT_EQ(SL_STATUS_OK, call_info_report_parsed(final_info));

    EXPECT_FALSE(endpoint.child_by_type(INFO_GET_GROUP).is_valid());
}

// CC:0070.03.0D.11.002 — A final Info Report does not advance discovery without an Info phase.
TEST_F(ConfigurationCommandClassTest, InfoReportWithoutPhaseDoesNotAdvance)
{
    constexpr auto DESCRIPTIONS_ACTIVE = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_descriptions_active);
    constexpr auto DESCRIPTION_PARAM   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_parameter);
    const command_class_configuration_attribute_map_t final_info {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(0)}};
    endpoint.emplace_node(DESCRIPTIONS_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(DESCRIPTION_PARAM).set_reported<uint16_t>(TEST_PARAMETER_1);

    ASSERT_EQ(SL_STATUS_OK, call_info_report_parsed(final_info));

    EXPECT_FALSE(endpoint.child_by_type(INFO_GET_GROUP).is_valid());
    EXPECT_EQ(1, endpoint.child_by_type(DESCRIPTIONS_ACTIVE).reported<uint8_t>());
}

// CC:0070.03.0D.11.002 — A desired-only Info phase cannot advance report processing.
TEST_F(ConfigurationCommandClassTest, InfoReportWithDesiredOnlyPhaseDoesNotAdvance)
{
    constexpr auto DESCRIPTIONS_ACTIVE = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_descriptions_active);
    constexpr auto DESCRIPTION_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_phase);
    constexpr auto DESCRIPTION_PARAM   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_parameter);
    const command_class_configuration_attribute_map_t final_info {{"parameter_number", static_cast<uint16_t>(TEST_PARAMETER_1)}, {"reports_to_follow", static_cast<uint8_t>(0)}};
    endpoint.emplace_node(DESCRIPTIONS_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(DESCRIPTION_PHASE).set_desired(static_cast<uint8_t>(discovery_description_phase_t::INFO));
    endpoint.emplace_node(DESCRIPTION_PARAM).set_reported<uint16_t>(TEST_PARAMETER_1);

    ASSERT_EQ(SL_STATUS_OK, call_info_report_parsed(final_info));

    EXPECT_FALSE(endpoint.child_by_type(INFO_GET_GROUP).is_valid());
    EXPECT_EQ(1, endpoint.child_by_type(DESCRIPTIONS_ACTIVE).reported<uint8_t>());
}

// The command control path retains a Properties Report frame for parameter-zero quirk handling.
TEST_F(ConfigurationCommandClassTest, PropertiesReportControlHandlerStoresAndCompletesWalk)
{
    constexpr uint8_t COMMAND_CLASS_ID           = 0x70;
    constexpr uint8_t PROPERTIES_REPORT_COMMAND  = 0x0F;
    constexpr uint8_t ONE_BYTE_SIGNED_PROPERTIES = 0x01;
    constexpr uint8_t MINIMUM_SIGNED_VALUE       = 0x80;
    constexpr uint8_t MAXIMUM_SIGNED_VALUE       = 0x7F;
    constexpr uint8_t DEFAULT_VALUE              = 0x05;
    endpoint.emplace_node(PROPERTIES_WALK_ACTIVE).set_reported<uint8_t>(1);
    const std::array<uint8_t, 11> frame {COMMAND_CLASS_ID, PROPERTIES_REPORT_COMMAND, 0, TEST_PARAMETER_1, ONE_BYTE_SIGNED_PROPERTIES, MINIMUM_SIGNED_VALUE, MAXIMUM_SIGNED_VALUE, DEFAULT_VALUE, 0, 0, 0};

    ASSERT_EQ(SL_STATUS_OK, call_control_handler(frame.data(), static_cast<uint16_t>(frame.size())));

    const auto stored_parameter = endpoint.child_by_type_and_value(PARAMETER_ID, TEST_PARAMETER_1);
    ASSERT_TRUE(stored_parameter.is_valid());
    EXPECT_EQ(-128, stored_parameter.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::min_value)).reported<int64_t>());
    EXPECT_EQ(127, stored_parameter.child_by_type(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::max_value)).reported<int64_t>());
    EXPECT_EQ(0, endpoint.child_by_type(PROPERTIES_WALK_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
}

// Spec 2026B Configuration CC v3, Configuration Name Report.
// A completed name for one parameter advances the description chain to the next valid parameter.
TEST_F(ConfigurationCommandClassTest, NameReportFrameAdvancesToNextParameter)
{
    constexpr uint8_t COMMAND_CLASS_ID       = 0x70;
    constexpr uint8_t NAME_REPORT_COMMAND    = 0x0B;
    constexpr uint8_t REPORTS_TO_FOLLOW_NONE = 0;
    constexpr uint8_t NAME_TERMINATOR        = 0;
    constexpr uint16_t FIRST_PARAMETER       = 1;
    constexpr uint16_t NEXT_PARAMETER        = 2;
    constexpr auto ACTIVE                    = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_descriptions_active);
    constexpr auto PHASE                     = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_phase);
    constexpr auto CURRENT_PARAMETER         = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_parameter);
    constexpr auto NAME_PHASE                = static_cast<uint8_t>(discovery_description_phase_t::NAME);
    auto first                               = parameter(FIRST_PARAMETER);
    first.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    auto next = parameter(NEXT_PARAMETER);
    next.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    endpoint.emplace_node(ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(PHASE).set_reported(NAME_PHASE);
    endpoint.emplace_node(CURRENT_PARAMETER).set_reported(FIRST_PARAMETER);
    const std::array<uint8_t, 7> frame {COMMAND_CLASS_ID, NAME_REPORT_COMMAND, 0, FIRST_PARAMETER, REPORTS_TO_FOLLOW_NONE, 'A', NAME_TERMINATOR};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_configuration_core::report_received_args args {endpoint, parser, nullptr};

    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_name_report_received(args));

    EXPECT_EQ(NEXT_PARAMETER, endpoint.child_by_type(CURRENT_PARAMETER).reported<uint16_t>());
    EXPECT_TRUE(endpoint.child_by_type(NAME_GET_GROUP).is_valid());
}

// Spec 2026B Configuration CC v3, Configuration Info Report.
// The final info fragment ends discovery when there are no later parameter descriptions.
TEST_F(ConfigurationCommandClassTest, InfoReportFrameCompletesDescriptionChain)
{
    constexpr uint8_t COMMAND_CLASS_ID       = 0x70;
    constexpr uint8_t INFO_REPORT_COMMAND    = 0x0D;
    constexpr uint8_t REPORTS_TO_FOLLOW_NONE = 0;
    constexpr uint8_t INFO_TERMINATOR        = 0;
    constexpr uint16_t PARAMETER_NUMBER      = 1;
    constexpr auto ACTIVE                    = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_descriptions_active);
    constexpr auto PHASE                     = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_phase);
    constexpr auto CURRENT_PARAMETER         = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_parameter);
    constexpr auto INFO_PHASE                = static_cast<uint8_t>(discovery_description_phase_t::INFO);
    endpoint.emplace_node(ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(PHASE).set_reported(INFO_PHASE);
    endpoint.emplace_node(CURRENT_PARAMETER).set_reported(PARAMETER_NUMBER);
    const std::array<uint8_t, 7> frame {COMMAND_CLASS_ID, INFO_REPORT_COMMAND, 0, PARAMETER_NUMBER, REPORTS_TO_FOLLOW_NONE, 'I', INFO_TERMINATOR};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_configuration_core::report_received_args args {endpoint, parser, nullptr};

    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_info_report_received(args));

    EXPECT_EQ(0, endpoint.child_by_type(ACTIVE).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
}

// Spec 2026B Configuration CC v3, Configuration Info Report.
// A completed info report requests the next parameter in the description chain.
TEST_F(ConfigurationCommandClassTest, InfoReportFrameRequestsNextParameterDescription)
{
    constexpr uint8_t COMMAND_CLASS_ID       = 0x70;
    constexpr uint8_t INFO_REPORT_COMMAND    = 0x0D;
    constexpr uint8_t REPORTS_TO_FOLLOW_NONE = 0;
    constexpr uint8_t INFO_TERMINATOR        = 0;
    constexpr uint16_t CURRENT_PARAMETER     = 1;
    constexpr uint16_t NEXT_PARAMETER        = 2;
    constexpr auto ACTIVE                    = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_descriptions_active);
    constexpr auto PHASE                     = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_phase);
    constexpr auto CURRENT                   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::discovery_description_parameter);
    constexpr auto INFO_PHASE                = static_cast<uint8_t>(discovery_description_phase_t::INFO);
    parameter(NEXT_PARAMETER).emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    endpoint.emplace_node(ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(PHASE).set_reported(INFO_PHASE);
    endpoint.emplace_node(CURRENT).set_reported(CURRENT_PARAMETER);
    const std::array<uint8_t, 7> frame {COMMAND_CLASS_ID, INFO_REPORT_COMMAND, 0, CURRENT_PARAMETER, REPORTS_TO_FOLLOW_NONE, 'I', INFO_TERMINATOR};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_configuration_core::report_received_args args {endpoint, parser, nullptr};

    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_info_report_received(args));

    EXPECT_EQ(NEXT_PARAMETER, endpoint.child_by_type(CURRENT).reported<uint16_t>());
    EXPECT_EQ(NEXT_PARAMETER, endpoint.child_by_type(INFO_GET_GROUP).child_by_type(static_cast<attribute_store_type_t>(configuration_info_get_group_attributes_t::parameter_number)).desired<uint16_t>());
}

// Spec 2026B Configuration CC v2, Configuration Bulk Report.
// A valid bulk response is accepted and creates the resolver group that the response completes.
TEST_F(ConfigurationCommandClassTest, BulkReportFrameStoresValues)
{
    constexpr uint8_t COMMAND_CLASS_ID       = 0x70;
    constexpr uint8_t BULK_REPORT_COMMAND    = 0x09;
    constexpr uint8_t REPORTS_TO_FOLLOW_NONE = 0;
    constexpr uint8_t ONE_BYTE_VALUES        = 1;
    constexpr uint8_t FIRST_VALUE            = 7;
    constexpr uint8_t SECOND_VALUE           = 8;
    const std::array<uint8_t, 9> frame {COMMAND_CLASS_ID, BULK_REPORT_COMMAND, 0, TEST_PARAMETER_1, 2, REPORTS_TO_FOLLOW_NONE, ONE_BYTE_VALUES, FIRST_VALUE, SECOND_VALUE};
    zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
    command_class_configuration_core::report_received_args args {endpoint, parser, nullptr};

    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_bulk_report_received(args));

    EXPECT_TRUE(endpoint.child_by_type(BULK_GET_GROUP).is_valid());
}

// Spec 2026B Configuration CC v2 Bulk Report (CC:0070.02.09.11.010 / CC:0070.02.09.11.011).
// The control handler parses each value at the advertised common size and stores the range.
TEST_F(ConfigurationCommandClassTest, BulkReportControlHandlerStoresEveryParameter)
{
    constexpr uint8_t COMMAND_CLASS_ID       = 0x70;
    constexpr uint8_t BULK_REPORT_COMMAND    = 0x09;
    constexpr uint8_t REPORTS_TO_FOLLOW_NONE = 0;
    constexpr uint8_t ONE_BYTE_VALUES        = 1;
    constexpr uint8_t FIRST_VALUE            = 7;
    constexpr uint8_t SECOND_VALUE           = 8;
    const std::array<uint8_t, 9> frame {COMMAND_CLASS_ID, BULK_REPORT_COMMAND, 0, TEST_PARAMETER_1, 2, REPORTS_TO_FOLLOW_NONE, ONE_BYTE_VALUES, FIRST_VALUE, SECOND_VALUE};

    ASSERT_EQ(SL_STATUS_OK, call_control_handler(frame.data(), static_cast<uint16_t>(frame.size())));

    const auto first_parameter  = endpoint.child_by_type_and_value(PARAMETER_ID, TEST_PARAMETER_1);
    const auto second_parameter = endpoint.child_by_type_and_value(PARAMETER_ID, TEST_PARAMETER_2);
    ASSERT_TRUE(first_parameter.is_valid());
    ASSERT_TRUE(second_parameter.is_valid());
    EXPECT_EQ(FIRST_VALUE, first_parameter.child_by_type(PARAMETER_VALUE).reported<int64_t>());
    EXPECT_EQ(SECOND_VALUE, second_parameter.child_by_type(PARAMETER_VALUE).reported<int64_t>());
}

// CC:0070.02.09.11.007 / CC:0070.02.09.11.008 / CC:0070.02.09.11.00A — Default and Handshake flags follow the v2 Bulk Report requirements.
TEST_F(ConfigurationCommandClassTest, BulkReportControlHandlerStoresPropertiesFlags)
{
    constexpr uint8_t COMMAND_CLASS_ID       = 0x70;
    constexpr uint8_t BULK_REPORT_COMMAND    = 0x09;
    constexpr uint8_t REPORTS_TO_FOLLOW_NONE = 0;
    constexpr uint8_t ONE_BYTE_SIZE_MASK     = 0x01;
    constexpr uint8_t HANDSHAKE_SET_MASK     = 0x40;
    constexpr uint8_t DEFAULT_SET_MASK       = 0x80;
    constexpr uint8_t SIZE_HANDSHAKE_DEFAULT = ONE_BYTE_SIZE_MASK | HANDSHAKE_SET_MASK | DEFAULT_SET_MASK;
    constexpr uint8_t REPORTED_VALUE         = 7;
    constexpr auto BULK_REPORT_GROUP         = static_cast<attribute_store_type_t>(configuration_bulk_report_group_attributes_t::CONFIGURATION_BULK_REPORT_GROUP);
    constexpr auto HANDSHAKE_ATTRIBUTE       = static_cast<attribute_store_type_t>(configuration_bulk_report_group_attributes_t::handshake);
    constexpr auto DEFAULT_ATTRIBUTE         = static_cast<attribute_store_type_t>(configuration_bulk_report_group_attributes_t::default_flag);
    // Bulk Report uses a two-byte offset; size 1, handshake 1, default 1; then one value byte.
    const std::array<uint8_t, 8> frame {COMMAND_CLASS_ID, BULK_REPORT_COMMAND, 0, TEST_PARAMETER_1, 1, REPORTS_TO_FOLLOW_NONE, SIZE_HANDSHAKE_DEFAULT, REPORTED_VALUE};

    ASSERT_EQ(SL_STATUS_OK, call_control_handler(frame.data(), static_cast<uint16_t>(frame.size())));

    // The report records both flags and stores the supplied value for parameter one.
    const auto group = endpoint.child_by_type(BULK_REPORT_GROUP);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(1, group.child_by_type(HANDSHAKE_ATTRIBUTE).reported<uint8_t>());
    EXPECT_EQ(1, group.child_by_type(DEFAULT_ATTRIBUTE).reported<uint8_t>());
    EXPECT_EQ(REPORTED_VALUE, endpoint.child_by_type_and_value(PARAMETER_ID, TEST_PARAMETER_1).child_by_type(PARAMETER_VALUE).reported<int64_t>());
}

// CC:0070.02.09.11.00F — Reserved size encodings are ignored by a v2 receiver.
// A Bulk Report with a reserved size is ignored without creating parameter values.
TEST_F(ConfigurationCommandClassTest, BulkReportIgnoresReservedSize)
{
    constexpr uint8_t COMMAND_CLASS_ID       = 0x70;
    constexpr uint8_t BULK_REPORT_COMMAND    = 0x09;
    constexpr uint8_t REPORTS_TO_FOLLOW_NONE = 0;
    constexpr uint8_t RESERVED_SIZE          = 3;
    const std::array<uint8_t, 7> frame {COMMAND_CLASS_ID, BULK_REPORT_COMMAND, 0, TEST_PARAMETER_1, 1, REPORTS_TO_FOLLOW_NONE, RESERVED_SIZE};

    ASSERT_EQ(SL_STATUS_OK, call_control_handler(frame.data(), static_cast<uint16_t>(frame.size())));

    EXPECT_FALSE(endpoint.child_by_type_and_value(PARAMETER_ID, TEST_PARAMETER_1).is_valid());
}

// Truncated Bulk Report frames are rejected by the specialized control handler.
TEST_F(ConfigurationCommandClassTest, TruncatedBulkReportControlFrameFails)
{
    constexpr uint8_t COMMAND_CLASS_ID    = 0x70;
    constexpr uint8_t BULK_REPORT_COMMAND = 0x09;
    const std::array<uint8_t, 2> frame {COMMAND_CLASS_ID, BULK_REPORT_COMMAND};

    EXPECT_EQ(SL_STATUS_FAIL, call_control_handler(frame.data(), static_cast<uint16_t>(frame.size())));
}

// Unknown control commands use the generated parser fallback and return not supported.
TEST_F(ConfigurationCommandClassTest, UnknownControlFrameReturnsNotSupported)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t UNKNOWN_COMMAND  = 0xFF;
    const std::array<uint8_t, 2> frame {COMMAND_CLASS_ID, UNKNOWN_COMMAND};

    EXPECT_EQ(SL_STATUS_NOT_SUPPORTED, call_control_handler(frame.data(), static_cast<uint16_t>(frame.size())));
}

// Spec 2026B Configuration CC v1, configuration parameter discovery behavior.
// Reaching the end of the size scan starts the default-bit probe for two known parameters.
TEST_F(ConfigurationCommandClassTest, FinalSizeProbeStartsDefaultBitProbe)
{
    constexpr uint8_t LAST_V1_PARAMETER = 255;
    constexpr auto SCAN_PHASE           = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase);
    constexpr auto SCAN_REMAINING       = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining);
    constexpr auto PROBE_FIRST          = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first);
    constexpr auto PROBE_SECOND         = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second);
    constexpr auto DEFAULT_READ_PHASE   = static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS);
    auto first                          = parameter(TEST_PARAMETER_1);
    first.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    auto second = parameter(TEST_PARAMETER_2);
    second.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    endpoint.emplace_node(PARAMETER_SCAN_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(PARAMETER_SCAN_NUMBER).set_reported(LAST_V1_PARAMETER);
    endpoint.emplace_node(PARAMETER_SCAN_SIZE_INDEX).set_reported<uint8_t>(0);

    command_class_configuration::advance_parameter_scan(endpoint, true);

    EXPECT_EQ(DEFAULT_READ_PHASE, endpoint.child_by_type(SCAN_PHASE).reported<uint8_t>());
    EXPECT_EQ(2, endpoint.child_by_type(SCAN_REMAINING).reported<uint8_t>());
    EXPECT_EQ(TEST_PARAMETER_1, endpoint.child_by_type(PROBE_FIRST).reported<uint8_t>());
    EXPECT_EQ(TEST_PARAMETER_2, endpoint.child_by_type(PROBE_SECOND).reported<uint8_t>());
}

// Spec 2026B Configuration CC v4, Properties Report metadata.
// Version 4 skips the legacy default-bit probe after the final parameter scan probe.
TEST_F(ConfigurationCommandClassTest, FinalSizeProbeSkipsDefaultBitProbeForVersionFour)
{
    constexpr uint8_t LAST_V1_PARAMETER = 255;
    endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_CONFIGURATION)).set_reported(CONFIGURATION_VERSION_4);
    endpoint.emplace_node(PARAMETER_SCAN_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(PARAMETER_SCAN_NUMBER).set_reported(LAST_V1_PARAMETER);
    endpoint.emplace_node(PARAMETER_SCAN_SIZE_INDEX).set_reported<uint8_t>(0);

    command_class_configuration::advance_parameter_scan(endpoint, true);

    EXPECT_EQ(0, endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
}

// Spec 2026B Configuration CC v1, Configuration Report.
// Reports for both probe parameters capture their defaults and transition into the partial-reset phase.
TEST_F(ConfigurationCommandClassTest, DefaultProbeReportsCaptureDefaultsAndReset)
{
    constexpr uint8_t COMMAND_CLASS_ID      = 0x70;
    constexpr uint8_t REPORT_COMMAND        = 0x06;
    constexpr uint8_t REPORT_LEVEL_ONE_BYTE = 1;
    constexpr uint8_t FIRST_DEFAULT         = 4;
    constexpr uint8_t SECOND_DEFAULT        = 5;
    constexpr auto SCAN_PHASE               = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase);
    constexpr auto SCAN_REMAINING           = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining);
    constexpr auto PROBE_FIRST              = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first);
    constexpr auto PROBE_SECOND             = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second);
    constexpr auto DEFAULT_VALUE            = static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value);
    constexpr auto PARTIAL_RESET_PHASE      = static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_PARTIAL_RESET);
    auto first                              = parameter(TEST_PARAMETER_1);
    first.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    auto second = parameter(TEST_PARAMETER_2);
    second.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    endpoint.emplace_node(PARAMETER_SCAN_ACTIVE).set_reported<uint8_t>(1);
    endpoint.emplace_node(SCAN_PHASE).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS));
    endpoint.emplace_node(SCAN_REMAINING).set_reported<uint8_t>(2);
    endpoint.emplace_node(PROBE_FIRST).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PROBE_SECOND).set_reported(TEST_PARAMETER_2);

    // Each payload is a v1 Report: parameter number, one-byte size, then the default value.
    const std::array<uint8_t, 5> first_frame {COMMAND_CLASS_ID, REPORT_COMMAND, TEST_PARAMETER_1, REPORT_LEVEL_ONE_BYTE, FIRST_DEFAULT};
    zwave_frame_parser first_parser {first_frame.data(), static_cast<uint16_t>(first_frame.size())};
    command_class_configuration_core::report_received_args first_args {endpoint, first_parser, nullptr};
    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_report_received(first_args));

    const std::array<uint8_t, 5> second_frame {COMMAND_CLASS_ID, REPORT_COMMAND, TEST_PARAMETER_2, REPORT_LEVEL_ONE_BYTE, SECOND_DEFAULT};
    zwave_frame_parser second_parser {second_frame.data(), static_cast<uint16_t>(second_frame.size())};
    command_class_configuration_core::report_received_args second_args {endpoint, second_parser, nullptr};
    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_report_received(second_args));

    EXPECT_EQ(FIRST_DEFAULT, first.child_by_type(DEFAULT_VALUE).reported<int64_t>());
    EXPECT_EQ(SECOND_DEFAULT, second.child_by_type(DEFAULT_VALUE).reported<int64_t>());
    EXPECT_EQ(PARTIAL_RESET_PHASE, endpoint.child_by_type(SCAN_PHASE).reported<uint8_t>());
    EXPECT_EQ(2, endpoint.child_by_type(SCAN_REMAINING).reported<uint8_t>());
}

// Spec 2026B Configuration CC v1 default probing.
// Equal read-back values after a partial reset mark that the device resets every parameter.
TEST_F(ConfigurationCommandClassTest, PartialResetProbeDetectsGlobalDefaultReset)
{
    constexpr uint8_t ONE_BYTE_SIZE      = 1;
    constexpr uint8_t FIRST_DEFAULT      = 4;
    constexpr uint8_t SECOND_DEFAULT     = 5;
    constexpr auto SCAN_PHASE            = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase);
    constexpr auto SCAN_REMAINING        = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining);
    constexpr auto PROBE_FIRST           = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first);
    constexpr auto PROBE_SECOND          = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second);
    constexpr auto DEFAULT_VALUE         = static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value);
    constexpr auto VALUE                 = static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::value);
    constexpr auto RESET_EVERY_PARAMETER = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::default_resets_every_parameter);
    auto first                           = parameter(TEST_PARAMETER_1);
    first.emplace_node(PARAMETER_SIZE).set_reported(ONE_BYTE_SIZE);
    first.emplace_node(DEFAULT_VALUE).set_reported<int64_t>(FIRST_DEFAULT);
    auto second = parameter(TEST_PARAMETER_2);
    second.emplace_node(PARAMETER_SIZE).set_reported(ONE_BYTE_SIZE);
    second.emplace_node(DEFAULT_VALUE).set_reported<int64_t>(SECOND_DEFAULT);
    first.emplace_node(VALUE).set_reported<int64_t>(FIRST_DEFAULT);
    second.emplace_node(VALUE).set_reported<int64_t>(SECOND_DEFAULT);
    endpoint.emplace_node(SCAN_PHASE).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_PARTIAL_RESET));
    endpoint.emplace_node(SCAN_REMAINING).set_reported<uint8_t>(2);
    endpoint.emplace_node(PROBE_FIRST).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PROBE_SECOND).set_reported(TEST_PARAMETER_2);
    call_default_probe_report(TEST_PARAMETER_1, FIRST_DEFAULT);
    call_default_probe_report(TEST_PARAMETER_2, SECOND_DEFAULT);

    EXPECT_EQ(1, endpoint.child_by_type(RESET_EVERY_PARAMETER).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
}

// Spec 2026B Configuration CC v1 default probing.
// A parameter that does not return to its default disproves global default reset behavior.
TEST_F(ConfigurationCommandClassTest, PartialResetProbeDetectsParameterSpecificReset)
{
    constexpr uint8_t ONE_BYTE_SIZE      = 1;
    constexpr uint8_t DEFAULT_VALUE      = 5;
    constexpr uint8_t READ_BACK_VALUE    = 6;
    constexpr auto SCAN_PHASE            = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase);
    constexpr auto SCAN_REMAINING        = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining);
    constexpr auto PROBE_FIRST           = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first);
    constexpr auto PROBE_SECOND          = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second);
    constexpr auto STORED_DEFAULT        = static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value);
    constexpr auto RESET_EVERY_PARAMETER = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::default_resets_every_parameter);
    for (const auto id: {TEST_PARAMETER_1, TEST_PARAMETER_2}) {
        auto parameter_node = parameter(id);
        parameter_node.emplace_node(PARAMETER_SIZE).set_reported(ONE_BYTE_SIZE);
        parameter_node.emplace_node(STORED_DEFAULT).set_reported<int64_t>(DEFAULT_VALUE);
    }
    endpoint.emplace_node(SCAN_PHASE).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_PARTIAL_RESET));
    endpoint.emplace_node(SCAN_REMAINING).set_reported<uint8_t>(2);
    endpoint.emplace_node(PROBE_FIRST).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PROBE_SECOND).set_reported(TEST_PARAMETER_2);
    parameter(TEST_PARAMETER_1).emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::value)).set_reported<int64_t>(DEFAULT_VALUE);
    parameter(TEST_PARAMETER_2).emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::value)).set_reported<int64_t>(READ_BACK_VALUE);
    call_default_probe_report(TEST_PARAMETER_1, DEFAULT_VALUE);
    call_default_probe_report(TEST_PARAMETER_2, READ_BACK_VALUE);

    EXPECT_EQ(0, endpoint.child_by_type(RESET_EVERY_PARAMETER).reported<uint8_t>());
}

// A late default Report without active probe state is ignored safely.
TEST_F(ConfigurationCommandClassTest, DefaultProbeIgnoresReportWithoutPhase)
{
    call_default_probe_report(TEST_PARAMETER_1, 5);

    EXPECT_FALSE(endpoint.child_by_type(DISCOVERY_COMPLETE).reported_exists());
}

// A default probe finishes discovery if its parameter metadata disappears before all reports arrive.
TEST_F(ConfigurationCommandClassTest, DefaultProbeCompletesWhenMetadataIsIncomplete)
{
    constexpr auto SCAN_PHASE     = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase);
    constexpr auto SCAN_REMAINING = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining);
    constexpr auto PROBE_FIRST    = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first);
    constexpr auto PROBE_SECOND   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second);
    parameter(TEST_PARAMETER_1).emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    endpoint.emplace_node(SCAN_PHASE).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS));
    endpoint.emplace_node(SCAN_REMAINING).set_reported<uint8_t>(1);
    endpoint.emplace_node(PROBE_FIRST).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PROBE_SECOND).set_reported(TEST_PARAMETER_2);

    call_default_probe_report(TEST_PARAMETER_1, 5);

    EXPECT_EQ(0, endpoint.child_by_type(PARAMETER_SCAN_ACTIVE).reported<uint8_t>());
    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
}

// A late default report at zero remaining count still completes the defaults phase safely.
TEST_F(ConfigurationCommandClassTest, DefaultProbeHandlesZeroRemainingCount)
{
    constexpr auto SCAN_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase);
    constexpr auto PROBE_FIRST  = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first);
    constexpr auto PROBE_SECOND = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second);
    constexpr auto REMAINING    = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining);
    auto first                  = parameter(TEST_PARAMETER_1);
    first.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    auto second = parameter(TEST_PARAMETER_2);
    second.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    second.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value)).set_reported<int64_t>(0);
    endpoint.emplace_node(SCAN_PHASE).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS));
    endpoint.emplace_node(PROBE_FIRST).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PROBE_SECOND).set_reported(TEST_PARAMETER_2);
    endpoint.emplace_node(REMAINING).set_reported<uint8_t>(0);

    call_default_probe_report(TEST_PARAMETER_1, 0);

    EXPECT_EQ(2, endpoint.child_by_type(REMAINING).reported<uint8_t>());
    EXPECT_EQ(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_PARTIAL_RESET), endpoint.child_by_type(SCAN_PHASE).reported<uint8_t>());
}

// A Defaults Report cannot infer reset behavior when the first probe size is unreported.
TEST_F(ConfigurationCommandClassTest, DefaultProbeStopsWhenFirstSizeIsUnreported)
{
    constexpr auto SCAN_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase);
    constexpr auto PROBE_FIRST  = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first);
    constexpr auto PROBE_SECOND = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second);
    constexpr auto REMAINING    = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining);
    auto first                  = parameter(TEST_PARAMETER_1);
    first.emplace_node(PARAMETER_SIZE).set_desired(TEST_SIZE_ONE_BYTE);
    auto second = parameter(TEST_PARAMETER_2);
    second.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    endpoint.emplace_node(SCAN_PHASE).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS));
    endpoint.emplace_node(PROBE_FIRST).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PROBE_SECOND).set_reported(TEST_PARAMETER_2);
    endpoint.emplace_node(REMAINING).set_reported<uint8_t>(1);

    call_default_probe_report(TEST_PARAMETER_2, 0);

    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
}

// A Defaults Report cannot infer reset behavior when the second probe size is unreported.
TEST_F(ConfigurationCommandClassTest, DefaultProbeStopsWhenSecondSizeIsUnreported)
{
    constexpr auto SCAN_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase);
    constexpr auto PROBE_FIRST  = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first);
    constexpr auto PROBE_SECOND = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second);
    constexpr auto REMAINING    = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining);
    auto first                  = parameter(TEST_PARAMETER_1);
    first.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    auto second = parameter(TEST_PARAMETER_2);
    second.emplace_node(PARAMETER_SIZE).set_desired(TEST_SIZE_ONE_BYTE);
    endpoint.emplace_node(SCAN_PHASE).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS));
    endpoint.emplace_node(PROBE_FIRST).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PROBE_SECOND).set_reported(TEST_PARAMETER_2);
    endpoint.emplace_node(REMAINING).set_reported<uint8_t>(1);

    call_default_probe_report(TEST_PARAMETER_1, 0);

    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
}

// A Defaults Report does not infer reset behavior when the first default is only desired.
TEST_F(ConfigurationCommandClassTest, DefaultProbeStopsWhenFirstDefaultIsUnreported)
{
    constexpr auto SCAN_PHASE   = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_phase);
    constexpr auto PROBE_FIRST  = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_first);
    constexpr auto PROBE_SECOND = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_probe_second);
    constexpr auto REMAINING    = static_cast<attribute_store_type_t>(configuration_endpoint_attributes_t::parameter_scan_reports_remaining);
    auto first                  = parameter(TEST_PARAMETER_1);
    first.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    first.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::default_value)).set_desired<int64_t>(0);
    auto second = parameter(TEST_PARAMETER_2);
    second.emplace_node(PARAMETER_SIZE).set_reported(TEST_SIZE_ONE_BYTE);
    endpoint.emplace_node(SCAN_PHASE).set_reported(static_cast<uint8_t>(parameter_scan_phase_t::DEFAULT_PROBE_READ_DEFAULTS));
    endpoint.emplace_node(PROBE_FIRST).set_reported(TEST_PARAMETER_1);
    endpoint.emplace_node(PROBE_SECOND).set_reported(TEST_PARAMETER_2);
    endpoint.emplace_node(REMAINING).set_reported<uint8_t>(1);

    call_default_probe_report(TEST_PARAMETER_2, 0);

    EXPECT_EQ(1, endpoint.child_by_type(DISCOVERY_COMPLETE).reported<uint8_t>());
}

// Spec 2026B Configuration CC v1, Configuration Get.
// The request assembler emits the command header followed by the requested parameter number.
TEST_F(ConfigurationCommandClassTest, ConfigurationGetAssemblerBuildsFrame)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t GET_COMMAND      = 0x05;
    auto group                         = endpoint.emplace_node(GET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_get_group_attributes_t::parameter_number)).set_desired(TEST_PARAMETER_1);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(GET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::get_requested_args args {group, &generator};

    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_get_requested(args, frame.data(), &frame_length));

    EXPECT_EQ((std::vector<uint8_t> {COMMAND_CLASS_ID, GET_COMMAND, TEST_PARAMETER_1}), std::vector<uint8_t>(frame.begin(), frame.begin() + frame_length));
}

// Spec 2026B Configuration CC v1, Configuration Get. A group with no desired parameter is not ready to send.
TEST_F(ConfigurationCommandClassTest, ConfigurationGetRejectsMissingParameter)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t GET_COMMAND      = 0x05;
    auto group                         = endpoint.emplace_node(GET_GROUP);
    auto parameter_node                = group.emplace_node(static_cast<attribute_store_type_t>(configuration_get_group_attributes_t::parameter_number));
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(GET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::get_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_configuration_get_requested(args, frame.data(), &frame_length));
    EXPECT_FALSE(parameter_node.desired_exists());
}

// Spec 2026B Configuration CC v3, Configuration Name Get. A group with no desired parameter is not ready to send.
TEST_F(ConfigurationCommandClassTest, ConfigurationNameGetRejectsMissingParameter)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t NAME_GET_COMMAND = 0x0A;
    auto group                         = endpoint.emplace_node(NAME_GET_GROUP);
    auto parameter_node                = group.emplace_node(static_cast<attribute_store_type_t>(configuration_name_get_group_attributes_t::parameter_number));
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(NAME_GET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::get_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_configuration_name_get_requested(args, frame.data(), &frame_length));
    EXPECT_FALSE(parameter_node.desired_exists());
}

// Spec 2026B Configuration CC v3, Configuration Info Get. A group with no desired parameter is not ready to send.
TEST_F(ConfigurationCommandClassTest, ConfigurationInfoGetRejectsMissingParameter)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t INFO_GET_COMMAND = 0x0C;
    auto group                         = endpoint.emplace_node(INFO_GET_GROUP);
    auto parameter_node                = group.emplace_node(static_cast<attribute_store_type_t>(configuration_info_get_group_attributes_t::parameter_number));
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(INFO_GET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::get_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_configuration_info_get_requested(args, frame.data(), &frame_length));
    EXPECT_FALSE(parameter_node.desired_exists());
}

// Spec 2026B Configuration CC v3, Configuration Properties Get. A group with no desired parameter is not ready to send.
TEST_F(ConfigurationCommandClassTest, ConfigurationPropertiesGetRejectsMissingParameter)
{
    constexpr uint8_t COMMAND_CLASS_ID       = 0x70;
    constexpr uint8_t PROPERTIES_GET_COMMAND = 0x0E;
    auto group                               = endpoint.emplace_node(PROPERTIES_GET_GROUP);
    auto parameter_node                      = group.emplace_node(static_cast<attribute_store_type_t>(configuration_properties_get_group_attributes_t::parameter_number));
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(PROPERTIES_GET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::get_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_configuration_properties_get_requested(args, frame.data(), &frame_length));
    EXPECT_FALSE(parameter_node.desired_exists());
}

// Spec 2026B Configuration CC v1, Configuration Set.
// A valid set request includes parameter, size, default flag, and the exact value bytes.
TEST_F(ConfigurationCommandClassTest, ConfigurationSetAssemblerBuildsFrame)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t SET_COMMAND      = 0x04;
    auto group                         = endpoint.emplace_node(SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::parameter_number)).set_desired(TEST_PARAMETER_1);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::size)).set_desired(TEST_SIZE_ONE_BYTE);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::default_flag)).set_desired<uint8_t>(0);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::configuration_value)).set_desired(std::vector<uint8_t> {7});
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_set_requested(args, frame.data(), &frame_length));

    // Size 1 in the low 3 bits, Default clear, then the one value byte.
    EXPECT_EQ((std::vector<uint8_t> {COMMAND_CLASS_ID, SET_COMMAND, TEST_PARAMETER_1, TEST_SIZE_ONE_BYTE, 7}), std::vector<uint8_t>(frame.begin(), frame.begin() + frame_length));
}

// CC:0000.00.00.11.002 — Configuration Set keeps the controller frame, including Default and Size, for a version 1 peer.
TEST_F(ConfigurationCommandClassTest, ConfigurationSetKeepsFullFrameForVersionOnePeer)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t SET_COMMAND      = 0x04;
    constexpr uint8_t SET_VALUE        = 7;
    endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_CONFIGURATION)).set_reported(CONFIGURATION_VERSION_1);
    auto group = endpoint.emplace_node(SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::parameter_number)).set_desired(TEST_PARAMETER_1);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::size)).set_desired(TEST_SIZE_ONE_BYTE);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::default_flag)).set_desired<uint8_t>(0);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::configuration_value)).set_desired(std::vector<uint8_t> {SET_VALUE});
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_set_requested(args, frame.data(), &frame_length));

    EXPECT_EQ((std::vector<uint8_t> {COMMAND_CLASS_ID, SET_COMMAND, TEST_PARAMETER_1, TEST_SIZE_ONE_BYTE, SET_VALUE}), std::vector<uint8_t>(frame.begin(), frame.begin() + frame_length));
}

// CC:0070.04.04.11.001 — Configuration Set remains writable when the parameter is not advertised read-only.
TEST_F(ConfigurationCommandClassTest, ConfigurationSetAssemblerAllowsParameterWithoutReadOnlyFlag)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t SET_COMMAND      = 0x04;
    auto group                         = endpoint.emplace_node(SET_GROUP);
    parameter(TEST_PARAMETER_1);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::parameter_number)).set_desired(TEST_PARAMETER_1);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::size)).set_desired(TEST_SIZE_ONE_BYTE);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::default_flag)).set_desired<uint8_t>(0);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::configuration_value)).set_desired(std::vector<uint8_t> {7});
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_set_requested(args, frame.data(), &frame_length));

    EXPECT_EQ((std::vector<uint8_t> {COMMAND_CLASS_ID, SET_COMMAND, TEST_PARAMETER_1, TEST_SIZE_ONE_BYTE, 7}), std::vector<uint8_t>(frame.begin(), frame.begin() + frame_length));
}

// Configuration Set encodes a default request with zero bytes matching the reported parameter width.
TEST_F(ConfigurationCommandClassTest, ConfigurationSetAssemblerBuildsDefaultFrame)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t SET_COMMAND      = 0x04;
    auto group                         = endpoint.emplace_node(SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::parameter_number)).set_desired(TEST_PARAMETER_1);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::size)).set_desired(TEST_SIZE_TWO_BYTES);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::default_flag)).set_desired<uint8_t>(1);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_set_requested(args, frame.data(), &frame_length));

    // Default is bit 7. Size 2 occupies the low 3 bits, so the properties byte is 0x80 | 2, followed by two zero value bytes.
    constexpr uint8_t DEFAULT_SIZE_TWO = static_cast<uint8_t>(0x80 | TEST_SIZE_TWO_BYTES);
    EXPECT_EQ((std::vector<uint8_t> {COMMAND_CLASS_ID, SET_COMMAND, TEST_PARAMETER_1, DEFAULT_SIZE_TWO, 0, 0}), std::vector<uint8_t>(frame.begin(), frame.begin() + frame_length));
}

// Spec 2026B Configuration CC v1, Configuration Set.
// A read-only parameter is rejected before a Set frame is generated.
TEST_F(ConfigurationCommandClassTest, ConfigurationSetAssemblerRejectsReadOnlyParameter)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t SET_COMMAND      = 0x04;
    auto known_parameter               = parameter(TEST_PARAMETER_1);
    known_parameter.emplace_node(static_cast<attribute_store_type_t>(configuration_parameter_attributes_t::read_only)).set_reported<uint8_t>(1);
    auto group = endpoint.emplace_node(SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::parameter_number)).set_desired(TEST_PARAMETER_1);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::size)).set_desired(TEST_SIZE_ONE_BYTE);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::default_flag)).set_desired<uint8_t>(0);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::configuration_value)).set_desired(std::vector<uint8_t> {7});
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_ALREADY_EXISTS, component().on_configuration_set_requested(args, frame.data(), &frame_length));
}

// Spec 2026B Configuration CC v1, Configuration Set. A group with no parameter, size, or default flag is not ready.
TEST_F(ConfigurationCommandClassTest, ConfigurationSetRejectsEmptyGroup)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t SET_COMMAND      = 0x04;
    auto group                         = endpoint.emplace_node(SET_GROUP);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_configuration_set_requested(args, frame.data(), &frame_length));
}

// Spec 2026B Configuration CC v1, Configuration Set. A parameter without a size is not ready.
TEST_F(ConfigurationCommandClassTest, ConfigurationSetRejectsMissingSize)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t SET_COMMAND      = 0x04;
    auto group                         = endpoint.emplace_node(SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::parameter_number)).set_desired(TEST_PARAMETER_1);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_configuration_set_requested(args, frame.data(), &frame_length));
}

// Spec 2026B Configuration CC v1, Configuration Set. A supported size still needs the Default flag before a frame is built.
TEST_F(ConfigurationCommandClassTest, ConfigurationSetRejectsMissingDefaultFlag)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t SET_COMMAND      = 0x04;
    auto group                         = endpoint.emplace_node(SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::parameter_number)).set_desired(TEST_PARAMETER_1);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::size)).set_desired(TEST_SIZE_ONE_BYTE);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_configuration_set_requested(args, frame.data(), &frame_length));
}

// Spec 2026B Configuration CC v1, Configuration Set. Size 0 together with a Default flag is rejected.
TEST_F(ConfigurationCommandClassTest, ConfigurationSetRejectsZeroSizeWithDefaultFlag)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t SET_COMMAND      = 0x04;
    constexpr uint8_t UNSUPPORTED_SIZE = 0;
    auto group                         = endpoint.emplace_node(SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::parameter_number)).set_desired(TEST_PARAMETER_1);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::size)).set_desired(UNSUPPORTED_SIZE);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::default_flag)).set_desired<uint8_t>(0);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_ALREADY_EXISTS, component().on_configuration_set_requested(args, frame.data(), &frame_length));
}

// Spec 2026B Configuration CC v1, Configuration Set. A non-default set needs value bytes.
TEST_F(ConfigurationCommandClassTest, ConfigurationSetRejectsMissingValue)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t SET_COMMAND      = 0x04;
    auto group                         = endpoint.emplace_node(SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::parameter_number)).set_desired(TEST_PARAMETER_1);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::size)).set_desired(TEST_SIZE_ONE_BYTE);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::default_flag)).set_desired<uint8_t>(0);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_configuration_set_requested(args, frame.data(), &frame_length));
}

// Spec 2026B Configuration CC v1, Configuration Set. The value length must match Size.
TEST_F(ConfigurationCommandClassTest, ConfigurationSetRejectsValueLengthMismatch)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t SET_COMMAND      = 0x04;
    auto group                         = endpoint.emplace_node(SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::parameter_number)).set_desired(TEST_PARAMETER_1);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::size)).set_desired(TEST_SIZE_ONE_BYTE);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::default_flag)).set_desired<uint8_t>(0);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_set_group_attributes_t::configuration_value)).set_desired(std::vector<uint8_t> {1, 2});
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_ALREADY_EXISTS, component().on_configuration_set_requested(args, frame.data(), &frame_length));
}

// Spec 2026B Configuration CC v3, Configuration Name Get.
// The request assembler writes a two-byte parameter number in network byte order.
TEST_F(ConfigurationCommandClassTest, ConfigurationNameGetAssemblerBuildsFrame)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t NAME_GET_COMMAND = 0x0A;
    auto group                         = endpoint.emplace_node(NAME_GET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_name_get_group_attributes_t::parameter_number)).set_desired(TEST_PARAMETER_OFFSET);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(NAME_GET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::get_requested_args args {group, &generator};

    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_name_get_requested(args, frame.data(), &frame_length));

    EXPECT_EQ((std::vector<uint8_t> {COMMAND_CLASS_ID, NAME_GET_COMMAND, 0x01, 0x02}), std::vector<uint8_t>(frame.begin(), frame.begin() + frame_length));
}

// Spec 2026B Configuration CC v3, Configuration Info Get.
// The assembler appends the parameter ID as a two-byte value after its command header.
TEST_F(ConfigurationCommandClassTest, ConfigurationInfoGetAssemblerBuildsFrame)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t INFO_GET_COMMAND = 0x0C;
    auto group                         = endpoint.emplace_node(INFO_GET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_info_get_group_attributes_t::parameter_number)).set_desired(TEST_PARAMETER_OFFSET);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(INFO_GET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::get_requested_args args {group, &generator};

    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_info_get_requested(args, frame.data(), &frame_length));

    EXPECT_EQ((std::vector<uint8_t> {COMMAND_CLASS_ID, INFO_GET_COMMAND, 0x01, 0x02}), std::vector<uint8_t>(frame.begin(), frame.begin() + frame_length));
}

// Spec 2026B Configuration CC v3, Configuration Properties Get.
// The assembler appends the two-byte parameter ID to the Properties Get command.
TEST_F(ConfigurationCommandClassTest, ConfigurationPropertiesGetAssemblerBuildsFrame)
{
    constexpr uint8_t COMMAND_CLASS_ID       = 0x70;
    constexpr uint8_t PROPERTIES_GET_COMMAND = 0x0E;
    auto group                               = endpoint.emplace_node(PROPERTIES_GET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_properties_get_group_attributes_t::parameter_number)).set_desired(TEST_PARAMETER_OFFSET);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(PROPERTIES_GET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::get_requested_args args {group, &generator};

    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_properties_get_requested(args, frame.data(), &frame_length));

    EXPECT_EQ((std::vector<uint8_t> {COMMAND_CLASS_ID, PROPERTIES_GET_COMMAND, 0x01, 0x02}), std::vector<uint8_t>(frame.begin(), frame.begin() + frame_length));
}

// Spec 2026B Configuration CC v2, Configuration Bulk Get.
// The assembler writes the 16-bit offset followed by the requested parameter count.
TEST_F(ConfigurationCommandClassTest, ConfigurationBulkGetAssemblerBuildsFrame)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t BULK_GET_COMMAND = 0x08;
    auto group                         = endpoint.emplace_node(BULK_GET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_get_group_attributes_t::parameter_offset)).set_desired(TEST_PARAMETER_OFFSET);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_get_group_attributes_t::number_of_parameters)).set_desired<uint8_t>(2);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(BULK_GET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::get_requested_args args {group, &generator};

    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_bulk_get_requested(args, frame.data(), &frame_length));

    EXPECT_EQ((std::vector<uint8_t> {COMMAND_CLASS_ID, BULK_GET_COMMAND, 0x01, 0x02, 2}), std::vector<uint8_t>(frame.begin(), frame.begin() + frame_length));
}

// Spec 2026B Configuration CC v2, Configuration Bulk Set.
// Valid flattened values are emitted after the shared size and handshake properties byte.
TEST_F(ConfigurationCommandClassTest, ConfigurationBulkSetAssemblerBuildsFrame)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t BULK_SET_COMMAND = 0x07;
    auto group                         = endpoint.emplace_node(BULK_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::parameter_offset)).set_desired(TEST_PARAMETER_OFFSET);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::number_of_parameters)).set_desired<uint8_t>(2);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::size)).set_desired(TEST_SIZE_ONE_BYTE);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::handshake)).set_desired<uint8_t>(0);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::default_flag)).set_desired<uint8_t>(0);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::vg)).set_desired(std::vector<uint8_t> {7, 8});
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(BULK_SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_bulk_set_requested(args, frame.data(), &frame_length));

    // Offset 258 is 0x01, 0x02. Properties byte is size 1 with Handshake and Default clear, then one byte per parameter.
    constexpr uint8_t OFFSET_MSB      = static_cast<uint8_t>(TEST_PARAMETER_OFFSET >> 8);
    constexpr uint8_t OFFSET_LSB      = static_cast<uint8_t>(TEST_PARAMETER_OFFSET & 0xFF);
    constexpr uint8_t PARAMETER_COUNT = 2;
    constexpr uint8_t FIRST_VALUE     = 7;
    constexpr uint8_t SECOND_VALUE    = 8;
    EXPECT_EQ((std::vector<uint8_t> {COMMAND_CLASS_ID, BULK_SET_COMMAND, OFFSET_MSB, OFFSET_LSB, PARAMETER_COUNT, TEST_SIZE_ONE_BYTE, FIRST_VALUE, SECOND_VALUE}), std::vector<uint8_t>(frame.begin(), frame.begin() + frame_length));
}

// CC:0000.00.00.11.002 — Bulk Set keeps the controller frame, including Handshake and Default, for a version 2 peer.
TEST_F(ConfigurationCommandClassTest, ConfigurationBulkSetKeepsFullFrameForVersionTwoPeer)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t BULK_SET_COMMAND = 0x07;
    constexpr uint8_t PARAMETER_COUNT  = 2;
    constexpr uint8_t FIRST_VALUE      = 7;
    constexpr uint8_t SECOND_VALUE     = 8;
    constexpr uint8_t OFFSET_MSB       = static_cast<uint8_t>(TEST_PARAMETER_OFFSET >> 8);
    constexpr uint8_t OFFSET_LSB       = static_cast<uint8_t>(TEST_PARAMETER_OFFSET & 0xFF);
    endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_CONFIGURATION)).set_reported(CONFIGURATION_VERSION_2);
    auto group = endpoint.emplace_node(BULK_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::parameter_offset)).set_desired(TEST_PARAMETER_OFFSET);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::number_of_parameters)).set_desired(PARAMETER_COUNT);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::size)).set_desired(TEST_SIZE_ONE_BYTE);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::handshake)).set_desired<uint8_t>(0);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::default_flag)).set_desired<uint8_t>(0);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::vg)).set_desired(std::vector<uint8_t> {FIRST_VALUE, SECOND_VALUE});
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(BULK_SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_bulk_set_requested(args, frame.data(), &frame_length));

    EXPECT_EQ((std::vector<uint8_t> {COMMAND_CLASS_ID, BULK_SET_COMMAND, OFFSET_MSB, OFFSET_LSB, PARAMETER_COUNT, TEST_SIZE_ONE_BYTE, FIRST_VALUE, SECOND_VALUE}), std::vector<uint8_t>(frame.begin(), frame.begin() + frame_length));
}

// Spec 2026B Configuration CC v2, Configuration Bulk Set.
// A default request fills the value payload with zero bytes for the requested parameter range.
TEST_F(ConfigurationCommandClassTest, ConfigurationBulkSetDefaultFillsZeroValues)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t BULK_SET_COMMAND = 0x07;
    auto group                         = endpoint.emplace_node(BULK_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::parameter_offset)).set_desired(TEST_PARAMETER_OFFSET);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::number_of_parameters)).set_desired<uint8_t>(2);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::size)).set_desired(TEST_SIZE_ONE_BYTE);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::handshake)).set_desired<uint8_t>(0);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::default_flag)).set_desired<uint8_t>(1);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(BULK_SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_bulk_set_requested(args, frame.data(), &frame_length));

    // Default is bit 7 of the properties byte. Size 1 with Default set is 0x81, then one zero byte per parameter.
    constexpr uint8_t OFFSET_MSB       = static_cast<uint8_t>(TEST_PARAMETER_OFFSET >> 8);
    constexpr uint8_t OFFSET_LSB       = static_cast<uint8_t>(TEST_PARAMETER_OFFSET & 0xFF);
    constexpr uint8_t PARAMETER_COUNT  = 2;
    constexpr uint8_t DEFAULT_SIZE_ONE = static_cast<uint8_t>(0x80 | TEST_SIZE_ONE_BYTE);
    EXPECT_EQ((std::vector<uint8_t> {COMMAND_CLASS_ID, BULK_SET_COMMAND, OFFSET_MSB, OFFSET_LSB, PARAMETER_COUNT, DEFAULT_SIZE_ONE, 0, 0}), std::vector<uint8_t>(frame.begin(), frame.begin() + frame_length));
}

// Spec 2026B Configuration CC v2, Configuration Bulk Get. Offset and count are both required.
TEST_F(ConfigurationCommandClassTest, ConfigurationBulkGetRejectsMissingOffset)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t BULK_GET_COMMAND = 0x08;
    auto group                         = endpoint.emplace_node(BULK_GET_GROUP);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(BULK_GET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::get_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_configuration_bulk_get_requested(args, frame.data(), &frame_length));
}

// Spec 2026B Configuration CC v2, Configuration Bulk Get. An offset without a count is not ready.
TEST_F(ConfigurationCommandClassTest, ConfigurationBulkGetRejectsMissingCount)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t BULK_GET_COMMAND = 0x08;
    constexpr uint16_t OFFSET          = 1;
    auto group                         = endpoint.emplace_node(BULK_GET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_get_group_attributes_t::parameter_offset)).set_desired(OFFSET);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(BULK_GET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::get_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_configuration_bulk_get_requested(args, frame.data(), &frame_length));
}

// Spec 2026B Configuration CC v2, Configuration Bulk Set. An empty group is not ready.
TEST_F(ConfigurationCommandClassTest, ConfigurationBulkSetRejectsEmptyGroup)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t BULK_SET_COMMAND = 0x07;
    auto group                         = endpoint.emplace_node(BULK_SET_GROUP);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(BULK_SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_configuration_bulk_set_requested(args, frame.data(), &frame_length));
}

// Spec 2026B Configuration CC v2, Configuration Bulk Set. An offset without a count is not ready.
TEST_F(ConfigurationCommandClassTest, ConfigurationBulkSetRejectsMissingCount)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t BULK_SET_COMMAND = 0x07;
    constexpr uint16_t OFFSET          = 1;
    auto group                         = endpoint.emplace_node(BULK_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::parameter_offset)).set_desired(OFFSET);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(BULK_SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_configuration_bulk_set_requested(args, frame.data(), &frame_length));
}

// Spec 2026B Configuration CC v2, Configuration Bulk Set. Offset and count still need a size.
TEST_F(ConfigurationCommandClassTest, ConfigurationBulkSetRejectsMissingSize)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t BULK_SET_COMMAND = 0x07;
    constexpr uint16_t OFFSET          = 1;
    constexpr uint8_t PARAMETER_COUNT  = 1;
    auto group                         = endpoint.emplace_node(BULK_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::parameter_offset)).set_desired(OFFSET);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::number_of_parameters)).set_desired(PARAMETER_COUNT);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(BULK_SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_configuration_bulk_set_requested(args, frame.data(), &frame_length));
}

// Spec 2026B Configuration CC v2, Configuration Bulk Set. Handshake and Default are required once a size is present.
TEST_F(ConfigurationCommandClassTest, ConfigurationBulkSetRejectsMissingHandshakeAndDefault)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t BULK_SET_COMMAND = 0x07;
    constexpr uint16_t OFFSET          = 1;
    constexpr uint8_t PARAMETER_COUNT  = 1;
    auto group                         = endpoint.emplace_node(BULK_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::parameter_offset)).set_desired(OFFSET);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::number_of_parameters)).set_desired(PARAMETER_COUNT);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::size)).set_desired(TEST_SIZE_ONE_BYTE);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(BULK_SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_configuration_bulk_set_requested(args, frame.data(), &frame_length));
}

// Spec 2026B Configuration CC v2, Configuration Bulk Set. Size 0 is rejected after Handshake and Default are set.
TEST_F(ConfigurationCommandClassTest, ConfigurationBulkSetRejectsZeroSizeWithFlags)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t BULK_SET_COMMAND = 0x07;
    constexpr uint16_t OFFSET          = 1;
    constexpr uint8_t PARAMETER_COUNT  = 1;
    constexpr uint8_t UNSUPPORTED_SIZE = 0;
    auto group                         = endpoint.emplace_node(BULK_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::parameter_offset)).set_desired(OFFSET);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::number_of_parameters)).set_desired(PARAMETER_COUNT);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::size)).set_desired(UNSUPPORTED_SIZE);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::handshake)).set_desired<uint8_t>(0);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::default_flag)).set_desired<uint8_t>(0);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(BULK_SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_FAIL, component().on_configuration_bulk_set_requested(args, frame.data(), &frame_length));
}

// Spec 2026B Configuration CC v2, Configuration Bulk Set (CC:0070.02.04.11.001). A zero parameter count is rejected.
TEST_F(ConfigurationCommandClassTest, ConfigurationBulkSetRejectsZeroCount)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t BULK_SET_COMMAND = 0x07;
    constexpr uint16_t OFFSET          = 1;
    constexpr uint8_t PARAMETER_COUNT  = 0;
    auto group                         = endpoint.emplace_node(BULK_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::parameter_offset)).set_desired(OFFSET);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::number_of_parameters)).set_desired(PARAMETER_COUNT);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::size)).set_desired(TEST_SIZE_ONE_BYTE);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::handshake)).set_desired<uint8_t>(0);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::default_flag)).set_desired<uint8_t>(0);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(BULK_SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_FAIL, component().on_configuration_bulk_set_requested(args, frame.data(), &frame_length));
}

// Spec 2026B Configuration CC v2, Configuration Bulk Set. A non-default set needs one value byte per parameter.
TEST_F(ConfigurationCommandClassTest, ConfigurationBulkSetRejectsMissingValues)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t BULK_SET_COMMAND = 0x07;
    constexpr uint16_t OFFSET          = 1;
    constexpr uint8_t PARAMETER_COUNT  = 1;
    auto group                         = endpoint.emplace_node(BULK_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::parameter_offset)).set_desired(OFFSET);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::number_of_parameters)).set_desired(PARAMETER_COUNT);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::size)).set_desired(TEST_SIZE_ONE_BYTE);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::handshake)).set_desired<uint8_t>(0);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::default_flag)).set_desired<uint8_t>(0);
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(BULK_SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_NOT_READY, component().on_configuration_bulk_set_requested(args, frame.data(), &frame_length));
}

// Spec 2026B Configuration CC v2, Configuration Bulk Set. The value payload length must equal count times size.
TEST_F(ConfigurationCommandClassTest, ConfigurationBulkSetRejectsValueLengthMismatch)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t BULK_SET_COMMAND = 0x07;
    constexpr uint16_t OFFSET          = 1;
    constexpr uint8_t PARAMETER_COUNT  = 1;
    auto group                         = endpoint.emplace_node(BULK_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::parameter_offset)).set_desired(OFFSET);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::number_of_parameters)).set_desired(PARAMETER_COUNT);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::size)).set_desired(TEST_SIZE_ONE_BYTE);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::handshake)).set_desired<uint8_t>(0);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::default_flag)).set_desired<uint8_t>(0);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::vg)).set_desired(std::vector<uint8_t> {1, 2});
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(BULK_SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    EXPECT_EQ(SL_STATUS_FAIL, component().on_configuration_bulk_set_requested(args, frame.data(), &frame_length));
}

// A give-up for a node that is not in the tree has no endpoint scan to advance.
TEST_F(ConfigurationCommandClassTest, GetResolutionGiveUpIgnoresMissingEndpoint)
{
    endpoint.emplace_node(PARAMETER_SCAN_NUMBER).set_reported(TEST_PARAMETER_1);

    command_class_configuration::on_configuration_get_resolution_give_up(ATTRIBUTE_STORE_INVALID_NODE);

    EXPECT_EQ(TEST_PARAMETER_1, endpoint.child_by_type(PARAMETER_SCAN_NUMBER).reported<uint8_t>());
}

// CC:0070.02.07.11.006 — Handshake 1 asks the receiver to return a Bulk Report. The controller still sends the full Bulk Set.
TEST_F(ConfigurationCommandClassTest, ConfigurationBulkSetSetsHandshakeBit)
{
    constexpr uint8_t COMMAND_CLASS_ID = 0x70;
    constexpr uint8_t BULK_SET_COMMAND = 0x07;
    constexpr uint8_t HANDSHAKE_BIT    = 0x40;  // Properties1 bit 6
    constexpr uint8_t PARAMETER_COUNT  = 1;
    constexpr uint8_t SET_VALUE        = 7;
    constexpr uint8_t OFFSET_MSB       = static_cast<uint8_t>(TEST_PARAMETER_OFFSET >> 8);
    constexpr uint8_t OFFSET_LSB       = static_cast<uint8_t>(TEST_PARAMETER_OFFSET & 0xFF);
    constexpr uint8_t HANDSHAKE_SIZE   = static_cast<uint8_t>(HANDSHAKE_BIT | TEST_SIZE_ONE_BYTE);
    auto group                         = endpoint.emplace_node(BULK_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::parameter_offset)).set_desired(TEST_PARAMETER_OFFSET);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::number_of_parameters)).set_desired(PARAMETER_COUNT);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::size)).set_desired(TEST_SIZE_ONE_BYTE);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::handshake)).set_desired<uint8_t>(1);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::default_flag)).set_desired<uint8_t>(0);
    group.emplace_node(static_cast<attribute_store_type_t>(configuration_bulk_set_group_attributes_t::vg)).set_desired(std::vector<uint8_t> {SET_VALUE});
    std::array<uint8_t, 8> frame {};
    uint16_t frame_length = 0;
    zwave_frame_generator generator {COMMAND_CLASS_ID};
    generator.initialize_frame(BULK_SET_COMMAND, frame.data(), &frame_length);
    command_class_configuration_core::set_requested_args args {group, &generator};

    ASSERT_EQ(SL_STATUS_OK, component().on_configuration_bulk_set_requested(args, frame.data(), &frame_length));

    EXPECT_EQ((std::vector<uint8_t> {COMMAND_CLASS_ID, BULK_SET_COMMAND, OFFSET_MSB, OFFSET_LSB, PARAMETER_COUNT, HANDSHAKE_SIZE, SET_VALUE}), std::vector<uint8_t>(frame.begin(), frame.begin() + frame_length));
}

// CC:0070.01.04.11.006 — A reserved Size is ignored, so the report does not become parameter state.
TEST_F(ConfigurationCommandClassTest, ReportWithReservedSizeDoesNotCreateParameterState)
{
    constexpr uint8_t RESERVED_SIZE = 3;
    const command_class_configuration_attribute_map_t report {{"parameter_number", TEST_PARAMETER_1}, {"size", RESERVED_SIZE}, {"configuration_value", std::vector<uint8_t> {0x01}}};

    ASSERT_EQ(SL_STATUS_OK, store().on_configuration_report_received_store(endpoint, report));

    EXPECT_TRUE(endpoint.child_by_type(REPORT_GROUP).is_valid());
    EXPECT_FALSE(endpoint.child_by_type_and_value(PARAMETER_ID, TEST_PARAMETER_1).is_valid());
}

// CC:0070.02.09.11.00F — A Bulk Report with a reserved Size does not update parameter state.
TEST_F(ConfigurationCommandClassTest, BulkReportWithReservedSizeDoesNotCreateParameterState)
{
    constexpr uint8_t RESERVED_SIZE   = 3;
    constexpr uint8_t PARAMETER_COUNT = 1;
    const configuration_bulk_report_vg_t values {{.parameter = {0x01}}};
    const command_class_configuration_attribute_map_t report {
      {"parameter_offset", TEST_PARAMETER_OFFSET},
      {"number_of_parameters", PARAMETER_COUNT},
      {"size", RESERVED_SIZE},
      {"vg", values},
    };

    ASSERT_EQ(SL_STATUS_OK, store().on_configuration_bulk_report_received_store(endpoint, report));

    EXPECT_FALSE(endpoint.child_by_type_and_value(PARAMETER_ID, TEST_PARAMETER_OFFSET).is_valid());
}

// CC:0070.02.07.11.00C — Bulk Set Size must be a defined width. A parameter with no size child cannot supply one.
TEST_F(ConfigurationCommandClassTest, MqttBulkSetRejectsParameterWithoutSize)
{
    parameter(TEST_PARAMETER_1);
    const auto payload = R"({"parameter_offset":1,"number_of_parameters":1,"properties1":{"default_flag":1}})";

    EXPECT_NE(SL_STATUS_OK, command_class_configuration_mqtt::mqtt_on_configuration_bulk_set_command(endpoint, payload));
    EXPECT_FALSE(endpoint.child_by_type(BULK_SET_GROUP).is_valid());
}
