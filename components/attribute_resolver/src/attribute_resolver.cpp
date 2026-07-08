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
#include "attribute_resolver.h"
#include "attribute_resolver_internal.h"
#include "attribute_resolver_rule_internal.h"
#include "attribute_resolver_rule_internal.hpp"

#include "attribute.hpp"
#include "attribute_resolver.hpp"
#include "attribute_resolver_handler.hpp"

// Includes from other components
#include "multi_invoke.hpp"  // Internal template utility
#include "attribute_store_helper.h"
#include "attribute_store_type_registration.h"
#include "log.h"
#include "clock_platform.h"
#include "timer.hpp"

#include <atomic>
#include <mutex>

constexpr char LOG_TAG[] = "attribute_resolver";

// Generic includes
#include <unordered_set>
#include <deque>
#include <map>
#include <string>
#include <sstream>
#include <inttypes.h>
#include <cassert>
using namespace attribute_store;

using resumption_listeners_t = std::multimap<attribute_store_node_t, attribute_resolver_callback_t>;

// Private variables
namespace
{
    // A stack of node that need either Set or Get resolution
    std::deque<std::pair<attribute_store_node_t, uint32_t>> stack;
    // A list of nodes that are paused and should not be resolved until they are resumed.
    std::unordered_set<attribute_store_node_t> paused_nodes;
    // List of callbacks to invoke when a resolution has been performed on a subtree.
    multi_invoke<attribute_store_node_t, attribute_store_node_t> listeners;
    // List of callback functions that want to be informed when we give up trying to
    // make a get resolution on a node.
    multi_invoke<attribute_store_type_t, attribute_store_node_t> get_give_up_listeners;
    // List of callback functions that want to be informed when resume the resolution
    // on a node.
    resumption_listeners_t resumption_listeners;
    attribute_resolver_config_t attribute_resolver_config;
}  // namespace

/*
 * Set of nodes for which we have sent a get. The value of the map
 * has information about the number of gets already tried as
 * well as when the get was sent.
 */
typedef struct {
        clock_time_t send_timeout;
        uint8_t count;
} pending_get_t;

// Pending Get resolutions
static std::map<attribute_store_node_t, pending_get_t> pending_get_resolutions;
struct timer_handle_t pending_get_resume_timer = {0};
// Pending Sets resolutions <map of attribute nodes / needs more frame boolean
static std::map<attribute_store_node_t, bool> pending_set_resolutions;
// Earliest clock_time() at which a SET on this node may be re-attempted after
// a transmission failure.  Mirrors pending_get_resolutions[].send_timeout.
static std::map<attribute_store_node_t, clock_time_t> set_retry_cooldown_until_;
// Group nodes that asked for a re-arm of their Set resolution because a Set
// was already in flight when the new request came in. The in-flight completion
// callback would otherwise clear desired/reported and silently drop the queued
// request. Key: group attribute node, value: retry count to restore on
// `desired` once the resolver becomes idle again.
static std::map<attribute_store_node_t, uint8_t> rearm_after_completion;

// Has a full scan of the attribute store been requested
static std::atomic<bool> scan_requested;

// Protects pending_get_resolutions, pending_set_resolutions, stack, and
// paused_nodes from concurrent access across the resolver thread, attribute
// store callbacks, and TX-complete callbacks.  Recursive because several
// code-paths re-enter (e.g. give_up_get_resolution_on_group triggers
// attribute store callbacks that call on_resolver_node_update).
static std::recursive_mutex resolver_mutex;

// Forward declaration for static functions
static bool needs_get(attribute_store_node_t node);
static bool needs_set(attribute_store_node_t node);
static void on_resolver_node_deleted(attribute_store_node_t node);
static void pending_get_resume_timer_expired_event(void *ptr);
static uint8_t get_node_retry_count(attribute_store_node_t node);

/**
 * @brief This function traverses attribute store to check if any parent
 *        of the attribute node has a resolution listener
 *
 * @param node  Attribute node
 * @returns   The Attribute Store ID of the first parent with a resolution
 *            listener, if any. It can be the node itself.
 *            ATTRIBUTE_STORE_INVALID_NODE if none if the node and none of its
 *            parents have a resolution listener
 */
static attribute_store_node_t get_highest_parent_with_resolution_listener(attribute_store_node_t node);

/**
 * @brief This function returns true if a resolution is ongoing for the node
 *
 * @param node  Attribute node
 * @returns   true if the node is pending a resolution
 *            false if the node is not pending a resolution
 */
static bool is_node_under_resolution(attribute_store_node_t node);

/**
 * @brief Verifies if we gave up trying to resolve a get
 *
 * i.e. we tried up to maximum number of retries and stalled there.
 *
 * @param node  Attribute node
 * @returns     true if the node has reached the maximum number of retries
 *              false if the node has not reached the maximum number of retries
 */
static bool is_node_get_resolution_max_retries_reached(attribute_store_node_t node);

/**
 * @brief This function traverses the stack and set the child_index of the common parent to 0.
 *        This enables the attribute resolver to scan updated attribute node.
 *
 * @param node  Attribute node
 */
static void set_common_parent_stack_index_zero(attribute_store_node_t node);

/**
 * @brief This function traverses the list of nodes pending a get resolution
 *        response and restarts a timer expiring when the next node is to be
 *        retried.
 */
static void attribute_resolver_restart_pending_get_nodes_timer();

/**
 * @brief This function traverses the list of nodes pending a get resolution
 *        response and ensures that they get scanned if it's time for them
 *        to get scanned again.
 */
static void attribute_resume_expired_pending_get_nodes();

/**
 * @brief Ensures that a node gets scanned, as part of the current
 *        scan or by starting a new scan.
 */
static void scan_node(attribute_store_node_t node_to_scan);

/**
 * @brief Helper function verifying that all the conditions are met
 *        to schedule a scan for node
 *
 * <!> this only checks for the node, not for its children.
 * It can be that some children need a scan
 *
 * @param node  Attribute node
 * @returns   true if the node must be scanned/resolved.
 *            false if the node does not need to be scanned
 */
static bool is_node_to_be_scanned(attribute_store_node_t node);

