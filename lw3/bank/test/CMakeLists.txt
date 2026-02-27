include(GoogleTest)

add_executable(bank_test
        Bank_tests.cpp
        Actors_test.cpp
)

target_link_libraries(bank_test PRIVATE GTest::GTest GTest::gtest_main bank_lib)

gtest_discover_tests(bank_test)
