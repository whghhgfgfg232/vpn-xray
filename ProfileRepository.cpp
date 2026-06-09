#include "ProfileRepository.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>

ProfileRepository::ProfileRepository()
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir::currentPath();
    }

    QDir().mkpath(baseDir);
    path_ = QDir(baseDir).filePath("profiles.json");
}

QList<Profile> ProfileRepository::load(QString *error) const
{
    return loadFromFile(path_, error);
}

bool ProfileRepository::save(const QList<Profile> &profiles, QString *error) const
{
    return saveToFile(path_, profiles, error);
}

QString ProfileRepository::storagePath() const
{
    return path_;
}

QList<Profile> ProfileRepository::loadFromFile(const QString &path, QString *error)
{
    QList<Profile> profiles;
    QFile file(path);
    if (!file.exists()) {
        return profiles;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = QStringLiteral("Не удалось открыть %1").arg(path);
        }
        return profiles;
    }

    const QByteArray raw = file.readAll();
    file.close();

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        if (error) {
            *error = QStringLiteral("JSON профилей повреждён или имеет неверный формат");
        }
        return profiles;
    }

    const QJsonArray array = doc.array();
    for (const QJsonValue &value : array) {
        if (value.isObject()) {
            profiles.append(Profile::fromJson(value.toObject()));
        }
    }

    return profiles;
}

bool ProfileRepository::saveToFile(const QString &path, const QList<Profile> &profiles, QString *error)
{
    QJsonArray array;
    for (const Profile &profile : profiles) {
        array.append(profile.toJson());
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = QStringLiteral("Не удалось сохранить %1").arg(path);
        }
        return false;
    }

    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}
