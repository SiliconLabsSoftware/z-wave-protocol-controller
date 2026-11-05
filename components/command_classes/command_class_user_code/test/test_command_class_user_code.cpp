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
#include <iterator>
#include <string>
#include <vector>

#include "attribute.hpp"
#include "attribute_store_fixt.h"
#include "attribute_store_type_registration.h"
#include "command_class_user_code.hpp"
#include "command_class_user_code_core.hpp"
#include "command_class_user_code_types.hpp"
#include "component_connector.hpp"
#include "component_connector_common_events.hpp"
#include "component_connector_types.hpp"
#include "zwave_frame_generator.hpp"
#include "zwave_frame_parser.hpp"
#include "ZW_classcmd.h"

using namespace zwave_command_class;
using namespace zwave_command_class::command_class_user_code_types;

extern "C" void zwave_network_management_publish_remove_failed_report(zwave_node_id_t, const char *) {}

namespace user_code_test
{
    constexpr uint8_t USER_CODE_CC                      = COMMAND_CLASS_USER_CODE;
    constexpr uint8_t USER_CODE_VERSION_1               = 1;
    constexpr uint8_t USER_CODE_VERSION_2               = 2;
    constexpr uint8_t CAPABILITIES_GET                  = 0x06;
    constexpr uint8_t CAPABILITIES_REPORT               = 0x07;
    constexpr uint8_t KEYPAD_MODE_SET                   = 0x08;
    constexpr uint8_t KEYPAD_MODE_REPORT                = 0x0A;
    constexpr uint8_t EXTENDED_USER_CODE_SET            = 0x0B;
    constexpr uint8_t EXTENDED_USER_CODE_GET            = 0x0C;
    constexpr uint8_t EXTENDED_USER_CODE_REPORT         = 0x0D;
    constexpr uint8_t ADMIN_CODE_SET                    = 0x0E;
    constexpr uint8_t ADMIN_CODE_REPORT                 = 0x10;
    constexpr uint8_t USER_CODE_CHECKSUM_GET            = 0x11;
    constexpr uint8_t USER_CODE_CHECKSUM_REPORT         = 0x12;
    constexpr uint8_t STATUS_AVAILABLE                  = 0x00;
    constexpr uint8_t STATUS_OCCUPIED                   = 0x01;
    constexpr uint8_t STATUS_RESERVED                   = 0x02;
    constexpr uint8_t STATUS_PASSAGE                    = 0x04;
    constexpr uint8_t STATUS_OUT_OF_RANGE               = 0x05;
    constexpr uint8_t KEYPAD_VACATION                   = 0x01;
    constexpr uint8_t KEYPAD_MODE_OUTSIDE_MASK          = 0x02;
    constexpr uint8_t UCC_SUPPORT_MASK                  = 0x80;
    constexpr uint8_t RESERVED_LENGTH_NIBBLE            = 0x14;
    constexpr uint8_t USER_IDENTIFIER_ZERO              = 0;
    constexpr uint8_t USER_IDENTIFIER_1                 = 1;
    constexpr uint8_t USER_IDENTIFIER_2                 = 2;
    constexpr uint8_t TWO_SUPPORTED_USERS               = 2;
    constexpr uint8_t UNSUPPORTED_USER_IDENTIFIER       = 3;
    constexpr uint8_t KEYPAD_NORMAL                     = 0x00;
    constexpr uint8_t KEYPAD_ONE_MODE                   = 0x01;  // bit 0, Normal only
    constexpr uint8_t KEYPAD_TWO_MODES                  = 0x03;  // bits 0 and 1
    constexpr uint8_t AC_SUPPORT_MASK                   = 0x80;
    constexpr uint8_t ACD_SUPPORT_MASK                  = 0x40;
    constexpr uint8_t MUCS_SUPPORT_MASK                 = 0x20;
    constexpr uint8_t MUCR_SUPPORT_MASK                 = 0x40;
    constexpr uint8_t MASK_LENGTH_ONE                   = 1;
    constexpr uint8_t STATUS_MASK_OCCUPIED_AND_RESERVED = 0x06;  // bits 1 and 2
    constexpr uint8_t ADMIN_CODE_LENGTH                 = 4;
    constexpr uint8_t DIGIT_1                           = 0x31;
    constexpr uint8_t DIGIT_2                           = 0x32;
    constexpr uint8_t DIGIT_3                           = 0x33;
    constexpr uint8_t DIGIT_4                           = 0x34;
    constexpr uint8_t NON_DIGIT                         = 0x41;
    constexpr uint8_t BELOW_DIGIT                       = 0x2F;
    constexpr uint8_t ABOVE_PRINTABLE                   = 0x7F;
    constexpr uint8_t CODE_LENGTH_TOO_SHORT             = 3;
    constexpr uint8_t CODE_LENGTH_TOO_LONG              = 11;
    constexpr uint8_t CODE_LENGTH_MISMATCH              = 3;
    constexpr uint8_t ZERO_SUPPORTED_USERS              = 0;
    constexpr uint16_t NO_EXTENDED_TOTAL                = 0;
    constexpr uint8_t REPORT_MORE_REQUESTED             = 1;
    constexpr uint8_t REPORT_MORE_NOT_REQUESTED         = 0;
    constexpr uint8_t ONE_USER_CODE                     = 1;
    constexpr uint8_t TWO_USER_CODES                    = 2;
    constexpr uint8_t ZERO_USER_CODES                   = 0;
    constexpr uint8_t LENGTH_ZERO                       = 0;
    constexpr uint8_t IDENTIFIER_MSB_ZERO               = 0;
    constexpr uint8_t NEXT_USER_MSB_ZERO                = 0;
    constexpr uint8_t NEXT_USER_LSB_NONE                = 0;
    constexpr uint8_t IDENTIFIER_PAST_TOTAL_MSB         = 0x01;
    constexpr uint8_t IDENTIFIER_PAST_TOTAL_LSB         = 0x01;
    constexpr uint8_t PROPERTIES3_NO_KEYS_MASK          = 0;
    constexpr uint8_t NON_PRINTABLE_BYTE                = 0x01;
    constexpr uint8_t KEYS_MASK_ASCII_1                 = 0x02;
    constexpr uint8_t CHECKSUM_MSB                      = 0x12;
    constexpr uint8_t CHECKSUM_LSB                      = 0x34;
    constexpr uint16_t CHECKSUM_STORED                  = 0x1234;
    constexpr uint8_t STORED_BLOCK_COUNT                = 2;
    constexpr size_t MINIMUM_STORED_BLOCK_BYTES         = 3;
    constexpr size_t STORED_IDENTIFIER_LSB_INDEX        = 1;
    constexpr size_t STORED_STATUS_INDEX                = 2;
    constexpr size_t COMMAND_INDEX                      = 1;
    constexpr size_t V1_USER_IDENTIFIER_INDEX           = 2;
    constexpr size_t V1_STATUS_INDEX                    = 3;
    constexpr size_t V1_CODE_INDEX                      = 4;
    constexpr size_t EXTENDED_USER_COUNT_INDEX          = 2;
    constexpr size_t EXTENDED_IDENTIFIER_MSB_INDEX      = 2;
    constexpr size_t EXTENDED_IDENTIFIER_LSB_INDEX      = 3;
    constexpr size_t EXTENDED_REPORT_MORE_INDEX         = 4;
    constexpr size_t ADMIN_LENGTH_INDEX                 = 2;
    constexpr size_t KEYPAD_MODE_INDEX                  = 2;
    constexpr uint8_t CLOSED_ENDPOINT_ID                = 1;
    constexpr uint16_t TEST_NODE_ID                     = 2;
    constexpr uint8_t TEST_ENDPOINT_ID                  = 0;
    constexpr uint16_t EXTENDED_TOTAL                   = 256;
    constexpr uint8_t EXTENDED_TOTAL_MSB                = 0x01;
    constexpr uint8_t EXTENDED_TOTAL_LSB                = 0x00;
    constexpr uint8_t SUPPORTED_USERS_SATURATED         = 255;
    constexpr size_t FRAME_BUFFER_SIZE                  = 32;
    constexpr std::array<uint8_t, 4> TEST_HOME_ID {0x12, 0x34, 0x56, 0x78};
    const std::vector<uint8_t> ASCII_CODE {DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4};
    const std::vector<uint8_t> ERASED_CODE {0x00, 0x00, 0x00, 0x00};

    const auto V1_REPORT_GROUP       = static_cast<attribute_store_type_t>(user_code_report_group_attributes_t::USER_CODE_REPORT_GROUP);
    const auto V1_REPORT_STATUS      = static_cast<attribute_store_type_t>(user_code_report_group_attributes_t::user_id_status);
    const auto V1_REPORT_CODE        = static_cast<attribute_store_type_t>(user_code_report_group_attributes_t::user_code);
    const auto V1_GET_GROUP          = static_cast<attribute_store_type_t>(user_code_get_group_attributes_t::USER_CODE_GET_GROUP);
    const auto V1_GET_ID             = static_cast<attribute_store_type_t>(user_code_get_group_attributes_t::user_identifier);
    const auto V1_SET_GROUP          = static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::USER_CODE_SET_GROUP);
    const auto USERS_GET_GROUP       = static_cast<attribute_store_type_t>(users_number_get_group_attributes_t::USERS_NUMBER_GET_GROUP);
    const auto USERS_REPORT_GROUP    = static_cast<attribute_store_type_t>(users_number_report_group_attributes_t::USERS_NUMBER_REPORT_GROUP);
    const auto SUPPORTED_USERS       = static_cast<attribute_store_type_t>(users_number_report_group_attributes_t::supported_users);
    const auto EXTENDED_USERS        = static_cast<attribute_store_type_t>(users_number_report_group_attributes_t::extended_supported_users);
    const auto CAP_GET_GROUP         = static_cast<attribute_store_type_t>(user_code_capabilities_get_group_attributes_t::USER_CODE_CAPABILITIES_GET_GROUP);
    const auto CAP_GROUP             = static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::USER_CODE_CAPABILITIES_REPORT_GROUP);
    const auto AC_SUPPORT            = static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::ac_support);
    const auto MUCS_SUPPORT          = static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::mucs_support);
    const auto EXT_GET_GROUP         = static_cast<attribute_store_type_t>(extended_user_code_get_group_attributes_t::EXTENDED_USER_CODE_GET_GROUP);
    const auto EXT_GET_ID            = static_cast<attribute_store_type_t>(extended_user_code_get_group_attributes_t::user_identifier);
    const auto EXT_GET_MORE          = static_cast<attribute_store_type_t>(extended_user_code_get_group_attributes_t::report_more);
    const auto EXT_SET_GROUP         = static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::EXTENDED_USER_CODE_SET_GROUP);
    const auto EXT_REPORT_GROUP      = static_cast<attribute_store_type_t>(extended_user_code_report_group_attributes_t::EXTENDED_USER_CODE_REPORT_GROUP);
    const auto EXT_REPORT_NEXT       = static_cast<attribute_store_type_t>(extended_user_code_report_group_attributes_t::next_user_identifier);
    const auto EXT_REPORT_VG1        = static_cast<attribute_store_type_t>(extended_user_code_report_group_attributes_t::vg1);
    const auto KEYPAD_GET_GROUP      = static_cast<attribute_store_type_t>(user_code_keypad_mode_get_group_attributes_t::USER_CODE_KEYPAD_MODE_GET_GROUP);
    const auto KEYPAD_SET_GROUP      = static_cast<attribute_store_type_t>(user_code_keypad_mode_set_group_attributes_t::USER_CODE_KEYPAD_MODE_SET_GROUP);
    const auto KEYPAD_REPORT_GROUP   = static_cast<attribute_store_type_t>(user_code_keypad_mode_report_group_attributes_t::USER_CODE_KEYPAD_MODE_REPORT_GROUP);
    const auto KEYPAD_REPORT_MODE    = static_cast<attribute_store_type_t>(user_code_keypad_mode_report_group_attributes_t::keypad_mode);
    const auto ADMIN_GET_GROUP       = static_cast<attribute_store_type_t>(admin_code_get_group_attributes_t::ADMIN_CODE_GET_GROUP);
    const auto ADMIN_SET_GROUP       = static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::ADMIN_CODE_SET_GROUP);
    const auto ADMIN_REPORT_GROUP    = static_cast<attribute_store_type_t>(admin_code_report_group_attributes_t::ADMIN_CODE_REPORT_GROUP);
    const auto ADMIN_REPORT_LENGTH   = static_cast<attribute_store_type_t>(admin_code_report_group_attributes_t::admin_code_length);
    const auto ADMIN_REPORT_CODE     = static_cast<attribute_store_type_t>(admin_code_report_group_attributes_t::admin_code);
    const auto CHECKSUM_GET_GROUP    = static_cast<attribute_store_type_t>(user_code_checksum_get_group_attributes_t::USER_CODE_CHECKSUM_GET_GROUP);
    const auto CHECKSUM_REPORT_GROUP = static_cast<attribute_store_type_t>(user_code_checksum_report_group_attributes_t::USER_CODE_CHECKSUM_REPORT_GROUP);
    const auto CHECKSUM_VALUE        = static_cast<attribute_store_type_t>(user_code_checksum_report_group_attributes_t::user_code_checksum);

    std::vector<uint8_t> capabilities_frame(uint8_t properties1, uint8_t properties2, uint8_t keypad_mask)
    {
        // Properties1 and Properties2 carry a mask length of 1. Properties3 length 0 omits the Supported Keys Bit Mask,
        // so a later set falls back to printable ASCII (CC:0063.02.07.11.012).
        return {USER_CODE_CC, CAPABILITIES_REPORT, properties1, STATUS_MASK_OCCUPIED_AND_RESERVED, properties2, keypad_mask, PROPERTIES3_NO_KEYS_MASK};
    }

    // attribute_store_teardown() drops type registration. The command class registers its types once, so later tests
    // would treat byte arrays as unknown and emit them reversed.
    void register_byte_array_attributes()
    {
        const auto report_group    = static_cast<attribute_store_type_t>(user_code_report_group_attributes_t::USER_CODE_REPORT_GROUP);
        const auto set_group       = static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::USER_CODE_SET_GROUP);
        const auto extended_set    = static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::EXTENDED_USER_CODE_SET_GROUP);
        const auto extended_report = static_cast<attribute_store_type_t>(extended_user_code_report_group_attributes_t::EXTENDED_USER_CODE_REPORT_GROUP);
        const auto capabilities    = static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::USER_CODE_CAPABILITIES_REPORT_GROUP);
        const auto admin_set       = static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::ADMIN_CODE_SET_GROUP);
        const auto admin_report    = static_cast<attribute_store_type_t>(admin_code_report_group_attributes_t::ADMIN_CODE_REPORT_GROUP);
        const auto add             = [](attribute_store_type_t type, const char *name, attribute_store_type_t parent) {
            if (!attribute_store_is_type_registered(type)) {
                attribute_store_register_type(type, name, parent, BYTE_ARRAY_STORAGE_TYPE);
            }
        };
        add(static_cast<attribute_store_type_t>(user_code_report_group_attributes_t::user_code), "user_code", report_group);
        add(static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::user_code), "user_code", set_group);
        add(static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::vg1), "vg1", extended_set);
        add(static_cast<attribute_store_type_t>(extended_user_code_report_group_attributes_t::vg1), "vg1", extended_report);
        add(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::supported_user_id_status_bit_mask), "status_mask", capabilities);
        add(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::supported_keypad_modes_bit_mask), "keypad_mask", capabilities);
        add(static_cast<attribute_store_type_t>(user_code_capabilities_report_group_attributes_t::supported_keys_bit_mask), "keys_mask", capabilities);
        add(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code), "admin_code", admin_set);
        add(static_cast<attribute_store_type_t>(admin_code_report_group_attributes_t::admin_code), "admin_code", admin_report);
    }
}  // namespace user_code_test

