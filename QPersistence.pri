isEmpty(QPERSISTENCE_PATH) {
    error(You have to set QPERSISTENCE_PATH to the path of QPersistence relative to your project)
}

QPERSISTENCE_TARGET          = qpersistence
QPERSISTENCE_VERSION         = 0.0.0
QPERSISTENCE_INCLUDEPATH     = $$PWD/include
QPERSISTENCE_LIBS            = -L$$QPERSISTENCE_PATH/src -l$$QPERSISTENCE_TARGET
QPERSISTENCE_POST_TARGETDEPS = $$OUT_PWD/$$QPERSISTENCE_PATH/src/lib$${QPERSISTENCE_TARGET}.a
QPERSISTENCE_COMMON_QMAKE_CXXFLAGS = -Weverything -Wno-c++98-compat -Wno-padded -Wno-undefined-reinterpret-cast

#DEFINES += CLANG_DIAGNOSTIC_BEGIN_IGNORE_WARNINGS=\
#_Pragma("clang diagnostic push")\
#_Pragma("clang diagnostic ignore \\\"-Wno-c++98-compat\\\"")\

#                                        #-Wno-c++98-compat \
#                                        #-Wno-c++98-compat-pedantic \
#                                        #-Wno-disabled-macro-expansion \
#                                        #-Wno-documentation-unknown-command \
#                                        #-Wno-documentation
