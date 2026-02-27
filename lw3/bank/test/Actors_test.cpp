#include "Actors.h"
#include "Bank.h"

#include <gtest/gtest.h>

namespace
{
constexpr int bankInitialCash = 45000;
constexpr int homerInitialCash = 8000;
constexpr int margeInitialCash = 2000;
constexpr int bartInitialCash = 250;
constexpr int lisaInitialCash = 250;
constexpr int mrBurnsInitialCash = 20000;
constexpr int apuInitialCash = 5000;
constexpr int nelsonInitialCash = 250;
constexpr int snakeInitialCash = 250;
constexpr int smithersInitialCash = 9000;
} // namespace

struct ActorsTestMiddleware
{
	std::unique_ptr<Bank> bank;
	std::unique_ptr<Homer> homer;
	std::unique_ptr<Marge> marge;
	std::unique_ptr<Bart> bart;
	std::unique_ptr<Lisa> lisa;
	std::unique_ptr<Apu> apu;
	std::unique_ptr<MrBurns> mrBurns;
	std::unique_ptr<Nelson> nelson;
	std::unique_ptr<Snake> snake;
	std::unique_ptr<Smithers> smithers;

	Actors actors{};

	ActorsTestMiddleware()
		: bank(std::make_unique<Bank>(bankInitialCash))
	{
		AccountId homerAccount = bank->OpenAccount();
		AccountId margeAccount = bank->OpenAccount();
		AccountId apuAccount = bank->OpenAccount();
		AccountId mrBurnsAccount = bank->OpenAccount();
		AccountId snakeAccount = bank->OpenAccount();
		AccountId smithersAccount = bank->OpenAccount();

		homer = std::make_unique<Homer>(homerInitialCash, homerAccount, *bank);
		marge = std::make_unique<Marge>(margeInitialCash, margeAccount, *bank);
		bart = std::make_unique<Bart>(bartInitialCash);
		lisa = std::make_unique<Lisa>(lisaInitialCash);
		apu = std::make_unique<Apu>(apuInitialCash, apuAccount, *bank);
		mrBurns = std::make_unique<MrBurns>(mrBurnsInitialCash, mrBurnsAccount, *bank);
		nelson = std::make_unique<Nelson>(nelsonInitialCash);
		snake = std::make_unique<Snake>(snakeInitialCash, snakeAccount, *bank);
		smithers = std::make_unique<Smithers>(smithersInitialCash, smithersAccount, *bank);

		actors.homer = homer.get();
		actors.marge = marge.get();
		actors.bart = bart.get();
		actors.lisa = lisa.get();
		actors.apu = apu.get();
		actors.mrBurns = mrBurns.get();
		actors.nelson = nelson.get();
		actors.snake = snake.get();
		actors.smithers = smithers.get();
	}
};

TEST(Actors, TestHomerAct)
{
	ActorsTestMiddleware testMiddleware;

	testMiddleware.homer->Act(testMiddleware.actors);
	ASSERT_EQ(testMiddleware.homer->GetAccountBalance(), homerInitialCash - 250 - 500 - 50 - 50);
	ASSERT_EQ(testMiddleware.homer->GetCash(), 0);
	ASSERT_EQ(testMiddleware.marge->GetAccountBalance(), margeInitialCash + 250);
	ASSERT_EQ(testMiddleware.mrBurns->GetAccountBalance(), mrBurnsInitialCash + 500);
	ASSERT_EQ(testMiddleware.bart->GetCash(), bartInitialCash + 50);
	ASSERT_EQ(testMiddleware.lisa->GetCash(), lisaInitialCash + 50);
}

TEST(Actors, TestMargeAct)
{
	ActorsTestMiddleware testMiddleware;

	testMiddleware.marge->Act(testMiddleware.actors);
	ASSERT_EQ(testMiddleware.marge->GetAccountBalance(), 2000 - 250);
	ASSERT_EQ(testMiddleware.marge->GetCash(), 0);
	ASSERT_EQ(testMiddleware.apu->GetAccountBalance(), apuInitialCash + 250);
}

TEST(Actors, TestBartAndLisaAct)
{
	ActorsTestMiddleware testMiddleware;

	testMiddleware.bart->Act(testMiddleware.actors);
	testMiddleware.lisa->Act(testMiddleware.actors);
	ASSERT_EQ(testMiddleware.bart->GetCash(), bartInitialCash - 25);
	ASSERT_EQ(testMiddleware.lisa->GetCash(), lisaInitialCash - 25);
	ASSERT_EQ(testMiddleware.apu->GetCash(), 50);
}

TEST(Actors, TestApuAct)
{
	ActorsTestMiddleware testMiddleware;

	testMiddleware.apu->Act(testMiddleware.actors);
	ASSERT_EQ(testMiddleware.apu->GetAccountBalance(), apuInitialCash - 700);
	ASSERT_EQ(testMiddleware.apu->GetCash(), 0);
}

TEST(Actors, TestMrBurnsAct)
{
	ActorsTestMiddleware testMiddleware;

	testMiddleware.mrBurns->Act(testMiddleware.actors);
	ASSERT_EQ(testMiddleware.mrBurns->GetAccountBalance(), mrBurnsInitialCash - 850 - 350);
	ASSERT_EQ(testMiddleware.homer->GetAccountBalance(), homerInitialCash + 850);
	ASSERT_EQ(testMiddleware.smithers->GetAccountBalance(), smithersInitialCash + 350);
}

TEST(Actors, TestNelsonAct)
{
	ActorsTestMiddleware testMiddleware;

	testMiddleware.nelson->Act(testMiddleware.actors);
	ASSERT_TRUE(testMiddleware.nelson->GetCash() == nelsonInitialCash);
	ASSERT_TRUE(testMiddleware.bart->GetCash() <= bartInitialCash);
	ASSERT_TRUE(testMiddleware.apu->GetCash() >= 0);
}

TEST(Actors, TestSnakeAct)
{
	ActorsTestMiddleware testMiddleware;

	testMiddleware.snake->Act(testMiddleware.actors);
	ASSERT_EQ(testMiddleware.snake->GetAccountBalance(), snakeInitialCash + 100);
	ASSERT_EQ(testMiddleware.homer->GetAccountBalance(), homerInitialCash - 100);
}

TEST(Actors, TestSmithersAct)
{
	ActorsTestMiddleware testMiddleware;

	testMiddleware.smithers->Act(testMiddleware.actors);
	ASSERT_EQ(testMiddleware.smithers->GetAccountBalance(), smithersInitialCash - 150);
	ASSERT_EQ(testMiddleware.apu->GetAccountBalance(), apuInitialCash + 150);
}