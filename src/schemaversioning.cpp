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

    void applyVersion(const QpSchemaVersioning::Version &version, const QString &script);
};

void QpSchemaVersioningData::applyVersion(const QpSchemaVersioning::Version &version, const QString &script)
{
    QpSqlQuery query(storage->database());
    if (!query.exec(script) || query.lastError().isValid()) {
        storage->setLastError(QpError(query.lastError()));
        return;
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
        return;
    }
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

QpSchemaVersioning::Version QpSchemaVersioning::currentVersion() const
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

void QpSchemaVersioning::registerUpgradeScript(const Version &version, const QString &script)
{
    data->upgradeScripts.insert(version, script);
}

void QpSchemaVersioning::upgradeSchema()
{
    Version current = currentVersion();
    if(data->upgradeScripts.isEmpty()) {
        qDebug() << "No schema upgrades available. Current version: " << current;
        return;
    }

    Version latest = data->upgradeScripts.lastKey();
    if(current == latest) {
        qDebug() << "Already on latest schema version: " << current;
        return;
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
        data->applyVersion(it.key(), it.value());

        if(data->storage->lastError().isValid()) {
            qDebug() << "Schema upgrade failed" << data->storage->lastError();
            return;
        }
    }

    Version upgraded = currentVersion();
    qDebug() << "Successfully upgraded to schema version: " << upgraded;
}
