QPERSISTENCE_PATH = ..
include($$QPERSISTENCE_PATH/QPersistence.pri)

### General config ###

TARGET          = $$QPERSISTENCE_TARGET
VERSION         = $$QPERSISTENCE_VERSION
TEMPLATE        = lib
QT              += sql
CONFIG          += static c++11
unix:QMAKE_CXXFLAGS  += $$QPERSISTENCE_COMMON_QMAKE_CXXFLAGS
INCLUDEPATH     += $$QPERSISTENCE_INCLUDEPATH


### Files ###

HEADERS += \
    cache.h \
    conversion.h \
    dataaccessobject.h \
    databaseschema.h \
    error.h \
    metaobject.h \
    metaproperty.h \
    objectlistmodel.h \
    private.h \
    qpersistence.h \
    relationresolver.h \
    sortfilterproxyobjectmodel.h \
    sqlcondition.h \
    sqldataaccessobjecthelper.h \
    sqlquery.h \
    sqlbackend.h \
    lock.h \
    relation_hasone.h \
    relation_belongstoone.h \
    relation_hasmany.h \
    relation_belongstomany.h \
    defines.h \
    usermanagement.h \
    throttledfetchproxymodel.h

SOURCES += \
        cache.cpp \
    conversion.cpp \
    dataaccessobject.cpp \
    databaseschema.cpp \
    error.cpp \
    metaobject.cpp \
    metaproperty.cpp \
    objectlistmodel.cpp \
    private.cpp \
    qpersistence.cpp \
    qpersistence_impl.cpp \
    relationresolver.cpp \
    sortfilterproxyobjectmodel.cpp \
    sqlcondition.cpp \
    sqldataaccessobjecthelper.cpp \
    sqlquery.cpp \
    sqlbackend.cpp \
    lock.cpp \
    relation_hasone.cpp \
    relation_belongstoone.cpp \
    relation_hasmany.cpp \
    relation_belongstomany.cpp \
    usermanagement.cpp \
    throttledfetchproxymodel.cpp
