add_library(threading STATIC
    src/threading.cpp
)

target_include_directories(threading
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/inc>
)

target_link_libraries(threading
    PUBLIC common
    PRIVATE log
)