/**
 * @brief Marks all nodes of a resolution group as "given up" if we gave up
 * trying to resolve one.
 *
 * @param node Node that we gave up on.
 */
static void give_up_get_resolution_on_group(attribute_store_node_t node);

// Static event queue for the attribute resolver handler
namespace zwave_component
{
    ::threading::safe_queue<attribute_resolver_handler::attribute_resolver_event_data> attribute_resolver_handler::event_queue;
}

void attribute_resolver_pause_node_resolution(attribute_store_node_t node)
{
    sl_log_debug(LOG_TAG, "Pause wait mutex: node=%d tid=%lu", node, sl_log_thread_id());
    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
    sl_log_debug(LOG_TAG, "Pause got mutex: node=%d tid=%lu", node, sl_log_thread_id());
    sl_log_debug(LOG_TAG, "Resolution paused on Attribute ID %d", node);
    paused_nodes.insert(node);
}

void attribute_resolver_resume_node_resolution(attribute_store_node_t node)
{
    sl_log_debug(LOG_TAG, "Resume wait mutex: node=%d tid=%lu", node, sl_log_thread_id());
    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
    sl_log_debug(LOG_TAG, "Resume got mutex: node=%d tid=%lu", node, sl_log_thread_id());
    sl_log_debug(LOG_TAG, "Resolution resumed on Attribute ID %d", node);
    paused_nodes.erase(node);

    // Tell the world that we resumed resolution on a bunch of nodes.
    // Make a copy of the container, in case some function un-register themselves
    // when invoking the callback (would invalidate the container)
    resumption_listeners_t listeners_to_invoke;
    for (const auto &[registered_node, callback]: resumption_listeners) {
        if ((attribute_store_is_node_a_child(registered_node, node) || (registered_node == node)) && (!is_node_or_parent_paused(registered_node))) {
            listeners_to_invoke.emplace(registered_node, callback);
        }
    }
    for (const auto &[registered_node, callback]: listeners_to_invoke) {
        callback(registered_node);
    }

    // It can be that the node itself does not need to be scanned, but some
    // unpaused children do. So schedule a scan again unless we are
    // waiting for a response.
    // If a parent of this node is paused, the stack will detect it immediately
    if (!is_node_under_resolution(node)) {
        scan_node(node);
    }
}

void attribute_resolver_set_resolution_listener(attribute_store_node_t node, attribute_resolver_callback_t callback)
{
    if (node == ATTRIBUTE_STORE_INVALID_NODE) {
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
    listeners.add(node, callback);

    // Ensure that if somebody registers a listener while no scan is ongoing
    // and nothing needs to be resolved, they get a notification for their node
    // as soon as possible
    scan_node(node);
}

void attribute_resolver_clear_resolution_listener(attribute_store_node_t node, attribute_resolver_callback_t callback)
{
    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
    listeners.remove(node, callback);
}

void attribute_resolver_set_resolution_resumption_listener(attribute_store_node_t node, attribute_resolver_callback_t callback)
{
    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
    resumption_listeners.emplace(node, callback);
}

void attribute_resolver_clear_resolution_resumption_listener(attribute_store_node_t node, attribute_resolver_callback_t callback)
{
    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
    auto range = resumption_listeners.equal_range(node);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == callback) {
            resumption_listeners.erase(it);
            break;
        }
    }
}

void attribute_resolver_set_resolution_give_up_listener(attribute_store_type_t node_type, attribute_resolver_callback_t callback)
{
    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
    get_give_up_listeners.add(node_type, callback);
}

sl_status_t attribute_resolver_restart_set_resolution(attribute_store_node_t node)
{
    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
    if (pending_set_resolutions.contains(node)) {
        pending_set_resolutions.erase(node);
        set_retry_cooldown_until_.erase(node);
        attribute_resolver_resume_node_resolution(node);
        // here it's important to ask the lower layers to abort as well,
        // as some components (e.g. zpc_resolver_group start resolving sets)
        // before we ask them!
        sl_status_t abort_status = attribute_resolver_config.abort(node);
        scan_node(node);
        return abort_status;
    }
    return SL_STATUS_NOT_FOUND;
}

sl_status_t attribute_resolver_restart_get_resolution(attribute_store_node_t node)
{
    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
    if (pending_get_resolutions.contains(node)) {
        pending_get_resolutions.erase(node);
        scan_node(node);
        return SL_STATUS_OK;
    }
    return SL_STATUS_NOT_FOUND;
}

static size_t restart_exhausted_get_resolutions_locked(attribute_store_node_t node)
{
    size_t restarted = 0;
    if (needs_get(node) && is_node_get_resolution_max_retries_reached(node)) {
        pending_get_resolutions.erase(node);
        scan_node(node);
        restarted = 1;
    }

    const uint32_t child_count = attribute_store_get_node_child_count(node);
    for (uint32_t i = 0; i < child_count; i++) {
        restarted += restart_exhausted_get_resolutions_locked(attribute_store_get_node_child(node, i));
    }
    return restarted;
}

size_t attribute_resolver_restart_exhausted_get_resolutions(attribute_store_node_t node)
{
    if (node == ATTRIBUTE_STORE_INVALID_NODE) {
        return 0;
    }
    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
    const size_t restarted = restart_exhausted_get_resolutions_locked(node);
    if (restarted > 0) {
        sl_log_debug(LOG_TAG, "Restarted %zu exhausted Get resolution(s) under Attribute ID %d", restarted, node);
    }
    return restarted;
}

attribute_resolver_config_t attribute_resolver_get_config()
{
    return attribute_resolver_config;
}

bool attribute_resolver_node_or_child_needs_resolution(attribute_store_node_t node)
{
    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
    if (needs_get(node)) {
        const auto it = pending_get_resolutions.find(node);
        if ((it == pending_get_resolutions.end()) || (!is_node_get_resolution_max_retries_reached(node))) {
            return true;
        }
    }

    if (needs_set(node)) {
        return true;
    }

    for (uint32_t i = 0; i < attribute_store_get_node_child_count(node); i++) {
        if (attribute_resolver_node_or_child_needs_resolution(attribute_store_get_node_child(node, i))) {
            return true;
        }
    }
    return false;
}

