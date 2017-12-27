CONFIG(release, debug|release):TARGET = portaudio
CONFIG(debug, debug|release):TARGET = portaudiod
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += $$PWD/portaudio/include
INCLUDEPATH += $$PWD/portaudio/src/common
INCLUDEPATH += $$PWD/portaudio/src/os/win

DESTDIR = $$PWD/_build

SOURCES += \
        portaudio/src/common/pa_trace.c \
        portaudio/src/common/pa_allocation.c \
        portaudio/src/common/pa_converters.c \
        portaudio/src/common/pa_cpuload.c \
        portaudio/src/common/pa_debugprint.c \
        portaudio/src/common/pa_dither.c \
        portaudio/src/common/pa_front.c \
        portaudio/src/common/pa_process.c \
        portaudio/src/common/pa_ringbuffer.c \
		portaudio/src/common/pa_stream.c

unix:SOURCES += \
    portaudio/src/os/unix/pa_unix_hostapis.c \
    portaudio/src/os/unix/pa_unix_util.c

win32:SOURCES += \
		portaudio/src/hostapi/dsound/pa_win_ds_dynlink.c \
		portaudio/src/hostapi/dsound/pa_win_ds.c \
		portaudio/src/os/win/pa_x86_plain_converters.c \
		portaudio/src/os/win/pa_win_coinitialize.c \
        portaudio/src/os/win/pa_win_hostapis.c \
        portaudio/src/os/win/pa_win_util.c \
        portaudio/src/os/win/pa_win_waveformat.c \
        portaudio/src/os/win/pa_win_wdmks_utils.c

unix:HEADERS += \
    portaudio/src/os/unix/pa_unix_util.h
