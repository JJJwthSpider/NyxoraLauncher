# Vellora Client

Лаунчер Minecraft на основе Prism Launcher 11.1.1 в **бордово-красных тонах** с живым **красным дождём**
на фоне. Программа называется **Vellora Client**, исполняемый файл — `vellora` (раньше лаунчер назывался
Lullaby Launcher, ещё раньше — SculkShoot Client). Идеи оформления и функций взяты у
[Millida Launcher](https://github.com/millida/launcher) (GPL-3.0): его код на Tauri/React перенести
в Qt нельзя, поэтому похожие вещи сделаны заново; название, логотип и картинки Millida не используются.

## Что изменено

- **Тема «Vellora (Красный дождь)»** — тема по умолчанию (`launcher/ui/themes/SculkTheme.cpp`, вариант
  `Lunar`, id `lunar` оставлен, чтобы не ломать сохранённые настройки): бордовый фон (#0e0407), малиновые
  акценты (#e8364f), выделения, нажатые кнопки, прогресс и главные кнопки — малиновые градиенты.
  При первом запуске Vellora тема один раз сама включается у всех.
- **Красный дождь** (`launcher/ui/themes/SculkAmbience.cpp`): за списком установок и на плитках идёт
  косой красный дождь в два слоя (дальние капли тоньше и тусклее), внизу разлетаются брызги, а время
  от времени небо вспыхивает красным, как далёкая молния. За панелями — неподвижные тусклые капли.
  Выключается галочкой «Живой фон» в *Настройки → Внешний вид*. В Android-приложении то же самое
  (`android/src/.../RainDrawable.java`).
- **Иконка Vellora** — малиновый гранёный кристалл «V» под красным дождём
  (`program_info/vellora_icon.py`: SVG, PNG, `vellora.ico`, `vellora.icns`; в Android — `android/art/vellora_art.py`).
- Старые темы **«Скалк»** (обычная, светлая, тёмная) остались и выбираются в *Настройки → Внешний вид*.
  У тех, кто пользовался SculkShoot Client, внешний вид один раз сам переключается на Vellora.
- **Шрифт как в Minecraft** — пиксельный Tiny5 (лицензия SIL OFL, `launcher/resources/sculk_theme/fonts/`),
  с полной кириллицей.
- **Переезд с SculkShoot Client:** при первом запуске Vellora Client находит данные SculkShoot
  Client (установки, аккаунты, чаты, скины) и предлагает перенести их. Коды приглашения теперь
  начинаются с `LL-`, старые коды `SC-` тоже принимаются.
- **Удобное главное окно:**
  - над списком — **поиск установок** (Ctrl+F, Esc очищает, Enter выбирает первую найденную,
    второй Enter запускает), **сортировка** «По имени» / «Сначала недавние» и счётчик «2 из 4»;
  - на карточке установки видно **версию Minecraft и загрузчик** («1.21.1 · NeoForge»);
  - в правой панели главная кнопка «Запустить» выделена, «Удалить» при наведении красная;
  - везде одно слово — **«установка»** (вместо «экземпляр» из перевода Prism);
  - полоса новостей Prism скрыта (включается в меню «Вид»).
- **Кнопка «Играть» на каждой установке** — запускает её одним нажатием.
- Для тем «Скалк» есть свои наборы иконок «Sculk» (`launcher/resources/sculk/`, `sculk_light/`);
  иконки сами переключаются вместе с темой.
- **Java меняется вместе с версией Minecraft.** Перед каждым запуском лаунчер выбирает
  подходящую Java (8 для 1.16 и старше, 17 для 1.17–1.20.4, 21 для 1.20.5+): сначала среди
  установленных, а если нужной нет — скачивает её сам. Java, выбранная вручную в настройках
  установки, не трогается (`launcher/minecraft/launch/AutoInstallJava.cpp`).

### Левая панель главного окна

Три большие плитки в одинаковой фирменной рамке (срезанные углы, светящиеся уголки,
звёздное небо внутри; `launcher/ui/widgets/LaunchPanel.cpp`):

1. **ИИ-маг Аркан** — бордовый волшебник в капюшоне в пиксельном стиле (`launcher/resources/sculk_theme/wizard_body.png`,
   рисует `android/art/wizard.py`), над его ладонью вращается **красный тессеракт** — четырёхмерный куб,
   который проецируется 4D → 3D → 2D каждый кадр (`launcher/ui/widgets/MoonyWidget.cpp`). Когда наводишь мышь
   или Аркан собирает сборку, тессеракт крутится быстрее и светится ярче. Открывает вкладку «ИИ-сборка».
2. **Готовые сборки** (скрещённые гаечные ключи, в теме Vellora — малиновые) — каталоги готовых сборок Modrinth и CurseForge.
3. **Новая установка** (малиновый плюс на бордовом) — создать установку самому, как в Prism Launcher:
   версия, загрузчик и т. д.

### Интерфейс и удобство

- **Всё на русском из коробки.** Полный русский перевод встроен в лаунчер
  (`launcher/resources/translations/`) и включается сразу, даже без интернета.
  Язык можно сменить в настройках.
- **Версии в одну строку.** В списках версий каждая версия занимает ровно одну строку:
  текст не переносится, длинные значения обрезаются с «…»
  (`launcher/ui/widgets/VersionSelectWidget.cpp`).
- **Быстрое создание установки** (вкладка «Быстрое создание», открывается первой):
  1) выберите версию Minecraft (по умолчанию последний релиз);
  2) одной кнопкой выберите загрузчик (без модов / NeoForge / Fabric / Forge / Quilt),
  под кнопками коротко объясняется, когда какой выбирать;
  3) рекомендуемая версия загрузчика подставляется сама. Остаётся нажать «ОК».
