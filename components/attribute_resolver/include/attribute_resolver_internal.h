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

/**
 * @file attribute_resolver_internal.h
 * @brief Private internal functions and definitions for the Attribute resolver.
 *
 * This is a list of private functions used by the Attribute Resolver.
 *
 * @{
 */

#ifndef ATTRIBUTE_RESOLVER_INTERNAL_H
#define ATTRIBUTE_RESOLVER_INTERNAL_H

#include "attribute_store.h"
#include "clock_platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback function to inform the attribute resolver that a rule execution
 *  is done for a node
 *
 * @param node                The node for which the rule is conpleted.
 * @param transmission_time   The time in ms that it took, to execute the rule.
 *                            set this to 0 if no data is available.
 */
void on_resolver_rule_execute_complete(attribute_store_node_t node, clock_time_t transmission_time);

/**
 * @brief Register an attribute type exempt from Supervision encapsulation.
 *
 * Internal only. Must be called from attribute_resolver_start_group_resolution()
 * while resolver_mutex is held, before the pending-set check, so registration
 * precedes both immediate and rearm-deferred resolution.
 *
 * Exemptions are per attribute type for the process lifetime (append-only).
 *
 * @param node_type  Attribute type to exempt from Supervision encapsulation.
 */
void attribute_resolver_register_no_supervision_type(attribute_store_type_t node_type);

/**
 * @brief Query whether an attribute type is exempt from Supervision encapsulation.
 *
 * @param node_type  Attribute type to query.
 * @returns true if this type is exempt from Supervision encapsulation, false otherwise.
 */
bool attribute_resolver_is_no_supervision_type(attribute_store_type_t node_type);

#ifdef __cplusplus
}
#endif

#endif  // ATTRIBUTE_RESOLVER_INTERNAL_H
/** @} end attribute_resolver_internal */
