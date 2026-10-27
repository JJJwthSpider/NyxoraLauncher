# SculkShoot Client

Лаунчер Minecraft на основе Prism Launcher 11.1.1 в цветах **скалка (sculk)** из Minecraft.
Программа называется **SculkShoot Client**, исполняемый файл — `sculkshoot`.

## Что изменено

- **Тема «Скалк»** в трёх вариантах (`launcher/ui/themes/SculkTheme.cpp`):
  - **«Скалк»** (по умолчанию) — светлее прежней: глубокий бирюзовый фон (#1c4049),
    светлые карточки и светящийся акцент (#3ff0f8);
  - **«Скалк (светлая)»** — светлый фон и тёмно-бирюзовый акцент; иконки сами
    переключаются на набор «Скалк (светлый)» (`launcher/resources/sculk_light/`);
  - **«Скалк (тёмная)»** — прежний очень тёмный вариант.
  Современный вид: скруглённые кнопки и поля ввода, тонкие полосы прокрутки, вкладки с
  подчёркиванием, подсветка при наведении и фокусе, градиентный прогресс-бар,
  свои флажки/радиокнопки/стрелки (`launcher/resources/sculk_theme/`).
  Цвета лога (ошибки, предупреждения) подобраны под тёмный фон.
- **Шрифт как в Minecraft.** В темах «Скалк» весь интерфейс написан пиксельным шрифтом
  Tiny5 (лицензия SIL OFL, `launcher/resources/sculk_theme/fonts/`), с полной кириллицей.
- **Текстура скалка.** Фон списка установок, левая панель и плитки — пиксельный блок скалка
  (пятна разных оттенков и светящиеся прожилки), строится из цветов темы
  (`SculkTheme::texture`), поэтому подходит ко всем трём вариантам.
- **Текстура на всём фоне окна**: верхняя и правая панели, строка состояния и меню тоже из скалка.
- **Живой фон.** Прожилки скалка светятся в такт сердцебиению Вардена (сильный удар, слабый,
  пауза), снизу вверх плывут светящиеся частицы скалка — в списке установок и на плитках
  левой панели (`launcher/ui/themes/SculkAmbience.cpp`). Неподвижные части плиток кэшируются,
  а между ударами перерисовываются только области вокруг частиц, чтобы не грузить процессор.
  Выключается галочкой «Живой фон скалка» в *Настройки → Внешний вид*.
- **Кнопка «Играть» на каждой установке** — запускает её одним нажатием.
- **Набор иконок «Sculk»** (`launcher/resources/sculk/`) — плоские иконки с бирюзовым свечением.
- Тема и иконки «Sculk» включаются **по умолчанию** при первом запуске
  (`launcher/Application.cpp`); переключить можно в *Настройки → Внешний вид*.
- **Уникальная иконка SculkShoot**: прицел из скалка — кольцо с разрывами и рисками,
  из которого растут рога Вардена, а в центре светится душа сенсора скалка.
  Генерируется скриптом `program_info/warden_icon.py` (SVG-иконки, логотипы, иконка ИИ-агента);
  `sculkshoot.ico`, `sculkshoot.icns` и PNG пересозданы из него.
- **Java меняется вместе с версией Minecraft.** Перед каждым запуском лаунчер выбирает
  подходящую Java (8 для 1.16 и старше, 17 для 1.17–1.20.4, 21 для 1.20.5+): сначала среди
  установленных, а если нужной нет — скачивает её сам. Java, выбранная вручную в настройках
  установки, не трогается (`launcher/minecraft/launch/AutoInstallJava.cpp`).

### Левая панель главного окна

Три большие плитки в одинаковой фирменной рамке (срезанные углы, светящиеся уголки,
«прожилки» скалка и кристалл-катализатор сверху; `launcher/ui/widgets/LaunchPanel.cpp`):

1. **ИИ-агент** — анимированная голова Вардена (`launcher/ui/widgets/WardenWidget.cpp`):
   рога покачиваются и светятся, ореол бьётся как сердце Вардена, поднимаются частицы скалка;
   при наведении сердце бьётся чаще. Открывает вкладку «ИИ-сборка».
2. **Готовые сборки** (скрещённые гаечные ключи) — каталоги готовых сборок Modrinth и CurseForge.
3. **Новая установка** (белый плюс на чёрном) — создать установку самому, как в Prism Launcher:
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

- **Своя картинка как иконка установки.** В окне выбора иконки есть кнопка «Своя картинка...»:
  выберите любой PNG/JPG/SVG с компьютера, и он сразу станет иконкой установки.
- **Самостоятельная сборка.** В «Быстром создании» стоит галочка «Сразу после создания открыть
  каталог модов». После «ОК» открывается окно добавления модов с вкладками Modrinth и CurseForge.

### ИИ-сборка: чат с Варденом

Вкладка **«ИИ-сборка»** (плитка с Варденом слева) — это чат. Варден сам ведёт разговор:

1. спрашивает, **про что сборка** — например: «сделай мне сборку модов про зомби-апокалипсис,
   должны быть города, зомби и оружие»;
2. спрашивает **загрузчик и версию** Minecraft. Подходит **любая версия**, для которой есть
   загрузчик: человек сам пишет её («Forge 1.7.10», «Fabric 1.20.1», «NeoForge 1.21.1»…).
   Варден проверяет, что такая версия Minecraft существует и что для неё есть загрузчик;
3. ищет моды по темам **на Modrinth и CurseForge** (источники выбираются галочками), добавляет
   моды на **оптимизацию** (Sodium/Embeddium, FerriteCore, ModernFix, Entity Culling,
   ImmediatelyFast, Dynamic FPS, Lithium) и **удобство** (JEI, Jade, AppleSkin, Mouse Tweaks),
   по просьбе — шейдеры и карту, и все **обязательные зависимости**;
4. проверяет совместимость с версией и загрузчиком, убирает дубли между площадками,
   несовместимые моды и моды, которые нельзя скачать через сторонние лаунчеры;
5. пишет итог (сколько модов по темам, на оптимизацию и библиотек), а справа показывает
   состав сборки с галочками. «ОК» создаёт установку. Файлы скачиваются и проверяются по хешу.

После этого с Варденом можно продолжать разговор: «добавь магию», «добавь шейдеры»,
«давай на Fabric 1.20.1», «начать заново». Пока Варден работает, точки в его облачке
прыгают, а в чате обновляется пузырь с прогрессом.

Без настроек агент понимает запросы по встроенным правилам (около 25 тем, русские и
английские слова). Если ввести ключ **Anthropic API** (шестерёнка в шапке чата), запрос
сначала разбирает Claude (`claude-opus-5-5`, структурированный JSON-ответ). Если Claude
недоступен, агент автоматически переходит на встроенные правила.

Код: `launcher/modplatform/agent/` (план, Claude, сборщик, импорт),
`launcher/ui/pages/modplatform/agent/AgentPage.cpp` (чат).

### Скины и ник

Внизу левой панели — кнопка профиля (лицо скина и ник). Меню:
- **Скины и плащи** — менеджер скинов: 3D-превью, загрузка из файла, по ссылке или по нику,
  выбор плаща, модель Стива или Алекс;
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
- Варден на плитке и в чате живой.
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
