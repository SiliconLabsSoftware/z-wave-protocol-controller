# Header-only generic state machine engine
add_library(state_machine INTERFACE)
target_include_directories(state_machine INTERFACE include)
target_link_libraries(state_machine INTERFACE log common)
