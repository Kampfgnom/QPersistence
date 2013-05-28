QPERSISTENCE_PATH = ..
include($$QPERSISTENCE_PATH/QPersistence.pri)

### General config ###

TARGET          = $$QPERSISTENCE_TARGET
VERSION         = $$QPERSISTENCE_VERSION
TEMPLATE        = lib
QT              += sql
CONFIG          += static c++11
QMAKE_CXXFLAGS  += $$QPERSISTENCE_COMMON_QMAKE_CXXFLAGS
INCLUDEPATH     += $$QPERSISTENCE_INCLUDEPATH

### Files ###

HEADERS += \
    metaproperty.h \
    error.h \
    metaobject.h \
    sqlquery.h \
    sqldataaccessobjecthelper.h \
    sqlcondition.h \
    databaseschema.h \
    qpersistence.h \
    conversion.h \
    private.h \
    dataaccessobject.h \
    cache.h \
    relationresolver.h \
    relations.h \
    abstractobjectlistmodel.h \
    objectlistmodel.h
SOURCES += \
    metaproperty.cpp \
    error.cpp \
    metaobject.cpp \
    sqlquery.cpp \
    sqldataaccessobjecthelper.cpp \
    sqlcondition.cpp \
    databaseschema.cpp \
    qpersistence.cpp \
    conversion.cpp \
    private.cpp \
    qpersistence_impl.cpp \
    dataaccessobject.cpp \
    cache.cpp \
    relationresolver.cpp \
    relations.cpp \
    abstractobjectlistmodel.cpp \
    objectlistmodel.cpp
