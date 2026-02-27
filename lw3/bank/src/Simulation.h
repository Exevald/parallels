#pragma once

#include "Actors.h"

#include <memory>

class Simulation
{
public:
	Simulation();

	void StartSimulation(int iterationsCount);

private:
	[[nodiscard]] bool IsStateConsistent() const;

	std::unique_ptr<Bank> m_bank;

	AccountId m_homerAccount;
	AccountId m_margeAccount;
	AccountId m_apuAccount;
	AccountId m_mrBurnsAccount;
	AccountId m_snakeAccount;
	AccountId m_smithersAccount;

	std::unique_ptr<Homer> m_homer;
	std::unique_ptr<Marge> m_marge;
	std::unique_ptr<Bart> m_bart;
	std::unique_ptr<Lisa> m_lisa;
	std::unique_ptr<Apu> m_apu;
	std::unique_ptr<MrBurns> m_mrBurns;
	std::unique_ptr<Nelson> m_nelson;
	std::unique_ptr<Snake> m_snake;
	std::unique_ptr<Smithers> m_smithers;

	Actors m_actors{};

	Money m_initialCash = 75000;
};