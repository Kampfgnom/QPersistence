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
DEFINES         += SRCDIR=\\\"$$PWD/\\\"


### Qp ###

INCLUDEPATH     += $$QPERSISTENCE_INCLUDEPATH
LIBS            += $$QPERSISTENCE_LIBS
PRE_TARGETDEPS  += $$QPERSISTENCE_POST_TARGETDEPS

INCLUDEPATH     += $$TESTMODEL_INCLUDEPATH

SOURCES +=  \
    main.cpp \
    tst_cachetest.cpp \
    tst_onetoonerelationtest.cpp \
    tst_onetomanyrelationtest.cpp \
    tst_manytomanyrelationstest.cpp \
    tst_locktest.cpp \
    tst_synchronizetest.cpp \
    tst_enumerationtest.cpp \
    tst_flagstest.cpp \
    tst_usermanagementtest.cpp \
    tests_common.cpp \
    tst_propertydependenciestest.cpp

HEADERS += \
    tst_cachetest.h \
    tst_onetoonerelationtest.h \
    tst_onetomanyrelationtest.h \
    tst_manytomanyrelationstest.h \
    tst_locktest.h \
    tst_enumerationtest.h \
    tst_synchronizetest.h \
    tst_flagstest.h \
    tst_usermanagementtest.h \
    tests_common.h \
    tst_propertydependenciestest.h
