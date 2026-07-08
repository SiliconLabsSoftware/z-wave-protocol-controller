### Create connector event library
add_library(command_class_association_grp_info_interface INTERFACE)

target_include_directories(command_class_association_grp_info_interface 
 INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/generated/inc>
)

target_link_libraries(command_class_association_grp_info_interface INTERFACE zwave_definitions)