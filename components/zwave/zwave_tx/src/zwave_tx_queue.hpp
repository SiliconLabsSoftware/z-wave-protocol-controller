/******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef ZWAVE_TX_QUEUE_HPP
#define ZWAVE_TX_QUEUE_HPP

// Includes from this component
#include "zwave_tx.h"

// Includes from other components
#include "sl_status.h"

// Common component
#include "clock_platform.h"

#include "priority_queue.hpp"

#include <mutex>
#include <unordered_map>
#include <vector>

/**
 * @defgroup zwave_tx_queue Z-Wave TX queue
 * @ingroup zwave_tx
 * @brief Data model definition for the Z-Wave TX Queue and its elements
 *
 * The Z-Wave TX queue is a class that accomodates the queueing
 * requirements of the Z-Wave TX component.
 *
 * @{
 */

/**
 * @brief Z-Wave TX queue element
 *
 * This structure represents an element in our Z-Wave TX queue
 * where incoming element to transmit are saved and tracked.
 *
 * The order of this struct is manually picked to get it as best as possible
 * aligned with 32 bit systems! Please respect this when changing.
 */
typedef struct zwave_tx_queue_element {
        /// Frame payload
        uint8_t data[ZWAVE_MAX_FRAME_SIZE];
        /// Size of the frame payload
        uint16_t data_length;
        /// Transmission data returned by the @ref zwave_api
        /// Refer to \ref zwapi_send_data()
        zwapi_tx_report_t send_data_tx_status;
        /// Options for transmitting this element. Refer to \ref zwave_tx_options_t
        zwave_tx_options_t options;
        /// Requested connection_info (transport/encapsulation information)
        zwave_controller_connection_info_t connection_info;
        /// Session ID for this element. It's a unique identifier
        zwave_tx_session_id_t zwave_tx_session_id;
        /// Queuing timestamp, records when the frame was added to the queue.
        /// Will be used with the zwave_tx_options.discard_timeout_ms to decide
        // if it should still be sent.
        clock_time_t queue_timestamp;
        /// Transmissions timestamp, used to record when the frame is sent.
        clock_time_t transmission_timestamp;
        /// Transmission time, records how long the frame took to be delivered
        /// to a destination.
        clock_time_t transmission_time;
        /// Set when the chip transport reports completion (\ref set_transmissions_results);
        /// cleared when \ref zwave_tx_message_transmission_completed_step runs for that
        /// completion. Prevents duplicate processing (e.g. \ref is_frame_to_be_sent
        /// re-scanning a session still queued for replies after BACKOFF pre-emption).
        bool transport_completion_step_pending;
        /// Callback function for the component/user that requested the
        /// frame to be transmitted. It will return the status of transmission
        /// as well as the user pointer.
        on_zwave_tx_send_data_complete_t callback_function;
        /// User poiner that will be passed as argument to the
        /// on_zwave_tx_send_data_complete_t callback_function
        void *user;
        /// Transmission status returned by the @ref zwave_api.
        /// See \ref zwapi_transmit_complete_codes
        uint8_t send_data_status;
} zwave_tx_queue_element_t;

struct queue_element_qos_compare {
        bool operator()(const zwave_tx_queue_element_t &lhs, const zwave_tx_queue_element_t &rhs) const
        {
            return lhs.options.qos_priority > rhs.options.qos_priority;
        }
};

/** Z-Wave TX Queue class
 *
 * This class is a multiset of zwave_tx_queue_element_t objects,
 * that keeps sorted by element qos_priority.
 *
 * All queue elements are uniquely identified by a zwave_tx_session_id_t
 * that gets assigned to the element when queueing them. It is possible
 * to get a copy of an element providing a zwave_tx_session_id_t, but
 * they cannot be modified directly.
 *
 * Setters are available for changing elements properties.
 *
 */
class zwave_tx_queue
{
    private:
        // Global variable used to get new session IDs.
        uint32_t zwave_tx_session_id_counter = 0;
        mutable std::recursive_mutex queue_mutex_;
        // Tracks the number of queued singlecast frames per remote NodeID.
        // Only top-level and child singlecast frames are counted; multicast
        // frames are excluded.  Used to enforce ZWAVE_TX_QUEUE_MAX_FRAMES_PER_NODE.
        std::unordered_map<zwave_node_id_t, int> per_node_frame_count_;

