#include "XrayConfigBuilder.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
QJsonObject buildStreamSettings(const Profile &profile)
{
    QJsonObject stream;
    stream.insert("network", profile.transport.isEmpty() ? QStringLiteral("tcp") : profile.transport);

    const QString security = profile.security.isEmpty() ? QStringLiteral("none") : profile.security;
    stream.insert("security", security);

    if (profile.transport.compare(QStringLiteral("ws"), Qt::CaseInsensitive) == 0) {
        QJsonObject headers;
        if (!profile.hostHeader.isEmpty()) {
            headers.insert("Host", profile.hostHeader);
        }

        QJsonObject ws;
        ws.insert("path", profile.path.isEmpty() ? QStringLiteral("/") : profile.path);
        if (!headers.isEmpty()) {
            ws.insert("headers", headers);
        }
        stream.insert("wsSettings", ws);
    } else if (profile.transport.compare(QStringLiteral("grpc"), Qt::CaseInsensitive) == 0) {
        QJsonObject grpc;
        grpc.insert("serviceName", profile.serviceName);
        stream.insert("grpcSettings", grpc);
    }

    if (security.compare(QStringLiteral("tls"), Qt::CaseInsensitive) == 0) {
        QJsonObject tls;
        tls.insert("serverName", profile.sni.isEmpty() ? profile.host : profile.sni);
        tls.insert("allowInsecure", false);
        tls.insert("fingerprint", QStringLiteral("chrome"));
        stream.insert("tlsSettings", tls);
    } else if (security.compare(QStringLiteral("reality"), Qt::CaseInsensitive) == 0) {
        QJsonObject reality;
        reality.insert("serverName", profile.sni.isEmpty() ? profile.host : profile.sni);
        reality.insert("fingerprint", QStringLiteral("chrome"));
        reality.insert("publicKey", profile.publicKey);
        reality.insert("shortId", profile.shortId);
        reality.insert("spiderX", QStringLiteral("/"));
        stream.insert("realitySettings", reality);
    }

    return stream;
}

QJsonObject buildOutbound(const Profile &profile)
{
    QJsonObject outbound;
    outbound.insert("tag", QStringLiteral("proxy"));

    if (profile.scheme == QStringLiteral("vless")) {
        outbound.insert("protocol", QStringLiteral("vless"));

        QJsonObject user;
        user.insert("id", profile.credential);
        user.insert("encryption", profile.encryption.isEmpty() ? QStringLiteral("none") : profile.encryption);
        if (!profile.flow.isEmpty()) {
            user.insert("flow", profile.flow);
        }

        QJsonObject server;
        server.insert("address", profile.host);
        server.insert("port", profile.port);
        server.insert("users", QJsonArray{user});

        outbound.insert("settings", QJsonObject{{QStringLiteral("vnext"), QJsonArray{server}}});
        outbound.insert("streamSettings", buildStreamSettings(profile));
        return outbound;
    }

    if (profile.scheme == QStringLiteral("vmess")) {
        outbound.insert("protocol", QStringLiteral("vmess"));

        QJsonObject user;
        user.insert("id", profile.credential);
        user.insert("alterId", 0);
        user.insert("security", profile.encryption.isEmpty() ? QStringLiteral("auto") : profile.encryption);

        QJsonObject server;
        server.insert("address", profile.host);
        server.insert("port", profile.port);
        server.insert("users", QJsonArray{user});

        outbound.insert("settings", QJsonObject{{QStringLiteral("vnext"), QJsonArray{server}}});
        outbound.insert("streamSettings", buildStreamSettings(profile));
        return outbound;
    }

    if (profile.scheme == QStringLiteral("trojan")) {
        outbound.insert("protocol", QStringLiteral("trojan"));

        QJsonObject server;
        server.insert("address", profile.host);
        server.insert("port", profile.port);
        server.insert("password", profile.credential);

        outbound.insert("settings", QJsonObject{{QStringLiteral("servers"), QJsonArray{server}}});
        outbound.insert("streamSettings", buildStreamSettings(profile));
        return outbound;
    }

    outbound.insert("protocol", QStringLiteral("shadowsocks"));
    QJsonObject server;
    server.insert("address", profile.host);
    server.insert("port", profile.port);
    server.insert("method", profile.method);
    server.insert("password", profile.credential);
    outbound.insert("settings", QJsonObject{{QStringLiteral("servers"), QJsonArray{server}}});
    return outbound;
}
}

