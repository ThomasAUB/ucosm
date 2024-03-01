cmake_minimum_required(VERSION 3.13)            # CMake version check
project(ucosm VERSION 0.0.1 LANGUAGES CXX)      # Create project
set(CMAKE_CXX_STANDARD 17)                      # Enable c++17 standard

include_directories(${CMAKE_SOURCE_DIR}/include)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

# if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    enable_testing()

    add_subdirectory(tests)

    # add_test(ucosm_tests  ucosm_tests)
    
    # add_test(NAME ucosmTests COMMAND $<TARGET_FILE:tests/tests.cpp>)
    
# endif()