bool is_node_or_parent_paused(attribute_store_node_t node)
{
    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
    for (attribute updated_node = node; updated_node.is_valid(); updated_node = updated_node.parent()) {
        if (paused_nodes.contains(updated_node)) {
            return true;
        }
    }
    return false;
}

bool is_node_pending_set_resolution(attribute_store_node_t node)
{
    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
    return pending_set_resolutions.contains(node);
}

void attribute_resolver_request_rearm_after_completion(attribute_store_node_t node, uint8_t retry_count)
{
    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
    rearm_after_completion[node] = retry_count;
}

void attribute_resolver_start_group_resolution(attribute_store_node_t node, group_resolution_options options)
{
    // Hold resolver_mutex across the entire check-and-act so the resolver
    // thread cannot start a SET rule between the pending-check and the
    // attribute mutations (TOCTOU). resolver_mutex is recursive, so the
    // on_resolver_node_update callbacks fired by set_desired_and_undefine_reported
    // can safely re-acquire it on the same thread.
    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
    // Register before the pending-set check so skip_supervision applies even
    // when resolution is deferred to rearm_after_completion.
    if (options.skip_supervision) {
        attribute_resolver_register_no_supervision_type(attribute_store_get_node_type(node));
    }
    if (pending_set_resolutions.contains(node)) {
        rearm_after_completion[node] = options.retry_count;
        return;
    }
    attribute(node).set_desired_and_undefine_reported<uint8_t>(options.retry_count);
}

void attribute_resolver_state_log()
{
    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
    sl_log_debug(LOG_TAG, "Scan Requested: %d", scan_requested.load());
    if (stack.empty()) {
        sl_log_debug(LOG_TAG, "Stack is empty");
    } else {
        sl_log_debug(LOG_TAG, "Stack next node: %d", stack.back().first);
    }

    std::stringstream stream;
    for (auto it = paused_nodes.begin(); it != paused_nodes.end(); ++it) {
        stream << *it;
        stream << " ";
    }

    std::string message = stream.str();
    sl_log_debug(LOG_TAG, "Paused nodes: %lu - [%s]", paused_nodes.size(), message.c_str());

    sl_log_debug(LOG_TAG, "Nodes pending Get resolution: %lu", pending_get_resolutions.size());
    for (auto it = pending_get_resolutions.begin(); it != pending_get_resolutions.end(); ++it) {
        sl_log_debug(LOG_TAG, "\t Node %d, timeout: %lu, retries: %" PRIu8, it->first, it->second.send_timeout, it->second.count);
    }
    if (!timer_expired(&pending_get_resume_timer)) {
        sl_log_debug(LOG_TAG, "Get Retry Timer is running ");
    } else {
        sl_log_debug(LOG_TAG, "Get Retry Timer is not running ");
    }

    stream.str("");
    stream.clear();
    for (auto it = pending_set_resolutions.begin(); it != pending_set_resolutions.end(); ++it) {
        stream << it->first;
        stream << " (more frames = ";
        stream << it->second;
        stream << "),";
    }
    message = stream.str();
    sl_log_debug(LOG_TAG, "Nodes pending Set resolution: %lu - [%s]", pending_set_resolutions.size(), message.c_str());

    stream.str("");
    stream.clear();
    for (auto it = rearm_after_completion.begin(); it != rearm_after_completion.end(); ++it) {
        stream << it->first;
        stream << " (retry count = ";
        stream << static_cast<unsigned int>(it->second);
        stream << "),";
    }
    message = stream.str();
    sl_log_debug(LOG_TAG, "Nodes pending Set re-arm after completion: %lu - [%s]", rearm_after_completion.size(), message.c_str());
}

///////////////////////////////////////////////////////////////////////////////
// Private functions
///////////////////////////////////////////////////////////////////////////////
/**
 * @brief Invokes all the get_give_up listeners for a given node
 *
 * @param node    The attribute store node on which we gave up.
 */
static void invoke_give_up_listeners(attribute_store_node_t node)
{
    attribute_store_type_t node_type = attribute_store_get_node_type(node);
    // Invoke all the callbacks matching the type
    get_give_up_listeners(node_type, node);
}

/**
 * @brief Check if we all ready have a get or set resolution for this attribute node.
 *
 * @param rule_type SET or GET
 * @param node Node to check
 * @return true There is already a pending resolution for this node.
 * @return false  No pending resolution will resolve this.
 */
static bool resolution_already_queued(resolver_rule_type_t rule_type, attribute node)
{
    for (attribute a: attribute_resolver_rule_get_group_nodes(rule_type, node)) {
        if ((rule_type == RESOLVER_GET_RULE) && (pending_get_resolutions.contains(a))) {
            sl_log_debug(LOG_TAG, "Already resolving GET %s vs %s id %i", attribute_store_type_get_node_type_name(a), attribute_store_type_get_node_type_name(node), a);
            return true;
        }
        if ((rule_type == RESOLVER_SET_RULE) && (pending_set_resolutions.contains(a))) {
            sl_log_debug(LOG_TAG, "Already resolving SET %s id %i", attribute_store_type_get_node_type_name(a), a);
            return true;
        }
    }
    return false;
}

static void give_up_get_resolution_on_group(attribute_store_node_t node)
{
    for (attribute a: attribute_resolver_rule_get_group_nodes(RESOLVER_GET_RULE, node)) {
        // If we are not able to get, we will not set either.
        sl_log_debug(LOG_TAG,
                     "Undefining Attribute ID %d desired value after "
                     "failing to resolve the reported value of Attribute ID %d.",
                     a,
                     node);
        attribute_store_undefine_desired(a);
        // Mark the node as "given up on", do that after undefining the desired as
        // updating attributes triggers the resolver to retry
        pending_get_resolutions[a] = {.send_timeout = 0, .count = attribute_resolver_config.get_retry_count};
    }
}

/**
 * @brief Helper function executing a Get rule.
 *
 *
 * @param node  Attribute node that needs a get resolution
 * @returns   SL_STATUS_ABORT if we gave up trying to resolve this node
 *            \ref attribute_resolver_rule_execute return code otherwise
 */
