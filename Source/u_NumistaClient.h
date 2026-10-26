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
	static const int BudgetExhausted = -1;	// ENumistaError::StatusCode: лимит RequestBudget исчерпан

	// Лимит запросов в сеть для этого клиента (-1 — без лимита) и сколько уже отправлено.
	int RequestBudget;
	int RequestsMade;

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

	// GET каталога (searchTypes, getIssues) с кэшем ответов в CacheDir()\cache	// (<md5 адреса>.json). Ответ из кэша квоту не тратит; AFromCache — взят из кэша.
	std::unique_ptr<TJSONValue> GetCached(const String &APath, bool *AFromCache = nullptr);
	// Ответ на APath уже есть в кэше (GetCached не потратит запрос).
	static bool IsCached(const String &APath);

	// Своя коллекция (OAuth: сначала Authorize).
	// getCollectedItems — GET /users/{user_id}/collected_items: вся коллекция одним ответом.
	std::unique_ptr<TJSONValue> GetCollectedItems();
	// addCollectedItems — POST /users/{user_id}/collected_items, тело — JSON.
	// Возвращает id добавленного предмета.
	int AddCollectedItem(const String &AJson);

	// Запросов к API за текущий месяц (по счётчику программы).
	static int RequestsThisMonth();

private:
	// GET BaseUrl + APath; AWithToken — добавить Authorization: Bearer.
	// Не 2xx — ENumistaError с понятным текстом. Возвращает тело ответа.
	String Get(const String &APath, bool AWithToken);
	// Общая часть GET / POST: ABody не пустое — POST с JSON.
	String Send(const String &APath, bool AWithToken, const String &ABody);
	int    LoadList(const String &AName, const String &APath, const String &AArrayName, bool ARefresh);
	static void CountRequest();
	static std::unique_ptr<TJSONValue> ParseJson(const String &AText);
};
//---------------------------------------------------------------------------
#endif
