# CoinsCollection — Технический документ

Как устроена программа и как с ней работать: сборка, выпуск версии, база, локализация, помощь, Numista. Документ задач (`Docs/Jobs/CoinsCollection-tasks.md`) отвечает на вопрос «что делаем», этот — «как устроено и как делать».

Правило: задача, которая меняет сборку, структуру файлов, схему базы, настройки или порядок выпуска, обновляет и этот документ (в той же ветке).

Собрать .docx (нужен Python 3): `python Docs/Jobs/build_docx.py Docs/Tech/CoinsCollection-tech.md Docs/Tech/CoinsCollection-tech.docx`.

---

## 1. Среда разработки

| Что | Где / версия |
|---|---|
| IDE | RAD Studio 13 (BDS 37.0), C++Builder, VCL, компилятор Clang (Win32) |
| Проект | `Source/_CoinsCollection.cbproj` |
| Инсталлятор | Inno Setup 5: `C:\Program Files (x86)\Inno Setup 5\ISCC.exe` |
| Python | 2.5 — генератор помощи `build_help.py` (`python` в PATH); 3.x — генератор .docx `build_docx.py` (на машине разработчика пока не установлен) |
| База | SQLite, вшит в программу (FireDAC, `FireDAC.Phys.SQLiteWrapper.Stat`) — `sqlite3.dll` не нужен |

Глобальные настройки IDE, пути библиотек и установленные пакеты ради этого проекта не меняются: всё нужное — внутри `.cbproj`.

---

## 2. Структура репозитория

| Папка / файл | Что внутри |
|---|---|
| `Docs/Jobs/` | Документ задач, генератор .docx |
| `Docs/Tech/` | Этот документ |
| `Source/` | Исходники (плоская раскладка), проект, `Lang/`, `Help/` |
| `Source/Lang/` | Переводы интерфейса: `en.ini` (базовый), `uk.ini` |
| `Source/Help/` | Помощь: `build_help.py` (тексты), `en/`, `uk/` (сгенерированные страницы), `images/` (скриншоты), `help.css` |
| `Setup/` | Инсталлятор: `CoinsCollection.iss`, `build-setup.bat`; `Build/` и `Output/` — результаты (в git не попадают) |
| `Exe/` | Рабочая папка разработчика: exe, база, фото, ini, кэш Numista (в git не попадает) |

---

## 3. Модули

| Модуль | Назначение |
|---|---|
| `_CoinsCollection.cpp` | Точка входа: загрузка настроек, переводов, создание форм |
| `dm_Data` | Датамодуль: соединение FireDAC с SQLite |
| `u_AppConfig` | Настройки (`CoinsCollection.ini` рядом с exe), пути базы и фото |
| `u_Database` | Открытие базы, лестница миграций схемы, автокопии `.bak` |
| `u_SeedData` | Начальные справочники (континенты, страны uCoin, шкала состояний) |
| `u_CoinTypes` | Типы данных (`TCoinRecord`, `TNumistaIssue`, `TNumistaTypeInfo` …), `YearText` |
| `u_CoinRepository` | Коллекция: чтение, сохранение, удаление, импорт uCoin с переносом связей и фото |
| `u_LookupRepository` | Справочники: страны, периоды, валюты, состояния, эмитенты |
| `u_PhotoRepository` | Фото: файлы `Photos\*.jpg`, таблица `coin_photos` |
| `u_ImportSource`, `u_CoinImporter` | Чтение экспорта uCoin `.xlsx` |
| `u_NumistaClient` | HTTP к Numista API (ключ, OAuth, счётчик запросов, кэш, подпись программы, загрузка картинок) |
| `u_NumistaMatcher` | Поиск типов и выпусков, эмитенты, `getType` |
| `u_NumistaUploader` | Выгрузка экземпляров в коллекцию Numista |
| `u_NumistaComment` | Разбор / сборка Private comment (`R:` `S:` `C:`), самопроверка |
| `u_Backup` | Резервная копия (zip) и чтение копии |
| `u_Translator` | Переводы (`Tr()`, перевод форм) |
| `u_Help` | Помощь: открыть тему, F1 в окнах |
| `versionConfig` | Имя и версия программы (`AppInfo`) |
| `f_Main` | Главное окно, все команды меню |
| `f_CoinEdit` | Карточка монеты |
| `f_Reference` | Справочники |
| `f_NumistaSettings`, `f_NumistaChoose`, `f_NumistaIssue`, `f_NumistaImport` | Окна Numista |
| `f_PhotoView`, `f_Progress`, `f_About` | Просмотр фото, окно хода, «О программе» (формы без .dfm) |

