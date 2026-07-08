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

#include "attribute_resolver.hpp"

// Includes from this component
#include "attribute_resolver.h"

#include "attribute_resolver_rule_internal.h"
#include "attribute_resolver_rule_internal.hpp"
#include "attribute_resolver_rule.h"
#include "attribute.hpp"
// Generic includes
#include <set>
#include <map>
#include <mutex>
#include <vector>
#include <memory>

// ZPC components
#include "attribute_store_helper.h"
#include "timer.hpp"
#include "log.h"

constexpr char LOG_TAG[]                   = "attribute_resolver_rule";
constexpr int DEFAULT_GROUPING_DEPTH       = 1;
constexpr clock_time_t MAX_RESOLUTION_TIME = 60 * CLOCK_SECOND;

struct attribute_rules {
        attribute_resolver::attribute_resolver_function set_func;
        attribute_resolver::attribute_resolver_function get_func;
};
using attribute_rules_ptr = std::shared_ptr<attribute_rules>;

static enum {
    RESOLVER_IDLE,
    RESOLVER_EXECUTING_SET_RULE,
    RESOLVER_EXECUTING_GET_RULE,
} resolver_state;

static std::map<attribute_store_type_t, attribute_rules_ptr> rule_book;
// To save time we also have an inverted rule book
// This allows us to quickly find all attributes that belong to the same rule book
static std::multimap<attribute_rules_ptr, attribute_store_type_t> rule_book_inverted;

static std::map<attribute_store_type_t, int> relatives;

static attribute_rule_complete_t compl_func;
static attribute_store_node_t node_pending_resolution = ATTRIBUTE_STORE_INVALID_NODE;
static bool node_needs_more_frames                    = false;
// Timer to limit how long we try to execute a rule
static struct timer_handle_t rule_execution_timer = {nullptr};

// We have a list of "callback" functions,
// that we notify when we get new "set" rules
static std::set<resolver_on_set_rule_registered_t> set_rule_listeners;

// Protects all mutable state in this file: resolver_state, node_pending_resolution,
// node_needs_more_frames, rule_book, rule_book_inverted, relatives, compl_func,
// set_rule_listeners, rule_execution_timer.
// Lock ordering: rule_mutex must NEVER be held while calling compl_func, because
// compl_func (on_resolver_rule_execute_complete) acquires resolver_mutex. To call
// compl_func, snapshot the values under rule_mutex, release it, then invoke.
static std::mutex rule_mutex;

/**
 * @brief Prints the name of the resolution status code (resolver_send_status_t)

 * @param status resolver_send_status_t that we want a string representation of
 * @return const char*
 */
static const char *attribute_resolver_get_send_status_name(resolver_send_status_t status)
{
    switch (status) {
        case RESOLVER_SEND_STATUS_OK:
            return "Transmit OK, No application status";
        case RESOLVER_SEND_STATUS_OK_EXECUTION_PENDING:
            return "Transmit OK, Application working";
        case RESOLVER_SEND_STATUS_FAIL:
            return "Transmit Failed";
        case RESOLVER_SEND_STATUS_OK_EXECUTION_VERIFIED:
            return "Transmit OK, Application OK";
        case RESOLVER_SEND_STATUS_OK_EXECUTION_FAILED:
            return "Transmit OK, Application Failed";
        case RESOLVER_SEND_STATUS_ALREADY_HANDLED:
            return "Already handled (custom handler)";
        case RESOLVER_SEND_STATUS_ABORTED:
            return "Aborted";
        default:
            thread_local static std::string message;
            message = "Unknown status: " + std::to_string(status);
            return message.c_str();
    }
}

/**
 * @brief Prints the name of a rule type

 * @param status resolver_send_status_t that we want a string representation of
 * @return const char*
 */
static const char *attribute_resolver_get_rule_type_name(resolver_rule_type_t rule_type)
{
    switch (rule_type) {
        case RESOLVER_SET_RULE:
            return "Set rule";
        case RESOLVER_GET_RULE:
            return "Get rule";
        default:
            thread_local static std::string message;
            message = "Unknown rule type: " + std::to_string(rule_type);
            return message.c_str();
    }
}