    public:
        /**
         * @brief Adds a new element into the queue.
         *
         * A session_id will be allocated to the element and written back at the
         * location of the new_element parameter.
         *
         * @param new_element     Pointer to a variable containing element data to be
         *                        added into the queue.
         * @param user_session_id Pointer to a zwave_tx_session_id_t variable
         *                        used to return the allocated session_id.
         *
         * @returns
         *  SL_STATUS_OK if the element was accepted and added into the list.
         *               A session_id as assigned and written to
         *               new_element->zwave_tx_session_id
         */
        sl_status_t enqueue(const zwave_tx_queue_element_t &new_element, zwave_tx_session_id_t *user_session_id);

        /**
         * @brief Removes an element from the queue.
         *
         * @param session_id Used to identify the queue entry to remove.
         *
         * @returns
         *  SL_STATUS_OK if the element was found and removed
         *  SL_STATUS_NOT_FOUND if the element identified by session_id does not exist in the queue.
         */
        sl_status_t pop(const zwave_tx_session_id_t session_id);

        /**
         * @brief gets the element which has the highest priority
         *
         * @returns pointer to the first element
         * - nullptr if the queue is empty
         */
        zwave_tx_queue_element_t *first_in_queue();

        /**
         * @brief clears the tx queue
         *
         */
        void clear();

        /**
         * @returns true if the tx queue is empty
         * return false if the tx queue is not empty
         *
         */
        bool empty() const;

        /**
         * @returns The number of elements in the Tx Queue
         */
        int size() const noexcept;

        /////////////////////////////////////////////////////////////////////////////
        // Getter functions
        /////////////////////////////////////////////////////////////////////////////
        /**
         * @brief Gets a copy of the data for an element in the queue.
         *
         * @param element Variable where to copy the data from the element identified by the session_id
         * @param session_id Used to identify the queue entry to copy to element.
         *
         * @returns
         *  SL_STATUS_OK if the element was found and its data was copied
         *  SL_STATUS_NOT_FOUND if the element identified by session_id does not exist in the queue.
         */
        sl_status_t get_by_id(zwave_tx_queue_element_t *element, const zwave_tx_session_id_t session_id) const;

        /**
         * @brief Verifies if a session ID is in the queue
         *
         * @param session_id Used to identify the queue entry to find.
         *
         * @returns true if the Tx Queue contains the Session ID, false otherwise.
         */
        bool contains(const zwave_tx_session_id_t session_id) const;
        /**
         * @brief Find the child in the queue with the highest priority.
         *
         * @param element     Variable where to copy the data from the child element
         *                    identified by the session_id
         * @param session_id  Used to identify parent of the queue entry to copy to
         *                    the element pointer.
         *
         * @returns
         *  SL_STATUS_OK if an element was found and its data was copied
         *  SL_STATUS_NOT_FOUND if the element identified by session_id has no child  in the queue.
         */
        sl_status_t get_highest_priority_child(zwave_tx_queue_element_t *element, const zwave_tx_session_id_t session_id) const;

        /////////////////////////////////////////////////////////////////////////////
        // Setter functions
        /////////////////////////////////////////////////////////////////////////////
        /**
         * @brief Sets @ref zwave_api transmission results for an element in the queue.
         *
         * @param session_id   Used to identify the queue entry for which the
         *                     transmissions results must be copied to.
         * @param status       Status returned by the @ref zwave_api.
         * @param tx_status    Pointer to the zwapi_tx_report_t report returned by the
         *                     @ref zwave_api.
         *
         * @returns
         *  SL_STATUS_OK if the element was found and the new transmissions results were written to the element
         *  SL_STATUS_NOT_FOUND if the element identified by session_id does not exist in the queue.
         */
        sl_status_t set_transmissions_results(const zwave_tx_session_id_t session_id, uint8_t status, zwapi_tx_report_t *tx_status);

        /**
         * @brief Decrement the number of expected responses for an element in the queue.
         *
         * @param session_id   Used to identify the queue entry for which the
         *                     expected responses must be decremented.
         *
         * @returns
         *  SL_STATUS_OK if the element was found and number of responses was decremented
         *  SL_STATUS_NOT_FOUND if the element identified by session_id does not exist in the queue.
         */
        sl_status_t decrement_expected_responses(const zwave_tx_session_id_t session_id);

        /**
         * @brief Finds the total the number of expected responses for an element and
         *        its parents in the queue.
         *
         * @param session_id   Used to identify the queue entry for which the
         *                     total number of responses must be returned.
         *
         * @returns The number of responses that the session id will generate.
         *          0 if the session_id is not in the queue.
         */
        uint8_t get_number_of_responses(const zwave_tx_session_id_t session_id);

