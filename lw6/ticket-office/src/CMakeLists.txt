add_library(ticket-office-lib STATIC
        TicketOffice.cpp
)

target_include_directories(ticket-office-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