void on_rule_execution_timeout(void *user)
{
    attribute_store_node_t timed_out_node;
    resolver_rule_type_t rule_type;
    {
        std::lock_guard<std::mutex> lk(rule_mutex);
        timed_out_node = node_pending_resolution;
        rule_type      = (resolver_state == RESOLVER_EXECUTING_SET_RULE) ? RESOLVER_SET_RULE : RESOLVER_GET_RULE;
    }
    sl_log_debug(LOG_TAG, "Rule timeout: node=%d rule_type=%d", timed_out_node, rule_type);
    // Pass 0 as transmission_time so GET retry uses only get_retry_timeout,
    // not another MAX_RESOLUTION_TIME wait after we already waited 60s.
    on_resolver_send_data_complete(RESOLVER_SEND_STATUS_FAIL, 0, timed_out_node, rule_type);
}

/**
 * @brief Find other attribute belonging to the same rule.

 * @param rule_type
 * @param attribute_type
 * @return std::set<attribute_store_type_t>
 */
static void attribute_resolver_rule_get_group(resolver_rule_type_t rule_type, attribute_store_type_t attribute_type, std::set<attribute_store_type_t> &output)
{
    auto attribute_rule = rule_book[attribute_type];
    if (attribute_rule != nullptr) {
        auto rule_range = rule_book_inverted.equal_range(attribute_rule);
        for (auto i = rule_range.first; i != rule_range.second; ++i) {
            if (rule_type == RESOLVER_GET_RULE && i->first->get_func != nullptr) {
                output.insert(i->second);
            } else if (rule_type == RESOLVER_SET_RULE && i->first->set_func != nullptr) {
                output.insert(i->second);
            }
        }
    }
}

std::set<attribute_store_node_t> attribute_resolver_rule_get_group_nodes(resolver_rule_type_t rule_type, attribute_store_node_t _node)
{
    std::set<attribute_store_node_t> result;
    std::set<attribute_store_type_t> group_types;
    attribute_store::attribute node = _node;

    if (!node.is_valid()) {
        return result;
    }

    int depth = DEFAULT_GROUPING_DEPTH;
    {
        std::lock_guard<std::mutex> lk(rule_mutex);
        attribute_resolver_rule_get_group(rule_type, node.type(), group_types);
        if (relatives.contains(node.type())) {
            depth = relatives.at(node.type());
        }
    }

    // find common ancestor
    attribute_store::attribute parent = node;
    for (int i = 0; i < depth; i++) {
        parent = parent.parent();
    }
    // Go though the tree an collect relatives on the same level
    parent.visit([=, &result](attribute_store::attribute &child, int level) -> sl_status_t {
        if (level > depth) {
            return SL_STATUS_SUSPENDED;
        }
        if (level == depth) {
            if (group_types.contains(child.type())) {
                result.insert(child);
            }
        }
        return SL_STATUS_OK;
    });

    return result;
}

