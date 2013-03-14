QDATASUITE_PATH = ../../QDataSuite
include($$QDATASUITE_PATH/QDataSuite.pri)

QPERSISTENCE_PATH = ../../QPersistence
include($$QPERSISTENCE_PATH/QPersistence.pri)

include(../seriesModel/seriesModel.pri)


### General config ###

TARGET          = datasuite_example
VERSION         = 0.0.0
TEMPLATE        = app
QT              += sql widgets
CONFIG          += c++11
QMAKE_CXXFLAGS  += $$QDATASUITE_COMMON_QMAKE_CXXFLAGS


### QDataSuite ###

INCLUDEPATH     += $$QDATASUITE_INCLUDEPATH
LIBS            += $$QDATASUITE_LIBS


### QPersistence ###

INCLUDEPATH     += $$QPERSISTENCE_INCLUDEPATH
LIBS            += $$QPERSISTENCE_LIBS


### seriesModel ###

INCLUDEPATH     += $$SERIESMODEL_INCLUDEPATH


### Files ###

HEADERS +=

SOURCES += \
    main.cpp
