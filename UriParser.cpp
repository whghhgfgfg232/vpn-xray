#include "UriParser.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>

namespace {
QString decodedFragment(const QUrl &url)
{
    return QUrl::fromPercentEncoding(url.fragment().toUtf8());
}
}

QByteArray UriParser::decodeBase64Relaxed(QString input)
{
    input = input.trimmed();
    input.replace('-', '+');
    input.replace('_', '/');

    while (input.size() % 4 != 0) {
        input.append('=');
    }

    return QByteArray::fromBase64(input.toUtf8());
}

std::optional<Profile> UriParser::parse(const QString &uri, QString *error)
{
    const QString trimmed = uri.trimmed();
    if (trimmed.isEmpty()) {
        if (error) {
            *error = QStringLiteral("Ссылка конфигурации пуста");
        }
        return std::nullopt;
    }

    if (trimmed.startsWith("vless://", Qt::CaseInsensitive)) {
        return parseVless(trimmed, error);
    }
    if (trimmed.startsWith("vmess://", Qt::CaseInsensitive)) {
        return parseVmess(trimmed, error);
    }
    if (trimmed.startsWith("trojan://", Qt::CaseInsensitive)) {
        return parseTrojan(trimmed, error);
    }
    if (trimmed.startsWith("ss://", Qt::CaseInsensitive)) {
        return parseShadowsocks(trimmed, error);
    }

    if (error) {
        *error = QStringLiteral("Поддерживаются только vless://, vmess://, trojan:// и ss:// ссылки");
    }
    return std::nullopt;
}

std::optional<Profile> UriParser::parseVless(const QString &uri, QString *error)
{
    const QUrl url(uri);
    if (!url.isValid()) {
        if (error) {
            *error = QStringLiteral("Некорректная VLESS-ссылка");
        }
        return std::nullopt;
    }

    Profile profile;
    profile.scheme = "vless";
    profile.host = url.host();
    profile.port = url.port(443);
    profile.credential = url.userName();
    profile.name = decodedFragment(url);
    profile.uri = uri;

    QUrlQuery query(url);
    profile.encryption = query.queryItemValue("encryption");
    if (profile.encryption.isEmpty()) {
        profile.encryption = "none";
    }

    profile.security = query.queryItemValue("security");
    if (profile.security.isEmpty()) {
        profile.security = "none";
    }

    profile.transport = query.queryItemValue("type");
    if (profile.transport.isEmpty()) {
        profile.transport = "tcp";
    }

    profile.path = query.queryItemValue("path", QUrl::FullyDecoded);
    profile.hostHeader = query.queryItemValue("host");
    profile.serviceName = query.queryItemValue("serviceName");
    profile.sni = query.queryItemValue("sni");
    profile.publicKey = query.queryItemValue("pbk");
    profile.shortId = query.queryItemValue("sid");
    profile.flow = query.queryItemValue("flow");

    if (profile.name.isEmpty()) {
        profile.name = QStringLiteral("%1:%2").arg(profile.host).arg(profile.port);
    }

    if (profile.host.isEmpty() || profile.credential.isEmpty() || profile.port <= 0) {
        if (error) {
            *error = QStringLiteral("VLESS-ссылка не содержит host/port/uuid");
        }
        return std::nullopt;
    }

    return profile;
}

std::optional<Profile> UriParser::parseVmess(const QString &uri, QString *error)
{
    const QString payload = uri.mid(QStringLiteral("vmess://").size());
    const QByteArray decoded = decodeBase64Relaxed(payload);
    if (decoded.isEmpty()) {
        if (error) {
            *error = QStringLiteral("Не удалось декодировать VMess base64");
        }
        return std::nullopt;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(decoded, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error) {
            *error = QStringLiteral("VMess JSON повреждён или имеет неверный формат");
        }
        return std::nullopt;
    }

    const QJsonObject obj = doc.object();
    Profile profile;
    profile.scheme = "vmess";
    profile.name = obj.value("ps").toString();
    profile.host = obj.value("add").toString();
    profile.port = obj.value("port").toVariant().toInt();
    profile.credential = obj.value("id").toString();
    profile.transport = obj.value("net").toString("tcp");
    profile.path = obj.value("path").toString();
    profile.hostHeader = obj.value("host").toString();
    profile.sni = obj.value("sni").toString();
    profile.serviceName = obj.value("serviceName").toString();
    profile.encryption = obj.value("scy").toString("auto");
    profile.uri = uri;

    const QString tlsValue = obj.value("tls").toString();
    if (tlsValue.compare("tls", Qt::CaseInsensitive) == 0) {
        profile.security = "tls";
    } else if (tlsValue.compare("reality", Qt::CaseInsensitive) == 0) {
        profile.security = "reality";
    } else {
        profile.security = "none";
    }

    if (profile.name.isEmpty()) {
        profile.name = QStringLiteral("%1:%2").arg(profile.host).arg(profile.port);
    }

    if (profile.host.isEmpty() || profile.credential.isEmpty() || profile.port <= 0) {
        if (error) {
            *error = QStringLiteral("VMess-ссылка не содержит обязательные поля add/port/id");
        }
        return std::nullopt;
    }

    return profile;
}

