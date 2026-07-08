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

// Includes from this component
#include "zwave_tx.h"
#include "zwave_tx_process.h"
#include "zwave_tx_queue.hpp"

// Includes from other components
#include "zwave_tx_groups.h"
#include "log.h"

// Generic includes
#include <string.h>  // Using memcpy
#include <algorithm>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

#define LOG_TAG "zwave_tx_queue"

namespace
{
    bool parent_session_id_equals(const zwave_tx_queue_element_t &element, const zwave_tx_session_id_t parent_session)
    {
        return (element.options.transport.valid_parent_session_id) && (element.options.transport.parent_session_id == parent_session);
    }

    // Hex-dump a byte buffer as "AA BB CC " (uppercase, space-separated).
    // Returns the result by value; each caller gets its own buffer so logging
    // is safe to use from multiple threads.
    std::string to_hex_string(const uint8_t *data, uint16_t length)
    {
        std::ostringstream oss;
        oss << std::uppercase << std::hex << std::setfill('0');
        for (uint16_t i = 0; i < length; ++i) {
            oss << std::setw(2) << static_cast<unsigned>(data[i]) << ' ';
        }
        return oss.str();
    }
}  // namespace

sl_status_t zwave_tx_queue::enqueue(const zwave_tx_queue_element_t &new_element, zwave_tx_session_id_t *user_session_id)
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    zwave_tx_queue_element_t e = new_element;

    // Enforce per-node queue cap for singlecast frames to prevent a single
    // slow or misbehaving node from monopolising all queue slots.
    if (!e.connection_info.remote.is_multicast) {
        const zwave_node_id_t node_id = e.connection_info.remote.node_id;
        const auto it                 = per_node_frame_count_.find(node_id);
        const int current_count       = (it != per_node_frame_count_.end()) ? it->second : 0;
        if (current_count >= ZWAVE_TX_QUEUE_MAX_FRAMES_PER_NODE) {
            sl_log_warning(LOG_TAG,
                           "Per-node queue limit (%d) reached for NodeID %d. "
                           "Rejecting frame.",
                           ZWAVE_TX_QUEUE_MAX_FRAMES_PER_NODE,
                           node_id);
            return SL_STATUS_FULL;
        }
    }

    // Assign a session ID and provide it back to the user.
    e.zwave_tx_session_id = (zwave_tx_session_id_t)(uintptr_t)this->zwave_tx_session_id_counter++;
    if (user_session_id != nullptr) {
        *user_session_id = e.zwave_tx_session_id;
    }

    // Ensure that everything is initialized correctly:
    if (!e.options.transport.valid_parent_session_id) {
        e.options.transport.parent_session_id = nullptr;
    }
    // Ensure that the timing variables are correctly set:
    e.transmission_timestamp            = 0;
    e.transmission_time                 = 0;
    e.transport_completion_step_pending = false;
    e.queue_timestamp                   = clock_time();

    bool insert_status = queue.insert(std::move(e));

    // Make a console message about our new frame
    this->simple_log(&e);

    if (!insert_status) {
        return SL_STATUS_FULL;
    }

    if (!new_element.connection_info.remote.is_multicast) {
        per_node_frame_count_[new_element.connection_info.remote.node_id]++;
    }
    return SL_STATUS_OK;
}

sl_status_t zwave_tx_queue::pop(const zwave_tx_session_id_t session_id)
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    if (auto *const it = find(session_id); it != queue.end()) {
        if (!it->connection_info.remote.is_multicast) {
            const zwave_node_id_t node_id = it->connection_info.remote.node_id;
            auto count_it                 = per_node_frame_count_.find(node_id);
            if (count_it != per_node_frame_count_.end()) {
                if (--(count_it->second) <= 0) {
                    per_node_frame_count_.erase(count_it);
                }
            }
        }
        queue.erase(it);
        return SL_STATUS_OK;
    }
    return SL_STATUS_NOT_FOUND;
}

zwave_tx_queue_element_t *zwave_tx_queue::first_in_queue()
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    return queue.begin();
}

void zwave_tx_queue::clear()
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    queue.clear();
    per_node_frame_count_.clear();
}

bool zwave_tx_queue::empty() const
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    return queue.empty();
}

int zwave_tx_queue::size() const noexcept
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    return queue.size();
}

///////////////////////////////////////////////////////////////////////////////
// Getter functions
///////////////////////////////////////////////////////////////////////////////
bool zwave_tx_queue::contains(const zwave_tx_session_id_t session_id) const
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    return find(session_id) != queue.end();
}

