TEMPLATE = subdirs

CONFIG += ordered
SUBDIRS = \
    src \
    examples \
    tests \
    testDatabaseChanger

src.subdir      = src
examples.subdir     = examples
examples.depends    = src

OTHER_FILES += \
    README.md \
    LICENSE \
    include/QPersistence.h
