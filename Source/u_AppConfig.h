//---------------------------------------------------------------------------
// Настройки приложения. Хранение — CoinsCollection.ini рядом с exe.
// Один экземпляр на приложение: AppConfig(); загружается в _CoinsCollection.cpp.
//---------------------------------------------------------------------------
#ifndef u_AppConfigH
#define u_AppConfigH
//---------------------------------------------------------------------------
#include <System.hpp>
//---------------------------------------------------------------------------
class TAppConfig
{
public:
	TAppConfig();
	~TAppConfig();

	// Загрузить / сохранить настройки из / в INI рядом с exe.
	void Load();
	void Save();

	// Путь к файлу БД SQLite (абсолютный после Load()).
	String DbPath;
	// Язык интерфейса: en / ru / uk ([UI] Language; по умолчанию en).
	String Language;
	// Папка фото монет с завершающим "\" ([Photos] Path; по умолчанию <каталог exe>\Photos\).
	String PhotosPath;

	// Numista ([Numista]). Ключ и токен хранятся в ini (в git не попадает).
	String    NumistaApiKey;
	String    NumistaToken;          // OAuth access token (client credentials)
	TDateTime NumistaTokenExpires;   // 0 — нет токена
	int       NumistaUserId;
	String    NumistaRequestsMonth;  // 'yyyy-mm', к которому относится счётчик
	int       NumistaRequestsCount;  // запросов к API за этот месяц
	int       NumistaMonthlyQuota;   // квота ключа в месяц (по умолчанию 2000)
	bool      NumistaAutoLinkDetails; // автосвязь сразу загружает характеристики и фото типа (задача 17c)

	// Каталог exe с завершающим "\".
	static String ExeDir();

private:
	String FDbPathInIni;	// значение [Database] Path как записано (может быть относительным)
	String FPhotosPathInIni;	// значение [Photos] Path как записано ("" — по умолчанию)

	static String IniFileName();
	static String DefaultDbPath();
	static String ResolveDbPath(const String &APath);
};
//---------------------------------------------------------------------------
TAppConfig &AppConfig();
//---------------------------------------------------------------------------
#endif
