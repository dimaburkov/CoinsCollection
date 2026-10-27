# -*- coding: utf-8 -*-
# Сборка страниц помощи (задача 18): Help\en\*.html и Help\uk\*.html из текстов ниже.
# Запуск (Python 2.5+): python build_help.py   — из папки Source\Help.
# Разметка в текстах:
#   [[ui:English|Українська]]  — элемент интерфейса: en — "English", uk — "English/Українська"
#   [[shot:file.png|подпись]]  — скриншот из images\ (нет файла — рамка с именем)
#   [[page:id|текст]]          — ссылка на другую страницу помощи
import re, os

LANGS = ('en', 'uk')
APP = {'en': u'Coins Collection — Help', 'uk': u'Колекція монет — Довідка'}
NAV = {'en': (u'Contents', u'Українською'), 'uk': (u'Зміст', u'In English')}
MISSING = {'en': u'Screenshot', 'uk': u'Скриншот'}

PAGES = [
 # id, title_en, title_uk
 ('index', 'Contents', 'Зміст'),
 ('getting-started', 'Getting started', 'Початок роботи'),
 ('import', 'Import from uCoin', 'Імпорт з uCoin'),
 ('main-window', 'Main window', 'Головне вікно'),
 ('checks', 'Checking coins', 'Позначки монет'),
 ('coin-card', 'Coin card', 'Картка монети'),
 ('photos', 'Photos', 'Фото'),
 ('references', 'Reference lists', 'Довідники'),
 ('numista-connect', 'Numista: connection and requests', 'Numista: підключення та запити'),
 ('numista-link', 'Numista: linking coins', "Numista: зв'язок монет"),
 ('numista-upload', 'Numista: upload and rarity index', 'Numista: вивантаження та індекс рідкісності'),
 ('numista-catalog', 'Numista: mintage and type details', 'Numista: тираж і характеристики'),
 ('faq', 'Questions and problems', 'Питання та проблеми'),
]

T = {}  # (id, lang) -> body

# =====================================================================
T['index', 'en'] = u"""
<p>Coins Collection keeps your coin collection on your computer and helps you move it from
<b>uCoin</b> to <b>Numista</b>: it imports the uCoin export file, finds every coin in the Numista
catalogue and adds it to your Numista collection.</p>
<ol class="toc">
[[toc]]
</ol>
<div class="note">Press <b>F1</b> in any window to open the help page about that window.</div>
"""
T['index', 'uk'] = u"""
<p>«Колекція монет» зберігає вашу колекцію монет на комп'ютері й допомагає перенести її з
<b>uCoin</b> на <b>Numista</b>: імпортує файл експорту uCoin, знаходить кожну монету в каталозі Numista
і додає її до вашої колекції на Numista.</p>
<ol class="toc">
[[toc]]
</ol>
<div class="note">Назви елементів програми подано так: <b>англійською/українською</b> — наприклад,
[[ui:Link manually|Пов'язати вручну]]. Натисніть <b>F1</b> у будь-якому вікні, щоб відкрити довідку про це вікно.</div>
"""

# =====================================================================
T['getting-started', 'en'] = u"""
[[shot:01-main-window.png|The main window: countries on the left, the coin list, coin details and photos below.]]
<h2>First steps</h2>
<ol>
<li>Export your collection from uCoin to an <code>.xlsx</code> file and import it: [[page:import|Import from uCoin]].</li>
<li>Register on Numista, get your own API key and enter it in [[ui:Numista → Connect / Settings|Numista → Підключення / налаштування]]: [[page:numista-connect|Numista: connection]].</li>
<li>Link your coins with the Numista catalogue: [[page:numista-link|Numista: linking coins]].</li>
<li>Upload them to your Numista collection: [[page:numista-upload|Numista: upload]].</li>
</ol>
<h2>Language</h2>
<p>[[ui:Settings → Language|Налаштування → Мова]] — English or Ukrainian. English is used by default.</p>
<h2>Where the data is</h2>
<table>
<tr><th>Folder / file next to the program</th><th>What is inside</th></tr>
<tr><td><code>Db\\</code></td><td>The collection database and its automatic backup copies (<code>*.bak</code>): before every database upgrade and before every import.</td></tr>
<tr><td><code>Photos\\</code></td><td>Coin photos (JPEG). The database keeps only the file names.</td></tr>
<tr><td><code>Numista\\</code></td><td>Cached Numista answers: a coin type that was found once is never requested again.</td></tr>
<tr><td><code>CoinsCollection.ini</code></td><td>Settings: language, your Numista API key, request counter for the month.</td></tr>
</table>
<div class="note">To move the collection to another computer, copy the whole program folder.</div>
"""
T['getting-started', 'uk'] = u"""
[[shot:01-main-window.png|Головне вікно: країни ліворуч, список монет, подробиці про монету та фото внизу.]]
<h2>Перші кроки</h2>
<ol>
<li>Експортуйте колекцію з uCoin у файл <code>.xlsx</code> та імпортуйте його: [[page:import|Імпорт з uCoin]].</li>
<li>Зареєструйтеся на Numista, отримайте власний ключ API та введіть його в [[ui:Numista → Connect / Settings|Numista → Підключення / налаштування]]: [[page:numista-connect|Numista: підключення]].</li>
<li>Пов'яжіть монети з каталогом Numista: [[page:numista-link|Numista: зв'язок монет]].</li>
<li>Вивантажте їх у свою колекцію на Numista: [[page:numista-upload|Numista: вивантаження]].</li>
</ol>
<h2>Мова</h2>
<p>[[ui:Settings → Language|Налаштування → Мова]] — English або Українська. Без налаштування — англійська.</p>
<h2>Де зберігаються дані</h2>
<table>
<tr><th>Папка / файл поруч із програмою</th><th>Що всередині</th></tr>
<tr><td><code>Db\\</code></td><td>База колекції та її автоматичні резервні копії (<code>*.bak</code>): перед кожним оновленням бази й перед кожним імпортом.</td></tr>
<tr><td><code>Photos\\</code></td><td>Фото монет (JPEG). У базі — лише імена файлів.</td></tr>
<tr><td><code>Numista\\</code></td><td>Збережені відповіді Numista: тип монети, знайдений один раз, більше не запитується.</td></tr>
<tr><td><code>CoinsCollection.ini</code></td><td>Налаштування: мова, ваш ключ API Numista, лічильник запитів за місяць.</td></tr>
</table>
<div class="note">Щоб перенести колекцію на інший комп'ютер, скопіюйте всю папку програми.</div>
"""

