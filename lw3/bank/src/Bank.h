#pragma once

#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>

using AccountId = unsigned long long;
using Money = long long;

class BankOperationError : public std::runtime_error
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

	void SendMoney(AccountId srcAccountId, AccountId dstAccountId, Money amount);
	[[nodiscard]] bool TrySendMoney(AccountId srcAccountId, AccountId dstAccountId, Money amount);
	[[nodiscard]] Money GetCash() const noexcept;
	[[nodiscard]] Money GetAccountBalance(AccountId accountId) const;
	void WithdrawMoney(AccountId accountId, Money amount);
	[[nodiscard]] bool TryWithdrawMoney(AccountId accountId, Money amount);
	void DepositMoney(AccountId accountId, Money amount);
	[[nodiscard]] AccountId OpenAccount();
	[[nodiscard]] Money CloseAccount(AccountId accountId);

private:
	struct Account
	{
		Money balance = 0;
		mutable std::mutex mtx;
	};

	[[nodiscard]] Account& GetAccountRef(AccountId id);
	[[nodiscard]] const Account& GetAccountRef(AccountId id) const;

	Money m_cash;
	std::unordered_map<AccountId, Account> m_accounts;
	AccountId m_nextAccountId = 1;
	mutable std::mutex m_cashMutex;
	mutable std::mutex m_bankMutex;
};