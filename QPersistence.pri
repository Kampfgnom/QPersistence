isEmpty(QPERSISTENCE_PATH) {
    error(You have to set QPERSISTENCE_PATH to the path of QPersistence relative to your project)
}

QPERSISTENCE_TARGET          = qpersistence
QPERSISTENCE_VERSION         = 0.0.0
QPERSISTENCE_INCLUDEPATH     = $$PWD/include

unix: {
    QPERSISTENCE_LIBPATH = $$QPERSISTENCE_PATH/src
    QPERSISTENCE_LIB = lib$${QPERSISTENCE_TARGET}.a
}
win32 {
    CONFIG(debug, release | debug) {
        QPERSISTENCE_LIBPATH = $$QPERSISTENCE_PATH/src/debug
    }
    CONFIG(release, release | debug) {
        QPERSISTENCE_LIBPATH = $$QPERSISTENCE_PATH/src/release
    }
    win32-msvc*: {
        QPERSISTENCE_LIB = $${QPERSISTENCE_TARGET}.lib
    }
    win32-g++ {
        QPERSISTENCE_LIB = lib$${QPERSISTENCE_TARGET}.a
    }
}

QPERSISTENCE_LIBS            = -L$$QPERSISTENCE_LIBPATH -l$$QPERSISTENCE_TARGET
QPERSISTENCE_POST_TARGETDEPS = $$OUT_PWD/$$QPERSISTENCE_LIBPATH/$$QPERSISTENCE_LIB
macx:QPERSISTENCE_COMMON_QMAKE_CXXFLAGS = -Weverything \
                                          -Wno-c++98-compat \
                                          -Wno-c++98-compat-pedantic \
                                          -Wno-padded  \
                                          -Wno-undefined-reinterpret-cast  \
                                          -Wno-pragmas  \
                                          -Wno-unknown-warning-option \
                                          -Wno-unkown-pragmas

qpoptions = $$find(CONFIG, "qpmysql") $$find(CONFIG, "qpsqlite")
count(qpoptions, 0) {
    CONFIG += qpmysql
}
contains(CONFIG, qpmysql) {
    DEFINES += QP_FOR_MYSQL
}
contains(CONFIG, qpsqlite) {
    DEFINES += QP_FOR_SQLITE
}
!contains(QT, gui) {
    DEFINES += QP_NO_GUI
}
