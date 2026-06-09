#pragma once

#include "Profile.h"
#include "RunOptions.h"
#include <QByteArray>

class XrayConfigBuilder {
public:
    static QByteArray buildJson(const Profile &profile, const ClientRunOptions &options);
};
