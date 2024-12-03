cmake_minimum_required(VERSION 3.14)
project(PurrSockCTests)

# Enable testing
enable_testing()

# Include FetchContent to fetch external dependencies
include(FetchContent)

# Declare CMocka dependency
FetchContent_Declare(
    CMocka
    GIT_REPOSITORY https://git.cryptomilk.org/projects/cmocka.git
    GIT_TAG cmocka-1.1.5
)

# Set options before making the dependency available
set(CMocka_BUILD_SHARED_LIBS OFF CACHE BOOL "Build CMocka as a static library" FORCE)

# Make CMocka available
FetchContent_MakeAvailable(CMocka)

# Add the test executable
if (WIN32)
    add_executable(runTests test_windows_purrsock.c)
else()
    add_executable(runTests test_linux_purrsock.c)
endif()

# Link the required libraries
target_link_libraries(runTests PRIVATE cmocka purrsock)

# Add include directories if required (CMocka may need this explicitly in some cases)
target_include_directories(runTests PRIVATE ${CMocka_SOURCE_DIR}/include)

# Register the test
add_test(NAME PurrSockTests COMMAND runTests)
