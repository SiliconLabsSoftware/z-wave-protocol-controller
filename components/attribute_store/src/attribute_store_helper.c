
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
#include "attribute_store_helper.h"
#include "attribute_store_type_registration.h"
#include "log.h"

// Generic includes
#include <stdbool.h>
#include <float.h>
#include <math.h>
#include <stdio.h>   // for snprintf
#include <string.h>  // for memcpy

// NOTE: this file used to keep a file-scope `received_value`/`received_value_size`
// scratch buffer that was reused by every helper. It was not thread-safe: the
// internal `attribute_store_get_node_attribute_value()` mutex is released before
// the post-call size check / memcpy below, so a concurrent helper invocation on
// another thread could overwrite the buffer in between, causing spurious read
// failures (observed as "Unknown keyset" S2 send aborts during node interview).
// Each helper now uses its own stack-local buffer.

bool attribute_store_is_value_defined(attribute_store_node_t node, attribute_store_node_value_state_t value_state)
{
    uint8_t value_buffer[ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH];
    uint8_t value_size = 0;
    attribute_store_get_node_attribute_value(node, value_state, value_buffer, &value_size);
    return value_size != 0;
}

bool attribute_store_is_reported_defined(attribute_store_node_t node)
{
    return attribute_store_is_value_defined(node, REPORTED_ATTRIBUTE);
}

bool attribute_store_is_desired_defined(attribute_store_node_t node)
{
    return attribute_store_is_value_defined(node, DESIRED_ATTRIBUTE);
}

bool attribute_store_is_value_matched(attribute_store_node_t node)
{
    uint8_t reported_value[ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH] = {0};
    uint8_t reported_value_size                                  = 0;
    uint8_t desired_value[ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH]  = {0};
    uint8_t desired_value_size                                   = 0;
    attribute_store_get_node_attribute_value(node, REPORTED_ATTRIBUTE, reported_value, &reported_value_size);
    attribute_store_get_node_attribute_value(node, DESIRED_ATTRIBUTE, desired_value, &desired_value_size);
    if ((reported_value_size == desired_value_size) && (memcmp(reported_value, desired_value, reported_value_size) == 0)) {
        return true;
    }
    return false;
}

sl_status_t attribute_store_set_desired_as_reported(attribute_store_node_t node)
{
    uint8_t value_buffer[ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH];
    uint8_t value_size      = 0;
    sl_status_t read_status = attribute_store_get_node_attribute_value(node, REPORTED_ATTRIBUTE, value_buffer, &value_size);
    if (read_status == SL_STATUS_OK) {
        return attribute_store_set_node_attribute_value(node, DESIRED_ATTRIBUTE, value_buffer, value_size);
    }
    return read_status;
}

sl_status_t attribute_store_set_reported_as_desired(attribute_store_node_t node)
{
    uint8_t value_buffer[ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH];
    uint8_t value_size      = 0;
    sl_status_t read_status = attribute_store_get_node_attribute_value(node, DESIRED_ATTRIBUTE, value_buffer, &value_size);
    if (read_status == SL_STATUS_OK) {
        return attribute_store_set_node_attribute_value(node, REPORTED_ATTRIBUTE, value_buffer, value_size);
    }
    return read_status;
}

void attribute_store_undefine_reported(attribute_store_node_t node)
{
    attribute_store_set_node_attribute_value(node, REPORTED_ATTRIBUTE, NULL, 0);
}

sl_status_t attribute_store_set_reported(attribute_store_node_t node, const void *value, uint8_t value_size)
{
    return attribute_store_set_node_attribute_value(node, REPORTED_ATTRIBUTE, value, value_size);
}

sl_status_t attribute_store_get_reported(attribute_store_node_t node, void *value, size_t expected_size)
{
    return attribute_store_read_value(node, REPORTED_ATTRIBUTE, value, expected_size);
}

/**
 * @brief Sanitizes a char* pointer and copies a C string into the caller-supplied
 * buffer.
 *
 * @param string    C string to be sanitized and copied into the buffer.
 * @param buffer    Destination buffer of size ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH.
 * @returns The length of the string stored in the buffer (including the NUL terminator).
 */
