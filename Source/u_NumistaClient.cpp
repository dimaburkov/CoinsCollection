//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <System.Net.HttpClient.hpp>
#include <System.Net.URLClient.hpp>
#include <System.IOUtils.hpp>
#include <System.Hash.hpp>
#include "u_NumistaClient.h"
#include "u_AppConfig.h"
#include "u_Translator.h"
#include "versionConfig.h"
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
	: RequestBudget(-1),
	  RequestsMade(0)
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
	return Send(L"GET", APath, AWithToken, String());
}
//---------------------------------------------------------------------------
String TNumistaClient::Send(const String &AMethod, const String &APath, bool AWithToken, const String &ABody)
{
	const TAppConfig &config = AppConfig();
	if (config.NumistaApiKey.IsEmpty())
		throw ENumistaError(0, Tr(L"Numista.Error.NoKey"));
	if (RequestBudget >= 0 && RequestsMade >= RequestBudget)
		throw ENumistaError(BudgetExhausted, Tr(L"Numista.Error.Budget"));

	const bool withBody = !ABody.IsEmpty();
	TNetHeaders headers;
	headers.Length = 1 + (AWithToken ? 1 : 0);
	int h = 0;
	headers[h++] = TNameValuePair(L"Numista-API-Key", config.NumistaApiKey);
	if (AWithToken)
		headers[h++] = TNameValuePair(L"Authorization", L"Bearer " + config.NumistaToken);

	std::unique_ptr<THTTPClient> http(THTTPClient::Create());
	http->ConnectionTimeout = cConnectTimeoutMs;
	http->ResponseTimeout   = cResponseTimeoutMs;
	http->UserAgent         = UserAgent();
	http->Accept            = L"application/json";
	if (withBody)
		http->ContentType = L"application/json";

	CountRequest();	// запрос уходит — он учитывается в квоте, даже если вернётся ошибка
	++RequestsMade;
	_di_IHTTPResponse response;
	try
	{
		const String url = String(BaseUrl) + APath;
		std::unique_ptr<TStringStream> body(new TStringStream(ABody, TEncoding::UTF8, false));
		if (AMethod == L"POST")
			response = http->Post(url, body.get(), nullptr, headers);
		else if (AMethod == L"PATCH")
			response = http->Patch(url, body.get(), nullptr, headers);
		else if (AMethod == L"DELETE")
			response = http->Delete(url, nullptr, headers);
		else
			response = http->Get(url, nullptr, headers);
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
bool TNumistaClient::IsCached(const String &APath)
{
	return FileExists(CacheDir() + L"cache\\" + THashMD5::GetHashString(APath) + L".json");
}
//---------------------------------------------------------------------------
std::unique_ptr<TJSONValue> TNumistaClient::GetCached(const String &APath, bool *AFromCache, bool ARefresh)
{
	const String dir  = CacheDir() + L"cache\\";
	const String file = dir + THashMD5::GetHashString(APath) + L".json";
	if (!ARefresh && FileExists(file))
	{
		std::unique_ptr<TJSONValue> json(TJSONObject::ParseJSONValue(TFile::ReadAllText(file, TEncoding::UTF8)));
		if (json)
		{
			if (AFromCache)
				*AFromCache = true;
			return json;
		}
	}

	const String body = Get(APath, false);
	std::unique_ptr<TJSONValue> json = ParseJson(body);
	ForceDirectories(dir);
	TFile::WriteAllText(file, body, TEncoding::UTF8);
	if (AFromCache)
		*AFromCache = false;
	return json;
}
//---------------------------------------------------------------------------
TGraphic *TNumistaClient::DownloadImage(const String &AUrl)
{
	// Только сайт Numista по https: ссылка пришла из ответа API, но в чужой адрес не ходим.
	const String prefix = L"https://";
	String host = SameText(AUrl.SubString(1, prefix.Length()), prefix) ? AUrl.SubString(prefix.Length() + 1, AUrl.Length()) : String();
	if (host.Pos(L"/") > 0)
		host = host.SubString(1, host.Pos(L"/") - 1);
	host = host.LowerCase();
	const String domain = L".numista.com";
	if (!(host == L"numista.com" || (host.Length() > domain.Length()
		&& host.SubString(host.Length() - domain.Length() + 1, domain.Length()) == domain)))
		throw ENumistaError(0, Tr(L"Numista.Error.ImageUrl") + L" " + AUrl);

	std::unique_ptr<THTTPClient> http(THTTPClient::Create());
	http->ConnectionTimeout = cConnectTimeoutMs;
	http->ResponseTimeout   = cResponseTimeoutMs;
	http->UserAgent         = UserAgent();
	std::unique_ptr<TMemoryStream> data(new TMemoryStream());
	_di_IHTTPResponse response;
	try
	{
		response = http->Get(AUrl, data.get());
	}
	catch (Exception &e)
	{
		throw ENumistaError(0, Tr(L"Numista.Error.Network") + L" " + e.Message);
	}
	if (response->StatusCode < 200 || response->StatusCode >= 300)
		throw ENumistaError(response->StatusCode, Format(Tr(L"Numista.Error.Image"),
			ARRAYOFCONST((response->StatusCode, AUrl))));

	data->Position = 0;
	std::unique_ptr<TWICImage> image(new TWICImage());
	try
	{
		image->LoadFromStream(data.get());
	}
	catch (Exception &)
	{
		throw ENumistaError(0, Tr(L"Numista.Error.ImageUrl") + L" " + AUrl);
	}
	return image.release();
}
//---------------------------------------------------------------------------
String TNumistaClient::UserAgent()
{
	return AppInfo::Name + L"/" + AppInfo::Version + L" (uCoin to Numista migration tool)";
}
//---------------------------------------------------------------------------
std::unique_ptr<TJSONValue> TNumistaClient::GetCollectedItems()
{
	const String body = Get(L"/users/" + IntToStr(AppConfig().NumistaUserId) + L"/collected_items", true);
	std::unique_ptr<TJSONValue> json = ParseJson(body);
	if (!dynamic_cast<TJSONArray *>(json->FindValue(L"items")))
		throw ENumistaError(0, Tr(L"Numista.Error.Json"));
	return json;
}
//---------------------------------------------------------------------------
int TNumistaClient::AddCollectedItem(const String &AJson)
{
	const String body = Send(L"POST", L"/users/" + IntToStr(AppConfig().NumistaUserId) + L"/collected_items", true, AJson);
	std::unique_ptr<TJSONValue> json = ParseJson(body);
	const int id = StrToIntDef(JsonString(json.get(), L"id"), 0);
	if (id == 0)
		throw ENumistaError(0, Tr(L"Numista.Error.Json"));
	return id;
}
//---------------------------------------------------------------------------
void TNumistaClient::EditCollectedItemType(int AItemId, int ATypeId, int AIssueId)
{
	const String json = L"{\"type\":" + IntToStr(ATypeId) + L",\"issue\":"
		+ (AIssueId != 0 ? IntToStr(AIssueId) : String(L"null")) + L"}";
	Send(L"PATCH", L"/users/" + IntToStr(AppConfig().NumistaUserId) + L"/collected_items/" + IntToStr(AItemId), true, json);
}
//---------------------------------------------------------------------------
void TNumistaClient::DeleteCollectedItem(int AItemId)
{
	try
	{
		Send(L"DELETE", L"/users/" + IntToStr(AppConfig().NumistaUserId) + L"/collected_items/" + IntToStr(AItemId),
			true, String());
	}
	catch (ENumistaError &e)
	{
		if (e.StatusCode != 404)
			throw;
	}
}
//---------------------------------------------------------------------------
