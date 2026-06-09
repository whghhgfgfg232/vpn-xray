#pragma once

#include <QJsonObject>
#include <QString>
#include <QUuid>

struct Subscription {
    QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString name;
    QString url;
    bool enabled = true;
    QString lastUpdatedIso;
    QString lastError;

    QJsonObject toJson() const
    {
        return QJsonObject{
            {"id", id},
            {"name", name},
            {"url", url},
            {"enabled", enabled},
            {"lastUpdatedIso", lastUpdatedIso},
            {"lastError", lastError}
        };
    }

    static Subscription fromJson(const QJsonObject &obj)
    {
        Subscription s;
        s.id = obj.value("id").toString(QUuid::createUuid().toString(QUuid::WithoutBraces));
        s.name = obj.value("name").toString();
        s.url = obj.value("url").toString();
        s.enabled = obj.value("enabled").toBool(true);
        s.lastUpdatedIso = obj.value("lastUpdatedIso").toString();
        s.lastError = obj.value("lastError").toString();
        return s;
    }
};
