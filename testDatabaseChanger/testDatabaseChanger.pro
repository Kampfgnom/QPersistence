QPERSISTENCE_PATH = ../
include($$QPERSISTENCE_PATH/QPersistence.pri)
include($$QPERSISTENCE_PATH/examples/testModel/testModel.pri)

### General config ###

TARGET          = qpersistencetestdatabasechanger
VERSION         = 0.0.0
TEMPLATE        = app
QT              += sql testlib
CONFIG          += c++11
CONFIG          -= app_bundle
QMAKE_CXXFLAGS  += $$QPERSISTENCE_COMMON_QMAKE_CXXFLAGS
DEFINES         += SRCDIR=\\\"$$PWD/\\\" QP_LOCALDB


### Qp ###

INCLUDEPATH     += $$QPERSISTENCE_INCLUDEPATH
LIBS            += $$QPERSISTENCE_LIBS

INCLUDEPATH     += $$TESTMODEL_INCLUDEPATH

SOURCES +=  \
    main.cpp
HEADERS +=
