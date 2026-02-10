# Tests CMakeLists.txt

# Unit tests
add_executable(pq_unit_tests
    unit/test_result.cpp
    unit/test_types.cpp
    unit/test_entity.cpp
    unit/test_query_result.cpp
    unit/test_connection.cpp
    unit/test_mapper.cpp
    unit/test_repository.cpp
)

# Link GTest - handle both system-installed and FetchContent versions
if(TARGET GTest::gtest)
    target_link_libraries(pq_unit_tests
        PRIVATE
            pq
            GTest::gtest
            GTest::gtest_main
    )
else()
    target_link_libraries(pq_unit_tests
        PRIVATE
            pq
            gtest
            gtest_main
    )
endif()

# Add tests
include(GoogleTest)
gtest_discover_tests(pq_unit_tests)