static sl_status_t execute_get(attribute_store_node_t node)
{
    sl_status_t execution_status = SL_STATUS_OK;
    if (pending_get_resolutions.contains(node)) {
        uint8_t retry_count = get_node_retry_count(node);
        if (pending_get_resolutions[node].count >= retry_count) {
            // If we exceed the retry count don't try this anymore
            return SL_STATUS_ABORT;
        }
        if ((clock_time() > pending_get_resolutions[node].send_timeout) && (pending_get_resolutions[node].send_timeout != 0)) {
            pending_get_resolutions[node].count++;
            sl_log_debug(LOG_TAG,
                         "Retransmitting Get command for Attribute ID %d "
                         "(attempt %i out of %i)",
                         node,
                         pending_get_resolutions[node].count,
                         retry_count);
            execution_status = attribute_resolver_rule_execute(node, false);
        } else {
            // We have to wait before a retry.
            return SL_STATUS_IS_WAITING;
        }
    } else {
        if (resolution_already_queued(RESOLVER_GET_RULE, node)) {
            return SL_STATUS_IS_WAITING;
        }
        // Send for the first time.
        execution_status = attribute_resolver_rule_execute(node, false);
        if ((SL_STATUS_OK == execution_status) || (SL_STATUS_IN_PROGRESS == execution_status)) {
            pending_get_resolutions[node] = {.send_timeout = 0, .count = 1};
        } else if (SL_STATUS_IS_WAITING == execution_status) {
            sl_log_debug(LOG_TAG,
                         "Rule status is waiting. "
                         "Scheduling re-resolution of Attribute ID %d in 1000 ms.",
                         node);
            pending_get_resolutions[node] = {.send_timeout = clock_time() + 1000, .count = 0};
            attribute_resolver_restart_pending_get_nodes_timer();
        }
    }

    return execution_status;
}

/**
 * @brief Helper function executing a Set rule.
 *
 *
 * @param node  Attribute node that needs a set resolution
 * @returns   \ref attribute_resolver_rule_execute return code
 *              SL_STATUS_IS_WAITING if the resolver just has to wait and do
 *              nothing about this attribute.
 */
static sl_status_t execute_set(attribute_store_node_t node)
{
    // If it is pending a set (i.e. WORKING), we don't try to resolve again.
    if (pending_set_resolutions.contains(node)) {
        sl_log_debug(LOG_TAG, "Set waiting: node=%d tid=%lu", node, sl_log_thread_id());
        return SL_STATUS_IS_WAITING;
    }

    // Honor SET retry cooldown: nodes arriving via scan_node bypass
    // is_node_to_be_scanned, so we must gate here too.
    auto cd_it = set_retry_cooldown_until_.find(node);
    if (cd_it != set_retry_cooldown_until_.end()) {
        if (clock_time() < cd_it->second) {
            return SL_STATUS_IS_WAITING;  // still cooling down
        }
        set_retry_cooldown_until_.erase(cd_it);  // expired — clean up
    }

    if (resolution_already_queued(RESOLVER_SET_RULE, node)) {
        return SL_STATUS_IS_WAITING;
    }

    // If we are about set this value, reset the pending get resolution.
    pending_get_resolutions.erase(node);

    sl_status_t rule_status = attribute_resolver_rule_execute(node, true);

    sl_log_debug(LOG_TAG, "Set executed: node=%d rule_status=0x%02X tid=%lu", node, rule_status, sl_log_thread_id());

    if (rule_status == SL_STATUS_OK) {
        pending_set_resolutions[node] = false;
    } else if (rule_status == SL_STATUS_IN_PROGRESS) {
        pending_set_resolutions[node] = true;
    }

    return rule_status;
}

/**
 * @brief This function traverses attribute store iteratively by using a stack.
 * Given the nature of a stack, the nodes that get pushed will be visited later
 * on, that is simulating backtracking of recursion.
 *
 * During traversal, all the stack elements are considered as root of a subtree
 * and all their children will be visited. Therefore, if visiting the whole is
 * intended, root node should be pushed before calling this function.
 * E.g.
 *    traverse_stack.push_back(ROOT_NODE);
 *    resolver_find_next_resolve();
 *
 * - Why iteratively traversing, not recursively?
 *   In order to be have asynchronous traversal, the plan is to resolve one node
 *   at a time. The possibility of saving/restoring state of traversal is needed
 *   So in this case, iteratively traversing is easier to achieve the goal.
 *
 * A given node in the tree will only be executed when all its children has
 * been executed. Ie for the tree:
 *
 *         A
 *        / \
 *        B   E
 *       / \
 *      C   D
 *
 * The execution order will be: C D B E A
 */
