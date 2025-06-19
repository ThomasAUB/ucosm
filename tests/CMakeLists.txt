set(UCOSM_UNIT_TESTS ucosm_tests)

include(CTest)

file(GLOB TARGET_SRC "./*.cpp" )

add_executable(${UCOSM_UNIT_TESTS} ${TARGET_SRC})

add_test(${UCOSM_UNIT_TESTS} ${UCOSM_UNIT_TESTS})