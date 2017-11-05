#-------------------------------------------------
#
# Project created by QtCreator 2017-07-15T15:13:10
#
#-------------------------------------------------

QT       += core gui widgets multimedia

TARGET = qSIP
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += $$PWD/baresip/include
INCLUDEPATH += $$PWD/re/include
INCLUDEPATH += $$PWD/qSIP

LIBS += -L$$PWD/_build -lws2_32 -liphlpapi -lwinmm
Release:LIBS += -lbaresip -lre -lrem -lportaudio -lspeex -lwebrtc -lgsm -lg722
Debug:LIBS += -lbaresipd -lred -lremd -lportaudiod -lspeexd -lwebrtcd -lgsmd -lg722d

SOURCES += \
	qSIP/main.cpp \
	qSIP/MainWindow.cpp \
	qSIP/PhoneThread.cpp \
    qSIP/joinpath.cpp \
    qSIP/MySettings.cpp \
    qSIP/SettingsDialog.cpp \
    qSIP/SettingAccountForm.cpp \
    qSIP/AbstractSettingForm.cpp \
    qSIP/misc.cpp \
    qSIP/StatusLabel.cpp \
	qSIP/PhoneWidget.cpp \
	qSIP/ClearButton.cpp

HEADERS += \
	qSIP/MainWindow.h \
	qSIP/PhoneThread.h \
    qSIP/joinpath.h \
    qSIP/MySettings.h \
    qSIP/SettingsDialog.h \
    qSIP/SettingAccountForm.h \
    qSIP/AbstractSettingForm.h \
    qSIP/misc.h \
    qSIP/main.h \
    qSIP/Account.h \
    qSIP/StatusLabel.h \
	qSIP/PhoneWidget.h \
	qSIP/ClearButton.h

FORMS += \
	qSIP/MainWindow.ui \
    qSIP/SettingsDialog.ui \
	qSIP/SettingAccountForm.ui \
	qSIP/PhoneWidget.ui

RESOURCES += \
    qSIP/resources.qrc
