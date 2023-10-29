/**
 * Crash Stats
 * Copyright (C) 2023 Soruce2ZE
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <vector>
#include "crashstats.h"
#include "iserver.h"
#include "httpmanager.h"
#include <msgpack.hpp>

#define NETWORK_DISCONNECT_TIMEDOUT 29
#define API_URL	"IMPLEMENT"

SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);
SH_DECL_HOOK5_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, CPlayerSlot, int, const char *, uint64, const char *);
SH_DECL_HOOK0_void(IServerGameDLL, GameServerSteamAPIActivated, SH_NOATTRIB, 0);
SH_DECL_HOOK0_void(IServerGameDLL, GameServerSteamAPIDeactivated, SH_NOATTRIB, 0);

CrashStats g_CrashStats;
IServerGameDLL *server = NULL;
IServerGameClients *gameclients = NULL;

ISteamHTTP* g_http = nullptr;
CSteamGameServerAPIContext g_steamAPI;

std::vector<std::tuple<std::string, uint64, time_t>> g_vecCrashes;

void SendCrashReport()
{
	if (!g_vecCrashes.size())
		return;

	for (auto crash : g_vecCrashes)
	{
		ConMsg("Reporting %lli crash\n", std::get<1>(crash));
	}

	msgpack::sbuffer sbuf;
	msgpack::pack(sbuf, g_vecCrashes);

	g_HTTPManager.POSTRAW(API_URL, "application/msgpack", (uint8*)sbuf.data(), sbuf.size(), [](HTTPRequestHandle, char*) {
	});

	g_vecCrashes.clear();
}

PLUGIN_EXPOSE(CrashStats, g_CrashStats);
bool CrashStats::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_ANY(GetServerFactory, server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
	GET_V_IFACE_ANY(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_ANY(GetServerFactory, g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);

	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameFrame, server, this, &CrashStats::Hook_GameFrame, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, gameclients, this, &CrashStats::Hook_ClientDisconnect, true);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameServerSteamAPIActivated, g_pSource2Server, this, &CrashStats::Hook_GameServerSteamAPIActivated, false);
	SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameServerSteamAPIDeactivated, g_pSource2Server, this, &CrashStats::Hook_GameServerSteamAPIDeactivated, false);

	return true;
}

void CrashStats::Hook_GameServerSteamAPIActivated()
{
	g_steamAPI.Init();
	g_http = g_steamAPI.SteamHTTP();

	RETURN_META(MRES_IGNORED);
}

void CrashStats::Hook_GameServerSteamAPIDeactivated()
{
	g_http = nullptr;

	RETURN_META(MRES_IGNORED);
}

bool CrashStats::Unload(char *error, size_t maxlen)
{
	SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, GameFrame, server, this, &CrashStats::Hook_GameFrame, true);
	SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, gameclients, this, &CrashStats::Hook_ClientDisconnect, true);

	g_vecCrashes.clear();
	return true;
}

void CrashStats::AllPluginsLoaded()
{
}

void CrashStats::Hook_ClientDisconnect( CPlayerSlot slot, int reason, const char *pszName, uint64 xuid, const char *pszNetworkID )
{
	if (reason == NETWORK_DISCONNECT_TIMEDOUT)
		g_vecCrashes.push_back(std::tuple<std::string, uint64, time_t>(std::string(pszName), xuid, std::time(0)));
}

void CrashStats::Hook_GameFrame( bool simulating, bool bFirstTick, bool bLastTick )
{
	static uint16_t ticks;
	ticks++;

	if (ticks >= (64 * 60)) // once per minute
	{
		SendCrashReport();
		ticks = 0;
	}
}

// Potentially might not work
void CrashStats::OnLevelInit( char const *pMapName,
									 char const *pMapEntities,
									 char const *pOldLevel,
									 char const *pLandmarkName,
									 bool loadGame,
									 bool background )
{
}

// Potentially might not work
void CrashStats::OnLevelShutdown()
{
}

bool CrashStats::Pause(char *error, size_t maxlen)
{
	return true;
}

bool CrashStats::Unpause(char *error, size_t maxlen)
{
	return true;
}

const char *CrashStats::GetLicense()
{
	return "GPLv3";
}

const char *CrashStats::GetVersion()
{
	return "1.0.0.0";
}

const char *CrashStats::GetDate()
{
	return __DATE__;
}

const char *CrashStats::GetLogTag()
{
	return "CrashStats";
}

const char *CrashStats::GetAuthor()
{
	return "Poggu";
}

const char *CrashStats::GetDescription()
{
	return "Monitors the amount of client crashes";
}

const char *CrashStats::GetName()
{
	return "Crash Stats";
}

const char *CrashStats::GetURL()
{
	return "https://poggu.me";
}
