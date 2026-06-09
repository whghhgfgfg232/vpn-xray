#include "SubscriptionRepository.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>

SubscriptionRepository::SubscriptionRepository()
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir::currentPath();
    }

    QDir().mkpath(baseDir);
    path_ = QDir(baseDir).filePath("subscriptions.json");
}

QList<Subscription> SubscriptionRepository::load(QString *error) const
{
    QList<Subscription> subscriptions;
    QFile file(path_);
    if (!file.exists()) {
        return subscriptions;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = QStringLiteral("Не удалось открыть %1").arg(path_);
        }
        return subscriptions;
    }

    const QByteArray raw = file.readAll();
    file.close();

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        if (error) {
            *error = QStringLiteral("subscriptions.json повреждён или имеет неверный формат");
        }
        return subscriptions;
    }

    const QJsonArray array = doc.array();
    for (const QJsonValue &value : array) {
        if (value.isObject()) {
            subscriptions.append(Subscription::fromJson(value.toObject()));
        }
    }

    return subscriptions;
}

bool SubscriptionRepository::save(const QList<Subscription> &subscriptions, QString *error) const
{
    QJsonArray array;
    for (const Subscription &subscription : subscriptions) {
        array.append(subscription.toJson());
    }

    QFile file(path_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = QStringLiteral("Не удалось сохранить %1").arg(path_);
        }
        return false;
    }

    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

QString SubscriptionRepository::storagePath() const
{
    return path_;
}
