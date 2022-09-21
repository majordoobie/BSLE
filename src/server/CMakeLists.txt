include(build_utils)

add_library(util SHARED utils.c)
set_project_properties(util ${PROJECT_SOURCE_DIR}/include)

add_library(server_hash SHARED server_hash.c)
set_project_properties(server_hash ${PROJECT_SOURCE_DIR}/include)

add_library(server_backend SHARED server_backend.c server_args.c)
target_link_libraries(server_backend PUBLIC util thread_pool)
set_project_properties(server_backend ${PROJECT_SOURCE_DIR}/include)

add_executable(server server.c)
target_link_libraries(server PUBLIC server_backend)
set_project_properties(server ${PROJECT_SOURCE_DIR}/include)