static void resolver_find_next_resolve()
{
    while (!stack.empty()) {
        attribute_store_node_t node = stack.back().first;
        uint32_t index              = stack.back().second;

        // Node is paused skip this and all its children
        if (is_node_or_parent_paused(node)) {
            stack.pop_back();
            continue;
        }

        // Verify that our stack is not out of sync with the Attribute Store
        if (!attribute_store_node_exists(node)) {
            stack.pop_back();
            continue;
        }
        // One child_count snapshot per iteration (avoid mismatch vs. separate get_node_child_count calls).
        const size_t child_count = attribute_store_get_node_child_count(node);
        if (index > child_count) {
            stack.back().second = 0;
            index               = 0;
        }

        // All children has been visited
        if (index == child_count) {
            stack.pop_back();
            // Get operations have priority.
            // Some resolvers need proper state information before they
            // can set.
            sl_status_t rule_status = SL_STATUS_NOT_INITIALIZED;
            if (needs_get(node)) {
                sl_log_debug(LOG_TAG, "Attribute ID %d needs a Get\n", node);
                rule_status = execute_get(node);

                if (rule_status == SL_STATUS_ABORT) {
                    continue;
                }

            } else if (needs_set(node)) {
                sl_log_debug(LOG_TAG, "Attribute ID %d needs a Set\n", node);
                rule_status = execute_set(node);
            }

            // If a rule was executed successfully, we return and wait for a callback
            if (rule_status == SL_STATUS_OK || rule_status == SL_STATUS_IN_PROGRESS) {
                return;
            }
            // Resolution of this node is done with no frame
            if (rule_status == SL_STATUS_NOT_INITIALIZED || rule_status == SL_STATUS_ALREADY_EXISTS) {
                on_resolver_rule_execute_complete(node, 0);
                return;
            }

            if (rule_status == SL_STATUS_IS_WAITING) {
                sl_log_debug(LOG_TAG, "Attribute ID %d resolution is working. Skipping\n", node);
                continue;
            }

            if (rule_status == SL_STATUS_NOT_READY || rule_status == SL_STATUS_BUSY) {
                sl_log_debug(LOG_TAG,
                             "Send function is not ready to send. "
                             "Skipping attribute ID %d resolution.\n",
                             node);
                continue;
            }

            sl_log_info(LOG_TAG, "Unexpected Resolver rule status: 0x%02X for node %d.", rule_status, node);

        } else {
            if (stack.empty() || stack.back().first != node || stack.back().second != index) {
                sl_log_warning(LOG_TAG,
                               "Resolver stack desync before child push: "
                               "expected node %d index %" PRIu32 ", actual node %d index %" PRIu32,
                               node,
                               index,
                               stack.empty() ? ATTRIBUTE_STORE_INVALID_NODE : stack.back().first,
                               stack.empty() ? 0U : stack.back().second);
                continue;
            }
            // Increment the child index
            stack.back().second++;
            attribute_store_node_t child   = attribute_store_get_node_child(node, index);
            const size_t child_count_after = attribute_store_get_node_child_count(node);
            if (child_count_after != child_count) {
                sl_log_warning(LOG_TAG,
                               "Child count changed during resolver descent: parent=%d "
                               "was %zu now %zu child_index=%" PRIu32,
                               node,
                               child_count,
                               child_count_after,
                               index);
            }
            if (child == ATTRIBUTE_STORE_INVALID_NODE) {
                sl_log_warning(LOG_TAG, "Skipping push of invalid child: parent=%d index=%" PRIu32 " child_count=%zu (parent index advanced, no push)", node, index, child_count);
                continue;
            }
            stack.push_back(std::pair<attribute_store_node_t, int>(child, 0));
        }
    }
    sl_log_debug(LOG_TAG, "Attribute Store scan completed.");
}

// Check if attribute node needs get
//
static bool needs_get(attribute_store_node_t node)
{
    if (!attribute_resolver_has_get_rule(attribute_store_get_node_type(node))) {
        return false;
    }

    return !attribute_store_is_value_defined(node, REPORTED_ATTRIBUTE);
}

// Returns true if attribute needs a set operation
//
static bool needs_set(attribute_store_node_t node)
{
    if (!attribute_resolver_has_set_rule(attribute_store_get_node_type(node))) {
        return false;
    }

    return attribute_store_is_value_defined(node, DESIRED_ATTRIBUTE) && !attribute_store_is_value_matched(node);
}

/**
 * @brief Callback function registered to attribute store when node has been updated
 *
 * @param updated_node      The node that was updated
 * @param change            The change that occured.
 */
static void on_resolver_node_update(attribute_store_node_t updated_node, attribute_store_change_t change)
{
    // Defensive check: if attribute store is not initialized, ignore the callback
    // This can happen during shutdown when callbacks are still being invoked
    if (attribute_store_get_root() == ATTRIBUTE_STORE_INVALID_NODE) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);

    // If the node has been deleted make sure to clear it from our watch
    if ((change == ATTRIBUTE_DELETED) || (attribute_store_get_node_type(updated_node) == ATTRIBUTE_STORE_INVALID_ATTRIBUTE_TYPE)) {
        on_resolver_node_deleted(updated_node);
        return;
    }
    // If the node is blocked due to maximum retry count and it was just updated
    // try to see if resolution is needed or if it works now.
    if ((!needs_get(updated_node)) || (is_node_get_resolution_max_retries_reached(updated_node))) {
        pending_get_resolutions.erase(updated_node);
    }

    // If the node was updated and does not need a set anymore, we remove it from
    // the pending sets
    if (!needs_set(updated_node)) {
        pending_set_resolutions.erase(updated_node);
        set_retry_cooldown_until_.erase(updated_node);
    }

    // See if we need to scan something.
    // if somebody is waiting for a resolution notification, (possibly above the current node)
    // ensure that the node gets scanned again
    attribute_store_node_t parent_with_listener = get_highest_parent_with_resolution_listener(updated_node);
    if (parent_with_listener != ATTRIBUTE_STORE_INVALID_NODE) {
        scan_node(parent_with_listener);
    } else if (is_node_to_be_scanned(updated_node)) {
        scan_node(updated_node);
    }
}

static void on_resolver_node_deleted(attribute_store_node_t node)
{
    // Remove listeners and also from the pause resolution set.
    listeners.erase(node);
    resumption_listeners.erase(node);
    paused_nodes.erase(node);
    pending_get_resolutions.erase(node);
    pending_set_resolutions.erase(node);
    set_retry_cooldown_until_.erase(node);
    rearm_after_completion.erase(node);
    // Don't remove the node from the stack, the stack will
    // detect missing nodes.
    // Tell the Resolver Rule part to stop waiting for a callback for deleted nodes.
    attribute_resolver_rule_abort(node);

    // See if we need to scan something.
    // if somebody is waiting for a resolution notification, (above the current node)
    // ensure that the node gets scanned again
    scan_node(get_highest_parent_with_resolution_listener(node));
}

static bool is_node_under_resolution(attribute_store_node_t node)
{
    return pending_get_resolutions.contains(node) || pending_set_resolutions.contains(node);
}

/**
 * @brief Returns the effective retry count for a node's GET resolution.
 *
 * Group nodes created by start_group_resolution() store a per-node retry count
 * as their desired value. For all other nodes (where the desired value is
 * undefined or used for unrelated purposes), fall back to the global config.
 */
static uint8_t get_node_retry_count(attribute_store_node_t node)
{
    if (attribute_store_is_value_defined(node, DESIRED_ATTRIBUTE)) {
        number_t desired = attribute_store_get_desired_number(node);
        if (desired >= 1 && desired <= UINT8_MAX) {
            return static_cast<uint8_t>(desired);
        }
    }
    return attribute_resolver_config.get_retry_count;
}

