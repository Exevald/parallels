#pragma once

#include "Actors.h"
#include "Bank.h"

#include <memory>
#include <thread>
#include <vector>

class Simulation
{
public:
	explicit Simulation(Money initialBankCash);

	void RunSequential(int iterations);
	void RunParallel(int iterations);

	[[nodiscard]] bool IsStateConsistent() const;
	[[nodiscard]] unsigned long long GetTotalOps() const { return m_bank->GetOperationsCount(); }

private:
	void ActorWorker(Actor* actor, int iterations, const std::stop_token& stoken);

	std::unique_ptr<Bank> m_bank;
	const Money m_initialCash;

	std::unique_ptr<Homer> m_homer;
	std::unique_ptr<Marge> m_marge;
	std::unique_ptr<Bart> m_bart;
	std::unique_ptr<Lisa> m_lisa;
	std::unique_ptr<Apu> m_apu;
	std::unique_ptr<MrBurns> m_mrBurns;
	std::unique_ptr<Nelson> m_nelson;
	std::unique_ptr<Snake> m_snake;
	std::unique_ptr<Smithers> m_smithers;

	Actors m_actorsRefs{};
	std::vector<std::jthread> m_threads;
};