using namespace user_code_test;

class UserCodeCommandClassTest : public ::testing::Test
{
    protected:
        static command_class_user_code &component()
        {
            static command_class_user_code instance;
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

        void TearDown() override
        {
            attribute_store_teardown();
        }

        // Node ids restart after teardown, so an interview left open on `endpoint` is still open
        // for the next test. A second endpoint has a new id and a closed interview window.
        attribute_store::attribute closed_endpoint()
        {
            return endpoint.parent().emplace_node(ATTRIBUTE_ENDPOINT_ID, CLOSED_ENDPOINT_ID);
        }

        void set_peer_version(uint8_t version)
        {
            endpoint.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_USER_CODE)).set_reported(version);
        }

        // The device interviewer opens this command class through the interview-done event.
        void open_interview(attribute_store::attribute target)
        {
            component_connector connector;
            const component_connector_interview_done_payload_t payload {static_cast<attribute_store_node_t>(target), SL_STATUS_OK};
            auto status = connector.fire_event_async(static_cast<uint32_t>(component_connector_common_events_t::COMPONENT_CONNECTOR_INTERVIEW_DONE), payload);
            connector.run();
            ASSERT_EQ(SL_STATUS_OK, status.get());
        }

        sl_status_t receive(const std::vector<uint8_t> &frame)
        {
            return receive(frame, endpoint);
        }

        sl_status_t receive(const std::vector<uint8_t> &frame, attribute_store::attribute target)
        {
            // The parser starts after the command-class and command bytes.
            zwave_frame_parser parser {frame.data(), static_cast<uint16_t>(frame.size())};
            command_class_user_code_core::report_received_args args {target, parser, nullptr};
            switch (frame[COMMAND_INDEX]) {
                case USER_CODE_REPORT:
                    return component().on_user_code_report_received(args);
                case USERS_NUMBER_REPORT:
                    return component().on_users_number_report_received(args);
                case CAPABILITIES_REPORT:
                    return component().on_user_code_capabilities_report_received(args);
                case KEYPAD_MODE_REPORT:
                    return component().on_user_code_keypad_mode_report_received(args);
                case EXTENDED_USER_CODE_REPORT:
                    return component().on_extended_user_code_report_received(args);
                case ADMIN_CODE_REPORT:
                    return component().on_admin_code_report_received(args);
                case USER_CODE_CHECKSUM_REPORT:
                    return component().on_user_code_checksum_report_received(args);
                default:
                    return SL_STATUS_FAIL;
            }
        }

        sl_status_t assemble(attribute_store::attribute group, uint8_t command, uint8_t *frame, uint16_t *frame_length, bool is_set)
        {
            zwave_frame_generator generator {USER_CODE_CC};
            generator.initialize_frame(command, frame, frame_length);
            if (is_set) {
                command_class_user_code_core::set_requested_args args {group, &generator};
                switch (command) {
                    case USER_CODE_SET:
                        return component().on_user_code_set_requested(args, frame, frame_length);
                    case EXTENDED_USER_CODE_SET:
                        return component().on_extended_user_code_set_requested(args, frame, frame_length);
                    case KEYPAD_MODE_SET:
                        return component().on_user_code_keypad_mode_set_requested(args, frame, frame_length);
                    case ADMIN_CODE_SET:
                        return component().on_admin_code_set_requested(args, frame, frame_length);
                    default:
                        return SL_STATUS_FAIL;
                }
            }
            command_class_user_code_core::get_requested_args args {group, &generator};
            switch (command) {
                case USER_CODE_GET:
                    return component().on_user_code_get_requested(args, frame, frame_length);
                case EXTENDED_USER_CODE_GET:
                    return component().on_extended_user_code_get_requested(args, frame, frame_length);
                case USER_CODE_CHECKSUM_GET:
                    return component().on_user_code_checksum_get_requested(args, frame, frame_length);
                case USERS_NUMBER_GET:
                    return component().on_users_number_get_requested(args, frame, frame_length);
                case CAPABILITIES_GET:
                    return component().on_user_code_capabilities_get_requested(args, frame, frame_length);
                default:
                    return SL_STATUS_FAIL;
            }
        }

        void store_users(uint8_t supported, uint16_t extended)
        {
            store_users(endpoint, supported, extended);
        }

        void store_users(attribute_store::attribute target, uint8_t supported, uint16_t extended)
        {
            constexpr uint8_t LOW_BYTE_MASK = 0xFF;
            std::vector<uint8_t> frame {USER_CODE_CC, USERS_NUMBER_REPORT, supported, static_cast<uint8_t>(extended >> 8), static_cast<uint8_t>(extended & LOW_BYTE_MASK)};
            if (extended == 0 && supported != 0) {
                frame = {USER_CODE_CC, USERS_NUMBER_REPORT, supported};
            }
            ASSERT_EQ(SL_STATUS_OK, receive(frame, target));
        }

        // Interview tests use the fixture endpoint. Reports that must not continue an interview
        // use this peer: its attribute-store node is not the recycled interview-open handle.
        attribute_store::attribute versioned_peer(uint8_t version)
        {
            auto target = closed_endpoint();
            target.emplace_node(ZWAVE_CC_VERSION_ATTRIBUTE(COMMAND_CLASS_USER_CODE)).set_reported(version);
            return target;
        }

        attribute_store::attribute version_two_peer(uint8_t properties1, uint8_t properties2, uint8_t keypad_mask, uint8_t supported_users, uint16_t extended_users)
        {
            auto target = versioned_peer(USER_CODE_VERSION_2);
            store_users(target, supported_users, extended_users);
            receive(capabilities_frame(properties1, properties2, keypad_mask), target);
            return target;
        }

        std::string json_bytes(const std::vector<uint8_t> &bytes)
        {
            std::string encoded = "[";
            for (size_t index = 0; index < bytes.size(); ++index) {
                if (index != 0) {
                    encoded += ',';
                }
                encoded += std::to_string(bytes[index]);
            }
            encoded += ']';
            return encoded;
        }

        std::string user_set_json(uint8_t identifier, uint8_t status, const std::vector<uint8_t> &code)
        {
            return std::string("{\"user_identifier\":") + std::to_string(identifier) + ",\"user_id_status\":" + std::to_string(status) + ",\"user_code\":" + json_bytes(code) + "}";
        }

        attribute_store::attribute user_code_set_group(attribute_store::attribute target, uint8_t identifier, uint8_t status, const std::vector<uint8_t> &code)
        {
            auto group = target.emplace_node(V1_SET_GROUP);
            group.emplace_node(static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::user_identifier)).set_desired(identifier);
            group.emplace_node(static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::user_id_status)).set_desired(status);
            group.emplace_node(static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::user_code)).set_desired(code);
            return group;
        }

        attribute_store::attribute extended_set_group(attribute_store::attribute target, uint8_t count, const std::vector<uint8_t> &blocks)
        {
            auto group = target.emplace_node(EXT_SET_GROUP);
            group.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::number_of_user_codes)).set_desired(count);
            group.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::vg1)).set_desired(blocks);
            return group;
        }

        std::vector<uint8_t> occupied_block(uint8_t identifier_msb, uint8_t identifier_lsb)
        {
            return {identifier_msb, identifier_lsb, STATUS_OCCUPIED, ADMIN_CODE_LENGTH, DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4};
        }

        std::vector<uint8_t> available_block(uint8_t identifier)
        {
            return {IDENTIFIER_MSB_ZERO, identifier, STATUS_AVAILABLE, LENGTH_ZERO};
        }

        attribute_store::attribute endpoint;
};

// Spec 2026B User Code v1, Available code (CC:0063.01.01.11.009).
// Status 0 carries exactly four 0x00 bytes, and those bytes are part of the code.
TEST_F(UserCodeCommandClassTest, UserCodeReportAvailableStoresFourZeroBytes)
{
    set_peer_version(USER_CODE_VERSION_1);
    std::vector<uint8_t> frame {USER_CODE_CC, USER_CODE_REPORT, USER_IDENTIFIER_1, STATUS_AVAILABLE};
    frame.insert(frame.end(), ERASED_CODE.begin(), ERASED_CODE.end());

    ASSERT_EQ(SL_STATUS_OK, receive(frame));

    const auto group = endpoint.child_by_type(V1_REPORT_GROUP);
    ASSERT_TRUE(group.is_valid());
    EXPECT_EQ(STATUS_AVAILABLE, group.child_by_type(V1_REPORT_STATUS).reported<uint8_t>());
    EXPECT_EQ(ERASED_CODE, group.child_by_type(V1_REPORT_CODE).reported<std::vector<uint8_t>>());
}

// Spec 2026B User Code v1, occupied digits (CC:0063.01.01.11.007).
// Occupied codes are ASCII digits and are stored as sent.
TEST_F(UserCodeCommandClassTest, UserCodeReportOccupiedStoresAsciiDigits)
{
    set_peer_version(USER_CODE_VERSION_1);
    std::vector<uint8_t> frame {USER_CODE_CC, USER_CODE_REPORT, USER_IDENTIFIER_1, STATUS_OCCUPIED};
    frame.insert(frame.end(), ASCII_CODE.begin(), ASCII_CODE.end());

    ASSERT_EQ(SL_STATUS_OK, receive(frame));

    EXPECT_EQ(ASCII_CODE, endpoint.child_by_type(V1_REPORT_GROUP).child_by_type(V1_REPORT_CODE).reported<std::vector<uint8_t>>());
}

// Spec 2026B User Code v2 Users Number Report (CC:0063.02.05.11.003).
// 256 or more users are advertised as 255 plus the extended total.
TEST_F(UserCodeCommandClassTest, UsersNumberReportVersionTwoStoresExtendedTotal)
{
    set_peer_version(USER_CODE_VERSION_2);

    store_users(SUPPORTED_USERS_SATURATED, EXTENDED_TOTAL);

    const auto group = endpoint.child_by_type(USERS_REPORT_GROUP);
    EXPECT_EQ(SUPPORTED_USERS_SATURATED, group.child_by_type(SUPPORTED_USERS).reported<uint8_t>());
    EXPECT_EQ(EXTENDED_TOTAL, group.child_by_type(EXTENDED_USERS).reported<uint16_t>());
}

// Spec 2026B User Code interview figure (CL:0063.01.21.01.2).
// Version 2 starts with Capabilities Get. Users Number Get waits for that report.
TEST_F(UserCodeCommandClassTest, VersionTwoInterviewStartsWithCapabilitiesGet)
{
    set_peer_version(USER_CODE_VERSION_2);

    open_interview(endpoint);

    EXPECT_TRUE(endpoint.child_by_type(CAP_GET_GROUP).is_valid());
    EXPECT_FALSE(endpoint.child_by_type(USERS_GET_GROUP).is_valid());
}

