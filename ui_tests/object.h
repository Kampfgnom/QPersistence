#ifndef OBJECT_H
#define OBJECT_H

#include <QObject>
#include <QDateTime>

class Object : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ulong date READ date  WRITE setDate)
    Q_PROPERTY(QString string READ string  WRITE setString)

public:
    explicit Object(QObject *parent = 0);

    ulong date() const;
    void setDate(const ulong &arg);

    QString string() const;
    void setString(const QString &arg);

private:
    ulong m_date;
    QString m_string;
};

#endif // OBJECT_H
