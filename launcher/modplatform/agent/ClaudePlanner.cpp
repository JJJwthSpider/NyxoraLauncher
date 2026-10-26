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
#include "ClaudePlanner.h"

#include <algorithm>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "Application.h"

namespace ModpackAgent {

namespace {

const char* s_systemPrompt = R"(You plan Minecraft modpacks that a launcher assembles automatically from Modrinth and CurseForge.
The user describes the pack they want, usually in Russian, for example "сборка с зомби neoforge 1.21.1".

Decide:
- minecraft_version: the version the user asked for. If none was given, use "1.21.1".
- loader: the mod loader the user asked for (neoforge, forge, fabric or quilt). If none was given, use neoforge for 1.20.2 and newer, forge for older versions.
- themes: 2 to 8 content themes that make up the pack. Each theme has a short title in the user's language, 1 to 3 English search queries, and how many mods to take for it (1 to 5). Think about what a player expects from such a pack: a zombie pack also wants weapons, armor and survival mods.
- mods: project slugs (as used in Modrinth or CurseForge URLs) of well known content mods that clearly fit the request and exist for that version and loader. Only list slugs you are sure about; leave the list empty otherwise.
- name: a short catchy pack name in the user's language.
- summary: one or two sentences in the user's language describing the pack.

Performance mods (Sodium, FerriteCore, ModernFix and so on) and quality-of-life mods (JEI, Jade) are added by the launcher itself, so do not include them.)";

QJsonObject planSchema()
{
    auto stringType = QJsonObject{ { "type", "string" } };
    auto themeSchema = QJsonObject{
        { "type", "object" },
        { "additionalProperties", false },
        { "required", QJsonArray{ "title", "queries", "count" } },
        { "properties", QJsonObject{ { "title", stringType },
                                     { "queries", QJsonObject{ { "type", "array" }, { "items", stringType } } },
                                     { "count", QJsonObject{ { "type", "integer" } } } } },
    };
    return QJsonObject{
        { "type", "object" },
        { "additionalProperties", false },
        { "required", QJsonArray{ "name", "summary", "minecraft_version", "loader", "themes", "mods" } },
        { "properties",
          QJsonObject{
              { "name", stringType },
              { "summary", stringType },
              { "minecraft_version", stringType },
              { "loader", QJsonObject{ { "type", "string" }, { "enum", QJsonArray{ "neoforge", "forge", "fabric", "quilt" } } } },
              { "themes", QJsonObject{ { "type", "array" }, { "items", themeSchema } } },
              { "mods", QJsonObject{ { "type", "array" }, { "items", stringType } } },
          } },
    };
}

}  // namespace

ClaudePlanner::ClaudePlanner(QObject* parent) : QObject(parent) {}

void ClaudePlanner::plan(const QString& request, const QString& apiKey)
{
    abort();
    m_request = request;

    QNetworkRequest req(QUrl("https://api.anthropic.com/v1/messages"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("x-api-key", apiKey.trimmed().toUtf8());
    req.setRawHeader("anthropic-version", "2026-10-21");
    // if a safety classifier declines, let the API re-run the request on its recommended fallback model
    req.setRawHeader("anthropic-beta", "server-side-fallback-2026-07-01");
    req.setHeader(QNetworkRequest::UserAgentHeader, APPLICATION->getUserAgent());
    req.setTransferTimeout(120000);

    QJsonObject body{
        { "model", "claude-opus-5-5" },
        { "max_tokens", 16000 },
        { "fallbacks", "default" },
        { "system", s_systemPrompt },
        { "output_config",
          QJsonObject{ { "effort", "low" }, { "format", QJsonObject{ { "type", "json_schema" }, { "schema", planSchema() } } } } },
        { "messages", QJsonArray{ QJsonObject{ { "role", "user" }, { "content", request } } } },
    };

    m_reply = APPLICATION->network()->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(m_reply, &QNetworkReply::finished, this, &ClaudePlanner::onFinished);
}

void ClaudePlanner::abort()
{
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}

void ClaudePlanner::onFinished()
{
    auto* reply = m_reply.data();
    m_reply = nullptr;
    if (!reply) {
        return;
    }
    reply->deleteLater();

    const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const auto data = reply->readAll();
    const auto response = QJsonDocument::fromJson(data).object();

    if (reply->error() != QNetworkReply::NoError || status != 200) {
        auto message = response["error"].toObject()["message"].toString();
        if (message.isEmpty()) {
            message = reply->errorString();
        }
        if (status == 401) {
            message = tr("The API key was not accepted (%1).").arg(message);
        }
        emit failed(message);
        return;
    }

    if (response["stop_reason"].toString() == "refusal") {
        emit failed(tr("Claude declined this request."));
        return;
    }

    QString text;
    for (const auto& block : response["content"].toArray()) {
        auto obj = block.toObject();
        if (obj["type"].toString() == "text") {
            text += obj["text"].toString();
        }
    }
    const auto json = QJsonDocument::fromJson(text.toUtf8()).object();
    if (json.isEmpty()) {
        emit failed(tr("Claude returned an answer that could not be read."));
        return;
    }

    // start from the local plan so version/loader detection and the essential mods stay consistent
    auto plan = parseLocally(m_request);
    plan.fromClaude = true;

    const auto version = json["minecraft_version"].toString().trimmed();
    if (!version.isEmpty() && !plan.versionGiven) {
        plan.minecraftVersion = version;
    }
    const auto loader = json["loader"].toString().trimmed().toLower();
    if (!loader.isEmpty() && !plan.loaderGiven) {
        plan.loader = loader;
    }
    if (auto name = json["name"].toString().trimmed(); !name.isEmpty()) {
        plan.name = name;
    }
    if (auto summary = json["summary"].toString().trimmed(); !summary.isEmpty()) {
        plan.summary = summary;
    }

    QList<Theme> themes;
    for (const auto& value : json["themes"].toArray()) {
        auto obj = value.toObject();
        Theme theme;
        theme.title = obj["title"].toString();
        for (const auto& query : obj["queries"].toArray()) {
            if (auto q = query.toString().trimmed(); !q.isEmpty()) {
                theme.queries << q;
            }
        }
        theme.count = std::clamp(obj["count"].toInt(3), 1, 5);
        if (!theme.title.isEmpty() && !theme.queries.isEmpty()) {
            themes << theme;
        }
    }
    if (!themes.isEmpty()) {
        plan.themes = themes;
    }

    plan.curated = essentialMods(plan.loader, requestWantsShaders(m_request), requestWantsMap(m_request));
    const auto content = tr("Suggested by Claude");
    for (const auto& slug : json["mods"].toArray()) {
        if (auto s = slug.toString().trimmed().toLower(); !s.isEmpty()) {
            plan.curated << CuratedMod{ { s }, content };
        }
    }

    emit planned(plan);
}

}  // namespace ModpackAgent
