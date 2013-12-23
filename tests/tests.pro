QPERSISTENCE_PATH = ../
include($$QPERSISTENCE_PATH/QPersistence.pri)


### General config ###

TARGET          = qpersistencetests
VERSION         = 0.0.0
TEMPLATE        = app
QT              += sql testlib
QT              -= gui
CONFIG          += c++11 console
CONFIG          -= app_bundle
QMAKE_CXXFLAGS  += $$QPERSISTENCE_COMMON_QMAKE_CXXFLAGS
DEFINES         += SRCDIR=\\\"$$PWD/\\\"


### Qp ###

INCLUDEPATH     += $$QPERSISTENCE_INCLUDEPATH
LIBS            += $$QPERSISTENCE_LIBS

SOURCES +=  \
    main.cpp \
    tst_cachetest.cpp \
    tst_metaobjecttest.cpp

HEADERS += \
    tst_cachetest.h \
    tst_metaobjecttest.h
