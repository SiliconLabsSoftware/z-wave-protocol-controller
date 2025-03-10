/* © 2017 Silicon Laboratories Inc.
 */
/*
 * S2_external.h
 *
 *  Created on: Sep 24, 2015
 *      Author: aes
 */

#ifndef INCLUDE_S2_EXTERNAL_H_
#define INCLUDE_S2_EXTERNAL_H_

#include "S2.h"
/**
 * \ingroup S2trans
 * \defgroup s2external Extenal functions used by S2
 *
 * All of these function must be implemented elsewhere. These are
 * the hooks that libs2 use for actually sending frames, and to timer tracking.
 * @{
 */

/**
 * Emitted when the security engine is done.
 * Note that this is also emitted when the security layer sends a SECURE_COMMANDS_SUPPORTED_REPORT
 *
 * \param ctxt the S2 context
 * \param status The status of the S2 transmisison
 */
void S2_send_done_event(struct S2* ctxt, s2_tx_status_t status);

/**
 * Emitted when a messages has been received and decrypted
 * \param ctxt the S2 context
 * \param peer Transaction parameters. peer->class_id will indicate which security class was used for
 * decryption.
 * \param buf The received message
 * \param len The length of the received message
 */
void S2_msg_received_event(struct S2* ctxt,s2_connection_t* peer , uint8_t* buf, uint16_t len);


/**
 * Must be implemented elsewhere maps to ZW_SendData or ZW_SendDataBridge note that ctxt is
 * passed as a handle. The ctxt MUST be provided when the \ref S2_send_frame_done_notify is called
 *
 * \param ctxt the S2 context
 * \param peer Transaction parameters.
 * \param buf The received message
 * \param len The length of the received message
 *
 */
uint8_t S2_send_frame(struct S2* ctxt,const s2_connection_t* peer, uint8_t* buf, uint16_t len);


/**
 * Send without emitting a callback into S2 and by caching the buffer
 *
 * Must be implemented elsewhere maps to ZW_SendData or ZW_SendDataBridge note that ctxt is
 * passed as a handle. The ctxt MUST be provided when the \ref S2_send_frame_done_notify is called.
 *
 * \param ctxt the S2 context
 * \param peer Transaction parameters.
 * \param buf The received message
 * \param len The length of the received message
 *
 */
uint8_t S2_send_frame_no_cb(struct S2* ctxt,const s2_connection_t* peer, uint8_t* buf, uint16_t len);


/**
 * This must send a broadcast frame.
 * \ref S2_send_frame_done_notify must be called when transmisison is done.
 *
 * \param ctxt the S2 context
 * \param peer Transaction parameters.
 * \param buf The received message
 * \param len The length of the received message
 */
uint8_t S2_send_frame_multi(struct S2* ctxt,s2_connection_t* peer, uint8_t* buf, uint16_t len);

/**
 * Must be implemented elsewhere maps to ZW_TimerStart. Note that this must start the same timer every time.
 * Ie. two calls to this function must reset the timer to a new value. On timeout \ref S2_timeout_notify must be called.
 *
 * \param ctxt the S2 context
 * \param interval Timeout in milliseconds.
 */
void S2_set_timeout(struct S2* ctxt, uint32_t interval);

/**
 * @brief Stop the timer set with S2 set timeout.
 * 
 * @param ctxt 
 */
void S2_stop_timeout(struct S2* ctxt);

/**
 * Get a number of bytes from a hardware random number generator
 *
 * \param buf pointer to buffer where the random bytes must be written.
 * \param len number of bytes to write.
 */
void S2_get_hw_random(uint8_t *buf, uint8_t len);

/**
 * Get the command classes supported by S2
 * \param lnode The node id to which the frame was sent.
 * \param class_id the security class this request was on
 * \param cmdClasses points to the first command class
 * \param length the length of the command class list
 */
void S2_get_commands_supported(node_t lnode, uint8_t class_id, const uint8_t ** cmdClasses, uint8_t* length);


