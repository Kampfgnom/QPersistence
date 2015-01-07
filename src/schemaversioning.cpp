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


/******************************************************************************
 * QpSchemaVersioningData
 */
class QpSchemaVersioningData : public QSharedData
{
public:
    QpSchemaVersioningData() :
        QSharedData(),
        requiredVersion(QpSchemaVersioning::NullVersion)
    {
    }

    QpStorage *storage;
    QMap<QpSchemaVersioning::Version, std::function<void()> > upgradeFunctions;
    QMap<QpSchemaVersioning::Version, QString> descriptions;
    QpSchemaVersioning::Version requiredVersion;

    void applyScript(const QString &script);
    void insertVersion(const QpSchemaVersioning::Version &version);
};

void QpSchemaVersioningData::applyScript(const QString &script)
{
    QpSqlQuery query(storage->database());
    if (!query.exec(script) || query.lastError().isValid()) {
        storage->setLastError(QpError(query.lastError()));
        return;
    }
}

void QpSchemaVersioningData::insertVersion(const QpSchemaVersioning::Version &version)
{
    QpSqlQuery query(storage->database());
    query.setTable(QpDatabaseSchema::TABLENAME_SCHEMAVERSIONING);
    query.addField(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_MAJOR, version.major);
    query.addField(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_MINOR, version.minor);
    query.addField(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_DOT, version.dot);
    query.addField(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_SCRIPT, descriptions.value(version));
    query.addRawField(QpDatabaseSchema::COLUMN_NAME_SCHEMAVERSION_DATEAPPLIED, QpSqlBackend::forDatabase(storage->database())->nowTimestamp());
    query.prepareInsert();

    if (!query.exec() || query.lastError().isValid()) {
        storage->setLastError(QpError(query.lastError()));
        return;
    }
}

QpSchemaVersioning::Version QpSchemaVersioning::parseVersionString(const QString &versionString)
{
    QStringList parts = versionString.split('.');
    if (parts.size() != 3) {
        qWarning() << "Invalid version string" << versionString;
        return NullVersion;
    }

    bool valid = true;
    bool validation = false;
    Version version;
    version.major = parts.at(0).toInt(&validation);
    valid &= validation;
    version.minor = parts.at(1).toInt(&validation);
    valid &= validation;
    version.dot = parts.at(2).toInt(&validation);
    valid &= validation;
    if (!valid) {
        qWarning() << "Invalid version string" << versionString;
        return NullVersion;
    }
    return version;
}


/******************************************************************************
 * QpSchemaVersioning
 */
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

void QpSchemaVersioning::setInitialVersion(const QpSchemaVersioning::Version &version, const QString description)
{
    data->upgradeFunctions.insert(version, [] {});
    data->descriptions.insert(version, description);
}

QpSchemaVersioning::Version QpSchemaVersioning::latestVersion() const
{
    if (data->upgradeFunctions.isEmpty())
        return QpSchemaVersioning::NullVersion;

    return data->upgradeFunctions.lastKey();
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

void QpSchemaVersioning::registerUpgradeScript(const Version &version, const QString &script)
{
    registerUpgradeFunction(version, script, [this, script] {
        foreach (QString query, script.split(';')) {
            if (QString(query).remove(QRegularExpression("[\\s]")).isEmpty())
                continue;
            data->applyScript(query);
        }
    });
}

void QpSchemaVersioning::registerUpgradeFunction(const QpSchemaVersioning::Version &version, const QString &description, std::function<void()> function)
{
    data->upgradeFunctions.insert(version, function);
    data->descriptions.insert(version, description);
}

bool QpSchemaVersioning::upgradeSchema()
{
    if (!data->storage->beginTransaction()) {
        qDebug() << "Transaction failed";
        return false;
    }

    Version current = currentDatabaseVersion();
    if (data->upgradeFunctions.isEmpty()) {
        qDebug() << "No schema upgrades available. Current version: " << current;
        return true;
    }

    Version latest = latestVersion();
    if (current == latest) {
        qDebug() << "Already on latest schema version: " << current;
        return true;
    }

    qDebug() << "Schema version outdated: " << current;
    qDebug() << "Upgrading to latest version: " << latest;

    bool success = true;
    QMapIterator<Version, std::function<void()> > it(data->upgradeFunctions);
    while (it.hasNext()) {
        it.next();

        if (it.key() <= current) {
            qDebug() << "Version" << it.key() << "already installed.";
            continue;
        }

        qDebug() << "Applying version " << it.key();
        it.value() ();
        data->insertVersion(it.key());

        if (data->storage->lastError().isValid()) {
            qDebug() << "Schema upgrade failed" << data->storage->lastError();
            success = false;
            break;
        }
    }

    if (!data->storage->commitOrRollbackTransaction()) {
        qDebug() << "Commit failed";
        return false;
    }

    if (!success)
        return false;

    Version upgraded = currentDatabaseVersion();
    qDebug() << "Successfully upgraded to schema version: " << upgraded;
    return true;
}

QpStorage *QpSchemaVersioning::storage() const
{
    return data->storage;
}

QpSchemaVersioning::Version QpSchemaVersioning::requiredVersion() const
{
    return data->requiredVersion;
}

void QpSchemaVersioning::setRequiredVersion(const QpSchemaVersioning::Version &value)
{
    data->requiredVersion = value;
}
