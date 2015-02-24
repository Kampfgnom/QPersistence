#ifndef QPERSISTENCE_SCHEMAVERSIONING_H
#define QPERSISTENCE_SCHEMAVERSIONING_H

#include "defines.h"

#include <functional>

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QSharedData>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

class QpStorage;

class QpSchemaVersioningData;
class QpSchemaVersioning
{
public:
    struct Version {
        int major;
        int minor;
        int dot;
    };
    static const Version NullVersion;
    static Version parseVersionString(const QString &version);

    QpSchemaVersioning(QpStorage *storage);
    ~QpSchemaVersioning();

    void setInitialVersion(const QpSchemaVersioning::Version &version, const QString description);
    Version currentDatabaseVersion() const;
    Version latestVersion() const;
    void registerUpgradeScript(const Version &version, const QString &script);
    void registerUpgradeFunction(const QpSchemaVersioning::Version &version, const QString &description, std::function<void()> function);
    bool upgradeSchema();

    QpStorage *storage() const;

    QpSchemaVersioning::Version requiredVersion() const;
    void setRequiredVersion(const QpSchemaVersioning::Version &value);

private:
    QExplicitlySharedDataPointer<QpSchemaVersioningData> data;
};

inline uint qHash(const QpSchemaVersioning::Version &version, uint seed = 0);
inline bool operator <(const QpSchemaVersioning::Version &v1, const QpSchemaVersioning::Version &v2);
inline bool operator >(const QpSchemaVersioning::Version &v1, const QpSchemaVersioning::Version &v2);
inline bool operator ==(const QpSchemaVersioning::Version &v1, const QpSchemaVersioning::Version &v2);
inline bool operator !=(const QpSchemaVersioning::Version &v1, const QpSchemaVersioning::Version &v2);
inline bool operator <=(const QpSchemaVersioning::Version &v1, const QpSchemaVersioning::Version &v2);
inline QDebug operator<<(QDebug dbg, const QpSchemaVersioning::Version &version);



uint qHash(const QpSchemaVersioning::Version &version, uint seed)
{
    return qHash(version.major, seed) ^ qHash(version.minor, seed) ^ qHash(version.dot, seed);
}

bool operator <(const QpSchemaVersioning::Version &v1, const QpSchemaVersioning::Version &v2)
{
    return v1.major < v2.major
           || (v1.major == v2.major && v1.minor < v2.minor)
           || (v1.major == v2.major && v1.minor == v2.minor && v1.dot < v2.dot);
}

bool operator >(const QpSchemaVersioning::Version &v1, const QpSchemaVersioning::Version &v2)
{
    return !operator <(v1, v2) && !operator ==(v1, v2);
}

bool operator <=(const QpSchemaVersioning::Version &v1, const QpSchemaVersioning::Version &v2)
{
    return operator <(v1, v2) || operator ==(v1, v2);
}

bool operator ==(const QpSchemaVersioning::Version &v1, const QpSchemaVersioning::Version &v2)
{
    return v1.major == v2.major
           && v1.minor == v2.minor
           && v1.dot == v2.dot;
}

bool operator !=(const QpSchemaVersioning::Version &v1, const QpSchemaVersioning::Version &v2)
{
    return !operator ==(v1, v2);
}

QDebug operator<<(QDebug dbg, const QpSchemaVersioning::Version &version)
{
    dbg.nospace() << version.major << "." << version.minor << "." << version.dot;
    return dbg.space();
}

#endif // QPERSISTENCE_SCHEMAVERSIONING_H
