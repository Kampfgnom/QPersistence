QPERSISTENCE_PATH = ../
include($$QPERSISTENCE_PATH/QPersistence.pri)
include($$QPERSISTENCE_PATH/examples/testModel/testModel.pri)

### General config ###

TARGET          = qpersistencetests
VERSION         = 0.0.0
TEMPLATE        = app
QT              += sql testlib
CONFIG          += c++11 console
CONFIG          -= app_bundle
QMAKE_CXXFLAGS  += $$QPERSISTENCE_COMMON_QMAKE_CXXFLAGS
DEFINES         += SRCDIR=\\\"$$PWD/\\\"


### Qp ###

INCLUDEPATH     += $$QPERSISTENCE_INCLUDEPATH
LIBS            += $$QPERSISTENCE_LIBS

INCLUDEPATH     += $$TESTMODEL_INCLUDEPATH

SOURCES +=  \
    main.cpp \
    tst_cachetest.cpp \
    tst_metaobjecttest.cpp \
    tst_creationandupdatetimestest.cpp \
    tst_relationsindatabasetest.cpp

HEADERS += \
    tst_cachetest.h \
    tst_metaobjecttest.h \
    tst_creationandupdatetimestest.h \
    tst_relationsindatabasetest.h