# =====================================================================
T['import', 'en'] = u"""
[[shot:08-ucoin-export.png|uCoin: exporting the collection to an .xlsx file.]]
<h2>Export from uCoin</h2>
<p>On the uCoin website open your collection and export it to Excel (<code>.xlsx</code>). Save the file on your computer.</p>
<h2>Import</h2>
<ol>
<li>[[ui:File → Import from uCoin (.xlsx)|Файл → Імпорт з uCoin (.xlsx)]] (or the [[ui:Import|Імпорт]] button) and choose the file.</li>
<li>The program shows how many rows were read and which rows have errors (skipped). Confirm the import.</li>
<li>The current collection is <b>replaced</b> by the file. A backup copy of the database is made first: <code>Db\\…before-import-&lt;date&gt;.bak</code>.</li>
</ol>
<h2>What is kept on a repeated import</h2>
<p>You can import a newer uCoin export at any time. For coins that are in the new file, the program keeps:</p>
<ul>
<li>Numista links (#Numista, issue, rarity index, Numista search phrase) and the ✓ of uploaded items;</li>
<li>photos of coins and items.</li>
</ul>
<p>The result shows how many links and photos were kept and lost. Photos of coins that are no longer in the file are moved to <code>Photos\\_before-import-&lt;date&gt;\\</code>.</p>
<div class="note">Countries are matched by name. If you rename a country in [[page:references|Reference lists]], the old uCoin name will appear again after the next import.</div>
<h2>Import from your Numista collection</h2>
<p>Coins that you added on Numista yourself and that are not in uCoin (for example ancient coins) can be imported too:
[[ui:Numista → Import from my Numista collection|Numista → Імпорт з моєї колекції Numista]].</p>
<ol>
<li>The window lists the items of your Numista collection that are not in the program yet.</li>
<li>Filter by [[ui:Issuer|Емітент]] (for example Roman Empire), check the items and press [[ui:Import|Імпортувати]].</li>
<li>For each item the program creates the coin (country by the Numista issuer, period by the ruler, currency, denomination,
years, catalogue number) and your item (grade, quantity, purchase date, comment), links it with Numista (✓) and loads
the type details and photos — the type photos and your own photos of the item on Numista.</li>
</ol>
<p>A new country (for example <i>Roman Empire</i>) appears under [[ui:No continent|Без континенту]]: set its continent and
“extinct” in [[page:references|Reference lists]]. Years can be a range (<code>174–179</code>) and before Christ (<code>44 BC</code>).
A price in another currency than UAH is added to the comment.</p>
<div class="note">Coins imported from Numista are kept when you import a uCoin file again.</div>
"""
T['import', 'uk'] = u"""
[[shot:08-ucoin-export.png|uCoin: експорт колекції у файл .xlsx.]]
<h2>Експорт з uCoin</h2>
<p>На сайті uCoin відкрийте свою колекцію та експортуйте її в Excel (<code>.xlsx</code>). Збережіть файл на комп'ютері.</p>
<h2>Імпорт</h2>
<ol>
<li>[[ui:File → Import from uCoin (.xlsx)|Файл → Імпорт з uCoin (.xlsx)]] (або кнопка [[ui:Import|Імпорт]]) і виберіть файл.</li>
<li>Програма показує, скільки рядків прочитано і в яких рядках помилки (вони пропускаються). Підтвердьте імпорт.</li>
<li>Поточна колекція <b>замінюється</b> файлом. Перед цим робиться копія бази: <code>Db\\…before-import-&lt;дата&gt;.bak</code>.</li>
</ol>
<h2>Що зберігається при повторному імпорті</h2>
<p>Новіший експорт uCoin можна імпортувати будь-коли. Для монет, які є в новому файлі, програма зберігає:</p>
<ul>
<li>зв'язки з Numista (#Numista, випуск, індекс рідкісності, фразу пошуку Numista) і ✓ вивантажених примірників;</li>
<li>фото монет і примірників.</li>
</ul>
<p>У підсумку видно, скільки зв'язків і фото збережено та втрачено. Фото монет, яких більше немає у файлі, переносяться до <code>Photos\\_before-import-&lt;дата&gt;\\</code>.</p>
<div class="note">Країни зіставляються за назвою. Якщо перейменувати країну в [[page:references|Довідниках]], стара назва з uCoin з'явиться знову після наступного імпорту.</div>
<h2>Імпорт з вашої колекції Numista</h2>
<p>Монети, які ви самі додали на Numista і яких немає в uCoin (наприклад античні), теж можна імпортувати:
[[ui:Numista → Import from my Numista collection|Numista → Імпорт з моєї колекції Numista]].</p>
<ol>
<li>У вікні — предмети вашої колекції Numista, яких ще немає в програмі.</li>
<li>Відберіть за [[ui:Issuer|Емітент]] (наприклад Roman Empire), позначте предмети й натисніть [[ui:Import|Імпортувати]].</li>
<li>Для кожного предмета програма створює монету (країна — за емітентом Numista, період — за правителем, валюта, номінал,
роки, номер у каталозі) і ваш примірник (стан, кількість, дата купівлі, коментар), пов'язує його з Numista (✓) і завантажує
характеристики типу та фото — фото типу й ваші власні фото предмета на Numista.</li>
</ol>
<p>Нова країна (наприклад <i>Roman Empire</i>) з'являється в [[ui:No continent|Без континенту]]: вкажіть її континент і
«зникла держава» в [[page:references|Довідниках]]. Роки можуть бути діапазоном (<code>174–179</code>) і до нашої ери (<code>44 до н. е.</code>).
Ціна в іншій валюті, ніж гривня, додається до коментаря.</p>
<div class="note">Монети, імпортовані з Numista, зберігаються при повторному імпорті файлу uCoin.</div>
"""

