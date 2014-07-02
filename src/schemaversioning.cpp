#include "schemaversioning.h"

#include "error.h"
#include "sqlbackend.h"
#include "sqlquery.h"
#include "storage.h"

BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QMap>
#include <QSqlError>
END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

const QpSchemaVersioning::Version QpSchemaVersioning::NullVersion = {0,0,0};

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

bool operator ==(const QpSchemaVersioning::Version &v1, const QpSchemaVersioning::Version &v2)
{
    return v1.major == v2.major
            && v1.minor == v2.minor
            && v1.dot == v2.dot;
}

bool operator >(const QpSchemaVersioning::Version &v1, const QpSchemaVersioning::Version &v2)
{
    return !operator <(v1, v2) && !operator ==(v1, v2);
}

QDebug operator<<(QDebug dbg, const QpSchemaVersioning::Version &version)
{
    dbg.nospace() << version.major << "." << version.minor << "." << version.dot;
    return dbg.space();
}

class QpSchemaVersioningData : public QSharedData
{
public:
    QpSchemaVersioningData() :
        QSharedData()
    {}

    QpStorage *storage;
    QMap<QpSchemaVersioning::Version, QString> upgradeScripts;

    bool applyVersion(const QpSchemaVersioning::Version &version, const QString &script);
};

bool QpSchemaVersioningData::applyVersion(const QpSchemaVersioning::Version &version, const QString &script)
{
    QpSqlQuery query(storage->database());
    if (!query.exec(script) || query.lastError().isValid()) {
        storage->setLastError(QpError(query.lastError()));
        return false;
    }

    query.clear();
    query.setTable(QpDatabaseSchema::TABLENAME_SCHEMAVERSIONING);
    query.addField(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_MAJOR, version.major);
    query.addField(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_MINOR, version.minor);
    query.addField(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_DOT, version.dot);
    query.addField(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_SCRIPT, script);
    query.addRawField(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_DATEAPPLIED, QpSqlBackend::forDatabase(storage->database())->nowTimestamp());
    query.prepareInsert();

    if (!query.exec() || query.lastError().isValid()) {
        storage->setLastError(QpError(query.lastError()));
        return false;
    }

    return true;
}

QpSchemaVersioning::QpSchemaVersioning(QObject *parent) :
    QpSchemaVersioning(QpStorage::defaultStorage(), parent)
{
}

QpSchemaVersioning::QpSchemaVersioning(QpStorage *storage, QObject *parent) :
    QObject(parent),
    data(new QpSchemaVersioningData)
{
    data->storage = storage;
}

QpSchemaVersioning::~QpSchemaVersioning()
{
}

QpSchemaVersioning::Version QpSchemaVersioning::currentDatabaseVersion() const
{
    QpSqlQuery query(data->storage->database());
    query.setTable(QpDatabaseSchema::TABLENAME_SCHEMAVERSIONING);
    query.addOrder(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_MAJOR, QpSqlQuery::Descending);
    query.addOrder(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_MINOR, QpSqlQuery::Descending);
    query.addOrder(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_DOT, QpSqlQuery::Descending);
    query.addField(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_MAJOR);
    query.addField(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_MINOR);
    query.addField(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_DOT);
    query.prepareSelect();

    if (!query.exec() || !query.first() ||  query.lastError().isValid()) {
        data->storage->setLastError(QpError(query.lastError()));
        return QpSchemaVersioning::NullVersion;
    }

    Version version;
    version.major = query.value(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_MAJOR).toInt();
    version.minor = query.value(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_MINOR).toInt();
    version.dot = query.value(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_DOT).toInt();

    return version;
}

QpSchemaVersioning::Version QpSchemaVersioning::latestVersion() const
{
    if(data->upgradeScripts.isEmpty())
        return QpSchemaVersioning::NullVersion;

    return data->upgradeScripts.lastKey();
}

void QpSchemaVersioning::registerUpgradeScript(const Version &version, const QString &script)
{
    data->upgradeScripts.insert(version, script);
}

bool QpSchemaVersioning::upgradeSchema()
{
    Version current = currentDatabaseVersion();
    if(data->upgradeScripts.isEmpty()) {
        qDebug() << "No schema upgrades available. Current version: " << current;
        return false;
    }

    Version latest = latestVersion();
    if(current == latest) {
        qDebug() << "Already on latest schema version: " << current;
        return false;
    }

    qDebug() << "Schema version outdated: " << current;
    qDebug() << "Upgrading to latest version: " << latest;

    QMapIterator<Version, QString> it(data->upgradeScripts);
    while(it.hasNext()) {
        it.next();

        if(it.key() < current) {
            qDebug() << "Version" << it.key() << "already installed.";
            continue;
        }

        qDebug() << "Applying version " << it.key();

        if(!data->applyVersion(it.key(), it.value())
           || data->storage->lastError().isValid()) {
            qDebug() << "Schema upgrade failed" << data->storage->lastError();
            return false;
        }
    }

    Version upgraded = currentDatabaseVersion();
    qDebug() << "Successfully upgraded to schema version: " << upgraded;
    return true;
}

QpStorage *QpSchemaVersioning::storage() const
{
    return data->storage;
}
