# Tests CMakeLists.txt

# Unit tests
add_executable(pq_unit_tests
    unit/test_result.cpp
    unit/test_types.cpp
)

target_link_libraries(pq_unit_tests
    PRIVATE
        pq
        GTest::gtest
        GTest::gtest_main
)

# Add tests
include(GoogleTest)
gtest_discover_tests(pq_unit_tests)