# =====================================================================
T['main-window', 'en'] = u"""
[[shot:01-main-window.png|The main window.]]
<h2>Navigation tree</h2>
<p>On the left: [[ui:All|Усі]], continents with their countries, [[ui:No continent|Без континенту]],
[[ui:Extinct states|Зниклі держави]] and [[ui:Checked|Позначені]] (all checked coins from any country, see [[page:checks|Checking coins]]).
The number in brackets is the number of items.</p>
<h2>Search</h2>
<p>The search field on the toolbar filters the list by country, period, currency, denomination, year, subject, variety, catalogue number and comment.</p>
<h2>Coin list</h2>
<table>
<tr><th>Column</th><th>Meaning</th></tr>
<tr><td>☐</td><td>Check box for mass operations ([[page:checks|Checking coins]]).</td></tr>
<tr><td>Country, Period, Currency, Denomination</td><td>From uCoin. Country is hidden when one country is selected in the tree.</td></tr>
<tr><td>#Numista</td><td>Numista type number (N#). Grey — the issue is not chosen yet.</td></tr>
<tr><td>✓</td><td>The item is uploaded to your Numista collection.</td></tr>
<tr><td>Rarity</td><td>Rarity index from your Numista collection comment ([[page:numista-upload|Numista: upload]]).</td></tr>
<tr><td>Mintage</td><td>Mintage of the issue. <code>n/a</code> — Numista has no mintage for it; empty — not loaded yet.</td></tr>
<tr><td>Composition</td><td>Metal of the type from Numista.</td></tr>
<tr><td>Year, Condition, Value</td><td>From uCoin.</td></tr>
</table>
<p>Click a column header to sort, click it again to reverse. By default the list is sorted by year, then by denomination.</p>
<h2>Coin details</h2>
<p>Under the list: obverse and reverse photos (double-click — [[page:photos|photo viewer]]) and the coin data in two columns.
The #Numista value is a link to the type page on Numista. The right column shows the type details loaded from Numista
(weight, size, shape, ruler, series, years, mints, demonetization).</p>
<h2>Status bar</h2>
<p>Number of shown items, their quantity and value, and the number of checked items.</p>
"""
T['main-window', 'uk'] = u"""
[[shot:01-main-window.png|Головне вікно.]]
<h2>Дерево навігації</h2>
<p>Ліворуч: [[ui:All|Усі]], континенти з країнами, [[ui:No continent|Без континенту]],
[[ui:Extinct states|Зниклі держави]] і [[ui:Checked|Позначені]] (усі позначені монети з будь-яких країн, див. [[page:checks|Позначки монет]]).
Число в дужках — кількість примірників.</p>
<h2>Пошук</h2>
<p>Поле пошуку на панелі інструментів фільтрує список за країною, періодом, валютою, номіналом, роком, сюжетом, різновидом, номером у каталозі та коментарем.</p>
<h2>Список монет</h2>
<table>
<tr><th>Колонка</th><th>Значення</th></tr>
<tr><td>☐</td><td>Позначка для масових операцій ([[page:checks|Позначки монет]]).</td></tr>
<tr><td>[[ui:Country|Країна]], [[ui:Period|Період]], [[ui:Currency|Валюта]], [[ui:Denomination|Номінал]]</td><td>З uCoin. Країна прихована, коли в дереві вибрано одну країну.</td></tr>
<tr><td>#Numista</td><td>Номер типу Numista (N#). Сірим — випуск ще не вибрано.</td></tr>
<tr><td>✓</td><td>Примірник вивантажено у вашу колекцію на Numista.</td></tr>
<tr><td>[[ui:Rarity|Рідкість]]</td><td>Індекс рідкісності з коментаря вашої колекції на Numista ([[page:numista-upload|Numista: вивантаження]]).</td></tr>
<tr><td>[[ui:Mintage|Тираж]]</td><td>Тираж випуску. <code>н/д</code> — у Numista тиражу немає; порожньо — ще не завантажено.</td></tr>
<tr><td>[[ui:Composition|Метал]]</td><td>Метал типу з Numista.</td></tr>
<tr><td>[[ui:Year|Рік]], [[ui:Condition|Стан]], [[ui:Value|Вартість]]</td><td>З uCoin.</td></tr>
</table>
<p>Клацніть заголовок колонки, щоб відсортувати, ще раз — у зворотному порядку. Типово — за роком, потім за номіналом.</p>
<h2>Подробиці про монету</h2>
<p>Під списком: фото аверсу й реверсу (подвійне клацання — [[page:photos|перегляд фото]]) і дані монети у дві колонки.
Значення #Numista — посилання на сторінку типу на Numista. У правій колонці — характеристики типу з Numista
(вага, розмір, форма, правитель, серія, роки, монетні двори, демонетизація).</p>
<h2>Рядок стану</h2>
<p>Кількість показаних примірників, їх кількість і вартість, кількість позначених.</p>
"""