static uint8_t sanitize_string_to_buffer(const char *string, uint8_t buffer[ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH])
{
    if (string == NULL) {
        buffer[0] = '\0';
        return sizeof(char);
    }

    // snprintf truncates to fit and always NUL-terminates within the buffer.
    int written = snprintf((char *)buffer, ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH, "%s", string);
    if (written < 0) {
        buffer[0] = '\0';
        return sizeof(char);
    }
    size_t length = ((size_t)written < ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH) ? (size_t)written : (ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH - 1);
    return (uint8_t)(length + sizeof(char));
}

sl_status_t attribute_store_set_reported_string(attribute_store_node_t node, const char *string)
{
    uint8_t buffer[ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH];
    uint8_t string_length = sanitize_string_to_buffer(string, buffer);
    return attribute_store_set_node_attribute_value(node, REPORTED_ATTRIBUTE, (const uint8_t *)buffer, string_length);
}

sl_status_t attribute_store_set_desired_string(attribute_store_node_t node, const char *string)
{
    uint8_t buffer[ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH];
    uint8_t string_length = sanitize_string_to_buffer(string, buffer);
    return attribute_store_set_node_attribute_value(node, DESIRED_ATTRIBUTE, (const uint8_t *)buffer, string_length);
}

sl_status_t attribute_store_concatenate_to_reported_string(attribute_store_node_t node, const char *string)
{
    uint8_t buffer[ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH];
    uint8_t stored_length = 0;
    attribute_store_get_node_attribute_value(node, REPORTED_ATTRIBUTE, buffer, &stored_length);

    // Previous value was undefined, just set the new string value.
    if (stored_length == 0) {
        return attribute_store_set_reported_string(node, string);
    }

    // Stored value should include a trailing NUL; existing string length excludes it.
    // Clamp defensively in case the store returns a value without a terminator.
    size_t existing_len = (size_t)stored_length - 1;
    if (existing_len >= ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH - 1) {
        existing_len = ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH - 1;
    }
    buffer[existing_len] = '\0';

    // Append, truncating to fit. snprintf returns the would-be length excluding NUL
    // and always NUL-terminates within the destination capacity.
    size_t remaining = ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH - existing_len;
    int written      = snprintf((char *)buffer + existing_len, remaining, "%s", string != NULL ? string : "");
    if (written < 0) {
        return SL_STATUS_FAIL;
    }
    size_t appended_len    = ((size_t)written < remaining) ? (size_t)written : (remaining - 1);
    uint8_t total_with_nul = (uint8_t)(existing_len + appended_len + sizeof(char));
    return attribute_store_set_node_attribute_value(node, REPORTED_ATTRIBUTE, (const uint8_t *)buffer, total_with_nul);
}

sl_status_t attribute_store_append_to_reported(attribute_store_node_t node, const uint8_t *array, uint8_t extra_array_length)
{
    uint8_t buffer[ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH];
    uint8_t array_length = 0;
    attribute_store_get_node_attribute_value(node, REPORTED_ATTRIBUTE, buffer, &array_length);

    if (array_length == 0) {
        return attribute_store_set_reported(node, array, extra_array_length);
    }

    uint8_t new_array_length;
    if (array_length + extra_array_length < 255) {
        new_array_length = array_length + extra_array_length;
    } else {
        return SL_STATUS_FAIL;
    }
    memcpy(buffer + array_length, array, extra_array_length);

    return attribute_store_set_node_attribute_value(node, REPORTED_ATTRIBUTE, (const uint8_t *)buffer, new_array_length);
}

/**
 * @brief Helper function that fetches the Reported or Desired value of a string
 *
 * Note: Does not work with the DESIRED_OR_REPORTED attribute value type.
 *
 * @param node            Attribute Store node to read the value from
 * @param string          Pointer where to store the read value
 * @param maximum_size    Maximum capacity of the pointer.
 * @param value_state     Attribute value state, reported or desired.
 * @return sl_status_t
 */
