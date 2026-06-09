#pragma once

#include <QJsonObject>
#include <QString>
#include <QUuid>

struct Profile {
    QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QString name;
    QString scheme;
    QString host;
    int port = 0;

    QString credential;
    QString method;

    QString encryption = "none";
    QString security = "none";
    QString transport = "tcp";

    QString path;
    QString hostHeader;
    QString serviceName;
    QString sni;
    QString publicKey;
    QString shortId;
    QString flow;

    QString uri;
    int lastPingMs = -1;
    QString source = "manual";
    QString subscriptionId;

    QJsonObject toJson() const
    {
        return QJsonObject{
            {"id", id},
            {"name", name},
            {"scheme", scheme},
            {"host", host},
            {"port", port},
            {"credential", credential},
            {"method", method},
            {"encryption", encryption},
            {"security", security},
            {"transport", transport},
            {"path", path},
            {"hostHeader", hostHeader},
            {"serviceName", serviceName},
            {"sni", sni},
            {"publicKey", publicKey},
            {"shortId", shortId},
            {"flow", flow},
            {"uri", uri},
            {"lastPingMs", lastPingMs},
            {"source", source},
            {"subscriptionId", subscriptionId}
        };
    }

    static Profile fromJson(const QJsonObject &obj)
    {
        Profile p;
        p.id = obj.value("id").toString(QUuid::createUuid().toString(QUuid::WithoutBraces));
        p.name = obj.value("name").toString();
        p.scheme = obj.value("scheme").toString();
        p.host = obj.value("host").toString();
        p.port = obj.value("port").toInt();
        p.credential = obj.value("credential").toString();
        p.method = obj.value("method").toString();
        p.encryption = obj.value("encryption").toString("none");
        p.security = obj.value("security").toString("none");
        p.transport = obj.value("transport").toString("tcp");
        p.path = obj.value("path").toString();
        p.hostHeader = obj.value("hostHeader").toString();
        p.serviceName = obj.value("serviceName").toString();
        p.sni = obj.value("sni").toString();
        p.publicKey = obj.value("publicKey").toString();
        p.shortId = obj.value("shortId").toString();
        p.flow = obj.value("flow").toString();
        p.uri = obj.value("uri").toString();
        p.lastPingMs = obj.value("lastPingMs").toInt(-1);
        p.source = obj.value("source").toString("manual");
        p.subscriptionId = obj.value("subscriptionId").toString();
        return p;
    }
};
