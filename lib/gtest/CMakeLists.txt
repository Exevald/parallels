set(GTEST_HOMEBREW_ROOT "/opt/homebrew/Cellar/googletest/1.17.0")
set(GTEST_INCLUDE_DIR "${GTEST_HOMEBREW_ROOT}/include")
set(GTEST_SOURCE_DIR "${GTEST_INCLUDE_DIR}/googletest/googletest")

if (NOT EXISTS "${GTEST_SOURCE_DIR}/src/gtest-all.cc")
    message(FATAL_ERROR "Local googletest sources were not found at ${GTEST_SOURCE_DIR}")
endif()

add_library(gtest_local STATIC
        "${GTEST_SOURCE_DIR}/src/gtest-all.cc"
)

target_include_directories(gtest_local
        PUBLIC
        "${GTEST_INCLUDE_DIR}"
        "${GTEST_SOURCE_DIR}"
)

add_library(gtest_main_local STATIC
        "${GTEST_SOURCE_DIR}/src/gtest_main.cc"
)

target_include_directories(gtest_main_local
        PUBLIC
        "${GTEST_INCLUDE_DIR}"
        "${GTEST_SOURCE_DIR}"
)

target_link_libraries(gtest_main_local PUBLIC gtest_local)

add_library(GTest::gtest ALIAS gtest_local)
add_library(GTest::GTest ALIAS gtest_local)
add_library(GTest::gtest_main ALIAS gtest_main_local)
