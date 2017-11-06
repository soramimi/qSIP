win32:Release:TARGET = g722
win32:Debug:TARGET = g722d
unix:TARGET = g722
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