Формы без `.dfm` создаются конструктором `TForm(AOwner, 0)` и строят элементы в коде.

---

## 4. Данные программы

Программа хранит всё рядом с exe — поэтому папка установки должна быть доступна для записи (не Program Files).

| Путь | Что |
|---|---|
| `Db\coins.db` | База (имя по умолчанию; другое — `[Database] Path` в ini) |
| `Db\*.vN.bak` | Автокопия перед обновлением схемы с версии N |
| `Db\*.before-import-<время>.bak` | Автокопия перед импортом uCoin |
| `Db\*.before-restore-<время>.bak` | Автокопия перед восстановлением из резервной копии |
| `Photos\*.jpg` | Фото (имя — владелец, сторона, время: `item_123_obverse_<время>.jpg`) |
| `Photos\_before-import-…\`, `Photos\_before-restore-…\` | Фото, убранные импортом / восстановлением |
| `Numista\` | Кэш: справочники (`issuers.json`, `catalogues.json`), ответы API (`cache\<md5 адреса>.json`) |
| `CoinsCollection.ini` | Настройки (раздел 5) |
| `Lang\`, `Help\` | Переводы и помощь (ставятся инсталлятором) |

---

## 5. Настройки (CoinsCollection.ini)

| Раздел / ключ | Значение |
|---|---|
| `[Database] Path` | Путь к базе (пусто — `Db\coins.db`); относительный — от папки exe |
| `[Photos] Path` | Папка фото (пусто — `Photos\`) |
| `[UI] Language` | `en` (по умолчанию) / `uk` |
| `[Numista] ApiKey` | Ключ API пользователя |
| `[Numista] AccessToken`, `TokenExpires`, `UserId` | OAuth (client credentials) |
| `[Numista] RequestsMonth`, `RequestsCount`, `MonthlyQuota` | Счётчик запросов за месяц, квота (2000) |
| `[Numista] AutoLinkDetails` | Автосвязь сразу загружает характеристики и фото |
| `[Backup] Folder`, `Last`, `Asked` | Папка последней копии, дата копии, дата напоминания |

---

## 6. Схема базы и миграции

Версия схемы — `PRAGMA user_version`; текущая — `TDatabase::CurrentSchemaVersion`.

| Версия | Задача | Что добавлено |
|---|---|---|
| 1 | 3 | Нормализованная схема: справочники, `coins` (тип), `coin_items` (экземпляр) |
| 2 | 12 | Связь с Numista: эмитент страны, тип и выпуск монеты, предмет коллекции у экземпляра |
| 3 | 13a | `coins.numista_rarity` — индекс редкости |
| 4 | 13c | `numista_issues` — выпуски (тираж, примечание) |
| 5 | 13d | `numista_types` — характеристики типа |
| 6 | 14 | `coin_photos` — фото |
| 7 | 15a | `periods.numista_issuer` — эмитент периода |
| 8 | 17 | Автор фото, ссылки на изображения типа |
| 9 | 15b | `coins.numista_query` — своя фраза поиска |
| 10 | 20 | `coins.year_to` — диапазон лет, `coin_items.origin` — монеты из Numista |

**Как добавить версию:**

- `u_Database.h`: `CurrentSchemaVersion = N`, объявить `MigrateToN()`.
- `u_Database.cpp`: строка лестницы `if (version < N) { MigrateToN(); version = N; }` и сам метод (только `ALTER TABLE` / `CREATE …`; данные пользователей не удаляются).
- Автокопия `<имя>.v<N-1>.bak` делается сама перед миграцией.
- Обновить таблицу выше и, если меняется ключ монеты (поля, по которым импорт uCoin находит «ту же» монету), — `FindCoin`, `ReplaceAll`, `CoinKey` в `u_CoinRepository.cpp`.
- Проверка: копия рабочей базы в отдельную папку, запуск тестовой сборки оттуда, `PRAGMA user_version` и число записей.

---

## 7. Сборка

**Конфигурации**

| | Debug | Release |
|---|---|---|
| Runtime packages | да (`UsePackages=true`, `DynamicRTL=true`) — быстрая сборка; exe работает только там, где есть RAD Studio | нет — статическая сборка (`AllPackageLibs`: rtl, vcl, dbrtl, xmlrtl, FireDAC*, vclFireDAC, vclimg), один exe ~8 МБ, зависит только от системных DLL Windows |
| Где используется | ежедневная разработка (IDE) | инсталлятор |

Блок `<PackageImport>` в `.cbproj` подключается только при `UsePackages=true` (иначе статическая сборка всё равно тянула бы `.bpl`).

**Из командной строки**

- `call "C:\Program Files (x86)\Embarcadero\Studio\37.0\bin\rsvars.bat"`
- `msbuild Source\_CoinsCollection.cbproj /t:Build /p:Config=Debug /p:Platform=Win32` — в `Exe\`
- Чтобы не трогать рабочий `Exe\` (программа может быть запущена): `/p:FinalOutputDir=<другая папка>\`.

**После сборки (PostBuildEvent)** копирует в папку exe: `Lang\*.ini`, `Help\help.css`, `Help\en\`, `Help\uk\`, `Help\images\*.png`; удаляет устаревший `Lang\ru.ini`. Команды — **в одну строку через `&&`** (RAD Studio склеивает многострочное событие без разделителей → ошибка xcopy, код 4); `if exist …` — в скобках.

**Требование к сборке:** Debug и Release — 0 ошибок и 0 предупреждений.

**Зависимости exe:** `"C:\Program Files (x86)\Embarcadero\Studio\37.0\bin\tdump.exe" <exe>` → строки «Imports from»: у Release не должно быть `*.bpl`.

---

## 8. Выпуск версии (инсталлятор)

**Номер версии** — `a.b.c` (в exe — `a.b.c.0`). Меняется в трёх местах, одинаково:

- `Source/versionConfig.cpp` — `AppInfo::Version` (`L"0.2.0.0"`);
- `Source/_CoinsCollection.cbproj` — `VerInfo_MajorVer` / `VerInfo_MinorVer` / `VerInfo_Release`, `FileVersion` и `ProductVersion` в `VerInfo_Keys`;
- `Setup/CoinsCollection.iss` — `#define AppVersion "0.2.0"`.

