isEmpty(QPERSISTENCE_PATH) {
    error(You have to set QPERSISTENCE_PATH to the path of QPersistence relative to your project)
}

QPERSISTENCE_TARGET          = qpersistence
QPERSISTENCE_VERSION         = 0.0.0
QPERSISTENCE_INCLUDEPATH     = $$PWD/include

unix:LIBPATH = $$QPERSISTENCE_PATH/src
win32 {
    CONFIG(debug, release | debug) {
        LIBPATH = $$QPERSISTENCE_PATH/src/debug
    }
    CONFIG(release, release | debug) {
        LIBPATH = $$QPERSISTENCE_PATH/src/release
    }
}

QPERSISTENCE_LIBS            = -L$$LIBPATH -l$$QPERSISTENCE_TARGET
QPERSISTENCE_POST_TARGETDEPS = $$OUT_PWD/$$LIBPATH/lib$${QPERSISTENCE_TARGET}.a
unix:QPERSISTENCE_COMMON_QMAKE_CXXFLAGS = -Wall \
                                        -Wno-c++98-compat \
                                        -Wno-padded  \
                                        -Wno-undefined-reinterpret-cast  \
                                        -Wno-pragmas  \
                                        -Wno-unknown-warning-option

#DEFINES += QP_FOR_SQLITE
DEFINES += QP_FOR_MYSQL