static sl_status_t attribute_store_get_string(attribute_store_node_t node, char *string, size_t maximum_size, attribute_store_node_value_state_t value_state)
{
    // Parameter validation
    if (string == NULL || maximum_size == 0) {
        return SL_STATUS_FAIL;
    }
    // Ensure null termination of the user buffer if we abort:
    string[0] = '\0';
    if (false == attribute_store_is_value_defined(node, value_state)) {
        return SL_STATUS_FAIL;
    }

    uint8_t buffer[ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH];
    uint8_t string_length = 0;
    attribute_store_get_node_attribute_value(node, value_state, buffer, &string_length);

    if (string_length == 0 || string_length > maximum_size) {
        return SL_STATUS_FAIL;
    }

    // The attribute store should contain the null terminations, but who knows
    if (buffer[string_length - 1] != '\0') {
        return SL_STATUS_FAIL;
    }

    // Bounds verified above: string_length <= maximum_size and buffer is NUL-terminated.
    memcpy(string, buffer, string_length);
    return SL_STATUS_OK;
}

sl_status_t attribute_store_get_reported_string(attribute_store_node_t node, char *string, size_t maximum_size)
{
    return attribute_store_get_string(node, string, maximum_size, REPORTED_ATTRIBUTE);
}

sl_status_t attribute_store_get_desired_string(attribute_store_node_t node, char *string, size_t maximum_size)
{
    return attribute_store_get_string(node, string, maximum_size, DESIRED_ATTRIBUTE);
}

sl_status_t attribute_store_get_desired_else_reported_string(attribute_store_node_t node, char *string, size_t maximum_size)
{
    if (attribute_store_is_value_defined(node, DESIRED_ATTRIBUTE)) {
        return attribute_store_get_desired_string(node, string, maximum_size);
    }
    return attribute_store_get_reported_string(node, string, maximum_size);
}

void attribute_store_undefine_desired(attribute_store_node_t node)
{
    attribute_store_set_node_attribute_value(node, DESIRED_ATTRIBUTE, NULL, 0);
}

sl_status_t attribute_store_set_desired(attribute_store_node_t node, const void *value, uint8_t value_size)
{
    return attribute_store_set_node_attribute_value(node, DESIRED_ATTRIBUTE, value, value_size);
}

sl_status_t attribute_store_get_desired(attribute_store_node_t node, void *value, size_t expected_size)
{
    return attribute_store_read_value(node, DESIRED_ATTRIBUTE, value, expected_size);
}

sl_status_t attribute_store_get_desired_else_reported(attribute_store_node_t node, void *value, size_t expected_size)
{
    return attribute_store_read_value(node, DESIRED_OR_REPORTED_ATTRIBUTE, value, expected_size);
}

sl_status_t attribute_store_copy_value(attribute_store_node_t source_node, attribute_store_node_t destination_node, attribute_store_node_value_state_t value_state)
{
    uint8_t value_buffer[ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH];
    uint8_t value_size = 0;

    sl_status_t read_status = attribute_store_get_node_attribute_value(source_node, value_state, value_buffer, &value_size);

    if (read_status != SL_STATUS_OK) {
        return read_status;
    }

    return attribute_store_set_node_attribute_value(destination_node, value_state, value_buffer, value_size);
}

sl_status_t attribute_store_read_value(attribute_store_node_t node, attribute_store_node_value_state_t value_state, void *read_value, size_t expected_size)
{
    uint8_t value_buffer[ATTRIBUTE_STORE_MAXIMUM_VALUE_LENGTH];
    uint8_t value_size = 0;
    attribute_store_get_node_attribute_value(node, value_state, value_buffer, &value_size);

    // If the value is undefined, we return SL_STATUS_FAIL.
    if (value_size == 0) {
        return SL_STATUS_FAIL;
    }

    if (expected_size != value_size) {
        return SL_STATUS_FAIL;
    }

    memcpy(read_value, value_buffer, expected_size);

    return SL_STATUS_OK;
}

sl_status_t attribute_store_set_child_reported(attribute_store_node_t parent, attribute_store_type_t type, const void *value, uint8_t value_size)
{
    attribute_store_node_t child_node = attribute_store_get_first_child_by_type(parent, type);
    if (child_node == ATTRIBUTE_STORE_INVALID_NODE) {
        child_node = attribute_store_add_node(type, parent);
    }
    return attribute_store_set_reported(child_node, value, value_size);
}

