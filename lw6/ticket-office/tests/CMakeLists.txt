include(GoogleTest)

add_executable(ticket-office-test
        TicketOfficeTests.cpp
)

target_link_libraries(ticket-office-test PRIVATE GTest::gtest_main ticket-office-lib)
gtest_discover_tests(ticket-office-test)