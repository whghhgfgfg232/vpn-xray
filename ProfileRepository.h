#pragma once

#include "Profile.h"
#include <QList>
#include <QString>

class ProfileRepository {
public:
    ProfileRepository();

    QList<Profile> load(QString *error = nullptr) const;
    bool save(const QList<Profile> &profiles, QString *error = nullptr) const;
    QString storagePath() const;

    static QList<Profile> loadFromFile(const QString &path, QString *error = nullptr);
    static bool saveToFile(const QString &path, const QList<Profile> &profiles, QString *error = nullptr);

private:
    QString path_;
};
