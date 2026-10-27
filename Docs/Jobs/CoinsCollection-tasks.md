# CoinsCollection — Задачи

Проект: локальный учёт коллекции монет в SQLite с импортом из файла .xlsx (экспорт uCoin.net) и последующей выгрузкой на Numista через API.
Среда разработки: RAD Studio 10.4 (позже 13), C++Builder / VCL.

Порядок работ: каркас → БД (инфраструктура, затем репозитории) → локализация → главное окно → импорт из .xlsx → интерфейс (карточка, дерево и фильтры, справочники) → панель подробностей → Numista API → фото монет.
Каждая задача рассчитана на 1–2 часа. На задачу заводится отдельная ветка от master, по завершении — merge в master.

Тексты интерфейса (с задачи 4): любой новый текст — английский в .dfm и `Source/Lang/en.ini`, плюс ключи в `ru.ini` и `uk.ini`; строки из кода — через `Tr(L"Модуль.Назначение")`. Литералы на русском / украинском в коде запрещены.

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

## Задача 3. База данных SQLite: инфраструктура и схема

### Цель

Подключение к SQLite через FireDAC, файл БД в `Exe\Db\`, нормализованная (3НФ) схема v1 с версионированием и заполненными справочниками. CRUD монет — в задаче 3a.

### Ветка

3-database от master; по завершении — merge в master. Коммит: «3 - База данных SQLite».

### Среда

RAD Studio 13 (BDS 37.0), Win32, Clang + runtime-пакеты. Готовый exe требует стоковые пакеты *370.bpl. RAD 13 используется и для PawnShop, поэтому все настройки — только в Source/_CoinsCollection.cbproj; настройки IDE, глобальные пути и установленные пакеты не менять.

### Объём работ

**Размещение exe и БД (как в PawnShop)**

- В Source/_CoinsCollection.cbproj для Win32 (Debug и Release) FinalOutputDir = `..\Exe\` — exe собирается в `D:\My_project\Coins\Exe\`. IntermediateOutputDir (.obj, .pch) остаётся `Source\Win32\<Config>`.
- БД по умолчанию: `<каталог exe>\Db\coins.db`. Каталог `Db` создаётся при первом запуске (ForceDirectories).
- .gitignore: добавить `Exe/Db/`, `Exe/*.ini`, `Exe/*.xlsx` (локальные данные, настройки и экспорт коллекции не коммитятся).

**dm_Data — подключение к SQLite**

- Добавить на модуль данных TFDPhysSQLiteDriverLink (EngineLinkage = slStatic) и TFDGUIxWaitCursor (Provider = Forms).
- FDConnection: DriverID = SQLite, LoginPrompt = False, ForeignKeys = On, LockingMode = Normal (чтобы БД можно было открыть в DB Browser for SQLite при запущенной программе). Параметр Database задаётся в коде (TDatabase::Open), не в .dfm.
- Подключить FireDAC.Phys.SQLiteWrapper.Stat — SQLite-движок вшит в драйвер (пакет FireDACSqliteDriver370.bpl), внешний sqlite3.dll не нужен.
- FDQuery.Connection = FDConnection; FDTransaction.Connection = FDConnection.
- В Source/_CoinsCollection.cbproj, в ItemGroup с PackageImport для Win32, добавить `FireDACSqliteDriver.bpi` и `vclFireDAC.bpi`. Без них ilink32 ищет отсутствующие в RAD 13 .obj. Свойство LinkPackageImports не использовать — IDE его перекрывает.

**u_AppConfig — путь к файлу БД**

- Load() / Save() через TIniFile, файл `<каталог exe>\CoinsCollection.ini`, секция [Database], ключ Path.
- Нет ключа → DbPath = `<каталог exe>\Db\coins.db`. Относительный путь в ini считается от каталога exe.

**u_Database — TDatabase**

- Конструктор: `TDatabase(TFDConnection *AConnection)` (хранит указатель, не владеет).
- Open(const String &AFileName): создать каталог файла, Database = AFileName, Connected = true (файл создаётся при отсутствии).
- Close() / IsOpen() / Connection().
- SchemaVersion(): `PRAGMA user_version`.
- EnsureSchema(): лестница миграций в одной транзакции. v0 → v1: создать все таблицы и индексы (схема ниже), заполнить справочники continents, countries и conditions, выставить `PRAGMA user_version = 1`. Место под v1 → v2 и далее. Версия БД новее программы → исключение.
- Автокопия перед миграцией: если в БД уже есть схема (user_version > 0) и она старше CurrentSchemaVersion — закрыть соединение, скопировать файл в `<имя>.v<версия>.bak` рядом с БД (например `coins.v1.bak`, существующая копия перезаписывается), открыть соединение снова и только потом мигрировать. Не удалось скопировать — исключение, миграция не выполняется. Для новой пустой БД (user_version = 0) копия не нужна.

**u_SeedData.h/.cpp — начальные данные справочников (новый модуль, добавить в .cbproj и _CoinsCollection.cpp)**

- Массивы SeedContinents (id, name) и SeedCountries (name, id континента, is_exist). Вставка в миграции v1 параметризованными INSERT.

**u_CoinRepository**

- Реализацию CRUD по плоской таблице убрать; методы снова заглушки `// TODO задача 3a`.

**Запуск и проверка**

- fmMain.FormCreate: config.Load(); открыть TDatabase(dmData->FDConnection) на config.DbPath; EnsureSchema().
- fmMain.FormDestroy: освободить TDatabase, закрыть соединение.
- Временная кнопка «DB self-test»: показать путь к БД, SchemaVersion, `PRAGMA foreign_keys`, список таблиц с числом строк. Кнопку убрать в задаче с реальным UI.

### Схема БД v1

Все ссылки — `id_<таблица>`, `REFERENCES <таблица>(id)`. Даты — TEXT 'yyyy-mm-dd' или NULL.

**continents** — континент (заполняется в миграции)

| Колонка | Тип |
|---|---|
| id | INTEGER PRIMARY KEY |
| name | TEXT NOT NULL UNIQUE |

Данные: Europe, Asia, Africa, North America, South America, Australia and Oceania. Antarctica не нужна: суверенных государств там нет, монеты антарктических территорий относятся к их метрополиям. «Extinct states» — не континент: исчезнувшая страна хранит свой реальный континент (СССР — Europe) и is_exist = 0, а узел «Extinct states» в дереве строится по is_exist (задача 8).

**countries** — страна

| Колонка | Тип |
|---|---|
| id | INTEGER PRIMARY KEY |
| name | TEXT NOT NULL UNIQUE COLLATE NOCASE |
| id_continent | INTEGER REFERENCES continents(id) (NULL — не указан) |
| is_exist | INTEGER NOT NULL DEFAULT 1 (0 — страна больше не существует) |

Данные: страны из первого столбца (Country) экспорта `Exe\Collection_uCoin.net.xlsx` без повторов — 187 названий, написание один в один как у uCoin (иначе импорт создаст дубль без континента). Каждой стране назначен континент; 18 исчезнувших государств (USSR, Russian Empire, Czechoslovakia, Yugoslavia, Germany - GDR, Germany - Third Reich, Prussia, Ottoman Empire, Rhodesia, Zaire и др.) — с реальным континентом и is_exist = 0. Страны, которых нет в справочнике, импорт создаёт с id_continent = NULL (узел «No continent», правка — задача 9).

**periods** — период страны (например, «Ukraine (1991 – Today)»)

| Колонка | Тип |
|---|---|
| id | INTEGER PRIMARY KEY |
| id_country | INTEGER NOT NULL REFERENCES countries(id) |
| name | TEXT NOT NULL (пустая строка — период не указан) |
| | UNIQUE (id_country, name) |

**currencies** — валюта

| Колонка | Тип |
|---|---|
| id | INTEGER PRIMARY KEY |
| name | TEXT NOT NULL UNIQUE COLLATE NOCASE |

**conditions** — шкала сохранности (заполняется в миграции, совпадает с enum TCoinCondition)

| Колонка | Тип |
|---|---|
| id | INTEGER PRIMARY KEY |
| code | TEXT NOT NULL UNIQUE |
| name | TEXT NOT NULL |
| sort_order | INTEGER NOT NULL |

Данные (code / name / sort_order): P / Poor / 1, FR / Fair / 2, G / Good / 3, VG / Very Good / 4, F / Fine / 5, VF / Very Fine / 6, XF / Extremely Fine / 7, AU / About Uncirculated / 8, UNC / Uncirculated / 9, PRF / Proof / 10.

**coins** — монета-тип (общее для всех экземпляров; страна — через период)

| Колонка | Тип | Поле TCoinRecord |
|---|---|---|
| id | INTEGER PRIMARY KEY | CoinId (задача 3a) |
| id_period | INTEGER NOT NULL REFERENCES periods(id) | Country + Period |
| id_currency | INTEGER REFERENCES currencies(id) | Currency |
| denomination | TEXT NOT NULL | Denomination |
| year | INTEGER (NULL — не указан) | Year |
| variety | TEXT | Variety |
| subject | TEXT | Subject |
| diameter_mm | REAL | DiameterMm |
| catalog_number | TEXT | CatalogNumber |

Индекс: coins(id_period).

**coin_items** — мой экземпляр монеты

| Колонка | Тип | Поле TCoinRecord |
|---|---|---|
| id | INTEGER PRIMARY KEY | Id |
| id_coin | INTEGER NOT NULL REFERENCES coins(id) ON DELETE CASCADE | — |
| id_condition | INTEGER REFERENCES conditions(id) | Condition (code) |
| quantity | INTEGER NOT NULL DEFAULT 1 | Quantity |
| need_to_replace | INTEGER NOT NULL DEFAULT 0 | NeedToReplace |
| catalog_value_uah | REAL | CatalogValueUah |
| my_value_uah | REAL | MyValueUah |
| purchase_date | TEXT | PurchaseDate |
| published_date | TEXT | PublishedDate |
| swap_info | TEXT | SwapInfo |
| color | TEXT | Color |
| grading_company | TEXT | GradingCompany |
| grading_number | TEXT | GradingNumber |
| grade | TEXT | Grade |
| label_text | TEXT | LabelText |
| comment | TEXT | Comment |

Индекс: coin_items(id_coin).

Year, номинал, диаметр, номер по каталогу — атрибуты монеты, отдельных таблиц не требуют. Grading company пока текстом.

### Не входит в задачу

- CRUD монет и справочников (задача 3a)
- Импорт из .xlsx
- Любая работа с Numista API
- Реальный UI (только временная кнопка проверки)
- Полноценный экран настроек
- Любые изменения в IDE и в проекте PawnShop

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release) без ошибок — и из IDE, и через msbuild; exe появляется в `Exe\`.
- Exe зависит только от стоковых *370.bpl; настройки IDE и проект PawnShop не изменены.
- Первый запуск создаёт `Exe\Db\coins.db`; SchemaVersion() == 1; есть все 7 таблиц с колонками и ограничениями по схеме; continents — 6 строк, countries — 187 (из них 18 с is_exist = 0), conditions — 10.
- Каждое название из столбца Country файла `Exe\Collection_uCoin.net.xlsx` находится в countries один в один; у каждой страны заполнен id_continent.
- `PRAGMA foreign_keys` = 1: вставка coin с несуществующим id_period отклоняется.
- Повторный запуск не пересоздаёт схему и не дублирует справочники.
- Автокопия: при обновлении схемы существующей БД рядом появляется `coins.v<старая версия>.bak` с данными до миграции; при создании новой БД копия не делается; если копию сделать не удалось — миграция не выполняется.
- БД открывается в DB Browser for SQLite при запущенной программе.
- Внешний sqlite3.dll не требуется; `Exe/Db/`, `Exe/*.ini` и `Exe/*.xlsx` не попадают в git.
- Ветка 3-database смержена в master.

---

## Задача 3a. Репозитории: справочники и монеты

### Цель

CRUD коллекции поверх нормализованной схемы: плоская запись TCoinRecord (как строка uCoin) раскладывается по справочникам, coins и coin_items и собирается обратно.

### Ветка

3a-repositories от master (после merge задачи 3); по завершении — merge в master. Коммит: «3a - Репозитории».

### Объём работ

**u_CoinTypes.h**

- TCoinRecord: `Id` — это coin_items.id; добавить `CoinId` (coins.id, 0 = ещё не сохранена).
- Структуры для справочников: TContinent {Id, Name}, TCountry {Id, Name, ContinentId, IsExist}, TPeriod {Id, CountryId, Name}, TCurrency {Id, Name}, TCondition {Id, Code, Name, SortOrder}.

**u_LookupRepository.h/.cpp — TLookupRepository (новый модуль, добавить в .cbproj)**

- Конструктор принимает TDatabase*.
- Списки: Continents(), Countries(), Periods(int ACountryId), Currencies(), Conditions() → std::vector<...>, по алфавиту (Conditions — по sort_order).
- FindOrCreateCountry(const String &AName) → id. Новая страна: id_continent = NULL, is_exist = 1.
- FindOrCreatePeriod(int ACountryId, const String &AName) → id (пустое имя — допустимо).
- FindOrCreateCurrency(const String &AName) → id; пустое имя → 0 (NULL в coins).
- FindCondition(const String &ACode) → id; пустой код → 0; неизвестный код → исключение (шкала фиксированная, нормализация кодов — в задаче импорта).
- UpdateCountry(const TCountry &) — континент и is_exist.
- Имена сравниваются после Trim; без учёта регистра — COLLATE NOCASE (работает для латиницы; данные uCoin на английском).

**u_Database — TDbTransaction**

- Транзакция на время жизни объекта: без Commit() — откат в деструкторе. Если транзакция уже открыта снаружи (будущий пакетный импорт), своя не начинается.

**u_CoinRepository — TCoinRepository**

- Конструктор принимает TDatabase* (и использует TLookupRepository).
- LoadAll(): coin_items JOIN coins, periods, countries, LEFT JOIN currencies, conditions → std::vector<TCoinRecord>, ORDER BY countries.name, coins.denomination, coins.year.
- Save(TCoinRecord &ARecord) — одна транзакция:
  1. страна → период → валюта (FindOrCreate), состояние (FindCondition);
  2. CoinId == 0 — найти запись coins с тем же (id_period, id_currency, denomination, year, variety, subject, catalog_number; сравнение NULL-безопасное, `IS`): найдена — её поля не перезаписываются (пустой diameter_mm дополняется), не найдена — создать; CoinId != 0 — UPDATE coins (меняет тип для всех его экземпляров); записать CoinId;
  3. Id == 0 — INSERT coin_items, иначе UPDATE; записать Id. Вернуть Id.
- Delete(int AId): удалить запись coin_items; если у монеты не осталось экземпляров — удалить и запись coins. Справочники не трогаются. Вернуть true, если экземпляр существовал.
- Все запросы параметризованные. Маппинг: TDateTime ↔ 'yyyy-mm-dd' (0 ↔ NULL); Year / DiameterMm / CatalogValueUah / MyValueUah: 0 ↔ NULL; пустые строки (после Trim) ↔ NULL.

**Запуск и проверка**

- fmMain: создать TCoinRepository после EnsureSchema, освободить в FormDestroy.
- «DB self-test» дополнить: сохранить 2 экземпляра одной монеты (разные Condition) и 1 другой монеты той же страны → coins: +2, coin_items: +3, countries: +0 (страна из справочника), periods: +1; LoadAll возвращает 3 записи с правильными полями (включая даты и NULL-поля); удалить все 3 → coins и coin_items вернулись к исходному числу, countries и periods остались. Показать счётчики до/после.

### Не входит в задачу

- Импорт из .xlsx и нормализация кодов состояния uCoin
- UI списка, карточки монеты и справочников
- Удаление неиспользуемых записей справочников
- Любая работа с Numista API
- Любые изменения в IDE и в проекте PawnShop

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release) без ошибок и предупреждений.
- DB self-test проходит: два экземпляра одной монеты дают одну строку coins; повторное сохранение той же страны/периода/валюты не создаёт дублей в справочниках.
- Save с неизвестным кодом состояния откатывает транзакцию целиком (в БД ничего не добавляется).
- Delete последнего экземпляра удаляет и запись coins.
- Запросы параметризованные.
- Ветка 3a-repositories смержена в master.

---

## Задача 4. Локализация интерфейса (EN / RU / UK)

### Цель

Интерфейс на трёх языках — английский (основной), русский, украинский — с переключением без перезапуска. Свой лёгкий механизм на текстовых файлах, только стоковый RTL/VCL (без ITE, TsiLang, gettext).

### Ветка

4-localization от master (после merge задачи 3a); по завершении — merge в master. Коммит: «4 - Локализация интерфейса».

### Принципы

- Английский — базовый язык: тексты в .dfm и en.ini пишутся по-английски; en.ini — эталонный список ключей.
- Коды языков — ISO 639-1: `en`, `ru`, `uk` (не `ua`).
- Данные коллекции (страны, номиналы и т.д. из uCoin) не переводятся — только интерфейс.
- Цепочка подстановки: текущий язык → en.ini → исходный текст (для форм — значение из .dfm, для строк кода — сам ключ). Программа никогда не показывает пустую строку.

### Объём работ

**Файлы переводов**

- Исходники: `Source/Lang/en.ini`, `ru.ini`, `uk.ini`, кодировка UTF-8.
- Чтение через TMemIniFile с TEncoding::UTF8 (TIniFile использует WinAPI и UTF-8 не понимает).
- Post-build event в Source/_CoinsCollection.cbproj копирует `Lang\*.ini` в `<каталог exe>\Lang\` (xcopy, только внутри проекта).
- Формат:

```ini
[Language]
Code=en
Name=English

[Strings]
Main.SelfTest.Result=Rows before: %d, after: %d

[fmMain]
Caption=Coins Collection
btnSelfTest.Caption=DB self-test
miLanguage.Caption=Language
```

- Секция `[Strings]` — строки из кода, ключи вида `Модуль.Назначение`.
- Секция на каждую форму (по Name формы): `Caption` самой формы и `<Компонент>.<Свойство>`.
- `[Language] Name` пишется на самом языке (English / Русский / Українська).

**u_Translator.h/.cpp — TTranslator**

- Один экземпляр на приложение, доступ через `Translator()`.
- `void Load(const String &ALangDir)` — найти `*.ini` в каталоге, прочитать список доступных языков (Code, Name); всегда загрузить en.ini как запасной.
- `bool SetLanguage(const String &ACode)` — загрузить язык; неизвестный код → `en`.
- `String Language()`; `void Languages(...)` — список (Code, Name) для меню.
- `String Tr(const String &AKey)` — строка из `[Strings]` по цепочке подстановки. Параметры подставлять на месте вызова через `Format(Tr("..."), ARRAYOFCONST((...)))`.
- `void TranslateForm(TCustomForm *AForm)` — Caption формы и Caption / Hint её компонентов (обход `AForm->Components[]`, запись через System.TypInfo: IsPublishedProp / SetStrProp). При первом вызове для формы запомнить исходные тексты из .dfm, чтобы возврат на язык без ключа восстанавливал оригинал.
- `TranslateAll()` — TranslateForm для всех `Screen->Forms[]` + вызов подписчиков.
- Событие смены языка (список `std::function<void()>` или TNotifyEvent) — для строк, выставленных из кода.
- В Debug при загрузке ru/uk писать в OutputDebugString ключи, которые есть в en.ini, но отсутствуют в переводе.

**u_AppConfig**

- Секция `[UI]`, ключ `Language`. Нет ключа → `en`.

**Подключение к приложению**

- _CoinsCollection.cpp: после config.Load() — Translator().Load(`<каталог exe>\Lang`) и SetLanguage(config.Language). Если конфиг к этому моменту грузится в fmMain — перенести загрузку конфига в _CoinsCollection.cpp.
- Каждая форма в конструкторе (или FormCreate) вызывает `Translator().TranslateForm(this)`.
- fmMain: главное меню с пунктом Language; подпункты строятся из Translator().Languages(), текущий отмечен (RadioItem / Checked). Выбор → SetLanguage, TranslateAll, сохранение в config.
- Существующие строки интерфейса перевести на ключи: Caption форм fmMain / fmCoinEdit / fmImport, кнопка и сообщение «DB self-test» из задачи 3.
- Английские тексты в .dfm всех трёх форм.

**Правило на будущее (дописать в шапку этого файла)**

- Любой новый текст интерфейса: английский в .dfm / en.ini + ключи в ru.ini и uk.ini; литералы на русском/украинском в коде запрещены.

### Не входит в задачу

- Перевод данных коллекции и шаблонов импорта
- Списки строк (TComboBox.Items, TRadioGroup.Items), заголовки колонок гридов — появятся вместе с реальным UI и будут добавлены отдельно
- Множественные формы (1 монета / 2 монеты / 5 монет)
- Форматы дат и чисел — берутся из настроек Windows
- Автоопределение языка по Windows (первый запуск всегда на английском)
- Любые изменения в IDE и в проекте PawnShop

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release); новых зависимостей, кроме стоковых *370.bpl, нет.
- Первый запуск без ключа в ini — интерфейс на английском.
- Меню Language показывает English / Русский / Українська; переключение сразу меняет тексты всех открытых форм и сообщения из кода, без перезапуска.
- Выбранный язык сохраняется в CoinsCollection.ini и применяется при следующем запуске.
- Удалённый из ru.ini ключ показывается по-английски (и попадает в отладочный лог в Debug); повреждённый/отсутствующий язык в конфиге → английский.
- Каталог `Lang\` с тремя .ini появляется рядом с exe после сборки.
- Ветка 4-localization смержена в master.

---

## Общие правила для задач интерфейса (5, 7–9)

- Только стоковый VCL: TMainMenu, TActionList, TToolBar, TDrawGrid (с задачи 7a; в задаче 5 был TListView), TTreeView, TSplitter, TStatusBar, TComboBox, TDateTimePicker, TUpDown. Без EhLib, AlphaControls и прочих библиотек PawnShop.
- Команды меню и кнопок — через TActionList (одно действие на пункт меню и кнопку). Ещё не реализованные команды: `Enabled = false`.
- Все тексты — по правилам задачи 4: английский в .dfm / en.ini, ключи в ru.ini и uk.ini.
- Иконки не рисуем: кнопки панели с текстом (ShowCaptions). Иконки — отдельной задачей при необходимости.
- Данные — через TCoinRepository / TLookupRepository (задача 3a); SQL в формах не пишется.

---

## Задача 5 (UI-1). Главное окно: меню, панель, список

### Цель

Рабочее главное окно: меню и панель кнопок по согласованному составу, список экземпляров с сортировкой, строка состояния. Временная кнопка «DB self-test» убирается.

### Ветка

5-ui-main от master (после merge задачи 4); по завершении — merge в master. Коммит: «5 - Главное окно».

### Объём работ

**Действия (TActionList на fmMain)**

| Меню | Действие | Горячая клавиша | В этой задаче |
|---|---|---|---|
| File | Import from uCoin (.xlsx)… | — | disabled (задача 6) |
| File | Backup database… | — | disabled |
| File | Exit | Alt+F4 | работает |
| Collection | Add | Ins | disabled (задача 7) |
| Collection | Edit | Enter | disabled (задача 7) |
| Collection | Delete | Del | disabled (задача 7) |
| Collection | Need to replace (флажок) | — | disabled (задача 8) |
| Collection | Refresh | F5 | работает: перечитать LoadAll |
| Reference | Countries… / Periods… / Currencies… | — | disabled (задача 9) |
| Numista | Connect / Settings… ; Upload selected ; Upload all | — | disabled (задача Numista) |
| Settings | Language ▸ | — | работает (из задачи 4) |
| Settings | Settings… | — | disabled |
| Help | About | F1 | работает: имя и версия из versionConfig.h (окно с заголовком на текущем языке) |

**Панель кнопок (TToolBar):** Add, Edit, Delete | Import. Место под поле поиска справа — в задаче 8.

**Список экземпляров (TListView)**

- ViewStyle = vsReport, OwnerData = true (виртуальный режим, данные из std::vector<TCoinRecord>), RowSelect, MultiSelect, ReadOnly, HideSelection = false.
- Колонки: Period, Currency, Denomination, Year, Condition, Number (CatalogNumber), Value (CatalogValueUah, формат `#,##0.00`). Year = 0 и пустые значения — пустая ячейка.
- Клик по заголовку — сортировка по колонке, повторный клик — обратный порядок; стрелка в заголовке (HDF_SORTUP / HDF_SORTDOWN). Condition сортируется по sort_order шкалы, Year и Value — как числа, остальное — как строки без учёта регистра в «натуральном» порядке, как в Проводнике (StrCmpLogicalW: "2 hryvni" < "10 hryven", "KM# 9" < "KM# 10"). Сортировка по умолчанию — Denomination, Year.
- Двойной клик / Enter — действие Edit.
- Выделение сохраняется после Refresh (по Id).

**Строка состояния (TStatusBar):** `Items: <число записей>`. Суммы — в задаче 8.

**Прочее**

- Удалить кнопку «DB self-test» и её обработчик.
- Позиция и размер окна, ширины колонок в этой задаче не сохраняются.

### Не входит в задачу

- Карточка монеты, добавление / изменение / удаление (задача 7)
- Дерево стран, поиск, фильтры, суммы (задача 8)
- Справочники (задача 9), импорт (задача 6), Numista, резервная копия, окно настроек
- Иконки

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release) без ошибок и предупреждений.
- Меню и панель соответствуют таблице; нереализованные пункты неактивны.
- Список показывает записи из БД (проверка на данных из self-test задачи 3a или вставленных вручную); сортировка работает по всем колонкам в обе стороны.
- F5 перечитывает список, выделение сохраняется.
- About показывает имя и версию; все тексты переключаются языком из задачи 4.
- Ветка 5-ui-main смержена в master.

---

## Задача 6. Импорт коллекции из экспорта uCoin (.xlsx)

### Цель

Загрузить коллекцию из файла экспорта uCoin.net в БД, чтобы дальше работать с реальными данными. Импорт заменяет текущую коллекцию целиком (перед этим — автокопия БД); справочники не очищаются.

### Ветка

6-import от master (после merge задачи 5); по завершении — merge в master. Коммит: «6 - Импорт из uCoin».

### Формат файла (по `Exe\Collection_uCoin.net.xlsx`)

Один лист, первая строка — заголовки, далее по строке на экземпляр. Колонки ищутся по тексту заголовка, а не по букве. Нет колонки Country или Denomination — файл отклоняется.

| Заголовок | Поле TCoinRecord | Преобразование |
|---|---|---|
| Country | Country | текст |
| Period | Period | текст |
| Currency | Currency | текст |
| Denomination | Denomination | текст, обязательно |
| Year | Year | целое; пусто → 0 |
| Var. | Variety | текст (в файле бывает и числом) |
| Subject | Subject | текст |
| Diameter, mm | DiameterMm | число с точкой; пусто → 0 |
| Condition | Condition | код шкалы (VF, XF, UNC …), без учёта регистра; пусто → нет; неизвестный код — ошибка строки |
| Value, UAH [uCoin] | CatalogValueUah | число с точкой |
| Number | CatalogNumber | текст |
| Color | Color | текст |
| Quantity | Quantity | целое; пусто → 1 |
| Need to replace | NeedToReplace | непустая ячейка → true |
| Published | PublishedDate | число Excel (дни от 30.12.1899) → дата; 0 / пусто → нет |
| Swap | SwapInfo | текст |
| Purchase | PurchaseDate | как Published |
| Value, UAH [My] | MyValueUah | число с точкой |
| Grading company / Grading number / Grade | GradingCompany / GradingNumber / Grade | текст |
| Label | LabelText | текст (бывает числом) |
| Comment | Comment | текст |

### Объём работ

**u_ImportSource — TXlsxSource (чтение .xlsx только стоковыми средствами)**

- Open(const String &AFileName): .xlsx — это zip; TZipFile (System.Zip) читает `xl/workbook.xml` + `xl/_rels/workbook.xml.rels` (путь первого листа), `xl/sharedStrings.xml` (может отсутствовать) и лист. XML разбирается TXMLDocument (Xml.XMLDoc, MSXML); в .cbproj добавить `PackageImport xmlrtl.bpi`.
- Ячейки: `t="s"` — индекс в sharedStrings (текст из всех `<t>` элемента `<si>`), `t="inlineStr"`, `t="str"` / число / логическое — значение `<v>` как текст. Колонка определяется по ссылке ячейки (`r="F12"`), пустые ячейки в XML пропущены.
- Header() → TArray<String>; RowCount(); Row(int) → TArray<String> (по числу колонок заголовка). Не .xlsx / нет листа — исключение с понятным текстом.

**u_CoinImporter — TCoinImporter**

- Конструктор получает список допустимых кодов состояния (из TLookupRepository::Conditions()).
- Parse(TXlsxSource &) → std::vector<TCoinRecord> + ошибки {номер строки в файле, текст}. Строка с ошибкой (нет Country / Denomination, неверное число или дата, неизвестный код состояния) пропускается целиком, остальные импортируются. Полностью пустые строки игнорируются.
- Числа разбираются с точкой как разделителем независимо от настроек Windows.

**Запись в БД**

- TCoinRepository::ReplaceAll(const std::vector<TCoinRecord> &) → число сохранённых: одна транзакция (TDbTransaction) — DELETE coin_items, DELETE coins, затем Save по каждой записи. Ошибка — откат, прежняя коллекция не тронута. Справочники (countries, periods, currencies) не очищаются; новые страны создаются без континента.
- Перед заменой непустой коллекции — копия файла БД: `<имя>.before-import-<yyyymmdd-hhnnss>.bak` рядом с БД (механизм копирования из задачи 3 сделать публичным методом TDatabase::Backup(const String &ASuffix)).

**UI (fmMain)**

- Действие Import from uCoin (.xlsx)… и кнопка Import становятся активными.
- TOpenDialog: фильтр `uCoin export (*.xlsx)`, начальная папка — каталог exe.
- Разбор файла с песочными часами → подтверждение: «Файл: N строк; к импорту M экземпляров; с ошибками K строк. Текущая коллекция (X экземпляров) будет заменена, перед этим создаётся резервная копия. Продолжить?». Если ошибок > 0 — в том же окне первые 10 ошибок («Row 17: unknown condition 'ZZ'»).
- После импорта — Refresh списка и сообщение «Импортировано M экземпляров (C монет), пропущено K строк».
- Все тексты — через ключи (en / ru / uk).

### Не входит в задачу

- Форма предпросмотра fmImport (остаётся заготовкой) и выбор колонок вручную
- Дозапись / обновление без замены коллекции, сопоставление с уже внесёнными вручную монетами
- Другие форматы (CSV, старый .xls)
- Назначение континентов новым странам (задача 9)

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release) без ошибок и предупреждений; новые зависимости — только стоковый xmlrtl370.bpl.
- Импорт `Exe\Collection_uCoin.net.xlsx`: 2305 экземпляров, 2305 монет (монета определяется и по номеру по каталогу: строки с одинаковыми номиналом и годом, но разными KM# — разные монеты), 0 ошибок; новых стран не появляется (все 187 есть в справочнике); у 17 экземпляров состояние пустое; даты Published / Purchase и суммы совпадают с файлом (выборочная проверка нескольких строк).
- Повторный импорт того же файла даёт те же числа (коллекция заменена, не удвоена), рядом с БД появляется копия `*.before-import-*.bak`.
- Неверный файл (не .xlsx, битый zip, лист без колонки Country) — понятное сообщение, БД не изменена.
- Импорт занимает не более нескольких секунд.
- Ветка 6-import смержена в master.

---

## Задача 7 (UI-2). Карточка монеты: добавление, изменение, удаление

### Цель

Форма fmCoinEdit для ввода и правки экземпляра и действия Add / Edit / Delete в главном окне.

### Ветка

7-ui-coin-edit от master (после merge задачи 6); по завершении — merge в master. Коммит: «7 - Карточка монеты».

### Объём работ

**fmCoinEdit — модальная форма, два блока (TGroupBox)**

Блок «Coin» (общее для всех экземпляров монеты):

| Поле | Контрол | Правило |
|---|---|---|
| Country | TComboBox csDropDown (выбор из списка или ввод новой) | обязательно |
| Period | TComboBox csDropDown, список периодов выбранной страны | можно пусто |
| Currency | TComboBox csDropDown | можно пусто |
| Denomination | TEdit | обязательно |
| Year | TEdit, только цифры | пусто или 1..(текущий год + 1) |
| Variety, Subject, Catalog number | TEdit | — |
| Diameter, mm | TEdit | пусто или число > 0 |

Блок «My item» (этот экземпляр):

| Поле | Контрол | Правило |
|---|---|---|
| Condition | TComboBox csDropDownList: пусто + шкала (code — name) | — |
| Quantity | TEdit + TUpDown | 1..999 |
| Need to replace | TCheckBox | — |
| Purchase date, Published date | TDateTimePicker, ShowCheckbox (снят = нет даты) | — |
| Catalog value, UAH / My value, UAH | TEdit | пусто или число ≥ 0 |
| Grading company, Grading number, Grade, Label, Swap, Color | TEdit | — |
| Comment | TMemo | — |

- Числа вводятся с разделителем из настроек Windows; точка тоже принимается.
- OK: проверка полей → при ошибке сообщение и фокус на поле; иначе запись в TCoinRecord, ModalResult = mrOk. Cancel / Esc — без изменений.
- Если у монеты больше одного экземпляра — над блоком «Coin» надпись «Changes in this block apply to all N items of this coin» (TCoinRepository::ItemCount(CoinId) — добавить метод).

**fmMain — действия**

- Add (Ins): пустая карточка; OK → Save → Refresh, выделить новую запись.
- Edit (Enter / двойной клик): карточка текущей записи; OK → Save → Refresh.
- Delete (Del): подтверждение «Delete N item(s)?» → Delete для каждой выделенной записи → Refresh. (С задачи 7a — только одна текущая запись.)
- Enabled: Edit — ровно одна выделенная запись; Delete — хотя бы одна.

### Не входит в задачу

- Копирование экземпляра («ещё один такой же»)
- Фото монет
- Редактирование справочников (задача 9), кроме ввода новой страны / периода / валюты в карточке

### Критерий приёмки

- Добавление, изменение и удаление работают; новые страна / период / валюта из карточки попадают в справочники без дублей.
- Незаполненные Country / Denomination и некорректные числа не дают сохранить.
- Даты и пустые поля сохраняются и читаются без искажений (NULL ↔ пусто).
- Надпись про общие поля монеты показывается только при нескольких экземплярах.
- Ветка 7-ui-coin-edit смержена в master.

---

## Задача 7a. Список коллекции на TDrawGrid

### Цель

Заменить TListView в главном окне на TDrawGrid — так же, как сделаны списки в проекте Склад (форма Ov): данные в std::vector, ячейки рисуются в OnDrawCell. Удаление — по одной записи.

### Ветка

7a-drawgrid от master (после merge задачи 7); по завершении — merge в master. Коммит: «7a - Список на TDrawGrid».

### Объём работ

- `lvItems: TListView` → `dgItems: TDrawGrid`: FixedRows = 1 (строка 0 — заголовок), FixedCols = 0, ColCount = 7, строка N = FItems[N - 1]; при пустом списке одна пустая строка данных.
- Options: линии, goRowSelect, goDrawFocusSelected, goColSizing (ширина колонок мышью), goThumbTracking, goFixedRowClick + goFixedHotTrack (клик и подсветка заголовка).
- OnDrawCell: фон и выделение рисует сетка (DefaultDrawing), в обработчике — текст ячейки (CellText) с многоточием при нехватке места; Year и Value — по правому краю; заголовок — Tr(ключ колонки) и стрелка сортировки (треугольник на Canvas) у текущей колонки.
- OnFixedCellClick по строке 0 — сортировка (та же колонка — обратный порядок); текущая запись сохраняется.
- Текущая запись — dgItems->Row: CurrentIndex() / CurrentId() / SelectId() / SelectIndex(); F5, Add, Edit сохраняют / выделяют запись по Id.
- Enter и двойной клик по строке данных — Edit; двойной клик по заголовку — ничего (строка нажатия запоминается в OnMouseDown).
- Delete — только текущая запись; подтверждение с её описанием («Delete the item '¼ dollar, 1976, KM# 204'?»); после удаления курсор на следующей записи. Edit / Delete доступны, если список не пуст.
- Убрать код TListView (сообщения LVM_*, CommCtrl.h, множественный выбор).
- Post-build копирование Lang: путь вывода через `System.IO.Path::Combine`, чтобы работал и абсолютный FinalOutputDir.

### Не входит в задачу

- Множественный выбор и удаление нескольких записей
- Сохранение ширины колонок между запусками

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release) без ошибок и предупреждений.
- Список показывает коллекцию, сортировка кликом по заголовку работает по всем колонкам в обе стороны, стрелка у текущей колонки.
- Навигация клавишами, Enter / двойной клик — карточка текущей записи; F5 сохраняет текущую запись.
- Delete удаляет одну текущую запись после подтверждения; курсор остаётся на соседней записи.
- Тексты заголовков переключаются языком.
- Ветка 7a-drawgrid смержена в master.

---

## Задача 8 (UI-3). Дерево континентов и стран, поиск, фильтры

### Цель

Навигация по коллекции: дерево слева, поиск, фильтр «Need to replace», итоги в строке состояния.

### Ветка

8-ui-filters от master (после merge задачи 7a); по завершении — merge в master. Коммит: «8 - Дерево и фильтры».

### Объём работ

**Дерево (TTreeView слева + TSplitter)**

```
All (1234)
├─ Europe (800)
│   ├─ Germany (120)
│   └─ Ukraine (300)
├─ Asia …
├─ Africa …
├─ North America …
├─ South America …
├─ Australia and Oceania …
├─ No continent (15)        ← страны без id_continent (после импорта — все новые)
└─ Extinct states (60)      ← страны с is_exist = 0
    ├─ USSR (40)
    └─ Yugoslavia (20)
```

- В скобках — число экземпляров (записей списка). Континенты без монет показываются с (0); страны без монет не показываются.
- Исчезнувшие страны (is_exist = 0) показываются только в узле «Extinct states», не под своим континентом.
- Узел «No continent» показывается, только если такие страны есть.
- Выбор узла фильтрует список. Выбранный узел и раскрытие сохраняются при Refresh.
- Дерево строится в памяти: Countries() из TLookupRepository + записи LoadAll (связь по имени страны).
- Названия континентов — данные справочника (на английском), не переводятся; подписи All / No continent / Extinct states — через Tr().

**Список**

- Колонка Country (первой) показывается (видимые колонки — список FColumns, ColCount меняется; ширины, заданные мышью, запоминаются по колонкам), когда выбран не узел одной страны (All, континент, Extinct states, No continent); при выборе страны скрывается.

**Поиск и фильтры**

- Поле Search на панели (TEdit + кнопка очистки): подстрока без учёта регистра по Country, Period, Currency, Denomination, Year, Subject, Variety, Number, Comment; применяется при вводе.
- Collection → Need to replace — флажок (AutoCheck): только записи с NeedToReplace.
- Фильтры складываются: узел дерева И поиск И Need to replace. Счётчики в дереве не зависят от поиска.
- Пока фокус в поле поиска, действия Edit / Delete выключены — иначе Enter и Del уходили бы в действия, а не в поле (выключенное действие не перехватывает свою клавишу). В поле поиска: Esc — очистить, Enter / стрелка вниз — к списку. При запуске фокус в списке (ActiveControl = dgItems).

**Строка состояния**

- `Items: <показано> / <всего>`, `Quantity: <сумма Quantity>`, `Value: <сумма Value × Quantity>` UAH — по показанным записям.

### Не входит в задачу

- Сохранение выбранного узла и фильтров между запусками
- Расширенный фильтр (по годам, состоянию и т.д.)
- Правка континента / is_exist страны (задача 9)

### Критерий приёмки

- Дерево соответствует схеме выше; исчезнувшие страны только в «Extinct states»; счётчики верные.
- Фильтры по дереву, поиску и Need to replace работают вместе; итоги в строке состояния считаются по видимым записям.
- Колонка Country появляется / скрывается по правилу.
- После Add / Edit / Delete дерево и итоги обновляются.
- Ветка 8-ui-filters смержена в master.

---

## Задача 9 (UI-4). Справочники: страны, периоды, валюты

### Цель

Правка справочников: главное — назначить стране континент и признак «существует», чтобы после импорта разложить страны по дереву.

### Ветка

9-ui-reference от master (после merge задачи 8); по завершении — merge в master. Коммит: «9 - Справочники».

### Объём работ

**fmReference — одна форма с TPageControl, вкладки Countries / Periods / Currencies**

- Меню Reference → Countries… / Periods… / Currencies… открывают форму на нужной вкладке.
- Списки — TDrawGrid (как в главном окне), под списком — панель правки текущей записи и кнопки Add new / Save / Delete.
- Countries: список (Name, Continent, Exists, Items); правка выбранной страны — Name, Continent (TComboBox csDropDownList, первый пункт «—» = нет), Exists (TCheckBox). Фильтр «Only without continent» для быстрого разбора после импорта. Массового назначения континента нет (решение при старте задачи: все 187 стран экспорта уже с континентами, новые появляются по одной). При изменении названия — предупреждение: импорт из uCoin сопоставляет страны по имени.
- Periods: выбор страны → список её периодов (Name, Items); переименование.
- Currencies: список (Name, Items); переименование.
- На каждой вкладке: Add, Delete. Удаление — только неиспользуемых записей (Items = 0), иначе сообщение.
- Переименование в уже существующее имя запрещено (сообщение).
- После закрытия формы главное окно делает Refresh (дерево и список).

**TLookupRepository — дополнить**

- AddCountry / AddPeriod / AddCurrency, Rename…, Delete… (только без ссылок), UpdateCountry (уже есть), счётчики использования (число экземпляров для страны / периода / валюты).

### Не входит в задачу

- Правка шкалы состояний и списка континентов (фиксированы в миграции)
- Объединение дублей (слияние двух стран в одну)
- Массовое назначение континента нескольким странам

### Попутно исправлено (импорт, задача 6)

- Файл .xlsx открывается только на чтение без запрета совместного доступа (fmShareDenyNone) — импорт работает, даже если книга открыта в Excel; если файл открыть нельзя, показывается настоящая причина (Import.Error.Open), а не «это не .xlsx».

### Критерий приёмки

- Страна получает континент и признак Exists; в главном дереве она сразу переезжает в нужный узел.
- Используемую запись удалить нельзя, неиспользуемую — можно; дубли имён не создаются.
- Ветка 9-ui-reference смержена в master.

---

## Задача 10. Панель «Подробности о монете»

### Цель

Под списком монет в главном окне — панель с подробностями текущей записи: место под фото (аверс и реверс), сюжет (Subject), номер uCoin. Колонку Number из списка убрать — номер теперь в панели. Панель задумана расширяемой: позже в ней появятся фото (задачи фото), номер Numista и другие сведения.

### Ветка

10-details-panel от master (после merge задачи 9); по завершении — merge в master. Коммит: «10 - Панель подробностей».

### Где размещать (расчёт)

| | Справа от списка | Под списком (выбрано) |
|---|---|---|
| Ширина списка при окне 1100 px | ≈ 530 px — колонкам нужно ≈ 740 px, горизонтальная прокрутка | ≈ 856 px — все колонки помещаются |
| Высота списка | вся (≈ 25 строк) | ≈ 290 px (≈ 15 строк); в развёрнутом окне на 1080p ≈ 40 строк |
| Фото | только друг под другом, мало места под текст | рядом (аверс | реверс), справа широкое место под длинный Subject |

### Объём работ

**Раскладка главного окна**

- Дерево tvNav — слева на всю высоту (как сейчас).
- Справа — контейнер pnlRight (alClient): список dgItems (alClient), под ним разделитель splDetails (alBottom) и панель pnlDetails (alBottom, высота ≈ 190, меняется мышью; разделитель не даёт сжать панель и список меньше 120 px).

**pnlDetails**

- Слева две рамки фото ≈ 170 × 170 с подписями Obverse / Reverse. До задач фото — серая рамка с текстом «No photo»; рамки — TPanel с TImage внутри, чтобы в задачах фото просто подставить картинку.
- Справа — сведения о текущей записи:
  - заголовок (жирным): «Номинал, год — Страна» (например, «¼ dollar, 1976 — USA»);
  - Period, Currency;
  - Subject — несколько строк с переносом (TLabel WordWrap или ReadOnly TMemo без рамки — по месту);
  - uCoin Number.
- Пары «подпись — значение» оформить так, чтобы новую строку (Numista Number и др.) добавлять одной строкой кода.
- Пустые значения — «—». Нет текущей записи (пустой список) — панель пустая.
- Обновление при смене текущей строки (клавиши, мышь, сортировка, фильтры, F5, Add / Edit / Delete).

**Список**

- Убрать колонку Number (icNumber): из сетки, сортировки и ширин колонок. Поиск по номеру остаётся.

**Тексты**

- Подписи панели — через Tr() / [fmMain] (en / ru / uk).

### Не входит в задачу

- Хранение и показ фото (задачи фото после этой)
- Номер Numista и другие новые поля
- Сохранение высоты панели между запусками
- Скрытие / показ панели из меню

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release) без ошибок и предупреждений.
- Под списком — панель подробностей; высота меняется разделителем; дерево по-прежнему на всю высоту слева.
- Панель показывает данные текущей записи и обновляется при любой смене текущей строки; при пустом списке — пустая.
- Длинный Subject переносится по строкам и не обрезается молча.
- Колонки Number в списке нет; поиск по номеру работает.
- Подписи переключаются языком.
- Ветка 10-details-panel смержена в master.


---

## Общие решения по Numista (задачи 11–13)

- API v3: `https://api.numista.com/v3`. Ключ — в заголовке `Numista-API-Key`.

**Документация Numista API**

| Что | Ссылка |
|---|---|
| Документация (страница для чтения в браузере) | https://en.numista.com/api/doc/index.php |
| Спецификация OpenAPI 3 (все методы, параметры, схемы ответов; на 25.10.2026 — версия 3.36.4) | https://en.numista.com/api/doc/swagger.yaml |
| Пример клиента на Python от Numista | https://en.numista.com/api/doc/numista_api_example.py |

В задачах ниже методы указаны как «раздел документации → метод (operationId в спецификации)»: по operationId метод однозначно находится в swagger.yaml поиском, в документации — в указанном разделе.
- Доступ к своей коллекции — OAuth «client credentials» без браузера: `GET /oauth_token?grant_type=client_credentials&scope=view_collection,edit_collection` → `access_token` (живёт долго, в примере 90 дней), `expires_in`, `user_id`. Дальше — заголовок `Authorization: Bearer <token>`.
- Квота бесплатного ключа — 2000 запросов в месяц; 429 — лимит исчерпан. Программа считает свои запросы за месяц, кэширует то, что можно, и останавливает пакетные операции на 429.
- HTTP и JSON — стоковые `System.Net.HttpClient` (THTTPClient) и `System.JSON`. Таймаут, понятные сообщения об ошибках сети / 401 / 404 / 429.
- Ключ и токен хранятся в CoinsCollection.ini (секция `[Numista]`, ini не попадает в git); в логи и сообщения ключ не выводится.

---

## Задача 11 (Numista-1). Подключение: настройки, токен, справочники Numista

### Цель

Настроить доступ к Numista API и проверить его, расходуя минимум запросов: ключ, токен своей коллекции, кэш списков эмитентов и каталогов.

### Документация

- Authentication — ключ API и OAuth (раздел в начале документации): https://en.numista.com/api/doc/index.php
- OAuth → Get the OAuth access token (`getOAuthToken`, `GET /oauth_token`)
- Catalogue → Get the list of issuers (`getIssuers`, `GET /issuers`)
- Catalogue → Get the list of reference catalogues (`getCatalogues`, `GET /catalogues`)
- Спецификация: https://en.numista.com/api/doc/swagger.yaml

### Ветка

11-numista-connect от master (после merge задачи 10); по завершении — merge в master. Коммит: «11 - Подключение к Numista».

### Объём работ

**u_NumistaClient — TNumistaClient (заготовка из задачи 2)**

- GET / POST с заголовками `Numista-API-Key`, при необходимости `Authorization: Bearer`; разбор JSON; ошибки → исключение с кодом HTTP и текстом из ответа (`error_message`).
- Счётчик запросов за текущий месяц (`[Numista] RequestsMonth`, `RequestsCount` в ini) — растёт на каждом реальном запросе.
- `Authorize()` — client credentials, scope `view_collection,edit_collection`; токен, срок действия и user_id — в ini; повторно запрашивается только по истечении или при 401.
- `Issuers()` — `/issuers` (1 запрос), `Catalogues()` — `/catalogues` (1 запрос); ответы кэшируются в `<каталог exe>\Numista\issuers.json`, `catalogues.json` и повторно не запрашиваются (кнопка «Обновить» — по требованию).

**Окно Numista → Connect / Settings… (fmNumistaSettings)**

- Поле «API key» (маска), кнопка «Check connection» → Authorize → «Connected as user N, token valid until …».
- Счётчик: «Requests this month: N of 2000».
- Кнопка «Refresh Numista lists» (эмитенты, каталоги) и число загруженных записей.
- Меню Numista → Connect / Settings… становится активным.

### Не входит в задачу

- Поиск и сопоставление монет (задача 12), выгрузка (задача 13)
- OAuth «authorization code» (вход другим пользователем через браузер)

### Критерий приёмки

- С правильным ключом «Check connection» показывает user_id и срок токена; с неправильным — понятное сообщение (401).
- Повторная проверка в пределах срока токена не тратит запрос на /oauth_token.
- Списки эмитентов и каталогов загружаются один раз (2 запроса) и берутся из кэша при следующих запусках.
- Счётчик запросов растёт ровно на число сделанных запросов.
- Ветка 11-numista-connect смержена в master.

---

## Задача 12 (Numista-2). Сопоставление монет с типами Numista

### Цель

Связать монеты коллекции с типами (N#) и выпусками Numista, расходуя минимум запросов: точно — по стране и номеру каталога (KM#, Y#, C#), для UC# — по стране, номиналу и году с выбором из вариантов.

### Документация

- Catalogue → Search the catalogue (`searchTypes`, `GET /types`; параметры `issuer`, `catalogue`, `number`, `q`, `year`, `lang`, `count`)
- Catalogue → Get the issues of a type (`getIssues`, `GET /types/{type_id}/issues`; поля выпуска `year`, `gregorian_year`, `mint_letter`)
- Catalogue → Get the list of issuers (`getIssuers`) и reference catalogues (`getCatalogues`) — коды эмитентов и id каталогов (из кэша задачи 11)
- Документация: https://en.numista.com/api/doc/index.php · спецификация: https://en.numista.com/api/doc/swagger.yaml

### Ветка

12-numista-match от master (после merge задачи 11); по завершении — merge в master. Коммит: «12 - Сопоставление с Numista».

### Решения (приняты перед задачей)

- Поиск — через параметры API, а не текстом: `GET /types?issuer=<код страны>&catalogue=<id каталога>&number=<номер>` (например, United States KM# 308 → `issuer=united-states&catalogue=3&number=308`).
- Анализ коллекции без запросов: номера KM# — 1894 монеты, Y# — 287, C# — 13 (все три каталога есть в Numista: KM = 3, Y = 9, C = 8); UC# — 111 (нумерация uCoin, в Numista нет). Страны: 169 из 187 совпадают с эмитентами Numista по названию, остальные — по таблице соответствий в коде или вручную в справочнике стран.
- Выпуск (год) определяется сразу при сопоставлении (+1 запрос, кэшируется).
- Экономия квоты: тест — на нескольких монетах; пакетный режим — с лимитом по умолчанию 10 запросов; каждый ответ кэшируется.

### Объём работ

**Миграция схемы v1 → v2** (автокопия `coins.v1.bak`)

- `countries.numista_issuer` TEXT — код эмитента Numista.
- `coins.numista_type_id` INTEGER, `coins.numista_issue_id` INTEGER.
- `coin_items.numista_item_id` INTEGER — для задачи 13 (чтобы не делать отдельную миграцию).

**Коды стран (без запросов)**

- Для стран без `numista_issuer`: таблица соответствий в коде, иначе точное совпадение названия с эмитентом из кэша `issuers.json` (из одноимённых — дочерний: Austria → `autriche`, а не группа `austria`). Фильтр `issuer` в Numista включает дочерних эмитентов (Germany → `germany` вернул все германские государства), поэтому у стран — конкретный эмитент. Таблица: Germany → `allemagne` (ФРГ), Russia → `russie`, USSR → `ancienne_urss`, Prussia → `prussia_kingdom`, Western Africa (BCEAO) → `bceao`, USA → `united-states`, Congo - DRC → `drc`, Germany - GDR → `ddr`, Eswatini (Swaziland) → `swaziland`, Gambia → `gambie`, Bahamas → `bahamas`, East Timor → `timor_oriental`, Eastern Caribbean → `oeco`, Rhodesia → `rhodesie`, Russian Empire → `russia-empire`, Ceylon → `ceylon_period`, Zaire → `zaire_period`, Lombardy-Venetia → `lombardy_venetia`, Germany - Third Reich → `allemagne-pre1945`, British East Africa → `afrique_de_l_est`).
- Справочник стран: поле «Numista issuer» для ручной правки.

**Кэш запросов**

- Ответы `searchTypes` и `getIssues` — в `Exe\Numista\cache\<md5 адреса>.json`; повторный запрос того же адреса берётся из кэша и не тратит квоту.

**Поиск для текущей монеты (Numista → Find in Numista…)**

1. Номер вида `KM# 308` / `Y# 141` / `C# 12`: каталог по коду из кэша каталогов → `GET /types?issuer=<код>&catalogue=<id>&number=308`. Ничего не найдено (эмитенты Numista иерархичны, монета может быть у «дочернего» эмитента) — повтор без `issuer`.
2. Иначе (UC#, нет номера) — `GET /types?issuer=<код>&q=<номинал>&year=<год>`.
3. Отбор по году монеты по годам типа (`min_year`–`max_year`) из полученного ответа — без запросов; если под год не подошёл ни один тип, остаются все. Ровно один результат — связать сразу; несколько — окно выбора (N#, название, эмитент, годы; «Open in Numista»).
4. `GET /types/{id}/issues` → выпуск: год совпадает и (если в монете указана разновидность) `mint_letter` совпадает с Variety; ровно один подходящий (из нескольких — единственный без `comment`, т. е. обычный тираж, а не «BU set» / «Proof») — сохранить `numista_issue_id`, иначе оставить пустым.
5. Итог: что связано и сколько запросов потрачено («запросов: 2, за месяц 105 из 2000»).

**Пакетное сопоставление (Numista → Match by catalogue number…)**

- Монеты текущего узла дерева / поиска без N# и с номером KM / Y / C.
- Перед запуском — лимит запросов (по умолчанию 10) и подтверждение; кэшированные ответы лимит не расходуют.
- Связывается только однозначный результат (тип + выпуск); остановка на исчерпании лимита и на 429.
- Итог: связано / неоднозначно / не найдено / не хватило лимита; запросов потрачено.

**Отображение**

- Панель подробностей: строка «Numista: N# 12345» — ссылка на страницу типа на Numista.

### Не входит в задачу

- Выгрузка в коллекцию Numista (задача 13)
- Фото из Numista (задачи фото)
- Повторное сопоставление уже связанных монет (только через ручной поиск)

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release) без ошибок и предупреждений.
- Миграция v1 → v2 с автокопией; данные коллекции не меняются.
- Монета с номером KM# / Y# / C# связывается 1–2 запросами (+1 на выпуск); UC# — через выбор из результатов.
- Повторный поиск той же монеты не тратит запросов (кэш).
- Пакетный режим соблюдает лимит и останавливается на 429.
- N# виден в панели подробностей и открывает страницу Numista.
- Проверка — на нескольких монетах (USA KM#, Ukraine KM#, USSR Y#, монета Германии, монета с UC#, монета с C#), без пакетного прогона по всей коллекции.
- Ветка 12-numista-match смержена в master.

---

## Задача 12a. Колонка N# и отметка монет для проверки в Numista

### Цель

В списке монет сразу видно, какие монеты уже связаны с Numista, а для пакетной проверки пользователь сам отмечает нужные монеты флажками — и понимает, какие именно будут проверены.

### Документация

- Используются те же методы, что в задаче 12: Catalogue → Search the catalogue (`searchTypes`, `GET /types`), Get the issues of a type (`getIssues`, `GET /types/{type_id}/issues`)
- Документация: https://en.numista.com/api/doc/index.php · спецификация: https://en.numista.com/api/doc/swagger.yaml

### Ветка

12a-numista-checks от master (после merge задачи 12); по завершении — merge в master. Коммит: «12a - Колонка N# и отметка монет для Numista».

### Объём работ

**Колонка флажка (первая)**

- Узкая колонка с флажком в каждой строке; в заголовке — флажок «отметить все / снять все» (для строк текущего списка, с учётом дерева и поиска).
- Переключение — клик по флажку или клавиша Space на текущей строке; клик по остальной части строки флажок не меняет.
- Отметки хранятся в памяти по id экземпляра (в БД не пишутся, сбрасываются при закрытии программы); сохраняются при сортировке, смене узла дерева, поиске и F5. Удалённый экземпляр из отметок убирается.
- Строка состояния: «Checked: N» (всего отмеченных, включая не видимые в текущем фильтре).

**Колонка N# (вторая, перед Period)**

- Номер типа Numista; пусто — монета не связана. Если тип связан, а выпуск не определён — номер серым.
- Сортировка по колонке: несвязанные — в начале (по возрастанию).

**Контекстное меню списка (правая кнопка)**

- «Auto-link checked (N)…» (Связать отмеченные автоматически) — пакетная проверка отмеченных монет (бывшая команда Match by catalogue number): без вопроса о лимите (монеты выбраны флажками); больше 10 монет — подтверждение с оценкой запросов (≈ 2 на монету) и счётчиком за месяц; не больше 3 запросов на монету; связывается только однозначный результат, остановка на лимите и на 429; итог как раньше. Недоступна, если ничего не отмечено.
- «Link manually…» (Связать вручную; бывшая Link manually) — поиск для текущей монеты с выбором из вариантов (как в задаче 12).
- «Check all» / «Uncheck all» — для строк текущего списка.
- В меню Numista пункт Match by catalogue number заменяется на тот же «Auto-link checked (N)…».

**Что делает проверка отмеченных**

- Экземпляры одной монеты проверяются один раз.
- Пропускаются (без запросов) и отдельно указываются в итоге: уже связанные с Numista; без номера KM# / Y# / C# (UC# и пустой — через Link manually).
- После проверки с монет, которые удалось связать, флажок снимается; неоднозначные, не найденные и не обработанные (лимит) остаются отмеченными — видно, что с ними ещё работать.

**Выбор выпуска вручную (дополнение)**

- Причина: у типа бывает несколько выпусков одного года, различимых только комментарием Numista (например, Romania 2 lei 1924 KM# 47, N# 5973: «Brussels Mint» и «Poissy Mint (thunderbolt mark)»), — программа их не различает, N# остаётся серым.
- Окно выбора выпуска: колонки год, знак двора, тираж, комментарий; по умолчанию — выпуски года монеты, флажок «Show all years» — все; текущий выпуск выделен. Кнопки: «Link» — связать выбранный выпуск; «Open in Numista» — страница типа; «Type only» — оставить связь только с типом; «Cancel».
- Команда «Choose issue…» (контекстное меню и меню Numista) — для монеты, уже связанной с типом; выпуски — из кэша (`getIssues`, `GET /types/{type_id}/issues`), без кэша — 1 запрос.
- «Link manually…»: если после выбора типа выпуск не определён однозначно — сразу окно выбора выпуска (Cancel — связь только с типом).
- «Auto-link checked» окон не открывает (как раньше).
- Поле Var. карточки не меняется; выпуск хранится в `coins.numista_issue_id`.

### Не входит в задачу

- Сохранение отметок между запусками
- Повторное сопоставление уже связанных монет (только через Link manually)
- Выгрузка в коллекцию Numista (задача 13)

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release) без ошибок и предупреждений.
- Флажки ставятся мышью и клавишей Space, флажок в заголовке отмечает / снимает все строки текущего списка; отметки не теряются при сортировке, фильтре и F5.
- Колонка N# показывает номер связанных монет, сортируется.
- «Auto-link checked» проверяет только отмеченные монеты, до 10 монет — без вопросов, больше — с подтверждением; связанные монеты получают N# в списке и теряют флажок.
- Проверка без расхода квоты (кэш / без ключа) + 2–3 монеты пользователем на настоящем API.
- «Choose issue…» для Romania 2 lei 1924 (N# 5973) показывает 2 выпуска; после выбора N# в списке чёрный.
- Ветка 12a-numista-checks смержена в master.

---

## Задача 13 (Numista-3). Выгрузка экземпляров в коллекцию Numista

### Цель

Добавить отмеченные экземпляры, связанные с типами Numista, в свою коллекцию на Numista без дублей; в списке видно, что уже выгружено.

### Документация

- User → Add an item in the user collection (`addCollectedItems`, `POST /users/{user_id}/collected_items`; тело — JSON: `type` (обязательно), `issue`, `quantity`, `grade` (g, vg, f, vf, xf, au, unc), `price` {`value`, `currency` — ISO 4217}, `acquisition_date` (YYYY-MM-DD), `private_comment`, `internal_id`; ответ 201 — добавленный предмет с `id`). Требует OAuth, scope `edit_collection`.
- User → Get the items owned by a user (`getCollectedItems`, `GET /users/{user_id}/collected_items`; вся коллекция одним ответом: `item_count`, `items[]` с `id`, `type`, `issue`, `internal_id`) — проверка дублей. Scope `view_collection`.
- Для справки (не в этой задаче): `getCollectedItem`, `editCollectedItem`, `deleteCollectedItem` (`/users/{user_id}/collected_items/{item_id}`)
- Документация: https://en.numista.com/api/doc/index.php · спецификация: https://en.numista.com/api/doc/swagger.yaml

### Ветка

13-numista-upload от коммита задачи 12a; по завершении — merge в master. Коммит: «13 - Выгрузка в Numista».

### Решения и ограничения

- Один экземпляр — один запрос (API не принимает пачку). В коллекции 2302 экземпляра при квоте 2000 в месяц — выгрузка частями, по отмеченным (как в 12a).
- Экземпляры одной монеты выгружаются отдельными предметами Numista (у них может быть разное состояние / цена), `quantity` = Quantity экземпляра.
- Токен коллекции — `Authorize()` из задачи 11 (действующий токен не запрашивается заново).

### Объём работ

**Схема** — `coin_items.numista_item_id` (есть с задачи 12, схема v2); новых миграций нет.

**Колонка «в коллекции Numista»** — узкая колонка после N#: ✓ — экземпляр выгружен (есть `numista_item_id`); сортируется. В панели подробностей — строка «In Numista collection: #id предмета».

**Команда «Upload checked (N)…»** (контекстное меню и меню Numista; заменяет Upload selected / Upload all)

1. Отмеченные экземпляры делятся на: к выгрузке (есть N#, нет `numista_item_id`); без N# — пропуск; уже выгружены — пропуск.
2. Больше 10 к выгрузке — подтверждение с числом запросов и счётчиком за месяц (как в 12a); больше, чем осталось квоты за месяц, — отказ с сообщением.
3. Токен (`Authorize`, 0–1 запрос) → `GET /users/{id}/collected_items` (1 запрос, без файлового кэша — коллекция меняется): экземпляр, чей Id уже есть в `internal_id`, получает `numista_item_id` без повторной отправки (защита от дублей после сбоя). Монета уже есть в коллекции Numista, добавленная не программой (предмет без нашего `internal_id`) того же типа — экземпляр не отправляется (решение пользователя); если у обоих известен выпуск и выпуски разные — это не дубль.
4. Для остальных — `POST /users/{id}/collected_items`:
   - `type` = N#, `issue` = выпуск (если определён; серый N# — только тип);
   - `quantity` = Quantity;
   - `grade`: G / VG / F / VF / XF / AU / UNC → g / vg / f / vf / xf / au / unc; P, FR, PRF и пусто — без оценки;
   - `price` = My value в UAH (если > 0; Catalog value не отправляется — это оценка uCoin, а не цена покупки);
   - `acquisition_date` = Purchase (если указана);
   - `private_comment` = Comment; Swap (если есть) — дописывается строкой «Swap: …»;
   - `internal_id` = Id экземпляра в программе.
5. Ответ 201 → `numista_item_id`, флажок с экземпляра снимается. Остановка на 429 и на ошибке сети; ошибка 400 по одному экземпляру — в итог, продолжаем.
6. Итог: выгружено / найдено в коллекции по `internal_id` / уже есть в коллекции (добавлены вручную, флажок остаётся) / без N# / уже были выгружены / ошибки (с текстом первой) / не обработано; запросов потрачено.

**Тело запроса POST** — JSON (`application/json`), `TNumistaClient::Post` (новый метод: заголовки ключа и токена, учёт в счётчике запросов, разбор ошибок как у Get).

### Не входит в задачу

- Изменение и удаление предметов на Numista, обратная синхронизация (из Numista в программу); удаление экземпляра в программе не удаляет его на Numista
- Фото экземпляров (задачи фото)
- Коллекции Numista (`collection`), место хранения, для обмена (`for_swap`)

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release) без ошибок и предупреждений.
- Тест пользователем на 2–3 экземплярах: появляются в коллекции на Numista с правильными типом, выпуском, состоянием, количеством, ценой, датой и комментарием; в списке — ✓, флажки сняты.
- Повторная выгрузка тех же экземпляров не отправляет запросов POST; после ручного сброса `numista_item_id` проверка дублей находит их по `internal_id`.
- Остановка на 429 и проверка квоты перед запуском.
- Ветка 13-numista-upload смержена в master.
---

## Общие решения по фото (задачи 14–16)

- Фото хранятся файлами в `<каталог exe>\Photos\`, в БД — только имя файла (таблица `coin_photos`). BLOB в БД не используем.
- Два вида фото: **фото экземпляра** (ваш снимок своей монеты) и **фото типа** монеты (общее для всех экземпляров; позже — загрузка из Numista). Показывается фото экземпляра, а если его нет — фото типа.
- На каждую сторону (аверс / реверс) — не больше одного фото экземпляра и одного фото типа.
- При добавлении фото уменьшается до 1200 px по длинной стороне и сохраняется в JPEG (≈ 150–300 КБ) стоковым TWICImage. Принимаются JPEG, PNG, BMP, GIF, TIFF (WebP — если в Windows есть кодек).

---

## Задача 13a. Индекс редкости из комментария коллекции Numista

### Цель

Индекс редкости Numista (0–100) в API не отдаётся — пользователь вручную пишет его в Private comment предмета своей коллекции на Numista в формате атрибутов `R:9;`. Программа одним запросом читает коллекцию, извлекает индекс и показывает его в списке монет.

### Документация

- User → Get the items owned by a user (`getCollectedItems`, `GET /users/{user_id}/collected_items`; вся коллекция одним ответом, у предмета — `id`, `type`, `issue`, `private_comment`, `internal_id`). OAuth, scope `view_collection`.
- Индекса редкости нет ни в `getType`, ни в `getIssues` (проверено по спецификации).
- Документация: https://en.numista.com/api/doc/index.php · спецификация: https://en.numista.com/api/doc/swagger.yaml

### Ветка

13a-numista-rarity от коммита задачи 13; по завершении — merge в master. Коммит: «13a - Индекс редкости из Numista».

### Формат комментария (атрибуты)

- Комментарий — пары `Код:значение`, разделённые `;`. Пример: `R:9;S:Am___cos /uid124372;C:Куплена на ярмарке`.
- Код — латинские буквы, регистр не важен; пробелы вокруг `:` и `;` игнорируются; значение — до первого `;` (отделяется от кода первым `:`).
- Коды:
  - `R` — индекс редкости Numista, целое 0–100 (вводит пользователь на сайте);
  - `S` — обмен (от кого получена; бывшее поле Swap uCoin);
  - `C` — свободный комментарий; пишется последним и забирает всё до конца строки, включая `;` и `:`.
- Неизвестные коды и куски без `:` — пропускаются (не ошибка); можно добавлять свои коды.
- Уже выгруженные в задаче 13 предметы с комментарием `Swap: …` читаются без ошибок (код `Swap` — неизвестный, пропускается); индекс в них — после добавления `R:9;` на сайте.

### Объём работ

**Выгрузка (задача 13) — новый формат комментария**

- `private_comment` = `S:<Swap>;C:<Comment>` (пустые части не пишутся), вместо `<Comment>` + строка `Swap: …`.

**Миграция схемы v2 → v3** (автокопия `coins.v2.bak`)

- `coins.numista_rarity` INTEGER — индекс редкости (NULL — не известен). Хранится у монеты (редкость — свойство типа).
- Задачи фото (14–16): их миграция становится v5 → v6 (v4, v5 — задачи 13c, 13d).

**Команда «Numista → Read rarity index from Numista»** (также в контекстном меню списка)

1. Токен (`Authorize`, 0–1 запрос) → `GET /users/{id}/collected_items` (1 запрос, без файлового кэша).
2. Предмет Numista сопоставляется с экземпляром программы по `numista_item_id` (выгружен программой), иначе по `internal_id` = Id экземпляра.
3. Для каждой монеты, у которой хотя бы один экземпляр найден в коллекции: индекс = наибольший из `R` её экземпляров; если ни у одного нет `R` — индекс очищается (пользователь убрал его на сайте). Монеты, не найденные в коллекции, не меняются.
4. Итог: предметов в коллекции Numista / сопоставлено с экземплярами / монет с индексом / индекс изменён / очищен; запросов потрачено.

**Отображение**

- Колонка «R» в списке (после ✓): индекс; сортируется (без индекса — в начале по возрастанию).
- Панель подробностей: индекс не показывается (убран в задаче 13c — достаточно колонки).

### Не входит в задачу

- Запись индекса в комментарий на Numista из программы (`editCollectedItem`) — пользователь пишет его на сайте
- Ручной ввод индекса в карточке монеты
- Редкость для монет, не выгруженных в коллекцию Numista

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release) без ошибок и предупреждений.
- Миграция v2 → v3 с автокопией; данные коллекции не меняются.
- Разбор комментария (самопроверка в Debug-сборке на наборе строк): `R:9;S:Am___cos` → R 9, S «Am___cos»; ` r : 12 ; c: a;b:c` → R 12, C «a;b:c»; `Swap: Am___cos`, `7;S:x`, `R:abc`, `R:150`, пустой → нет индекса; `S:x;C:y` строится обратно в тот же текст.
- Тест пользователем: у 1 lek Albania 1996 (comment `R:9;Swap: …`) после команды — 9 в колонке R и в панели; запросов — 1–2. Новая выгрузка пишет комментарий в формате `S:…;C:…`.
- Ветка 13a-numista-rarity смержена в master.

---

## Задача 13b. Импорт из uCoin сохраняет связи с Numista

### Цель

Повторный импорт из uCoin (File → Import, полная замена коллекции) не теряет то, что сделано через Numista: N#, выпуск, индекс редкости монеты и отметку «выгружено» у экземпляров.

### Причина

`TCoinRepository::ReplaceAll` удаляет все `coins` и `coin_items` и создаёт их заново — поля Numista (задачи 12, 13, 13a) пропадают. Случай 26.10.2026: после импорта из 25 связанных монет и 4 выгруженных экземпляров не осталось ничего (восстановлено разово из автокопии `before-import`).

### Документация

- Новых запросов к Numista нет. Для справки: `internal_id` предмета коллекции — `addCollectedItems` / `getCollectedItems` (https://en.numista.com/api/doc/index.php · https://en.numista.com/api/doc/swagger.yaml)

### Ветка

13b-import-keeps-numista от коммита задачи 13a; по завершении — merge в master. Коммит: «13b - Импорт сохраняет связи с Numista».

### Объём работ

**ReplaceAll: перенос полей Numista**

- До удаления — прочитать монеты с `numista_type_id` / `numista_issue_id` / `numista_rarity` по ключу монеты импорта (период, валюта, номинал, год, разновидность, сюжет, номер — тот же ключ, что в ReplaceAll / FindCoin; id справочников импорт не меняет).
- Новая монета с тем же ключом получает эти поля.
- Экземпляры с `numista_item_id` старой монеты переносятся на экземпляры новой монеты с тем же ключом, сопоставление по полям экземпляра: состояние, количество, My value, дата покупки, комментарий, Swap; если у старой монеты один выгруженный экземпляр, а у новой — один экземпляр, переносится и без совпадения полей.
- Всё в той же транзакции; ошибка — откат всего импорта (как сейчас).
- Итог импорта дополняется: «Numista links kept: монет N, выгруженных экземпляров M; lost: K» (lost — монеты / экземпляры, которых в новом файле нет или сопоставление неоднозначно).

**Выгрузка: предметы с устаревшим internal_id**

- После импорта Id экземпляров меняются, `internal_id` предметов на Numista указывает на старые Id. Чтобы такие предметы не считались «добавленными вручную» (и не блокировали выгрузку других экземпляров того же типа), при проверке дублей предмет считается своим и по `id` предмета = `numista_item_id` любого экземпляра программы.
- Старый `internal_id` может совпасть с Id другого экземпляра после импорта. Поэтому сопоставление по `internal_id` (выгрузка и «Read rarity index from Numista») засчитывается, только если тип Numista предмета совпадает с N# монеты экземпляра; предмет, уже связанный по `numista_item_id`, по `internal_id` не сопоставляется.

### Не входит в задачу

- Сохранение Id экземпляров при импорте (обновление `internal_id` на Numista — `editCollectedItem`)
- Частичный импорт (добавление без замены)

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release) без ошибок и предупреждений.
- Повторный импорт того же файла: все N#, выпуски, индексы R и отметки ✓ на месте; итог импорта показывает число сохранённых связей.
- Импорт файла без монеты: её связь не переносится и учтена в «lost».
- Выгрузка экземпляра того же типа, что и предмет с устаревшим internal_id, — не блокируется как «добавлен вручную».
- Ветка 13b-import-keeps-numista смержена в master.

---

## Задача 13c. Тираж выпуска из Numista

### Цель

Тираж (mintage) выпуска монеты из каталога Numista — в колонке списка (сортировка: малотиражные можно поднять наверх) и в панели подробностей. Данные выпусков хранятся в БД и повторно не запрашиваются.

### Документация

- Catalogue → Get the issues of a type (`getIssues`, `GET /types/{type_id}/issues`): у выпуска `id`, `year`, `gregorian_year`, `min_year`/`max_year` (недатированный), `mint_letter`, `marks`, `mintage`, `comment`. Авторизация — API key (без OAuth). Один запрос на тип — сразу все его выпуски.
- Документация: https://en.numista.com/api/doc/index.php · спецификация: https://en.numista.com/api/doc/swagger.yaml

### Ветка

13c-numista-mintage от коммита задачи 13b; по завершении — merge в master. Коммит: «13c - Тираж из Numista».

### Анализ

- Ответы `getIssues` уже кэшируются в `Exe\Numista\cache` (задачи 12, 12a): сейчас там выпуски 31 типа — для связанных монет тираж в основном читается без запросов.
- Тираж есть не у всех выпусков (пример: 5 bani 1958 — только `comment: "Considered Trial Strike"`).

### Объём работ

**Миграция схемы v3 → v4** (автокопия `coins.v3.bak`)

```
numista_issues (
  id          INTEGER PRIMARY KEY,  -- id выпуска Numista
  type_id     INTEGER NOT NULL,     -- id типа Numista
  year        TEXT,                 -- год как на монете; недатированный — "min–max"
  mint_letter TEXT,
  mintage     INTEGER,              -- NULL — неизвестен
  comment     TEXT,
  updated_at  TEXT NOT NULL         -- дата получения, yyyy-mm-dd
)
INDEX ix_numista_issues_type ON numista_issues (type_id)
```

- Монета ссылается на выпуск существующим `coins.numista_issue_id` (без внешнего ключа: выпуск может быть ещё не загружен).
- Задачи фото (14–16): их миграция становится v5 → v6.

**Сохранение выпусков**

- Любой полученный ответ `getIssues` (автосвязь — задача 12, выбор выпуска — задача 12a, новая команда) сохраняется в `numista_issues`: все выпуски типа, вставка или обновление по `id`.

**Команда «Numista → Update catalogue data»** (также в контекстном меню списка)

1. Типы для обработки — различные `numista_type_id` монет, у которых выбран выпуск, а его нет в `numista_issues`.
2. Для каждого типа — `getIssues` через кэш (ответ из кэша квоту не тратит).
3. Квота: до начала — число типов без кэша; не хватает — сообщение (как в выгрузке, задача 13); больше 10 запросов — подтверждение.
4. Прогресс в строке состояния; сеть / 401 / 429 — остановка, уже полученное сохранено.
5. Итог: типов обработано (из них из кэша), выпусков сохранено, монет с тиражом, монет без тиража у Numista; запросов потрачено.
- Повторный запуск обрабатывает только новые связи.

**Отображение**

- Колонка «Тираж» / «Mintage» после «Rarity»: число с разделителями тысяч (`39 800 000`); «н/д» / «n/a» — выпуск загружен, но у Numista тиража нет; пусто — выпуск не загружен. Сортировка: пусто < н/д < тираж.
- Панель подробностей: «Примечание выпуска» (`comment`); тираж и индекс редкости в панели не показываются — они есть в таблице.

### Не входит в задачу

- Характеристики типа (`getType`) — задача 13d
- Тираж монет без выбранного выпуска (связан только тип) — колонка пуста
- Цены (`getPrices`)
- Принудительное обновление уже загруженных данных

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release) без ошибок и предупреждений.
- Миграция v3 → v4 с автокопией; данные коллекции не меняются.
- Команда на текущей базе: тираж появляется у монет с выпуском; запросы тратятся только на типы без кэша; повторный запуск — 0 запросов.
- 5 bani 1956 (Румыния, KM# 83.2) — тираж 39 800 000.
- Сортировка по колонке «Тираж» в обе стороны.
- Ветка 13c-numista-mintage смержена в master.

---

## Задача 13e. Меню Numista: одна монета / отмеченные

### Цель

В контекстном меню списка сейчас перемешаны команды для выделенной монеты, для отмеченных флажками и для всей коллекции: часть пунктов активна, хотя к выделенной монете отношения не имеет, и кажется, что «не работает». Меню делится на две части — «эта монета» и «отмеченные»; пункт активен, только если применим. «Отметить все / Снять все» переезжают в отдельное меню на ячейке флажка в заголовке. Для одной монеты добавляется обновление данных каталога мимо кэша.

### Документация

- Catalogue → Get the issues of a type (`getIssues`, `GET /types/{type_id}/issues`) — обновление без кэша (задача 13c).
- User → Get the items owned by a user (`getCollectedItems`, `GET /users/{user_id}/collected_items`) — индекс редкости (задача 13a); OAuth, scope `view_collection`.
- User → Add an item to the collection (`addCollectedItems`, `POST /users/{user_id}/collected_items`) — выгрузка (задача 13).
- Catalogue → Search the catalogue (`searchTypes`, `GET /types`) — связывание (задача 12).
- Документация: https://en.numista.com/api/doc/index.php · спецификация: https://en.numista.com/api/doc/swagger.yaml

### Ветка

13e-numista-menus от коммита задачи 13c; по завершении — merge в master. Выполняется перед 13d. Коммит: «13e - Меню Numista: одна монета / отмеченные».

### Объём работ

**Меню заголовка (правый щелчок по ячейке флажка в заголовке)**

- Отдельное меню: «Отметить все в списке», «Снять отметки в списке», «Снять все отметки (N)» (во всей коллекции), разделитель, «Показать отмеченные (N)».
- Правый щелчок по другим ячейкам заголовка — меню нет; по строке монеты — основное меню.
- Из основного меню эти пункты убираются.

**Основное контекстное меню и главное меню Numista — одинаковая структура**

```
1 lek, 1996 — Albania                    (подпись, неактивна)
  Связать вручную...                     есть выделенная монета
  Выбрать выпуск...                      у монеты есть N#
  Выгрузить в Numista                    есть N# и нет ✓
  Прочитать индекс редкости              есть ✓
  Обновить данные каталога (без кэша)    есть N#
──────────
Отмеченные: 12                           (подпись, неактивна)
  Автосвязь                              среди отмеченных есть без N# (номер KM / Y / C проверяет сама команда)
  Выгрузить                              есть отмеченные с N# без ✓
  Прочитать индекс редкости              есть отмеченные с ✓
  Обновить данные каталога               есть отмеченные с выпуском без данных каталога
```

- В главном меню Numista сверху остаётся «Подключение / настройки».
- Подписи частей обновляются при открытии меню (название монеты, число отмеченных); ничего не отмечено — вторая часть неактивна.
- Команды для одной монеты и для отмеченных — разные TAction (у каждой своё условие активности).

**Команды для одной монеты (новые)**

- «Выгрузить в Numista» — та же выгрузка (задача 13), но для экземпляра выделенной строки; проверка дублей — как в 13 / 13b.
- «Прочитать индекс редкости» — `getCollectedItems` (1 запрос + токен), индекс применяется только к выделенной монете.
- «Обновить данные каталога (без кэша)» — `getIssues` типа монеты мимо кэша (1 запрос); ответ перезаписывает кэш и `numista_issues`. Для случая, когда на Numista данные изменились (например, добавили тираж). Итог: тираж до / после.

**Массовые команды — по отмеченным (изменение поведения)**

- «Прочитать индекс редкости» и «Обновить данные каталога» работают по отмеченным монетам, а не по всей коллекции. Вся коллекция — «Отметить все» в меню заголовка, затем команда.
- Индекс по-прежнему читается одним запросом на всю коллекцию Numista, применяется к отмеченным; выгрузка и автосвязь — без изменений.

**Отметки вне текущего списка** (отметки хранятся по всей коллекции, а видна и снимается только текущая страна)

- Узел дерева «Отмеченные (N)» — после «Все»: отмеченные экземпляры из любых стран; счётчик обновляется при каждой отметке. Снятая в этом узле отметка не убирает строку до повторного выбора узла.
- «Снять все отметки (N)» — со всей коллекции; «Показать отмеченные (N)» — переход на узел «Отмеченные».
- Строка состояния и подпись части «Отмеченные» в меню Numista: «Отмечено: 14 (вне списка: 3)», если часть отметок не видна в текущем списке.

### Не входит в задачу

- Массовое обновление данных каталога без кэша
- Новые данные из Numista (характеристики типа — задача 13d)
- Изменение панели инструментов

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release) без ошибок и предупреждений.
- Правый щелчок по ячейке флажка в заголовке — меню «Отметить / снять в списке, снять все, показать отмеченные»; по строке — основное меню без этих пунктов.
- Отмечены 3 монеты Беларуси и 11 Хорватии, показана Хорватия: в строке состояния «Отмечено: 14 (вне списка: 3)»; «Снять все отметки (14)» снимает все; «Отмеченные» в дереве показывает все 14.
- Монета без N#: в части «эта монета» активна только «Связать вручную...».
- 1 lek 1996 (N# 6530, ✓): активны все пункты части «эта монета»; «Обновить данные каталога (без кэша)» тратит 1 запрос, тираж — как на Numista.
- Ничего не отмечено — часть «Отмеченные» неактивна; отмечены только монеты Румынии — индекс и данные каталога меняются только у них.
- Главное меню Numista устроено так же, как контекстное.
- Ветка 13e-numista-menus смержена в master.

---

## Задача 13d. Характеристики типа из Numista

### Цель

Характеристики типа монеты из каталога Numista (металл, вес, размеры, правитель, демонетизация и т. д.) — в панели подробностей; металл — ещё и колонкой списка. Данные хранятся в БД.

### Документация

- Catalogue → Get details about a type (`getType`, `GET /types/{type_id}`). Авторизация — API key. Параметр `lang`: только `en`, `es`, `fr` — значения будут на английском.
- Документация: https://en.numista.com/api/doc/index.php · спецификация: https://en.numista.com/api/doc/swagger.yaml

### Ветка

13d-numista-type-details от коммита задачи 13e; по завершении — merge в master. Коммит: «13d - Характеристики типа из Numista».

### Какие поля берём (анализ ответа getType)

| Поле Numista | Зачем |
|---|---|
| `composition.text` | металл и проба (серебро .925, биметалл) — главное после тиража |
| `weight`, `size`, `size2`, `thickness`, `shape` | физические параметры; из uCoin есть только диаметр |
| `demonetization.is_demonetized`, `demonetization_date` | в обращении монета или нет, с какой даты |
| `ruler[].name` | правитель / период |
| `series`, `commemorated_topic` | серия и памятное событие |
| `min_year`, `max_year` | годы чеканки типа |
| `mints[].name` | монетные дворы |
| `object_type.name` | категория (обиходная, памятная, …) |
| `technique.text` | техника чеканки |

Не берём: `obverse` / `reverse` / `edge` (описания, гравёры, изображения — к задачам фото), `references` (номера по каталогам — есть свои), `tags`, `related_types`, `comments` (HTML), `orientation`.

### Объём работ

**Миграция схемы v4 → v5** (автокопия `coins.v4.bak`)

```
numista_types (
  id                  INTEGER PRIMARY KEY,  -- id типа Numista
  title               TEXT,
  object_type         TEXT,
  min_year            INTEGER,
  max_year            INTEGER,
  composition         TEXT,
  technique           TEXT,
  weight              REAL,
  size                REAL,
  size2               REAL,
  thickness           REAL,
  shape               TEXT,
  is_demonetized      INTEGER,              -- NULL — неизвестно, 0 / 1
  demonetization_date TEXT,
  rulers              TEXT,                 -- имена через "; " (только для показа)
  series              TEXT,
  commemorated_topic  TEXT,
  mints               TEXT,                 -- имена через "; " (только для показа)
  updated_at          TEXT NOT NULL
)
```

- Монета ссылается на тип существующим `coins.numista_type_id`.
- Задачи фото (14–16): их миграция становится v5 → v6.

**Команды «Обновить данные каталога» (задачи 13c, 13e)**

- Для отмеченных: дополнительно типы монет с N#, которых нет в `numista_types`, — `getType` через кэш. Новый тип стоит до 2 запросов (`getType` + `getIssues`); учёт квоты и подтверждение — как в 13c. Итог дополняется: типов с характеристиками сохранено.
- Для одной монеты (без кэша): `getIssues` и `getType` мимо кэша — 2 запроса.
- «Связать вручную» сразу загружает и характеристики типа (`getType`, +1 запрос); ошибка — примечание в итоге, связь сохраняется. Автосвязь отмеченных `getType` не делает (характеристики — массовой командой).

**Отображение**

- Колонка «Металл» / «Composition» после «Тираж» (120 px, форма шире на столько же); сортировка по тексту.
- Панель подробностей — две колонки. Первая (всегда, «—» если пусто): Период, Валюта, Правитель, Сюжет, Номер uCoin, Numista, В коллекции Numista, Примечание выпуска. Вторая (пустые не показываются): Вес (г), Размер × толщина (мм), Форма, Техника, Серия, Памятная тема, Годы чеканки типа, Монетные дворы, Категория, «Демонетизирована: да, с 2002-01-01» / «нет». Металл в панели не показывается — он в таблице. Узкая панель — одна колонка.

**Исправление выгрузки (задача 13)**

- Экземпляр, который уже есть в коллекции Numista, добавленный на сайте вручную (тот же тип и выпуск), не отправляется, а связывается с найденным предметом: `numista_item_id` = его id, в списке появляется ✓. Один предмет сайта связывается не больше чем с одним экземпляром; сначала — с тем же выпуском.

### Не входит в задачу

- Перевод значений на русский / украинский (Numista отдаёт только en / es / fr)
- Изображения и описания сторон — задачи фото
- Цены (`getPrices`)
- Сверка диаметра uCoin с размером Numista

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release) без ошибок и предупреждений.
- Миграция v4 → v5 с автокопией; данные коллекции не меняются.
- Команда на текущей базе: характеристики у всех связанных типов; запросов — не больше 2 на новый тип; повторный запуск — 0 запросов.
- 1 lek Albania 1996: металл, вес, размер, форма и правитель (если есть на Numista) в панели; металл — в колонке.
- Ветка 13d-numista-type-details смержена в master.

---

## Задача 14 (Фото-1). Хранение фото: схема v6, папка Photos, репозиторий

### Цель

Инфраструктура фото без интерфейса: таблица в БД (миграция схемы v5 → v6; v3–v5 — задачи 13a, 13c, 13d), папка с файлами, репозиторий для добавления, получения и удаления фото.

### Ветка

14-photos-storage от коммита задачи 13d; по завершении — merge в master. Коммит: «14 - Хранение фото».

### Объём работ

**Миграция v5 → v6 (u_Database, CurrentSchemaVersion = 6)**

```
coin_photos (
  id         INTEGER PRIMARY KEY,
  id_coin    INTEGER REFERENCES coins(id) ON DELETE CASCADE,       -- фото типа
  id_item    INTEGER REFERENCES coin_items(id) ON DELETE CASCADE,  -- фото экземпляра
  side       TEXT NOT NULL CHECK (side IN ('obverse', 'reverse')),
  file_name  TEXT NOT NULL,                                        -- имя файла в папке Photos
  source     TEXT NOT NULL DEFAULT 'file',                         -- file / numista
  added_at   TEXT NOT NULL,                                        -- 'yyyy-mm-dd hh:nn:ss'
  CHECK ((id_coin IS NULL) <> (id_item IS NULL))                   -- ровно один владелец
)
UNIQUE INDEX ux_coin_photos_item ON coin_photos (id_item, side) WHERE id_item IS NOT NULL
UNIQUE INDEX ux_coin_photos_coin ON coin_photos (id_coin, side) WHERE id_coin IS NOT NULL
```

- Ровно одна ссылка (на экземпляр или на тип) — без дублирования id_coin у фото экземпляра (3НФ).
- Перед миграцией существующей БД — автокопия `coins.v5.bak` (механизм задачи 3).

**Папка фото (u_AppConfig)**

- `[Photos] Path` в CoinsCollection.ini, по умолчанию `<каталог exe>\Photos\`; создаётся при первом добавлении фото.
- Имена файлов: `item_<id>_obverse_<yyyymmddhhnnsszzz>.jpg`, `coin_<id>_reverse_<…>.jpg` — с уникальным суффиксом: после импорта Id меняются, перенесённые фото сохраняют прежние имена, и простое `item_<id>_obverse.jpg` могло бы совпасть с фото другой монеты. При замене фото старый файл удаляется.
- .gitignore: `Exe/Photos/`.

**u_PhotoRepository — TPhotoRepository (новый модуль)**

- `SetItemPhoto(itemId, side, sourceFile)` / `SetCoinPhoto(coinId, side, sourceFile)`: загрузить изображение (TWICImage), уменьшить до 1200 px по длинной стороне, сохранить JPEG в папку Photos, записать / заменить строку в coin_photos. Не картинка — исключение с понятным текстом.
- `SetItemPhoto(..., TGraphic*)` — то же из картинки в памяти (для вставки из буфера обмена в задаче 15).
- `RemoveItemPhoto(itemId, side)` / `RemoveCoinPhoto(coinId, side)` — строка и файл.
- `PhotoFile(itemId, coinId, side, &isTypePhoto)` — полный путь показываемого фото: фото экземпляра, иначе фото типа, иначе пусто.
- Файл, на который ссылается строка, но которого нет на диске, — считается «нет фото» (без ошибок).

**Удаление монет и импорт**

- TCoinRepository::Delete: после удаления экземпляра (и монеты без экземпляров) удалить их файлы фото (строки уходят каскадом).
- Импорт (ReplaceAll) переносит фото так же, как связи Numista (задача 13b): фото типа — на монету с тем же ключом, фото экземпляра — на экземпляр с теми же полями (или на единственный экземпляр монеты). Решение владельца 26.10.2026 — вместо отвязки всех фото.
- Фото без пары (монеты / экземпляра нет в новом файле или сопоставление неоднозначно): строки удаляются, файлы переносятся в `Photos\_before-import-<дата-время>\` (можно восстановить вручную).
- В подтверждении импорта — «Фото в коллекции: N (останутся у тех же монет и экземпляров)»; в итоге — фото сохранено / без пары.

### Не входит в задачу

- Интерфейс добавления и просмотра фото (задачи 15, 16)
- Загрузка фото из Numista
- Несколько фото на сторону, фото «прочее» (гурт, этикетка слаба)

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release) без ошибок и предупреждений.
- Существующая БД версии 5 обновляется до версии 6 с автокопией `coins.v5.bak`; новая БД создаётся сразу версии 6; данные коллекции не меняются.
- Ограничения таблицы работают: нельзя два фото одной стороны у одного владельца, нельзя строку без владельца или с двумя владельцами.
- Добавление фото из файла уменьшает его до ≤ 1200 px и пишет JPEG в Photos; замена и удаление убирают старый файл.
- Удаление экземпляра удаляет его фото (строки и файлы); повторный импорт того же файла сохраняет все фото, фото монет, которых нет в файле, — в `_before-import-…`.
- Ветка 14-photos-storage смержена в master.

---

## Задача 15 (Фото-2). Фото в карточке монеты

### Цель

Добавлять, заменять и удалять фото аверса и реверса прямо в карточке монеты.

### Ветка

15-photos-card от коммита задачи 14; по завершении — merge в master. Коммит: «15 - Фото в карточке».

### Объём работ

- Карточка fmCoinEdit расширяется вправо: блок «Photo» с двумя рамками (аверс, реверс, ≈ 160 × 160).
- У каждой рамки: «Load…» (TOpenPictureDialog), «Paste» (картинка из буфера обмена), «Remove»; перетаскивание файла из Проводника на рамку.
- В рамке — фото экземпляра; если его нет, но есть фото типа — оно с пометкой «coin type photo».
- Флажок «Save as coin type photo (for all items of this coin)» — куда сохранять добавленное фото: к экземпляру (по умолчанию) или к типу.
- Изменения применяются по OK (для новой записи — после её сохранения, когда есть Id); Cancel — ничего не меняется. Новые изображения до OK хранятся в памяти (`TPhotoEdits`), временных файлов нет; карточка только собирает изменения, применяет их главное окно после сохранения записи.
- «Удалить» действует на выбранного флажком владельца: новое (не сохранённое) фото — отменяется, сохранённое — удаляется по OK. Активна, только если у владельца есть фото.
- Ошибка записи фото после сохранения записи — сообщение «Экземпляр сохранён, но фото не сохранены».
- Тексты — en / ru / uk.

### Не входит в задачу

- Фото в главном окне и окно просмотра (задача 16)
- Обрезка, поворот, редактирование изображения

### Критерий приёмки

- Фото добавляются из файла, буфера и перетаскиванием; заменяются и удаляются; Cancel ничего не меняет.
- Фото типа видно у всех экземпляров монеты, пока у экземпляра нет своего.
- Ветка 15-photos-card смержена в master.

---

## Задача 15a. Поиск в Numista: исторические эмитенты

### Цель

Монеты исторических государств находятся по номеру KM / Y / C, даже если у Numista они числятся не у эмитента страны из uCoin. Пример: Poland, 2 silver grosze, 1766, KM# 184 — у Numista эмитент «Polish–Lithuanian Commonwealth» (`pologne-lituanie`), отдельная ветка, не «Poland».

### Причина (анализ 26.10.2026)

- `issuer=pologne` + KM# 184 → 0 результатов.
- Запасной поиск без эмитента → 118 монет с KM# 184 со всего мира; программа читает одну страницу (50, по алфавиту стран — до Польши не доходит) и отбрасывает эмитентов вне ветки страны (`pologne-lituanie` — свой корень, не `poland_section`).

### Документация

- Catalogue → Search the catalogue (`searchTypes`, `GET /types`): параметры `issuer`, `catalogue`, `number`, `date` (год по григорианскому календарю), `count`; у результата — `issuer.code`, `issuer.name`.
- Документация: https://en.numista.com/api/doc/index.php · спецификация: https://en.numista.com/api/doc/swagger.yaml

### Ветка

15a-numista-historical-issuers от коммита задачи 15; по завершении — merge в master. Коммит: «15a - Поиск в Numista: исторические эмитенты».

### Объём работ

**Миграция схемы v6 → v7** (автокопия `coins.v6.bak`)

- `periods.numista_issuer TEXT` — эмитент Numista периода (NULL — как у страны).

**Правила поиска по номеру KM / Y / C (TNumistaMatcher::Search)**

1. Эмитент периода (если запомнен), иначе страны + каталог + номер — как сейчас.
2. Пусто → каталог + номер + `date=<год монеты>` без эмитента (монета без года — без `date`, как сейчас).
3. Результаты из ветки эмитента страны / периода — как сейчас. Таких нет, но есть другие → они не отбрасываются, а помечаются «другой эмитент».
4. Поиск по номиналу (UC#, без номера) — без изменений.

**Связать вручную**

- Результат с пометкой «другой эмитент» — всегда через окно выбора (даже один), в окне видно название эмитента.
- Выбран тип, эмитент которого не входит в ветку эмитента страны, — запоминается у периода монеты (`periods.numista_issuer`); итог: «Эмитент Numista «…» запомнен для периода «…»».

**Автосвязь отмеченных**

- Единственный результат с пометкой «другой эмитент» не связывается; итог: «Другой эмитент — свяжите вручную: N».
- Эмитент периода (п. 1) используется так же, как при ручной связи.

**Справочник периодов**

- Не входит: правка `numista_issuer` периода в форме Reference (пока только автоматически).

### Не входит в задачу

- Чтение следующих страниц результатов (с фильтром по году не нужно)
- Сравнение названий номиналов (поиск UC# / без номера)
- Правка эмитента периода вручную

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release) без ошибок и предупреждений.
- Миграция v6 → v7 с автокопией; данные коллекции не меняются.
- Poland, 2 silver grosze, 1766, KM# 184: «Связать вручную» показывает окно выбора с эмитентом «Polish–Lithuanian Commonwealth»; после выбора эмитент запомнен для периода «Stanisław August Poniatowski».
- Затем 1 grosz 1768 KM# 177 (тот же период) находится автосвязью сразу (шаг 1).
- Автосвязь не связывает монету с единственным результатом другого эмитента.
- Ветка 15a-numista-historical-issuers смержена в master.

---

## Задача 15b. Фраза поиска Numista

### Цель

Монеты, которые не находятся по номеру каталога или находятся «не те» (пример: Poland, 1 ort 1623 — шесть чужих вариантов), ищутся по фразе, как на сайте Numista («Ort koronny - Sigismund III Vasa 1623»). Фраза строится автоматически, пользователь может её исправить в карточке монеты и прямо в окне выбора типа.

### Документация

- Catalogue → Search the catalogue (`searchTypes`, `GET /types?q=…`).
- Документация: https://en.numista.com/api/doc/index.php · спецификация: https://en.numista.com/api/doc/swagger.yaml

### Ветка

15b-numista-search-phrase от коммита задачи 17a; по завершении — merge в master. Коммит: «15b - Фраза поиска Numista».

### Объём работ

**Миграция схемы v8 → v9** (автокопия `coins.v8.bak`)

- `coins.numista_query TEXT` — своя фраза поиска (NULL — автофраза). Импорт переносит её, как связи Numista (задача 13b).

**Автофраза** — номинал, период, год: `1 ort Sigismund III 1623` (без страны и эмитента).

**Карточка монеты** — поле «Numista search / Пошук Numista» в блоке Coin: пустое — автофраза серой подсказкой (обновляется при правке номинала, периода, года); своя фраза — общая для всех экземпляров монеты.

**Связать вручную**

1. Своя фраза — поиск только по ней.
2. Иначе — как раньше (номер KM / Y / C, затем с годом); ничего — поиск по автофразе (результаты — только с подтверждением).
3. Окно выбора открывается всегда, кроме единственного точного результата — в том числе при пустом списке («Nothing found…»).

**Окно Choose Numista type**

- Поле «Search phrase / Фраза пошуку» и кнопка «Search / Пошук» (Enter в поле — поиск): новый поиск — 1 запрос, ответ кэшируется.
- Link — фраза, если её меняли, сохраняется у монеты (совпала с автофразой — очищается).

**Автосвязь**

- Монеты со своей фразой — поиск по ней (и без номера KM / Y / C); единственный результат связывается.
- Автофраза в автосвязи не используется (лишние запросы, неточные результаты).

### Не входит в задачу

- «Умная» автофраза (словарь названий номиналов uCoin → Numista)
- Фильтр результатов фразы по стране / году

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release) без ошибок и предупреждений.
- Миграция v8 → v9 с автокопией; данные коллекции не меняются.
- Poland, 1 ort 1623: в окне выбора фраза меняется на «Ort koronny - Sigismund III Vasa 1623», Search — нужный тип в списке; после Link фраза видна в карточке монеты.
- Монета без результатов — окно выбора с пустым списком и полем фразы.
- Ветка 15b-numista-search-phrase смержена в master.

---

## Задача 16 (Фото-3). Фото в панели подробностей и окно просмотра

### Цель

Показывать фото текущей монеты в панели подробностей главного окна и открывать их крупно.

### Ветка

16-photos-view от коммита задачи 15a; по завершении — merge в master. Коммит: «16 - Просмотр фото».

### Объём работ

- Рамки «Obverse / Reverse» панели подробностей (задача 10) показывают фото текущей записи (экземпляра, иначе типа — с пометкой «coin type photo»); нет фото — «No photo».
- Двойной клик по фото — окно просмотра fmPhotoView: крупное изображение «по размеру окна», колесо мыши — масштаб, переключение аверс / реверс, Esc — закрыть.
- Фото грузится при смене записи; ошибка чтения файла — «No photo», без сообщений.
- Пометка фото типа — в подписи рамки: «Аверс (фото типа)».
- Окно просмотра: кнопки «Аверс / Реверс / По размеру окна», перетаскивание — сдвиг увеличенного фото, ←/→/пробел — другая сторона, 0 / Home — по размеру окна; маленькое фото не увеличивается «по размеру окна».
- Тексты — en / ru / uk.

### Не входит в задачу

- Миниатюры в списке монет
- Слайд-шоу, печать

### Критерий приёмки

- При переходе по списку фото в панели меняются без заметной задержки.
- Окно просмотра открывается по двойному клику, масштабирует и переключает стороны.
- Ветка 16-photos-view смержена в master.

---

## Задача 17 (Фото-4). Фото типа из Numista — подготовка (загрузка отложена)

### Цель

Подготовить хранение для фото типа из каталога Numista. Автоматическая загрузка изображений отложена: сайт Numista отвечает 403 программам (защита от ботов), обходить её нельзя. Вопрос об условиях использования изображений клиентами API — в письме в Numista (26.10.2026).

### Документация

- Catalogue → Get details about a type (`getType`, `GET /types/{type_id}`): `obverse.picture`, `reverse.picture`, `obverse.picture_copyright`, `reverse.picture_copyright`.
- Документация: https://en.numista.com/api/doc/index.php · спецификация: https://en.numista.com/api/doc/swagger.yaml

### Ветка

17-photos-numista от коммита задачи 16; по завершении — merge в master. Коммит: «17 - Подготовка фото типа из Numista».

### Объём работ

**Миграция схемы v7 → v8** (автокопия `coins.v7.bak`)

- `coin_photos.copyright TEXT` — автор фото (для фото Numista).
- `numista_types`: `obverse_picture`, `reverse_picture`, `obverse_copyright`, `reverse_copyright` (TEXT), `pictures_known INTEGER NOT NULL DEFAULT 0` — ссылки на изображения из getType и признак, что они прочитаны.

**Сохранение и показ**

- getType (ручная связь, «Обновить данные каталога») сохраняет ссылки и авторов изображений.
- Импорт (задача 14) переносит и автора фото.
- Окно просмотра (задача 16): у фото с автором — строка «Фото: <автор> / Numista».

**Фото вручную (уже работает, задача 15)**

- Страница типа на Numista (ссылка N# в панели) → в браузере «Копировать изображение» → карточка монеты → «Вставить» с флажком «Сохранить как фото типа».

### Не входит в задачу

- Автоматическая загрузка изображений с сайта Numista — до ответа Numista об условиях
- Обход защиты сайта (подмена клиента под браузер) — не делается

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release) без ошибок и предупреждений.
- Миграция v7 → v8 с автокопией; данные коллекции и фото не меняются.
- После ручной связи у типа в `numista_types` сохранены ссылки на изображения.
- Ветка 17-photos-numista смержена в master.

---

## Задача 18. Помощь (Help) на английском и украинском; без русского языка

### Цель

Встроенная помощь: HTML-страницы на английском и украинском со скриншотами, F1 открывает раздел о текущем окне. Русский язык из программы убирается; язык по умолчанию — английский.

### Ветка

18-help от коммита задачи 17; по завершении — merge в master. Коммит: «18 - Помощь, без русского языка».

### Объём работ

**1. Языки**

- Удалить `Source\Lang\ru.ini` (меню Settings → Language строится по файлам — пункт исчезнет сам) и `Exe\Lang\ru.ini` (копия после сборки).
- В ini `Language=ru` или неизвестный язык — английский (проверить, что так и есть).
- Язык по умолчанию — английский (без ini, `[UI] Language` пуст).
- Комментарии в коде на русском не меняются.

**2. Файлы помощи**

```
Source\Help\
  images\              скриншоты (PNG, английский интерфейс; общие для обоих языков)
  en\  *.html          английский
  uk\  *.html          украинский
  help.css             общий стиль
```

- После сборки копируются в `Exe\Help\` (как `Lang`).
- Обычный HTML, открывается в браузере по умолчанию; без внешних инструментов (CHM не используется).
- В украинской помощи названия элементов интерфейса — «английское/украинское»: *Link manually/Пов'язати вручну*, *Numista → Connect / Settings/Підключення / налаштування*. В английской — только английские.
- На скриншотах — номера-выноски поверх картинки средствами HTML / CSS (картинки не редактируются); под скриншотом — расшифровка номеров.
- Скриншота ещё нет — на его месте рамка с именем файла (страница работает и без картинок).

**Разделы (одинаковые имена файлов в `en\` и `uk\`)**

| Файл | Раздел |
|---|---|
| `index.html` | Содержание |
| `getting-started.html` | Что делает программа, первый запуск, язык, где лежат данные (Db, Photos, ini) |
| `import.html` | Экспорт из uCoin, импорт, что сохраняется при повторном импорте, копии `before-import` |
| `main-window.html` | Панель инструментов, поиск, дерево, колонки, сортировка, панель подробностей, строка состояния |
| `checks.html` | Отметки, меню заголовка, узел «Checked», «outside the list» |
| `coin-card.html` | Добавление, правка, удаление; блоки Coin / My item |
| `photos.html` | Фото экземпляра и типа, файл / буфер / перетаскивание, окно просмотра; фото с Numista вручную |
| `references.html` | Справочники: страны (континент, исчезнувшие, эмитент Numista), периоды, валюты |
| `numista-connect.html` | Ключ API, подключение, квота и счётчик запросов, кэш |
| `numista-link.html` | Link manually, Choose issue, Auto-link, исторические эмитенты (эмитент периода) |
| `numista-upload.html` | Выгрузка в коллекцию, формат комментария `R:` `S:` `C:`, чтение индекса редкости |
| `numista-catalog.html` | Тираж, характеристики типа, Update catalogue data, «без кэша», n/a |
| `faq.html` | Вопросы и проблемы: монета не находится, квота, файл ini занят, фото Numista, восстановление из копии |

**3. Вызов помощи**

- Меню Help → Contents (F1) — `index.html` текущего языка; нет страницы на этом языке — английская.
- F1 в окне — раздел этого окна: главное окно → `main-window`, карточка → `coin-card`, импорт → `import`, справочники → `references`, Numista Settings → `numista-connect`, выбор типа / выпуска → `numista-link`, окно просмотра фото → `photos`.
- Нет папки Help — сообщение «Help files not found: <путь>».

**4. Скриншоты — делает владелец** (английский интерфейс, PNG, папка `Source\Help\images\`, имена — точно как в таблице)

| № | Файл | Что на экране | Раздел |
|---|---|---|---|
| 1 | `01-main-window.png` | Главное окно целиком: дерево раскрыто, выбрана монета с фото и связью Numista, в панели — фото и обе колонки | main-window, getting-started |
| 2 | `02-toolbar-search.png` | Панель инструментов и поле поиска с введённым текстом | main-window |
| 3 | `03-tree.png` | Дерево: раскрытый континент со странами, Extinct states, узел Checked | main-window |
| 4 | `04-columns.png` | Шапка и несколько строк списка: видны #Numista, ✓, Rarity, Mintage (есть n/a), Composition; стрелка сортировки | main-window |
| 5 | `05-details-panel.png` | Панель подробностей крупно: фото (одно — «coin type»), обе колонки | main-window, photos |
| 6 | `06-status-bar.png` | Строка состояния с «Checked: N (outside the list: M)» | checks |
| 7 | `07-header-menu.png` | Меню правого щелчка по ячейке флажка в заголовке | checks |
| 8 | `08-context-menu.png` | Контекстное меню строки: части «монета» и «Checked: N» | numista-link |
| 9 | `09-menu-numista.png` | Открытое меню Numista | numista-connect |
| 10 | `10-import-confirm.png` | Окно подтверждения импорта (строки файла, замена коллекции, фото) | import |
| 11 | `11-import-result.png` | Итог импорта (связи Numista и фото сохранены) | import |
| 12 | `12-coin-card.png` | Карточка монеты целиком, с фото | coin-card |
| 13 | `13-card-photos.png` | Блок Photo карточки крупно: фото, кнопки, флажок «coin type photo» | photos |
| 14 | `14-photo-viewer.png` | Окно просмотра фото, увеличенное | photos |
| 15 | `15-ref-countries.png` | Справочник Countries | references |
| 16 | `16-ref-periods.png` | Справочник Periods | references |
| 17 | `17-numista-settings.png` | Numista → Connect / Settings (**ключ API закрыть / размыть**) | numista-connect |
| 18 | `18-choose-type.png` | Окно выбора типа Numista (несколько вариантов, колонка Issuer) | numista-link |
| 19 | `19-choose-issue.png` | Окно выбора выпуска (годы, двор, тираж) | numista-link |
| 20 | `20-link-result.png` | Итог Link manually (тип, выпуск, характеристики, эмитент периода) | numista-link |
| 21 | `21-autolink-result.png` | Итог Auto-link | numista-link |
| 22 | `22-upload-result.png` | Итог Upload | numista-upload |
| 23 | `23-numista-comment.png` | Сайт Numista: предмет коллекции, Private comment вида `R:9;S:…;C:…` | numista-upload |
| 24 | `24-rarity-result.png` | Итог Read rarity index | numista-upload |
| 25 | `25-catalog-result.png` | Итог Update catalogue data | numista-catalog |
| 26 | `26-ucoin-export.png` | Сайт uCoin: где делается экспорт коллекции в .xlsx | import |
| 27 | `27-numista-api-key.png` | Сайт Numista: страница ключа API (**ключ закрыть**) | numista-connect |

- Ширина — как есть (не уменьшать); личные данные (ключ, токен, e-mail) — закрыть.
- Скриншоты можно добавлять постепенно: страница показывает рамку с именем недостающего файла.

### Не входит в задачу

- Русская помощь, перевод помощи на другие языки
- Видео, анимация, поиск по помощи
- Формат CHM
- Описание функций, которых ещё нет (резервная копия, окно Settings)

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release) без ошибок и предупреждений.
- В Settings → Language только English и Українська; без ini — английский.
- Help → Contents и F1 открывают нужный раздел на текущем языке; в украинской помощи элементы интерфейса — «английский/український».
- Все 13 страниц на обоих языках; с добавленными скриншотами — выноски на своих местах.
- Ветка 18-help смержена в master.

---