// Spec 2026B User Code interview figure (CL:0063.01.21.01.2).
// After capabilities, the controller asks how many users the node supports.
TEST_F(UserCodeCommandClassTest, CapabilitiesReportContinuesWithUsersNumberGet)
{
    set_peer_version(USER_CODE_VERSION_2);
    open_interview(endpoint);

    ASSERT_EQ(SL_STATUS_OK, receive(capabilities_frame(static_cast<uint8_t>(AC_SUPPORT_MASK | MASK_LENGTH_ONE), static_cast<uint8_t>(MUCR_SUPPORT_MASK | MUCS_SUPPORT_MASK | MASK_LENGTH_ONE), KEYPAD_TWO_MODES)));

    EXPECT_TRUE(endpoint.child_by_type(USERS_GET_GROUP).is_valid());
    EXPECT_FALSE(endpoint.child_by_type(ADMIN_GET_GROUP).is_valid());
}

// Spec 2026B User Code interview figure (CL:0063.01.21.01.2).
// Admin Code Get is next only when the stored AC bit is set, and it precedes Keypad Mode Get.
TEST_F(UserCodeCommandClassTest, UsersNumberReportRequestsAdminGetWhenAdminCodeSupported)
{
    set_peer_version(USER_CODE_VERSION_2);
    open_interview(endpoint);
    ASSERT_EQ(SL_STATUS_OK, receive(capabilities_frame(static_cast<uint8_t>(AC_SUPPORT_MASK | MASK_LENGTH_ONE), static_cast<uint8_t>(MASK_LENGTH_ONE), KEYPAD_TWO_MODES)));

    store_users(TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    EXPECT_TRUE(endpoint.child_by_type(ADMIN_GET_GROUP).is_valid());
    EXPECT_FALSE(endpoint.child_by_type(KEYPAD_GET_GROUP).is_valid());
}

// Spec 2026B User Code interview figure (CL:0063.01.21.01.2).
// Keypad Mode Get follows Admin Code when more than one keypad mode is supported.
TEST_F(UserCodeCommandClassTest, AdminCodeReportRequestsKeypadGetWhenTwoModesSupported)
{
    set_peer_version(USER_CODE_VERSION_2);
    open_interview(endpoint);
    ASSERT_EQ(SL_STATUS_OK, receive(capabilities_frame(static_cast<uint8_t>(AC_SUPPORT_MASK | ACD_SUPPORT_MASK | MASK_LENGTH_ONE), static_cast<uint8_t>(MUCR_SUPPORT_MASK | MASK_LENGTH_ONE), KEYPAD_TWO_MODES)));
    store_users(TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    ASSERT_EQ(SL_STATUS_OK, receive({USER_CODE_CC, ADMIN_CODE_REPORT, ADMIN_CODE_LENGTH, DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4}));

    EXPECT_TRUE(endpoint.child_by_type(KEYPAD_GET_GROUP).is_valid());
}

// Spec 2026B Extended User Code Get Report More (CC:0063.02.0C.11.008).
// The first database read asks for user 1 and sets Report More when MUCR is supported.
TEST_F(UserCodeCommandClassTest, KeypadReportStartsExtendedGetWithReportMore)
{
    set_peer_version(USER_CODE_VERSION_2);
    open_interview(endpoint);
    ASSERT_EQ(SL_STATUS_OK, receive(capabilities_frame(static_cast<uint8_t>(MASK_LENGTH_ONE), static_cast<uint8_t>(MUCR_SUPPORT_MASK | MASK_LENGTH_ONE), KEYPAD_TWO_MODES)));
    store_users(TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    ASSERT_EQ(SL_STATUS_OK, receive({USER_CODE_CC, KEYPAD_MODE_REPORT, KEYPAD_NORMAL}));

    const auto get = endpoint.child_by_type(EXT_GET_GROUP);
    ASSERT_TRUE(get.is_valid());
    EXPECT_EQ(USER_IDENTIFIER_1, get.child_by_type(EXT_GET_ID).desired<uint16_t>());
    EXPECT_EQ(REPORT_MORE_REQUESTED, get.child_by_type(EXT_GET_MORE).desired<uint8_t>());
}

namespace user_code_test
{
    // Next User Identifier follows the last block, MSB then LSB (CC:0063.02.0D.11.00A).
    std::vector<uint8_t> extended_report(uint8_t user, uint8_t status, const std::vector<uint8_t> &code, uint8_t next_user_lsb)
    {
        std::vector<uint8_t> frame {USER_CODE_CC, EXTENDED_USER_CODE_REPORT, ONE_USER_CODE, IDENTIFIER_MSB_ZERO, user, status, static_cast<uint8_t>(code.size())};
        frame.insert(frame.end(), code.begin(), code.end());
        frame.push_back(NEXT_USER_MSB_ZERO);
        frame.push_back(next_user_lsb);
        return frame;
    }

    std::vector<uint8_t> with_keys_mask(uint8_t properties1, uint8_t properties2, uint8_t keypad_mask, uint8_t keys_mask)
    {
        // Properties3 length 1 means the next byte is the Supported Keys Bit Mask (CC:0063.02.07.11.012).
        return {USER_CODE_CC, CAPABILITIES_REPORT, properties1, STATUS_MASK_OCCUPIED_AND_RESERVED, properties2, keypad_mask, MASK_LENGTH_ONE, keys_mask};
    }
}  // namespace user_code_test

// Spec 2026B interview (CL:0063.01.21.01.2, CC:0063.02.0C.11.008).
// One stored keypad mode and a clear MUCR bit ask for user 1 without Report More.
TEST_F(UserCodeCommandClassTest, VersionTwoInterviewOmitsReportMoreWhenMucrIsClear)
{
    // Version 2, no Admin Code, one keypad mode, MUCR clear.
    set_peer_version(USER_CODE_VERSION_2);
    open_interview(endpoint);
    ASSERT_EQ(SL_STATUS_OK, receive(capabilities_frame(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_ONE_MODE)));
    store_users(TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    const auto get = endpoint.child_by_type(EXT_GET_GROUP);
    ASSERT_TRUE(get.is_valid());
    EXPECT_EQ(static_cast<uint16_t>(USER_IDENTIFIER_1), get.child_by_type(EXT_GET_ID).desired<uint16_t>());
    EXPECT_EQ(REPORT_MORE_NOT_REQUESTED, get.child_by_type(EXT_GET_MORE).desired<uint8_t>());
}

// Spec 2026B Extended User Code Report (CC:0063.02.0D.11.00A).
// A non-zero Next User Identifier continues the interview at that user.
TEST_F(UserCodeCommandClassTest, ExtendedReportRequestsTheNextUser)
{
    set_peer_version(USER_CODE_VERSION_2);
    open_interview(endpoint);
    ASSERT_EQ(SL_STATUS_OK, receive(capabilities_frame(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_ONE_MODE)));
    store_users(TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    // User 1 is occupied. Next User Identifier LSB is user 2.
    ASSERT_EQ(SL_STATUS_OK, receive(extended_report(USER_IDENTIFIER_1, STATUS_OCCUPIED, ASCII_CODE, USER_IDENTIFIER_2)));

    EXPECT_EQ(static_cast<uint16_t>(USER_IDENTIFIER_2), endpoint.child_by_type(EXT_GET_GROUP).child_by_type(EXT_GET_ID).desired<uint16_t>());
    EXPECT_FALSE(endpoint.child_by_type(EXT_REPORT_GROUP).child_by_type(EXT_REPORT_VG1).reported<std::vector<uint8_t>>().empty());
}

// Spec 2026B Extended User Code Report (CC:0063.02.0D.11.00B).
// Next User Identifier 0 means this user is the last one set, so the walk stops.
TEST_F(UserCodeCommandClassTest, ExtendedReportStopsWhenNextUserIsZero)
{
    set_peer_version(USER_CODE_VERSION_2);
    open_interview(endpoint);
    ASSERT_EQ(SL_STATUS_OK, receive(capabilities_frame(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_ONE_MODE)));
    store_users(TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    const auto get = endpoint.child_by_type(EXT_GET_GROUP);
    ASSERT_EQ(static_cast<uint16_t>(USER_IDENTIFIER_1), get.child_by_type(EXT_GET_ID).desired<uint16_t>());

    ASSERT_EQ(SL_STATUS_OK, receive(extended_report(USER_IDENTIFIER_1, STATUS_AVAILABLE, {}, NEXT_USER_LSB_NONE)));

    EXPECT_EQ(static_cast<uint16_t>(NEXT_USER_LSB_NONE), endpoint.child_by_type(EXT_REPORT_GROUP).child_by_type(EXT_REPORT_NEXT).reported<uint16_t>());
    EXPECT_EQ(static_cast<uint16_t>(USER_IDENTIFIER_1), get.child_by_type(EXT_GET_ID).desired<uint16_t>());
}

// Spec 2026B version 1 interview (CL:0063.01.21.01.2).
// Version 1 starts with Users Number Get and does not send Capabilities Get.
TEST_F(UserCodeCommandClassTest, VersionOneInterviewStartsWithUsersNumberGet)
{
    set_peer_version(USER_CODE_VERSION_1);

    open_interview(endpoint);

    EXPECT_TRUE(endpoint.child_by_type(USERS_GET_GROUP).is_valid());
    EXPECT_FALSE(endpoint.child_by_type(CAP_GET_GROUP).is_valid());
}

// Spec 2026B version 1 interview (CL:0063.01.21.01.2).
// After Users Number Report the controller reads user 1.
TEST_F(UserCodeCommandClassTest, VersionOneUsersNumberReportRequestsTheFirstUser)
{
    set_peer_version(USER_CODE_VERSION_1);
    open_interview(endpoint);

    ASSERT_EQ(SL_STATUS_OK, receive({USER_CODE_CC, USERS_NUMBER_REPORT, TWO_SUPPORTED_USERS}));

    EXPECT_EQ(USER_IDENTIFIER_1, endpoint.child_by_type(V1_GET_GROUP).child_by_type(V1_GET_ID).desired<uint8_t>());
}

// Spec 2026B version 1 interview (CL:0063.01.21.01.2).
// A report for a user below the supported total requests the next identifier.
TEST_F(UserCodeCommandClassTest, VersionOneReportRequestsTheNextUser)
{
    set_peer_version(USER_CODE_VERSION_1);
    open_interview(endpoint);
    ASSERT_EQ(SL_STATUS_OK, receive({USER_CODE_CC, USERS_NUMBER_REPORT, TWO_SUPPORTED_USERS}));
    std::vector<uint8_t> report {USER_CODE_CC, USER_CODE_REPORT, USER_IDENTIFIER_1, STATUS_AVAILABLE};
    report.insert(report.end(), ERASED_CODE.begin(), ERASED_CODE.end());

    ASSERT_EQ(SL_STATUS_OK, receive(report));

    EXPECT_EQ(USER_IDENTIFIER_2, endpoint.child_by_type(V1_GET_GROUP).child_by_type(V1_GET_ID).desired<uint8_t>());
}

// Spec 2026B version 1 interview (CL:0063.01.21.01.2).
// The report for the last supported user does not start another Get.
TEST_F(UserCodeCommandClassTest, VersionOneReportForTheLastUserDoesNotRequestAnother)
{
    set_peer_version(USER_CODE_VERSION_1);
    open_interview(endpoint);
    ASSERT_EQ(SL_STATUS_OK, receive({USER_CODE_CC, USERS_NUMBER_REPORT, TWO_SUPPORTED_USERS}));
    std::vector<uint8_t> first {USER_CODE_CC, USER_CODE_REPORT, USER_IDENTIFIER_1, STATUS_OCCUPIED};
    first.insert(first.end(), ASCII_CODE.begin(), ASCII_CODE.end());
    ASSERT_EQ(SL_STATUS_OK, receive(first));
    ASSERT_EQ(USER_IDENTIFIER_2, endpoint.child_by_type(V1_GET_GROUP).child_by_type(V1_GET_ID).desired<uint8_t>());
    std::vector<uint8_t> last {USER_CODE_CC, USER_CODE_REPORT, USER_IDENTIFIER_2, STATUS_OCCUPIED};
    last.insert(last.end(), ASCII_CODE.begin(), ASCII_CODE.end());

    ASSERT_EQ(SL_STATUS_OK, receive(last));

    EXPECT_EQ(USER_IDENTIFIER_2, endpoint.child_by_type(V1_GET_GROUP).child_by_type(V1_GET_ID).desired<uint8_t>());
}

// Spec 2026B version 1 interview (CL:0063.01.21.01.2).
// Supported Users 0 ends the walk before any User Code Get.
TEST_F(UserCodeCommandClassTest, VersionOneZeroSupportedUsersDoesNotRequestAUser)
{
    set_peer_version(USER_CODE_VERSION_1);
    open_interview(endpoint);

    ASSERT_EQ(SL_STATUS_OK, receive({USER_CODE_CC, USERS_NUMBER_REPORT, ZERO_SUPPORTED_USERS}));

    EXPECT_FALSE(endpoint.child_by_type(V1_GET_GROUP).is_valid());
}

// Spec 2026B version 2 interview (CL:0063.01.21.01.2).
// Supported Users 0 ends the walk before any Extended User Code Get.
TEST_F(UserCodeCommandClassTest, VersionTwoZeroSupportedUsersDoesNotRequestAUser)
{
    set_peer_version(USER_CODE_VERSION_2);
    open_interview(endpoint);
    ASSERT_EQ(SL_STATUS_OK, receive(capabilities_frame(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_ONE_MODE)));

    ASSERT_EQ(SL_STATUS_OK, receive({USER_CODE_CC, USERS_NUMBER_REPORT, ZERO_SUPPORTED_USERS}));

    EXPECT_FALSE(endpoint.child_by_type(EXT_GET_GROUP).is_valid());
}

// Spec 2026B version 2 interview (CL:0063.01.21.01.2).
// A version 1 User Code Report does not advance a version 2 interview.
TEST_F(UserCodeCommandClassTest, VersionTwoInterviewIgnoresVersionOneUserCodeReport)
{
    set_peer_version(USER_CODE_VERSION_2);
    open_interview(endpoint);
    std::vector<uint8_t> report {USER_CODE_CC, USER_CODE_REPORT, USER_IDENTIFIER_1, STATUS_OCCUPIED};
    report.insert(report.end(), ASCII_CODE.begin(), ASCII_CODE.end());

    ASSERT_EQ(SL_STATUS_OK, receive(report));

    // The report is stored, but version 2 does not write a version 1 Get identifier.
    EXPECT_FALSE(endpoint.child_by_type(V1_GET_GROUP).child_by_type(V1_GET_ID).desired_exists());
}

// Spec 2026B version 2 interview (CL:0063.01.21.01.2).
// A keypad report does not start the user walk when Supported Users is 0.
TEST_F(UserCodeCommandClassTest, VersionTwoKeypadReportAfterZeroUsersDoesNotStartTheWalk)
{
    set_peer_version(USER_CODE_VERSION_2);
    open_interview(endpoint);
    ASSERT_EQ(SL_STATUS_OK, receive(capabilities_frame(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_TWO_MODES)));
    ASSERT_EQ(SL_STATUS_OK, receive({USER_CODE_CC, USERS_NUMBER_REPORT, ZERO_SUPPORTED_USERS}));

    ASSERT_EQ(SL_STATUS_OK, receive({USER_CODE_CC, KEYPAD_MODE_REPORT, KEYPAD_NORMAL}));

    EXPECT_FALSE(endpoint.child_by_type(EXT_GET_GROUP).is_valid());
}

// Spec 2026B version 1 interview (CL:0063.01.21.01.2).
// User Identifier 0 in a report is not a user in the walk, so the pending Get stays at user 1.
TEST_F(UserCodeCommandClassTest, VersionOneReportWithIdentifierZeroDoesNotAdvance)
{
    set_peer_version(USER_CODE_VERSION_1);
    open_interview(endpoint);
    ASSERT_EQ(SL_STATUS_OK, receive({USER_CODE_CC, USERS_NUMBER_REPORT, TWO_SUPPORTED_USERS}));
    std::vector<uint8_t> report {USER_CODE_CC, USER_CODE_REPORT, USER_IDENTIFIER_ZERO, STATUS_AVAILABLE};
    report.insert(report.end(), ERASED_CODE.begin(), ERASED_CODE.end());

    ASSERT_EQ(SL_STATUS_OK, receive(report));

    EXPECT_EQ(USER_IDENTIFIER_1, endpoint.child_by_type(V1_GET_GROUP).child_by_type(V1_GET_ID).desired<uint8_t>());
}

// Spec 2026B User Code Capabilities Report (CC:0063.02.07.11.001).
// The AC Support bit is stored from the capabilities report.
TEST_F(UserCodeCommandClassTest, CapabilitiesReportStoresAdminCodeSupport)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);

    ASSERT_EQ(SL_STATUS_OK, receive(capabilities_frame(static_cast<uint8_t>(AC_SUPPORT_MASK | MASK_LENGTH_ONE), MASK_LENGTH_ONE, KEYPAD_ONE_MODE), target));

    EXPECT_EQ(AC_SUPPORT_MASK, target.child_by_type(CAP_GROUP).child_by_type(AC_SUPPORT).reported<uint8_t>());
}

// Spec 2026B Admin Code Report (CC:0063.02.10.11.002, CC:0063.02.10.11.005).
// A reported Admin Code is stored with the length that preceded it.
TEST_F(UserCodeCommandClassTest, AdminCodeReportStoresLengthAndCode)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);
    std::vector<uint8_t> report {USER_CODE_CC, ADMIN_CODE_REPORT, ADMIN_CODE_LENGTH};
    report.insert(report.end(), ASCII_CODE.begin(), ASCII_CODE.end());

    ASSERT_EQ(SL_STATUS_OK, receive(report, target));

    EXPECT_EQ(ADMIN_CODE_LENGTH, target.child_by_type(ADMIN_REPORT_GROUP).child_by_type(ADMIN_REPORT_LENGTH).reported<uint8_t>());
    EXPECT_EQ(ASCII_CODE, target.child_by_type(ADMIN_REPORT_GROUP).child_by_type(ADMIN_REPORT_CODE).reported<std::vector<uint8_t>>());
}

// Spec 2026B Admin Code Report (CC:0063.02.10.11.003, CC:0063.02.10.11.004).
// Length 0 means the Admin Code is deactivated and the code field is omitted.
TEST_F(UserCodeCommandClassTest, AdminCodeReportWithLengthZeroOmitsTheCode)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);

    ASSERT_EQ(SL_STATUS_OK, receive({USER_CODE_CC, ADMIN_CODE_REPORT, LENGTH_ZERO}, target));

    EXPECT_EQ(LENGTH_ZERO, target.child_by_type(ADMIN_REPORT_GROUP).child_by_type(ADMIN_REPORT_LENGTH).reported<uint8_t>());
    // Length 0 makes the parser present an empty code, which is the omitted field.
    EXPECT_TRUE(target.child_by_type(ADMIN_REPORT_GROUP).child_by_type(ADMIN_REPORT_CODE).reported<std::vector<uint8_t>>().empty());
}