void on_resolver_send_data_complete(resolver_send_status_t status, clock_time_t transmission_time, attribute_store_node_t _node, resolver_rule_type_t rule_type)
{
    using namespace attribute_store;
    attribute node = _node;

    // Snapshot needs_more_frames under lock before any attribute store access.
    bool needs_more_frames = false;
    {
        std::lock_guard<std::mutex> lk(rule_mutex);
        if ((_node == node_pending_resolution) && (_node != ATTRIBUTE_STORE_INVALID_NODE)) {
            needs_more_frames = node_needs_more_frames;
        }
    }

    sl_log_debug(LOG_TAG,
                 "%s send data complete for Attribute ID %d: "
                 "Status: %s - more_frames = %d",
                 attribute_resolver_get_rule_type_name(rule_type),
                 node,
                 attribute_resolver_get_send_status_name(status),
                 needs_more_frames);

    if (rule_type == RESOLVER_SET_RULE) {
        // Log the state of the node, so we can see the transformation operated here.
        attribute_store_log_node(node, false);
    }

    switch (status) {
        case RESOLVER_SEND_STATUS_OK:
            if (rule_type == RESOLVER_SET_RULE) {
                // We need to update the reported value, undefine it to trigger
                // a get rule.
                attribute_store_undefine_reported(node);
                if (!needs_more_frames) {
                    for (attribute_store::attribute a: attribute_resolver_rule_get_group_nodes(rule_type, node)) {
                        // Now we can align the reported to the desired value
                        a.clear_reported();
                        a.clear_desired();
                        attribute_store_log_node(node, false);
                    }
                }
            }
            break;

        case RESOLVER_SEND_STATUS_FAIL:
            // Roll back the desired, if it was a set rule.
            if (rule_type == RESOLVER_SET_RULE) {
                for (attribute_store::attribute a: attribute_resolver_rule_get_group_nodes(rule_type, node)) {
                    // Now we can align the reported to the desired value
                    a.clear_desired();
                    attribute_store_log_node(a, false);
                }
            }
            break;

        case RESOLVER_SEND_STATUS_OK_EXECUTION_PENDING:
            // We just wait for a subsequent callback. Nothing to do here.
            break;

        case RESOLVER_SEND_STATUS_OK_EXECUTION_VERIFIED:
            if (rule_type == RESOLVER_SET_RULE) {
                if (!needs_more_frames) {
                    for (attribute_store::attribute a: attribute_resolver_rule_get_group_nodes(rule_type, node)) {
                        // Now we can align the reported to the desired value
                        a.set_reported(a.desired_or_reported<std::vector<uint8_t>>());
                        a.clear_desired();
                        attribute_store_log_node(a, false);
                    }
                }
            }
            break;
        case RESOLVER_SEND_STATUS_OK_EXECUTION_FAILED:
            if (rule_type == RESOLVER_SET_RULE) {
                if (!needs_more_frames) {
                    for (attribute_store::attribute a: attribute_resolver_rule_get_group_nodes(rule_type, node)) {
                        // Now we can align the reported to the desired value
                        a.clear_reported();
                        a.clear_desired();
                        attribute_store_log_node(node, false);
                    }
                }
            }
            break;
        case RESOLVER_SEND_STATUS_ALREADY_HANDLED:
        case RESOLVER_SEND_STATUS_ABORTED:
            break;
    }

    // If this is the callback for the node we were waiting on, reset state and
    // invoke compl_func OUTSIDE rule_mutex (compl_func acquires resolver_mutex;
    // holding both would invert the documented lock order).
    attribute_rule_complete_t local_compl = nullptr;
    attribute_store_node_t local_node     = ATTRIBUTE_STORE_INVALID_NODE;
    clock_time_t local_tx_time            = 0;
    {
        std::lock_guard<std::mutex> lk(rule_mutex);
        if (node_pending_resolution == _node) {
            local_compl             = compl_func;
            local_node              = _node;
            local_tx_time           = transmission_time;
            node_pending_resolution = ATTRIBUTE_STORE_INVALID_NODE;
            resolver_state          = RESOLVER_IDLE;
            timer_stop(&rule_execution_timer);
        }
    }
    if (local_compl != nullptr) {
        local_compl(local_node, local_tx_time);
    }
}

void attribute_resolver_register_set_rule_listener(resolver_on_set_rule_registered_t function)
{
    // Snapshot existing set-rule types under lock, then notify outside.
    std::vector<attribute_store_type_t> existing_set_types;
    {
        std::lock_guard<std::mutex> lk(rule_mutex);
        set_rule_listeners.insert(function);
        for (auto it = rule_book.begin(); it != rule_book.end(); ++it) {
            if (it->second->set_func != nullptr) {
                existing_set_types.push_back(it->first);
            }
        }
    }
    for (auto node_type: existing_set_types) {
        function(node_type);
    }
}