- **Удобный поиск модов.** Можно искать по-русски: «зомби», «оружие», «миникарта» и т. п.
  автоматически переводятся в английские запросы для Modrinth и CurseForge
  (`launcher/modplatform/agent/SearchTranslator.cpp`). Над результатами есть кнопки
  популярных категорий: «Оптимизация», «Оружие», «Мобы», «Магия», «Технологии» и др.

- **Смена иконки установки — тремя способами:** нажать на иконку прямо на карточке (при
  наведении появляется карандаш), блок «Иконка» в *Изменить → Параметры* или кнопка с иконкой
  в правой панели. Можно выбрать любую из предложенных иконок или поставить **свою картинку**
  (PNG/JPG/SVG с компьютера) — кнопка «Своя картинка...».
- **Самостоятельная сборка.** В «Быстром создании» стоит галочка «Сразу после создания открыть
  каталог модов». После «ОК» открывается окно добавления модов с вкладками Modrinth и CurseForge.

### ИИ-сборка: чат с Арканом

Вкладка **«ИИ-сборка»** (плитка с Арканом слева) — это чат. Аркан сам ведёт разговор:

1. спрашивает, **про что сборка** — например: «сделай мне сборку модов про зомби-апокалипсис,
   должны быть города, зомби и оружие»;
2. спрашивает **загрузчик и версию** Minecraft. Подходит **любая версия**, для которой есть
   загрузчик: человек сам пишет её («Forge 1.7.10», «Fabric 1.20.1», «NeoForge 1.21.1»…).
   Аркан проверяет, что такая версия Minecraft существует и что для неё есть загрузчик;
3. ищет моды по темам **на Modrinth и CurseForge** (источники выбираются галочками), добавляет
   моды на **оптимизацию** (Sodium/Embeddium, FerriteCore, ModernFix, Entity Culling,
   ImmediatelyFast, Dynamic FPS, Lithium) и **удобство** (JEI, Jade, AppleSkin, Mouse Tweaks),
   по просьбе — шейдеры и карту, и все **обязательные зависимости**;
4. проверяет совместимость с версией и загрузчиком, убирает дубли между площадками,
   несовместимые моды и моды, которые нельзя скачать через сторонние лаунчеры;
