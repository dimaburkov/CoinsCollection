# CoinsCollection — Задачи

Проект: локальный учёт коллекции монет в SQLite с импортом из файла .xlsx (экспорт uCoin.net) и последующей выгрузкой на Numista через API.
Среда разработки: RAD Studio 10.4 (позже 13), C++Builder / VCL.

Порядок работ: каркас → БД (инфраструктура, затем репозитории) → локализация → интерфейс (главное окно, карточка, дерево и фильтры, справочники) → импорт из .xlsx → Numista API.
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

Данные: Europe, Asia, Africa, North America, South America, Australia and Oceania. Antarctica не нужна: суверенных государств там нет, монеты антарктических территорий относятся к их метрополиям. «Extinct states» — не континент: исчезнувшая страна хранит свой реальный континент (СССР — Europe) и is_exist = 0, а узел «Extinct states» в дереве строится по is_exist (задача 7).

**countries** — страна

| Колонка | Тип |
|---|---|
| id | INTEGER PRIMARY KEY |
| name | TEXT NOT NULL UNIQUE COLLATE NOCASE |
| id_continent | INTEGER REFERENCES continents(id) (NULL — не указан) |
| is_exist | INTEGER NOT NULL DEFAULT 1 (0 — страна больше не существует) |

Данные: страны из первого столбца (Country) экспорта `Exe\Collection_uCoin.net.xlsx` без повторов — 187 названий, написание один в один как у uCoin (иначе импорт создаст дубль без континента). Каждой стране назначен континент; 18 исчезнувших государств (USSR, Russian Empire, Czechoslovakia, Yugoslavia, Germany - GDR, Germany - Third Reich, Prussia, Ottoman Empire, Rhodesia, Zaire и др.) — с реальным континентом и is_exist = 0. Страны, которых нет в справочнике, импорт создаёт с id_continent = NULL (узел «No continent», правка — задача 8).

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
  2. CoinId == 0 — найти запись coins с тем же (id_period, id_currency, denomination, year, variety, subject; сравнение NULL-безопасное, `IS`) или создать; иначе UPDATE coins (меняет тип для всех его экземпляров); записать CoinId;
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

## Общие правила для задач интерфейса (5–8)

- Только стоковый VCL: TMainMenu, TActionList, TToolBar, TListView, TTreeView, TSplitter, TStatusBar, TComboBox, TDateTimePicker, TUpDown. Без EhLib, AlphaControls и прочих библиотек PawnShop.
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
| File | Import from uCoin (.xlsx)… | — | disabled (задача импорта) |
| File | Backup database… | — | disabled |
| File | Exit | Alt+F4 | работает |
| Collection | Add | Ins | disabled (задача 6) |
| Collection | Edit | Enter | disabled (задача 6) |
| Collection | Delete | Del | disabled (задача 6) |
| Collection | Need to replace (флажок) | — | disabled (задача 7) |
| Collection | Refresh | F5 | работает: перечитать LoadAll |
| Reference | Countries… / Periods… / Currencies… | — | disabled (задача 8) |
| Numista | Connect / Settings… / Upload selected / Upload all | — | disabled (задача Numista) |
| Settings | Language ▸ | — | работает (из задачи 4) |
| Settings | Settings… | — | disabled |
| Help | About | F1 | работает: имя и версия из versionConfig.h |

**Панель кнопок (TToolBar):** Add, Edit, Delete | Import. Место под поле поиска справа — в задаче 7.

**Список экземпляров (TListView)**

- ViewStyle = vsReport, OwnerData = true (виртуальный режим, данные из std::vector<TCoinRecord>), RowSelect, MultiSelect, ReadOnly, HideSelection = false.
- Колонки: Period, Currency, Denomination, Year, Condition, Number (CatalogNumber), Value (CatalogValueUah, формат `#,##0.00`). Year = 0 и пустые значения — пустая ячейка.
- Клик по заголовку — сортировка по колонке, повторный клик — обратный порядок; стрелка в заголовке (HDF_SORTUP / HDF_SORTDOWN). Condition сортируется по sort_order шкалы, Year и Value — как числа, остальное — как строки без учёта регистра. Сортировка по умолчанию — Denomination, Year.
- Двойной клик / Enter — действие Edit.
- Выделение сохраняется после Refresh (по Id).

**Строка состояния (TStatusBar):** `Items: <число записей>`. Суммы — в задаче 7.

**Прочее**

- Удалить кнопку «DB self-test» и её обработчик.
- Позиция и размер окна, ширины колонок в этой задаче не сохраняются.

### Не входит в задачу