sl_status_t attribute_store_set_all_children_reported(attribute_store_node_t parent, attribute_store_type_t type, const void *value, uint8_t value_size)
{
    size_t child_index                = 0;
    sl_status_t status                = SL_STATUS_OK;
    attribute_store_node_t child_node = attribute_store_get_first_child_by_type(parent, type);

    while (child_node != ATTRIBUTE_STORE_INVALID_NODE) {
        status |= attribute_store_set_reported(child_node, value, value_size);
        child_index += 1;
        child_node = attribute_store_get_node_child_by_type(parent, type, child_index);
    }

    return status;
}

sl_status_t attribute_store_set_child_desired(attribute_store_node_t parent, attribute_store_type_t type, const void *value, uint8_t value_size)
{
    attribute_store_node_t child_node = attribute_store_get_first_child_by_type(parent, type);
    if (child_node == ATTRIBUTE_STORE_INVALID_NODE) {
        child_node = attribute_store_add_node(type, parent);
    }
    return attribute_store_set_desired(child_node, value, value_size);
}

sl_status_t attribute_store_get_child_reported(attribute_store_node_t parent, attribute_store_type_t type, void *value, size_t expected_size)
{
    return attribute_store_get_reported(attribute_store_get_first_child_by_type(parent, type), value, expected_size);
}

sl_status_t attribute_store_set_child_reported_only_if_missing(attribute_store_node_t parent, attribute_store_type_t type, const void *value, uint8_t value_size)
{
    attribute_store_node_t child_node = attribute_store_get_first_child_by_type(parent, type);
    if (child_node != ATTRIBUTE_STORE_INVALID_NODE) {
        return SL_STATUS_ALREADY_EXISTS;
    }
    child_node = attribute_store_add_node(type, parent);
    return attribute_store_set_reported(child_node, value, value_size);
}

sl_status_t attribute_store_set_child_reported_only_if_exists(attribute_store_node_t parent, attribute_store_type_t type, const void *value, uint8_t value_size)
{
    attribute_store_node_t child_node = attribute_store_get_first_child_by_type(parent, type);
    return attribute_store_set_reported(child_node, value, value_size);
}

sl_status_t attribute_store_set_child_reported_only_if_undefined(attribute_store_node_t parent, attribute_store_type_t type, const void *value, uint8_t value_size)
{
    attribute_store_node_t child_node = attribute_store_get_first_child_by_type(parent, type);
    if ((attribute_store_node_exists(child_node)) && attribute_store_is_reported_defined(child_node) == false) {
        return attribute_store_set_reported(child_node, value, value_size);
    }
    return SL_STATUS_OK;
}

sl_status_t attribute_store_set_child_desired_only_if_exists(attribute_store_node_t parent, attribute_store_type_t type, const void *value, uint8_t value_size)
{
    attribute_store_node_t child_node = attribute_store_get_first_child_by_type(parent, type);
    return attribute_store_set_desired(child_node, value, value_size);
}

void attribute_store_walk_tree(attribute_store_node_t top, void (*function)(attribute_store_node_t))
{
    // Apply the function on the current node:
    function(top);
    for (size_t i = 0; i < attribute_store_get_node_child_count(top); i++) {
        attribute_store_walk_tree(attribute_store_get_node_child(top, i), function);
    }
}

void attribute_store_walk_tree_with_return_value(attribute_store_node_t top, sl_status_t (*function)(attribute_store_node_t))
{
    // Apply the function on the current node, ignore the sl_status_t return value:
    function(top);
    for (size_t i = 0; i < attribute_store_get_node_child_count(top); i++) {
        attribute_store_walk_tree_with_return_value(attribute_store_get_node_child(top, i), function);
    }
}

void attribute_store_add_if_missing(attribute_store_node_t parent_node, const attribute_store_type_t attributes[], uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        // Verify if there is already one value attribute for this endpoint
        attribute_store_node_t value_node = attribute_store_get_first_child_by_type(parent_node, attributes[i]);

        // If not, create it. We only need this attribute.
        if (value_node == ATTRIBUTE_STORE_INVALID_NODE) {
            attribute_store_add_node(attributes[i], parent_node);
        }
    }
}

sl_status_t attribute_store_register_callback_by_type_to_array(attribute_store_node_changed_callback_t callback_function, const attribute_store_type_t types[], uint32_t types_count)
{
    sl_status_t status = SL_STATUS_OK;
    for (uint32_t i = 0; i < types_count; i++) {
        status |= attribute_store_register_callback_by_type(callback_function, types[i]);
    }
    return status;
}

