QPERSISTENCE_PATH = ../../
include($$QPERSISTENCE_PATH/QPersistence.pri)

include(../seriesModel/seriesModel.pri)


### General config ###

TARGET          = persistence_example
VERSION         = 0.0.0
TEMPLATE        = app
QT              += sql widgets
CONFIG          += c++11
QMAKE_CXXFLAGS  += $$QPERSISTENCE_COMMON_QMAKE_CXXFLAGS


### QPERSISTENCE ###

INCLUDEPATH     += $$QPERSISTENCE_INCLUDEPATH
LIBS            += $$QPERSISTENCE_LIBS


### QPersistence ###

INCLUDEPATH     += $$QPERSISTENCE_INCLUDEPATH
LIBS            += $$QPERSISTENCE_LIBS


### seriesModel ###

INCLUDEPATH     += $$SERIESMODEL_INCLUDEPATH


### Files ###

HEADERS +=

SOURCES += main.cpp