// Spec 2026B User Code Keypad Mode Report (CC:0063.02.0A.11.001).
TEST_F(UserCodeCommandClassTest, KeypadModeReportStoresTheReportedMode)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);

    ASSERT_EQ(SL_STATUS_OK, receive({USER_CODE_CC, KEYPAD_MODE_REPORT, KEYPAD_NORMAL}, target));

    EXPECT_EQ(KEYPAD_NORMAL, target.child_by_type(KEYPAD_REPORT_GROUP).child_by_type(KEYPAD_REPORT_MODE).reported<uint8_t>());
}

// Spec 2026B Extended User Code Report (CC:0063.02.0D.11.00B).
// Outside interview, Next User Identifier 0 is stored and no follow-up user is requested.
TEST_F(UserCodeCommandClassTest, ExtendedReportOutsideInterviewStoresNextUserZero)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);

    ASSERT_EQ(SL_STATUS_OK, receive(extended_report(USER_IDENTIFIER_1, STATUS_OCCUPIED, ASCII_CODE, NEXT_USER_LSB_NONE), target));

    EXPECT_EQ(static_cast<uint16_t>(NEXT_USER_LSB_NONE), target.child_by_type(EXT_REPORT_GROUP).child_by_type(EXT_REPORT_NEXT).reported<uint16_t>());
    // Stopping an outstanding Get creates an empty group. The walk would also write a desired identifier.
    EXPECT_FALSE(target.child_by_type(EXT_GET_GROUP).child_by_type(EXT_GET_ID).desired_exists());
}

// Spec 2026B Extended User Code Report. A later report for the same user replaces that block.
TEST_F(UserCodeCommandClassTest, ExtendedReportReplacesTheStoredUserBlock)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);
    ASSERT_EQ(SL_STATUS_OK, receive(extended_report(USER_IDENTIFIER_1, STATUS_OCCUPIED, ASCII_CODE, NEXT_USER_LSB_NONE), target));
    ASSERT_EQ(SL_STATUS_OK, receive(extended_report(USER_IDENTIFIER_2, STATUS_AVAILABLE, {}, NEXT_USER_LSB_NONE), target));

    ASSERT_EQ(SL_STATUS_OK, receive(extended_report(USER_IDENTIFIER_1, STATUS_AVAILABLE, {}, NEXT_USER_LSB_NONE), target));

    const auto blocks = target.child_by_type(EXT_REPORT_GROUP).children(EXT_REPORT_VG1);
    ASSERT_EQ(STORED_BLOCK_COUNT, std::distance(blocks.begin(), blocks.end()));
    bool replaced = false;
    for (auto block: blocks) {
        const auto stored = block.reported<std::vector<uint8_t>>();
        if (stored.size() >= MINIMUM_STORED_BLOCK_BYTES && stored[STORED_IDENTIFIER_LSB_INDEX] == USER_IDENTIFIER_1) {
            replaced = stored[STORED_STATUS_INDEX] == STATUS_AVAILABLE;
        }
    }
    EXPECT_TRUE(replaced);
}

// Spec 2026B User Code Checksum Report. Checksum byte 1 is the MSB and byte 2 is the LSB.
TEST_F(UserCodeCommandClassTest, ChecksumReportStoresMostSignificantByteFirst)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);

    ASSERT_EQ(SL_STATUS_OK, receive({USER_CODE_CC, USER_CODE_CHECKSUM_REPORT, CHECKSUM_MSB, CHECKSUM_LSB}, target));

    EXPECT_EQ(CHECKSUM_STORED, target.child_by_type(CHECKSUM_REPORT_GROUP).child_by_type(CHECKSUM_VALUE).reported<uint16_t>());
}

// Spec 2026B User Code Get (CC:0063.01.02.11.004).
// Identifier 0 is not a User Code Get.
TEST_F(UserCodeCommandClassTest, UserCodeGetRejectsIdentifierZero)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    store_users(target, TWO_SUPPORTED_USERS, NO_EXTENDED_TOTAL);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(V1_GET_GROUP);
    group.emplace_node(V1_GET_ID).set_desired(USER_IDENTIFIER_ZERO);

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, USER_CODE_GET, frame, &frame_length, false));
}

// Spec 2026B User Code Get (CC:0063.01.02.12.001).
// An identifier above the stored user total is refused.
TEST_F(UserCodeCommandClassTest, UserCodeGetRejectsIdentifierAboveStoredTotal)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    store_users(target, TWO_SUPPORTED_USERS, NO_EXTENDED_TOTAL);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(V1_GET_GROUP);
    group.emplace_node(V1_GET_ID).set_desired(UNSUPPORTED_USER_IDENTIFIER);

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, USER_CODE_GET, frame, &frame_length, false));
}

// Spec 2026B User Code Get (CC:0063.01.02.11.004). A supported User Identifier is carried in the Get.
TEST_F(UserCodeCommandClassTest, UserCodeGetSendsSupportedIdentifier)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    store_users(target, TWO_SUPPORTED_USERS, NO_EXTENDED_TOTAL);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(V1_GET_GROUP);
    group.emplace_node(V1_GET_ID).set_desired(USER_IDENTIFIER_1);

    ASSERT_EQ(SL_STATUS_OK, assemble(group, USER_CODE_GET, frame, &frame_length, false));

    // Byte 1 is the command. Byte 2 is the User Identifier.
    EXPECT_EQ(USER_CODE_GET, frame[COMMAND_INDEX]);
    EXPECT_EQ(USER_IDENTIFIER_1, frame[V1_USER_IDENTIFIER_INDEX]);
}

// A Get group with no desired identifier is not ready to send.
TEST_F(UserCodeCommandClassTest, UserCodeGetNotReadyWithoutIdentifier)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(V1_GET_GROUP);

    EXPECT_EQ(SL_STATUS_NOT_READY, assemble(group, USER_CODE_GET, frame, &frame_length, false));
}

// Spec 2026B User Code Set (CC:0063.01.01.11.007).
// A non-digit byte is not a version 1 user code.
TEST_F(UserCodeCommandClassTest, UserCodeSetRejectsNonDigit)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    store_users(target, TWO_SUPPORTED_USERS, NO_EXTENDED_TOTAL);
    const std::vector<uint8_t> non_digit {NON_DIGIT, DIGIT_1, DIGIT_2, DIGIT_3};
    auto group = user_code_set_group(target, USER_IDENTIFIER_1, STATUS_OCCUPIED, non_digit);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, USER_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B User Code Set (CC:0063.01.01.11.007).
// Four ASCII digits are sent for an occupied user.
TEST_F(UserCodeCommandClassTest, UserCodeSetSendsOccupiedDigits)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    store_users(target, TWO_SUPPORTED_USERS, NO_EXTENDED_TOTAL);
    auto group = user_code_set_group(target, USER_IDENTIFIER_1, STATUS_OCCUPIED, ASCII_CODE);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    ASSERT_EQ(SL_STATUS_OK, assemble(group, USER_CODE_SET, frame, &frame_length, true));

    EXPECT_EQ(USER_CODE_SET, frame[COMMAND_INDEX]);
    // The first User Code byte follows the identifier and the status.
    EXPECT_EQ(DIGIT_1, frame[V1_CODE_INDEX]);
}

// Spec 2026B User Code Set (CC:0063.01.01.11.003).
// An identifier above the stored total is not sent.
TEST_F(UserCodeCommandClassTest, UserCodeSetRejectsIdentifierAboveStoredTotal)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    store_users(target, TWO_SUPPORTED_USERS, NO_EXTENDED_TOTAL);
    auto group = user_code_set_group(target, UNSUPPORTED_USER_IDENTIFIER, STATUS_OCCUPIED, ASCII_CODE);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, USER_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B erase-all (CL:0063.01.31.04.1, CC:0063.01.01.11.009).
// Identifier 0 with status Available and four 0x00 bytes erases every user code.
TEST_F(UserCodeCommandClassTest, UserCodeSetSendsEraseAll)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    store_users(target, TWO_SUPPORTED_USERS, NO_EXTENDED_TOTAL);
    auto group = user_code_set_group(target, USER_IDENTIFIER_ZERO, STATUS_AVAILABLE, ERASED_CODE);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    ASSERT_EQ(SL_STATUS_OK, assemble(group, USER_CODE_SET, frame, &frame_length, true));

    EXPECT_EQ(USER_IDENTIFIER_ZERO, frame[V1_USER_IDENTIFIER_INDEX]);
    EXPECT_EQ(STATUS_AVAILABLE, frame[V1_STATUS_INDEX]);
}

