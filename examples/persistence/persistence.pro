QPERSISTENCE_PATH = ../../
include($$QPERSISTENCE_PATH/QPersistence.pri)

include(../testModel/testModel.pri)


### General config ###

TARGET          = persistence_example
VERSION         = 0.0.0
TEMPLATE        = app
QT              += sql widgets
CONFIG          += c++11
QMAKE_CXXFLAGS  += $$QPERSISTENCE_COMMON_QMAKE_CXXFLAGS

### Qp ###

INCLUDEPATH     += $$QPERSISTENCE_INCLUDEPATH
LIBS            += $$QPERSISTENCE_LIBS


### testModel ###

INCLUDEPATH     += $$TESTMODEL_INCLUDEPATH


### Files ###

SOURCES += main.cpp