# =====================================================================
T['checks', 'en'] = u"""
[[shot:10-header-menu.png|The menu of the check box column header.]]
<p>Mass Numista operations (auto-link, upload, update catalogue data, rarity index) work with <b>checked</b> items.
Checks are kept for the whole collection, also when you switch to another country.</p>
<ul>
<li>Click the check box in a row, or press <b>Space</b>.</li>
<li>Click the check box in the column header — check / uncheck all items of the current list.</li>
<li>Right-click the check box column header: [[ui:Check all in the list|Позначити всі у списку]],
[[ui:Uncheck all in the list|Зняти позначки у списку]], [[ui:Uncheck everywhere|Зняти всі позначки]] (whole collection),
[[ui:Show checked|Показати позначені]].</li>
</ul>
<h2>Checks outside the current list</h2>
<p>If some checked items are not in the current list (another country, search), the status bar shows
<code>Checked: 14 (outside the list: 3)</code>. Open [[ui:Checked|Позначені]] in the tree to see all of them.</p>
"""
T['checks', 'uk'] = u"""
[[shot:10-header-menu.png|Меню заголовка колонки позначок.]]
<p>Масові операції Numista (автозв'язок, вивантаження, оновлення даних каталогу, індекс рідкісності) працюють з <b>позначеними</b> примірниками.
Позначки зберігаються для всієї колекції, також коли ви переходите до іншої країни.</p>
<ul>
<li>Клацніть позначку в рядку або натисніть <b>Пробіл</b>.</li>
<li>Клацніть позначку в заголовку колонки — позначити / зняти всі примірники поточного списку.</li>
<li>Праве клацання на заголовку колонки позначок: [[ui:Check all in the list|Позначити всі у списку]],
[[ui:Uncheck all in the list|Зняти позначки у списку]], [[ui:Uncheck everywhere|Зняти всі позначки]] (уся колекція),
[[ui:Show checked|Показати позначені]].</li>
</ul>
<h2>Позначки поза поточним списком</h2>
<p>Якщо частина позначених примірників не в поточному списку (інша країна, пошук), рядок стану показує
<code>Позначено: 14 (поза списком: 3)</code>. Відкрийте [[ui:Checked|Позначені]] у дереві, щоб побачити їх усі.</p>
"""

# =====================================================================
T['coin-card', 'en'] = u"""
[[shot:03-coin-card.png|The coin card: Coin, My item and Photo.]]
<p>[[ui:Collection → Add / Edit / Delete|Колекція → Додати / Змінити / Видалити]], or double-click a row to edit it.</p>
<h2>Coin and My item</h2>
<ul>
<li><b>Coin</b> — data of the coin type: country, period, currency, denomination, year (a range — the second field; before Christ — a negative year, <code>-44</code>), variety, diameter, catalogue number, subject.
They are shared by all items of this coin; if there are several items, the card warns you.</li>
<li>[[ui:Numista search|Пошук Numista]] — your own search phrase for Numista. Empty — an automatic phrase is used (shown in grey):
denomination, period, year. See [[page:numista-link|Numista: linking coins]].</li>
<li><b>My item</b> — your copy: condition, quantity, prices, dates, swap, comment, grading.</li>
</ul>
<h2>Photo</h2>
<p>Obverse and reverse: [[ui:File...|Файл...]], [[ui:Paste|Вставити]] (from the clipboard), [[ui:Remove|Видалити]], or drag a file from Explorer onto the frame.
See [[page:photos|Photos]].</p>
<p>Changes are saved with <b>OK</b>; <b>Cancel</b> changes nothing.</p>
"""
T['coin-card', 'uk'] = u"""
[[shot:03-coin-card.png|Картка монети: Монета, Мій примірник і Фото.]]
<p>[[ui:Collection → Add / Edit / Delete|Колекція → Додати / Змінити / Видалити]] або подвійне клацання по рядку — змінити.</p>
<h2>Монета і Мій примірник</h2>
<ul>
<li><b>[[ui:Coin|Монета]]</b> — дані типу монети: країна, період, валюта, номінал, рік (діапазон — друге поле; до н. е. — від'ємний рік, <code>-44</code>), різновид, діаметр, номер у каталозі, сюжет.
Вони спільні для всіх примірників цієї монети; якщо примірників кілька, картка попереджає.</li>
<li>[[ui:Numista search|Пошук Numista]] — власна фраза пошуку на Numista. Порожньо — автоматична фраза (сірим):
номінал, період, рік. Див. [[page:numista-link|Numista: зв'язок монет]].</li>
<li><b>[[ui:My item|Мій примірник]]</b> — ваш примірник: стан, кількість, ціни, дати, обмін, коментар, грейдинг.</li>
</ul>
<h2>Фото</h2>
<p>Аверс і реверс: [[ui:File...|Файл...]], [[ui:Paste|Вставити]] (з буфера обміну), [[ui:Remove|Видалити]] або перетягніть файл із Провідника на рамку.
Див. [[page:photos|Фото]].</p>
<p>Зміни зберігаються кнопкою <b>OK</b>; <b>[[ui:Cancel|Скасувати]]</b> нічого не змінює.</p>
"""