5. пишет итог (сколько модов по темам, на оптимизацию и библиотек), а справа показывает
   состав сборки с галочками. «ОК» создаёт установку. Файлы скачиваются и проверяются по хешу.

Как Аркан читает сообщения (Vellora сделала его умнее):

- **опечатки** понимаются: «зомьби», «драконнов» — это зомби и драконы;
- **«для слабого ПК / телефона»** — только лёгкие моды и вся оптимизация, без шейдеров (он объясняет почему);
- **«с друзьями», «по сети», «кооп»** — добавляется голосовой чат Simple Voice Chat;
- **«сборка на 40 модов»** — число модов по темам подстраивается под эту цифру;
- **OptiFine** заменяется на Sodium + Iris с объяснением (OptiFine нет на Modrinth, и он конфликтует с Sodium);
- новые темы: **покемоны** (Cobblemon), **динозавры**, **рыбалка**, **скайблок**, **аниме**, **военное** (танки, армия).

Остальное:
- **моды по названию** — «добавь Create и Farmer's Delight», «хочу «Alex's Mobs»»: латинские
  названия и всё в кавычках ищется на обеих площадках по точному имени и попадает в группу
  «По твоей просьбе». Если мода нет для этой версии, Аркан прямо об этом пишет и ищет похожие;
- **«убери / без / не надо»** — «без зомби», «убери JEI»: тема и моды с таким названием
  не попадают в сборку. Более позднее сообщение отменяет более раннее («добавь зомби обратно»);
- **«побольше / много»** — «побольше оружия» удваивает число модов в теме;
- **непонятные слова** Аркан не выкидывает молча, а переспрашивает: «я не знаю этих слов: …».

Зависимости: для каждого мода берутся все обязательные библиотеки (и библиотеки библиотек),
включая те, что Modrinth указывает только номером версии. Если библиотеку нельзя скачать на
её площадке (нет файла для версии или автор запретил сторонние лаунчеры на CurseForge),
Аркан берёт ту же библиотеку с другой площадки. В списке у библиотеки написано, какому моду
она нужна, а если снять галочку с мода, ненужные больше библиотеки тоже не скачиваются.

**Чаты сохраняются.** Слева от переписки — список «Мои сборки»: каждый разговор с Арканом
сохраняется вместе с сообщениями, выбранными загрузчиком и версией и готовым списком модов
(с галочками) в `<папка лаунчера>/agent-chats/<id>.json`. Можно открыть старый чат и продолжить
(«добавь броню» пересоберёт именно эту сборку), начать «Новый чат», переименовать чат двойным
щелчком или удалить его. Если закрыть окно во время сборки, при следующем открытии чата Аркан
доделает её сам.

После этого с Арканом можно продолжать разговор: «добавь магию», «добавь шейдеры»,
«убери Create», «давай на Fabric 1.20.1», «начать заново». Пока Аркан работает, точки в его облачке
прыгают, а в чате обновляется пузырь с прогрессом.

Без настроек агент понимает запросы по встроенным правилам (около 25 тем, русские и
английские слова). Если ввести ключ **Anthropic API** (шестерёнка в шапке чата), запрос
сначала разбирает Claude (`claude-opus-5-5`, структурированный JSON-ответ). Если Claude
недоступен, агент автоматически переходит на встроенные правила.

Код: `launcher/modplatform/agent/` (план, Claude, сборщик, импорт),
`launcher/ui/pages/modplatform/agent/AgentPage.cpp` (чат).

### Приложение для Android

В папке `android/` — **Vellora Client для Android** (`android/build-apk.sh` собирает APK): чат с Арканом,
который собирает сборку с Modrinth по тем же правилам, сохраняет чаты и отдаёт сборку `.mrpack` лаунчеру
Minecraft на телефоне; там же расшифровка кодов приглашения. Подробности — в `android/README.md`.

### Игра с друзьями

