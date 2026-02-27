#include "Simulation.h"

#include <iostream>
#include <syncstream>
#include <vector>

Simulation::Simulation(Money totalEconomyMoney)
	: m_initialCash(totalEconomyMoney)
{
	m_bank = std::make_unique<Bank>(m_initialCash);

	m_homer = std::make_unique<Homer>(0, 0, *m_bank);
	m_marge = std::make_unique<Marge>(0, 0, *m_bank);
	m_bart = std::make_unique<Bart>(0);
	m_lisa = std::make_unique<Lisa>(0);
	m_apu = std::make_unique<Apu>(0, 0, *m_bank);
	m_mrBurns = std::make_unique<MrBurns>(0, 0, *m_bank);
	m_nelson = std::make_unique<Nelson>(0);
	m_snake = std::make_unique<Snake>(0, 0, *m_bank);
	m_smithers = std::make_unique<Smithers>(0, 0, *m_bank);

	std::vector<Actor*> allActors = {
		m_homer.get(), m_marge.get(), m_bart.get(), m_lisa.get(),
		m_apu.get(), m_mrBurns.get(), m_nelson.get(), m_snake.get(), m_smithers.get()
	};

	Money share = m_initialCash / static_cast<Money>(allActors.size());
	for (auto* actor : allActors)
	{
		actor->ReceiveCashFromWorld(share);
	}
	m_mrBurns->ReceiveCashFromWorld(static_cast<long long>(m_initialCash % allActors.size()));

	auto initAccount = [](ActorWithBankAccount* a, Money deposit) {
		a->OpenAccount();
		if (deposit > 0)
		{
			a->DepositMoney(deposit);
		}
	};

	initAccount(m_homer.get(), 4000);
	initAccount(m_marge.get(), 2000);
	initAccount(m_apu.get(), 10000);
	initAccount(m_mrBurns.get(), 50000);
	initAccount(m_snake.get(), 500);
	initAccount(m_smithers.get(), 3000);

	m_actorsRefs = {
		m_homer.get(), m_marge.get(), m_bart.get(), m_lisa.get(),
		m_apu.get(), m_mrBurns.get(), m_nelson.get(), m_snake.get(), m_smithers.get()
	};
}

void Simulation::ActorWorker(Actor* actor, int iterations, const std::stop_token& stoken)
{
	for (int i = 0; i < iterations; ++i)
	{
		if (stoken.stop_requested())
		{
			break;
		}

		try
		{
			actor->Act(m_actorsRefs);
		}
		catch (const BankOperationError& e)
		{
			std::osyncstream(std::cout) << "Transaction skipped: " << e.what() << std::endl;
		}
		catch (const std::exception& e)
		{
			std::osyncstream(std::cerr) << "Error: " << e.what() << std::endl;
		}
		std::this_thread::yield();
	}
}

void Simulation::RunSequential(int iterations)
{
	std::vector<Actor*> targets = {
		m_homer.get(), m_marge.get(), m_bart.get(), m_lisa.get(),
		m_apu.get(), m_mrBurns.get(), m_nelson.get(), m_snake.get(), m_smithers.get()
	};

	for (int i = 0; i < iterations; ++i)
	{
		for (auto* actor : targets)
		{
			try
			{
				actor->Act(m_actorsRefs);
			}
			catch (...)
			{
			}
		}
	}
}

void Simulation::RunParallel(int iterations)
{
	m_threads.clear();
	std::vector<Actor*> targets = {
		m_homer.get(), m_marge.get(), m_bart.get(), m_lisa.get(),
		m_apu.get(), m_mrBurns.get(), m_nelson.get(), m_snake.get(), m_smithers.get()
	};

	for (auto* actor : targets)
	{
		m_threads.emplace_back([this, actor, iterations](const std::stop_token& st) {
			this->ActorWorker(actor, iterations, st);
		});
	}
}

bool Simulation::IsStateConsistent() const
{
	Money totalOnAccounts = 0;
	Money totalInHands = 0;

	std::vector<Actor*> allActors = {
		m_homer.get(), m_marge.get(), m_bart.get(), m_lisa.get(),
		m_apu.get(), m_mrBurns.get(), m_nelson.get(), m_snake.get(), m_smithers.get()
	};

	for (auto* actor : allActors)
	{
		totalInHands += actor->GetCash();
	}
	auto getAccBalance = [](ActorWithBankAccount* a) {
		try
		{
			return a->GetAccountBalance();
		}
		catch (...)
		{
			return 0LL;
		}
	};

	totalOnAccounts += getAccBalance(m_homer.get());
	totalOnAccounts += getAccBalance(m_marge.get());
	totalOnAccounts += getAccBalance(m_apu.get());
	totalOnAccounts += getAccBalance(m_mrBurns.get());
	totalOnAccounts += getAccBalance(m_snake.get());
	totalOnAccounts += getAccBalance(m_smithers.get());

	Money bankCash = m_bank->GetCash();
	Money totalMoney = totalOnAccounts + bankCash;

	bool cashConsistency = (totalInHands == bankCash);
	bool totalConsistency = (totalMoney == m_initialCash);

	std::osyncstream(std::cout)
		<< "\n=== BANK'S CONSISTENCY REPORT ===\n"
		<< "Cash in Actors' pockets: " << totalInHands << "\n"
		<< "Cash in Bank's register: " << bankCash << "\n"
		<< "Total on Bank Accounts:  " << totalOnAccounts << "\n"
		<< "-----------------------------------\n"
		<< "System Total:         " << totalMoney << "\n"
		<< "Expected Initial Total:     " << m_initialCash << "\n"
		<< "-----------------------------------\n"
		<< "Cash Consistency Check:     " << (cashConsistency ? "PASSED" : "FAILED") << "\n"
		<< "Total Money Check:          " << (totalConsistency ? "PASSED" : "FAILED") << "\n"
		<< "===================================\n"
		<< std::endl;

	return cashConsistency && totalConsistency;
}