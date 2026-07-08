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
#include "attribute_store.h"
#include "attribute_store_node.h"
#include "attribute_store_internal.h"
#include "attribute_store_callbacks.h"
#include "attribute_store_fixt.h"
#include "attribute_store_type_registration.h"
#include "attribute_store_type_registration_internal.h"
#include "attribute_store_configuration_internal.h"
#include "attribute_store_validation.h"
#include "attribute_store_process.h"

// Generic includes
#include <stdbool.h>
#include <map>
#include <mutex>
#include <set>
#include <queue>
#include <string.h>
#include <assert.h>

// Includes from other components
#include "log.h"
#include "sl_status.h"
#include "datastore.h"
#include "datastore_attributes.h"

/// Setup Log tag
constexpr char LOG_TAG[] = "attribute_store";

// Private attribute store variables
namespace
{
    // The base of our attribute store tree
    attribute_store_node *root_node = nullptr;

    // Map between ids and pointers for speedy identification
    // instead of using node->find_id
    std::map<attribute_store_node_t, attribute_store_node *> id_node_map;

    // List of attribute store nodes that we have added/modified, that are not
    // saved yet in the datastore.
    std::set<attribute_store_node_t> node_id_never_saved;
    std::set<attribute_store_node_t> node_id_pending_save;
    // List of attribute store nodes that we have deleted from the attribute store
    std::queue<attribute_store_node_t> node_id_pending_deletion;

    // Keep track of which ID to assign next in the datastore
    attribute_store_node_t last_assigned_id = ATTRIBUTE_STORE_INVALID_NODE;

    // Protects id_node_map, the tree structure (child_nodes, parent_node, values),
    // and bookkeeping sets from concurrent access.  Recursive because several
    // public functions are re-entrant (delete_node recurses, get_attribute_value
    // with DESIRED_OR_REPORTED calls itself, etc.).
    // IMPORTANT: this mutex must NEVER be held when invoking attribute store
    // callbacks, because callbacks may acquire resolver_mutex -- holding both
    // in the opposite order would deadlock with the resolver thread.
    std::recursive_mutex attribute_store_mutex;
}  // namespace

constexpr char DATASTORE_LAST_ASSIGNED_ID_KEY[] = "attribute_store_last_assigned_id";

///////////////////////////////////////////////////////////////////////////////
// Private helper functions
///////////////////////////////////////////////////////////////////////////////
/**
 * @brief Prints out that the attribute store is not initialized.
 */
static inline void log_attribute_store_not_initialized()
{
    sl_log_error(LOG_TAG, "Attribute Store is not initialized.");
}

/**
 * @brief Find the next available attribute_store_node_t ID
 *
 * Will modify the static last_assigned_id variable
 * @returns the next value that can be used for attribute_store_node_t ID
 */
static attribute_store_node_t attribute_store_get_next_id()
{
    // Increment at least once.
    // It will prevent to re-use an id pending deletion in the datastore.
    // (Add -> Delete -> Re-add before datastore delete is called)
    last_assigned_id += 1;

    while (last_assigned_id == ATTRIBUTE_STORE_INVALID_NODE || last_assigned_id == ATTRIBUTE_STORE_ROOT_ID || (id_node_map.contains(last_assigned_id))) {
        last_assigned_id++;
    }

    return last_assigned_id;
}

/**
 * @brief Decides if we push attribute modifications directly to the datastore
 * or add it in the pending queue
 *
 * Note: Do not use this function when an attribute gets deleted,
 * only on creation, modification or move.
 */
static void attribute_store_store_attribute(attribute_store_node *node)
{
    if (node == root_node) {
        STORE_ROOT_ATTRIBUTE(root_node);
    } else if (attribute_store_get_auto_save_cooldown_interval() == 0) {
        STORE_ATTRIBUTE(node);
        node_id_never_saved.erase(node->id);
    } else {
        node_id_pending_save.insert(node->id);
        attribute_store_process_restart_auto_save_cooldown_timer();
    }
}

/**
 * @brief Finds attribute_store_node corresponding to a attribute_store_node_t
 *
 * @param id The node to find and return its pointer
 *
 * @returns nullptr if the node is not in our attribute store tree
 * @returns attribute_store_node_t pointer if the node is in our attribute store tree
 */
