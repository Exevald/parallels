#include "TicketOffice.h"

#include <algorithm>
#include <atomic>
#include <gtest/gtest.h>
#include <stdexcept>
#include <thread>
#include <vector>

TEST(TicketOfficeTest, InitialState)
{
	const TicketOffice office(100);
	EXPECT_EQ(office.GetTicketsLeft(), 100);
}

TEST(TicketOfficeTest, BasicSale)
{
	TicketOffice office(50);
	EXPECT_EQ(office.SellTickets(10), 10);
	EXPECT_EQ(office.GetTicketsLeft(), 40);
}

TEST(TicketOfficeTest, SellMoreThanAvailable)
{
	TicketOffice office(10);
	EXPECT_EQ(office.SellTickets(15), 10);
	EXPECT_EQ(office.GetTicketsLeft(), 0);
	EXPECT_EQ(office.SellTickets(1), 0);
}

TEST(TicketOfficeTest, InvalidArguments)
{
	TicketOffice office(100);
	EXPECT_THROW(office.SellTickets(0), std::invalid_argument);
	EXPECT_THROW(office.SellTickets(-5), std::invalid_argument);
}

TEST(TicketOfficeTest, ConcurrentSales)
{
	constexpr int initialTickets = 50000;
	constexpr int numThreads = 12;
	TicketOffice office(initialTickets);

	std::atomic totalSoldAcrossThreads{ 0 };

	auto worker = [&] {
		while (true)
		{
			const int toBuy = (std::rand() % 10) + 1;
			const int sold = office.SellTickets(toBuy);

			if (sold == 0)
			{
				break;
			}

			totalSoldAcrossThreads.fetch_add(sold, std::memory_order_relaxed);
		}
	};

	std::vector<std::thread> threads;
	for (int i = 0; i < numThreads; ++i)
	{
		threads.emplace_back(worker);
	}

	for (auto& t : threads)
	{
		t.join();
	}

	EXPECT_EQ(totalSoldAcrossThreads.load(), initialTickets);
	EXPECT_EQ(office.GetTicketsLeft(), 0);
}

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}