sl_status_t zwave_tx_queue::get_by_id(zwave_tx_queue_element_t *element, const zwave_tx_session_id_t session_id) const
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    if (const auto *it = find(session_id); it != queue.end()) {
        memcpy(element, &(*it), sizeof(zwave_tx_queue_element_t));
        return SL_STATUS_OK;
    }

    return SL_STATUS_NOT_FOUND;
}

sl_status_t zwave_tx_queue::get_highest_priority_child(zwave_tx_queue_element_t *element, const zwave_tx_session_id_t session_id) const
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    sl_status_t status                    = SL_STATUS_NOT_FOUND;
    const_queue_iterator highest_priority = queue.end();

    const auto *it = queue.begin();
    while (it != queue.end()) {
        if (parent_session_id_equals(*it, session_id) && (highest_priority == queue.end() || queue_element_qos_compare()(*it, *highest_priority))) {
            highest_priority = it;
        }
        ++it;
    }

    if (highest_priority != queue.end()) {
        memcpy(element, highest_priority, sizeof(zwave_tx_queue_element_t));
        status = SL_STATUS_OK;
    }
    return status;
}

///////////////////////////////////////////////////////////////////////////////
// Setter functions
///////////////////////////////////////////////////////////////////////////////
sl_status_t zwave_tx_queue::set_transmissions_results(const zwave_tx_session_id_t session_id, uint8_t status, zwapi_tx_report_t *tx_status)
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    if (auto *it = find(session_id); it != queue.end()) {
        it->send_data_status = status;
        if (nullptr != tx_status) {
            it->send_data_tx_status = *tx_status;
        } else {
            memset(&(it->send_data_tx_status), 0, sizeof(zwapi_tx_report_t));
        }
        // Capture the transmission time here.
        it->transmission_time                 = clock_time() - it->transmission_timestamp;
        it->transport_completion_step_pending = true;
        // Prevent 0ms transmission time, we check on this value
        // to verify that the element was sent
        if (it->transmission_time == 0) {
            it->transmission_time = 1;
        }
        // Copy this time in the tx_report, so the user component can read this data
        it->send_data_tx_status.transmit_ticks = (uint16_t)(it->transmission_time / 10);

        return SL_STATUS_OK;
    }
    return SL_STATUS_NOT_FOUND;
}

sl_status_t zwave_tx_queue::decrement_expected_responses(const zwave_tx_session_id_t session_id)
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    if (auto *it = find(session_id); it != queue.end()) {
        if (it->options.number_of_responses > 0) {
            it->options.number_of_responses--;

        } else if (it->options.transport.valid_parent_session_id) {
            // Check if parents frame are expecting responses
            return this->decrement_expected_responses(it->options.transport.parent_session_id);
        }
        return SL_STATUS_OK;
    }
    return SL_STATUS_NOT_FOUND;
}

uint8_t zwave_tx_queue::get_number_of_responses(const zwave_tx_session_id_t session_id)
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    uint8_t total_number_of_responses = 0;
    zwave_tx_queue_element_t element  = {};
    if (this->get_by_id(&element, session_id) == SL_STATUS_OK) {
        total_number_of_responses += element.options.number_of_responses;
        if (element.options.transport.valid_parent_session_id) {
            total_number_of_responses += this->get_number_of_responses(element.options.transport.parent_session_id);
        }
    }

    return total_number_of_responses;
}

const uint8_t *zwave_tx_queue::get_frame(const zwave_tx_session_id_t session_id)
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    if (auto *it = find(session_id); it != queue.end()) {
        return it->data;
    }

    return nullptr;
}

uint16_t zwave_tx_queue::get_frame_length(const zwave_tx_session_id_t session_id)
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    if (auto *it = find(session_id); it != queue.end()) {
        return it->data_length;
    }

    return 0;
}

sl_status_t zwave_tx_queue::set_transmission_timestamp(const zwave_tx_session_id_t session_id)
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    auto *it = find(session_id);
    if (it != queue.end()) {
        it->transmission_timestamp = clock_time();
        return SL_STATUS_OK;
    }
    return SL_STATUS_NOT_FOUND;
}

sl_status_t zwave_tx_queue::reset_transmission_timestamp(const zwave_tx_session_id_t session_id)
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    auto *it = find(session_id);
    if (it != queue.end()) {
        it->transmission_timestamp = 0;
        it->transmission_time      = 0;
        return SL_STATUS_OK;
    }
    return SL_STATUS_NOT_FOUND;
}

