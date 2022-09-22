include(build_utils)

add_library(util SHARED utils.c)
set_project_properties(util ${PROJECT_SOURCE_DIR}/include)

add_library(server_crypto SHARED server_crypto.c server_file_api.c)
target_link_libraries(server_crypto PUBLIC util ssl crypto)
set_project_properties(server_crypto ${PROJECT_SOURCE_DIR}/include)

add_library(server_backend SHARED server_ctrl.c server_args.c)
target_link_libraries(server_backend PUBLIC util thread_pool)
set_project_properties(server_backend ${PROJECT_SOURCE_DIR}/include)

add_executable(server server_main.c)
target_link_libraries(server PUBLIC server_backend)
set_project_properties(server ${PROJECT_SOURCE_DIR}/include)