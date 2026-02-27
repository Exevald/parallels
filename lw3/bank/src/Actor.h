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
		if (cash < 0)
		{
			throw std::out_of_range("Initial cash can't be negative");
		}
	}

	[[nodiscard]] Money GetCash() const
	{
		return m_cash;
	}

	[[nodiscard]] bool SendCash(Actor& recipient, Money amount)
	{
		if (m_cash < amount)
		{
			return false;
		}
		m_cash -= amount;
		recipient.AddCash(amount);
		return true;
	}

	[[nodiscard]] bool StealCash(Actor& victim, Money amount)
	{
		return victim.SendCash(*this, amount);
	}

	virtual void Act(Actors& actors) = 0;

	virtual ~Actor() = default;

protected:
	void AddCash(Money amount)
	{
		m_cash += amount;
	}

	bool SpendCash(Money amount)
	{
		if (m_cash < amount)
		{
			return false;
		}
		m_cash -= amount;
		return true;
	}

private:
	Money m_cash;
};

class ActorWithBankAccount : public Actor
{
public:
	ActorWithBankAccount(Money cash, AccountId accountId, Bank& bank)
		: Actor(cash)
		, m_accountId(accountId)
		, m_bank(bank)
	{
		ActorWithBankAccount::DepositMoney(cash);
	}

	[[nodiscard]] Money GetAccountBalance() const
	{
		return m_bank.GetAccountBalance(m_accountId);
	}

	[[nodiscard]] AccountId GetAccountId() const
	{
		return m_accountId;
	}

	[[nodiscard]] bool SendMoney(AccountId dstAccountId, Money amount)
	{
		return m_bank.TrySendMoney(m_accountId, dstAccountId, amount);
	}

	[[nodiscard]] bool StealMoney(AccountId victimAccountId, Money amount)
	{
		return m_bank.TrySendMoney(victimAccountId, m_accountId, amount);
	}

	[[nodiscard]] bool WithdrawMoney(Money amount)
	{
		if (m_bank.TryWithdrawMoney(m_accountId, amount))
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
			m_bank.DepositMoney(m_accountId, amount);
			return true;
		}
		return false;
	}

	[[nodiscard]] Money CloseAccount()
	{
		Money balance = m_bank.CloseAccount(m_accountId);
		AddCash(balance);
		return balance;
	}

	void OpenAccount()
	{
		m_accountId = m_bank.OpenAccount();
	}

private:
	AccountId m_accountId;
	Bank& m_bank;
};