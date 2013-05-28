QPERSISTENCE_PATH = ../../
include($$QPERSISTENCE_PATH/QPersistence.pri)

### General config ###

TARGET          = projectstats
VERSION         = 0.0.0
TEMPLATE        = app
QT              += sql widgets
CONFIG          += c++11
QMAKE_CXXFLAGS  += $$QPERSISTENCE_COMMON_QMAKE_CXXFLAGS

### Qp ###

INCLUDEPATH     += $$QPERSISTENCE_INCLUDEPATH
LIBS            += $$QPERSISTENCE_LIBS

### Files ###

SOURCES += main.cpp\
        mainwindow.cpp \
    model/player.cpp \
    model/place.cpp \
    model/drink.cpp \
    model/game.cpp \
    model/livedrink.cpp \
    model/round.cpp \
    model/point.cpp \
    model/schmeisserei.cpp

HEADERS  += mainwindow.h \
    model/player.h \
    model/place.h \
    model/drink.h \
    model/game.h \
    model/livedrink.h \
    model/round.h \
    model/point.h \
    model/schmeisserei.h

FORMS    += mainwindow.ui
