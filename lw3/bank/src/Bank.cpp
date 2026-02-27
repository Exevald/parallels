#include "Bank.h"

#include <algorithm>
#include <stdexcept>

namespace
{
void LockAccounts(std::mutex& a, std::mutex& b)
{
	if (std::addressof(a) == std::addressof(b))
	{
		a.lock();
	}
	else if (std::less<>{}(std::addressof(a), std::addressof(b)))
	{
		a.lock();
		b.lock();
	}
	else
	{
		b.lock();
		a.lock();
	}
}

void UnlockAccounts(std::mutex& a, std::mutex& b)
{
	a.unlock();
	if (std::addressof(a) != std::addressof(b))
		b.unlock();
}

void ValidateAmount(Money amount)
{
	if (amount < 0)
	{
		throw std::out_of_range("Amount can't be negative");
	}
}
} // namespace

Bank::Bank(Money cash)
	: m_cash(cash)
{
	if (cash < 0)
		throw BankOperationError("Initial cash can't be less than 0");
}

void Bank::SendMoney(AccountId srcId, AccountId dstId, Money amount)
{
	if (!TrySendMoney(srcId, dstId, amount))
	{
		throw BankOperationError("Insufficient funds in source account: " + std::to_string(srcId));
	}
}

bool Bank::TrySendMoney(AccountId srcId, AccountId dstId, Money amount)
{
	ValidateAmount(amount);
	if (srcId == dstId)
	{
		return true;
	}

	auto& srcAcc = GetAccountRef(srcId);
	auto& dstAcc = GetAccountRef(dstId);

	LockAccounts(srcAcc.mtx, dstAcc.mtx);
	bool success = false;
	try
	{
		if (srcAcc.balance >= amount)
		{
			srcAcc.balance -= amount;
			dstAcc.balance += amount;
			success = true;
		}
	}
	catch (...)
	{
		UnlockAccounts(srcAcc.mtx, dstAcc.mtx);
		throw;
	}
	UnlockAccounts(srcAcc.mtx, dstAcc.mtx);
	return success;
}

Money Bank::GetCash() const noexcept
{
	std::lock_guard lock(m_cashMutex);
	return m_cash;
}

Money Bank::GetAccountBalance(AccountId id) const
{
	const auto& acc = GetAccountRef(id);
	std::lock_guard lock(acc.mtx);
	return acc.balance;
}

void Bank::WithdrawMoney(AccountId id, Money amount)
{
	ValidateAmount(amount);
	auto& acc = GetAccountRef(id);

	std::lock_guard accLock(acc.mtx);
	std::lock_guard cashLock(m_cashMutex);

	if (acc.balance < amount)
	{
		throw BankOperationError("Insufficient funds in account: " + std::to_string(id));
	}
	acc.balance -= amount;
	m_cash += amount;
}

bool Bank::TryWithdrawMoney(AccountId id, Money amount)
{
	ValidateAmount(amount);
	auto& acc = GetAccountRef(id);

	std::lock_guard accLock(acc.mtx);
	std::lock_guard cashLock(m_cashMutex);

	if (acc.balance < amount)
	{
		return false;
	}
	acc.balance -= amount;
	m_cash += amount;
	return true;
}

void Bank::DepositMoney(AccountId id, Money amount)
{
	ValidateAmount(amount);
	auto& acc = GetAccountRef(id);

	std::lock_guard cashLock(m_cashMutex);
	if (m_cash < amount)
	{
		throw BankOperationError("Insufficient cash in bank");
	}
	std::lock_guard accLock(acc.mtx);
	m_cash -= amount;
	acc.balance += amount;
}

AccountId Bank::OpenAccount()
{
	std::lock_guard lock(m_bankMutex);
	AccountId newId = m_nextAccountId++;
	m_accounts.try_emplace(newId);

	return newId;
}

Money Bank::CloseAccount(AccountId id)
{
	std::lock_guard globalLock(m_bankMutex);
	auto it = m_accounts.find(id);
	if (it == m_accounts.end())
	{
		throw BankOperationError("Account not found: " + std::to_string(id));
	}
	Money balance = it->second.balance;
	m_accounts.erase(it);

	{
		std::lock_guard cashLock(m_cashMutex);
		m_cash += balance;
	}

	return balance;
}

Bank::Account& Bank::GetAccountRef(AccountId id)
{
	std::lock_guard lock(m_bankMutex);
	auto it = m_accounts.find(id);
	if (it == m_accounts.end())
	{
		throw BankOperationError("Account not found: " + std::to_string(id));
	}
	return it->second;
}

const Bank::Account& Bank::GetAccountRef(AccountId id) const
{
	std::lock_guard lock(m_bankMutex);
	auto it = m_accounts.find(id);
	if (it == m_accounts.end())
	{
		throw BankOperationError("Account not found: " + std::to_string(id));
	}
	return it->second;
}