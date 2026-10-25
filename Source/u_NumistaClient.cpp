//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <System.Net.HttpClient.hpp>
#include <System.Net.URLClient.hpp>
#include <System.IOUtils.hpp>
#include "u_NumistaClient.h"
#include "u_AppConfig.h"
#include "u_Translator.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
//---------------------------------------------------------------------------

namespace
{
	const int cConnectTimeoutMs  = 15000;
	const int cResponseTimeoutMs = 30000;

	String CurrentMonth()
	{
		return FormatDateTime(L"yyyy-mm", Now());
	}
	//-----------------------------------------------------------------------
	// Текст ошибки из ответа Numista: {"error_message": "..."}.
	String ErrorMessageOf(const String &ABody)
	{
		std::unique_ptr<TJSONValue> json(TJSONObject::ParseJSONValue(ABody));
		if (!json)
			return String();
		TJSONValue *message = json->FindValue(L"error_message");
		return message ? message->Value() : String();
	}
	//-----------------------------------------------------------------------
	String JsonString(TJSONValue *AJson, const String &AName)
	{
		TJSONValue *value = AJson ? AJson->FindValue(AName) : nullptr;
		return value ? value->Value() : String();
	}
}
//---------------------------------------------------------------------------
const wchar_t *const TNumistaClient::BaseUrl = L"https://api.numista.com/v3";
//---------------------------------------------------------------------------
TNumistaClient::TNumistaClient()
{
}
//---------------------------------------------------------------------------
TNumistaClient::~TNumistaClient()
{
}
//---------------------------------------------------------------------------
// Счётчик запросов за месяц: новый месяц — счёт с нуля.
void TNumistaClient::CountRequest()
{
	TAppConfig &config = AppConfig();
	if (config.NumistaRequestsMonth != CurrentMonth())
	{
		config.NumistaRequestsMonth = CurrentMonth();
		config.NumistaRequestsCount = 0;
	}
	++config.NumistaRequestsCount;
	config.Save();
}
//---------------------------------------------------------------------------
int TNumistaClient::RequestsThisMonth()
{
	const TAppConfig &config = AppConfig();
	return config.NumistaRequestsMonth == CurrentMonth() ? config.NumistaRequestsCount : 0;
}
//---------------------------------------------------------------------------
std::unique_ptr<TJSONValue> TNumistaClient::ParseJson(const String &AText)
{
	std::unique_ptr<TJSONValue> json(TJSONObject::ParseJSONValue(AText));
	if (!json)
		throw ENumistaError(0, Tr(L"Numista.Error.Json"));
	return json;
}
//---------------------------------------------------------------------------
String TNumistaClient::Get(const String &APath, bool AWithToken)
{
	const TAppConfig &config = AppConfig();
	if (config.NumistaApiKey.IsEmpty())
		throw ENumistaError(0, Tr(L"Numista.Error.NoKey"));

	TNetHeaders headers;
	headers.Length = AWithToken ? 2 : 1;
	headers[0] = TNameValuePair(L"Numista-API-Key", config.NumistaApiKey);
	if (AWithToken)
		headers[1] = TNameValuePair(L"Authorization", L"Bearer " + config.NumistaToken);

	std::unique_ptr<THTTPClient> http(THTTPClient::Create());
	http->ConnectionTimeout = cConnectTimeoutMs;
	http->ResponseTimeout   = cResponseTimeoutMs;
	http->Accept            = L"application/json";

	CountRequest();	// запрос уходит — он учитывается в квоте, даже если вернётся ошибка
	_di_IHTTPResponse response;
	try
	{
		response = http->Get(String(BaseUrl) + APath, nullptr, headers);
	}
	catch (Exception &e)
	{
		throw ENumistaError(0, Tr(L"Numista.Error.Network") + L" " + e.Message);
	}

	const int code = response->StatusCode;
	const String body = response->ContentAsString(TEncoding::UTF8);
	if (code >= 200 && code < 300)
		return body;

	const String message = ErrorMessageOf(body);
	switch (code)
	{
		case 401: throw ENumistaError(code, Tr(L"Numista.Error.Unauthorized"));
		case 429: throw ENumistaError(code, Tr(L"Numista.Error.Quota"));
	}
	throw ENumistaError(code, Format(Tr(L"Numista.Error.Http"),
		ARRAYOFCONST((code, message.IsEmpty() ? response->StatusText : message))));
}
//---------------------------------------------------------------------------
bool TNumistaClient::HasValidToken()
{
	const TAppConfig &config = AppConfig();
	return !config.NumistaToken.IsEmpty() && config.NumistaUserId != 0
		&& (double)config.NumistaTokenExpires > (double)(Now() + 1.0 / 24.0);	// запас — час
}
//---------------------------------------------------------------------------
void TNumistaClient::Authorize(bool AForce)
{
	if (!AForce && HasValidToken())
		return;

	const String body = Get(L"/oauth_token?grant_type=client_credentials&scope=view_collection,edit_collection", false);
	std::unique_ptr<TJSONValue> json = ParseJson(body);
	const String token = JsonString(json.get(), L"access_token");
	const int expiresIn = StrToIntDef(JsonString(json.get(), L"expires_in"), 0);
	const int userId = StrToIntDef(JsonString(json.get(), L"user_id"), 0);
	if (token.IsEmpty() || userId == 0)
		throw ENumistaError(0, Tr(L"Numista.Error.Json"));

	TAppConfig &config = AppConfig();
	config.NumistaToken        = token;
	config.NumistaUserId       = userId;
	config.NumistaTokenExpires = Now() + expiresIn / 86400.0;
	config.Save();
}
//---------------------------------------------------------------------------
String TNumistaClient::CacheDir()
{
	return TAppConfig::ExeDir() + L"Numista\\";
}
//---------------------------------------------------------------------------
// Кэш — ответ API как есть (<name>.json); число записей — по полю "count"
// или по длине массива.
int TNumistaClient::CachedCount(const String &AName)
{
	const String file = CacheDir() + AName + L".json";
	if (!FileExists(file))
		return -1;
	try
	{
		std::unique_ptr<TJSONValue> json(TJSONObject::ParseJSONValue(TFile::ReadAllText(file, TEncoding::UTF8)));
		if (!json)
			return -1;
		const int count = StrToIntDef(JsonString(json.get(), L"count"), -1);
		if (count >= 0)
			return count;
		TJSONObject *object = dynamic_cast<TJSONObject *>(json.get());
		for (int i = 0; object && i < object->Count; ++i)
			if (TJSONArray *array = dynamic_cast<TJSONArray *>(object->Pairs[i]->JsonValue))
				return array->Count;
	}
	catch (Exception &)
	{
	}
	return -1;
}
//---------------------------------------------------------------------------
int TNumistaClient::LoadList(const String &AName, const String &APath, const String &AArrayName, bool ARefresh)
{
	const int cached = CachedCount(AName);
	if (!ARefresh && cached >= 0)
		return cached;

	const String body = Get(APath, false);
	std::unique_ptr<TJSONValue> json = ParseJson(body);
	if (!json->FindValue(AArrayName))
		throw ENumistaError(0, Tr(L"Numista.Error.Json"));

	ForceDirectories(CacheDir());
	TFile::WriteAllText(CacheDir() + AName + L".json", body, TEncoding::UTF8);
	return CachedCount(AName);
}
//---------------------------------------------------------------------------
int TNumistaClient::LoadIssuers(bool ARefresh)
{
	return LoadList(L"issuers", L"/issuers?lang=en", L"issuers", ARefresh);
}
//---------------------------------------------------------------------------
int TNumistaClient::LoadCatalogues(bool ARefresh)
{
	return LoadList(L"catalogues", L"/catalogues", L"catalogues", ARefresh);
}
//---------------------------------------------------------------------------