static attribute_store_node *attribute_store_get_node_from_id(attribute_store_node_t id)
{
    // If the resevered ID was asked for, just return nullptr.
    if (id == ATTRIBUTE_STORE_INVALID_NODE) {
        return nullptr;
    }

    // Look into our ID map.
    std::map<attribute_store_node_t, attribute_store_node *>::iterator it;
    it = id_node_map.find(id);
    if (it != id_node_map.end()) {
        return it->second;
    }

    // Else if we get here, we did not find the ID using our map.
    // Give a go at traversing the tree, in case the map got out-of-sync
    // (can happen under concurrent access or re-entrancy during add_node).
    attribute_store_node *found_node = root_node->find_id(id);

    if (found_node != nullptr) {
        id_node_map[id] = found_node;
        sl_log_error(LOG_TAG,
                     "Attribute store Map got out-of-sync with the attribute store tree. "
                     "Please verify that the map gets correctly updated at any tree update!");
        attribute_store_log_node(id, false);
    }

    return found_node;
}

/**
 * @brief Compares if a node value is identical to the value/value size
 *
 * @param node        The node handle of the node for which the value
 *                    must be compared
 * @param value_state The value state of the node that must be compared.
 * @param value       A unint8_t pointer to a buffer with the value to compare
 *                    to the node's value.
 * @param value_size  The size of the data to compare in the value pointer.
 *
 * @returns true if the value stored in the node is identical
 * @returns false if the value stored in the node is different
 */
static bool attribute_store_is_value_identical(const attribute_store_node *node, attribute_store_node_value_state_t value_state, const uint8_t *value, uint8_t value_size)
{
    // Make a vector our of the received uint8_t array:
    std::vector<uint8_t> received_value(value, value + value_size);

    // Compare our received vector with the right value state
    if (value_state == REPORTED_ATTRIBUTE) {
        return node->reported_value == received_value;
    }
    if (value_state == DESIRED_ATTRIBUTE) {
        return node->desired_value == received_value;
    }

    // value state was invalid
    return false;
}

/**
 * @brief Loads the Attribute Store from the Datastore.
 *
 * This function will wipe the existing attribute store tree, and if invoked
 * while some pending saves/deletions were to be done in the datastore, the
 * pending changes will be lost.
 *
 * @returns SL_STATUS_OK   If the Attribute Store was loaded from the datastore.
 * @returns any other value if an error occurred.
 */
static sl_status_t attribute_store_load_all_nodes_from_datastore()
{
    // Clear the current attribute store, do not cache the deletion as something to push down to the datastore.
    attribute_store_delete_node(root_node->id);
    // Efficient way of clearing a std::queue
    std::queue<attribute_store_node_t> empty_queue;
    std::swap(node_id_pending_deletion, empty_queue);

    // Container to cache the node links. (parent-child)
    std::vector<std::pair<attribute_store_node_t, attribute_store_node_t>> node_links;
    // Run the query:
    datastore_attribute_t received_attribute = {};
    sl_status_t datastore_status             = datastore_fetch_all_attributes(&received_attribute);
    while (datastore_status == SL_STATUS_IN_PROGRESS) {
        attribute_store_node *node = nullptr;
        if (ATTRIBUTE_STORE_ROOT_ID == received_attribute.id) {
            node = root_node;
        } else {
            node = new attribute_store_node(nullptr, received_attribute.type, received_attribute.id);
        }

        // Save the parent -> child links until we created all the nodes.
        node_links.push_back({received_attribute.parent_id, received_attribute.id});

        // Push the data into the node
        node->reported_value.resize(received_attribute.reported_value_size);
        if (received_attribute.reported_value_size > 0) {
            node->reported_value.assign(received_attribute.reported_value, received_attribute.reported_value + received_attribute.reported_value_size);
        }
        node->desired_value.resize(received_attribute.desired_value_size);
        if (received_attribute.desired_value_size > 0) {
            node->desired_value.assign(received_attribute.desired_value, received_attribute.desired_value + received_attribute.desired_value_size);
        }

        // We just created a node, keep the local map updated
        id_node_map[node->id] = node;

        // Get the next attribute
        datastore_status = datastore_fetch_all_attributes(&received_attribute);
    }

    // Attach the links into the attributes
    for (const auto &[parent_node_id, child_node_id]: node_links) {
        attribute_store_node *parent_node = attribute_store_get_node_from_id(parent_node_id);
        attribute_store_node *child_node  = attribute_store_get_node_from_id(child_node_id);

        if (parent_node == nullptr) {
            // Root node has no parent, no biggie.
            continue;
        }
        if (child_node == nullptr) {
            sl_log_error(LOG_TAG,
                         "Child Node is a nullptr, something went wrong while "
                         "loading the attribute store");
        }

        parent_node->child_nodes.push_back(child_node);
        child_node->parent_node = parent_node;
    }

    // Invoke callbacks on our brand new tree
    attribute_store_refresh_node_and_children_callbacks(root_node->id);

    return datastore_status;
}

