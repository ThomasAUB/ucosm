set(UCOSM_UNIT_TESTS ucosm_tests)

file(GLOB TARGET_SRC "./*.cpp" )

add_executable(${UCOSM_UNIT_TESTS} ${TARGET_SRC})

# Link with ucosm implementation library
target_link_libraries(${UCOSM_UNIT_TESTS} ucosm_impl)

# Register test with correct executable path
add_test(NAME ${UCOSM_UNIT_TESTS} COMMAND ${UCOSM_UNIT_TESTS})