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

#ifndef ZPC_ATTRIBUTE_STORE_TEST_HELPER_CPP_HPP
#define ZPC_ATTRIBUTE_STORE_TEST_HELPER_CPP_HPP

// Unify cpp
#include "attribute.hpp"

extern "C" {
// Z-Wave types
#include "zwave_generic_types.h"
#include "zwave_command_class_version_types.h"
#include "zpc_attribute_store_test_helper.h"

// Test framework
#include "unity.h"

/**
 * @brief Helper namespace for the ZPC attribute store tests
 * 
 * CPP wrapper of the ZPC attribute store test helper. 
 * This is done in a separate file to avoid breaking linkage to existing tests. 
 */
namespace zpc_attribute_store_test_helper
{

////////////////////////////////////////////////////////////////////////////////////
// Global variables
// Must be declared as "extern" and defined in the cpp to avoid multiple definition
// More information : https://stackoverflow.com/questions/11478152/how-to-work-with-variable-in-namespace
////////////////////////////////////////////////////////////////////////////////////
// Endpoint id node wrapper
extern attribute_store::attribute
  cpp_endpoint_id_node;  //NOSONAR - false positive

/**
 * @brief Initialize the test helper
 * 
 * Initialize the Z-Wave network and create the base structure for the tests.
 */
void zpc_attribute_store_test_helper_init();

////////////////////////////////////////////////////////////////////////////////////
// Version
////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Set version for current class
 * 
 * @param command_class_id Command class id to set version
 * @param version Command class version to set
 * @param parent Parent node of the node to get (default to current endpoint)
 */
void helper_set_command_class_version(zwave_command_class_t command_class_id,
                                      const zwave_cc_version_t &version,
                                      attribute_store::attribute parent
                                      = cpp_endpoint_id_node);

/**
 * @brief Get version for current class
 * 
 * @param command_class_id Command class id to get version
 *  
 * @return Command class version
 */
zwave_cc_version_t
  helper_get_command_class_version(zwave_command_class_t command_class_id);

////////////////////////////////////////////////////////////////////////////////////
// Generic Node/Attribute Test Helpers
////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Get a node and check that it exists
 * 
 * @note Test will fail if node doesn't exists
 * 
 * @param node_type Node type to get
 * @param parent Parent node of the node to get (default to current endpoint)
 * 
 * @return attribute_store::attribute Node that was found (garmented to exists)
 */
attribute_store::attribute
  helper_test_and_get_node(attribute_store_type_t node_type,
                           attribute_store::attribute parent
                           = cpp_endpoint_id_node);

/**
 * @brief Test that a node exists
 * 
 * @param node_type Node type to test
 * @param parent Parent node of the node to get (default to current endpoint)
 */
void helper_test_node_exists(attribute_store_type_t node_type,
                             attribute_store::attribute parent
                             = cpp_endpoint_id_node);
/**
 * @brief Test that a node doesn't exists
 * 
 * @param node_type Node type to test
 * @param parent Parent node of the node to get (default to current endpoint)
 */
void helper_test_node_does_not_exists(attribute_store_type_t node_type,
                                      attribute_store::attribute parent
                                      = cpp_endpoint_id_node);
}  // namespace zpc_attribute_store_test_helper

}  // extern "C"

// Cpp template functions
namespace zpc_attribute_store_test_helper
{
template<typename T> attribute_store::attribute helper_test_attribute_value(
  attribute_store_type_t node_type,
  T expected_value,
  attribute_store::attribute parent        = cpp_endpoint_id_node,
  attribute_store_node_value_state_t state = REPORTED_ATTRIBUTE)
{
  auto current_node = helper_test_and_get_node(node_type, parent);

  try {
    const std::string error_message
      = (std::string("Value mismatch for ") + current_node.name_and_id())
          .c_str();

    if constexpr (std::is_same<T, std::vector<uint8_t>>::value) {
      TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(
        expected_value.data(),
        current_node.reported<std::vector<uint8_t>>().data(),
        expected_value.size(),
        error_message.c_str());
    } else if constexpr (std::is_same<T, std::string>::value) {
      TEST_ASSERT_EQUAL_STRING_MESSAGE(
        expected_value.c_str(),
        current_node.reported<std::string>().c_str(),
        error_message.c_str());
    } else {
      TEST_ASSERT_EQUAL_MESSAGE(expected_value,
                                current_node.get<T>(state),
                                error_message.c_str());
    }
  } catch (std::exception &e) {
    TEST_FAIL_MESSAGE(e.what());
  }

  return current_node;
}
}  // namespace zpc_attribute_store_test_helper
#endif  // ZPC_ATTRIBUTE_STORE_TEST_HELPER_CPP_HPP