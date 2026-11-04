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
 *****************************************************************************/

#include "zwapi_jamming.h"
#include "zwapi_internal.h"
#include "zwapi_session.h"
#include "zwapi_protocol_basis.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <vector>

namespace
{
    std::vector<uint8_t> sent_payload;
    std::vector<uint8_t> response;
    sl_status_t transport_status;
    constexpr uint8_t SENTINEL = 0x7f;  // Output must remain untouched on failure.

    class JammingSerialApiTest : public ::testing::Test
    {
        protected:
            void SetUp() override
            {
                sent_payload.clear();
                response.clear();
                transport_status = SL_STATUS_OK;
                zwapi_jamming_register_notification_callback(nullptr);
            }

            static void echo(std::initializer_list<uint8_t> payload)
            {
                response.assign(IDX_DATA, 0);  // Session responses include LEN/TYPE/FUNC_ID.
                response.insert(response.end(), payload);
            }
    };
}  // namespace

// The RX dispatcher needs initialized callbacks and a node-ID mode. Other
// initialization behavior is outside this feature's transport boundary.
extern "C" zwapi_callbacks_t *zwave_api_get_callbacks()
{
    static zwapi_callbacks_t callbacks {};
    return &callbacks;
}

extern "C" void zwapi_set_awaiting_zwave_api_started(bool)
{
    ADD_FAILURE() << "Jamming frames must not change startup state";
}

extern "C" zwave_node_id_basetype_t zwapi_get_node_id_basetype()
{
    return NODEID_8BITS;
}

extern "C" sl_status_t zwapi_send_command_with_response(uint8_t command, const uint8_t *payload, uint8_t length, uint8_t *buffer, uint8_t *response_length)
{
    EXPECT_EQ(command, FUNC_ID_JAMMING_DETECTION);
    sent_payload.assign(payload, payload + length);
    if (transport_status == SL_STATUS_OK) {
        // Match the session contract: response_length is output-only, not capacity.
        std::copy(response.begin(), response.end(), buffer);
        *response_length = static_cast<uint8_t>(response.size());
    }
    return transport_status;
}

// The channel command encodes signed dBm as one byte and returns the NCP's values.
TEST_F(JammingSerialApiTest, ChannelConfigurationUsesWireBytesAndEchoedValues)
{
    echo({0x02, 2, 0xc4, 90});  // Channel config, channel 2, -60 dBm, 90 samples.
    uint8_t channel = 0, samples = 0;
    int8_t threshold = 0;
    EXPECT_EQ(zwapi_jamming_set_channel_config(3, -50, 95, &channel, &threshold, &samples), SL_STATUS_OK);
    EXPECT_EQ(sent_payload, (std::vector<uint8_t> {0x02, 3, 0xce, 95}));  // -50 dBm = 0xce.
    EXPECT_EQ(channel, 2);
    EXPECT_EQ(threshold, -60);
    EXPECT_EQ(samples, 90);
}

// Both period commands use network byte order; collection exercises a nonzero high byte.
TEST_F(JammingSerialApiTest, PeriodCommandsUseBigEndianAndReturnEchoedValues)
{
    uint16_t period = 0;
    echo({0x03, 0, 7});  // Re-report configuration, seven seconds.
    EXPECT_EQ(zwapi_jamming_set_report_period(5, &period), SL_STATUS_OK);
    EXPECT_EQ(sent_payload, (std::vector<uint8_t> {0x03, 0, 5}));
    EXPECT_EQ(period, 7);
    constexpr uint16_t COLLECTION_PERIODS = 0x1234;
    echo({0x01, 0xab, 0xcd});  // RSSI collection configuration, echoed count 0xabcd.
    EXPECT_EQ(zwapi_jamming_set_rssi_collection(COLLECTION_PERIODS, &period), SL_STATUS_OK);
    EXPECT_EQ(sent_payload, (std::vector<uint8_t> {0x01, 0x12, 0x34}));
    EXPECT_EQ(period, 0xabcd);
}