bool zwave_tx_queue::consume_transport_completion_step_pending(const zwave_tx_session_id_t session_id)
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    if (auto *it = find(session_id); it != queue.end()) {
        if (!it->transport_completion_step_pending) {
            return false;
        }
        it->transport_completion_step_pending = false;
        return true;
    }
    return false;
}

bool zwave_tx_queue::transport_completion_step_is_pending(const zwave_tx_session_id_t session_id) const
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    if (const auto *const it = find(session_id); it != queue.end()) {
        return it->transport_completion_step_pending;
    }
    return false;
}

sl_status_t zwave_tx_queue::find_best_unsent_by_qos(zwave_tx_queue_element_t *element) const
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    if (empty() || (element == nullptr)) {
        return SL_STATUS_NOT_FOUND;
    }
    const queue_element_qos_compare better;
    bool found                    = false;
    zwave_tx_queue_element_t best = {};
    for (const auto *it = queue.begin(); it != queue.end(); ++it) {
        if (it->transmission_timestamp != 0) {
            continue;
        }
        // Skip child frames as they are tied to their parent session and must
        // only be dispatched through the parent's transmission context via
        // get_highest_priority_child().
        if (it->options.transport.valid_parent_session_id) {
            continue;
        }
        if (!found || better(*it, best)) {
            best  = *it;
            found = true;
        }
    }
    if (!found) {
        return SL_STATUS_NOT_FOUND;
    }
    *element = best;
    return SL_STATUS_OK;
}

sl_status_t zwave_tx_queue::find_best_unsent_backoff_bypass(zwave_tx_queue_element_t *element) const
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    if (empty() || (element == nullptr)) {
        return SL_STATUS_NOT_FOUND;
    }
    const queue_element_qos_compare better;
    bool found                    = false;
    zwave_tx_queue_element_t best = {};
    for (const auto *it = queue.begin(); it != queue.end(); ++it) {
        if (it->transmission_timestamp != 0) {
            continue;
        }
        if ((!it->options.transport.ignore_incoming_frames_back_off) || (it->options.transport.valid_parent_session_id)) {
            continue;
        }
        if (!found || better(*it, best)) {
            best  = *it;
            found = true;
        }
    }
    if (!found) {
        return SL_STATUS_NOT_FOUND;
    }
    *element = best;
    return SL_STATUS_OK;
}

sl_status_t zwave_tx_queue::find_worst_unsent_by_qos(zwave_tx_queue_element_t *element) const
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    if (empty() || (element == nullptr)) {
        return SL_STATUS_NOT_FOUND;
    }
    const queue_element_qos_compare better;
    bool found                     = false;
    zwave_tx_queue_element_t worst = {};
    for (const auto *it = queue.begin(); it != queue.end(); ++it) {
        if (it->transmission_timestamp != 0) {
            continue;
        }
        if (it->options.transport.valid_parent_session_id) {
            continue;
        }
        // Keep the element with the *lowest* QoS: replace when current worst
        // has higher priority than *it (i.e. *it is a worse candidate).
        if (!found || better(worst, *it)) {
            worst = *it;
            found = true;
        }
    }
    if (!found) {
        return SL_STATUS_NOT_FOUND;
    }
    *element = worst;
    return SL_STATUS_OK;
}

sl_status_t zwave_tx_queue::disable_fasttack(const zwave_tx_session_id_t session_id)
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    auto *it = find(session_id);
    if (it != queue.end()) {
        it->options.fasttrack = false;
        return SL_STATUS_OK;
    }
    return SL_STATUS_NOT_FOUND;
}

bool zwave_tx_queue::zwave_tx_has_frames_for_node(const zwave_node_id_t node_id)
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    for (auto *it = queue.begin(); it != queue.end(); ++it) {
        // If singlecast, just check the NodeID
        if ((!it->connection_info.remote.is_multicast) && (it->connection_info.remote.node_id == node_id)) {
            return true;
        }
        // If multicast, check if the NodeID is part of the group
        if (it->connection_info.remote.is_multicast) {
            zwave_nodemask_t nodes = {};
            zwave_tx_get_nodes(nodes, it->connection_info.remote.multicast_group);
            if (ZW_IS_NODE_IN_MASK(node_id, nodes)) {
                return true;
            }
        }
    }
    return false;
}

void zwave_tx_queue::collect_session_ids_for_node(const zwave_node_id_t node_id, std::vector<zwave_tx_session_id_t> &ids) const
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    for (const auto *it = queue.begin(); it != queue.end(); ++it) {
        if ((!it->connection_info.remote.is_multicast) && (it->connection_info.remote.node_id == node_id)) {
            ids.push_back(it->zwave_tx_session_id);
        }
    }
}