// A Set group that is still missing a field is not sent.
TEST_F(UserCodeCommandClassTest, UserCodeSetNotReadyWhenTheGroupIsEmpty)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(V1_SET_GROUP);

    EXPECT_EQ(SL_STATUS_NOT_READY, assemble(group, USER_CODE_SET, frame, &frame_length, true));
}

// Identifier alone is not enough to send User Code Set.
TEST_F(UserCodeCommandClassTest, UserCodeSetNotReadyWithOnlyTheIdentifier)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(V1_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::user_identifier)).set_desired(USER_IDENTIFIER_1);

    EXPECT_EQ(SL_STATUS_NOT_READY, assemble(group, USER_CODE_SET, frame, &frame_length, true));
}

// Status without a code is not enough to send User Code Set.
TEST_F(UserCodeCommandClassTest, UserCodeSetNotReadyWithoutTheCode)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(V1_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::user_identifier)).set_desired(USER_IDENTIFIER_1);
    group.emplace_node(static_cast<attribute_store_type_t>(user_code_set_group_attributes_t::user_id_status)).set_desired(STATUS_OCCUPIED);

    EXPECT_EQ(SL_STATUS_NOT_READY, assemble(group, USER_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B Available code (CC:0063.01.01.11.009).
// Available is exactly four 0x00 bytes, so one 0x00 byte is refused.
TEST_F(UserCodeCommandClassTest, UserCodeSetRejectsShortEraseCode)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    store_users(target, TWO_SUPPORTED_USERS, NO_EXTENDED_TOTAL);
    const std::vector<uint8_t> short_erase {LENGTH_ZERO};
    auto group = user_code_set_group(target, USER_IDENTIFIER_1, STATUS_AVAILABLE, short_erase);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, USER_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B Available code (CC:0063.01.01.11.009).
// Four bytes that are not all 0x00 are not an erased code.
TEST_F(UserCodeCommandClassTest, UserCodeSetRejectsEraseCodeThatIsNotAllZero)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    store_users(target, TWO_SUPPORTED_USERS, NO_EXTENDED_TOTAL);
    const std::vector<uint8_t> mixed {LENGTH_ZERO, LENGTH_ZERO, LENGTH_ZERO, DIGIT_1};
    auto group = user_code_set_group(target, USER_IDENTIFIER_1, STATUS_AVAILABLE, mixed);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, USER_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B User ID Status (CC:0063.01.01.11.005).
// A status outside the defined values is refused.
TEST_F(UserCodeCommandClassTest, UserCodeSetRejectsStatusOutsideTheDefinedRange)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    store_users(target, TWO_SUPPORTED_USERS, NO_EXTENDED_TOTAL);
    auto group = user_code_set_group(target, USER_IDENTIFIER_1, STATUS_OUT_OF_RANGE, ASCII_CODE);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, USER_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B erase-all (CC:0063.01.01.11.002).
// Identifier 0 is only valid with status Available.
TEST_F(UserCodeCommandClassTest, UserCodeSetRejectsEraseAllWithOccupiedStatus)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    store_users(target, TWO_SUPPORTED_USERS, NO_EXTENDED_TOTAL);
    auto group = user_code_set_group(target, USER_IDENTIFIER_ZERO, STATUS_OCCUPIED, ASCII_CODE);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, USER_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B User ID Status (CC:0063.01.01.11.004). Reserved by administrator is a defined status and its digits are sent.
TEST_F(UserCodeCommandClassTest, UserCodeSetSendsReservedStatus)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    store_users(target, TWO_SUPPORTED_USERS, NO_EXTENDED_TOTAL);
    auto group = user_code_set_group(target, USER_IDENTIFIER_1, STATUS_RESERVED, ASCII_CODE);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    ASSERT_EQ(SL_STATUS_OK, assemble(group, USER_CODE_SET, frame, &frame_length, true));

    EXPECT_EQ(STATUS_RESERVED, frame[V1_STATUS_INDEX]);
}

// Spec 2026B User Code length (CC:0063.01.01.11.006). A byte below ASCII '0' is refused.
TEST_F(UserCodeCommandClassTest, UserCodeSetRejectsByteBelowTheDigitRange)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    store_users(target, TWO_SUPPORTED_USERS, NO_EXTENDED_TOTAL);
    const std::vector<uint8_t> below {BELOW_DIGIT, DIGIT_1, DIGIT_2, DIGIT_3};
    auto group = user_code_set_group(target, USER_IDENTIFIER_1, STATUS_OCCUPIED, below);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, USER_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B User Code length (CC:0063.01.01.11.006). Three digits are shorter than the minimum.
TEST_F(UserCodeCommandClassTest, UserCodeSetRejectsCodeShorterThanFourDigits)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    store_users(target, TWO_SUPPORTED_USERS, NO_EXTENDED_TOTAL);
    const std::vector<uint8_t> short_code {DIGIT_1, DIGIT_2, DIGIT_3};
    auto group = user_code_set_group(target, USER_IDENTIFIER_1, STATUS_OCCUPIED, short_code);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, USER_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B User Code length (CC:0063.01.01.11.006). Eleven digits are longer than the maximum.
TEST_F(UserCodeCommandClassTest, UserCodeSetRejectsCodeLongerThanTenDigits)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    store_users(target, TWO_SUPPORTED_USERS, NO_EXTENDED_TOTAL);
    const std::vector<uint8_t> long_code(CODE_LENGTH_TOO_LONG, DIGIT_1);
    auto group = user_code_set_group(target, USER_IDENTIFIER_1, STATUS_OCCUPIED, long_code);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, USER_CODE_SET, frame, &frame_length, true));
}
// Spec 2026B Extended User Code Set (CC:0063.02.0B.11.002, CC:0063.02.07.11.006).
// Two blocks are refused when MUCS is stored clear.
TEST_F(UserCodeCommandClassTest, ExtendedUserCodeSetRejectsTwoBlocksWhenMucsIsClear)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_ONE_MODE, SUPPORTED_USERS_SATURATED, EXTENDED_TOTAL);
    auto blocks = available_block(USER_IDENTIFIER_1);
    auto second = available_block(USER_IDENTIFIER_2);
    blocks.insert(blocks.end(), second.begin(), second.end());
    auto group = extended_set_group(target, TWO_USER_CODES, blocks);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, EXTENDED_USER_CODE_SET, frame, &frame_length, true));
    EXPECT_EQ(LENGTH_ZERO, target.child_by_type(CAP_GROUP).child_by_type(MUCS_SUPPORT).reported<uint8_t>());
}

// Spec 2026B Extended User Code Set (CC:0063.02.05.11.003, CC:0063.02.0B.11.006).
// Identifier 256 is the extended total, so it is still supported. The count byte is 1.
TEST_F(UserCodeCommandClassTest, ExtendedUserCodeSetSendsIdentifierAtTheExtendedTotal)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_ONE_MODE, SUPPORTED_USERS_SATURATED, EXTENDED_TOTAL);
    auto group  = extended_set_group(target, ONE_USER_CODE, occupied_block(EXTENDED_TOTAL_MSB, EXTENDED_TOTAL_LSB));
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    ASSERT_EQ(SL_STATUS_OK, assemble(group, EXTENDED_USER_CODE_SET, frame, &frame_length, true));

    EXPECT_EQ(EXTENDED_USER_CODE_SET, frame[COMMAND_INDEX]);
    EXPECT_EQ(ONE_USER_CODE, frame[EXTENDED_USER_COUNT_INDEX]);
}

// Spec 2026B Extended User Code Set (CC:0063.02.0B.11.006).
// Identifier 257 is past the extended total of 256.
TEST_F(UserCodeCommandClassTest, ExtendedUserCodeSetRejectsIdentifierPastTheExtendedTotal)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_ONE_MODE, SUPPORTED_USERS_SATURATED, EXTENDED_TOTAL);
    auto group  = extended_set_group(target, ONE_USER_CODE, occupied_block(IDENTIFIER_PAST_TOTAL_MSB, IDENTIFIER_PAST_TOTAL_LSB));
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, EXTENDED_USER_CODE_SET, frame, &frame_length, true));
}

// An Extended User Code Set group with no fields is not ready.
TEST_F(UserCodeCommandClassTest, ExtendedUserCodeSetNotReadyWhenTheGroupIsEmpty)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(EXT_SET_GROUP);

    EXPECT_EQ(SL_STATUS_NOT_READY, assemble(group, EXTENDED_USER_CODE_SET, frame, &frame_length, true));
}

// A count without the block bytes is not ready.
TEST_F(UserCodeCommandClassTest, ExtendedUserCodeSetNotReadyWithoutTheBlocks)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, static_cast<uint8_t>(MUCS_SUPPORT_MASK | MASK_LENGTH_ONE), KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(EXT_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::number_of_user_codes)).set_desired(ONE_USER_CODE);

    EXPECT_EQ(SL_STATUS_NOT_READY, assemble(group, EXTENDED_USER_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B Extended User Code Set (CC:0063.02.0B.11.001). The block count must not be 0.
TEST_F(UserCodeCommandClassTest, ExtendedUserCodeSetRejectsZeroBlockCount)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, static_cast<uint8_t>(MUCS_SUPPORT_MASK | MASK_LENGTH_ONE), KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    auto group  = extended_set_group(target, ZERO_USER_CODES, available_block(USER_IDENTIFIER_1));
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, EXTENDED_USER_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B Extended User Code Set (CC:0063.02.0B.11.003). A block shorter than its header is refused.
TEST_F(UserCodeCommandClassTest, ExtendedUserCodeSetRejectsTruncatedHeader)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, static_cast<uint8_t>(MUCS_SUPPORT_MASK | MASK_LENGTH_ONE), KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    const std::vector<uint8_t> header_only {IDENTIFIER_MSB_ZERO, USER_IDENTIFIER_1};
    auto group = extended_set_group(target, ONE_USER_CODE, header_only);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, EXTENDED_USER_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B Extended User Code Set (CC:0063.02.0B.11.009). The reserved nibble above the length must be 0.
TEST_F(UserCodeCommandClassTest, ExtendedUserCodeSetRejectsReservedLengthBits)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, static_cast<uint8_t>(MUCS_SUPPORT_MASK | MASK_LENGTH_ONE), KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    const std::vector<uint8_t> reserved {IDENTIFIER_MSB_ZERO, USER_IDENTIFIER_1, STATUS_OCCUPIED, RESERVED_LENGTH_NIBBLE, DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4};
    auto group = extended_set_group(target, ONE_USER_CODE, reserved);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, EXTENDED_USER_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B Extended User Code Set (CC:0063.02.0B.11.003). The code must contain the advertised number of bytes.
TEST_F(UserCodeCommandClassTest, ExtendedUserCodeSetRejectsCodeShorterThanAdvertisedLength)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, static_cast<uint8_t>(MUCS_SUPPORT_MASK | MASK_LENGTH_ONE), KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    const std::vector<uint8_t> short_code {IDENTIFIER_MSB_ZERO, USER_IDENTIFIER_1, STATUS_OCCUPIED, ADMIN_CODE_LENGTH, DIGIT_1};
    auto group = extended_set_group(target, ONE_USER_CODE, short_code);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, EXTENDED_USER_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B Extended User Code Set (CC:0063.02.0B.11.00A). Available requires length 0 and no code bytes.
TEST_F(UserCodeCommandClassTest, ExtendedUserCodeSetRejectsAvailableStatusWithACode)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, static_cast<uint8_t>(MUCS_SUPPORT_MASK | MASK_LENGTH_ONE), KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    const std::vector<uint8_t> available {IDENTIFIER_MSB_ZERO, USER_IDENTIFIER_1, STATUS_AVAILABLE, ADMIN_CODE_LENGTH, DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4};
    auto group = extended_set_group(target, ONE_USER_CODE, available);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, EXTENDED_USER_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B User ID Status (CC:0063.02.0B.11.008). A status outside the defined values is refused.
TEST_F(UserCodeCommandClassTest, ExtendedUserCodeSetRejectsStatusOutsideTheDefinedRange)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, static_cast<uint8_t>(MUCS_SUPPORT_MASK | MASK_LENGTH_ONE), KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    const std::vector<uint8_t> unknown {IDENTIFIER_MSB_ZERO, USER_IDENTIFIER_1, STATUS_OUT_OF_RANGE, ADMIN_CODE_LENGTH, DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4};
    auto group = extended_set_group(target, ONE_USER_CODE, unknown);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, EXTENDED_USER_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B Supported User ID Status Bit Mask (CC:0063.02.07.11.00A).
// Passage is in range, but its bit is clear in the stored mask.
TEST_F(UserCodeCommandClassTest, ExtendedUserCodeSetRejectsPassageWhenTheStatusMaskOmitsIt)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, static_cast<uint8_t>(MUCS_SUPPORT_MASK | MASK_LENGTH_ONE), KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    const std::vector<uint8_t> passage {IDENTIFIER_MSB_ZERO, USER_IDENTIFIER_1, STATUS_PASSAGE, ADMIN_CODE_LENGTH, DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4};
    auto group = extended_set_group(target, ONE_USER_CODE, passage);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, EXTENDED_USER_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B User Code length (CC:0063.02.0B.11.00A). Length 3 is below the minimum of 4.
TEST_F(UserCodeCommandClassTest, ExtendedUserCodeSetRejectsLengthThree)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, static_cast<uint8_t>(MUCS_SUPPORT_MASK | MASK_LENGTH_ONE), KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    const std::vector<uint8_t> short_code {IDENTIFIER_MSB_ZERO, USER_IDENTIFIER_1, STATUS_OCCUPIED, CODE_LENGTH_TOO_SHORT, DIGIT_1, DIGIT_2, DIGIT_3};
    auto group = extended_set_group(target, ONE_USER_CODE, short_code);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, EXTENDED_USER_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B User Code length (CC:0063.02.0B.11.00A). Length 11 is above the maximum of 10.
TEST_F(UserCodeCommandClassTest, ExtendedUserCodeSetRejectsLengthEleven)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, static_cast<uint8_t>(MUCS_SUPPORT_MASK | MASK_LENGTH_ONE), KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    std::vector<uint8_t> long_code {IDENTIFIER_MSB_ZERO, USER_IDENTIFIER_1, STATUS_OCCUPIED, CODE_LENGTH_TOO_LONG};
    long_code.insert(long_code.end(), CODE_LENGTH_TOO_LONG, DIGIT_1);
    auto group = extended_set_group(target, ONE_USER_CODE, long_code);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, EXTENDED_USER_CODE_SET, frame, &frame_length, true));
}

