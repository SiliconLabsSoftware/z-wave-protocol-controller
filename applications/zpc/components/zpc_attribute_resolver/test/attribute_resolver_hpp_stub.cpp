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

#include "attribute_resolver.hpp"
#include "attribute_resolver_hpp_stub.h"

attribute_resolver_function_t set_function_stub = nullptr;

void attribute_resolver_set_function_stub(
  attribute_resolver_function_t function)
{
  set_function_stub = function;
}

namespace attribute_resolver
{
attribute_resolver_function set_function(attribute_store_type_t node_type)
{
  return set_function_stub;
}
}  // namespace attribute_resolver