/**
 * @brief Takes a node, saves it and all its children in
 * the datastore from the "in-memory" attribute store.
 *
 * @param node  Pointer to the in memory object where to copy the data.
 *
 * @returns SL_STATUS_OK if the node and its subtree have been loaded
 * @returns SL_STATUS_FAIL if an error occurred.
 */
static sl_status_t attribute_store_save_node_to_datastore(attribute_store_node *node)
{
    if (node == nullptr) {
        sl_log_error(LOG_TAG, "Attempting to save data from a NULL pointer. Ignoring");
        return SL_STATUS_FAIL;
    }

    // Save the node first, then its children:
    sl_status_t save_status = SL_STATUS_OK;
    if (node->parent_node == nullptr) {
        // This is the root, no parent
        save_status = STORE_ROOT_ATTRIBUTE(node);
    } else if (node_id_pending_save.contains(node->id)) {
        save_status = STORE_ATTRIBUTE(node);
        node_id_pending_save.erase(node->id);
    }

    if (save_status != SL_STATUS_OK) {
        sl_log_error(LOG_TAG, "Error saving Attribute ID %d. Aborting", node->id);
        attribute_store_log_node(node->id, false);
        return save_status;
    }

    // Save all children next
    for (uint32_t i = 0; i < node->child_nodes.size(); i++) {
        save_status = attribute_store_save_node_to_datastore(node->child_nodes.at(i));
        if (save_status != SL_STATUS_OK) {
            sl_log_error(LOG_TAG, "Error saving children of Attribute ID %d. Aborting", node->id);
            attribute_store_log_node(node->child_nodes.at(i)->id, false);
            return save_status;
        }
    }
    return save_status;
}

/**
 * @brief Deletes all the nodes pending datastore deletion from the datastore
 *
 * @returns SL_STATUS_OK if all the nodes pending deletions have been removed
 * @returns SL_STATUS_FAIL if an error occurred.
 */
static sl_status_t attribute_store_delete_pending_deletions_from_datastore()
{
    sl_status_t aggregated_status = SL_STATUS_OK;
    while (!node_id_pending_deletion.empty()) {
        attribute_store_node_t node_id_to_delete = node_id_pending_deletion.front();
        sl_status_t deletion_status              = SL_STATUS_OK;
        if (!node_id_never_saved.contains(node_id_to_delete)) {
            deletion_status = datastore_delete_attribute(node_id_to_delete);
        }
        if (SL_STATUS_OK != deletion_status) {
            sl_log_error(LOG_TAG, "Could not delete ID %d from the datastore.", node_id_to_delete);
        }
        aggregated_status |= deletion_status;
        node_id_pending_deletion.pop();
    }

    return aggregated_status;
}

///////////////////////////////////////////////////////////////////////////////
// Init and teardown functions, declared in the attribute_store_fixt.h file
///////////////////////////////////////////////////////////////////////////////
sl_status_t attribute_store_init(void)
{
    // Announce an issue with the type storage at start if we detect it.
    if (sizeof(attribute_store_change_t) != 4) {
        sl_log_error(LOG_TAG,
                     "Enum type is not 32-bits. "
                     "Please adjust ENUM_STORAGE_TYPE to be the right type");
    }

    // Prepare our root node, if not ready
    if (root_node == nullptr) {
        root_node = new attribute_store_node(nullptr, ATTRIBUTE_TREE_ROOT, ATTRIBUTE_STORE_ROOT_ID);

        // Add the root as our first node in the id map:
        id_node_map.clear();
        id_node_map[root_node->id] = root_node;

        // Load the root data and all its children from the datastore, in case it's there.
        // Do not reload from SQLite if root_node was already allocated.
        sl_log_info(LOG_TAG, "Loading Attribute Store data from the datastore.");
        if (attribute_store_load_all_nodes_from_datastore() != SL_STATUS_OK) {
            sl_log_info(LOG_TAG,
                        "Attribute Store data could not be loaded from the datastore. "
                        "Starting with an empty Attribute Store.");
        }

        // Print how much was loaded from the datastore.
        sl_log_info(LOG_TAG, "Attribute Store size: %d Attributes.", attribute_store_get_node_total_child_count(root_node->id));

        // Reload our last assigned ID. Start from 0 by default.
        last_assigned_id = 0;
        int64_t last_assigned_id_value;
        if (SL_STATUS_OK == datastore_fetch_int(DATASTORE_LAST_ASSIGNED_ID_KEY, &last_assigned_id_value)) {
            last_assigned_id = static_cast<attribute_store_node_t>(last_assigned_id_value);
        }
    }

    if (nullptr != root_node) {
        // Save the root in the datastore:
        STORE_ROOT_ATTRIBUTE(root_node);
        return SL_STATUS_OK;
    }

    sl_log_critical(LOG_TAG, "Could not initialize the Attribute Store.");
    return SL_STATUS_FAIL;
}

