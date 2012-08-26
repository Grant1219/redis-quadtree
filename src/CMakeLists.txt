include_directories (${RQTREE_SOURCE_DIR}/vendor/include ${RQTREE_SOURCE_DIR}/include)
link_directories (${RQTREE_SOURCE_DIR}/vendor/lib)

add_library (rqtree STATIC util.cpp quadtree.cpp)

add_executable (rqtree_demo main.cpp)

target_link_libraries (rqtree_demo hiredis rqtree)
