#ifndef SCHEMAVERSIONING_H
#define SCHEMAVERSIONING_H

#include "defines.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QObject>
#include <QtCore/QSharedData>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpStorage;

class QpSchemaVersioningData;
class QpSchemaVersioning : public QObject
{
    Q_OBJECT
public:
    struct Version {
        int major;
        int minor;
        int dot;
    };
    static const Version NullVersion;

    explicit QpSchemaVersioning(QpStorage *storage, QObject *parent = 0);
    ~QpSchemaVersioning();

    Version currentVersion() const;
    void registerUpgradeScript(const Version &currentVersion, const QString &script);
    void upgradeSchema();

private:
    QExplicitlySharedDataPointer<QpSchemaVersioningData> data;
};

uint qHash(const QpSchemaVersioning::Version &version, uint seed = 0);
bool operator <(const QpSchemaVersioning::Version &v1, const QpSchemaVersioning::Version &v2);
bool operator ==(const QpSchemaVersioning::Version &v1, const QpSchemaVersioning::Version &v2);
QDebug operator<<(QDebug dbg, const QpSchemaVersioning::Version &version);

#endif // SCHEMAVERSIONING_H