void zwave_tx_queue::collect_all_session_ids(std::vector<zwave_tx_session_id_t> &ids) const
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    for (const auto *it = queue.begin(); it != queue.end(); ++it) {
        ids.push_back(it->zwave_tx_session_id);
    }
}

void zwave_tx_queue::collect_descendants(const zwave_tx_session_id_t root, std::vector<zwave_tx_session_id_t> &ids) const
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    if (find(root) == queue.end()) {
        return;
    }
    ids.push_back(root);

    // Repeatedly sweep the queue, adding any element whose parent is already
    // collected, until no more descendants are found. Parents are therefore
    // always added before their children.
    bool added = true;
    while (added) {
        added = false;
        for (const auto *it = queue.begin(); it != queue.end(); ++it) {
            const zwave_tx_session_id_t id = it->zwave_tx_session_id;
            if (std::find(ids.begin(), ids.end(), id) != ids.end()) {
                continue;
            }
            if (it->options.transport.valid_parent_session_id && (std::find(ids.begin(), ids.end(), it->options.transport.parent_session_id) != ids.end())) {
                ids.push_back(id);
                added = true;
            }
        }
    }
}

zwave_tx_session_id_t zwave_tx_queue::find_root(const zwave_tx_session_id_t session_id) const
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    const auto *it = find(session_id);
    if (it == queue.end()) {
        return session_id;
    }

    // Walk up the parent links while the parent is still queued. The number of
    // hops is bounded by the queue size so a corrupted parent link can never
    // spin forever.
    zwave_tx_session_id_t root = session_id;
    for (int hops = 0; (hops < queue.size()) && it->options.transport.valid_parent_session_id; hops++) {
        const auto *parent_it = find(it->options.transport.parent_session_id);
        if (parent_it == queue.end()) {
            break;
        }
        root = it->options.transport.parent_session_id;
        it   = parent_it;
    }
    return root;
}

///////////////////////////////////////////////////////////////////////////////
// Print functions
///////////////////////////////////////////////////////////////////////////////

void zwave_tx_queue::log(bool log_messages_payload) const
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    sl_log_debug(LOG_TAG, "Queue size: %lu\n", (unsigned long)queue.size());
    for (const auto *it = queue.begin(); it != queue.end(); ++it) {
        sl_log_debug(LOG_TAG, "Entry (id=%p): (address %p)\n", it->zwave_tx_session_id, &(*it));
        sl_log_debug(LOG_TAG, "\tCallback: %p, user pointer: %p \n", it->callback_function, it->user);
        sl_log_debug(LOG_TAG, "\tAddresses: (NodeID:Endpoint) %d:%d -> %d:%d, is_multicast: %d\n", it->connection_info.local.node_id, it->connection_info.local.endpoint_id, it->connection_info.remote.node_id, it->connection_info.remote.endpoint_id, it->connection_info.remote.is_multicast);
        sl_log_debug(LOG_TAG, "\tEncapsulation: %d - Number of expected responses: %d\n", it->connection_info.encapsulation, it->options.number_of_responses);
        sl_log_debug(LOG_TAG, "\tQoS priority: %u / fasttrack %d\n", it->options.qos_priority, it->options.fasttrack);
        sl_log_debug(LOG_TAG, "\tDiscard timeout: %d ms\n", it->options.discard_timeout_ms);
        sl_log_debug(LOG_TAG, "\tParent frame: %p, parent frame valid: %d\n", it->options.transport.parent_session_id, it->options.transport.valid_parent_session_id);
        sl_log_debug(LOG_TAG,
                     "\tMulticast group: %p, is first follow-up: %d, send "
                     "follow-ups: %d - Skip back-off %d\n",
                     it->options.transport.group_id,
                     it->options.transport.is_first_follow_up,
                     it->options.send_follow_ups,
                     it->options.transport.ignore_incoming_frames_back_off);
        sl_log_debug(LOG_TAG,
                     "\tTimestamps: queued %lu - transmitted %lu - Transmission "
                     "time (ms): %lu\n",
                     it->queue_timestamp,
                     it->transmission_timestamp,
                     it->transmission_time);
        if (log_messages_payload) {
            const std::string payload = to_hex_string(it->data, it->data_length);
            sl_log_debug(LOG_TAG, "\tFrame payload (hex): %s\n", payload.c_str());
        }
    }
}

