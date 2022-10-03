# File contains all the deps for the project to build properly

# The FetchContent module is capable of fetching git repositories and handling
# the cloning of them so you do not have to deal with them
include(FetchContent)

# Only get the gtest if we are in debug mode otherwise just skip
IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Fetch the gtest framework and make it available
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG release-1.11.0
    )
    # GMock does not work in C only C++ so just disable
    set(BUILD_GTEST ON CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

    # Make available then include
    FetchContent_MakeAvailable(googletest)
ENDIF()

# Fetch the c_dsa libs
FetchContent_Declare(
        c_dsa
        GIT_REPOSITORY https://github.com/majordoobie/c_abstract_data_types.git
        GIT_TAG b888f3eeca9be30f12bf19143dbc548636de8552
)

# disable building c_dsa gtestss
set(BUILD_DSA_C_GTESTS OFF CACHE BOOL "" FORCE)

# Make available then include
FetchContent_MakeAvailable(c_dsa)


# Fetch the thread pool api
FetchContent_Declare(
        thpool
        GIT_REPOSITORY https://github.com/majordoobie/thread_pool_c
        GIT_TAG v0.0.2
)

# Disable building thpool gtests
set(BUILD_THPOOL_GTEST OFF CACHE BOOL "" FORCE)
# Make available then include
FetchContent_MakeAvailable(thpool)