**Порядок выпуска**

- Все задачи версии закоммичены и смержены в master; рабочее дерево чистое.
- Помощь актуальна: если менялись тексты — `python build_help.py` в `Source\Help`; скриншоты — в `Source\Help\images\`.
- Поднять версию (три места выше), закоммитить «Версия a.b.c».
- Запустить `Setup\build-setup.bat` (двойной клик или из консоли): Rebuild Release → `Setup\Build\` (exe, Lang, Help) → ISCC → `Setup\Output\CoinsCollection-Setup-a.b.c.exe`. Рабочий `Exe\` не трогается.
- Проверить установку (ниже), отправить файл из `Setup\Output\`.

**Проверка инсталлятора на своём компьютере (без ярлыков, во временную папку)**

- Установка: `CoinsCollection-Setup-a.b.c.exe /VERYSILENT /SUPPRESSMSGBOXES /NOICONS /LANG=ukrainian /DIR="C:\Temp\cc-test"`.
- Запустить `C:\Temp\cc-test\CoinsCollection.exe`: открывается, пустая коллекция, язык — как у мастера.
- Удаление: `C:\Temp\cc-test\unins000.exe /VERYSILENT` — должны остаться только `Db\` и `CoinsCollection.ini`.

**Публикация на GitHub** (репозиторий публичный, лицензия MIT; README — страница загрузки)

- Текст релиза — `Setup/ReleaseNotes/<версия>.md` (en + uk, что нового, установка, SmartScreen, SHA-256).
- SHA-256 инсталлятора: PowerShell `Get-FileHash Setup\Output\CoinsCollection-Setup-a.b.c.exe` — вписать в текст релиза (после финальной сборки: каждая сборка меняет сумму).
- github.com → репозиторий → **Releases → Draft a new release**: тег `va.b.c` на коммите master, заголовок «Coins Collection a.b.c», описание — из `ReleaseNotes`, в *Assets* — инсталлятор из `Setup\Output\`. **Publish release**.
- Ссылка для пользователей всегда одна: https://github.com/dimaburkov/CoinsCollection/releases/latest

**Проверка на «чистом» компьютере** (без RAD Studio) — перед первой раздачей и после изменений сборки: установка, первый запуск, импорт uCoin, подключение Numista, установка поверх, удаление.

**Что делает инсталлятор** (`Setup/CoinsCollection.iss`)

- Для текущего пользователя, без прав администратора; по умолчанию `%LOCALAPPDATA%\Programs\CoinsCollection`; папку внутри Program Files не принимает.
- Ставит: `CoinsCollection.exe` (из `_CoinsCollection.exe`), `Lang\`, `Help\`. Не ставит и не удаляет: `Db\`, `Photos\`, ini, кэш Numista.
- Язык мастера — English / Українська; язык программы — как у мастера (если ещё не выбран).
- `AppId` (GUID в .iss) не менять — по нему установка поверх находит прежнюю.
- Файл .iss — **UTF-8 с BOM** (Inno Setup 5 иначе читает его как ANSI, украинские тексты ломаются).
- Инсталлятор не подписан: SmartScreen может предупредить («Подробнее → Выполнить в любом случае»).

---

## 9. Локализация

- Базовый язык — английский: тексты в `.dfm` и `Source/Lang/en.ini`; перевод — `uk.ini` (русский убран, задача 18).
- Строки из кода — `Tr(L"Модуль.Назначение")`, раздел `[Strings]`; подписи форм — раздел `[<имя формы>]`, ключ `<компонент>.<свойство>`.
- Литералы на русском / украинском в коде запрещены (кроме комментариев).
- В ini нет экранирования: `\r\n` в значении — это четыре символа. Многострочное сообщение — несколько ключей, склеенных в коде.
- Нет ключа в `uk.ini` — берётся английский (`en.ini`). В Debug-сборке отсутствующие ключи можно найти `TTranslator::ReportMissingKeys`.
- Язык по умолчанию — `en`; список языков — по файлам `Lang\*.ini` (`[Language] Code`, `Name`).

---

## 10. Помощь

- Тексты всех страниц — в `Source/Help/build_help.py`; страницы `en/*.html`, `uk/*.html` **генерируются**: `python build_help.py` в `Source\Help`. HTML руками не править.
- Разметка в текстах: `[[ui:English|Українська]]` — элемент интерфейса (в украинской помощи — «English/Українська»), `[[shot:file.png|подпись]]` — скриншот (нет файла — рамка с именем), `[[page:id|текст]]` — ссылка.
- Скриншоты — английский интерфейс, `Source/Help/images/NN-name.png`; ключ API и личные данные закрывать.
- F1: `InstallHelpKey(this, L"<тема>")` в конструкторе формы; Help → Contents — `ShowHelp(L"index")`. Нет страницы на текущем языке — английская.

---

## 11. Numista

- Документация API: https://en.numista.com/api/doc/index.php · спецификация: https://en.numista.com/api/doc/swagger.yaml. Ключ пользователь получает сам: https://en.numista.com/api/api_key.php
- Каждый пользователь — свой ключ; бесплатно 2000 запросов в месяц. Программа считает запросы (`[Numista] RequestsCount`) и проверяет квоту перед массовыми операциями.
- Подпись всех запросов (API и картинок): `CoinsCollection/<версия> (uCoin to Numista migration tool)` — `TNumistaClient::UserAgent()`. Префикс `CoinsCollection/` внесён Numista в белый список (переписка с основателем Numista, 27–28.10.2026); без него сайт отвечает 403 на картинки. Под браузер не маскироваться.
- Разрешено (та же переписка): бесплатно раздавать программу с пустой базой, каждый пользователь — со своим ключом; хранить данные каталога в локальной базе.
- Кэш ответов каталога (`searchTypes`, `getIssues`, `getType`) — в `Numista\cache\`, бессрочно; «без кэша» — только команда для одной монеты.
- Изображения (`obverse/reverse.picture`, `pictures` предмета) — файлы `*.numista.com`, квоту не тратят; хранятся как фото с источником `numista` и автором.
- Индекс редкости в API нет (запрошено у Numista 28.10.2026): пользователь пишет его в Private comment предмета.

**Private comment** (`u_NumistaComment`): атрибуты `Код:значение` через `;` — `R` индекс редкости 0–100, `S` обмен, `C` комментарий (последний, до конца строки). Индекс можно записать и числом в начале: `97`, `97;текст`. Самопроверка `TNumistaComment::SelfTest` выполняется при каждом запуске Debug-сборки (ошибка — окно «Debug»).

---

## 12. Порядок работы над задачей

- Текст задачи — в `Docs/Jobs/CoinsCollection-tasks.md` (цель, документация, ветка, объём, не входит, критерий приёмки); владелец одобряет текст до кода.
- Ветка от предыдущего коммита: `<номер>-<кратко>` (например `21-backup`); коммиты и merge делает владелец.
- Перед сдачей: Debug и Release — 0 ошибок и 0 предупреждений; миграция — на копии базы; окна, собранные кодом, — снимок окна в тестовой копии.
- Новые файлы — добавить в `.cbproj` (`CppCompile`, `BuildOrder`) и сообщить владельцу (git add).