        /**
         * @brief Returns a pointer to the payload of a frame in the Tx Queue.
         *
         * @param session_id   Used to identify the queue entry for which the
         *                     the frame data poiner must be returned.
         *
         * @returns NULL if the session id is not found, a pointer otherwise.
         */
        const uint8_t *get_frame(const zwave_tx_session_id_t session_id);

        /**
         * @brief Returns the length of a frame in the Z-Wave Tx Queue.
         * @param  session_id Used to identify the queue entry for which the
         *                    the frame length poiner must be returned.
         *
         * @returns 0 if the session ID is not found, the length of the frame in bytes.
         */
        uint16_t get_frame_length(const zwave_tx_session_id_t session_id);

        /**
         * @brief Records a timestamp of the current time for the transmission of an element
         *
         * The timestamp will subsequently be used by set_transmissions_results()
         * to calculate how long the transmission took.
         *
         * @param session_id Used to identify the queue entry for which the
         *                   timestamp must be marked.
         *
         * @returns
         *  SL_STATUS_OK if the element was found and not timestamp was set.
         *  SL_STATUS_NOT_FOUND if the element identified by session_id does not exist in the queue.
         */
        sl_status_t set_transmission_timestamp(const zwave_tx_session_id_t session_id);

        /**
         * @brief Reset the timestamp for the transmission of an element
         *
         * This is unsed in case the element transmission was initiated, but it
         * needs to be requeued and a new transmission will be initiated later on.
         *
         * @param session_id Used to identify the queue entry for which the
         *                   timestamp must be reset.
         *
         * @returns
         *  SL_STATUS_OK if the element was found and not timestamp was reset.
         *  SL_STATUS_NOT_FOUND if the element identified by session_id does not exist in the queue.
         */
        sl_status_t reset_transmission_timestamp(const zwave_tx_session_id_t session_id);

        /**
         * @brief Consumes the transmission completion flag for a queue element.
         *
         * @returns true if the flag was set and is now cleared;
         *          false if the session is missing or the flag was already clear.
         */
        bool consume_transport_completion_step_pending(const zwave_tx_session_id_t session_id);

        /**
         * @brief Non-consuming read of the transmission completion flag (see \ref consume_transport_completion_step_pending).
         */
        bool transport_completion_step_is_pending(const zwave_tx_session_id_t session_id) const;

        /**
         * @brief Highest-QoS element that has not yet been handed to a transport
         *        (`transmission_timestamp == 0`).
         *
         * Used when the queue head is only waiting for application replies (transmission
         * already completed) so lower-priority unsent frames must not stall behind it.
         *
         * Child frames (`options.transport.valid_parent_session_id == true`) are
         * excluded: they must only be dispatched through their parent's
         * transmission context (see \ref get_highest_priority_child) so that the
         * parent-child ordering is preserved.
         */
        sl_status_t find_best_unsent_by_qos(zwave_tx_queue_element_t *element) const;

        /**
         * @brief Like \ref find_best_unsent_by_qos but restricted to standalone
         *        back-off-bypass frames (`ignore_incoming_frames_back_off`, no parent).
         *
         * Used so an SOS Nonce Report can overtake an in-flight parent that is
         * still waiting for an S2 send-data callback (parent remains queue head
         * by QoS or insertion order).
         */
        sl_status_t find_best_unsent_backoff_bypass(zwave_tx_queue_element_t *element) const;

        /**
         * @brief Lowest-QoS element that has not yet been handed to a transport
         *        (`transmission_timestamp == 0`).
         *
         * Used by the queue-full failsafe to evict the least important unsent
         * frame and make room for a higher-priority incoming one, rather than
         * sacrificing the highest-priority head of the queue.
         *
         * Child frames and multicast frames are excluded by the same rules as
         * \ref find_best_unsent_by_qos.
         */
        sl_status_t find_worst_unsent_by_qos(zwave_tx_queue_element_t *element) const;

        /**
         * @brief Disables the fasttrack options from the Z-Wave Tx Options
         *
         * This should be used when a "fasttracked" transmission attempt failed.
         *
         * @param session_id Used to identify the queue entry for which the
         *                   fasttrack flag is to be disabled
         *
         * @returns
         *  SL_STATUS_OK if the element was found and not timestamp was reset.
         *  SL_STATUS_NOT_FOUND if the element identified by session_id does not exist in the queue.
         */
        sl_status_t disable_fasttack(const zwave_tx_session_id_t session_id);

