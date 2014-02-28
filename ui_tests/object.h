#ifndef OBJECT_H
#define OBJECT_H

#include "../src/defines.h"
BEGIN_CLANG_DIAGNOSTIC_IGNORE_WARNINGS
#include <QObject>
#include <QDateTime>

class Object : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int date READ date  WRITE setDate)
    Q_PROPERTY(QString string READ string  WRITE setString)

public:
    explicit Object(QObject *parent = 0);

    int date() const;
    void setDate(const int &arg);

    QString string() const;
    void setString(const QString &arg);

private:
    int m_date;
    QString m_string;
};

END_CLANG_DIAGNOSTIC_IGNORE_WARNINGS

#endif // OBJECT_H
