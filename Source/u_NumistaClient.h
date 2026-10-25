//---------------------------------------------------------------------------
// Клиент Numista API v3 (https://api.numista.com/v3).
// Документация: https://en.numista.com/api/doc/index.php
// Спецификация: https://en.numista.com/api/doc/swagger.yaml
//
// Ключ — заголовок Numista-API-Key; доступ к своей коллекции — OAuth
// "client credentials" (getOAuthToken), токен — Authorization: Bearer.
// Ключ, токен и счётчик запросов за месяц хранятся в AppConfig() ([Numista]).
//---------------------------------------------------------------------------
#ifndef u_NumistaClientH
#define u_NumistaClientH
//---------------------------------------------------------------------------
#include <System.hpp>
#include <System.SysUtils.hpp>
#include <System.JSON.hpp>
#include <memory>
//---------------------------------------------------------------------------
// Ошибка обращения к Numista; StatusCode — код HTTP (0 — сеть / нет ответа).
class ENumistaError : public Exception
{
public:
	int StatusCode;
	__fastcall ENumistaError(int AStatusCode, const String &AMessage)
		: Exception(AMessage), StatusCode(AStatusCode) {}
};
//---------------------------------------------------------------------------
class TNumistaClient
{
public:
	static const wchar_t *const BaseUrl;	// https://api.numista.com/v3

	TNumistaClient();
	~TNumistaClient();

	// Токен своей коллекции (OAuth → getOAuthToken, grant_type=client_credentials,
	// scope=view_collection,edit_collection). Действующий токен повторно не
	// запрашивается; AForce — запросить заново.
	void Authorize(bool AForce = false);
	static bool HasValidToken();

	// Справочники Numista с кэшем в <каталог exe>\Numista\ (Catalogue → getIssuers,
	// getCatalogues). Без ARefresh берутся из кэша, запрос — только если кэша нет.
	// Возвращают число записей.
	int LoadIssuers(bool ARefresh);
	int LoadCatalogues(bool ARefresh);
	// Число записей в кэше; -1 — кэша нет.
	static int CachedCount(const String &AName);
	static String CacheDir();

	// Запросов к API за текущий месяц (по счётчику программы).
	static int RequestsThisMonth();

private:
	// GET BaseUrl + APath; AWithToken — добавить Authorization: Bearer.
	// Не 2xx — ENumistaError с понятным текстом. Возвращает тело ответа.
	String Get(const String &APath, bool AWithToken);
	int    LoadList(const String &AName, const String &APath, const String &AArrayName, bool ARefresh);
	static void CountRequest();
	static std::unique_ptr<TJSONValue> ParseJson(const String &AText);
};
//---------------------------------------------------------------------------
#endif