static bool is_node_get_resolution_max_retries_reached(attribute_store_node_t node)
{
    const auto it = pending_get_resolutions.find(node);
    return it != pending_get_resolutions.end() && it->second.count >= get_node_retry_count(node);
}

static attribute_store_node_t get_highest_parent_with_resolution_listener(attribute_store_node_t node)
{
    attribute_store_node_t parent_with_listener = ATTRIBUTE_STORE_INVALID_NODE;
    while (node != ATTRIBUTE_STORE_INVALID_NODE) {
        if (listeners.contains(node)) {
            parent_with_listener = node;
        }
        node = attribute_store_get_node_parent(node);
    }
    return parent_with_listener;
}

static void set_common_parent_stack_index_zero(attribute_store_node_t node)
{
    for (attribute updated_node = node; updated_node.is_valid(); updated_node = updated_node.parent()) {
        for (uint16_t i = 0; i < stack.size(); i++) {
            if (stack[i].first == updated_node) {
                stack[i].second = 0;
                return;
            }
        }
    }
}

static void scan_node(attribute_store_node_t node_to_scan)
{
    if (node_to_scan == ATTRIBUTE_STORE_INVALID_NODE) {
        return;
    }
    // Adjust the in-progress scan so the node is revisited.
    if (!stack.empty()) {
        set_common_parent_stack_index_zero(node_to_scan);
    }

    if (!scan_requested) {
        scan_requested = true;
        zwave_component::attribute_resolver_handler::attribute_resolver_event_data ev_data;
        ev_data.event = zwave_component::attribute_resolver_handler::attribute_resolver_event_t::NEXT_EVENT;
        ev_data.data  = std::any {};
        zwave_component::attribute_resolver_handler::event_queue.push(ev_data);
    }
}

static void attribute_resume_expired_pending_get_nodes()
{
    // Defensive check: if attribute store is not initialized, ignore
    // This can happen during shutdown when the timer fires after teardown
    if (attribute_store_get_root() == ATTRIBUTE_STORE_INVALID_NODE) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);

    // Go through all the nodes pending a Get Command response
    for (auto it = pending_get_resolutions.begin(); it != pending_get_resolutions.end(); ++it) {
        attribute_store_node_t node = it->first;

        if (is_node_to_be_scanned(node)) {
            scan_node(node);
        }
    }

    // Ensure that the timer is running, if needed
    attribute_resolver_restart_pending_get_nodes_timer();
}

static void attribute_resolver_restart_pending_get_nodes_timer()
{
    // Always recompute the next deadline. Skipping when a timer is already
    // running can leave a closer GET retry waiting behind a farther one
    // (e.g. another node's long cooldown), delaying interview Gets.

    // Find out among all candidates, the node that is next to "expire"
    // and can be retried.
    clock_time_t next_deadline = 0;
    clock_time_t now           = clock_time();

    // Go through all the nodes pending a Get Command response
    for (auto it = pending_get_resolutions.begin(); it != pending_get_resolutions.end(); ++it) {
        pending_get_t pending_data  = it->second;
        attribute_store_node_t node = it->first;

        // Do not restart a timer if the node should not be retried.
        if (pending_data.count >= get_node_retry_count(node) || is_node_or_parent_paused(node) || !needs_get(node)) {
            continue;
        }

        if (pending_data.send_timeout != 0) {
            if (pending_data.send_timeout <= now) {
                // Deadline is past, just schedule a scan directly
                scan_node(node);
            } else if (pending_data.send_timeout < next_deadline || next_deadline == 0) {
                // Save the next deadline in the list
                next_deadline = pending_data.send_timeout;
            }
        }
    }

    // Include SET retry cooldowns in the next-deadline calculation.
    for (const auto &[n, deadline]: set_retry_cooldown_until_) {
        if (clock_time() >= deadline) {
            // Cooldown already expired: schedule a scan directly, same as
            // the GET timeout path above, rather than computing a past deadline
            // that would make the now < next_deadline guard false.
            scan_node(n);
        } else if (deadline < next_deadline || next_deadline == 0) {
            next_deadline = deadline;
        }
    }

    // Do we have a next deadline?
    if (next_deadline != 0 && now < next_deadline) {
        clock_time_t time_until_deadline = next_deadline - now;
        zwave_component::attribute_resolver_handler::attribute_resolver_event_data ev_data;
        ev_data.event = zwave_component::attribute_resolver_handler::attribute_resolver_event_t::TIMER_SET_EVENT;
        ev_data.data  = time_until_deadline;
        zwave_component::attribute_resolver_handler::event_queue.push(ev_data);
    }
}

