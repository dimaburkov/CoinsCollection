//---------------------------------------------------------------------------
// Настройки приложения. Хранение — CoinsCollection.ini рядом с exe.
// Один экземпляр на приложение: AppConfig(); загружается в _CoinsCollection.cpp.
//---------------------------------------------------------------------------
#ifndef u_AppConfigH
#define u_AppConfigH
//---------------------------------------------------------------------------
#include <System.hpp>
#include <Vcl.Forms.hpp>
#include <map>
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
	bool      NumistaLoadTypePhotos;  // загружать фото типа из Numista (задача 33)
	// Цены Numista (задача 34): включены, валюта (UAH / USD / EUR), обновлять старше N дней.
	bool      NumistaLoadPrices;
	String    NumistaPriceCurrency;
	int       NumistaPriceDays;
	bool      NumistaPricesWithDetails; // загружать цены вместе с характеристиками (задача 34a; ini PriceMode = 1)
	// Поля примірника, отправляемые на Numista при выгрузке (задача 33).
	bool      NumistaUploadGrade;
	bool      NumistaUploadPrice;
	bool      NumistaUploadDate;
	bool      NumistaUploadComment;

	// Список монет ([Grid], задача 41): скрытые пользователем колонки — ключи через запятую
	// (по умолчанию "obverse,reverse"), высота строк в пикселях (18 / 40 / 64).
	String    GridHidden;
	int       GridRowHeight;
	// Порядок колонок (задача 42): ключи через запятую; пусто — по умолчанию.
	String    GridOrder;
	// Ширины колонок, заданные мышью ([Grid] Widths = "ключ:ширина,...", задача 44).
	std::map<String, int> GridWidths;

	// Главное окно ([Window], задача 44): обычные (не развёрнутые) положение и размер на экране
	// (пустой — не сохранено), развёрнуто, ширина дерева и высота подробностей (0 — по умолчанию).
	// Размеры здесь — при текущем масштабе экрана: записанные при другом ([Window] PixelsPerInch)
	// пересчитываются при загрузке; так же [Windows] и [Grid] Widths.
	TRect     WindowBounds;
	bool      WindowMaximized;
	int       WindowTreeWidth;
	int       WindowDetailsHeight;
	// Размеры окон с таблицами ([Windows] Имя=ШиринаxВысота, задача 44).
	std::map<String, TPoint> FormSizes;

	// Окно с таблицей: до показа — размер из [Windows] (не меньше Constraints, не больше рабочей
	// области); после закрытия — запомнить обычный размер и сохранить ini.
	void RestoreFormSize(TForm *AForm, const String &AName);
	void StoreFormSize(TForm *AForm, const String &AName);
	// Обычные (не развёрнутые) границы окна на экране; AMaximized — развёрнуто (и свёрнутое из развёрнутого).
	static TRect NormalBounds(TForm *AForm, bool *AMaximized = nullptr);

	// Резервные копии ([Backup], задача 21): папка последней копии, дата последней копии и
	// последнего напоминания (0 — не было).
	String    BackupFolder;
	TDateTime BackupLast;
	TDateTime BackupAsked;

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
