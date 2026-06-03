#pragma once

#include <QString>

struct FileManifest
{
    QString id;
    QString name;
    qint64 size = 0;
    QString sha256;
    QString relativePath;
};
