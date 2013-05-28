#ifndef SCHMEISSEREI_H
#define SCHMEISSEREI_H

#include <QObject>

#include <QPersistenceRelations.h>

namespace NewDatabase {

class Round;
class Player;

class Schmeisserei : public QObject
{
    Q_OBJECT
    Q_ENUMS(Type)
    Q_PROPERTY(Type type READ type WRITE setType)
    Q_PROPERTY(QString typeString READ typeString WRITE setTypeString)
    Q_PROPERTY(QSharedPointer<Round> round READ round WRITE setRound)
    Q_PROPERTY(QSharedPointer<Player> player READ player WRITE setPlayer)

    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:player",
                "reverserelation=schmeissereien")
    Q_CLASSINFO("QPERSISTENCE_PROPERTYMETADATA:round",
                "reverserelation=schmeissereien")

public:
    enum Type {
        UnkownType,
        MoreThan4Kings,
        NoTrumpfabgabeTaker,
        LessThan3Trumpf,
        MoreThan90Points
    };

    explicit Schmeisserei(QObject *parent = 0);

    Type type() const;
    void setType(const Type &type);

    QString typeString() const;

    QSharedPointer<Round> round() const;
    void setRound(const QSharedPointer<Round> &round);

    QSharedPointer<Player> player() const;
    void setPlayer(const QSharedPointer<Player> &player);

private:
    void setTypeString(const QString &typeString);

    Type m_type;
    QString m_typeString;

    QpWeakRelation<Round> m_round;
    QpWeakRelation<Player> m_player;

    
};

} // namespace NewDatabase

#endif // SCHMEISSEREI_H