# =====================================================================
T['photos', 'en'] = u"""
[[shot:04-photo-viewer.png|The photo viewer.]]
<h2>Item photo and coin type photo</h2>
<ul>
<li>An <b>item photo</b> is your own shot of your copy.</li>
<li>A <b>coin type photo</b> is shared by all items of the coin. In the card, check
[[ui:Save as coin type photo|Зберегти як фото типу]] before adding it.</li>
<li>The item photo is shown first; without it — the coin type photo, marked <i>coin type</i>.</li>
</ul>
<h2>Adding photos</h2>
<p>In the [[page:coin-card|coin card]]: [[ui:File...|Файл...]] (JPEG, PNG, BMP, GIF, TIFF), [[ui:Paste|Вставити]],
or drag a file onto the frame. Photos are reduced to 1200 px and saved as JPEG in <code>Photos\\</code>.</p>
<h2>Photos from Numista</h2>
<p>When a coin is linked with Numista, the obverse and reverse pictures of the type are downloaded automatically as coin type photos
([[page:numista-link|linking]], [[page:numista-catalog|update catalogue data]]). Your own coin type photo is never replaced.
The picture author is shown in the viewer: <i>Photo: &lt;author&gt; / Numista</i>.</p>
<h2>Viewer</h2>
<p>Double-click a photo in the coin details. Mouse wheel — zoom, drag — move, ←/→ or Space — other side,
<b>0</b> or Home — fit to window, Esc — close.</p>
"""
T['photos', 'uk'] = u"""
[[shot:04-photo-viewer.png|Вікно перегляду фото.]]
<h2>Фото примірника і фото типу</h2>
<ul>
<li><b>Фото примірника</b> — ваш власний знімок вашого примірника.</li>
<li><b>Фото типу</b> спільне для всіх примірників монети. У картці позначте
[[ui:Save as coin type photo|Зберегти як фото типу]] перед додаванням.</li>
<li>Спершу показується фото примірника; без нього — фото типу з позначкою <i>фото типу</i>.</li>
</ul>
<h2>Додавання фото</h2>
<p>У [[page:coin-card|картці монети]]: [[ui:File...|Файл...]] (JPEG, PNG, BMP, GIF, TIFF), [[ui:Paste|Вставити]]
або перетягніть файл на рамку. Фото зменшуються до 1200 px і зберігаються як JPEG у <code>Photos\\</code>.</p>
<h2>Фото з Numista</h2>
<p>Коли монету пов'язано з Numista, зображення аверсу й реверсу типу завантажуються автоматично як фото типу
([[page:numista-link|зв'язок]], [[page:numista-catalog|оновлення даних каталогу]]). Ваше власне фото типу ніколи не замінюється.
Автор зображення показується у вікні перегляду: <i>Photo: &lt;автор&gt; / Numista</i>.</p>
<h2>Перегляд</h2>
<p>Подвійне клацання по фото в подробицях. Коліщатко миші — масштаб, перетягування — зсув, ←/→ або Пробіл — інша сторона,
<b>0</b> або Home — за розміром вікна, Esc — закрити.</p>
"""

# =====================================================================
T['references', 'en'] = u"""
<p>[[ui:Reference → Countries / Periods / Currencies|Довідники → Країни / Періоди / Валюти]].</p>
<h2>Countries</h2>
<ul>
<li>[[ui:Continent|Континент]] — where the country is shown in the tree.</li>
<li>[[ui:Exists (not an extinct state)|Існує (не зникла держава)]] — unchecked: the country is shown under [[ui:Extinct states|Зниклі держави]].</li>
<li>[[ui:Numista issuer|Емітент Numista]] — the Numista issuer code used to search coins of this country. It is filled automatically;
change it only if coins of the country are not found.</li>
</ul>
<h2>Periods and currencies</h2>
<p>Names can be corrected; an entry can be deleted only if no coin uses it.</p>
<div class="note">The uCoin import matches countries by name: after renaming, the old name is created again on the next import.</div>
"""
T['references', 'uk'] = u"""
<p>[[ui:Reference → Countries / Periods / Currencies|Довідники → Країни / Періоди / Валюти]].</p>
<h2>Країни</h2>
<ul>
<li>[[ui:Continent|Континент]] — де країна показується в дереві.</li>
<li>[[ui:Exists (not an extinct state)|Існує (не зникла держава)]] — знято: країна показується в [[ui:Extinct states|Зниклі держави]].</li>
<li>[[ui:Numista issuer|Емітент Numista]] — код емітента Numista для пошуку монет цієї країни. Заповнюється автоматично;
змінюйте його, лише якщо монети країни не знаходяться.</li>
</ul>
<h2>Періоди та валюти</h2>
<p>Назви можна виправити; запис можна видалити, лише якщо жодна монета його не використовує.</p>
<div class="note">Імпорт з uCoin зіставляє країни за назвою: після перейменування стара назва створиться знову при наступному імпорті.</div>
"""

