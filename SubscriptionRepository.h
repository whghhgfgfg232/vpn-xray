#pragma once

#include "Subscription.h"
#include <QList>
#include <QString>

class SubscriptionRepository {
public:
    SubscriptionRepository();

    QList<Subscription> load(QString *error = nullptr) const;
    bool save(const QList<Subscription> &subscriptions, QString *error = nullptr) const;
    QString storagePath() const;

private:
    QString path_;
};
