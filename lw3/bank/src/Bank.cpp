#include "Bank.h"

namespace
{
void EnsureNotNegative(Money amount)
{
	if (amount < 0)
	{
		throw std::out_of_range("amount cannot be negative");
	}
}
} // namespace

Bank::Bank(Money cash)
	: m_cash(cash)
{
	if (cash < 0)
	{
		throw BankOperationError("Initial cash can't be less than 0");
	}
}

void Bank::SendMoney(AccountId srcAccountId, AccountId dstAccountId, Money amount)
{
	if (!SendMoneyInternal(srcAccountId, dstAccountId, amount, true))
	{
		throw BankOperationError("insufficient funds on source account");
	}
}

bool Bank::TrySendMoney(AccountId srcAccountId, AccountId dstAccountId, Money amount)
{
	return SendMoneyInternal(srcAccountId, dstAccountId, amount, false);
}

Money Bank::GetCash() const
{
	std::shared_lock lock(m_cashMutex);
	m_operationsCount.fetch_add(1, std::memory_order_relaxed);
	return m_cash;
}

Money Bank::GetAccountBalance(AccountId accountId) const
{
	std::shared_lock bankLock(m_bankMutex);
	EnsureExist(accountId);

	const auto& acc = m_accounts.at(accountId);
	std::shared_lock accountLock(acc.mutex);
	m_operationsCount.fetch_add(1, std::memory_order_relaxed);
	return acc.balance;
}

void Bank::WithdrawMoney(AccountId account, Money amount)
{
	if (!WithdrawMoneyInternal(account, amount, true))
	{
		throw BankOperationError("insufficient funds on account");
	}
}

bool Bank::TryWithdrawMoney(AccountId account, Money amount)
{
	return WithdrawMoneyInternal(account, amount, false);
}

void Bank::DepositMoney(AccountId accountId, Money amount)
{
	EnsureNotNegative(amount);
	std::shared_lock bankLock(m_bankMutex);
	EnsureExist(accountId);

	auto& acc = m_accounts.at(accountId);
	std::unique_lock accountLock(acc.mutex);
	std::unique_lock cashLock(m_cashMutex);

	if (m_cash < amount)
	{
		throw BankOperationError("Bank register cash is less than deposit amount");
	}

	acc.balance += amount;
	m_cash -= amount;
	m_operationsCount.fetch_add(1);
}

AccountId Bank::OpenAccount()
{
	std::unique_lock bankLock(m_bankMutex);
	const auto id = m_nextAccountId++;
	m_accounts.emplace(std::piecewise_construct, std::forward_as_tuple(id), std::forward_as_tuple());
	m_operationsCount.fetch_add(1, std::memory_order_relaxed);
	return id;
}

Money Bank::CloseAccount(AccountId accountId)
{
	std::unique_lock bankLock(m_bankMutex);
	EnsureExist(accountId);

	auto it = m_accounts.find(accountId);
	Money balance = it->second.balance;

	std::unique_lock cashLock(m_cashMutex);
	m_accounts.erase(it);
	m_cash += balance;
	m_operationsCount.fetch_add(1, std::memory_order_relaxed);

	return balance;
}

bool Bank::SendMoneyInternal(AccountId srcId, AccountId dstId, Money amount, bool throwOnError)
{
	EnsureNotNegative(amount);
	std::shared_lock bankLock(m_bankMutex);
	EnsureExist(srcId, dstId);

	if (srcId == dstId)
	{
		m_operationsCount.fetch_add(1);
		return true;
	}

	auto& src = m_accounts.at(srcId);
	auto& dst = m_accounts.at(dstId);

	if (srcId < dstId)
	{
		std::lock_guard l1(src.mutex);
		std::lock_guard l2(dst.mutex);
		if (src.balance < amount)
		{
			if (throwOnError)
			{
				throw BankOperationError("No money");
			}
			return false;
		}
		src.balance -= amount;
		dst.balance += amount;
	}
	else
	{
		std::lock_guard l1(dst.mutex);
		std::lock_guard l2(src.mutex);
		if (src.balance < amount)
		{
			if (throwOnError)
			{
				throw BankOperationError("No money");
			}
			return false;
		}
		src.balance -= amount;
		dst.balance += amount;
	}
	m_operationsCount.fetch_add(1);

	return true;
}

bool Bank::WithdrawMoneyInternal(AccountId accountId, Money amount, bool throwOnError)
{
	EnsureNotNegative(amount);
	std::shared_lock bankLock(m_bankMutex);
	EnsureExist(accountId);

	auto& acc = m_accounts.at(accountId);
	std::unique_lock accountLock(acc.mutex);

	if (acc.balance < amount)
	{
		if (throwOnError)
			throw BankOperationError("insufficient funds");
		return false;
	}

	std::unique_lock cashLock(m_cashMutex);
	acc.balance -= amount;
	m_cash += amount;
	m_operationsCount.fetch_add(1, std::memory_order_relaxed);

	return true;
}

void Bank::EnsureExist(AccountId id) const
{
	if (m_accounts.find(id) == m_accounts.end())
	{
		throw BankOperationError("account " + std::to_string(id) + " does not exist");
	}
}

void Bank::EnsureExist(AccountId id1, AccountId id2) const
{
	if (m_accounts.find(id1) == m_accounts.end() || m_accounts.find(id2) == m_accounts.end())
	{
		throw BankOperationError("one or both accounts do not exist");
	}
}
