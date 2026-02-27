#pragma once

#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <unordered_map>

using AccountId = unsigned long long;
using Money = long long;

class BankOperationError final : public std::runtime_error
{
public:
	using runtime_error::runtime_error;
};

class Bank
{
public:
	explicit Bank(Money cash);

	Bank(const Bank&) = delete;
	Bank& operator=(const Bank&) = delete;

	[[nodiscard]] unsigned long long GetOperationsCount() const { return m_operationsCount.load(); };

	void SendMoney(AccountId srcAccountId, AccountId dstAccountId, Money amount);
	[[nodiscard]] bool TrySendMoney(AccountId srcAccountId, AccountId dstAccountId, Money amount);

	[[nodiscard]] Money GetCash() const;
	[[nodiscard]] Money GetAccountBalance(AccountId accountId) const;

	void WithdrawMoney(AccountId account, Money amount);
	[[nodiscard]] bool TryWithdrawMoney(AccountId account, Money amount);
	void DepositMoney(AccountId accountId, Money amount);

	[[nodiscard]] AccountId OpenAccount();
	[[nodiscard]] Money CloseAccount(AccountId accountId);

private:
	bool SendMoneyInternal(AccountId srcId, AccountId dstId, Money amount, bool throwOnError);
	bool WithdrawMoneyInternal(AccountId accountId, Money amount, bool throwOnError);

	void EnsureExist(AccountId id) const;
	void EnsureExist(AccountId id1, AccountId id2) const;
private:
	struct Account
	{
		Money balance = 0;
		mutable std::shared_mutex mutex;
	};

	Money m_cash;
	mutable std::atomic<unsigned long long> m_operationsCount{ 0 };
	std::unordered_map<AccountId, Account> m_accounts;
	mutable std::shared_mutex m_cashMutex;
	mutable std::shared_mutex m_bankMutex;
	AccountId m_nextAccountId = 1;
};