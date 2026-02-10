# Tests CMakeLists.txt

function(pq_link_gtest target_name)
    if(TARGET GTest::gtest)
        target_link_libraries(${target_name}
            PRIVATE
                pq
                GTest::gtest
                GTest::gtest_main
        )
    else()
        target_link_libraries(${target_name}
            PRIVATE
                pq
                gtest
                gtest_main
        )
    endif()
endfunction()

include(GoogleTest)

# Unit tests
add_executable(pq_unit_tests
    unit/test_result.cpp
    unit/test_types.cpp
    unit/test_entity.cpp
    unit/test_query_result.cpp
    unit/test_connection.cpp
    unit/test_mapper.cpp
    unit/test_repository.cpp
    unit/test_schema_validator.cpp
)
pq_link_gtest(pq_unit_tests)
gtest_discover_tests(pq_unit_tests
    TEST_PREFIX "unit::"
    PROPERTIES LABELS "unit"
)

# Scenario tests (spec-level behavior without external DB dependency)
add_executable(pq_scenario_tests
    scenario/test_composite_pk_scenarios.cpp
    scenario/test_schema_and_types_scenarios.cpp
)
pq_link_gtest(pq_scenario_tests)
gtest_discover_tests(pq_scenario_tests
    TEST_PREFIX "scenario::"
    PROPERTIES LABELS "scenario"
)

# Integration tests (require real PostgreSQL only when PQ_TEST_CONN_STR is set)
add_executable(pq_integration_tests
    integration/test_repository_integration.cpp
    integration/test_schema_validator_integration.cpp
    integration/test_extended_types_integration.cpp
)
pq_link_gtest(pq_integration_tests)
gtest_discover_tests(pq_integration_tests
    TEST_PREFIX "integration::"
    PROPERTIES LABELS "integration"
)

# Performance-oriented tests (run only when explicitly enabled by env var)
add_executable(pq_perf_tests
    perf/test_sql_builder_perf.cpp
)
pq_link_gtest(pq_perf_tests)
gtest_discover_tests(pq_perf_tests
    TEST_PREFIX "perf::"
    PROPERTIES LABELS "perf"
)
