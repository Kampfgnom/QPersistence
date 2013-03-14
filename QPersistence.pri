isEmpty(QPERSISTENCE_PATH) {
    error(You have to set QPERSISTENCE_PATH to the path of QDataSuite relative to your project)
}

QPERSISTENCE_TARGET          = qpersistence
QPERSISTENCE_VERSION         = 0.0.0
QPERSISTENCE_INCLUDEPATH     = $$PWD/include
QPERSISTENCE_LIBS            = -L$$QPERSISTENCE_PATH/src -l$$QPERSISTENCE_TARGET