void zwave_tx_queue::log_element(const zwave_tx_session_id_t session_id, bool log_frame_payload) const
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    for (const auto *it = queue.begin(); it != queue.end(); ++it) {
        if (it->zwave_tx_session_id == session_id) {
            sl_log_debug(LOG_TAG,
                         "Entry (id=%p): (address %p), Qos: %u, discard timeout: %d ms, responses: %d\
 Addresses: (NodeID:Endpoint) %d:%d -> %d:%d. Multicast = %d\
 Parent frame: %p, parent frame valid: %d Ignore back-off: %d\
 fasttrack: %d, Queue timestamp: %lu, transmission timestamp: %lu, transmission time (ms): %lu\n",
                         it->zwave_tx_session_id,
                         &(*it),
                         it->options.qos_priority,
                         it->options.discard_timeout_ms,
                         it->options.number_of_responses,
                         it->connection_info.local.node_id,
                         it->connection_info.local.endpoint_id,
                         it->connection_info.remote.node_id,
                         it->connection_info.remote.endpoint_id,
                         it->connection_info.remote.is_multicast,
                         it->options.transport.parent_session_id,
                         it->options.transport.valid_parent_session_id,
                         it->options.transport.ignore_incoming_frames_back_off,
                         it->options.fasttrack,
                         it->queue_timestamp,
                         it->transmission_timestamp,
                         it->transmission_time);
            if (log_frame_payload) {
                const std::string payload = to_hex_string(it->data, it->data_length);
                sl_log_debug(LOG_TAG, "Frame payload (hex): %s\n", payload.c_str());
            }
            return;
        }
    }
    sl_log_warning(LOG_TAG, "Element (id=%p) is not in the queue\n", session_id);
}

void zwave_tx_queue::simple_log(zwave_tx_queue_element_t *e) const
{
    std::ostringstream oss;
    oss << "Enqueuing new frame (id=" << e->zwave_tx_session_id << ")";

    if (e->options.transport.valid_parent_session_id) {
        oss << " (parent id=" << e->options.transport.parent_session_id << ")";
    }

    oss << " - " << e->connection_info.local.node_id << ":" << static_cast<unsigned>(e->connection_info.local.endpoint_id) << " -> ";

    if (!e->connection_info.remote.is_multicast) {
        oss << " " << e->connection_info.remote.node_id << ":" << static_cast<unsigned>(e->connection_info.remote.endpoint_id) << " - ";
    } else {
        oss << "Group ID " << static_cast<unsigned>(e->connection_info.remote.multicast_group) << " (endpoint=" << static_cast<unsigned>(e->connection_info.remote.endpoint_id) << ") - ";
    }

    oss << "Encapsulation " << e->connection_info.encapsulation << " - Payload (" << e->data_length << " bytes) [" << to_hex_string(e->data, e->data_length) << "]";

    sl_log_debug(LOG_TAG, "%s - Tx Queue size: %d\n", oss.str().c_str(), queue.size());
}

void zwave_tx_queue::log_per_node_distribution() const
{
    std::lock_guard<std::recursive_mutex> lock(queue_mutex_);
    if (per_node_frame_count_.empty()) {
        sl_log_debug(LOG_TAG, "TX queue node distribution: total=%d (no singlecast frames)\n", queue.size());
        return;
    }
    std::ostringstream oss;
    for (const auto &entry: per_node_frame_count_) {
        oss << " node=" << entry.first << ":" << entry.second;
    }
    sl_log_debug(LOG_TAG, "TX queue node distribution: total=%d limit_per_node=%d%s\n", queue.size(), ZWAVE_TX_QUEUE_MAX_FRAMES_PER_NODE, oss.str().c_str());
}

zwave_tx_queue::queue_iterator zwave_tx_queue::find(const zwave_tx_session_id_t key)
{
    // TODO: Linear search of elements is rather slow. this can easily be
    // optimized to a constant complexity without excessive more memory. e.g. by
    // implementing a lookup inside the priority queue.
    return std::find_if(queue.begin(), queue.end(), [&key](const zwave_tx_queue_element_t &item) { return item.zwave_tx_session_id == key; });
}

zwave_tx_queue::const_queue_iterator zwave_tx_queue::find(const zwave_tx_session_id_t key) const
{
    // TODO: Linear search of elements is rather slow. this can easily be
    // optimized to a constant complexity without excessive more memory. e.g. by
    // implementing a lookup inside the priority queue.
    return std::find_if(queue.begin(), queue.end(), [&key](const zwave_tx_queue_element_t &item) { return item.zwave_tx_session_id == key; });
}
