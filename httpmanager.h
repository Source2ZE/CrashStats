#pragma once
/**
 * Crash Stats
 * Copyright (C) 2023 Poggu
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

#include "crashstats.h"
#include <steam/steam_gameserver.h>

#include <vector>
#include <functional>

class HTTPManager;
extern HTTPManager g_HTTPManager;

#define CompletedCallback std::function<void(HTTPRequestHandle, char*)>

class HTTPManager
{
public:
	void GET(const char* pszUrl, CompletedCallback callback);
	void POST(const char* pszUrl, const char* pszText, CompletedCallback callback);
	void POSTRAW(const char* pszUrl, const char* contentType, uint8* bytes, int size, CompletedCallback callback);
	bool HasAnyPendingRequests() const { return m_PendingRequests.size() > 0; }

private:
	class TrackedRequest
	{
	public:
		TrackedRequest(const TrackedRequest& req) = delete;
		TrackedRequest(HTTPRequestHandle hndl, SteamAPICall_t hCall, const char* pszUrl, const char* pszText, CompletedCallback callback);
		~TrackedRequest();
	private:
		void OnHTTPRequestCompleted(HTTPRequestCompleted_t* arg, bool bFailed);

		HTTPRequestHandle m_hHTTPReq;
		CCallResult<TrackedRequest, HTTPRequestCompleted_t> m_CallResult;
		char* m_pszUrl;
		char* m_pszText;
		CompletedCallback m_callback;
	};
private:
	std::vector<HTTPManager::TrackedRequest*> m_PendingRequests;
	void GenerateRequest(EHTTPMethod method, const char* pszUrl, const char* pszText, CompletedCallback callback);
	void GenerateRequestRaw(EHTTPMethod method, const char* pszUrl, const char* contentType, uint8* bytes, int size, CompletedCallback callback);
};