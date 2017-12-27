CONFIG(release, debug|release):TARGET = g722
CONFIG(debug, debug|release):TARGET = g722d
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += $$PWD/gsm/inc

DESTDIR = $$PWD/_build

SOURCES += \
        g722/g722_private.h \
        g722/g722_decode.c \
        g722/g722_encode.c

HEADERS += \
        g722/g722.h \
        g722/g722_decoder.h \
        g722/g722_encoder.h