# =====================================================================
T['numista-connect', 'en'] = u"""
[[shot:05-numista-settings.png|Numista → Connect / Settings.]]
<h2>Your API key</h2>
<ol>
<li>Register on <a href="https://en.numista.com/">numista.com</a>.</li>
<li>Request your API key (signed in to Numista): <a href="https://en.numista.com/api/api_key.php">en.numista.com/api/api_key.php</a>.</li>
<li>Enter the key in [[ui:Numista → Connect / Settings|Numista → Підключення / налаштування]] and press
[[ui:Check connection|Перевірити підключення]].</li>
</ol>
<p>Each user uses their own key. The key is stored only in <code>CoinsCollection.ini</code> next to the program.</p>
<h2>Requests and quota</h2>
<p>The free Numista key allows <b>2000 requests per month</b>. Every result window shows how many requests the operation used
and how many are used this month.</p>
<ul>
<li>Answers are cached in <code>Numista\\</code>: the same type is never requested twice.</li>
<li>Photos are files of the Numista website, not API requests — they do not use the quota.</li>
<li>Typical costs: linking a coin — 3–5 requests; uploading checked items — 1 per item plus 2 per run; rarity index — 1 per run.</li>
</ul>
"""
T['numista-connect', 'uk'] = u"""
[[shot:05-numista-settings.png|Numista → Підключення / налаштування.]]
<h2>Ваш ключ API</h2>
<ol>
<li>Зареєструйтеся на <a href="https://en.numista.com/">numista.com</a>.</li>
<li>Запросіть свій ключ API (увійшовши на Numista): <a href="https://en.numista.com/api/api_key.php">en.numista.com/api/api_key.php</a>.</li>
<li>Введіть ключ у [[ui:Numista → Connect / Settings|Numista → Підключення / налаштування]] і натисніть
[[ui:Check connection|Перевірити підключення]].</li>
</ol>
<p>Кожен користувач використовує власний ключ. Ключ зберігається лише в <code>CoinsCollection.ini</code> поруч із програмою.</p>
<h2>Запити та квота</h2>
<p>Безкоштовний ключ Numista дає <b>2000 запитів на місяць</b>. Кожне вікно з підсумком показує, скільки запитів використала операція
і скільки використано цього місяця.</p>
<ul>
<li>Відповіді зберігаються в <code>Numista\\</code>: той самий тип ніколи не запитується двічі.</li>
<li>Фото — файли сайту Numista, а не запити API: квоту вони не витрачають.</li>
<li>Звичайні витрати: зв'язок монети — 3–5 запитів; вивантаження позначених — 1 на примірник плюс 2 на запуск; індекс рідкісності — 1 на запуск.</li>
</ul>
"""

# =====================================================================
T['numista-link', 'en'] = u"""
[[shot:02-context-menu.png|Right-click menu of a coin: this coin (top) and checked coins (bottom).]]
<p>Linking finds the Numista type (#Numista) and issue of a coin. Then the program loads the mintage, the type details and the photos.</p>
<h2>One coin — Link manually</h2>
<p>Right-click a coin → [[ui:Link manually|Пов'язати вручну]] (or the [[ui:Numista|Numista]] menu).</p>
<ol>
<li>The program searches by the catalogue number (KM#, Y#, C#), then by number and year, then by the search phrase.</li>
<li>If there is one exact result, the coin is linked at once. Otherwise the type list opens.</li>
<li>Choose the issue (year, mint) if it is not found automatically: [[ui:Choose issue|Вибрати випуск]].</li>
</ol>
[[shot:06-choose-type.png|Choose Numista type: the search phrase can be changed and searched again.]]
<h2>Search phrase</h2>
<p>If the right type is not in the list, edit [[ui:Search phrase|Фраза пошуку]] and press [[ui:Search|Пошук]] (or Enter),
for example <code>Ort koronny - Sigismund III Vasa 1623</code>. The phrase you link with is saved in the coin card
([[ui:Numista search|Пошук Numista]]) and used next time.</p>
<h2>Historical states</h2>
<p>Some coins are listed on Numista under another issuer than the uCoin country (for example Poland 1766 →
<i>Polish–Lithuanian Commonwealth</i>). Such results always need your confirmation. When you choose one, the issuer is remembered
for the coin's period, and the next coins of that period are found at once.</p>
<h2>Many coins — Auto-link</h2>
<p>Check coins ([[page:checks|Checking coins]]) → [[ui:Auto-link|Автозв'язок]]. Only unambiguous results are linked;
the others are counted in the result — link them manually.
The check box [[ui:Also load type details and photos|Також завантажити характеристики й фото]] adds 1 request per coin.</p>
[[shot:07-progress.png|Progress of a mass operation. Stop — stops after the current coin.]]
"""
T['numista-link', 'uk'] = u"""
[[shot:02-context-menu.png|Меню правого клацання: ця монета (вгорі) і позначені монети (внизу).]]
<p>Зв'язок знаходить тип Numista (#Numista) і випуск монети. Потім програма завантажує тираж, характеристики типу та фото.</p>
<h2>Одна монета — Пов'язати вручну</h2>
<p>Праве клацання по монеті → [[ui:Link manually|Пов'язати вручну]] (або меню <span class="ui">Numista</span>).</p>
<ol>
<li>Програма шукає за номером у каталозі (KM#, Y#, C#), потім за номером і роком, потім за фразою пошуку.</li>
<li>Якщо результат один і точний — монету пов'язано одразу. Інакше відкривається список типів.</li>
<li>Виберіть випуск (рік, двір), якщо його не знайдено автоматично: [[ui:Choose issue|Вибрати випуск]].</li>
</ol>
[[shot:06-choose-type.png|Вибір типу Numista: фразу пошуку можна змінити й шукати знову.]]
<h2>Фраза пошуку</h2>
<p>Якщо потрібного типу немає у списку, змініть [[ui:Search phrase|Фраза пошуку]] і натисніть [[ui:Search|Пошук]] (або Enter),
наприклад <code>Ort koronny - Sigismund III Vasa 1623</code>. Фраза, з якою ви пов'язали монету, зберігається в картці
([[ui:Numista search|Пошук Numista]]) і використовується наступного разу.</p>
<h2>Історичні держави</h2>
<p>Деякі монети на Numista записані за іншим емітентом, ніж країна uCoin (наприклад Poland 1766 →
<i>Polish–Lithuanian Commonwealth</i>). Такі результати завжди потребують вашого підтвердження. Коли ви вибираєте такий, емітента запам'ятовується
для періоду монети, і наступні монети цього періоду знаходяться одразу.</p>
<h2>Багато монет — Автозв'язок</h2>
<p>Позначте монети ([[page:checks|Позначки монет]]) → [[ui:Auto-link|Автозв'язок]]. Пов'язуються лише однозначні результати;
решта рахується в підсумку — пов'яжіть їх вручну.
Позначка [[ui:Also load type details and photos|Також завантажити характеристики й фото]] додає 1 запит на монету.</p>
[[shot:07-progress.png|Хід масової операції. Зупинити — після поточної монети.]]
"""

