/******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 ******************************************************************************
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 *****************************************************************************/

#ifndef ATTRIBUTE_RESOLVER_HPP
#define ATTRIBUTE_RESOLVER_HPP

#include "attribute_resolver.h"

#include "sl_status.h"
#include "attribute_store.h"

#include <functional>
#include <set>

/**
 * @brief Options for start_group_resolution.
 *
 * All fields default to the legacy behaviour so existing call sites
 * that pass no second argument continue to compile and behave identically.
 */
struct group_resolution_options {
        bool skip_supervision = false;
        uint8_t retry_count   = 5;
};

/**
 * @brief Atomically checks whether a SET resolution is in flight for @p node
 *        and either defers via rearm or triggers a fresh GET resolution.
 *
 * When @p options.skip_supervision is true, the group node's attribute type is
 * registered (process-lifetime, per type) so SET frames are sent without
 * Supervision encapsulation. Registration runs under resolver_mutex before the
 * pending-set check, including when resolution is deferred via rearm.
 *
 * @param node     Group attribute node to resolve.
 * @param options  Resolution options (retry count, supervision behaviour).
 */
void attribute_resolver_start_group_resolution(attribute_store_node_t node, group_resolution_options options = {});

namespace attribute_resolver
{

    using attribute_resolver_function = std::function<sl_status_t(attribute_store_node_t, uint8_t *, uint16_t *)>;

    /**
     * @brief Register same rule for multiples types.
     *
     * This allows you to bind multiple attributes to the same rules.
     *
     * @param node_type Attribute type for which these rules apply.
     * @param set_func  Generator function which will generate the SET command for
     *                  this attribute type.
     * @param get_func Generator function which will generate the GET command for
     *                 this attribute type.
     * @return sl_status_t
     */
    sl_status_t register_multiple_types_rules(const std::set<attribute_store_type_t> &node_type, attribute_resolver_function set_func, attribute_resolver_function get_func);

    /**
     * @brief Return the get function for a given attribute type
     *
     * @param node_type
     * @returns         NULL if this attribute type cannot be resolved.
     *                  Else the attribute_resolver_function_t that can resolve
     *                  this particular attribute
     */
    attribute_resolver_function get_function(attribute_store_type_t node_type);

    /**
     * @brief Return the set function for a given attribute type
     *
     * @param node_type
     * @returns         NULL if this attribute type cannot be resolved.
     *                  Else the attribute_resolver_function_t that can resolve
     *                  this particular attribute
     */
    attribute_resolver_function set_function(attribute_store_type_t node_type);

    /**
     * @brief Register an attribute rule.
     *
     * This function registers a new rule into the resolver rule book.
     * Only one rule can exist per attribute type. Some attributes have both a set and get
     * rule, some only have a set rule, some have only a get rule, some have no
     * rules.
     *
     * @param node_type Attribute type for which these rules apply.
     * @param set_func  Generator function which will generate the SET command for
     *                  this attribute type.
     * @param get_func Generator function which will generate the GET command for
     *                 this attribute type.
     * @return sl_status_t
     */
    sl_status_t register_rules(attribute_store_type_t node_type, attribute_resolver::attribute_resolver_function set_func, attribute_resolver::attribute_resolver_function get_func);

}  // namespace attribute_resolver

#endif  // ZWAVE_ATTRIBUTE_RESOLVER_H
/** @} end attribute_resolver */