**Лобби** (кнопка «Лобби» сверху или «Играть с друзьями» у установки):
- *Создать лобби* — лаунчер ставит в установку мод **e4mc** (с Modrinth, с зависимостями и
  проверкой SHA-512), запускает игру, а когда ты открываешь мир для сети, сам находит в логе
  адрес вида `brave-creeper.eu.e4mc.link` и показывает его с кнопками «Копировать адрес» и
  «Копировать приглашение». Без проброса портов и регистрации; e4mc нужен только хозяину.
  Для установки без модов — адрес в локальной сети. Кнопка «Сохранить сборку для друзей (.mrpack)».
- *Присоединиться к другу* — вставь **код приглашения** или адрес (код из буфера обмена
  подставляется сам), лаунчер покажет IP и порт и запустит игру сразу с подключением.

**Мой сервер** (кнопка «Мой сервер» сверху или «Сделать сервер» у установки) — настоящий
сервер из установки, как обычные серверы:
- настройки: **PvP**, режим игры, сложность, хардкор, полёт, командные блоки, макс. игроков,
  дальность прорисовки, описание, сид, **белый список** друзей, память;
- лаунчер сам скачивает серверную часть (обычный сервер Mojang, Fabric, Forge, NeoForge),
  подбирает Java под версию, переносит моды установки **без клиентских** (Sodium, Iris, e4mc…),
  пишет `server.properties` и EULA (нужно принять галочкой);
- сервер открывается в интернет через бесплатный туннель **bore.pub** (без проброса портов);
- ты получаешь **код приглашения** вида `SC-05ZG-0001-JHMW-C` — в нём зашиты IP и порт.
  Друг вводит его в «Лобби → Присоединиться к другу», видит IP и порт и подключается;
- в окне — список игроков онлайн, консоль сервера и строка для команд (`op`, `time set day`…).
  Сервер работает, даже если закрыть окно, и корректно останавливается вместе с лаунчером.

### Скины и ник

Внизу левой панели — кнопка профиля (лицо скина и ник). Меню:
- **Скины и плащи** — менеджер скинов: 3D-превью, загрузка из файла, по ссылке или по нику,
  выбор плаща, модель Стива или Алекс;
  - **Создать / изменить скин** — редактор: холст 64×64 с подписанными частями тела
    («Голова — лицо», «Левый рукав (2-й слой) — спина»), карандаш, ластик, заливка в пределах
    грани, пипетка, тени и блики, зеркальное рисование (рисуешь правую руку — рисуется и левая),
    подсветка 1-го или 2-го слоя, палитра, отмена/повтор, живой 3D-просмотр. Готовые образы для
    старта: Стив (с ним открывается новый скин классической модели), Алекс (для стройной), случайный,
    скалк, рыцарь, ниндзя, шахтёр, волшебник, зомби. Сохранённый скин сразу
    появляется в списке, а «OK» надевает его в игре;
  - **Создать / изменить плащ** — тот же редактор для плаща и элитр (снаружи, изнанка, края,
    крылья элитр). Новый плащ начинается с белого листа; есть и узоры: скалк, градиент, полосы, шахматы, пламя, эмблема. Mojang разрешает
    носить в игре только выданные плащи, поэтому свой плащ виден в лаунчере (в списке плащей
    с пометкой «свой»), а «OK» оставляет в игре текущий плащ аккаунта;
- **Сменить ник** — новый ник проверяется на занятость прямо при вводе. Mojang разрешает
  менять ник раз в 30 дней;
- **Учётные записи**.

### Зависимости модов

- Каталог модов (Modrinth/CurseForge) и ИИ-агент ставят зависимости сами.
- Если добавить jar-файлы вручную («Добавить файл» или перетаскиванием), лаунчер **сам скачает**
  недостающие зависимости вместе с модами — без вопросов.
- Названия модов не переводятся: по-русски можно только искать.

### Анимации

- Плитки левой панели плавно подсвечиваются при наведении, кристалл на рамке мерцает.
- Аркан на плитке и в чате живой.
- Сообщения в чате плавно появляются, чат сам прокручивается к новым.
- Вкладки во всех окнах сменяются с плавным проявлением.

### Лицензия Minecraft

