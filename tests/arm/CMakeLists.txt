cmake_minimum_required(VERSION 3.22)

# Bare-metal build check, configured on its own :
#
#   cmake -S tests/arm -B build-arm -DUCOSM_ARM_CPU=cortex-m0plus
#
# Nothing runs : the check is that the headers and the ARM tasklet executor
# compile for the core and link without library calls the target doesn't
# have (__atomic_* on ARMv6-M).

# only ever built for arm-none-eabi, so the cross compiler is set here rather
# than through a toolchain file. Must come before project().
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(UCOSM_ARM_CPU "cortex-m4" CACHE STRING "Cortex-M core passed to -mcpu")
set(UCOSM_ARM_FPU "" CACHE STRING "FPU passed to -mfpu, empty for soft-float")

set(CMAKE_CXX_FLAGS_INIT "-mcpu=${UCOSM_ARM_CPU} -mthumb -fno-exceptions -fno-rtti")
if(UCOSM_ARM_FPU)
    string(APPEND CMAKE_CXX_FLAGS_INIT " -mfpu=${UCOSM_ARM_FPU} -mfloat-abi=hard")
endif()
set(CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nosys.specs")

project(ucosm_arm_check LANGUAGES CXX)

set(UCOSM_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# not the ucosm project itself : it would bring the host test suite along
add_subdirectory(${UCOSM_ROOT}/uatom uatom)

add_executable(${PROJECT_NAME} arm_build_check.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${UCOSM_ROOT}/include
    ${UCOSM_ROOT}/ulink/include
    ${UCOSM_ROOT}/tests
)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror)

target_link_libraries(${PROJECT_NAME} uatom)
