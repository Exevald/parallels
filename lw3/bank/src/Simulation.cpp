#include "Simulation.h"

Simulation::Simulation()
{
	m_bank = std::make_unique<Bank>(m_initialCash);

	m_homerAccount = m_bank->OpenAccount();
	m_margeAccount = m_bank->OpenAccount();
	m_apuAccount = m_bank->OpenAccount();
	m_mrBurnsAccount = m_bank->OpenAccount();
	m_snakeAccount = m_bank->OpenAccount();
	m_smithersAccount = m_bank->OpenAccount();

	m_homer = std::make_unique<Homer>(8000, m_homerAccount, *m_bank);
	m_marge = std::make_unique<Marge>(2000, m_margeAccount, *m_bank);
	m_bart = std::make_unique<Bart>(250);
	m_lisa = std::make_unique<Lisa>(250);
	m_apu = std::make_unique<Apu>(5000, m_apuAccount, *m_bank);
	m_mrBurns = std::make_unique<MrBurns>(50000, m_mrBurnsAccount, *m_bank);
	m_nelson = std::make_unique<Nelson>(250);
	m_snake = std::make_unique<Snake>(250, m_snakeAccount, *m_bank);
	m_smithers = std::make_unique<Smithers>(9000, m_smithersAccount, *m_bank);

	m_actors.homer = m_homer.get();
	m_actors.marge = m_marge.get();
	m_actors.bart = m_bart.get();
	m_actors.lisa = m_lisa.get();
	m_actors.apu = m_apu.get();
	m_actors.mrBurns = m_mrBurns.get();
	m_actors.nelson = m_nelson.get();
	m_actors.snake = m_snake.get();
	m_actors.smithers = m_smithers.get();
}

void Simulation::StartSimulation(int iterationsCount)
{
	for (int i = 0; i < iterationsCount; i++)
	{
		m_homer->Act(m_actors);
		m_marge->Act(m_actors);
		m_bart->Act(m_actors);
		m_lisa->Act(m_actors);
		m_apu->Act(m_actors);
		m_mrBurns->Act(m_actors);
		m_nelson->Act(m_actors);
		m_snake->Act(m_actors);
		m_smithers->Act(m_actors);
	}
	if (Simulation::IsStateConsistent())
	{
		std::cout << "State is consistent\n";
	}
	else
	{
		std::cout << "State is not consistent\n";
	}
}

bool Simulation::IsStateConsistent() const
{
	const Money bankCash = m_bank->GetCash();

	const Money totalCharacterCash = m_homer->GetCash()
		+ m_marge->GetCash()
		+ m_bart->GetCash()
		+ m_lisa->GetCash()
		+ m_apu->GetCash()
		+ m_mrBurns->GetCash()
		+ m_nelson->GetCash()
		+ m_snake->GetCash()
		+ m_smithers->GetCash();

	const Money totalCharacterBalance = m_homer->GetAccountBalance()
		+ m_marge->GetAccountBalance()
		+ m_apu->GetAccountBalance()
		+ m_mrBurns->GetAccountBalance()
		+ m_snake->GetAccountBalance()
		+ m_smithers->GetAccountBalance();

	const Money totalMoney = totalCharacterBalance + totalCharacterCash;
	std::cout << "Bank cash: " << bankCash << "\n"
			  << "Total accounts: " << totalCharacterBalance << "\n"
			  << "Total character cash: " << totalCharacterCash << "\n"
			  << "Initial cash: " << m_initialCash << "\n"
			  << "Total money: " << totalMoney << "\n";

	return totalMoney == m_initialCash;
}
