#pragma once

#include "Bank.h"

#include <iostream>
#include <memory>
#include <string>

class Actor;
class ActorWithBankAccount;

struct Actors
{
	ActorWithBankAccount* homer;
	ActorWithBankAccount* marge;
	Actor* bart;
	Actor* lisa;
	ActorWithBankAccount* apu;
	ActorWithBankAccount* mrBurns;
	Actor* nelson;
	ActorWithBankAccount* snake;
	ActorWithBankAccount* smithers;
};

class Actor
{
public:
	explicit Actor(Money cash)
		: m_cash(cash)
	{
	}

	virtual ~Actor() = default;

	Money GetCash() const
	{
		std::lock_guard lock(m_mtx);
		return m_cash;
	}

	bool SendCash(Actor& recipient, Money amount)
	{
		if (this == &recipient)
		{
			return true;
		}
		std::scoped_lock lock(m_mtx, recipient.m_mtx);
		if (m_cash < amount)
		{
			return false;
		}
		m_cash -= amount;
		recipient.m_cash += amount;
		return true;
	}

	bool StealCash(Actor& victim, Money amount)
	{
		return victim.SendCash(*this, amount);
	}

	bool SpendCash(Money amount)
	{
		if (m_cash < amount)
		{
			return false;
		}
		std::lock_guard lock(m_mtx);
		m_cash -= amount;
		return true;
	}

	void ReceiveCashFromWorld(Money amount)
	{
		std::lock_guard lock(m_mtx);
		m_cash += amount;
	}

	void OnAccountClosed(Money amount)
	{
		std::lock_guard lock(m_mtx);
		m_cash += amount;
	}

	virtual void Act(struct Actors& actors) = 0;

protected:
	mutable std::mutex m_mtx;
	Money m_cash;

	void AddCash(Money amount)
	{
		std::lock_guard lock(m_mtx);
		m_cash += amount;
	}
};

class ActorWithBankAccount : public Actor
{
public:
	ActorWithBankAccount(Money cash, AccountId id, Bank& bank)
		: Actor(cash)
		, m_accountId(id)
		, m_bank(bank)
	{
	}

	Money GetAccountBalance() const
	{
		return m_bank.GetAccountBalance(GetAccountId());
	}

	AccountId GetAccountId() const
	{
		std::lock_guard lock(m_idMtx);
		return m_accountId;
	}

	bool SendMoney(AccountId dstId, Money amount)
	{
		return m_bank.TrySendMoney(GetAccountId(), dstId, amount);
	}

	[[nodiscard]] bool StealMoney(AccountId victimAccountId, Money amount)
	{
		return m_bank.TrySendMoney(victimAccountId, GetAccountId(), amount);
	}

	[[nodiscard]] bool WithdrawMoney(Money amount)
	{
		if (m_bank.TryWithdrawMoney(GetAccountId(), amount))
		{
			AddCash(amount);
			return true;
		}
		return false;
	}

	bool DepositMoney(Money amount)
	{
		if (SpendCash(amount))
		{
			m_bank.DepositMoney(GetAccountId(), amount);
			return true;
		}
		return false;
	}

	Money CloseAccount()
	{
		AccountId idToClose;
		{
			std::lock_guard lock(m_idMtx);
			idToClose = m_accountId;
			m_accountId = 0;
		}
		Money balance = m_bank.CloseAccount(idToClose);
		this->OnAccountClosed(balance);
		return balance;
	}

	void OpenAccount()
	{
		std::lock_guard lock(m_idMtx);
		m_accountId = m_bank.OpenAccount();
	}

private:
	mutable std::mutex m_idMtx;
	AccountId m_accountId;
	Bank& m_bank;
};