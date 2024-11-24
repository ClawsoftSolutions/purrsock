cmake_minimum_required(VERSION 3.14)
project(PurrSockCTests)

include(FetchContent)

FetchContent_Declare(
    CMocka
    GIT_REPOSITORY https://git.cryptomilk.org/projects/cmocka.git
    GIT_TAG cmocka-1.1.5
)

FetchContent_MakeAvailable(CMocka)
set(CMocka_BUILD_SHARED_LIBS OFF CACHE BOOL "Build CMocka as a static library" FORCE)
if (WIN32)
    add_executable(runTests test_windows_purrsock.c)
    target_link_libraries(runTests PRIVATE cmocka purrsock)
else()
    add_executable(runTests test_linux_purrsock.c)
    target_link_libraries(runTests PRIVATE cmocka pthread purrsock)
endif()

add_test(NAME PurrSockTests COMMAND runTests)