QByteArray XrayConfigBuilder::buildJson(const Profile &profile, const ClientRunOptions &options)
{
    QJsonObject socksInbound;
    socksInbound.insert("tag", QStringLiteral("socks-in"));
    socksInbound.insert("listen", QStringLiteral("127.0.0.1"));
    socksInbound.insert("port", 10808);
    socksInbound.insert("protocol", QStringLiteral("socks"));
    socksInbound.insert("settings", QJsonObject{{QStringLiteral("udp"), true}});
    socksInbound.insert("sniffing", QJsonObject{{QStringLiteral("enabled"), true}, {QStringLiteral("destOverride"), QJsonArray{QStringLiteral("http"), QStringLiteral("tls"), QStringLiteral("quic")}}});

    QJsonObject httpInbound;
    httpInbound.insert("tag", QStringLiteral("http-in"));
    httpInbound.insert("listen", QStringLiteral("127.0.0.1"));
    httpInbound.insert("port", 10809);
    httpInbound.insert("protocol", QStringLiteral("http"));

    QJsonArray inbounds{socksInbound, httpInbound};

    if (options.enableStatsApi) {
        QJsonObject apiInbound;
        apiInbound.insert("tag", QStringLiteral("api"));
        apiInbound.insert("listen", QStringLiteral("127.0.0.1"));
        apiInbound.insert("port", 10085);
        apiInbound.insert("protocol", QStringLiteral("dokodemo-door"));
        apiInbound.insert("settings", QJsonObject{{QStringLiteral("address"), QStringLiteral("127.0.0.1")}});
        inbounds.append(apiInbound);
    }

    if (options.enableTunMode) {
        QJsonObject tunInbound;
        tunInbound.insert("tag", QStringLiteral("tun-in"));
        tunInbound.insert("port", 0);
        tunInbound.insert("protocol", QStringLiteral("tun"));
        tunInbound.insert("settings", QJsonObject{{QStringLiteral("name"), QStringLiteral("xray0")}, {QStringLiteral("MTU"), 1500}});
        inbounds.append(tunInbound);
    }

    QJsonArray outbounds{
        buildOutbound(profile),
        QJsonObject{{QStringLiteral("tag"), QStringLiteral("direct")}, {QStringLiteral("protocol"), QStringLiteral("freedom")}},
        QJsonObject{{QStringLiteral("tag"), QStringLiteral("block")}, {QStringLiteral("protocol"), QStringLiteral("blackhole")}}
    };

    if (options.enableStatsApi) {
        outbounds.append(QJsonObject{{QStringLiteral("tag"), QStringLiteral("api")}, {QStringLiteral("protocol"), QStringLiteral("freedom")}});
    }

    QJsonObject root;
    root.insert("log", QJsonObject{{QStringLiteral("loglevel"), QStringLiteral("warning")}});
    root.insert("inbounds", inbounds);
    root.insert("outbounds", outbounds);

    if (options.enableStatsApi) {
        root.insert("stats", QJsonObject{});
        root.insert("api", QJsonObject{{QStringLiteral("tag"), QStringLiteral("api")}, {QStringLiteral("services"), QJsonArray{QStringLiteral("StatsService")}}});
        root.insert("policy", QJsonObject{{QStringLiteral("system"), QJsonObject{{QStringLiteral("statsOutboundUplink"), true}, {QStringLiteral("statsOutboundDownlink"), true}, {QStringLiteral("statsInboundUplink"), true}, {QStringLiteral("statsInboundDownlink"), true}}}});

        const QJsonObject apiRule{
            {QStringLiteral("type"), QStringLiteral("field")},
            {QStringLiteral("inboundTag"), QJsonArray{QStringLiteral("api")}},
            {QStringLiteral("outboundTag"), QStringLiteral("api")}
        };
        root.insert("routing", QJsonObject{{QStringLiteral("rules"), QJsonArray{apiRule}}});
    }

    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}
