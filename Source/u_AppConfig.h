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

	// Каталог exe с завершающим "\".
	static String ExeDir();

private:
	String FDbPathInIni;	// значение [Database] Path как записано (может быть относительным)

	static String IniFileName();
	static String DefaultDbPath();
	static String ResolveDbPath(const String &APath);
};
//---------------------------------------------------------------------------
TAppConfig &AppConfig();
//---------------------------------------------------------------------------
#endif