sl_status_t attribute_store_delete_all_children(attribute_store_node_t node)
{
    attribute_store_node_t child_node = attribute_store_get_node_child(node, 0);

    while (child_node != ATTRIBUTE_STORE_INVALID_NODE) {
        if (SL_STATUS_OK != attribute_store_delete_node(child_node)) {
            return SL_STATUS_FAIL;
        }
        child_node = attribute_store_get_node_child(node, 0);
    }
    return SL_STATUS_OK;
}

sl_status_t attribute_store_set_number(attribute_store_node_t node, number_t value, attribute_store_node_value_state_t value_state)
{
    // Check if the attribute storage type is known
    attribute_store_storage_type_t storage_type = attribute_store_get_storage_type(attribute_store_get_node_type(node));

    switch (storage_type) {
        case U8_STORAGE_TYPE: {
            uint8_t new_value = (uint8_t)round(value);
            return attribute_store_set_node_attribute_value(node, value_state, &new_value, sizeof(new_value));
        }
        case U16_STORAGE_TYPE: {
            uint16_t new_value = (uint16_t)round(value);
            return attribute_store_set_node_attribute_value(node, value_state, (uint8_t *)&new_value, sizeof(new_value));
        }
        case U32_STORAGE_TYPE: {
            uint32_t new_value = (uint32_t)round(value);
            return attribute_store_set_node_attribute_value(node, value_state, (uint8_t *)&new_value, sizeof(new_value));
        }
        case U64_STORAGE_TYPE: {
            uint64_t new_value = (uint64_t)round(value);
            return attribute_store_set_node_attribute_value(node, value_state, (uint8_t *)&new_value, sizeof(new_value));
        }
        case I8_STORAGE_TYPE: {
            int8_t new_value = (int8_t)round(value);
            return attribute_store_set_node_attribute_value(node, value_state, (uint8_t *)&new_value, sizeof(new_value));
        }
        case I16_STORAGE_TYPE: {
            int16_t new_value = (int16_t)round(value);
            return attribute_store_set_node_attribute_value(node, value_state, (uint8_t *)&new_value, sizeof(new_value));
        }
        case UNKNOWN_STORAGE_TYPE:
        case I32_STORAGE_TYPE: {
            int32_t new_value = (int32_t)round(value);
            return attribute_store_set_node_attribute_value(node, value_state, (uint8_t *)&new_value, sizeof(new_value));
        }
        case I64_STORAGE_TYPE: {
            int64_t new_value = (int64_t)round(value);
            return attribute_store_set_node_attribute_value(node, value_state, (uint8_t *)&new_value, sizeof(new_value));
        }
        case FLOAT_STORAGE_TYPE: {
            float new_value = (float)value;
            return attribute_store_set_node_attribute_value(node, value_state, (uint8_t *)&new_value, sizeof(new_value));
        }
        case DOUBLE_STORAGE_TYPE: {
            double new_value = (double)value;
            return attribute_store_set_node_attribute_value(node, value_state, (uint8_t *)&new_value, sizeof(new_value));
        }

        default:
            return SL_STATUS_FAIL;
    }
}

sl_status_t attribute_store_set_reported_number(attribute_store_node_t node, number_t value)
{
    return attribute_store_set_number(node, value, REPORTED_ATTRIBUTE);
}

sl_status_t attribute_store_set_desired_number(attribute_store_node_t node, number_t value)
{
    return attribute_store_set_number(node, value, DESIRED_ATTRIBUTE);
}

