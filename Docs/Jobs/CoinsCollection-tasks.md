# CoinsCollection — Задачи

Проект: локальный учёт коллекции монет в SQLite с импортом из файла .xlsx (экспорт uCoin.net) и последующей выгрузкой на Numista через API.
Среда разработки: RAD Studio 10.4 (позже 13), C++Builder / VCL.

Порядок работ: каркас → БД → импорт из .xlsx → Numista API.
Каждая задача рассчитана на 1–2 часа. На задачу заводится отдельная ветка от master, по завершении — merge в master.

---

## Задача 1. Пустой проект и папки

Статус: выполнено (ветка 1-folders, коммит «1 - Создан пустой проект и 3 папки»).

Создан пустой проект C++Builder VCL и папки Docs, Exe, Source.

---

## Задача 2. Каркас файлов приложения

### Цель

Создать структуру модулей приложения (пустые классы, заглушки методов, связи между модулями), чтобы проект собирался и запускался. Без бизнес-логики.

### Ветка

2-skeleton от master; по завершении — merge в master. Коммит: «2 - Каркас файлов».

### Объём работ

**Немедийные модули — Source/ (плоская раскладка)**

Для каждого модуля: header guard, объявление класса, заглушки методов (тело `// TODO`, возвращают пустые значения).

| Файлы | Содержимое |
|---|---|
| u_CoinTypes.h | Структура TCoinRecord со всеми полями модели (таблица ниже) |
| u_CoinCollection.h/.cpp | TCoinCollection: контейнер std::vector<TCoinRecord> + методы Add / Update / Remove / Find / Count / Clear |
| dm_Data.h/.cpp/.dfm | DataModule TdmData: на форме TFDConnection, TFDTransaction, TFDQuery (без настройки параметров подключения) |
| u_Database.h/.cpp | TDatabase: Open() / Close() / EnsureSchema() / SchemaVersion() |
| u_CoinRepository.h/.cpp | TCoinRepository: LoadAll() / Save(TCoinRecord&) / Delete(int Id) |
| u_ImportSource.h/.cpp | TXlsxSource: Open(String) / RowCount() / Row(int) -> TArray<String> |
| u_CoinImporter.h/.cpp | TCoinImporter: Parse(TXlsxSource&) -> TArray<TCoinRecord> + список ошибок |
| u_NumistaClient.h/.cpp | TNumistaClient: пустой класс, `// TODO шаг 4` |
| u_AppConfig.h/.cpp | TAppConfig: Load() / Save(), поле DbPath |
| versionConfig.h | Константы APP_NAME, APP_VERSION |

