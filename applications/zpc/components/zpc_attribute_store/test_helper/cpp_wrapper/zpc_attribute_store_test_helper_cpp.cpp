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
// Helper class
#include "zpc_attribute_store_test_helper_cpp.hpp"
#include "attribute_store_defined_attribute_types.h"

#include "sl_log.h"

// C++ includes
#ifdef __cplusplus
#include <string>
#include <boost/format.hpp>
#endif

namespace zpc_attribute_store_test_helper
{

attribute_store::attribute cpp_endpoint_id_node = ATTRIBUTE_STORE_INVALID_NODE; //NOSONAR - false positive

void zpc_attribute_store_test_helper_init() {
  // Create base structure
  zpc_attribute_store_test_helper_create_network();
  // Cpp wrapper for endpoint id node
  cpp_endpoint_id_node = endpoint_id_node;
}

void helper_test_node_existence(attribute_store::attribute attribute,
                                bool should_exists,
                                const std::string &expected_attribute_name,
                                const std::string &expected_parent_name)
{
  TEST_ASSERT_EQUAL_MESSAGE(
    should_exists,
    attribute.is_valid(),
    (boost::format("Attribute '%1%' should %2% exists under '%3%'")
     % expected_attribute_name % (should_exists ? "" : "NOT")
     % expected_parent_name)
      .str()
      .c_str());
}

attribute_store::attribute
  helper_test_and_get_node(attribute_store_type_t node_type,
                           attribute_store::attribute parent)
{
  auto attribute = parent.child_by_type(node_type);

  helper_test_node_existence(attribute,
                             true,
                             attribute_store_get_type_name(node_type),
                             parent.name());
  return attribute;
}

void helper_test_node_exists(attribute_store_type_t node_type,
                             attribute_store::attribute parent)
{
  helper_test_node_existence(parent.child_by_type(node_type),
                             true,
                             attribute_store_get_type_name(node_type),
                             parent.name());
}
void helper_test_node_does_not_exists(attribute_store_type_t node_type,
                                      attribute_store::attribute parent)
{
  helper_test_node_existence(parent.child_by_type(node_type),
                             false,
                             attribute_store_get_type_name(node_type),
                             parent.name());
}


////////////////////////////////////////////////////////////////////////////////////
// Version helpers
////////////////////////////////////////////////////////////////////////////////////
void helper_set_command_class_version(zwave_command_class_t command_class_id,
                                      const zwave_cc_version_t &version,
                                      attribute_store::attribute parent)
{
  parent.add_node(ZWAVE_CC_VERSION_ATTRIBUTE(command_class_id))
    .set_reported(version);
}

zwave_cc_version_t
  helper_get_command_class_version(zwave_command_class_t command_class_id)
{
  try {
    return cpp_endpoint_id_node
      .child_by_type(ZWAVE_CC_VERSION_ATTRIBUTE(command_class_id))
      .reported<zwave_cc_version_t>();
  } catch (const std::exception &e) {
    sl_log_error("zpc_attribute_store_test_helper_cpp",
                 "Command class version not found for %d",
                 command_class_id);
    return 0;
  }
}

} // namespace zpc_attribute_store_test_helper