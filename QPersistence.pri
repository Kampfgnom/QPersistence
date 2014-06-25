isEmpty(QPERSISTENCE_PATH) {
    error(You have to set QPERSISTENCE_PATH to the path of QPersistence relative to your project)
}

QPERSISTENCE_TARGET          = qpersistence
QPERSISTENCE_VERSION         = 0.0.0
QPERSISTENCE_INCLUDEPATH     = $$PWD/include

unix:QPERSISTENCE_LIBPATH = $$QPERSISTENCE_PATH/src
win32 {
    CONFIG(debug, release | debug) {
        QPERSISTENCE_LIBPATH = $$QPERSISTENCE_PATH/src/debug
    }
    CONFIG(release, release | debug) {
        QPERSISTENCE_LIBPATH = $$QPERSISTENCE_PATH/src/release
    }
}

QPERSISTENCE_LIBS            = -L$$QPERSISTENCE_LIBPATH -l$$QPERSISTENCE_TARGET
QPERSISTENCE_POST_TARGETDEPS = $$OUT_PWD/$$QPERSISTENCE_LIBPATH/lib$${QPERSISTENCE_TARGET}.a
macx:QPERSISTENCE_COMMON_QMAKE_CXXFLAGS = -Weverything \
                                          -Wno-c++98-compat \
                                          -Wno-c++98-compat-pedantic \
                                          -Wno-padded  \
                                          -Wno-undefined-reinterpret-cast  \
                                          -Wno-pragmas  \
                                          -Wno-unknown-warning-option \
                                          -Wno-unkown-pragmas

DEFINES += QP_FOR_MYSQL

