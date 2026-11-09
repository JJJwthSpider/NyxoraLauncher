// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "VelloraLook.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "Application.h"
#include "FileSystem.h"
#include "minecraft/Component.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "minecraft/auth/AccountList.h"
#include "ui/dialogs/WardrobeDialog.h"
#include "ui/themes/VelloraGuest.h"
#include "ui/widgets/VelloraCharacter.h"

namespace VelloraLook {
QMap<QString, QString> components(MinecraftInstance* instance)
{
    QMap<QString, QString> out;
    if (!instance) {
        return out;
    }
    auto profile = instance->getPackProfile();
    for (int i = 0; i < profile->rowCount(); i++) {
        const auto component = profile->getComponent(size_t(i));
        if (component) {
            out[component->getID()] = component->getVersion().isEmpty() ? component->m_version : component->getVersion();
        }
    }
    if (!out.isEmpty()) {
        return out;
    }
    // not loaded yet: the instance's own list of components says it
    QFile pack(FS::PathCombine(instance->instanceRoot(), "mmc-pack.json"));
    if (pack.open(QIODevice::ReadOnly)) {
        for (const auto& value : QJsonDocument::fromJson(pack.readAll()).object().value("components").toArray()) {
            const QJsonObject component = value.toObject();
            out[component.value("uid").toString()] = component.value("cachedVersion").toString(component.value("version").toString());
        }
    }
    return out;
}

bool fits(const QMap<QString, QString>& components)
{
    return components.value("net.minecraft") == MinecraftVersion &&
           (components.contains("net.fabricmc.fabric-loader") || components.contains("org.quiltmc.quilt-loader"));
}

namespace {
/// the look shown in the lobby: the account's skin (or the guest's) and the wardrobe's cape (or the account's)
void currentLook(QImage& skin, bool& slim, QImage& cape)
{
    const QImage generated = WardrobeDialog::generatedCape();
    const auto account = APPLICATION->accounts()->defaultAccount();
    if (account) {
        const auto& profile = account->accountData()->minecraftProfile;
        skin.loadFromData(profile.skin.data, "PNG");
        slim = profile.skin.variant.compare("slim", Qt::CaseInsensitive) == 0;
        if (generated.isNull()) {
            cape.loadFromData(profile.capes.value(profile.currentCape).data, "PNG");
        } else {
            cape = generated;
        }
    } else {
        skin = VelloraGuest::skin(&slim);
        cape = generated;
    }
}
}  // namespace

void prepare(BaseInstance* base)
{
    auto* instance = dynamic_cast<MinecraftInstance*>(base);
    if (!instance) {
        return;
    }
    const QString jar = FS::PathCombine(instance->modsRoot(), ModFile);
    if (!fits(components(instance))) {
        if (QFile::exists(jar)) {
            QFile::remove(jar);  // another version or loader now: Fabric would not start with the mod
        }
        return;
    }
    if (QFile::exists(jar + ".disabled")) {
        return;  // switched off by the player in the list of mods
    }
    // the mod of this launcher, written again when it differs (a newer launcher brings a newer mod)
    QFile bundled(":/vellora_mods/" + ModFile);
    if (!bundled.open(QIODevice::ReadOnly)) {
        qWarning() << "Vellora Cosmetics is not bundled with this launcher";
        return;
    }
    const QByteArray mod = bundled.readAll();
    QFile current(jar);
    if (!current.open(QIODevice::ReadOnly) || current.readAll() != mod) {
        current.close();
        QDir().mkpath(instance->modsRoot());
        QFile out(jar);
        if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate) || out.write(mod) != mod.size()) {
            qWarning() << "Vellora Cosmetics could not be put into" << instance->modsRoot();
            return;
        }
    }
    QImage skin, cape;
    bool slim = false;
    currentLook(skin, slim, cape);
    if (!VelloraCharacter::exportLook(FS::PathCombine(instance->gameRoot(), "config", "vellora-cosmetics"), skin, slim, cape,
                                      Cosmetics::worn(Cosmetics::Head), Cosmetics::worn(Cosmetics::Body), Cosmetics::worn(Cosmetics::Back))) {
        qWarning() << "The look for Vellora Cosmetics could not be written";
    }
}
}  // namespace VelloraLook
