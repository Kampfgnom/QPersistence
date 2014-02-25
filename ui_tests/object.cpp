#include "object.h"

Object::Object(QObject *parent) :
    QObject(parent),
    m_date(0)
{
}

ulong Object::date() const
{
    return m_date;
}

void Object::setDate(const ulong &arg)
{
    m_date = arg;
}

QString Object::string() const
{
    return m_string;
}

void Object::setString(const QString &arg)
{
    m_string = arg;
}
