cmake_minimum_required(VERSION 3.16)

project(PosixLibPq
    VERSION 1.0.0
    DESCRIPTION "C++17 PostgreSQL ORM Library with RAII and Type Safety"
    LANGUAGES CXX
)

# ============================================================================
# C++ Standard
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Build Options
# ============================================================================
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # Standalone: 예제/테스트 ON
    option(PQ_BUILD_EXAMPLES "Build example programs" ON)
else()
    # 서브디렉토리: 예제/테스트 OFF  
    option(PQ_BUILD_EXAMPLES "Build example programs" OFF)
endif()
option(PQ_BUILD_TESTS "Build unit tests" ON)
option(PQ_RUN_TESTS "Run tests after build" OFF)
option(PQ_INSTALL "Generate install target" ON)

# ============================================================================
# Compiler Warnings
# ============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Werror=return-type
        -Wno-unused-parameter
    )
endif()

# ============================================================================
# Find Dependencies
# ============================================================================

# PostgreSQL (libpq) - Manual path options
# Use these to override automatic detection:
#   cmake .. -DPQ_LIBPQ_INCLUDE_DIR=/path/to/include
#   cmake .. -DPQ_LIBPQ_LIBRARY=/path/to/libpq.so
set(PQ_LIBPQ_INCLUDE_DIR "" CACHE PATH "Manual path to libpq-fe.h directory")
set(PQ_LIBPQ_LIBRARY "" CACHE FILEPATH "Manual path to libpq library (libpq.so/dylib/lib)")

if(PQ_LIBPQ_INCLUDE_DIR AND PQ_LIBPQ_LIBRARY)
    # Use manually specified paths
    message(STATUS "Using manually specified PostgreSQL paths:")
    message(STATUS "  Include: ${PQ_LIBPQ_INCLUDE_DIR}")
    message(STATUS "  Library: ${PQ_LIBPQ_LIBRARY}")
    
    # Validate paths exist
    if(NOT EXISTS "${PQ_LIBPQ_INCLUDE_DIR}/libpq-fe.h")
        message(FATAL_ERROR "libpq-fe.h not found in ${PQ_LIBPQ_INCLUDE_DIR}")
    endif()
    if(NOT EXISTS "${PQ_LIBPQ_LIBRARY}")
        message(FATAL_ERROR "Library not found: ${PQ_LIBPQ_LIBRARY}")
    endif()
    
    # Create imported target for consistency
    set(PostgreSQL_INCLUDE_DIRS "${PQ_LIBPQ_INCLUDE_DIR}")
    set(PostgreSQL_LIBRARIES "${PQ_LIBPQ_LIBRARY}")
    set(PostgreSQL_FOUND TRUE)
else()
    # Auto-detect using CMake's FindPostgreSQL module
    find_package(PostgreSQL REQUIRED)
    
    if(PostgreSQL_FOUND)
        message(STATUS "Found PostgreSQL: ${PostgreSQL_VERSION_STRING}")
        message(STATUS "  Include: ${PostgreSQL_INCLUDE_DIRS}")
        message(STATUS "  Library: ${PostgreSQL_LIBRARIES}")
    endif()
endif()

# ============================================================================
# Library Target
# ============================================================================

set(PQ_SOURCES
    src/core/Connection.cpp
    src/core/Transaction.cpp
    src/core/ConnectionPool.cpp
)

set(PQ_HEADERS
    include/pq/pq.hpp
    include/pq/core/PqHandle.hpp
    include/pq/core/Result.hpp
    include/pq/core/Types.hpp
    include/pq/core/QueryResult.hpp
    include/pq/core/Connection.hpp
    include/pq/core/Transaction.hpp
    include/pq/core/ConnectionPool.hpp
    include/pq/orm/Entity.hpp
    include/pq/orm/Mapper.hpp
    include/pq/orm/Repository.hpp
)

add_library(pq ${PQ_SOURCES})
add_library(pq::pq ALIAS pq)

target_include_directories(pq
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${PostgreSQL_INCLUDE_DIRS}
)

target_link_libraries(pq
    PUBLIC
        ${PostgreSQL_LIBRARIES}
)

target_compile_features(pq PUBLIC cxx_std_17)

# Set library properties
set_target_properties(pq PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    POSITION_INDEPENDENT_CODE ON
)

# ============================================================================
# Examples
# ============================================================================
if(PQ_BUILD_EXAMPLES)
    add_executable(usage_example examples/usage_example.cpp)
    target_link_libraries(usage_example PRIVATE pq)
endif()

# ============================================================================
# Tests
# ============================================================================
if(PQ_BUILD_TESTS)
    enable_testing()
    
    # Try to find system-installed Google Test first
    find_package(GTest QUIET)
    
    if(NOT GTest_FOUND)
        message(STATUS "Google Test not found locally, fetching from GitHub...")
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
        )
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
    
    add_subdirectory(tests)
    
    # Run tests after build if PQ_RUN_TESTS is ON
    if(PQ_RUN_TESTS)
        add_custom_target(run_tests ALL
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --verbose
            DEPENDS pq_unit_tests
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running unit tests..."
        )
    endif()
endif()

# ============================================================================
# Installation
# ============================================================================
if(PQ_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)
    
    # Install library
    install(TARGETS pq
        EXPORT pqTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    
    # Install headers
    install(DIRECTORY include/pq
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
    
    # Install CMake config files
    install(EXPORT pqTargets
        FILE pqTargets.cmake
        NAMESPACE pq::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pq
    )
    
    # Generate and install package config
    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/pqConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/pqConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pq
    )
    
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/pqConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/pqConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/pqConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pq
    )
endif()

# ============================================================================
# Summary
# ============================================================================
message(STATUS "")
message(STATUS "PosixLibPq Configuration Summary:")
message(STATUS "  Version:         ${PROJECT_VERSION}")
message(STATUS "  C++ Standard:    ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Examples:  ${PQ_BUILD_EXAMPLES}")
message(STATUS "  Build Tests:     ${PQ_BUILD_TESTS}")
message(STATUS "  Run Tests:       ${PQ_RUN_TESTS}")
message(STATUS "  Install:         ${PQ_INSTALL}")
message(STATUS "")
