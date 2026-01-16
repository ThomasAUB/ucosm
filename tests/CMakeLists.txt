cmake_minimum_required(VERSION 3.22)

find_package(Threads REQUIRED)

set(UCOSM_UNIT_TESTS ucosm_tests)

set(TARGET_SRC
	callable_test.cpp
	cfs_test.cpp
	periodic_test.cpp
	resumable_test.cpp
    rt_test.cpp
	tasklet_test.cpp
	tests.cpp
	ulink_test.cpp
)

add_executable(${UCOSM_UNIT_TESTS} ${TARGET_SRC})

# Link with ucosm implementation library
target_link_libraries(${UCOSM_UNIT_TESTS} ucosm_impl Threads::Threads)

# Register test with correct executable path
add_test(NAME ${UCOSM_UNIT_TESTS} COMMAND ${UCOSM_UNIT_TESTS})