Как и в Prism Launcher, для игры нужна учётная запись Microsoft с купленным Minecraft:
мастер первого запуска предлагает войти, и без лицензии игра не запустится. Эта проверка
не изменялась.

### Проверка запуска

Запуск проверен на тестовом стенде: установки 1.21.1 и 1.12.2, лаунчер сам выбрал Java 21 и
Java 8, при смене версии установки 1.21.1 → 1.12.2 Java переключилась с 21 на 8, игровой
процесс запустился с модами из папки `mods` и завершился с кодом 0.

## Сборка

```sh
git clone --recursive <этот репозиторий>
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Подробнее о зависимостях: https://prismlauncher.org/wiki/development/build-instructions/

**Windows без Windows.** `tools/build-windows-cross.sh` собирает 64-битную версию для Windows прямо на Linux:
MinGW-w64 GCC, Qt 6.4.2 для MinGW (aqtinstall), библиотеки из MSYS2 (libarchive, qrencode, zlib, toml++ и
среда выполнения GCC) и cmark, собранный тут же. Получается переносимая папка и `VelloraClient-Windows.zip`:
распаковать куда угодно и запустить `vellora.exe`. На самой Windows по-прежнему можно собрать через
`build-windows.bat` (см. `HOW-TO-BUILD-RU.txt`).

---

<p align="center">
<picture>
  <source media="(prefers-color-scheme: dark)" srcset="/program_info/org.prismlauncher.PrismLauncher.logo-darkmode.svg">
  <source media="(prefers-color-scheme: light)" srcset="/program_info/org.prismlauncher.PrismLauncher.logo.svg">
  <img alt="Prism Launcher" src="/program_info/org.prismlauncher.PrismLauncher.logo.svg" width="40%">
</picture>
</p>

<p align="center">
  Prism Launcher is a custom launcher for Minecraft that allows you to easily manage multiple installations of Minecraft at once.<br />
  <br />This is a <b>fork</b> of the MultiMC Launcher and is <b>not</b> endorsed by it.
</p>

## Installation

<a href="https://repology.org/project/prismlauncher/versions">
    <img src="https://repology.org/badge/vertical-allrepos/prismlauncher.svg" alt="Packaging status" align="right">
</a>

- All downloads and instructions for Prism Launcher can be found on our [Website](https://prismlauncher.org/download).
- Last build status can be found in the [GitHub Actions](https://github.com/PrismLauncher/PrismLauncher/actions) tab (this also includes the pull requests status).

### Development Builds

Please understand that these builds are not intended for most users. There may be bugs, and other instabilities. You have been warned.

There are development builds available through:

- [GitHub Actions](https://github.com/PrismLauncher/PrismLauncher/actions) (includes builds from pull requests opened by contributors)
- [nightly.link](https://prismlauncher.org/nightly) (this will always point only to the latest version of develop)

These have debug information in the binaries, so their file sizes are relatively larger.

Prebuilt Development builds are provided for **Linux**, **Windows** and **macOS**.

On Linux, we also offer our own [Flatpak nightly repository](https://github.com/PrismLauncher/flatpak). Most software centers are able to install it by opening [this link](https://flatpak.prismlauncher.org/prismlauncher-nightly.flatpakref).

## Community & Support

Feel free to create a GitHub issue if you find a bug or want to suggest a new feature. We have multiple community spaces where other community members can help you:

- **Our Discord server:**

[![Prism Launcher Discord server](https://discordapp.com/api/guilds/1031648380885147709/widget.png?style=banner3)](https://prismlauncher.org/discord)

- **Our Matrix space:**

[![Prism Launcher Space](https://img.shields.io/matrix/prismlauncher:matrix.org?style=for-the-badge&label=Matrix%20Space&logo=matrix&color=purple)](https://prismlauncher.org/matrix)

- **Our Subreddit:**

[![r/PrismLauncher](https://img.shields.io/reddit/subreddit-subscribers/prismlauncher?style=for-the-badge&logo=reddit)](https://prismlauncher.org/reddit)

## Translations

The translation effort for Prism Launcher is hosted on [Weblate](https://hosted.weblate.org/projects/prismlauncher/launcher/) and information about translating Prism Launcher is available at <https://github.com/PrismLauncher/Translations>.

## Building

If you want to build Prism Launcher yourself, check the [build instructions](https://prismlauncher.org/wiki/development/build-instructions).

## Sponsors & Partners

We thank all the wonderful backers over at Open Collective! Support Prism Launcher by [becoming a backer](https://opencollective.com/prismlauncher).

[![OpenCollective Backers](https://opencollective.com/prismlauncher/backers.svg?width=890&limit=1000)](https://opencollective.com/prismlauncher#backers)

Thanks to JetBrains for providing us a few licenses for all their products, as part of their [Open Source program](https://www.jetbrains.com/opensource/).

<a href="https://jb.gg/OpenSource">
<picture>
  <source media="(prefers-color-scheme: dark)" srcset="https://www.jetbrains.com/company/brand/img/logo_jb_dos_4.svg">
  <source media="(prefers-color-scheme: light)" srcset="https://resources.jetbrains.com/storage/products/company/brand/logos/jetbrains.svg">
  <img alt="JetBrains logo" src="https://resources.jetbrains.com/storage/products/company/brand/logos/jetbrains.svg" width="40%">
</picture>
</a>

Thanks to Weblate for hosting our translation efforts.

<a href="https://hosted.weblate.org/engage/prismlauncher/">
<img src="https://hosted.weblate.org/widgets/prismlauncher/-/open-graph.png" alt="Translation status" width="300" />
</a>

Thanks to Netlify for providing us their excellent web services, as part of their [Open Source program](https://www.netlify.com/open-source/).

<a href="https://www.netlify.com"> <img src="https://www.netlify.com/v3/img/components/netlify-color-accent.svg" alt="Deploys by Netlify" /> </a>

Thanks to the awesome people over at [MacStadium](https://www.macstadium.com/), for providing M1-Macs for development purposes!

<a href="https://www.macstadium.com"><img src="https://uploads-ssl.webflow.com/5ac3c046c82724970fc60918/5c019d917bba312af7553b49_MacStadium-developerlogo.png" alt="Powered by MacStadium" width="300"></a>

## Forking/Redistributing/Custom builds policy

You are free to fork, redistribute and provide custom builds as long as you follow the terms of the [license](LICENSE) (this is a legal responsibility), and if you made code changes rather than just packaging a custom build, please do the following as a basic courtesy:

- Make it clear that your fork is not Prism Launcher and is not endorsed by or affiliated with the Prism Launcher project (<https://prismlauncher.org>).
- Go through [CMakeLists.txt](CMakeLists.txt) and change Prism Launcher's API keys to your own or set them to empty strings (`""`) to disable them (this way the program will still compile but the functionality requiring those keys will be disabled).

If you have any questions or want any clarification on the above conditions please make an issue and ask us.

If you are just building Prism Launcher for your distribution, please make sure to set the `Launcher_BUILD_PLATFORM` to a slug representing your distribution. Examples are `archlinux`, `fedora` and `nixpkgs`.

Note that if you build this software without removing the provided API keys in [CMakeLists.txt](CMakeLists.txt) you are accepting the following terms and conditions:

- [Microsoft Identity Platform Terms of Use](https://docs.microsoft.com/en-us/legal/microsoft-identity-platform/terms-of-use)
- [CurseForge 3rd Party API Terms and Conditions](https://support.curseforge.com/en/support/solutions/articles/9000207405-curse-forge-3rd-party-api-terms-and-conditions)

If you do not agree with these terms and conditions, then remove the associated API keys from the [CMakeLists.txt](CMakeLists.txt) file by setting them to an empty string (`""`).

## License [![https://github.com/PrismLauncher/PrismLauncher/blob/develop/LICENSE](https://img.shields.io/github/license/PrismLauncher/PrismLauncher?label=License&logo=gnu&color=C4282D)](LICENSE)

All launcher code is available under the GPL-3.0-only license.

The logo and related assets are under the CC BY-SA 4.0 license.
