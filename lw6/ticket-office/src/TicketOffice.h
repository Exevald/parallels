#pragma once

#include <atomic>

class TicketOffice
{
public:
	explicit TicketOffice(int numTickets);

	TicketOffice(const TicketOffice&) = delete;
	TicketOffice& operator=(const TicketOffice&) = delete;

	int SellTickets(int ticketsToBuy);
	int GetTicketsLeft() const noexcept;

private:
	std::atomic<int> m_numTickets;
};