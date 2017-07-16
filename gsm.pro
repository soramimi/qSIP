Release:TARGET = gsm
Debug:TARGET = gsmd
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += $$PWD/gsm/inc

DESTDIR = $$PWD/_build

SOURCES += \
        gsm/src/toast_ulaw.c \
        gsm/src/add.c \
        gsm/src/code.c \
        gsm/src/debug.c \
        gsm/src/decode.c \
        gsm/src/gsm_create.c \
        gsm/src/gsm_decode.c \
        gsm/src/gsm_destroy.c \
        gsm/src/gsm_encode.c \
        gsm/src/gsm_explode.c \
        gsm/src/gsm_implode.c \
        gsm/src/gsm_option.c \
        gsm/src/gsm_print.c \
        gsm/src/long_term.c \
        gsm/src/lpc.c \
        gsm/src/preprocess.c \
        gsm/src/rpe.c \
        gsm/src/short_term.c \
        gsm/src/table.c \
        gsm/src/toast.c \
        gsm/src/toast_alaw.c \
        gsm/src/toast_audio.c \
        gsm/src/toast_lin.c

HEADERS += \
        gsm/inc/unproto.h \
        gsm/inc/config.h \
        gsm/inc/gsm.h \
        gsm/inc/private.h \
        gsm/inc/proto.h \
        gsm/inc/toast.h