- Карточка монеты, добавление / изменение / удаление (задача 6)
- Дерево стран, поиск, фильтры, суммы (задача 7)
- Справочники (задача 8), импорт, Numista, резервная копия, окно настроек
- Иконки

### Критерий приёмки

- Проект собирается в RAD Studio 13 (Win32, Debug и Release) без ошибок и предупреждений.
- Меню и панель соответствуют таблице; нереализованные пункты неактивны.
- Список показывает записи из БД (проверка на данных из self-test задачи 3a или вставленных вручную); сортировка работает по всем колонкам в обе стороны.
- F5 перечитывает список, выделение сохраняется.
- About показывает имя и версию; все тексты переключаются языком из задачи 4.
- Ветка 5-ui-main смержена в master.

---

## Задача 6 (UI-2). Карточка монеты: добавление, изменение, удаление

### Цель

Форма fmCoinEdit для ввода и правки экземпляра и действия Add / Edit / Delete в главном окне.

### Ветка

6-ui-coin-edit от master (после merge задачи 5); по завершении — merge в master. Коммит: «6 - Карточка монеты».

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
- Delete (Del): подтверждение «Delete N item(s)?» → Delete для каждой выделенной записи → Refresh.
- Enabled: Edit — ровно одна выделенная запись; Delete — хотя бы одна.

### Не входит в задачу

- Копирование экземпляра («ещё один такой же»)
- Фото монет
- Редактирование справочников (задача 8), кроме ввода новой страны / периода / валюты в карточке

### Критерий приёмки

- Добавление, изменение и удаление работают; новые страна / период / валюта из карточки попадают в справочники без дублей.
- Незаполненные Country / Denomination и некорректные числа не дают сохранить.
- Даты и пустые поля сохраняются и читаются без искажений (NULL ↔ пусто).
- Надпись про общие поля монеты показывается только при нескольких экземплярах.
- Ветка 6-ui-coin-edit смержена в master.

---

## Задача 7 (UI-3). Дерево континентов и стран, поиск, фильтры

### Цель

Навигация по коллекции: дерево слева, поиск, фильтр «Need to replace», итоги в строке состояния.

### Ветка

7-ui-filters от master (после merge задачи 6); по завершении — merge в master. Коммит: «7 - Дерево и фильтры».

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

**Список**

- Колонка Country (первой) показывается, когда выбран не узел одной страны (All, континент, Extinct states, No continent); при выборе страны скрывается.

**Поиск и фильтры**

- Поле Search на панели (TEdit + кнопка очистки): подстрока без учёта регистра по Country, Period, Currency, Denomination, Year, Subject, Variety, Number, Comment; применяется при вводе.
- Collection → Need to replace — флажок (AutoCheck): только записи с NeedToReplace.
- Фильтры складываются: узел дерева И поиск И Need to replace. Счётчики в дереве не зависят от поиска.

**Строка состояния**

- `Items: <показано> / <всего>`, `Quantity: <сумма Quantity>`, `Value: <сумма Value × Quantity>` UAH — по показанным записям.

### Не входит в задачу

- Сохранение выбранного узла и фильтров между запусками
- Расширенный фильтр (по годам, состоянию и т.д.)
- Правка континента / is_exist страны (задача 8)

### Критерий приёмки

- Дерево соответствует схеме выше; исчезнувшие страны только в «Extinct states»; счётчики верные.
- Фильтры по дереву, поиску и Need to replace работают вместе; итоги в строке состояния считаются по видимым записям.
- Колонка Country появляется / скрывается по правилу.
- После Add / Edit / Delete дерево и итоги обновляются.
- Ветка 7-ui-filters смержена в master.

---

## Задача 8 (UI-4). Справочники: страны, периоды, валюты

### Цель

Правка справочников: главное — назначить стране континент и признак «существует», чтобы после импорта разложить страны по дереву.

### Ветка

8-ui-reference от master (после merge задачи 7); по завершении — merge в master. Коммит: «8 - Справочники».

### Объём работ

**fmReference — одна форма с TPageControl, вкладки Countries / Periods / Currencies**

- Меню Reference → Countries… / Periods… / Currencies… открывают форму на нужной вкладке.
- Countries: список (Name, Continent, Exists, Items); правка выбранной страны — Name, Continent (TComboBox csDropDownList, первый пункт «—» = нет), Exists (TCheckBox). Фильтр «Only without continent» для быстрого разбора после импорта. Выбор нескольких стран → назначить континент всем сразу.
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

### Критерий приёмки

- Страна получает континент и признак Exists; в главном дереве она сразу переезжает в нужный узел.
- Массовое назначение континента работает.
- Используемую запись удалить нельзя, неиспользуемую — можно; дубли имён не создаются.
- Ветка 8-ui-reference смержена в master.