// An Extended User Code Get without its fields is not ready.
TEST_F(UserCodeCommandClassTest, ExtendedUserCodeGetNotReadyWithoutFields)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(EXT_GET_GROUP);

    EXPECT_EQ(SL_STATUS_NOT_READY, assemble(group, EXTENDED_USER_CODE_GET, frame, &frame_length, false));
}

// Report More without a User Identifier is not ready.
TEST_F(UserCodeCommandClassTest, ExtendedUserCodeGetNotReadyWithoutTheIdentifier)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(EXT_GET_GROUP);
    group.emplace_node(EXT_GET_MORE).set_desired(REPORT_MORE_NOT_REQUESTED);

    EXPECT_EQ(SL_STATUS_NOT_READY, assemble(group, EXTENDED_USER_CODE_GET, frame, &frame_length, false));
}

// Spec 2026B Extended User Code Get (CC:0063.02.0C.11.004). Identifier 0 must not be requested.
TEST_F(UserCodeCommandClassTest, ExtendedUserCodeGetRejectsIdentifierZero)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(EXT_GET_GROUP);
    group.emplace_node(EXT_GET_ID).set_desired(static_cast<uint16_t>(USER_IDENTIFIER_ZERO));
    group.emplace_node(EXT_GET_MORE).set_desired(REPORT_MORE_NOT_REQUESTED);

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, EXTENDED_USER_CODE_GET, frame, &frame_length, false));
}

// Spec 2026B Extended User Code Get (CC:0063.02.0C.11.004).
// The identifier is sent MSB then LSB, followed by Report More.
TEST_F(UserCodeCommandClassTest, ExtendedUserCodeGetSendsSupportedIdentifier)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(EXT_GET_GROUP);
    group.emplace_node(EXT_GET_ID).set_desired(static_cast<uint16_t>(USER_IDENTIFIER_1));
    group.emplace_node(EXT_GET_MORE).set_desired(REPORT_MORE_NOT_REQUESTED);

    ASSERT_EQ(SL_STATUS_OK, assemble(group, EXTENDED_USER_CODE_GET, frame, &frame_length, false));

    EXPECT_EQ(EXTENDED_USER_CODE_GET, frame[COMMAND_INDEX]);
    EXPECT_EQ(IDENTIFIER_MSB_ZERO, frame[EXTENDED_IDENTIFIER_MSB_INDEX]);
    EXPECT_EQ(USER_IDENTIFIER_1, frame[EXTENDED_IDENTIFIER_LSB_INDEX]);
    EXPECT_EQ(REPORT_MORE_NOT_REQUESTED, frame[EXTENDED_REPORT_MORE_INDEX]);
}

// Spec 2026B Extended User Code Get (CC:0063.02.0C.11.006). An identifier above the stored total is refused.
TEST_F(UserCodeCommandClassTest, ExtendedUserCodeGetRejectsIdentifierAboveStoredTotal)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(EXT_GET_GROUP);
    group.emplace_node(EXT_GET_ID).set_desired(static_cast<uint16_t>(UNSUPPORTED_USER_IDENTIFIER));
    group.emplace_node(EXT_GET_MORE).set_desired(REPORT_MORE_NOT_REQUESTED);

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, EXTENDED_USER_CODE_GET, frame, &frame_length, false));
}

// Spec 2026B keypad mode (CL:0063.01.31.05.1). One stored mode means the set action is not available.
TEST_F(UserCodeCommandClassTest, KeypadModeSetRejectedWhenOnlyOneModeIsStored)
{
    auto target = version_two_peer(static_cast<uint8_t>(AC_SUPPORT_MASK | MASK_LENGTH_ONE), MASK_LENGTH_ONE, KEYPAD_ONE_MODE, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(KEYPAD_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(user_code_keypad_mode_set_group_attributes_t::keypad_mode)).set_desired(KEYPAD_NORMAL);

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, KEYPAD_MODE_SET, frame, &frame_length, true));
}

// A keypad set without a mode is not ready.
TEST_F(UserCodeCommandClassTest, KeypadModeSetNotReadyWithoutTheMode)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(KEYPAD_SET_GROUP);

    EXPECT_EQ(SL_STATUS_NOT_READY, assemble(group, KEYPAD_MODE_SET, frame, &frame_length, true));
}

// Spec 2026B keypad mode (CC:0063.02.07.11.010). A mode whose bit is clear is refused.
TEST_F(UserCodeCommandClassTest, KeypadModeSetRejectsModeOutsideTheMask)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, static_cast<uint8_t>(UCC_SUPPORT_MASK | MASK_LENGTH_ONE), KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(KEYPAD_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(user_code_keypad_mode_set_group_attributes_t::keypad_mode)).set_desired(KEYPAD_MODE_OUTSIDE_MASK);

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, KEYPAD_MODE_SET, frame, &frame_length, true));
}

// Spec 2026B keypad mode (CL:0063.01.31.05.1). Vacation is bit 1 of a two-mode mask and is sent.
TEST_F(UserCodeCommandClassTest, KeypadModeSetSendsVacationMode)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(KEYPAD_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(user_code_keypad_mode_set_group_attributes_t::keypad_mode)).set_desired(KEYPAD_VACATION);

    ASSERT_EQ(SL_STATUS_OK, assemble(group, KEYPAD_MODE_SET, frame, &frame_length, true));

    // Byte 2 is the Keypad Mode.
    EXPECT_EQ(KEYPAD_VACATION, frame[KEYPAD_MODE_INDEX]);
}

// Spec 2026B Admin Code Set (CL:0063.01.31.06.1, CC:0063.02.0E.11.001).
// A stored AC bit allows the set, and the length is the first value byte.
TEST_F(UserCodeCommandClassTest, AdminCodeSetSendsWhenAdminCodeIsSupported)
{
    auto target = version_two_peer(static_cast<uint8_t>(AC_SUPPORT_MASK | MASK_LENGTH_ONE), MASK_LENGTH_ONE, KEYPAD_ONE_MODE, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(ADMIN_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code_length)).set_desired(ADMIN_CODE_LENGTH);
    group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code)).set_desired(ASCII_CODE);

    ASSERT_EQ(SL_STATUS_OK, assemble(group, ADMIN_CODE_SET, frame, &frame_length, true));

    EXPECT_EQ(ADMIN_CODE_SET, frame[COMMAND_INDEX]);
    EXPECT_EQ(ADMIN_CODE_LENGTH, frame[ADMIN_LENGTH_INDEX]);
}

// Spec 2026B Admin Code Set (CC:0063.02.0E.11.005). Length 0 is refused when ACD is stored clear.
TEST_F(UserCodeCommandClassTest, AdminCodeSetRejectsDeactivationWhenAcdIsClear)
{
    auto target = version_two_peer(static_cast<uint8_t>(AC_SUPPORT_MASK | MASK_LENGTH_ONE), MASK_LENGTH_ONE, KEYPAD_ONE_MODE, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(ADMIN_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code_length)).set_desired(LENGTH_ZERO);

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, ADMIN_CODE_SET, frame, &frame_length, true));
}

// An Admin Code Set without a length is not ready.
TEST_F(UserCodeCommandClassTest, AdminCodeSetNotReadyWithoutTheLength)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(ADMIN_SET_GROUP);

    EXPECT_EQ(SL_STATUS_NOT_READY, assemble(group, ADMIN_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B Admin Code Set (CC:0063.02.0E.11.006). The code length must match the length field.
TEST_F(UserCodeCommandClassTest, AdminCodeSetRejectsLengthMismatch)
{
    auto target = version_two_peer(static_cast<uint8_t>(AC_SUPPORT_MASK | ACD_SUPPORT_MASK | MASK_LENGTH_ONE), MASK_LENGTH_ONE, KEYPAD_ONE_MODE, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(ADMIN_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code_length)).set_desired(CODE_LENGTH_MISMATCH);
    group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code)).set_desired(ASCII_CODE);

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, ADMIN_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B Admin Code Set (CC:0063.02.0E.11.005).
// Length 0 with ACD set sends only the length. An empty code is not stored, so there is no code child.
TEST_F(UserCodeCommandClassTest, AdminCodeSetSendsDeactivationWhenAcdIsSet)
{
    auto target = version_two_peer(static_cast<uint8_t>(AC_SUPPORT_MASK | ACD_SUPPORT_MASK | MASK_LENGTH_ONE), MASK_LENGTH_ONE, KEYPAD_ONE_MODE, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(ADMIN_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code_length)).set_desired(LENGTH_ZERO);
    group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code)).set_desired(std::vector<uint8_t> {});

    ASSERT_EQ(SL_STATUS_OK, assemble(group, ADMIN_CODE_SET, frame, &frame_length, true));

    EXPECT_EQ(std::vector<uint8_t>({USER_CODE_CC, ADMIN_CODE_SET, LENGTH_ZERO}), std::vector<uint8_t>(frame, frame + frame_length));
}

// Spec 2026B Admin Code length (CC:0063.02.0E.11.003). Length 11 is outside 0 and 4..10.
TEST_F(UserCodeCommandClassTest, AdminCodeSetRejectsLengthEleven)
{
    auto target = version_two_peer(static_cast<uint8_t>(AC_SUPPORT_MASK | ACD_SUPPORT_MASK | MASK_LENGTH_ONE), MASK_LENGTH_ONE, KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(ADMIN_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code_length)).set_desired(CODE_LENGTH_TOO_LONG);
    group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code)).set_desired(std::vector<uint8_t>(CODE_LENGTH_TOO_LONG, DIGIT_1));

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, ADMIN_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B Admin Code Set (CC:0063.02.0E.11.006). A code longer than the length field is refused.
TEST_F(UserCodeCommandClassTest, AdminCodeSetRejectsCodeLongerThanTheLength)
{
    auto target = version_two_peer(static_cast<uint8_t>(AC_SUPPORT_MASK | ACD_SUPPORT_MASK | MASK_LENGTH_ONE), MASK_LENGTH_ONE, KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    const std::vector<uint8_t> extra {DIGIT_1, DIGIT_2, DIGIT_3, DIGIT_4, DIGIT_1};
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(ADMIN_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code_length)).set_desired(ADMIN_CODE_LENGTH);
    group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code)).set_desired(extra);

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, ADMIN_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B Supported Keys (CC:0063.02.07.11.012, CC:0063.02.0E.11.007).
// With no keys mask, a non-printable Admin Code is refused.
TEST_F(UserCodeCommandClassTest, AdminCodeSetRejectsNonPrintableWithoutAKeysMask)
{
    auto target = version_two_peer(static_cast<uint8_t>(AC_SUPPORT_MASK | MASK_LENGTH_ONE), MASK_LENGTH_ONE, KEYPAD_ONE_MODE, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    const std::vector<uint8_t> raw_key(ADMIN_CODE_LENGTH, NON_PRINTABLE_BYTE);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(ADMIN_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code_length)).set_desired(ADMIN_CODE_LENGTH);
    group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code)).set_desired(raw_key);

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, ADMIN_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B Supported Keys Bit Mask (CC:0063.02.07.11.015).
// Bit 1 of the first mask byte is ASCII code 1, so four of those bytes are accepted.
TEST_F(UserCodeCommandClassTest, AdminCodeSetAcceptsByteSelectedByTheKeysMask)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);
    ASSERT_EQ(SL_STATUS_OK, receive(with_keys_mask(static_cast<uint8_t>(AC_SUPPORT_MASK | MASK_LENGTH_ONE), MASK_LENGTH_ONE, KEYPAD_ONE_MODE, KEYS_MASK_ASCII_1), target));
    const std::vector<uint8_t> raw_key(ADMIN_CODE_LENGTH, NON_PRINTABLE_BYTE);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(ADMIN_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code_length)).set_desired(ADMIN_CODE_LENGTH);
    group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code)).set_desired(raw_key);

    ASSERT_EQ(SL_STATUS_OK, assemble(group, ADMIN_CODE_SET, frame, &frame_length, true));

    EXPECT_EQ(ADMIN_CODE_SET, frame[COMMAND_INDEX]);
}

