add_library(zpc_attribute_store_test_helper_cpp
    zpc_attribute_store_test_helper_cpp.cpp
)
target_include_directories(zpc_attribute_store_test_helper_cpp PUBLIC .)
target_link_libraries(zpc_attribute_store_test_helper_cpp
                      PUBLIC zpc_attribute_store_test_helper unity2)
