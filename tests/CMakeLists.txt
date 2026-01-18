set(UCOSM_UNIT_TESTS ucosm_tests)

set(TARGET_SRC
    tests.cpp
	# callable_test.cpp
	# cfs_test.cpp
	# periodic_test.cpp
	# resumable_test.cpp
	# rt_test.cpp
	ct_test.cpp
)

add_executable(${UCOSM_UNIT_TESTS} ${TARGET_SRC})

# Link with ucosm implementation library
target_link_libraries(${UCOSM_UNIT_TESTS} ucosm_impl)

# Ensure ugraph headers are visible to tests (ugraph is header-only for our use)
target_include_directories(${UCOSM_UNIT_TESTS} PRIVATE ${CMAKE_SOURCE_DIR}/ugraph/include)

# Register test with correct executable path
add_test(NAME ${UCOSM_UNIT_TESTS} COMMAND ${UCOSM_UNIT_TESTS})