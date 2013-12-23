TEMPLATE = subdirs

CONFIG += ordered
SUBDIRS = src examples \
    tests

src.subdir      = src
examples.subdir     = examples
examples.depends    = src