sl_status_t attribute_resolver_rule_execute(attribute_store_node_t node, bool set_rule)
{
    uint8_t frame[MAX_FRAME_LEN] = {0};
    uint16_t frame_size          = 0;

    attribute_store_type_t attribute_type = attribute_store_get_node_type(node);

    // Phase 1: validate preconditions and claim the executor slot under lock.
    // resolver_state and node_pending_resolution are set here — before releasing
    // the lock — so that (a) other callers see BUSY and (b) the synchronous
    // TX-failure callback path (on_resolver_send_data_complete) finds the correct
    // pending node when it re-acquires rule_mutex.
    attribute_resolver::attribute_resolver_function func = nullptr;
    {
        std::lock_guard<std::mutex> lk(rule_mutex);

        if (!rule_book.contains(attribute_type)) {
            return SL_STATUS_NOT_FOUND;
        }

        if (resolver_state != RESOLVER_IDLE) {
            sl_log_debug(LOG_TAG, "Resolver busy! waiting for node %d", node_pending_resolution);
            attribute_store_log_node(node_pending_resolution, false);
            sl_log_debug(LOG_TAG, "Rule execute busy: requested_node=%d pending_node=%d set_rule=%d tid=%lu", node, node_pending_resolution, set_rule ? 1 : 0, sl_log_thread_id());
            return SL_STATUS_BUSY;
        }

        const auto rules = rule_book[attribute_type];
        func             = set_rule ? rules->set_func : rules->get_func;

        if (!func) {
            return SL_STATUS_NOT_SUPPORTED;
        }

        resolver_state          = set_rule ? RESOLVER_EXECUTING_SET_RULE : RESOLVER_EXECUTING_GET_RULE;
        node_pending_resolution = node;
        node_needs_more_frames  = false;
    }

    // Phase 2: generate frame and transmit WITHOUT rule_mutex held.
    // send() may synchronously invoke on_resolver_zwave_send_data_complete on
    // failure, which calls on_resolver_send_data_complete, which re-acquires
    // rule_mutex. Holding rule_mutex here would deadlock on the same thread.
    auto reset_state = [&]() {
        std::lock_guard<std::mutex> lk(rule_mutex);
        if (node_pending_resolution == node) {
            resolver_state          = RESOLVER_IDLE;
            node_pending_resolution = ATTRIBUTE_STORE_INVALID_NODE;
        }
    };

    sl_status_t frame_status;
    try {
        frame_status = func(node, frame, &frame_size);
    } catch (const std::exception &e) {
        sl_log_debug("Exception in rule execution %s", e.what());
        reset_state();
        return SL_STATUS_NOT_SUPPORTED;
    }

    if ((frame_status == SL_STATUS_OK) || (frame_status == SL_STATUS_IN_PROGRESS)) {
        if (frame_size > sizeof(frame)) {
            reset_state();
            return SL_STATUS_WOULD_OVERFLOW;
        }

        if (attribute_resolver_get_config().send(node, frame, frame_size, set_rule) == SL_STATUS_OK) {
            sl_log_debug(LOG_TAG, "Rule send accepted: node=%d set_rule=%d frame_size=%u frame_status=0x%02X tid=%lu", node, set_rule ? 1 : 0, frame_size, frame_status, sl_log_thread_id());
            // send() queued the frame; finalize remaining state under lock.
            // The synchronous failure callback cannot have fired (send returned OK),
            // so node_pending_resolution is still set to node.
            std::lock_guard<std::mutex> lk(rule_mutex);
            if (node_pending_resolution == node) {
                node_needs_more_frames = (frame_status == SL_STATUS_IN_PROGRESS);
                timer_set(&rule_execution_timer, MAX_RESOLUTION_TIME, &on_rule_execution_timeout, nullptr);
            }
            return (frame_status == SL_STATUS_IN_PROGRESS) ? SL_STATUS_IN_PROGRESS : SL_STATUS_OK;
        }
        sl_log_debug(LOG_TAG, "Rule send rejected: node=%d set_rule=%d frame_size=%u frame_status=0x%02X tid=%lu", node, set_rule ? 1 : 0, frame_size, frame_status, sl_log_thread_id());
        // send() failed; the synchronous callback already fired and reset
        // resolver_state/node_pending_resolution via on_resolver_send_data_complete.
        // reset_state() is a no-op if the callback already cleared the slot.
        reset_state();
        return SL_STATUS_NOT_READY;
    }

    reset_state();

    if (frame_status == SL_STATUS_ALREADY_EXISTS) {
        return SL_STATUS_ALREADY_EXISTS;
    }
    if (frame_status == SL_STATUS_IS_WAITING) {
        return SL_STATUS_IS_WAITING;
    }
    if (frame_status == SL_STATUS_NOT_READY) {
        sl_log_debug(LOG_TAG, "Frame not ready for node type 0x%X. Will retry.", attribute_type);
        return SL_STATUS_IS_WAITING;
    }
    sl_log_info(LOG_TAG,
                "Unexpected frame status: 0x%02X. Please verify that the frame "
                "resolution function for node type 0x%X respects the return codes!",
                frame_status,
                attribute_type);
    return SL_STATUS_NOT_SUPPORTED;
}

