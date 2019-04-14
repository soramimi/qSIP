QT += core multimedia
QT -= gui

CONFIG(release, debug|release):TARGET = baresip
CONFIG(debug, debug|release):TARGET = baresipd
TEMPLATE = lib
CONFIG += staticlib c++11
CONFIG -= app_bundle

QMAKE_CFLAGS += -std=c11

INCLUDEPATH += $$PWD/re/include
INCLUDEPATH += $$PWD/rem/include
INCLUDEPATH += $$PWD/baresip/include
INCLUDEPATH += $$PWD/g722
INCLUDEPATH += $$PWD/gsm/inc
INCLUDEPATH += $$PWD/speex/include
INCLUDEPATH += $$PWD/portaudio/include
INCLUDEPATH += $$PWD/webrtc

QMAKE_CFLAGS += -DSTATIC

DESTDIR = $$PWD/_build

HEADERS += \
	baresip/src/core.h \
	baresip/src/magic.h \
	baresip/include/baresip.h \
	baresip/modules/g726_32/g726.h \
	baresip/modules/presence/presence.h \
	baresip/modules/dialog_info/dialog_info.h \
	baresip/modules/recorder/wavfile.h \
	include/module.h \
    baresip/modules/qtaudio/qtaudio.h \
    baresip/modules/qtaudio/audioio.h

SOURCES += \
	baresip/src/ua.c \
	baresip/src/account.c \
	baresip/src/aucodec.c \
	baresip/src/audio.c \
	baresip/src/aufilt.c \
	baresip/src/auplay.c \
	baresip/src/ausrc.c \
	baresip/src/bfcp.c \
	baresip/src/call.c \
	baresip/src/cmd.c \
	baresip/src/conf.c \
	baresip/src/config.c \
	baresip/src/contact.c \
	baresip/src/mctrl.c \
	baresip/src/menc.c \
	baresip/src/message.c \
	baresip/src/mnat.c \
	baresip/src/module.c \
	baresip/src/net.c \
	baresip/src/play.c \
	baresip/src/reg.c \
	baresip/src/rtpkeep.c \
	baresip/src/sdp.c \
	baresip/src/sipreq.c \
	baresip/src/static.c \
	baresip/src/stream.c \
	baresip/modules/g711/g711.c \
	baresip/modules/g722/g722.c \
	baresip/modules/g726_32/g726_32.c \
	baresip/modules/g726_32/g726.c \
	baresip/modules/gsm/gsm.c \
	baresip/modules/portaudio/portaudio.c \
	baresip/modules/speex/speex.c \
	baresip/modules/speex_aec/speex_aec.c \
	baresip/modules/speex_pp/speex_pp.c \
	baresip/modules/stun/stun.c \
	baresip/modules/presence/subscriber.c \
	baresip/modules/presence/notifier.c \
	baresip/modules/presence/presence.c \
	baresip/modules/dialog_info/dialog_info_subscriber.c \
	baresip/modules/dialog_info/dialog_info.c \
	baresip/modules/webrtc_aec/webrtc_aec.c \
	baresip/modules/mwi/mwi.c \
	baresip/src/paging_tx.c \
	baresip/modules/l16/l16.c \
	baresip/modules/softvol/softvol.c \
    baresip/modules/qtaudio/qtaudio_in.cpp \
    baresip/modules/qtaudio/qtaudio_out.cpp \
    baresip/modules/qtaudio/qtaudio.c

win32:HEADERS = \
	baresip/modules/winwave/winwave.h

win32:SOURCES += \
	baresip/modules/aufile/aufile.c \
	baresip/modules/recorder/recorder.c \
	baresip/modules/recorder/wavfile.c \
	baresip/modules/winwave/winwave_play.c \
	baresip/modules/winwave/src.c \
	baresip/modules/winwave/winwave.c