std::optional<Profile> UriParser::parseTrojan(const QString &uri, QString *error)
{
    const QUrl url(uri);
    if (!url.isValid()) {
        if (error) {
            *error = QStringLiteral("Некорректная Trojan-ссылка");
        }
        return std::nullopt;
    }

    Profile profile;
    profile.scheme = "trojan";
    profile.host = url.host();
    profile.port = url.port(443);
    profile.credential = url.userName();
    profile.name = decodedFragment(url);
    profile.uri = uri;

    QUrlQuery query(url);
    profile.security = query.queryItemValue("security");
    if (profile.security.isEmpty()) {
        profile.security = "tls";
    }
    profile.transport = query.queryItemValue("type");
    if (profile.transport.isEmpty()) {
        profile.transport = "tcp";
    }
    profile.path = query.queryItemValue("path", QUrl::FullyDecoded);
    profile.hostHeader = query.queryItemValue("host");
    profile.serviceName = query.queryItemValue("serviceName");
    profile.sni = query.queryItemValue("sni");

    if (profile.name.isEmpty()) {
        profile.name = QStringLiteral("%1:%2").arg(profile.host).arg(profile.port);
    }

    if (profile.host.isEmpty() || profile.credential.isEmpty() || profile.port <= 0) {
        if (error) {
            *error = QStringLiteral("Trojan-ссылка не содержит host/port/password");
        }
        return std::nullopt;
    }

    return profile;
}

std::optional<Profile> UriParser::parseShadowsocks(const QString &uri, QString *error)
{
    QString rest = uri.mid(QStringLiteral("ss://").size());

    QString name;
    const int hashPos = rest.indexOf('#');
    if (hashPos >= 0) {
        name = QUrl::fromPercentEncoding(rest.mid(hashPos + 1).toUtf8());
        rest = rest.left(hashPos);
    }

    const int queryPos = rest.indexOf('?');
    if (queryPos >= 0) {
        rest = rest.left(queryPos);
    }

    QString decodedLeft;
    QString hostPort;

    const int atPos = rest.indexOf('@');
    if (atPos >= 0) {
        const QString left = rest.left(atPos);
        hostPort = rest.mid(atPos + 1);
        const QByteArray maybeDecoded = decodeBase64Relaxed(left);
        decodedLeft = QString::fromUtf8(maybeDecoded);
        if (decodedLeft.isEmpty() || !decodedLeft.contains(':')) {
            decodedLeft = left;
        }
    } else {
        const QByteArray decoded = decodeBase64Relaxed(rest);
        decodedLeft = QString::fromUtf8(decoded);
    }

    QString normalized = decodedLeft;
    if (!hostPort.isEmpty()) {
        normalized += '@' + hostPort;
    }

    const QRegularExpression regex(QStringLiteral(R"(^([^:]+):(.+)@([^:]+):(\d+)$)"));
    const auto match = regex.match(normalized);
    if (!match.hasMatch()) {
        if (error) {
            *error = QStringLiteral("Не удалось разобрать Shadowsocks-ссылку");
        }
        return std::nullopt;
    }

    Profile profile;
    profile.scheme = "shadowsocks";
    profile.method = match.captured(1);
    profile.credential = match.captured(2);
    profile.host = match.captured(3);
    profile.port = match.captured(4).toInt();
    profile.name = name.isEmpty() ? QStringLiteral("%1:%2").arg(profile.host).arg(profile.port) : name;
    profile.uri = uri;
    profile.transport = "tcp";
    profile.security = "none";

    return profile;
}
