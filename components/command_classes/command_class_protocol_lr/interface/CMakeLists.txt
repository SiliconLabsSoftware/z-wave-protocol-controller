### Create connector event library
add_library(command_class_protocol_lr_interface INTERFACE)

target_include_directories(command_class_protocol_lr_interface 
 INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

target_link_libraries(command_class_protocol_lr_interface INTERFACE zwave_definitions)