# Nyxora Launcher: правила проекта

Лаунчер называется **Nyxora Launcher** (раньше — Vellora Client). Внутренние имена остались от старого названия и
не переименовываются: классы `Vellora…`, идентификатор мода `vellora_cosmetics`, файлы `vellora-cosmetics-*.jar`,
папка `config/vellora-cosmetics`, идентификаторы вещей гардероба (`vellora_cape`, `v_badge`), пакет Android
`com.sculkshoot.client`. Данные Vellora Client на ПК переносятся при первом запуске (`Application::handleDataMigration`).

## Гость (бесплатная версия)

Игрок может войти как гость (ник без аккаунта Microsoft). Это бесплатная версия, и для неё действует правило владельца
проекта:

- **Безлимитные установки.** Гость создаёт сколько угодно установок. Никаких лимитов на число установок, сборок или
  миров не вводить.
- **Любые моды.** Гость ставит любые моды, загрузчики (Fabric, Forge, NeoForge, Quilt), ресурспаки, шейдеры и готовые
  сборки с Modrinth и CurseForge, как и игрок с аккаунтом.
- **Любые версии.** Гость создаёт установки на любой версии Minecraft. Играет он в официальном демо-режиме Minecraft
  (бесплатная пробная версия от Mojang, флаг `--demo`) — со сборкой как есть, с модами.
- Единственная граница для запуска — версии до 1.3.1: в них демо-режима нет, `--demo` игнорируется, и запустилась бы
  полная игра без покупки. Такие версии гостю создавать можно, запускать — нет (`VelloraGuest::demoWorks`,
  `GameEngine.demoWorks`). Полную игру без покупки не делать: она открывается после входа аккаунтом Microsoft, на
  котором куплен Minecraft: Java Edition.
- Гостю закрыт только ИИ (Аркан, ИИ-генераторы скинов и плащей): `VelloraGuest::aiAllowed`, `MainActivity.requireAi`.

## Образ в игре (мод Nyxora Cosmetics)

В игре видны только скин, плащ и аксессуары слотов «голова», «тело», «спина» — не питомцы, не «в руке», не эффекты
(правило владельца проекта). Это делает свой Fabric-мод `cosmetics-mod/` (Minecraft 1.20.1, Fabric и Quilt):
лаунчер перед запуском кладёт его в `mods` и пишет `config/vellora-cosmetics/look.json` (кубики аксессуаров на частях
модели игрока Minecraft), `skin.png`, `cape.png` — ПК: `VelloraLook::prepare` + `VelloraCharacter::exportLook`,
телефон: `GameLook.prepare` + `CharacterView.exportLook` (выгрузки ПК и телефона должны совпадать). Из неподходящих
сборок мод убирается. Пересборка мода: `tools/build-cosmetics-mod.sh` (jar кладётся в `launcher/resources/vellora_mods/`
и `android/assets/`).

## Где код гостя

Где это в коде: ПК — `launcher/ui/themes/VelloraGuest.*`, `Application::launch`, `LaunchController`; телефон —
`Account.guest`, `MainActivity.playInstallation`, `PojavEngine.prepare` (демо-аккаунт `Demo.<ник>`).
