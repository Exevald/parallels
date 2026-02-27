#include "Bank.h"

#include <gtest/gtest.h>

namespace
{
constexpr int initialCash = 100;
}

TEST(Bank, TestBankCreation)
{
	auto bank = Bank(initialCash);
	ASSERT_EQ(bank.GetCash(), initialCash);
}

TEST(Bank, TestBankDepositAndWithdrawMoney)
{
	auto bank = Bank(initialCash);
	auto accountId1 = bank.OpenAccount();
	auto accountId2 = bank.OpenAccount();

	bank.DepositMoney(accountId1, 10);
	bank.DepositMoney(accountId2, 50);
	ASSERT_EQ(bank.GetAccountBalance(accountId1), 10);
	ASSERT_EQ(bank.GetAccountBalance(accountId2), 50);
	ASSERT_EQ(bank.GetCash(), 40);

	bank.WithdrawMoney(accountId1, 5);
	ASSERT_EQ(bank.GetAccountBalance(accountId1), 5);
	ASSERT_EQ(bank.GetCash(), 45);

	bank.SendMoney(accountId2, accountId1, 50);
	ASSERT_EQ(bank.GetAccountBalance(accountId1), 55);
	ASSERT_EQ(bank.GetAccountBalance(accountId2), 0);
	ASSERT_EQ(bank.GetCash(), 45);
}

TEST(Bank, TestBankInvalidOperations)
{
	auto bank = Bank(initialCash);
	auto accountId1 = bank.OpenAccount();
	auto accountId2 = bank.OpenAccount();

	ASSERT_THROW(bank.DepositMoney(-1, 100), BankOperationError);
	ASSERT_THROW(bank.DepositMoney(accountId1, -1), std::out_of_range);

	bank.DepositMoney(accountId1, 10);
	bank.DepositMoney(accountId2, 50);

	ASSERT_THROW(bank.SendMoney(accountId2, accountId1, -50), std::out_of_range);
	ASSERT_THROW(bank.SendMoney(-1, -2, 100), BankOperationError);

	ASSERT_THROW(bank.WithdrawMoney(-1, 100), BankOperationError);
	ASSERT_THROW(bank.WithdrawMoney(accountId1, -100), std::out_of_range);

	ASSERT_THROW(bank.TryWithdrawMoney(-1, 100), BankOperationError);
	ASSERT_THROW(bank.TryWithdrawMoney(accountId1, -100), std::out_of_range);
}