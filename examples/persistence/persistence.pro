QPERSISTENCE_PATH = ../../
include($$QPERSISTENCE_PATH/QPersistence.pri)
include($$QPERSISTENCE_PATH/examples/testModel/testModel.pri)


### General config ###

TARGET          = persistence_example
VERSION         = 0.0.0
TEMPLATE        = app
QT              += sql widgets testlib
CONFIG          += c++11
QMAKE_CXXFLAGS  += $$QPERSISTENCE_COMMON_QMAKE_CXXFLAGS
DEFINES         += QP_LOCALDB

### Qp ###

INCLUDEPATH     += $$QPERSISTENCE_INCLUDEPATH
LIBS            += $$QPERSISTENCE_LIBS


### testModel ###

INCLUDEPATH     += $$TESTMODEL_INCLUDEPATH


### Files ###

SOURCES += main.cpp