// Spec 2026B Supported Keys Bit Mask (CC:0063.02.07.11.016). Digits are outside a mask that only allows ASCII 1.
TEST_F(UserCodeCommandClassTest, AdminCodeSetRejectsDigitOutsideTheKeysMask)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);
    ASSERT_EQ(SL_STATUS_OK, receive(with_keys_mask(static_cast<uint8_t>(AC_SUPPORT_MASK | MASK_LENGTH_ONE), MASK_LENGTH_ONE, KEYPAD_ONE_MODE, KEYS_MASK_ASCII_1), target));
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(ADMIN_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code_length)).set_desired(ADMIN_CODE_LENGTH);
    group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code)).set_desired(ASCII_CODE);

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, ADMIN_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B Admin Code (CC:0063.02.0E.11.007). ASCII 0x7F is outside the printable fallback used when the keys mask is absent.
TEST_F(UserCodeCommandClassTest, AdminCodeSetRejectsByteAbovePrintableAscii)
{
    auto target = version_two_peer(static_cast<uint8_t>(AC_SUPPORT_MASK | ACD_SUPPORT_MASK | MASK_LENGTH_ONE), MASK_LENGTH_ONE, KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    const std::vector<uint8_t> above(ADMIN_CODE_LENGTH, ABOVE_PRINTABLE);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(ADMIN_SET_GROUP);
    group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code_length)).set_desired(ADMIN_CODE_LENGTH);
    group.emplace_node(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code)).set_desired(above);

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, ADMIN_CODE_SET, frame, &frame_length, true));
}

// Spec 2026B User Code Checksum Get (CC:0063.02.11.11.001). A stored clear UCC bit refuses the Get.
TEST_F(UserCodeCommandClassTest, ChecksumGetRejectedWhenUccIsClear)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_ONE_MODE, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = FRAME_BUFFER_SIZE;
    auto group            = target.emplace_node(CHECKSUM_GET_GROUP);

    EXPECT_EQ(SL_STATUS_FAIL, assemble(group, USER_CODE_CHECKSUM_GET, frame, &frame_length, false));
}

// Spec 2026B User Code Checksum Get (CC:0063.02.11.11.002, CC:0000.00.00.11.002).
// A stored UCC bit sends the implemented two-byte Get.
TEST_F(UserCodeCommandClassTest, ChecksumGetSendsTheImplementedFrameWhenUccIsSet)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, static_cast<uint8_t>(UCC_SUPPORT_MASK | MASK_LENGTH_ONE), KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(CHECKSUM_GET_GROUP);

    ASSERT_EQ(SL_STATUS_OK, assemble(group, USER_CODE_CHECKSUM_GET, frame, &frame_length, false));

    EXPECT_EQ(std::vector<uint8_t>({USER_CODE_CC, USER_CODE_CHECKSUM_GET}), std::vector<uint8_t>(frame, frame + frame_length));
}

// Spec 2026B command version (CC:0000.00.00.11.002).
// Users Number Get keeps the implemented two-byte command for a version 1 peer.
TEST_F(UserCodeCommandClassTest, UsersNumberGetKeepsTheImplementedFrame)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(USERS_GET_GROUP);

    ASSERT_EQ(SL_STATUS_OK, assemble(group, USERS_NUMBER_GET, frame, &frame_length, false));

    EXPECT_EQ(std::vector<uint8_t>({USER_CODE_CC, USERS_NUMBER_GET}), std::vector<uint8_t>(frame, frame + frame_length));
}

// Spec 2026B command version (CC:0000.00.00.11.002).
// Capabilities Get keeps the implemented two-byte command for a version 2 peer.
TEST_F(UserCodeCommandClassTest, CapabilitiesGetKeepsTheImplementedFrame)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);
    uint8_t frame[FRAME_BUFFER_SIZE] {};
    uint16_t frame_length = 0;
    auto group            = target.emplace_node(CAP_GET_GROUP);

    ASSERT_EQ(SL_STATUS_OK, assemble(group, CAPABILITIES_GET, frame, &frame_length, false));

    EXPECT_EQ(std::vector<uint8_t>({USER_CODE_CC, CAPABILITIES_GET}), std::vector<uint8_t>(frame, frame + frame_length));
}

// Spec 2026B controlling set (CL:0063.01.31.02.1). Version 1 MQTT User Code Set stays User Code Set.
TEST_F(UserCodeCommandClassTest, MqttUserCodeSetVersionOneUsesUserCodeSet)
{
    auto target = versioned_peer(USER_CODE_VERSION_1);
    store_users(target, TWO_SUPPORTED_USERS, NO_EXTENDED_TOTAL);

    ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_user_code_set_command(target, user_set_json(USER_IDENTIFIER_1, STATUS_OCCUPIED, ASCII_CODE)));

    EXPECT_TRUE(target.child_by_type(V1_SET_GROUP).is_valid());
    EXPECT_FALSE(target.child_by_type(EXT_SET_GROUP).is_valid());
}

// Spec 2026B controlling set (CL:0063.01.31.02.1). Version 2 MQTT User Code Set becomes Extended User Code Set.
TEST_F(UserCodeCommandClassTest, MqttUserCodeSetVersionTwoUsesExtendedUserCodeSet)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);
    store_users(target, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_user_code_set_command(target, user_set_json(USER_IDENTIFIER_1, STATUS_OCCUPIED, ASCII_CODE)));

    EXPECT_TRUE(target.child_by_type(EXT_SET_GROUP).is_valid());
    EXPECT_FALSE(target.child_by_type(V1_SET_GROUP).is_valid());
}

// Spec 2026B controlling set (CL:0063.01.31.02.1). An unreported version is not version 2, so the set stays User Code Set.
TEST_F(UserCodeCommandClassTest, MqttUserCodeSetUnversionedUsesUserCodeSet)
{
    auto target = closed_endpoint();
    store_users(target, TWO_SUPPORTED_USERS, NO_EXTENDED_TOTAL);

    ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_user_code_set_command(target, user_set_json(USER_IDENTIFIER_1, STATUS_OCCUPIED, ASCII_CODE)));

    EXPECT_TRUE(target.child_by_type(V1_SET_GROUP).is_valid());
}

// Spec 2026B erase-all (CC:0063.01.01.11.002). MQTT refuses identifier 0 with an occupied code.
TEST_F(UserCodeCommandClassTest, MqttUserCodeSetRejectsEraseAllWithOccupiedStatus)
{
    auto target = closed_endpoint();
    store_users(target, TWO_SUPPORTED_USERS, NO_EXTENDED_TOTAL);

    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_user_code_set_command(target, user_set_json(USER_IDENTIFIER_ZERO, STATUS_OCCUPIED, ASCII_CODE)));
    EXPECT_FALSE(target.child_by_type(V1_SET_GROUP).is_valid());
}

// Spec 2026B erase-all (CL:0063.01.31.04.1). MQTT accepts identifier 0 with four 0x00 bytes.
TEST_F(UserCodeCommandClassTest, MqttUserCodeSetSendsEraseAll)
{
    auto target = closed_endpoint();
    store_users(target, TWO_SUPPORTED_USERS, NO_EXTENDED_TOTAL);

    ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_user_code_set_command(target, user_set_json(USER_IDENTIFIER_ZERO, STATUS_AVAILABLE, ERASED_CODE)));

    EXPECT_TRUE(target.child_by_type(V1_SET_GROUP).is_valid());
}

// Spec 2026B User Code Set (CC:0063.01.01.11.003). MQTT refuses an identifier above the stored total.
TEST_F(UserCodeCommandClassTest, MqttUserCodeSetRejectsIdentifierAboveStoredTotal)
{
    auto target = closed_endpoint();
    store_users(target, TWO_SUPPORTED_USERS, NO_EXTENDED_TOTAL);

    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_user_code_set_command(target, user_set_json(UNSUPPORTED_USER_IDENTIFIER, STATUS_OCCUPIED, ASCII_CODE)));
    EXPECT_FALSE(target.child_by_type(V1_SET_GROUP).is_valid());
}

// Spec 2026B version 2 Available code (CC:0063.02.0B.11.00A). Four 0x00 bytes are stored with the code omitted.
TEST_F(UserCodeCommandClassTest, MqttUserCodeSetVersionTwoAcceptsFourZeroAvailableCode)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);
    store_users(target, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_user_code_set_command(target, user_set_json(USER_IDENTIFIER_1, STATUS_AVAILABLE, ERASED_CODE)));

    EXPECT_EQ(available_block(USER_IDENTIFIER_1), target.child_by_type(EXT_SET_GROUP).child_by_type(static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::vg1)).desired<std::vector<uint8_t>>());
}

// Spec 2026B version 2 Available code (CC:0063.02.0B.11.00A).
// Status Available omits the code even when the payload is not four 0x00 bytes.
TEST_F(UserCodeCommandClassTest, MqttUserCodeSetVersionTwoOmitsNonZeroAvailableCode)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);
    store_users(target, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    const std::vector<uint8_t> one_digit {DIGIT_1};

    ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_user_code_set_command(target, user_set_json(USER_IDENTIFIER_1, STATUS_AVAILABLE, one_digit)));

    EXPECT_EQ(available_block(USER_IDENTIFIER_1), target.child_by_type(EXT_SET_GROUP).child_by_type(static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::vg1)).desired<std::vector<uint8_t>>());
}

// Spec 2026B version 2 Available code (CC:0063.02.0B.11.00A). An empty code with status Available is stored with the code omitted.
TEST_F(UserCodeCommandClassTest, MqttUserCodeSetVersionTwoAcceptsEmptyAvailableCode)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);
    store_users(target, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_user_code_set_command(target, user_set_json(USER_IDENTIFIER_1, STATUS_AVAILABLE, {})));

    EXPECT_EQ(available_block(USER_IDENTIFIER_1), target.child_by_type(EXT_SET_GROUP).child_by_type(static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::vg1)).desired<std::vector<uint8_t>>());
}

// A User Code Set payload without a status is rejected and opens no group.
TEST_F(UserCodeCommandClassTest, MqttUserCodeSetRejectsMissingStatus)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);

    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_user_code_set_command(target, std::string("{\"user_id_status\":") + std::to_string(STATUS_OCCUPIED) + "}"));
    EXPECT_FALSE(target.child_by_type(EXT_SET_GROUP).is_valid());
}

// Spec 2026B User Code Set (CC:0063.01.01.11.003). Version 2 also refuses an identifier above the stored total.
TEST_F(UserCodeCommandClassTest, MqttUserCodeSetVersionTwoRejectsIdentifierAboveStoredTotal)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);
    store_users(target, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_user_code_set_command(target, user_set_json(UNSUPPORTED_USER_IDENTIFIER, STATUS_OCCUPIED, ASCII_CODE)));
    EXPECT_FALSE(target.child_by_type(EXT_SET_GROUP).is_valid());
}

// Spec 2026B keypad mode (CL:0063.01.31.05.1). MQTT refuses the set when only one keypad mode is stored.
TEST_F(UserCodeCommandClassTest, MqttKeypadModeSetRejectedWhenOnlyOneModeIsStored)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_ONE_MODE, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_user_code_keypad_mode_set_command(target, std::string("{\"keypad_mode\":") + std::to_string(KEYPAD_NORMAL) + "}"));
    EXPECT_FALSE(target.child_by_type(KEYPAD_SET_GROUP).is_valid());
}

// Spec 2026B Admin Code (CL:0063.01.31.06.1, CC:0063.02.0E.11.001). MQTT refuses Admin Code Set when AC is stored clear.
TEST_F(UserCodeCommandClassTest, MqttAdminCodeSetRejectedWhenAcIsClear)
{
    auto target               = version_two_peer(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_ONE_MODE, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    const std::string payload = std::string("{\"properties1\":{\"admin_code_length\":") + std::to_string(ADMIN_CODE_LENGTH) + "},\"admin_code\":" + json_bytes(ASCII_CODE) + "}";

    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_admin_code_set_command(target, payload));
    EXPECT_FALSE(target.child_by_type(ADMIN_SET_GROUP).is_valid());
}

// Spec 2026B Extended User Code Set (CC:0063.02.0B.11.002). MQTT refuses two blocks when MUCS is stored clear.
TEST_F(UserCodeCommandClassTest, MqttExtendedUserCodeSetRejectedWhenMucsIsClear)
{
    auto target               = version_two_peer(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_ONE_MODE, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    const std::string payload = std::string("{\"vg1\":[{\"user_identifier\":") + std::to_string(USER_IDENTIFIER_1) + ",\"user_id_status\":" + std::to_string(STATUS_AVAILABLE) + ",\"user_code\":[]},{\"user_identifier\":" + std::to_string(USER_IDENTIFIER_2)
                                + ",\"user_id_status\":" + std::to_string(STATUS_AVAILABLE) + ",\"user_code\":[]}]}";

    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_extended_user_code_set_command(target, payload));
    EXPECT_FALSE(target.child_by_type(EXT_SET_GROUP).is_valid());
}

// Spec 2026B User Code Checksum Get (CC:0063.02.11.11.001). MQTT refuses the Get when UCC is stored clear.
TEST_F(UserCodeCommandClassTest, MqttChecksumGetRejectedWhenUccIsClear)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_ONE_MODE, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_user_code_checksum_get_command(target, "{}"));
    EXPECT_FALSE(target.child_by_type(CHECKSUM_GET_GROUP).is_valid());
}

// A missing capabilities report has not advertised a single keypad mode, so the set is accepted (CL:0063.01.31.05.1).
TEST_F(UserCodeCommandClassTest, MqttKeypadModeSetAllowedWhenCapabilitiesAreUnknown)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);

    ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_user_code_keypad_mode_set_command(target, std::string("{\"keypad_mode\":") + std::to_string(KEYPAD_NORMAL) + "}"));

    EXPECT_EQ(KEYPAD_NORMAL, target.child_by_type(KEYPAD_SET_GROUP).child_by_type(static_cast<attribute_store_type_t>(user_code_keypad_mode_set_group_attributes_t::keypad_mode)).desired<uint8_t>());
}