# =====================================================================
T['numista-upload', 'en'] = u"""
[[shot:09-numista-comment.png|Numista: the private comment of a collection item with the rarity index.]]
<h2>Upload to your Numista collection</h2>
<p>One coin: right-click → [[ui:Upload to Numista|Вивантажити в Numista]]. Many: check items → [[ui:Upload|Вивантажити]].
Only items linked with Numista (#Numista) are uploaded: condition, price, purchase date and comment. Photos are not uploaded.</p>
<ul>
<li>Uploaded items get ✓ and are not sent again.</li>
<li>An item that you have already added on the Numista website is not duplicated: it is linked with that item and gets ✓.</li>
</ul>
<h2>Private comment</h2>
<p>The program writes the comment as attributes <code>Code:value</code> separated by <code>;</code>:</p>
<table>
<tr><th>Code</th><th>Meaning</th></tr>
<tr><td><code>S</code></td><td>Swap (uCoin)</td></tr>
<tr><td><code>C</code></td><td>Your comment (always last)</td></tr>
<tr><td><code>R</code></td><td>Rarity index 0–100 — you add it yourself on Numista</td></tr>
</table>
<h2>Rarity index</h2>
<p>The Numista API does not provide the rarity index. Copy it from the type page on Numista into the private comment of your
collection item, for example <code>R:9;S:…;C:…</code> — or simply as a number at the beginning: <code>9</code> or <code>9;my comment</code> — then use [[ui:Read rarity index|Прочитати індекс рідкісності]].
One request reads the whole collection, so check many coins and read them at once.</p>
"""
T['numista-upload', 'uk'] = u"""
[[shot:09-numista-comment.png|Numista: приватний коментар предмета колекції з індексом рідкісності.]]
<h2>Вивантаження у вашу колекцію на Numista</h2>
<p>Одна монета: праве клацання → [[ui:Upload to Numista|Вивантажити в Numista]]. Багато: позначте примірники → [[ui:Upload|Вивантажити]].
Вивантажуються лише примірники, пов'язані з Numista (#Numista): стан, ціна, дата купівлі та коментар. Фото не вивантажуються.</p>
<ul>
<li>Вивантажені примірники отримують ✓ і повторно не надсилаються.</li>
<li>Примірник, який ви вже додали на сайті Numista, не дублюється: його пов'язано з тим предметом, і він отримує ✓.</li>
</ul>
<h2>Приватний коментар</h2>
<p>Програма записує коментар як атрибути <code>Код:значення</code> через <code>;</code>:</p>
<table>
<tr><th>Код</th><th>Значення</th></tr>
<tr><td><code>S</code></td><td>Обмін (uCoin)</td></tr>
<tr><td><code>C</code></td><td>Ваш коментар (завжди останній)</td></tr>
<tr><td><code>R</code></td><td>Індекс рідкісності 0–100 — додаєте самі на Numista</td></tr>
</table>
<h2>Індекс рідкісності</h2>
<p>Numista API не надає індекс рідкісності. Скопіюйте його зі сторінки типу на Numista у приватний коментар предмета
вашої колекції, наприклад <code>R:9;S:…;C:…</code> — або просто числом на початку: <code>9</code> чи <code>9;мій коментар</code> — потім [[ui:Read rarity index|Прочитати індекс рідкісності]].
Один запит читає всю колекцію, тож позначте багато монет і прочитайте їх разом.</p>
"""

# =====================================================================
T['numista-catalog', 'en'] = u"""
<p>For linked coins the program keeps data from the Numista catalogue:</p>
<ul>
<li><b>Mintage</b> of the issue — the [[ui:Mintage|Тираж]] column (<code>n/a</code> — Numista has no mintage);</li>
<li><b>Type details</b> — composition (the [[ui:Composition|Метал]] column), weight, size, shape, technique, ruler, series,
commemorated topic, years, mints, demonetization (coin details, right column);</li>
<li><b>Photos</b> of the type ([[page:photos|Photos]]).</li>
</ul>
<h2>Update catalogue data</h2>
<p>Check coins → [[ui:Update catalogue data|Оновити дані каталогу]]: loads what is missing. Cached answers are free,
so a repeated run usually needs 0 requests.</p>
<p>One coin: right-click → [[ui:Update catalogue data (no cache)|Оновити дані каталогу (без кешу)]] — requests the data again
(2 requests), for example when a mintage was added on Numista.</p>
"""
T['numista-catalog', 'uk'] = u"""
<p>Для пов'язаних монет програма зберігає дані з каталогу Numista:</p>
<ul>
<li><b>Тираж</b> випуску — колонка [[ui:Mintage|Тираж]] (<code>н/д</code> — у Numista тиражу немає);</li>
<li><b>Характеристики типу</b> — метал (колонка [[ui:Composition|Метал]]), вага, розмір, форма, техніка, правитель, серія,
пам'ятна тема, роки, монетні двори, демонетизація (подробиці, права колонка);</li>
<li><b>Фото</b> типу ([[page:photos|Фото]]).</li>
</ul>
<h2>Оновити дані каталогу</h2>
<p>Позначте монети → [[ui:Update catalogue data|Оновити дані каталогу]]: завантажує те, чого бракує. Збережені відповіді безкоштовні,
тож повторний запуск зазвичай потребує 0 запитів.</p>
<p>Одна монета: праве клацання → [[ui:Update catalogue data (no cache)|Оновити дані каталогу (без кешу)]] — запитує дані знову
(2 запити), наприклад коли на Numista додали тираж.</p>
"""

