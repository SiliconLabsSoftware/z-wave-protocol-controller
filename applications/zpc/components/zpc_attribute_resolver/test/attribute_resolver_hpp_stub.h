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
#ifndef ATTRIBUTE_RESOLVER_HPP_STUB_H
#define ATTRIBUTE_RESOLVER_HPP_STUB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "attribute_resolver.h"

/**
 * Since this test use a LOT of mock, we mock this function that 
 * was switched to C++ due to internal rework. 
 */
void attribute_resolver_set_function_stub(
  attribute_resolver_function_t function);

#ifdef __cplusplus
}
#endif

#endif // ATTRIBUTE_RESOLVER_HPP_STUB_H