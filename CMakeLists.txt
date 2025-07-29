cmake_minimum_required(VERSION 3.22)            # CMake version check
project(ucosm VERSION 0.0.1 LANGUAGES CXX ASM)  # Create project with assembly support
set(CMAKE_CXX_STANDARD 17)                      # Enable c++17 standard


find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
# Update submodules as needed

option(GIT_SUBMODULE "Check submodules during build" ON)
if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/ulink/CMakeLists.txt")
    message(FATAL_ERROR "The submodules were not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()


# Create library with coroutine implementation using setjmp/longjmp
add_library(ucosm_impl 
    include/ucosm/coroutine/context.cpp
)
target_include_directories(ucosm_impl PUBLIC 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/ulink/include
)
target_compile_features(ucosm_impl PUBLIC cxx_std_17)

message(STATUS "Coroutines enabled with portable setjmp/longjmp implementation")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

include(CTest)

enable_testing()

add_subdirectory(tests)