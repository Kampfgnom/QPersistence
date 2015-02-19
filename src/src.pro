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

unix {
    QMAKE_CXXFLAGS  += \
    -Qunused-arguments \
    -Wall \
    -Wno-unreachable-code-loop-increment \
    -Wno-c++98-compat \
    -Wno-padded  \
    -Wno-undefined-reinterpret-cast  \
    -Wno-pragmas  \
    -Wno-unknown-warning-option \
    -Wno-unkown-pragmas
}

### Files ###

HEADERS += \
    cache.h \
    condition.h \
    conversion.h \
    dataaccessobject.h \
    databaseschema.h \
    datasource.h \
    datasourceresult.h \
    defaultstorage.h \
    defines.h \
    error.h \
    legacysqldatasource.h \
    lock.h \
    metaobject.h \
    metaproperty.h \
    model.h \
    objectlistmodel.h \
    private.h \
    propertydependencieshelper.h \
    qpersistence.h \
    relations.h \
    reply.h \
    schemaversioning.h \
    sortfilterproxyobjectmodel.h \
    sqlbackend.h \
    sqldataaccessobjecthelper.h \
    sqlquery.h \
    storage.h \
    throttledfetchproxymodel.h \
    transactionshelper.h \
    usermanagement.h

SOURCES += \
    cache.cpp \
    condition.cpp \
    conversion.cpp \
    dataaccessobject.cpp \
    databaseschema.cpp \
    datasource.cpp \
    datasourceresult.cpp \
    defaultstorage.cpp \
    error.cpp \
    legacysqldatasource.cpp \
    lock.cpp \
    metaobject.cpp \
    metaproperty.cpp \
    model.cpp \
    objectlistmodel.cpp \
    private.cpp \
    propertydependencieshelper.cpp \
    relations.cpp \
    reply.cpp \
    schemaversioning.cpp \
    sortfilterproxyobjectmodel.cpp \
    sqlbackend.cpp \
    sqldataaccessobjecthelper.cpp \
    sqlquery.cpp \
    storage.cpp \
    throttledfetchproxymodel.cpp \
    transactionshelper.cpp \
    usermanagement.cpp


OTHER_FILES += \
    uncrustify.cfg
