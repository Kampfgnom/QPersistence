QDATASUITE_PATH = ../../QDataSuite
include($$QDATASUITE_PATH/QDataSuite.pri)

QRESTSERVER_PATH = ../../QRestServer
include($$QRESTSERVER_PATH/QRestServer.pri)

QHAL_PATH = ../../QRestServer/lib/QHal
include($$QHAL_PATH/QHal.pri)

include(../seriesModel/seriesModel.pri)


### General config ###

TARGET          = restserver_example
VERSION         = 0.0.0
TEMPLATE        = app
QT              += sql widgets network
CONFIG          += static c++11
QMAKE_CXXFLAGS  += $$QDATASUITE_COMMON_QMAKE_CXXFLAGS


### QDataSuite ###

INCLUDEPATH     += $$QDATASUITE_INCLUDEPATH
LIBS            += $$QDATASUITE_LIBS


### QRestServer ###

INCLUDEPATH     += $$QRESTSERVER_INCLUDEPATH
LIBS            += $$QRESTSERVER_LIBS


### seriesModel ###

INCLUDEPATH     += $$SERIESMODEL_INCLUDEPATH


### QHttpServer ###

INCLUDEPATH     += $$QHTTPSERVER_INCLUDEPATH
LIBS            += $$QHTTPSERVER_LIBS


### QHAL ###

LIBS            += $$QHAL_LIBS
INCLUDEPATH     += $$QHAL_INCLUDEPATH


### Files ###

HEADERS +=

SOURCES += \
    main.cpp