static bool is_node_to_be_scanned(attribute_store_node_t node)
{
    // Verify if it is paused
    if (is_node_or_parent_paused(node)) {
        return false;
    }

    // If it needs a get. Get operations have priority
    if (needs_get(node)) {
        // Is it pending a get-response
        if (pending_get_resolutions.contains(node)) {
            if ((pending_get_resolutions[node].count < attribute_resolver_config.get_retry_count) && (clock_time() >= pending_get_resolutions[node].send_timeout) && (pending_get_resolutions[node].send_timeout != 0)) {
                return true;
            }
        } else {
            // if it needs a get and not pending a response, we need to scan
            return true;
        }
    } else if (needs_set(node) && !pending_set_resolutions.contains(node)) {
        auto it = set_retry_cooldown_until_.find(node);
        if (it != set_retry_cooldown_until_.end()) {
            if (clock_time() < it->second) {
                return false;  // still cooling down
            }
            set_retry_cooldown_until_.erase(it);  // expired — clean up
        }
        return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
// Attribute Resolver private functions
///////////////////////////////////////////////////////////////////////////////
void on_resolver_rule_execute_complete(attribute_store_node_t node, clock_time_t transmission_time)
{
    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
    (void)transmission_time;

    // Get rule is complete, check if we want to wait for a retry.
    // Use a fixed get_retry_timeout only — do not add transmission_time.
    // Airtime can be inflated (S2 send-data timer expiry, rule timeout using
    // MAX_RESOLUTION_TIME) and would otherwise delay the next Get by tens of
    // seconds after the frame already finished.
    if ((pending_get_resolutions.contains(node)) && needs_get(node)) {
        pending_get_resolutions[node].send_timeout = clock_time() + attribute_resolver_config.get_retry_timeout;

        // Ensure that the timer is running
        attribute_resolver_restart_pending_get_nodes_timer();

        // Did we just reach the last retry, verify if there is some parent waiting for it.
        if (pending_get_resolutions[node].count >= get_node_retry_count(node)) {
            sl_log_debug(LOG_TAG,
                         "Maximum amount of retries reached for Attribute ID "
                         "%d. Giving up.\n",
                         node);
            give_up_get_resolution_on_group(node);
            invoke_give_up_listeners(node);
            scan_node(get_highest_parent_with_resolution_listener(node));
        }
    }

    // SET retry pacing — evaluated in two steps so that re-arm can restore
    // desired before we decide whether a cooldown is warranted.
    //
    // Step 1: capture + clear the single-frame in-flight marker while we still
    // have it. Multi-frame (pending_set==true) is handled separately below.
    const bool single_frame_set_completed = (pending_set_resolutions.contains(node)) && (!pending_set_resolutions[node]);

    if (single_frame_set_completed) {
        pending_set_resolutions.erase(node);
    }

    // Is it a multi frame set rule?
    if ((pending_set_resolutions.contains(node)) && (pending_set_resolutions[node])) {
        // Multi-frame resolutions, just remove it from the pending sets and try again.
        pending_set_resolutions.erase(node);
        scan_node(node);
    }

    // Was a Set re-arm requested while the previous resolution was in flight?
    // Restoring `desired` re-creates the desired/reported mismatch the
    // completion callback wiped, so on_resolver_node_update will request a
    // scan and resolver_find_next_resolve will re-execute the Set rule using
    // the latest sub-attribute desired values.
    auto rearm_it = rearm_after_completion.find(node);
    if (rearm_it != rearm_after_completion.end()) {
        uint8_t retry_count = rearm_it->second;
        rearm_after_completion.erase(rearm_it);
        attribute(node).set_desired<uint8_t>(retry_count);
    }

    // Step 2: now that re-arm has synchronously restored desired (if any),
    // needs_set() reflects the true post-completion state. Set the cooldown
    // only if the node still needs a SET — this covers both the re-arm path
    // and any external attribute store update that arrived during the in-flight
    // window.
    if (single_frame_set_completed) {
        if (needs_set(node)) {
            static constexpr clock_time_t SET_RETRY_BACKOFF_MS = 500;
            set_retry_cooldown_until_[node]                    = clock_time() + SET_RETRY_BACKOFF_MS;
            attribute_resolver_restart_pending_get_nodes_timer();
        } else {
            set_retry_cooldown_until_.erase(node);
        }
    }

    // Resolution of this node is done (everything is resolved or we gave up trying)
    if (listeners.contains(node) && !attribute_resolver_node_or_child_needs_resolution(node)) {
        zwave_component::attribute_resolver_handler::attribute_resolver_event_data ev_data;
        ev_data.event = zwave_component::attribute_resolver_handler::attribute_resolver_event_t::WATCH_EVENT;
        ev_data.data  = node;
        zwave_component::attribute_resolver_handler::event_queue.push(ev_data);
    }

    // Process the next element in our scan
    if (!stack.empty()) {
        zwave_component::attribute_resolver_handler::attribute_resolver_event_data ev_data;
        ev_data.event = zwave_component::attribute_resolver_handler::attribute_resolver_event_t::NEXT_EVENT;
        ev_data.data  = std::any {};
        zwave_component::attribute_resolver_handler::event_queue.push(ev_data);
    }
}

///////////////////////////////////////////////////////////////////////////////
// C++ wrapper
///////////////////////////////////////////////////////////////////////////////
namespace attribute_resolver
{

    void create_attribute_store_callbacks(attribute_store_type_t node_type, const attribute_resolver_function &set_func, const attribute_resolver_function &get_func)
    {
        std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
        if (set_func != nullptr) {
            // Both Get and Set or only Set rule registered, we want to know about both DESIRED and REPORTED updates.
            attribute_store_register_callback_by_type(&on_resolver_node_update, node_type);
        } else if (get_func != nullptr) {
            // Just a Get Rule, we want to know about undefined Reported values only:
            attribute_store_register_callback_by_type_and_state(&on_resolver_node_update, node_type, REPORTED_ATTRIBUTE);
        }
        // Just verify the tree, in case some new rules can be applied.
        if (stack.empty() && (!scan_requested)) {
            scan_requested = true;
            zwave_component::attribute_resolver_handler::attribute_resolver_event_data ev_data;
            ev_data.event = zwave_component::attribute_resolver_handler::attribute_resolver_event_t::NEXT_EVENT;
            ev_data.data  = std::any {};
            zwave_component::attribute_resolver_handler::event_queue.push(ev_data);
        }
    }

    sl_status_t register_rules(attribute_store_type_t node_type, attribute_resolver::attribute_resolver_function set_func, attribute_resolver::attribute_resolver_function get_func)
    {
        register_rules_internal(node_type, set_func, get_func);
        create_attribute_store_callbacks(node_type, set_func, get_func);
        return SL_STATUS_OK;
    }

    sl_status_t register_multiple_types_rules(const std::set<attribute_store_type_t> &node_types, attribute_resolver_function set_func, attribute_resolver_function get_func)
    {
        register_group_rules_internal(node_types, set_func, get_func);

        for (auto node_type: node_types) {
            create_attribute_store_callbacks(node_type, set_func, get_func);
        }
        return SL_STATUS_OK;
    }

}  // namespace attribute_resolver

///////////////////////////////////////////////////////////////////////////////
// Attribute Resolver public functions
///////////////////////////////////////////////////////////////////////////////
sl_status_t attribute_resolver_register_rule(attribute_store_type_t node_type, attribute_resolver_function_t set_func, attribute_resolver_function_t get_func)
{
    return attribute_resolver::register_rules(node_type, set_func, get_func);
}

static sl_status_t attribute_resolver_config_init(attribute_resolver_config_t resolver_config)
{
    // Allow send_init to be NULL
    if (resolver_config.send == NULL) {
        return SL_STATUS_FAIL;
    }
    attribute_resolver_config = {.send_init = resolver_config.send_init, .send = resolver_config.send, .abort = resolver_config.abort, .get_retry_timeout = resolver_config.get_retry_timeout, .get_retry_count = resolver_config.get_retry_count};
    return SL_STATUS_OK;
}

sl_status_t attribute_resolver_init(attribute_resolver_config_t resolver_config)
{
    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
    // Set everything to 0
    listeners.clear();
    get_give_up_listeners.clear();
    paused_nodes.clear();
    pending_get_resolutions.clear();
    pending_set_resolutions.clear();
    set_retry_cooldown_until_.clear();
    rearm_after_completion.clear();
    stack.clear();
    timer_stop(&pending_get_resume_timer);
    scan_requested = true;

    attribute_resolver_rule_init(on_resolver_rule_execute_complete);
    if (attribute_resolver_config_init(resolver_config) == SL_STATUS_FAIL) {
        return SL_STATUS_FAIL;
    }
    if (attribute_resolver_config.send_init != NULL) {
        attribute_resolver_config.send_init();
    }

    // The thread is started via the Initializable interface in main.cpp,
    // so we just post the init event
    zwave_component::attribute_resolver_handler::attribute_resolver_event_data ev_data;
    ev_data.event = zwave_component::attribute_resolver_handler::attribute_resolver_event_t::NEXT_EVENT;
    ev_data.data  = std::any {};
    zwave_component::attribute_resolver_handler::event_queue.push(ev_data);

    return SL_STATUS_OK;
}

int attribute_resolver_teardown()
{
    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
    // Stop the timer to prevent it from firing after shutdown
    timer_stop(&pending_get_resume_timer);
    // Clear all pending resolutions to prevent any further processing
    pending_get_resolutions.clear();
    pending_set_resolutions.clear();
    set_retry_cooldown_until_.clear();
    rearm_after_completion.clear();
    stack.clear();
    // The thread is stopped in main.cpp cleanup
    return 0;
}

/************************ event handlers ****************************/

static void attribute_resolver_ev_next()
{
    // Defensive check: if attribute store is not initialized, ignore the event
    // This can happen during shutdown when events are still in the queue
    if (attribute_store_get_root() == ATTRIBUTE_STORE_INVALID_NODE) {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);

    if (stack.empty() && scan_requested) {
        sl_log_debug(LOG_TAG, "Starting scan from the top");
        scan_requested = false;
        stack.push_back(std::pair<attribute_store_node_t, int>(attribute_store_get_root(), 0));
    }
    // Find next node to resolve
    if (!attribute_resolver_rule_busy()) {
        resolver_find_next_resolve();
    } else {
        sl_log_debug(LOG_TAG, "Scan blocked: rule busy tid=%lu", sl_log_thread_id());
    }

    // A scan may have been requested (via scan_node) while we were busy or
    // while an earlier scan was in progress.  If the scan has now finished
    // but the request is still pending, schedule another pass so it is not
    // silently dropped.
    if (stack.empty() && scan_requested) {
        zwave_component::attribute_resolver_handler::attribute_resolver_event_data ev_data;
        ev_data.event = zwave_component::attribute_resolver_handler::attribute_resolver_event_t::NEXT_EVENT;
        ev_data.data  = std::any {};
        zwave_component::attribute_resolver_handler::event_queue.push(ev_data);
    }
}

static void attribute_resolver_ev_watch(const attribute_store_node_t node)
{
    std::lock_guard<std::recursive_mutex> lock(resolver_mutex);
    listeners(node, node);
}

static void pending_get_resume_timer_expired_event(void *ptr)
{
    // Defensive check: if attribute store is not initialized, ignore the timer
    // This can happen during shutdown when the timer fires after teardown
    if (attribute_store_get_root() == ATTRIBUTE_STORE_INVALID_NODE) {
        return;
    }
    attribute_resume_expired_pending_get_nodes();
}

namespace zwave_component
{
    attribute_resolver_handler::attribute_resolver_handler() : threading("Attribute Resolver Handler") {}

    void attribute_resolver_handler::run()
    {
        std::optional<attribute_resolver_event_data> ev = zwave_component::attribute_resolver_handler::event_queue.pop(10);
        if (ev.has_value()) {
            switch (ev.value().event) {
                case attribute_resolver_event_t::NEXT_EVENT: {
                    attribute_resolver_ev_next();
                } break;
                case attribute_resolver_event_t::WATCH_EVENT: {
                    attribute_store_node_t node = std::any_cast<attribute_store_node_t>(ev.value().data);
                    attribute_resolver_ev_watch(node);
                } break;
                case attribute_resolver_event_t::TIMER_SET_EVENT: {
                    clock_time_t time_until_deadline = std::any_cast<clock_time_t>(ev.value().data);
                    sl_log_debug(LOG_TAG,
                                 "Restarting timer for pending Get resolutions. Next "
                                 "attempt in %lu ms",
                                 time_until_deadline);
                    timer_set(&pending_get_resume_timer, time_until_deadline, pending_get_resume_timer_expired_event, 0);
                } break;

                default:
                    sl_log_warning(LOG_TAG, "Unhandled event %d", static_cast<int>(ev.value().event));
                    break;
            }
        }

        // Check if we should stop before attempting to read
        if (should_stop()) {
            return;
        }
    }

    attribute_resolver_handler::~attribute_resolver_handler() {}

    sl_status_t attribute_resolver_handler::initialize()
    {
        return SL_STATUS_OK;
    }

    int attribute_resolver_handler::shutdown()
    {
        stop();
        return 0;
    }

    std::string attribute_resolver_handler::name() const
    {
        return "Attribute Resolver Handler";
    }
}  // namespace zwave_component

///////////////////////////////////////////////////////////////////////////////
// Native threading implementation
///////////////////////////////////////////////////////////////////////////////
