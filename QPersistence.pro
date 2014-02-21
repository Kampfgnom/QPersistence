TEMPLATE = subdirs

CONFIG += ordered
SUBDIRS = \
    src \
    tests \
    testDatabaseChanger

OTHER_FILES += \
    README.md \
    LICENSE \
    include/QPersistence.h