# =====================================================================
T['faq', 'en'] = u"""
<h3>A coin is not found on Numista or the wrong types are offered</h3>
<p>Edit the search phrase in the type list and search again — [[page:numista-link|Numista: linking coins]].
Check the catalogue number in the coin card (KM#, Y#, C#). Coins with a uCoin number (UC#) are searched only by phrase.</p>
<h3>The monthly request quota is almost used</h3>
<p>Work with checked coins (one request per run for upload and rarity), turn off
[[ui:Also load type details and photos|Також завантажити характеристики й фото]] in auto-link and load the details later.</p>
<h3>“Unable to write … CoinsCollection.ini”</h3>
<p>The file is open in an editor that locks it. Close the editor and repeat the command.</p>
<h3>Photos from Numista were not loaded</h3>
<p>Check the internet connection and run [[ui:Update catalogue data|Оновити дані каталогу]] for the checked coins again.</p>
<h3>How to restore the collection from a backup copy</h3>
<p>Close the program. In <code>Db\\</code> rename the current database (for example add <code>.old</code>) and rename the needed
<code>*.bak</code> copy to the database name (by default <code>coins.db</code>). Start the program.</p>
"""
T['faq', 'uk'] = u"""
<h3>Монета не знаходиться на Numista або пропонуються не ті типи</h3>
<p>Змініть фразу пошуку у списку типів і шукайте знову — [[page:numista-link|Numista: зв'язок монет]].
Перевірте номер у каталозі в картці монети (KM#, Y#, C#). Монети з номером uCoin (UC#) шукаються лише за фразою.</p>
<h3>Місячну квоту запитів майже вичерпано</h3>
<p>Працюйте з позначеними монетами (один запит на запуск для вивантаження та індексу рідкісності), вимкніть
[[ui:Also load type details and photos|Також завантажити характеристики й фото]] в автозв'язку й завантажте характеристики пізніше.</p>
<h3>«Unable to write … CoinsCollection.ini»</h3>
<p>Файл відкритий у редакторі, який його блокує. Закрийте редактор і повторіть команду.</p>
<h3>Фото з Numista не завантажилися</h3>
<p>Перевірте підключення до інтернету й ще раз запустіть [[ui:Update catalogue data|Оновити дані каталогу]] для позначених монет.</p>
<h3>Як відновити колекцію з резервної копії</h3>
<p>Закрийте програму. У <code>Db\\</code> перейменуйте поточну базу (наприклад, додайте <code>.old</code>), а потрібну копію
<code>*.bak</code> — на ім'я бази (типово <code>coins.db</code>). Запустіть програму.</p>
"""

# =====================================================================
def render(page_id, lang, title):
    body = T[page_id, lang]
    titles = dict((p[0], (p[1] if lang == 'en' else p[2]).decode('utf-8')) for p in PAGES)

    def ui(m):
        en, uk = m.group(1), m.group(2)
        return u'<span class="ui">%s</span>' % (en if lang == 'en' else en + u'/' + uk)

    def shot(m):
        name, caption = m.group(1), m.group(2)
        return (u'<figure class="shot"><img src="../images/%s" alt="%s" '
                u'onerror="this.parentNode.className+=\' missing\'">'
                u'<div class="missing-frame">%s: images/%s</div>'
                u'<figcaption>%s</figcaption></figure>') % (name, caption, MISSING[lang], name, caption)

    def page(m):
        return u'<a href="%s.html">%s</a>' % (m.group(1), m.group(2))

    body = re.sub(u'\\[\\[ui:([^|\\]]+)\\|([^\\]]+)\\]\\]', ui, body)
    body = re.sub(u'\\[\\[shot:([^|\\]]+)\\|([^\\]]+)\\]\\]', shot, body)
    body = re.sub(u'\\[\\[page:([^|\\]]+)\\|([^\\]]+)\\]\\]', page, body)
    if u'[[toc]]' in body:
        toc = u'\n'.join(u'<li><a href="%s.html">%s</a></li>' % (p[0], titles[p[0]]) for p in PAGES if p[0] != 'index')
        body = body.replace(u'[[toc]]', toc)

    other = 'uk' if lang == 'en' else 'en'
    contents, other_name = NAV[lang]
    html = u"""<!doctype html>
<html lang="%(lang)s">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>%(title)s — %(app)s</title>
<link rel="stylesheet" href="../help.css">
</head>
<body>
<header>
<span class="app">%(app)s</span>
<nav><a href="index.html">%(contents)s</a><a href="../%(other)s/%(id)s.html">%(other_name)s</a></nav>
</header>
<main>
<h1>%(title)s</h1>
%(body)s
</main>
<footer>Coins Collection</footer>
</body>
</html>
""" % {'lang': lang, 'title': title, 'app': APP[lang], 'contents': contents, 'other': other,
       'id': page_id, 'other_name': other_name, 'body': body.strip()}
    return html

base = os.path.dirname(os.path.abspath(__file__))
for lang in LANGS:
    out = os.path.join(base, lang)
    if not os.path.isdir(out):
        os.makedirs(out)
    for page_id, title_en, title_uk in PAGES:
        title = title_en if lang == 'en' else title_uk
        html = render(page_id, lang, title.decode('utf-8') if isinstance(title, str) else title)
        f = open(os.path.join(out, page_id + '.html'), 'wb')
        f.write(html.encode('utf-8'))
        f.close()
print 'ok'
