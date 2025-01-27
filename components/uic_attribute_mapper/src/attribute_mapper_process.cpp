/******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/
#include "attribute_mapper.h"
#include "attribute_mapper_process.h"
#include "attribute_mapper_engine.hpp"

// Generic includes
#include <set>
#include <stdbool.h>

// Unify components
#include "attribute.hpp"

// Includes from other components
#include "sl_log.h"

// Constants
static constexpr char LOG_TAG[] = "unify_attribute_mapper_process";

using attribute_update_t = struct attribute_update {
  attribute_store_node_t node;
  attribute_store_node_value_state_t value_type;
};

struct attribute_update_compare {
  bool operator()(const attribute_update_t &lhs,
                  const attribute_update_t &rhs) const
  {
    if (lhs.node > rhs.node) {
      return true;
    } else if (lhs.node < rhs.node) {
      return false;
    } else {
      return lhs.value_type > rhs.value_type;
    }
  }
};

// Private variables
namespace
{
std::set<attribute_update_t, attribute_update_compare> pending_updates;
std::set<attribute_store_node_t> paused_reactions;
bool mapper_paused = false;
}  // namespace

////////////////////////////////////////////////////////////////////////////////
// Helper functions
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Removes all updates from the pending updates that have the same attribute
 * store node as passed in argument
 *
 * @param node    Attribute Store node to remove from the updates
 */
static void
  remove_node_updates_from_pending_udpates(attribute_store_node_t node)
{
  for (auto it = pending_updates.begin(); it != pending_updates.end();) {
    if (it->node == node) {
      it = pending_updates.erase(it);
    } else {
      ++it;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Attribute Store callback functions
////////////////////////////////////////////////////////////////////////////////
void on_reported_attribute_update(attribute_store_node_t updated_node,
                                  attribute_store_change_t change)
{
  if (mapper_paused == true) {
    sl_log_debug(LOG_TAG,
                 "Ignoring Attribute ID %d event as the mapper is paused.",
                 updated_node);
    return;
  }
  if (paused_reactions.count(updated_node) != 0) {
    if (change == ATTRIBUTE_DELETED) {
      paused_reactions.erase(updated_node);
    }
    sl_log_debug(LOG_TAG,
                 "Ignoring update to Attribute ID %d as the mapper"
                 " was instructed to ignore it.",
                 updated_node);
    return;
  }

  if (change == ATTRIBUTE_CREATED) {
    // Evaluate immediately if the attribute got created
    MapperEngine::get_instance().on_attribute_updated(updated_node,
                                                      REPORTED_ATTRIBUTE,
                                                      change);
  } else if (change == ATTRIBUTE_DELETED) {
    // Evaluate immediately if the attribute got deleted, if we wait we cannot
    // read its context again.
    MapperEngine::get_instance().on_attribute_updated(updated_node,
                                                      REPORTED_ATTRIBUTE,
                                                      change);
    // Make sure that we did not have a pending attribute
    // UPDATED in the update queue for the same attribute
    remove_node_updates_from_pending_udpates(updated_node);
  } else {
    if (pending_updates.empty()) {
      process_post(&unify_attribute_mapper_process,
                   MAPPER_EVALUATE_PENDING_UPDATES_EVENT,
                   nullptr);
    }
    attribute_update_t update = {};
    update.node               = updated_node;
    update.value_type         = REPORTED_ATTRIBUTE;
    pending_updates.insert(update);
  }
}

void on_desired_attribute_update(attribute_store_node_t updated_node,
                                 attribute_store_change_t)
{
  if (mapper_paused == true) {
    sl_log_debug(LOG_TAG,
                 "Ignoring Attribute ID %d event as the mapper is paused.",
                 updated_node);
    return;
  }
  if (paused_reactions.count(updated_node) != 0) {
    sl_log_debug(LOG_TAG,
                 "Ignoring update to Attribute ID %d as the mapper"
                 " was instructed to ignore it.",
                 updated_node);
    return;
  }

  if (pending_updates.empty()) {
    process_post(&unify_attribute_mapper_process,
                 MAPPER_EVALUATE_PENDING_UPDATES_EVENT,
                 nullptr);
  }
  attribute_update_t update = {};
  update.node               = updated_node;
  update.value_type         = DESIRED_ATTRIBUTE;
  pending_updates.insert(update);
}

////////////////////////////////////////////////////////////////////////////////
// Event functions
////////////////////////////////////////////////////////////////////////////////
void attribute_mapper_engine_evaluate_next_update()
{
  sl_log_debug(LOG_TAG,
               "%d pending attribute updates to evaluate.",
               pending_updates.size());

  if (pending_updates.empty()) {
    return;
  }
  auto update = pending_updates.begin();

  if (attribute_store_node_exists(update->node)) {
    sl_log_debug(LOG_TAG,
                 "Processing %s update to Attribute ID %d",
                 update->value_type == REPORTED_ATTRIBUTE ? "Reported"
                                                          : "Desired",
                 update->node);
    MapperEngine::get_instance().on_attribute_updated(update->node,
                                                      update->value_type,
                                                      ATTRIBUTE_UPDATED);
  }
  pending_updates.erase(update);

  if (!pending_updates.empty()) {
    process_post(&unify_attribute_mapper_process,
                 MAPPER_EVALUATE_PENDING_UPDATES_EVENT,
                 nullptr);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Functions shared within the mapper
////////////////////////////////////////////////////////////////////////////////
bool attribute_mapper_is_attribute_reactions_paused(attribute_store_node_t node)
{
  return paused_reactions.count(node);
}
////////////////////////////////////////////////////////////////////////////////
// Public functions
////////////////////////////////////////////////////////////////////////////////
void attribute_mapper_pause_reactions_to_attribute_updates(
  attribute_store_node_t node)
{
  paused_reactions.insert(node);
}

void attribute_mapper_resume_reactions_to_attribute_updates(
  attribute_store_node_t node)
{
  paused_reactions.erase(node);
}

bool attribute_mapper_has_pending_evaluations()
{
  return !pending_updates.empty();
}

void attribute_mapper_pause_mapping()
{
  mapper_paused = true;
}

void attribute_mapper_resume_mapping()
{
  mapper_paused = false;
}

////////////////////////////////////////////////////////////////////////////////
// Contiki Process
////////////////////////////////////////////////////////////////////////////////
PROCESS(unify_attribute_mapper_process, "unify_attribute_mapper_process");

PROCESS_THREAD(unify_attribute_mapper_process, ev, data)
{
  PROCESS_BEGIN();

  while (true) {
    if (ev == PROCESS_EVENT_INIT) {
      sl_log_info(LOG_TAG, "Process started.\n");
      mapper_paused = false;
    } else if (ev == PROCESS_EVENT_EXIT) {
      sl_log_info(LOG_TAG, "Process exited.\n");
      pending_updates.clear();
      paused_reactions.clear();

    } else if (ev == PROCESS_EVENT_EXITED) {
      // Do not do anything with this event, just wait to go down.

    } else if (ev == MAPPER_EVALUATE_PENDING_UPDATES_EVENT) {
      attribute_mapper_engine_evaluate_next_update();

    } else {
      sl_log_warning(LOG_TAG,
                     "Unknown Unify Mapper event: %d, data=%p\n",
                     ev,
                     data);
    }

    PROCESS_WAIT_EVENT();
  }
  PROCESS_END()
}
