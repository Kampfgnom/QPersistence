COMMON_CONFIGFILE = ../../../common.pri
exists($$COMMON_CONFIGFILE) {
    include($$COMMON_CONFIGFILE)
}

QPERSISTENCE_PATH = ..
include($$QPERSISTENCE_PATH/QPersistence.pri)

### General config ###

TARGET          = $$QPERSISTENCE_TARGET
VERSION         = $$QPERSISTENCE_VERSION
TEMPLATE        = lib
QT              += sql
CONFIG          += static c++11
macx:QMAKE_CXXFLAGS  += $$QPERSISTENCE_COMMON_QMAKE_CXXFLAGS
INCLUDEPATH     += $$QPERSISTENCE_INCLUDEPATH
INCLUDEPATH += ../include

macx {
    QMAKE_MAC_SDK = macosx10.9
}


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
    sortfilterproxyobjectmodel.h \
    sqldataaccessobjecthelper.h \
    sqlquery.h \
    sqlbackend.h \
    lock.h \
    defines.h \
    usermanagement.h \
    throttledfetchproxymodel.h \
    storage.h \
    defaultstorage.h \
    schemaversioning.h \
    lazyPixmap.h \
    model.h \
    transactionshelper.h \
    propertydependencieshelper.h \
    datasource.h \
    condition.h \
    datasourceresult.h \
    legacysqldatasource.h \
    relations.h

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
    sortfilterproxyobjectmodel.cpp \
    sqldataaccessobjecthelper.cpp \
    sqlquery.cpp \
    sqlbackend.cpp \
    lock.cpp \
    usermanagement.cpp \
    throttledfetchproxymodel.cpp \
    storage.cpp \
    defaultstorage.cpp \
    schemaversioning.cpp \
    lazyPixmap.cpp \
    model.cpp \
    transactionshelper.cpp \
    propertydependencieshelper.cpp \
    datasource.cpp \
    condition.cpp \
    datasourceresult.cpp \
    legacysqldatasource.cpp \
    relations.cpp

OTHER_FILES += \
    uncrustify.cfg
