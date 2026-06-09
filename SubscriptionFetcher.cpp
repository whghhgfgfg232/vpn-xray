#include "SubscriptionFetcher.h"

#include "UriParser.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSet>
#include <QTimer>
#include <QUrl>

QByteArray SubscriptionFetcher::decodeBase64Relaxed(QString input)
{
    input = input.trimmed();
    input.replace('-', '+');
    input.replace('_', '/');

    while (input.size() % 4 != 0) {
        input.append('=');
    }

    return QByteArray::fromBase64(input.toUtf8());
}

QStringList SubscriptionFetcher::extractConfigLines(const QByteArray &payload)
{
    const QString direct = QString::fromUtf8(payload).trimmed();
    QStringList lines;

    if (direct.contains("://")) {
        lines = direct.split(QRegularExpression(QStringLiteral("[\r\n]+")), Qt::SkipEmptyParts);
    } else {
        const QString decoded = QString::fromUtf8(decodeBase64Relaxed(direct));
        lines = decoded.split(QRegularExpression(QStringLiteral("[\r\n]+")), Qt::SkipEmptyParts);
    }

    QStringList normalized;
    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }
        normalized.append(line);
    }

    return normalized;
}

bool SubscriptionFetcher::fetch(const Subscription &subscription, QList<Profile> *profiles, QString *error)
{
    if (profiles == nullptr) {
        if (error) {
            *error = QStringLiteral("Не передан буфер для профилей");
        }
        return false;
    }

    profiles->clear();

    const QUrl url(subscription.url);
    if (!url.isValid() || subscription.url.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("Некорректный URL подписки");
        }
        return false;
    }

    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("XrayQtClient/0.2"));
#if QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)
    request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#endif

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);

    QNetworkReply *reply = manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeout, &QTimer::timeout, reply, &QNetworkReply::abort);
    timeout.start(15000);
    loop.exec();

    const QByteArray body = reply->readAll();
    const QString replyError = reply->error() == QNetworkReply::NoError ? QString() : reply->errorString();
    reply->deleteLater();

    if (!replyError.isEmpty()) {
        if (error) {
            *error = replyError;
        }
        return false;
    }

    const QStringList lines = extractConfigLines(body);
    QSet<QString> seenUris;

    for (const QString &line : lines) {
        QString parseError;
        const auto parsed = UriParser::parse(line, &parseError);
        if (!parsed.has_value()) {
            continue;
        }

        Profile profile = parsed.value();
        profile.source = QStringLiteral("subscription");
        profile.subscriptionId = subscription.id;
        if (profile.name.isEmpty()) {
            profile.name = subscription.name;
        }

        if (seenUris.contains(profile.uri)) {
            continue;
        }
        seenUris.insert(profile.uri);
        profiles->append(profile);
    }

    if (profiles->isEmpty()) {
        if (error) {
            *error = QStringLiteral("В подписке не найдено поддерживаемых конфигов");
        }
        return false;
    }

    return true;
}