number_t attribute_store_get_number(attribute_store_node_t node, attribute_store_node_value_state_t value_state)
{
    attribute_store_storage_type_t storage_type = attribute_store_get_storage_type(attribute_store_get_node_type(node));

    switch (storage_type) {
        case U8_STORAGE_TYPE: {
            uint8_t value;
            return (attribute_store_read_value(node, value_state, &value, sizeof(value)) == SL_STATUS_OK) ? (number_t)value : FLT_MIN;
        }
        case U16_STORAGE_TYPE: {
            uint16_t value;
            return (attribute_store_read_value(node, value_state, &value, sizeof(value)) == SL_STATUS_OK) ? (number_t)value : FLT_MIN;
        }
        case U32_STORAGE_TYPE: {
            uint32_t value;
            return (attribute_store_read_value(node, value_state, &value, sizeof(value)) == SL_STATUS_OK) ? (number_t)value : FLT_MIN;
        }
        case U64_STORAGE_TYPE: {
            uint64_t value;
            return (attribute_store_read_value(node, value_state, &value, sizeof(value)) == SL_STATUS_OK) ? (number_t)value : FLT_MIN;
        }
        case I8_STORAGE_TYPE: {
            int8_t value;
            return (attribute_store_read_value(node, value_state, &value, sizeof(value)) == SL_STATUS_OK) ? (number_t)value : FLT_MIN;
        }
        case I16_STORAGE_TYPE: {
            int16_t value;
            return (attribute_store_read_value(node, value_state, &value, sizeof(value)) == SL_STATUS_OK) ? (number_t)value : FLT_MIN;
        }
        case UNKNOWN_STORAGE_TYPE:
        case I32_STORAGE_TYPE: {
            int32_t value;
            return (attribute_store_read_value(node, value_state, &value, sizeof(value)) == SL_STATUS_OK) ? (number_t)value : FLT_MIN;
        }
        case I64_STORAGE_TYPE: {
            int64_t value;
            return (attribute_store_read_value(node, value_state, &value, sizeof(value)) == SL_STATUS_OK) ? (number_t)value : FLT_MIN;
        }
        case FLOAT_STORAGE_TYPE: {
            float value;
            return (attribute_store_read_value(node, value_state, &value, sizeof(value)) == SL_STATUS_OK) ? (number_t)value : FLT_MIN;
        }
        case DOUBLE_STORAGE_TYPE: {
            double value;
            return (attribute_store_read_value(node, value_state, &value, sizeof(value)) == SL_STATUS_OK) ? (number_t)value : FLT_MIN;
        }

        default:
            return FLT_MIN;
    }
}

number_t attribute_store_get_reported_number(attribute_store_node_t node)
{
    return attribute_store_get_number(node, REPORTED_ATTRIBUTE);
}

number_t attribute_store_get_desired_number(attribute_store_node_t node)
{
    return attribute_store_get_number(node, DESIRED_ATTRIBUTE);
}

attribute_store_node_t attribute_store_emplace(attribute_store_node_t parent_node, attribute_store_type_t type, const void *value, uint8_t value_size)
{
    attribute_store_node_t child_node = attribute_store_get_node_child_by_value(parent_node, type, REPORTED_ATTRIBUTE, value, value_size, 0);

    if (child_node != ATTRIBUTE_STORE_INVALID_NODE) {
        return child_node;
    }

    child_node = attribute_store_add_node(type, parent_node);
    attribute_store_set_reported(child_node, value, value_size);
    return child_node;
}

attribute_store_node_t attribute_store_emplace_desired(attribute_store_node_t parent_node, attribute_store_type_t type, const void *value, uint8_t value_size)
{
    attribute_store_node_t child_node = attribute_store_get_node_child_by_value(parent_node, type, DESIRED_ATTRIBUTE, value, value_size, 0);

    if (child_node != ATTRIBUTE_STORE_INVALID_NODE) {
        return child_node;
    }

    child_node = attribute_store_add_node(type, parent_node);
    attribute_store_set_desired(child_node, value, value_size);
    return child_node;
}

attribute_store_node_t attribute_store_create_child_if_missing(attribute_store_node_t node, attribute_store_type_t type)
{
    attribute_store_node_t child_node = attribute_store_get_first_child_by_type(node, type);
    if (ATTRIBUTE_STORE_INVALID_NODE == child_node) {
        child_node = attribute_store_add_node(type, node);
    }
    return child_node;
}

uint8_t attribute_store_get_reported_size(attribute_store_node_t node)
{
    return attribute_store_get_node_value_size(node, REPORTED_ATTRIBUTE);
}

uint8_t attribute_store_get_desired_size(attribute_store_node_t node)
{
    return attribute_store_get_node_value_size(node, DESIRED_ATTRIBUTE);
}

uint8_t attribute_store_get_desired_else_reported_size(attribute_store_node_t node)
{
    uint8_t size = attribute_store_get_desired_size(node);
    if (size == 0) {
        return attribute_store_get_reported_size(node);
    }
    return size;
}