// Truncated echoes and a different subcommand must not produce accepted configuration.
TEST_F(JammingSerialApiTest, RejectsTruncatedAndWrongSubcommandEchoes)
{
    for (const auto &payload: {std::vector<uint8_t> {}, {0x03}, {0x03, 0}, {0x01, 0, 5}}) {
        response.assign(IDX_DATA, 0);
        response.insert(response.end(), payload.begin(), payload.end());
        uint16_t period = SENTINEL;
        EXPECT_EQ(zwapi_jamming_set_report_period(5, &period), SL_STATUS_FAIL);
        EXPECT_EQ(period, SENTINEL);
    }
}

// Transport errors propagate without treating the unfilled response as an echo.
TEST_F(JammingSerialApiTest, TransportFailureLeavesOutputsUntouched)
{
    transport_status = SL_STATUS_NOT_SUPPORTED;
    uint8_t channel = SENTINEL, samples = SENTINEL;
    int8_t threshold = SENTINEL;
    uint16_t period  = SENTINEL;
    EXPECT_EQ(zwapi_jamming_set_channel_config(0, -50, 95, &channel, &threshold, &samples), transport_status);
    EXPECT_EQ(channel, SENTINEL);
    EXPECT_EQ(threshold, SENTINEL);
    EXPECT_EQ(samples, SENTINEL);
    EXPECT_EQ(zwapi_jamming_set_report_period(5, &period), transport_status);
    EXPECT_EQ(period, SENTINEL);
    EXPECT_EQ(zwapi_jamming_set_rssi_collection(100, &period), transport_status);
    EXPECT_EQ(period, SENTINEL);
}

// A full legal session response must fit before the wrapper validates its prefix.
TEST_F(JammingSerialApiTest, AcceptsMaximumSessionResponseWithoutOverflow)
{
    echo({0x03, 0, 5});  // Valid re-report echo followed by extension bytes.
    response.resize(SERIAL_BUFFER_SIZE, 0);
    uint16_t period = 0;
    EXPECT_EQ(zwapi_jamming_set_report_period(5, &period), SL_STATUS_OK);
    EXPECT_EQ(period, 5);
}

// Incoming REQUEST frames reach the registered callback with the subcommand removed.
TEST_F(JammingSerialApiTest, DispatchesNotificationPayload)
{
    static std::vector<uint8_t> notification;
    static unsigned notification_calls;
    notification.clear();
    notification_calls = 0;
    zwapi_jamming_register_notification_callback([](uint8_t subcommand, const uint8_t *data, uint8_t length) {
        ++notification_calls;
        EXPECT_EQ(subcommand, JAMMING_SUB_CMD_COLLECTION);
        notification.assign(data, data + length);
    });
    std::array<uint8_t, IDX_DATA + 3> frame {0};  // Header, collection subcommand, two sample bytes.
    frame[IDX_LEN]      = frame.size();           // Length includes the checksum on the wire; LEN replaces it here.
    frame[IDX_TYPE]     = FRAME_TYPE_REQUEST;
    frame[IDX_CMD]      = FUNC_ID_JAMMING_DETECTION;
    frame[IDX_DATA]     = JAMMING_SUB_CMD_COLLECTION;
    frame[IDX_DATA + 1] = 0x9a;  // -102 dBm.
    frame[IDX_DATA + 2] = 0x80;  // Unavailable RSSI.
    zwave_api_protocol_rx_dispatch(frame.data(), frame.size());
    EXPECT_EQ(notification, (std::vector<uint8_t> {0x9a, 0x80}));
    zwapi_jamming_register_notification_callback(nullptr);
    zwave_api_protocol_rx_dispatch(frame.data(), frame.size());
    EXPECT_EQ(notification_calls, 1U);  // Deregistration prevents a second callback.
}

// A REQUEST without a subcommand is dropped before reading the payload.
TEST_F(JammingSerialApiTest, DropsNotificationWithoutSubcommand)
{
    zwapi_jamming_register_notification_callback([](uint8_t, const uint8_t *, uint8_t) { ADD_FAILURE() << "Empty notification must not invoke the callback"; });
    std::array<uint8_t, IDX_DATA> frame {};  // Only LEN/TYPE/FUNC_ID, with no payload.
    frame[IDX_LEN]  = frame.size();
    frame[IDX_TYPE] = FRAME_TYPE_REQUEST;
    frame[IDX_CMD]  = FUNC_ID_JAMMING_DETECTION;
    zwave_api_protocol_rx_dispatch(frame.data(), frame.size());
    zwapi_jamming_register_notification_callback(nullptr);
}