// A missing capabilities report has not advertised that Admin Code is absent (CC:0063.02.0E.11.001).
TEST_F(UserCodeCommandClassTest, MqttAdminCodeSetAllowedWhenCapabilitiesAreUnknown)
{
    auto target               = versioned_peer(USER_CODE_VERSION_2);
    const std::string payload = std::string("{\"properties1\":{\"admin_code_length\":") + std::to_string(ADMIN_CODE_LENGTH) + "},\"admin_code\":" + json_bytes(ASCII_CODE) + "}";

    ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_admin_code_set_command(target, payload));

    EXPECT_EQ(ADMIN_CODE_LENGTH, target.child_by_type(ADMIN_SET_GROUP).child_by_type(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code_length)).desired<uint8_t>());
}

// A missing capabilities report has not advertised that MUCS is absent (CC:0063.02.0B.11.002).
TEST_F(UserCodeCommandClassTest, MqttExtendedUserCodeSetAllowedWhenCapabilitiesAreUnknown)
{
    auto target               = versioned_peer(USER_CODE_VERSION_2);
    const std::string payload = std::string("{\"vg1\":[{\"user_identifier\":") + std::to_string(USER_IDENTIFIER_1) + ",\"user_id_status\":" + std::to_string(STATUS_AVAILABLE) + ",\"user_code\":[]},{\"user_identifier\":" + std::to_string(USER_IDENTIFIER_2)
                                + ",\"user_id_status\":" + std::to_string(STATUS_AVAILABLE) + ",\"user_code\":[]}]}";

    ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_extended_user_code_set_command(target, payload));

    EXPECT_EQ(TWO_USER_CODES, target.child_by_type(EXT_SET_GROUP).child_by_type(static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::number_of_user_codes)).desired<uint8_t>());
}

// Spec 2026B User Code Get (CC:0063.01.02.11.004). MQTT refuses identifier 0.
TEST_F(UserCodeCommandClassTest, MqttUserCodeGetRejectsIdentifierZero)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_ONE_MODE, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_user_code_get_command(target, std::string("{\"user_identifier\":") + std::to_string(USER_IDENTIFIER_ZERO) + "}"));
    EXPECT_FALSE(target.child_by_type(V1_GET_GROUP).is_valid());
}

// A User Code Get payload without an identifier is rejected.
TEST_F(UserCodeCommandClassTest, MqttUserCodeGetRejectsMissingIdentifier)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);

    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_user_code_get_command(target, "{}"));
    EXPECT_FALSE(target.child_by_type(V1_GET_GROUP).is_valid());
}

// Spec 2026B User Code Get (CC:0063.01.02.11.004). MQTT stores a non-zero requested identifier.
TEST_F(UserCodeCommandClassTest, MqttUserCodeGetStoresTheRequestedIdentifier)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_ONE_MODE, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_user_code_get_command(target, std::string("{\"user_identifier\":") + std::to_string(USER_IDENTIFIER_1) + "}"));

    EXPECT_EQ(USER_IDENTIFIER_1, target.child_by_type(V1_GET_GROUP).child_by_type(V1_GET_ID).desired<uint8_t>());
}

// Spec 2026B User Code Get (CC:0063.01.02.12.001). MQTT refuses an identifier above the stored total.
TEST_F(UserCodeCommandClassTest, MqttUserCodeGetRejectsIdentifierAboveStoredTotal)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_ONE_MODE, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_user_code_get_command(target, std::string("{\"user_identifier\":") + std::to_string(UNSUPPORTED_USER_IDENTIFIER) + "}"));
    EXPECT_FALSE(target.child_by_type(V1_GET_GROUP).is_valid());
}

// Spec 2026B Extended User Code Get (CC:0063.02.0C.11.004). MQTT refuses identifier 0.
TEST_F(UserCodeCommandClassTest, MqttExtendedUserCodeGetRejectsIdentifierZero)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_ONE_MODE, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_extended_user_code_get_command(target, std::string("{\"user_identifier\":") + std::to_string(USER_IDENTIFIER_ZERO) + "}"));
    EXPECT_FALSE(target.child_by_type(EXT_GET_GROUP).is_valid());
}

// An Extended User Code Get without an identifier is rejected.
TEST_F(UserCodeCommandClassTest, MqttExtendedUserCodeGetRejectsMissingIdentifier)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);

    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_extended_user_code_get_command(target, "{}"));
    EXPECT_FALSE(target.child_by_type(EXT_GET_GROUP).is_valid());
}

// Spec 2026B Extended User Code Get (CC:0063.02.0C.11.006). MQTT refuses an identifier above the stored total.
TEST_F(UserCodeCommandClassTest, MqttExtendedUserCodeGetRejectsIdentifierAboveStoredTotal)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_ONE_MODE, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_extended_user_code_get_command(target, std::string("{\"user_identifier\":") + std::to_string(UNSUPPORTED_USER_IDENTIFIER) + "}"));
    EXPECT_FALSE(target.child_by_type(EXT_GET_GROUP).is_valid());
}

// Spec 2026B Extended User Code Get (CC:0063.02.0C.11.008). MQTT stores Report More with the requested user.
TEST_F(UserCodeCommandClassTest, MqttExtendedUserCodeGetStoresReportMore)
{
    auto target               = version_two_peer(MASK_LENGTH_ONE, static_cast<uint8_t>(MUCR_SUPPORT_MASK | MASK_LENGTH_ONE), KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    const std::string payload = std::string("{\"user_identifier\":") + std::to_string(USER_IDENTIFIER_1) + ",\"report_more\":" + std::to_string(REPORT_MORE_REQUESTED) + "}";

    ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_extended_user_code_get_command(target, payload));

    EXPECT_EQ(REPORT_MORE_REQUESTED, target.child_by_type(EXT_GET_GROUP).child_by_type(EXT_GET_MORE).desired<uint8_t>());
}

// MQTT Users Number Get opens its command group.
TEST_F(UserCodeCommandClassTest, MqttUsersNumberGetOpensTheCommandGroup)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);

    ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_users_number_get_command(target, "{}"));

    EXPECT_TRUE(target.child_by_type(USERS_GET_GROUP).is_valid());
}

// MQTT Capabilities Get opens its command group.
TEST_F(UserCodeCommandClassTest, MqttCapabilitiesGetOpensTheCommandGroup)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);

    ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_user_code_capabilities_get_command(target, "{}"));

    EXPECT_TRUE(target.child_by_type(CAP_GET_GROUP).is_valid());
}

// MQTT Keypad Mode Get opens its command group.
TEST_F(UserCodeCommandClassTest, MqttKeypadModeGetOpensTheCommandGroup)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);

    ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_user_code_keypad_mode_get_command(target, "{}"));

    EXPECT_TRUE(target.child_by_type(KEYPAD_GET_GROUP).is_valid());
}

// MQTT Admin Code Get opens its command group.
TEST_F(UserCodeCommandClassTest, MqttAdminCodeGetOpensTheCommandGroup)
{
    auto target = versioned_peer(USER_CODE_VERSION_2);

    ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_admin_code_get_command(target, "{}"));

    EXPECT_TRUE(target.child_by_type(ADMIN_GET_GROUP).is_valid());
}

// Spec 2026B User Code Checksum Get (CC:0063.02.11.11.002). MQTT opens the Get when UCC is stored set.
TEST_F(UserCodeCommandClassTest, MqttChecksumGetOpensTheCommandGroupWhenUccIsSet)
{
    auto target = version_two_peer(static_cast<uint8_t>(AC_SUPPORT_MASK | MASK_LENGTH_ONE), static_cast<uint8_t>(UCC_SUPPORT_MASK | MASK_LENGTH_ONE), KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_user_code_checksum_get_command(target, "{}"));

    EXPECT_TRUE(target.child_by_type(CHECKSUM_GET_GROUP).is_valid());
}

// A keypad set without a mode is rejected.
TEST_F(UserCodeCommandClassTest, MqttKeypadModeSetRejectsMissingMode)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_user_code_keypad_mode_set_command(target, "{}"));
    EXPECT_FALSE(target.child_by_type(KEYPAD_SET_GROUP).is_valid());
}

// Spec 2026B keypad mode (CC:0063.02.07.11.010). MQTT refuses a mode whose bit is clear.
TEST_F(UserCodeCommandClassTest, MqttKeypadModeSetRejectsModeOutsideTheMask)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_user_code_keypad_mode_set_command(target, std::string("{\"keypad_mode\":") + std::to_string(KEYPAD_MODE_OUTSIDE_MASK) + "}"));
    EXPECT_FALSE(target.child_by_type(KEYPAD_SET_GROUP).is_valid());
}

// Spec 2026B keypad mode (CL:0063.01.31.05.1). MQTT stores vacation mode when that bit is set.
TEST_F(UserCodeCommandClassTest, MqttKeypadModeSetStoresVacationMode)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, MASK_LENGTH_ONE, KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_user_code_keypad_mode_set_command(target, std::string("{\"keypad_mode\":") + std::to_string(KEYPAD_VACATION) + "}"));

    EXPECT_EQ(KEYPAD_VACATION, target.child_by_type(KEYPAD_SET_GROUP).child_by_type(static_cast<attribute_store_type_t>(user_code_keypad_mode_set_group_attributes_t::keypad_mode)).desired<uint8_t>());
}

// An Admin Code Set without a length is rejected.
TEST_F(UserCodeCommandClassTest, MqttAdminCodeSetRejectsMissingLength)
{
    auto target = version_two_peer(static_cast<uint8_t>(AC_SUPPORT_MASK | ACD_SUPPORT_MASK | MASK_LENGTH_ONE), MASK_LENGTH_ONE, KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_admin_code_set_command(target, "{}"));
    EXPECT_FALSE(target.child_by_type(ADMIN_SET_GROUP).is_valid());
}

// Spec 2026B Admin Code Set (CC:0063.02.0E.11.006). MQTT refuses a length that does not match the code.
TEST_F(UserCodeCommandClassTest, MqttAdminCodeSetRejectsLengthMismatch)
{
    auto target               = version_two_peer(static_cast<uint8_t>(AC_SUPPORT_MASK | ACD_SUPPORT_MASK | MASK_LENGTH_ONE), MASK_LENGTH_ONE, KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    const std::string payload = std::string("{\"properties1\":{\"admin_code_length\":") + std::to_string(CODE_LENGTH_MISMATCH) + "},\"admin_code\":" + json_bytes(ASCII_CODE) + "}";

    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_admin_code_set_command(target, payload));
    EXPECT_FALSE(target.child_by_type(ADMIN_SET_GROUP).is_valid());
}

// Spec 2026B Admin Code Set (CC:0063.02.0E.11.005). MQTT stores length 0 when ACD is set.
TEST_F(UserCodeCommandClassTest, MqttAdminCodeSetStoresDeactivation)
{
    auto target               = version_two_peer(static_cast<uint8_t>(AC_SUPPORT_MASK | ACD_SUPPORT_MASK | MASK_LENGTH_ONE), MASK_LENGTH_ONE, KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    const std::string payload = std::string("{\"properties1\":{\"admin_code_length\":") + std::to_string(LENGTH_ZERO) + "}}";

    ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_admin_code_set_command(target, payload));

    EXPECT_EQ(LENGTH_ZERO, target.child_by_type(ADMIN_SET_GROUP).child_by_type(static_cast<attribute_store_type_t>(admin_code_set_group_attributes_t::admin_code_length)).desired<uint8_t>());
}

// Spec 2026B Extended User Code Set (CC:0063.02.0B.11.001). An empty block list is refused.
TEST_F(UserCodeCommandClassTest, MqttExtendedUserCodeSetRejectsEmptyList)
{
    auto target = version_two_peer(MASK_LENGTH_ONE, static_cast<uint8_t>(MUCS_SUPPORT_MASK | MASK_LENGTH_ONE), KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);

    EXPECT_NE(SL_STATUS_OK, component().mqtt_on_extended_user_code_set_command(target, "{\"vg1\":[]}"));
    EXPECT_FALSE(target.child_by_type(EXT_SET_GROUP).is_valid());
}

// Spec 2026B Extended User Code Set (CC:0063.02.0B.11.003). MQTT stores one occupied block.
TEST_F(UserCodeCommandClassTest, MqttExtendedUserCodeSetStoresOccupiedBlock)
{
    auto target               = version_two_peer(MASK_LENGTH_ONE, static_cast<uint8_t>(MUCS_SUPPORT_MASK | MASK_LENGTH_ONE), KEYPAD_TWO_MODES, TWO_SUPPORTED_USERS, TWO_SUPPORTED_USERS);
    const std::string payload = std::string("{\"vg1\":[{\"user_identifier\":") + std::to_string(USER_IDENTIFIER_1) + ",\"user_id_status\":" + std::to_string(STATUS_OCCUPIED) + ",\"user_code\":" + json_bytes(ASCII_CODE) + "}]}";

    ASSERT_EQ(SL_STATUS_OK, component().mqtt_on_extended_user_code_set_command(target, payload));

    EXPECT_EQ(occupied_block(IDENTIFIER_MSB_ZERO, USER_IDENTIFIER_1), target.child_by_type(EXT_SET_GROUP).child_by_type(static_cast<attribute_store_type_t>(extended_user_code_set_group_attributes_t::vg1)).desired<std::vector<uint8_t>>());
}
