TEMPLATE = subdirs

CONFIG += ordered
SUBDIRS = src examples

src.subdir      = src
examples.subdir     = examples
examples.depends    = src
