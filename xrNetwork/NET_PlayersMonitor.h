#pragma once

#include "net_shared.h"
#include "NET_Common.h"
#include "../xrCore/fastdelegate.h"

class IClient;
extern bool IsSameClientID(IClient *client, ClientID const &id);

class PlayersMonitor
{
private:
	using PlayersCollectionT = xr_vector<IClient*>;
	xrCriticalSection csPlayers;
	PlayersCollectionT m_NetPlayers;
	PlayersCollectionT m_NetPlayersDisconnected;

	bool m_IteratingNowInPlayers;
	bool m_IteratingNowInDisconnectedPlayers;

public:
	PlayersMonitor(): m_IteratingNowInPlayers(false), m_IteratingNowInDisconnectedPlayers(false) {}

	template <typename ActionFunctor>
	void ForEachClientDo(ActionFunctor functor)
	{
		csPlayers.Enter();
		m_IteratingNowInPlayers = true;

		for (IClient *client: m_NetPlayers)
		{
			if (!client)
				continue;

			functor(client);
		}

		m_IteratingNowInPlayers = false;
		csPlayers.Leave();
	}

	void ForEachClientDo(fastdelegate::FastDelegate1<IClient *, void> &fast_delegate)
	{
		csPlayers.Enter();
		m_IteratingNowInPlayers = true;

		for (IClient *client : m_NetPlayers)
		{
			if (!client)
				continue;

			fast_delegate(client);
		}

		m_IteratingNowInPlayers = false;
		csPlayers.Leave();
	}

	template <typename SearchPredicate>
	IClient *FindAndEraseClient(SearchPredicate const &predicate)
	{
		csPlayers.Enter();
		VERIFY(!m_IteratingNowInPlayers);
		m_IteratingNowInPlayers = true;
		// TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		auto iter = std::find_if(m_NetPlayers.begin(), m_NetPlayers.end(), predicate);
		IClient *clResult = nullptr;

		if (iter != m_NetPlayers.end())
		{
			clResult = *iter;
			m_NetPlayers.erase(iter);
		}

		m_IteratingNowInPlayers = false;
		csPlayers.Leave();
		return clResult;
	}

	template <typename SearchPredicate>
	IClient *GetFoundClient(SearchPredicate const &predicate)
	{
		csPlayers.Enter();
		auto it = std::find_if(m_NetPlayers.begin(), m_NetPlayers.end(), predicate);
		IClient *clResult = nullptr;

		if (it != m_NetPlayers.end())		
			clResult = *it;
		
		csPlayers.Leave();
		return clResult;
	}

	IClient *GetClientById(ClientID const &id, bool disconnected)
	{
		IClient *clResult = nullptr;
		csPlayers.Enter();

		PlayersCollectionT::iterator itBegin;
		PlayersCollectionT::iterator itEnd;

		if (disconnected)
		{
			itBegin = m_NetPlayersDisconnected.begin();
			itEnd = m_NetPlayersDisconnected.end();
		}
		else
		{
			itBegin = m_NetPlayers.begin();
			itEnd = m_NetPlayers.end();
		}

		auto itSearch = std::find_if(itBegin, itEnd, [&id](IClient *client)
		{
			return IsSameClientID(client,id);
		});

		if (itSearch != itEnd)
			clResult = *itSearch;

		csPlayers.Leave();
		return clResult;
	}

	IClient *GetFirstClient(bool disconnected)
	{
		IClient *clResult = nullptr;
		csPlayers.Enter();

		if (disconnected)
		{
			auto it = m_NetPlayersDisconnected.begin();
			if (it != m_NetPlayersDisconnected.end())
				clResult = *it;
		}
		else
		{
			auto it = m_NetPlayers.begin();
			if (it != m_NetPlayers.end())
				clResult = *it;
		}

		csPlayers.Leave();
		return clResult;
	}

	void AddNewClient(IClient *new_client)
	{
		csPlayers.Enter();
		VERIFY(!m_IteratingNowInPlayers);
		m_NetPlayers.push_back(new_client);
		csPlayers.Leave();
	}

	template <typename ActionFunctor>
	void ForEachDisconnectedClientDo(ActionFunctor functor)
	{
		csPlayers.Enter();
		m_IteratingNowInDisconnectedPlayers = true;
		for_each(m_NetPlayersDisconnected.begin(), m_NetPlayersDisconnected.end(), functor);
		m_IteratingNowInDisconnectedPlayers = false;
		csPlayers.Leave();
	}

	template <typename SearchPredicate>
	IClient *FindAndEraseDisconnectedClient(SearchPredicate predicate)
	{
		csPlayers.Enter();
		VERIFY(!m_IteratingNowInDisconnectedPlayers);
		m_IteratingNowInDisconnectedPlayers = true;

		auto it = std::find_if(m_NetPlayersDisconnected.begin(), m_NetPlayersDisconnected.end(), predicate);
		IClient *clResult = nullptr;

		if (it != m_NetPlayersDisconnected.end())
		{
			clResult = *it;
			m_NetPlayersDisconnected.erase(it);
		}

		m_IteratingNowInDisconnectedPlayers = false;
		csPlayers.Leave();
		return clResult;
	}

	template <typename SearchPredicate>
	IClient *GetFoundDisconnectedClient(SearchPredicate predicate)
	{
		csPlayers.Enter();

		m_IteratingNowInDisconnectedPlayers = true;
		auto it = std::find_if(m_NetPlayersDisconnected.begin(), m_NetPlayersDisconnected.end(), predicate);
		m_IteratingNowInDisconnectedPlayers = false;
		IClient *clResult = nullptr;

		if (it != m_NetPlayersDisconnected.end())
			clResult = *it;
		
		csPlayers.Leave();
		return clResult;
	}

	void AddNewDisconnectedClient(IClient *new_client)
	{
		csPlayers.Enter();
		VERIFY(!m_IteratingNowInDisconnectedPlayers);
		m_NetPlayersDisconnected.push_back(new_client);
		csPlayers.Leave();
	}

	u32 ClientsCount()
	{
		csPlayers.Enter();
		u32 ret_count = m_NetPlayers.size();
		csPlayers.Leave();
		return ret_count;
	}

	//WARNING! for iteration in vector use ForEachClientDo !
	//This function can be used carefully (single call)
	/*IClient*	GetClientByIndex				(u32 index)
	{
		csPlayers.Enter();
		IClient* ret_client = m_NetPlayers[index];
		csPlayers.Leave();
		return ret_client;
	}*/
};
