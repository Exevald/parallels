#include "TicketOffice.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <thread>
#include <vector>

TicketOffice::TicketOffice(int numTickets)
	: m_numTickets(numTickets)
{
}
int TicketOffice::SellTickets(const int ticketsToBuy)
{
	if (ticketsToBuy <= 0)
	{
		throw std::invalid_argument("ticketsToBuy must be positive");
	}

	int currentTickets = m_numTickets.load(std::memory_order_relaxed);
	while (true)
	{
		const int sold = std::min(ticketsToBuy, currentTickets);
		if (sold == 0)
		{
			return 0;
		}

		if (const int nextTickets = currentTickets - sold;
			m_numTickets.compare_exchange_weak(currentTickets, nextTickets,
				std::memory_order_relaxed,
				std::memory_order_relaxed))
		{
			return sold;
		}
	}
}

int TicketOffice::GetTicketsLeft() const noexcept
{
	return m_numTickets.load(std::memory_order_relaxed);
}