/**
 * Notify the reception of a NLS STATE REPORT
 * \param srcNode Source node ID of the frame
 * \param class_id the security class this request was on
 * \param nls_capability The NLS capability of the source node
 * \param nls_state The NLS state of the source node
*/
void S2_notify_nls_state_report(node_t srcNode, uint8_t class_id, bool nls_capability, bool nls_state);

/**
 * Get the NLS nodes list
 * \param srcNode Source node ID of the frame
 * \param request request field. 0 for the first node in the list, 1 for the next node in the list
 * \param is_last_node out pointer to be filled by the host indicating whether the node in question is the last one or not
 * \param node_id out pointer to be filled by the host indicating the node ID in question
 * \param granted_keys out pointer to be filled by the host indicating the granted keys of the node in question
 * \param nls_state out pointer to be filled by the host indicating the NLS state of the node in question
 * \return 0 in case of success or else in case of error
 */
int8_t S2_get_nls_node_list(node_t srcNode, bool request, bool *is_last_node, uint16_t *node_id, uint8_t *granted_keys, bool *nls_state);

/**
 * Get the NLS node list report
 * \param srcNode Source node ID of the frame
 * \param id_of_node the Node ID of the node being advertised
 * \param keys_node_bitmask granted keys for current Node ID
 * \param nls_state NLS state of the current node ID
 * \return 0 in case of success or else in case of error
 */
int8_t S2_notify_nls_node_list_report(node_t srcNode, uint16_t id_of_node, uint8_t keys_node_bitmask, bool nls_state);

/**
 * Makes time in ms available to LibS2
 * \return Timer tick in MS
 */
#ifdef ZIPGW
unsigned long clock_time(void);
#else
uint32_t clock_time(void);
#endif

/**
 * SOS_EVENT_REASON_UNANSWERED means that a Nonce Report with Singlecast-out-of-Sync
 * (SOS) = 1 has been received at an unexpected time and no response was sent.
 *
 * A Nonce Report SOS is considered expected and no SOS_EVENT_REASON_UNANSWERED will be emitted in these case:
 *    1) libs2 is in Verify Delivery state and receives a Nonce Report SOS from the
 *       node being delivered to and will re-transmit the encrypted message
 *    2) libs2 has already re-transmitted and receives a second SOS from the node being transmitted to
 *       during Verify Delivery timeout. A \ref S2_send_done_event() with
 *       status=TRANSMIT_COMPLETE_NO_ACK will be emitted instead.
 *
 */
typedef enum {
  SOS_EVENT_REASON_UNANSWERED,            ///< A Nonce Report with SOS=1 was received at an unexpected time and no response was sent. Application may use this information to abort Supervision Report timeout if the remote nodeid matches.
}
sos_event_reason_t;

/**
 * Emitted when a Nonce Synchronization Event has happened.
 * This event is informational. The application can safely ignore these events.
 * The application may also choose to optimize frame transmission in certain
 * edge cases based on these events.
 *
 * \param remote_node The nodeID of the remote node.
 * \param reason The reason for the event. Only SOS_EVENT_REASON_UNANSWERED supported.
 * \param seqno The S2 sequence number of the frame triggering the event. Useful for correlating with zniffer traces.
 * \param local_node The (possible virtual) nodeID of the local node. Useful for correlating with zniffer traces.
 */
void S2_resynchronization_event(
    node_t remote_node,
    sos_event_reason_t reason,
    uint8_t seqno,
    node_t local_node);

/**
 * Save NLS state stored in S2 context in device memory
 */
void S2_save_nls_state(void);

/**
 * Load NLS state stored in device memory in S2 context
 *
 * \param ctxt the S2 context
 * \param nls_state NLS state
 */
void S2_load_nls_state(struct S2* ctxt, uint8_t nls_state);

/**
 * @}
 */
#endif /* INCLUDE_S2_EXTERNAL_H_ */