void attribute_resolver_rule_init(attribute_rule_complete_t __compl_func)
{
    std::lock_guard<std::mutex> lk(rule_mutex);
    relatives.clear();
    compl_func = __compl_func;
    rule_book.clear();
    resolver_state = RESOLVER_IDLE;
}

bool attribute_resolver_rule_busy()
{
    std::lock_guard<std::mutex> lk(rule_mutex);
    if (resolver_state != RESOLVER_IDLE) {
        sl_log_debug(LOG_TAG, "Resolver busy! waiting for node %d", node_pending_resolution);
        attribute_store_log_node(node_pending_resolution, false);
    }
    return resolver_state != RESOLVER_IDLE;
}

void attribute_resolver_rule_abort(attribute_store_node_t node)
{
    // Snapshot compl_func under lock; invoke outside (compl_func acquires resolver_mutex).
    attribute_rule_complete_t local_compl = nullptr;
    {
        std::lock_guard<std::mutex> lk(rule_mutex);
        if (node_pending_resolution == node) {
            local_compl             = compl_func;
            node_pending_resolution = ATTRIBUTE_STORE_INVALID_NODE;
            resolver_state          = RESOLVER_IDLE;
            timer_stop(&rule_execution_timer);
        }
    }
    if (local_compl != nullptr) {
        local_compl(node, 0);
    }
}

bool attribute_resolver_has_set_rule(attribute_store_type_t node_type)
{
    return attribute_resolver::set_function(node_type) != nullptr;
}

bool attribute_resolver_has_get_rule(attribute_store_type_t node_type)
{
    return attribute_resolver::get_function(node_type) != nullptr;
}

sl_status_t attribute_resolver_set_attribute_depth(attribute_store_type_t node_type, int depth)
{
    std::lock_guard<std::mutex> lk(rule_mutex);
    relatives.erase(node_type);
    relatives.insert(std::make_pair(node_type, depth));
    return SL_STATUS_OK;
}

///////////////////////////////////////////////////////////////////////////
/// C++ Wrapper
///////////////////////////////////////////////////////////////////////////
namespace attribute_resolver
{

    attribute_resolver_function set_function(attribute_store_type_t node_type)
    {
        std::lock_guard<std::mutex> lk(rule_mutex);
        if (!rule_book.contains(node_type)) {
            // No rule at all for this attribute
            return nullptr;
        }
        return rule_book[node_type]->set_func;
    }

    attribute_resolver_function get_function(attribute_store_type_t node_type)
    {
        std::lock_guard<std::mutex> lk(rule_mutex);
        if (!rule_book.contains(node_type)) {
            // No rule at all for this attribute
            return nullptr;
        }
        return rule_book[node_type]->get_func;
    }

    void helper_register_rules(attribute_store_type_t node_type, attribute_rules_ptr rules, bool notify_set_rule_listeners)
    {
        // Snapshot listeners under lock; notify outside to avoid calling arbitrary
        // code while rule_mutex is held.
        std::vector<resolver_on_set_rule_registered_t> listeners_to_notify;
        {
            std::lock_guard<std::mutex> lk(rule_mutex);
            rule_book[node_type] = rules;
            rule_book_inverted.insert({rules, node_type});
            if (notify_set_rule_listeners) {
                listeners_to_notify.assign(set_rule_listeners.begin(), set_rule_listeners.end());
            }
        }
        for (auto listener: listeners_to_notify) {
            listener(node_type);
        }
    }

    void register_rules_internal(attribute_store_type_t node_type, attribute_resolver_function set_func, attribute_resolver_function get_func)
    {
        auto rules      = std::make_shared<attribute_rules>();
        rules->set_func = set_func;
        rules->get_func = get_func;

        helper_register_rules(node_type, rules, set_func != nullptr);
    }

    void register_group_rules_internal(const std::set<attribute_store_type_t> &nodes, attribute_resolver_function set_func, attribute_resolver_function get_func)
    {
        auto rules      = std::make_shared<attribute_rules>();
        rules->set_func = set_func;
        rules->get_func = get_func;

        for (auto node_type: nodes) {
            helper_register_rules(node_type, rules, set_func != nullptr);
        }
    }

}  // namespace attribute_resolver
