#pragma once

#include "core/AppConfig.h"

#include <QString>

class ConfigStore
{
public:
    explicit ConfigStore(QString configPath);

    AppConfig load();
    bool save(const AppConfig &config, QString *errorMessage = nullptr) const;

private:
    AppConfig defaults() const;
    AppConfig normalize(const AppConfig &config) const;

    QString m_configPath;
};