**Формы — Source/**

| Файлы | Содержимое |
|---|---|
| f_Main.* | Переименовать TForm1 / Form1 -> TfmMain / fmMain; форма остаётся пустой |
| f_CoinEdit.h/.cpp/.dfm | Форма TfmCoinEdit, пустая |
| f_Import.h/.cpp/.dfm | Форма TfmImport, пустая |

**Проект**

- Добавить все новые модули в Source/_CoinsCollection.cbproj.
- В _CoinsCollection.cpp создавать TdmData до создания главной формы.
- Собрать конфигурацию Win32 / Debug, убедиться, что приложение запускается и показывает пустое главное окно.

**Изоляция проекта от библиотек PawnShop**

Цель — CoinsCollection не должен зависеть от ARLib, EhLib, TsiLang, Overbyte ICS, AlphaControls/AlphaDB. Эти библиотеки остаются установленными в IDE для PawnShop; в IDE и в проекте PawnShop ничего не меняем и ничего не деинсталлируем.

Способ сборки (решено): Clang-компилятор (BCC_UseClassicCompiler = false) + runtime-пакеты. FireDAC на Win32 с Clang нельзя слинковать статически (в lib\win32c нет ни vcl.lib, ни FireDAC*.lib), поэтому UsePackages остаётся true. Классический bcc32 умеет статику, но удаляется в RAD 13 — не берём. Изоляция достигается чисткой списка пакетов.

В Source/_CoinsCollection.cbproj:

- UsePackages = true, DynamicRTL = true (Clang VCL-приложение линкует RTL/VCL/FireDAC как runtime-пакеты; это норма).
- Из PackageImports (Win32 и Win64) убрать записи библиотек PawnShop: ARLibControls, EhLib210, acntCX10Sydney, acntCX10Sydney_R, alphaDBCX10Sydney, IcsCommonCBNewRun, IcsVclCBNewRun. Остальной стоковый список (VCL, FireDAC*, REST, DataSnap и т.д.) оставить как есть.
- Убрать запись Excluded_Packages для ARLibControls104.
- Пути IncludePath / ILINK_LibraryPath оставить только со своим Source: не добавлять C:\Users\user\PawnShop\..., $(CMPN_*), D:\Components\...
- Свой .groupproj; в PawnAps.groupproj проект не включать.

Разрешено использовать только сток RAD Studio 10.4: VCL, FireDAC + FireDACSqliteDriver (SQLite вшит в драйвер, без внешней DLL), REST Library / System.Net.HttpClient / REST.Authenticator.OAuth (для шага 4), System.Zip (для .xlsx). Готовый exe требует стоковые RAD-пакеты (*270.bpl) рядом или в PATH — на дев-машине они уже есть.

### Модель TCoinRecord

| Поле | Тип | Колонка файла |
|---|---|---|
| Id | int (PK) | внутренний |
| Country | String | Country |
| Period | String | Period |
| Currency | String | Currency |
| Denomination | String | Denomination |
| Year | int (0 = нет) | Year |
| Variety | String | Var. |
| Subject | String | Subject |
| DiameterMm | double (0 = нет) | Diameter, mm |
| Condition | String | Condition |
| CatalogValueUah | double | Value, UAH [uCoin] |
| CatalogNumber | String | Number (KM# ...) |
| Color | String | Color |
| Quantity | int | Quantity |
| NeedToReplace | bool | Need to replace |
| PublishedDate | TDateTime (0 = нет) | Published |
| SwapInfo | String | Swap |
| PurchaseDate | TDateTime (0 = нет) | Purchase (00/01/1900 -> нет) |
| MyValueUah | double | Value, UAH [My] |
| GradingCompany | String | Grading company |
| GradingNumber | String | Grading number |
| Grade | String | Grade |
| LabelText | String | Label |
| Comment | String | Comment |

### Не входит в задачу

- Схема БД и реальное подключение к SQLite
- Парсинг .xlsx
- Любая работа с Numista API
- Вёрстка форм, гриды, меню, иконки
- Любые изменения в IDE и в проекте PawnShop

### Критерий приёмки

- Проект компилируется и линкуется в RAD Studio 10.4 без ошибок.
- Приложение запускается и показывает пустую форму TfmMain.
- Все перечисленные классы объявлены, файлы добавлены в _CoinsCollection.cbproj.
- В PackageImports (Win32 и Win64) нет ARLibControls, EhLib210, acntCX10Sydney*, alphaDBCX10Sydney, Ics*CBNewRun; сток (FireDAC*, REST, VCL) на месте.
- В путях проекта нет ссылок на PawnShop, $(CMPN_*), D:\Components.
- Ветка 2-skeleton смержена в master.

---

## Задача 3. База данных SQLite

### Цель

Реальное хранение коллекции в SQLite: подключение через FireDAC, схема с версионированием, работающий CRUD через TCoinRepository.

### Ветка

3-database от master; по завершении — merge в master. Коммит: «3 - База данных SQLite».

### Объём работ

**dm_Data — подключение к SQLite**

- Добавить на модуль данных TFDPhysSQLiteDriverLink и TFDGUIxWaitCursor (иначе FireDAC ругается при первом запросе).
- FDConnection: DriverName = SQLite, LoginPrompt = False; параметр Database задаётся в коде (TDatabase::Open), не в .dfm.
- SQLite-движок линкуется статически (подключить FireDAC.Phys.SQLiteWrapper.Stat) — внешний sqlite3.dll рядом с exe не нужен.
- FDQuery.Connection = FDConnection; FDTransaction.Connection = FDConnection.

**u_AppConfig — путь к файлу БД**

- Load() / Save() через TIniFile по пути `<каталог exe>\CoinsCollection.ini`, секция [Database], ключ Path.
- Если ключа нет — DbPath = `<каталог exe>\coins.db`.

**u_Database — TDatabase**

- Конструктор: `TDatabase(TFDConnection *AConnection)` (хранит указатель, не владеет).
- Open(const String &AFileName): выставить параметр Database = AFileName, Connected = true (файл создаётся при отсутствии).
- Close(): Connected = false.
- IsOpen(): состояние соединения.
- SchemaVersion(): вернуть `PRAGMA user_version`.
- EnsureSchema(): лестница миграций в транзакции. v0 -> v1: создать таблицу coin (схема ниже), выставить `PRAGMA user_version = 1`. Заготовить место под будущие шаги (v1 -> v2 и т.д.).
- Connection(): доступ к TFDConnection для репозитория.

**u_CoinRepository — CRUD**

- Конструктор принимает TDatabase*; работает через его Connection().
- LoadAll(): `SELECT * FROM coin ORDER BY country, denomination, year` -> std::vector<TCoinRecord>.
- Save(TCoinRecord &ARecord): при Id == 0 — INSERT, затем записать last_insert_rowid() в ARecord.Id; иначе UPDATE ... WHERE id = :id. Вернуть Id.
- Delete(int AId): `DELETE FROM coin WHERE id = :id`; вернуть RowsAffected > 0.
- Все запросы параметризованные (:param), без склейки значений в строку.
- Вспомогательные функции маппинга в .cpp: TDateTime <-> TEXT 'yyyy-mm-dd' (0 <-> NULL/''); числовые поля с проверкой на NULL.

**Запуск и проверка**

- fmMain.FormCreate: config.Load(); открыть TDatabase(dmData->FDConnection) на config.DbPath; EnsureSchema(); создать TCoinRepository.
- fmMain.FormDestroy: освободить репозиторий и TDatabase, закрыть соединение.
- Временная кнопка «DB self-test» на fmMain: добавить тестовую запись через Save, перечитать LoadAll, удалить через Delete, показать ShowMessage с числом строк до и после. Кнопку убрать в задаче с реальным UI.

### Схема БД v1 — таблица coin

| Колонка | Тип SQLite | Поле TCoinRecord |
|---|---|---|
| id | INTEGER PRIMARY KEY | Id |
| country | TEXT | Country |
| period | TEXT | Period |
| currency | TEXT | Currency |
| denomination | TEXT | Denomination |
| year | INTEGER | Year |
| variety | TEXT | Variety |
| subject | TEXT | Subject |
| diameter_mm | REAL | DiameterMm |
| condition | TEXT | Condition |
| catalog_value_uah | REAL | CatalogValueUah |
| catalog_number | TEXT | CatalogNumber |
| color | TEXT | Color |
| quantity | INTEGER NOT NULL DEFAULT 1 | Quantity |
| need_to_replace | INTEGER NOT NULL DEFAULT 0 | NeedToReplace |
| published_date | TEXT | PublishedDate ('yyyy-mm-dd' / NULL) |
| swap_info | TEXT | SwapInfo |
| purchase_date | TEXT | PurchaseDate ('yyyy-mm-dd' / NULL) |
| my_value_uah | REAL | MyValueUah |
| grading_company | TEXT | GradingCompany |
| grading_number | TEXT | GradingNumber |
| grade | TEXT | Grade |
| label_text | TEXT | LabelText |
| comment | TEXT | Comment |

### Не входит в задачу

- Импорт из .xlsx
- Любая работа с Numista API
- Реальный UI списка и карточки монеты (только временная кнопка проверки)
- Полноценный экран настроек
- Любые изменения в IDE и в проекте PawnShop

### Критерий приёмки

- Первый запуск создаёт coins.db рядом с exe; SchemaVersion() == 1; таблица coin существует со всеми колонками.
- Повторный запуск не пересоздаёт схему (user_version уже 1), данные сохраняются между запусками.
- DB self-test: запись добавляется (Id > 0), видна в LoadAll, удаляется; счётчики строк до/после сходятся.
- Внешний sqlite3.dll рядом с exe не требуется.
- Запросы параметризованные.
- Ветка 3-database смержена в master.
