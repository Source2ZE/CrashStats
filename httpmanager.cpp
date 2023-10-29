/**
 * Crash Stats
 * Copyright (C) 2023 Poggu
 * Original code from D2Lobby2
 * Copyright (C) 2023 Nicholas Hastings
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

#include "httpmanager.h"
//#include "common.h"
#include <string>

extern ISteamHTTP* g_http;

HTTPManager g_HTTPManager;

#undef strdup

HTTPManager::TrackedRequest::TrackedRequest(HTTPRequestHandle hndl, SteamAPICall_t hCall, const char* pszUrl, const char* pszText, CompletedCallback callback)
{
	m_hHTTPReq = hndl;
	m_CallResult.SetGameserverFlag();
	m_CallResult.Set(hCall, this, &TrackedRequest::OnHTTPRequestCompleted);

	m_pszUrl = strdup(pszUrl);
	m_pszText = strdup(pszText);
	m_callback = callback;

	g_HTTPManager.m_PendingRequests.push_back(this);
}

HTTPManager::TrackedRequest::~TrackedRequest()
{
	for (auto e = g_HTTPManager.m_PendingRequests.begin(); e != g_HTTPManager.m_PendingRequests.end(); ++e)
	{
		if (*e == this)
		{
			g_HTTPManager.m_PendingRequests.erase(e);
			break;
		}
	}

	free(m_pszUrl);
	free(m_pszText);
}

void HTTPManager::TrackedRequest::OnHTTPRequestCompleted(HTTPRequestCompleted_t* arg, bool bFailed)
{
	if (bFailed || arg->m_eStatusCode < 200 || arg->m_eStatusCode > 299)
	{
		ConColorMsg(Color(255, 0, 255, 255), "HTTP request to %s failed with status code %i\n", m_pszUrl, arg->m_eStatusCode);
	}
	else
	{
		uint32 size;
		g_http->GetHTTPResponseBodySize(arg->m_hRequest, &size);

		uint8* response = new uint8[size];
		g_http->GetHTTPResponseBodyData(arg->m_hRequest, response, size);

		// Pass on response to the custom callback
		m_callback(arg->m_hRequest, (char*)response);
	}

	if (g_http)
		g_http->ReleaseHTTPRequest(arg->m_hRequest);

	delete this;
}

void HTTPManager::GET(const char* pszUrl, CompletedCallback callback)
{
	GenerateRequest(k_EHTTPMethodGET, pszUrl, "", callback);
}

void HTTPManager::POST(const char* pszUrl, const char* pszText, CompletedCallback callback)
{
	GenerateRequest(k_EHTTPMethodPOST, pszUrl, pszText, callback);
}

void HTTPManager::POSTRAW(const char* pszUrl, const char* contentType, uint8* bytes, int size, CompletedCallback callback)
{
	GenerateRequestRaw(k_EHTTPMethodPOST, pszUrl, contentType, bytes, size, callback);
}

void HTTPManager::GenerateRequest(EHTTPMethod method, const char* pszUrl, const char* pszText, CompletedCallback callback)
{
	//Message("Sending HTTP:\n%s\n", pszText);
	auto hReq = g_http->CreateHTTPRequest(method, pszUrl);
	int size = strlen(pszText);
	//Message("HTTP request: %p\n", hReq);

	if (method == k_EHTTPMethodPOST && !g_http->SetHTTPRequestRawPostBody(hReq, "application/json", (uint8*)pszText, size))
	{
		//Message("Failed to SetHTTPRequestRawPostBody\n");
		return;
	}

	// Prevent HTTP error 411 (probably not necessary?)
	//g_http->SetHTTPRequestHeaderValue(hReq, "Content-Length", std::to_string(size).c_str());

	SteamAPICall_t hCall;
	g_http->SendHTTPRequest(hReq, &hCall);

	new TrackedRequest(hReq, hCall, pszUrl, pszText, callback);
}

void HTTPManager::GenerateRequestRaw(EHTTPMethod method, const char* pszUrl, const char* contentType, uint8* bytes, int size, CompletedCallback callback)
{
	//Message("Sending HTTP:\n%s\n", pszText);
	auto hReq = g_http->CreateHTTPRequest(method, pszUrl);
	//Message("HTTP request: %p\n", hReq);

	if (method == k_EHTTPMethodPOST && !g_http->SetHTTPRequestRawPostBody(hReq, contentType, bytes, size))
	{
		return;
	}

	// Prevent HTTP error 411 (probably not necessary?)
	//g_http->SetHTTPRequestHeaderValue(hReq, "Content-Length", std::to_string(size).c_str());

	SteamAPICall_t hCall;
	g_http->SendHTTPRequest(hReq, &hCall);

	new TrackedRequest(hReq, hCall, pszUrl, (char*)bytes, callback);
}