set(ULINK_UNIT_TESTS ulink_tests)

include(CTest)

file(GLOB TARGET_SRC "./tests.cpp" )

add_executable(${ULINK_UNIT_TESTS} ${TARGET_SRC})

add_test(${ULINK_UNIT_TESTS} ${ULINK_UNIT_TESTS})