        /**
         * @brief Finds if we have elements to send to a NodeID destination
         *
         * @param node_id     The NodeID to check for in the queue
         *
         * @returns true if we have elements pending for this NodeID, false otherwise.
         */
        bool zwave_tx_has_frames_for_node(const zwave_node_id_t node_id);

        /**
         * @brief Collects the session IDs of all singlecast frames queued for
         *        a NodeID destination.
         *
         * Only singlecast frames are collected. Multicast frames whose group
         * happens to include the NodeID remain queued because they are still
         * meaningful for the other recipients.
         *
         * @param node_id  The NodeID whose queued frames must be collected.
         * @param ids      Vector that will be populated with session IDs;
         *                 existing contents are preserved.
         */
        void collect_session_ids_for_node(const zwave_node_id_t node_id, std::vector<zwave_tx_session_id_t> &ids) const;

        /**
         * @brief Collects the session IDs of all frames currently in the queue.
         *
         * @param ids  Vector that will be populated with session IDs;
         *             existing contents are preserved.
         */
        void collect_all_session_ids(std::vector<zwave_tx_session_id_t> &ids) const;

        /**
         * @brief Collects a session and all its transitive encapsulation
         *        descendants (children, grand-children, ...).
         *
         * Used to tear down a whole encapsulation chain at once: \ref pop only
         * removes a single element, so failing a parent on its own would orphan
         * its child frames. The root is added first and descendants after, so
         * callers can finalize the family child-first by iterating in reverse.
         *
         * @param root The session ID at the top of the chain to collect.
         * @param ids  Vector that will be populated with session IDs; existing
         *             contents are preserved. Nothing is added if @p root is not
         *             in the queue.
         */
        void collect_descendants(const zwave_tx_session_id_t root, std::vector<zwave_tx_session_id_t> &ids) const;

        /**
         * @brief Walks up the encapsulation chain to the top-most ancestor of a
         *        session that is still in the queue.
         *
         * While a transmission is ongoing the "current" session ID tracks the
         * deepest in-flight child (the encapsulated frame actually on the air),
         * not the original frame. Callers that must act on the whole chain (e.g.
         * the in-flight watchdog feeding \ref collect_descendants) have to resolve
         * the real root first, otherwise ancestor entries are left orphaned in the
         * queue.
         *
         * @param session_id The session to start from.
         *
         * @returns The session ID of the top-most ancestor still in the queue, or
         *          @p session_id itself if it has no queued parent or is not in
         *          the queue.
         */
        zwave_tx_session_id_t find_root(const zwave_tx_session_id_t session_id) const;

        /////////////////////////////////////////////////////////////////////////////
        // Print functions
        /////////////////////////////////////////////////////////////////////////////
        /**
         * @brief Logs the content of the queue using \ref sl_log
         *
         * @param log_messages_payload boolean indicating if the hex payload of
         *                             each frame must be included in the log.
         */
        void log(bool log_messages_payload) const;

        /**
         * @brief Logs the content one element of the queue using \ref sl_log
         *
         * @param session_id           boolean indicating if the hex payload of
         *                             each frame must be included in the log.
         * @param log_frame_payload    boolean indicating if the hex payload of
         *                             the frame must be included in the log.
         */
        void log_element(const zwave_tx_session_id_t session_id, bool log_frame_payload) const;

        /**
         * @brief Short log of one element using \ref sl_log
         *
         * @param e       Pointer of the element object to log.
         */
        void simple_log(zwave_tx_queue_element_t *e) const;

        /**
         * @brief Logs the per-NodeID frame count using \ref sl_log at CRITICAL level
         *        with a dedicated diagnostics prefix.
         *
         * Called from the periodic stall diagnostic in the TX process thread so
         * endurance-test logs show exactly which nodes are approaching the per-node
         * queue cap.
         */
        void log_per_node_distribution() const;

    private:
        using queue_iterator       = priority_queue<zwave_tx_queue_element_t, ZWAVE_TX_QUEUE_BUFFER_SIZE, queue_element_qos_compare>::iterator;
        using const_queue_iterator = priority_queue<zwave_tx_queue_element_t, ZWAVE_TX_QUEUE_BUFFER_SIZE, queue_element_qos_compare>::const_iterator;

        queue_iterator find(const zwave_tx_session_id_t key);
        const_queue_iterator find(const zwave_tx_session_id_t key) const;

        priority_queue<zwave_tx_queue_element_t, ZWAVE_TX_QUEUE_BUFFER_SIZE, queue_element_qos_compare> queue;
};

/** @} end of zwave_tx_queue */

#endif  // ZWAVE_TX_QUEUE_HPP