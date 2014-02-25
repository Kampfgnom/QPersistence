TEMPLATE = subdirs

CONFIG += ordered
SUBDIRS = \
    src \
    tests \
    testDatabaseChanger \
    ui_tests

OTHER_FILES += \
    README.md \
    LICENSE \
    include/QPersistence.h
