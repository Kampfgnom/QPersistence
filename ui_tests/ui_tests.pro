QPERSISTENCE_PATH = ../
include($$QPERSISTENCE_PATH/QPersistence.pri)


TARGET          = qpersistence_ui_tests
VERSION         = 0.0.0
TEMPLATE        = app
QT              += sql testlib widgets gui concurrent
CONFIG          += c++11 console
CONFIG          -= app_bundle
QMAKE_CXXFLAGS  += $$QPERSISTENCE_COMMON_QMAKE_CXXFLAGS
DEFINES         += SRCDIR=\\\"$$PWD/\\\"


### Qp ###

INCLUDEPATH     += $$QPERSISTENCE_INCLUDEPATH
LIBS            += $$QPERSISTENCE_LIBS
PRE_TARGETDEPS  += ../src/libqpersistence.a

INCLUDEPATH     += $$TESTMODEL_INCLUDEPATH


SOURCES += main.cpp\
        mainwindow.cpp \
    object.cpp

HEADERS  += mainwindow.h \
    object.h

FORMS    += mainwindow.ui
