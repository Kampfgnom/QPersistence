QPERSISTENCE_PATH = ..
include($$QPERSISTENCE_PATH/QPersistence.pri)

### General config ###

TARGET          = $$QPERSISTENCE_TARGET
VERSION         = $$QPERSISTENCE_VERSION
TEMPLATE        = lib
QT              += sql
QT              -= gui
CONFIG          += static c++11
QMAKE_CXXFLAGS  += $$QPERSISTENCE_COMMON_QMAKE_CXXFLAGS
INCLUDEPATH     += $$QPERSISTENCE_INCLUDEPATH


### Files ###

HEADERS += \
    metaproperty.h \
    error.h \
    metaobject.h \
    abstractdataaccessobject.h \
    simpledataaccessobject.h \
    cacheddataaccessobject.h \
    sqlquery.h \
    sqldataaccessobjecthelper.h \
    sqlcondition.h \
    persistentdataaccessobject.h \
    databaseschema.h \
    qpersistence.h
SOURCES += \
    metaproperty.cpp \
    error.cpp \
    metaobject.cpp \
    abstractdataaccessobject.cpp \
    simpledataaccessobject.cpp \
    cacheddataaccessobject.cpp \
    sqlquery.cpp \
    sqldataaccessobjecthelper.cpp \
    sqlcondition.cpp \
    persistentdataaccessobject.cpp \
    databaseschema.cpp \
    qpersistence.cpp
