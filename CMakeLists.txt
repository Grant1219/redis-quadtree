cmake_minimum_required (VERSION 2.8)
project (RQTREE)

add_definitions(-std=c++0x)

include_directories ("${RQTREE_SOURCE_DIR}/vendor/include" "${RQTREE_SOURCE_DIR}/include")
link_directories ("${RQTREE_SOURCE_DIR}/vendor/lib")

add_library (rqtree
    STATIC
    "${RQTREE_SOURCE_DIR}/src/util.cpp"
    "${RQTREE_SOURCE_DIR}/src/quadtree.cpp"
    )

add_executable (rqtree_demo
    "${RQTREE_SOURCE_DIR}/src/main.cpp"
    )

target_link_libraries (rqtree_demo hiredis rqtree)
