#pragma once

#include "Profile.h"
#include "Subscription.h"
#include <QList>

class SubscriptionFetcher {
public:
    static bool fetch(const Subscription &subscription, QList<Profile> *profiles, QString *error = nullptr);

private:
    static QByteArray decodeBase64Relaxed(QString input);
    static QStringList extractConfigLines(const QByteArray &payload);
};
