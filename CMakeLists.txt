cmake_minimum_required (VERSION 2.8 CXX)
project (RQTREE)

set (LD_LIBRARY_PATH ${LD_LIBRARY_PATH} ${RQTREE_BINARY_DIR}/vendor/lib)

add_definitions(-std=c++0x)
add_subdirectory (src)
