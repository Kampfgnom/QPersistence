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
PRE_TARGETDEPS  += ../src/libqpersistence.a

INCLUDEPATH     += $$TESTMODEL_INCLUDEPATH

SOURCES +=  \
    main.cpp \
    tst_cachetest.cpp \
    tst_creationandupdatetimestest.cpp \
    relationtestbase.cpp \
    tst_onetoonerelationtest.cpp \
    tst_onetomanyrelationtest.cpp \
    tst_manytomanyrelationstest.cpp \
    tst_synchronizetest.cpp \
    tst_locktest.cpp \
    tst_enumerationtest.cpp \
    tst_flagstest.cpp

HEADERS += \
    tst_cachetest.h \
    tst_creationandupdatetimestest.h \
    relationtestbase.h \
    tst_onetoonerelationtest.h \
    tst_onetomanyrelationtest.h \
    tst_manytomanyrelationstest.h \
    tst_synchronizetest.h \
    tst_locktest.h \
    tst_enumerationtest.h \
    tst_flagstest.h
