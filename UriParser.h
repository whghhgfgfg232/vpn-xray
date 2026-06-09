#pragma once

#include "Profile.h"
#include <optional>

class UriParser {
public:
    static std::optional<Profile> parse(const QString &uri, QString *error = nullptr);

private:
    static QByteArray decodeBase64Relaxed(QString input);
    static std::optional<Profile> parseVless(const QString &uri, QString *error);
    static std::optional<Profile> parseVmess(const QString &uri, QString *error);
    static std::optional<Profile> parseTrojan(const QString &uri, QString *error);
    static std::optional<Profile> parseShadowsocks(const QString &uri, QString *error);
};
