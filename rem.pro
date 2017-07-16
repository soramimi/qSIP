Release:TARGET = rem
Debug:TARGET = remd
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += $$PWD/re/include
INCLUDEPATH += $$PWD/rem/include

DESTDIR = $$PWD/_build

SOURCES += \
        rem/src/aubuf/aubuf.c \
        rem/src/auresamp/resamp.c \
        rem/src/autone/tone.c \
        rem/src/fir/fir.c \
        rem/src/g711/g711.c \
        rem/src/aufile/wave.c \
        rem/src/aufile/aufile.c

HEADERS += \
        rem/src/aufile/aufile.h \
        rem/include/rem.h \
        rem/include/rem_au.h \
        rem/include/rem_aubuf.h \
        rem/include/rem_audio.h \
        rem/include/rem_aufile.h \
        rem/include/rem_aumix.h \
        rem/include/rem_auresamp.h \
        rem/include/rem_autone.h \
        rem/include/rem_dsp.h \
        rem/include/rem_fir.h \
        rem/include/rem_g711.h \
        rem/include/rem_vid.h \
        rem/include/rem_vidconv.h \
        rem/include/rem_video.h \
        rem/include/rem_vidmix.h