int attribute_store_teardown(void)
{
    std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
    sl_log_debug(LOG_TAG, "Teardown of the attribute store");
    // Remove all registered callbacks
    attribute_store_callbacks_teardown();
    // Erase our record of registered types
    attribute_store_reset_registered_attribute_types();

    // make a last save in the datastore
    attribute_store_save_to_datastore();

    // Clear up our map
    id_node_map.clear();

    // Delete the root node, which will delete everything.
    if (root_node != nullptr) {
        delete root_node;
        // Reset to NULL, so we can detect un-initialized attribute store
        root_node = nullptr;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Internal functions, declared in attribute_store_internals.h
///////////////////////////////////////////////////////////////////////////////
sl_status_t attribute_store_save_to_datastore()
{
    std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
    if (root_node != nullptr) {
        sl_status_t res = SL_STATUS_OK;
        if ((!node_id_pending_save.empty()) || (!node_id_pending_deletion.empty())) {
            datastore_start_transaction();
            sl_log_debug(LOG_TAG, "Saving %d attributes to the datastore. ", node_id_pending_save.size());
            res |= attribute_store_save_node_to_datastore(root_node);
            sl_log_debug(LOG_TAG, "Deleting %d attributes from the datastore. ", node_id_pending_deletion.size());
            res |= attribute_store_delete_pending_deletions_from_datastore();

            // Save the last assigned ID:
            datastore_store_int(DATASTORE_LAST_ASSIGNED_ID_KEY, static_cast<int64_t>(last_assigned_id));

            datastore_commit_transaction();
        }
        // Mark that we have no node never saved.
        node_id_never_saved.clear();
        // Tell the process we made a fresh back-up.
        attribute_store_process_on_attribute_store_saved();
        return res;
    }
    log_attribute_store_not_initialized();
    return SL_STATUS_FAIL;
}

sl_status_t attribute_store_load_from_datastore()
{
    if (root_node != nullptr) {
        sl_log_info(LOG_TAG, "Loading Attribute Store data from the datastore.");
        return attribute_store_load_all_nodes_from_datastore();
    }
    log_attribute_store_not_initialized();
    return SL_STATUS_FAIL;
}

///////////////////////////////////////////////////////////////////////////////
// Public interface functions, declared in attribute_store.h
///////////////////////////////////////////////////////////////////////////////
attribute_store_node_t attribute_store_get_root()
{
    std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
    if (root_node == nullptr) {
        log_attribute_store_not_initialized();
        return ATTRIBUTE_STORE_INVALID_NODE;
    }
    return root_node->id;
}

attribute_store_node_t attribute_store_add_node(attribute_store_type_t type, attribute_store_node_t parent_node)
{
    attribute_store_node_t new_id = ATTRIBUTE_STORE_INVALID_NODE;
    {
        std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
        if (root_node == nullptr) {
            log_attribute_store_not_initialized();
            return ATTRIBUTE_STORE_INVALID_NODE;
        }

        if (type == ATTRIBUTE_STORE_INVALID_ATTRIBUTE_TYPE) {
            sl_log_warning(LOG_TAG,
                           "Attempt to create an attribute with the Invalid node type. "
                           "Ignoring.");
            return ATTRIBUTE_STORE_INVALID_NODE;
        }

        attribute_store_node *parent = attribute_store_get_node_from_id(parent_node);

        if (parent == nullptr) {
            return ATTRIBUTE_STORE_INVALID_NODE;
        }

        if (parent->undergoing_deletion) {
            sl_log_warning(LOG_TAG,
                           "Attempt to add a child node for Attribute Node ID %d "
                           "while it is undergoing deletion. Ignoring.",
                           parent->id);
            return ATTRIBUTE_STORE_INVALID_NODE;
        }

        if (!is_node_addition_valid(type, parent->type)) {
            return ATTRIBUTE_STORE_INVALID_NODE;
        }

        attribute_store_node *new_node = nullptr;
        new_node                       = parent->add_child(type, attribute_store_get_next_id());

        if (new_node != nullptr) {
            id_node_map[new_node->id] = new_node;
            node_id_never_saved.insert(new_node->id);
            attribute_store_store_attribute(new_node);
            new_id = new_node->id;
        }
    }

    if (new_id != ATTRIBUTE_STORE_INVALID_NODE) {
        attribute_store_invoke_callbacks(new_id, type, REPORTED_ATTRIBUTE, ATTRIBUTE_CREATED);
    }

    return new_id;
}

sl_status_t attribute_store_delete_node(attribute_store_node_t id)
{
    attribute_store_node *node_to_delete = nullptr;
    attribute_store_node_t node_id;
    attribute_store_type_t node_type;
    bool is_root = false;

    {
        std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
        if (root_node == nullptr) {
            log_attribute_store_not_initialized();
            return SL_STATUS_NOT_INITIALIZED;
        }
        if (id == ATTRIBUTE_STORE_INVALID_NODE) {
            return SL_STATUS_OK;
        }

        node_to_delete = attribute_store_get_node_from_id(id);
        if (node_to_delete == nullptr) {
            sl_log_debug(LOG_TAG, "Attempt to delete non-existing Attribute ID %d. Ignoring", id);
            return SL_STATUS_OK;
        }

        if (node_to_delete->undergoing_deletion) {
            return SL_STATUS_IN_PROGRESS;
        }

        is_root   = (node_to_delete == root_node);
        node_id   = node_to_delete->id;
        node_type = node_to_delete->type;

        if (!is_root) {
            node_to_delete->undergoing_deletion = true;
        }
    }

    // Delete children -- each recursive call handles its own locking and
    // callbacks, so we must NOT hold attribute_store_mutex here.
    sl_status_t deletion_status = SL_STATUS_OK;
    while (true) {
        attribute_store_node_t child_id = ATTRIBUTE_STORE_INVALID_NODE;
        {
            std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
            if (!node_to_delete->child_nodes.empty()) {
                child_id = node_to_delete->child_nodes.at(0)->id;
            }
        }
        if (child_id == ATTRIBUTE_STORE_INVALID_NODE) {
            break;
        }
        deletion_status = attribute_store_delete_node(child_id);
        if (deletion_status != SL_STATUS_OK) {
            sl_log_error(LOG_TAG, "Deletion error happened for a child of Attribute ID %d", node_id);
            return deletion_status;
        }
    }

    if (is_root) {
        std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
        root_node->reported_value.resize(0);
        root_node->desired_value.resize(0);
        STORE_ROOT_ATTRIBUTE(root_node);
    } else {
        {
            std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
            node_id_pending_save.erase(node_id);
            if (node_id_never_saved.contains(node_id)) {
                node_id_never_saved.erase(node_id);
            } else {
                node_id_pending_deletion.push(node_id);
            }
            if (attribute_store_get_auto_save_cooldown_interval() == 0) {
                attribute_store_delete_pending_deletions_from_datastore();
            } else {
                attribute_store_process_restart_auto_save_cooldown_timer();
            }
        }

        // Fire the callback without holding the lock so callbacks can safely
        // acquire resolver_mutex (avoids ABBA deadlock).
        // The node is still in the map so callbacks can read its value.
        attribute_store_invoke_callbacks(node_id, node_type, REPORTED_ATTRIBUTE, ATTRIBUTE_DELETED);

        {
            std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
            id_node_map.erase(node_id);
            delete node_to_delete;
        }
    }

    return deletion_status;
}

attribute_store_node_t attribute_store_get_node_parent(attribute_store_node_t id)
{
    std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
    if (root_node == nullptr) {
        log_attribute_store_not_initialized();
        return ATTRIBUTE_STORE_INVALID_NODE;
    }

    const attribute_store_node *current_node = attribute_store_get_node_from_id(id);

    if (current_node == nullptr) {
        return ATTRIBUTE_STORE_INVALID_NODE;
    }

    if (current_node->parent_node == nullptr) {  // The root has no parent.
        return ATTRIBUTE_STORE_INVALID_NODE;
    }

    return current_node->parent_node->id;
}

attribute_store_node_t attribute_store_get_first_parent_with_type(attribute_store_node_t id, attribute_store_type_t parent_type)
{
    std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
    if (root_node == nullptr) {
        log_attribute_store_not_initialized();
        return ATTRIBUTE_STORE_INVALID_NODE;
    }

    const attribute_store_node *current_node = attribute_store_get_node_from_id(id);

    if (current_node == nullptr) {
        return ATTRIBUTE_STORE_INVALID_NODE;
    }

    do {
        current_node = current_node->parent_node;
    } while ((current_node != nullptr) && (current_node->type != parent_type));

    if (current_node == nullptr) {
        return ATTRIBUTE_STORE_INVALID_NODE;
    }

    return current_node->id;
}

sl_status_t attribute_store_set_node_attribute_value(attribute_store_node_t id, attribute_store_node_value_state_t value_state, const uint8_t *value, uint8_t value_size)
{
    attribute_store_node_t cb_id   = ATTRIBUTE_STORE_INVALID_NODE;
    attribute_store_type_t cb_type = ATTRIBUTE_STORE_INVALID_ATTRIBUTE_TYPE;
    bool touch_only                = false;

    {
        std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
        if (root_node == nullptr) {
            log_attribute_store_not_initialized();
            return SL_STATUS_NOT_INITIALIZED;
        }

        if (id == ATTRIBUTE_STORE_INVALID_NODE) {
            return SL_STATUS_FAIL;
        }

        attribute_store_node *node_to_modify = attribute_store_get_node_from_id(id);

        if (node_to_modify == nullptr) {
            sl_log_warning(LOG_TAG,
                           "Attempt to set the value of a non-existing Attribute Store "
                           "Node (%d)",
                           id);
            return SL_STATUS_FAIL;
        }

        if (node_to_modify->undergoing_deletion) {
            sl_log_warning(LOG_TAG,
                           "Attempt to modify the value of Attribute Node ID %d "
                           "while it is undergoing deletion. Ignoring.",
                           node_to_modify->id);
            return SL_STATUS_FAIL;
        }

        if (attribute_store_is_value_identical(node_to_modify, value_state, value, value_size)) {
            cb_id      = node_to_modify->id;
            touch_only = true;
        } else if (!is_write_operation_valid(node_to_modify->type, value, value_size)) {
            sl_log_error(LOG_TAG,
                         "Attempted an invalid attribute write "
                         "for Attribute ID %d. Ignoring.",
                         id);
            attribute_store_log_attribute_type_information(node_to_modify->type);
            return SL_STATUS_FAIL;
        } else if (value_state == DESIRED_ATTRIBUTE) {
            node_to_modify->desired_value.resize(value_size);
            if (value_size > 0) {
                node_to_modify->desired_value.assign(value, value + value_size);
            }
            cb_id   = node_to_modify->id;
            cb_type = node_to_modify->type;
            attribute_store_store_attribute(node_to_modify);
        } else if (value_state == REPORTED_ATTRIBUTE) {
            node_to_modify->reported_value.resize(value_size);
            if (value_size > 0) {
                node_to_modify->reported_value.assign(value, value + value_size);
            }
            cb_id   = node_to_modify->id;
            cb_type = node_to_modify->type;
            attribute_store_store_attribute(node_to_modify);
        } else {
            sl_log_warning(LOG_TAG, "value_state was neither reported or desired: %u\n", value_state);
            return SL_STATUS_FAIL;
        }
    }

    if (touch_only) {
        attribute_store_invoke_touch_callbacks(cb_id);
    } else if (cb_id != ATTRIBUTE_STORE_INVALID_NODE) {
        attribute_store_invoke_callbacks(cb_id, cb_type, value_state, ATTRIBUTE_UPDATED);
    }

    return SL_STATUS_OK;
}

sl_status_t attribute_store_get_node_attribute_value(attribute_store_node_t id, attribute_store_node_value_state_t value_state, uint8_t *value, uint8_t *value_size)
{
    std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
    if (root_node == nullptr) {
        log_attribute_store_not_initialized();
        *value_size = 0;
        return SL_STATUS_NOT_INITIALIZED;
    }

    if (id == ATTRIBUTE_STORE_INVALID_NODE) {
        *value_size = 0;
        return SL_STATUS_FAIL;
    }

    attribute_store_node *node_to_read = attribute_store_get_node_from_id(id);

    if (node_to_read == nullptr) {
        sl_log_warning(LOG_TAG,
                       "Attempt to read values from a non-existing Attribute Store "
                       "Node (%d)",
                       id);
        *value_size = 0;
        return SL_STATUS_FAIL;
    }

    // Retrieve the value
    if (value_state == DESIRED_OR_REPORTED_ATTRIBUTE) {
        value_state = !node_to_read->desired_value.empty() ? DESIRED_ATTRIBUTE : REPORTED_ATTRIBUTE;
        return attribute_store_get_node_attribute_value(id, value_state, value, value_size);
    }
    if (value_state == DESIRED_ATTRIBUTE) {
        *value_size = node_to_read->desired_value.size();
        memcpy(value, node_to_read->desired_value.data(), *value_size);
        return SL_STATUS_OK;
    }
    if (value_state == REPORTED_ATTRIBUTE) {
        *value_size = node_to_read->reported_value.size();
        memcpy(value, node_to_read->reported_value.data(), *value_size);
        return SL_STATUS_OK;
    }

    sl_log_warning(LOG_TAG,
                   "Value state was neither REPORTED_ATTRIBUTE or "
                   "DESIRED_ATTRIBUTE: %u\n",
                   value_state);
    *value_size = 0;
    return SL_STATUS_FAIL;
}

attribute_store_type_t attribute_store_get_node_type(attribute_store_node_t id)
{
    std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
    if (root_node == nullptr) {
        log_attribute_store_not_initialized();
        return ATTRIBUTE_STORE_INVALID_ATTRIBUTE_TYPE;
    }

    attribute_store_node *node_to_read = attribute_store_get_node_from_id(id);
    if (node_to_read == nullptr) {
        return ATTRIBUTE_STORE_INVALID_ATTRIBUTE_TYPE;
    }

    return node_to_read->type;
}

attribute_store_node_t attribute_store_get_node_child(attribute_store_node_t id, uint32_t child_index)
{
    std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
    if (root_node == nullptr) {
        log_attribute_store_not_initialized();
        return ATTRIBUTE_STORE_INVALID_NODE;
    }

    attribute_store_node *node_to_read = attribute_store_get_node_from_id(id);

    if (node_to_read == nullptr) {
        return ATTRIBUTE_STORE_INVALID_NODE;
    }

    if (node_to_read->child_nodes.size() > child_index) {
        attribute_store_node *child_node = node_to_read->child_nodes.at(child_index);
        // Ensure the child node is in the map - this can happen if nodes are added
        // directly to the tree structure (e.g., during datastore loading or callbacks)
        if (!id_node_map.contains(child_node->id)) {
            id_node_map[child_node->id] = child_node;
        }
        return child_node->id;
    }
    return ATTRIBUTE_STORE_INVALID_NODE;
}

size_t attribute_store_get_node_child_count(attribute_store_node_t id)
{
    std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
    if (root_node == nullptr) {
        log_attribute_store_not_initialized();
        return 0;
    }

    if (id == ATTRIBUTE_STORE_INVALID_NODE) {
        return 0;
    }

    attribute_store_node *node_to_read = attribute_store_get_node_from_id(id);

    if (node_to_read == nullptr) {
        return 0;
    }

    return node_to_read->child_nodes.size();
}

size_t attribute_store_get_node_child_count_by_type(attribute_store_node_t id, attribute_store_type_t child_type)
{
    std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
    if (root_node == nullptr) {
        log_attribute_store_not_initialized();
        return 0;
    }

    if (id == ATTRIBUTE_STORE_INVALID_NODE) {
        return 0;
    }
    attribute_store_node *parent_node = attribute_store_get_node_from_id(id);

    if (parent_node == nullptr) {
        return 0;
    }

    size_t child_count = 0;
    for (size_t i = 0; i < parent_node->child_nodes.size(); i++) {
        if (parent_node->child_nodes[i]->type == child_type) {
            child_count += 1;
        }
    }
    return child_count;
}

size_t attribute_store_get_node_total_child_count(attribute_store_node_t node)
{
    std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
    size_t total = 0;
    if (root_node == nullptr) {
        log_attribute_store_not_initialized();
        return 0;
    }

    if (node == ATTRIBUTE_STORE_INVALID_NODE) {
        return 0;
    }

    attribute_store_node *node_to_read = attribute_store_get_node_from_id(node);

    if (node_to_read == nullptr) {
        return 0;
    }

    total += node_to_read->child_nodes.size();
    for (uint32_t i = 0; i < node_to_read->child_nodes.size(); i++) {
        total += attribute_store_get_node_total_child_count(node_to_read->child_nodes.at(i)->id);
    }

    return total;
}

bool attribute_store_node_exists(attribute_store_node_t id)
{
    std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
    if (root_node == nullptr) {
        log_attribute_store_not_initialized();
        return false;
    }

    if (id == ATTRIBUTE_STORE_INVALID_NODE) {
        return false;
    }
    const attribute_store_node *node_to_find = attribute_store_get_node_from_id(id);

    if (node_to_find == nullptr) {
        return false;
    }

    if (node_to_find->undergoing_deletion) {
        return false;
    }

    return true;
}

bool attribute_store_is_node_a_child(attribute_store_node_t id, attribute_store_node_t parent_id)
{
    std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
    if (root_node == nullptr) {
        log_attribute_store_not_initialized();
        return false;
    }

    const attribute_store_node *node = attribute_store_get_node_from_id(id);

    if (node == nullptr) {
        return false;
    }

    while (node->parent_node != nullptr) {
        if (node->parent_node->id == parent_id) {
            return true;
        }
        node = node->parent_node;
    }

    return false;
}

uint8_t attribute_store_get_node_value_size(attribute_store_node_t id, attribute_store_node_value_state_t value_state)
{
    std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
    if (root_node == nullptr) {
        log_attribute_store_not_initialized();
        return 0;
    }

    const attribute_store_node *node = attribute_store_get_node_from_id(id);

    if (node == nullptr) {
        return 0;
    }

    if (value_state == REPORTED_ATTRIBUTE) {
        return static_cast<uint8_t>(node->reported_value.size());
    }
    if (value_state == DESIRED_ATTRIBUTE) {
        return static_cast<uint8_t>(node->desired_value.size());
    }

    return 0;
}

attribute_store_node_t attribute_store_get_node_child_by_type(attribute_store_node_t id, attribute_store_type_t child_type, uint32_t child_index)
{
    std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
    if (root_node == nullptr) {
        log_attribute_store_not_initialized();
        return ATTRIBUTE_STORE_INVALID_NODE;
    }

    attribute_store_node *node_to_read = attribute_store_get_node_from_id(id);

    if (node_to_read == nullptr) {
        return ATTRIBUTE_STORE_INVALID_NODE;
    }

    uint32_t child_counter = 0;
    for (uint32_t i = 0; i < node_to_read->child_nodes.size(); i++) {
        if (node_to_read->child_nodes.at(i)->type == child_type) {
            if (child_counter == child_index) {
                return node_to_read->child_nodes.at(i)->id;
            }
            child_counter++;
        }
    }
    return ATTRIBUTE_STORE_INVALID_NODE;
}

attribute_store_node_t attribute_store_get_node_child_by_value(attribute_store_node_t id, attribute_store_type_t child_type, attribute_store_node_value_state_t value_state, const uint8_t *value, uint8_t value_size, uint32_t child_index)
{
    std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
    if (root_node == nullptr) {
        log_attribute_store_not_initialized();
        return ATTRIBUTE_STORE_INVALID_NODE;
    }

    attribute_store_node *node_to_read = attribute_store_get_node_from_id(id);

    if (node_to_read == nullptr) {
        return ATTRIBUTE_STORE_INVALID_NODE;
    }

    uint32_t child_counter = 0;
    for (uint32_t i = 0; i < node_to_read->child_nodes.size(); i++) {
        if (node_to_read->child_nodes.at(i)->type == child_type) {
            // Attribute ID is matching, compare the value:
            if (value_state == DESIRED_ATTRIBUTE) {
                if (node_to_read->child_nodes.at(i)->desired_value.size() == value_size) {
                    if (0 == memcmp(value, node_to_read->child_nodes.at(i)->desired_value.data(), value_size)) {
                        if (child_counter == child_index) {
                            return node_to_read->child_nodes.at(i)->id;
                        }
                        child_counter++;
                    }
                }
            } else if (value_state == REPORTED_ATTRIBUTE) {
                if (node_to_read->child_nodes.at(i)->reported_value.size() == value_size) {
                    if (0 == memcmp(value, node_to_read->child_nodes.at(i)->reported_value.data(), value_size)) {
                        if (child_counter == child_index) {
                            return node_to_read->child_nodes.at(i)->id;
                        }
                        child_counter++;
                    }
                }
            }
        }
    }
    return ATTRIBUTE_STORE_INVALID_NODE;
}

sl_status_t attribute_store_refresh_node_and_children_callbacks(attribute_store_node_t id)
{
    attribute_store_type_t node_type;
    {
        std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
        if (root_node == nullptr) {
            log_attribute_store_not_initialized();
            return SL_STATUS_FAIL;
        }
        node_type = attribute_store_get_node_type(id);
    }

    attribute_store_invoke_callbacks(id, node_type, REPORTED_ATTRIBUTE, ATTRIBUTE_CREATED);
    attribute_store_invoke_callbacks(id, node_type, REPORTED_ATTRIBUTE, ATTRIBUTE_UPDATED);

    uint32_t i = 0;
    while (i < attribute_store_get_node_child_count(id)) {
        attribute_store_refresh_node_and_children_callbacks(attribute_store_get_node_child(id, i));
        i += 1;
    }

    return SL_STATUS_OK;
}

void attribute_store_log()
{
    std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
    if (root_node != nullptr) {
        root_node->log_children(0);
    } else {
        sl_log_debug(LOG_TAG, "The attribute store is empty\n");
    }
}

void attribute_store_log_node(attribute_store_node_t id, bool log_children)
{
    std::lock_guard<std::recursive_mutex> lock(attribute_store_mutex);
    if (root_node == nullptr) {
        log_attribute_store_not_initialized();
        return;
    }

    attribute_store_node *node_to_read = attribute_store_get_node_from_id(id);
    if (node_to_read != nullptr) {
        if (log_children) {
            node_to_read->log_children(0);
        } else {
            node_to_read->log();
        }
    }
}
