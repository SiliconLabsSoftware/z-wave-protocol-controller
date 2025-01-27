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
 * @defgroup unify_enums Unify Enums types
 * @ingroup unify_definitions
 * @brief Contains defines to make enums with specified underlying types
 *
 * @{
 */

#ifndef UIC_ENUM_H
#define UIC_ENUM_H
#include <inttypes.h>

/**
 * @brief Enum with &lt;type&gt; as size, where type may be any integer type
 *
 * Usage:
 * typedef enum {
 *   MY_ENTRY_1,
 *   MY_ENTRY_2,
 *   MY_ENTRY_X
 * ) UIC_ENUM(my_type, int16_t);
 * my_type x; //<< this will be of type int16_6
 * my_type_enum y; //<< this will be of enum type
 */
#define UIC_ENUM(name, type) \
  name_enum;                \
  typedef type name

/**
 * @brief Enum with 8 bit size
 *
 * Usage:
 * typedef enum {
 *   MY_ENTRY_1,
 *   MY_ENTRY_2,
 *   MY_ENTRY_X
 * ) UIC_ENUM8(my_type);
 * my_type x; //<< this will be of type uint8_t
 * my_type_enum y; //<< this will be of enum type
 */
#define UIC_ENUM8(name) UIC_ENUM(name, uint8_t)

/**
 * @brief Enum with 16 bit size
 *
 * Usage:
 * typedef enum {
 *   MY_ENTRY_1,
 *   MY_ENTRY_2,
 *   MY_ENTRY_X
 * ) UIC_ENUM16(my_type);
 * my_type x; //<< this will be of type uint16_t
 * my_type_enum y; //<< this will be of enum type
 */
#define UIC_ENUM16(name) UIC_ENUM(name, uint16_t)

/**
 * @brief Enum with 32 bit size
 *
 * Usage:
 * typedef enum {
 *   MY_ENTRY_1,
 *   MY_ENTRY_2,
 *   MY_ENTRY_X
 * ) UIC_ENUM32(my_type);
 * my_type x; //<< this will be of type uint32_t
 * my_type_enum y; //<< this will be of enum type
 */
#define UIC_ENUM32(name) UIC_ENUM(name, uint32_t)

#endif  //UIC_ENUM_H
        /** @} end uic_enum */
