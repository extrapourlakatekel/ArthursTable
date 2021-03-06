######################################################################
# Automatically generated by qmake (3.1) Fri May 8 13:48:44 2020
######################################################################

TEMPLATE = app
TARGET = ArthursTable
INCLUDEPATH += . /usr/include/qt5/QtCore /usr/include/qt5/QtNetwork /usr/include/qt5/QtGui /usr/include/qt5/QtSerialPort /usr/include/qt5/QtWidgets 
CONFIG += qt debug 
CONFIG += optimize_full
QT += network serialport widgets
# We need OPUS and PULSEAUDIO support
LIBS += -lopus -lpulse -lm

# The following define makes your compiler warn you if you use any
# feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Input
HEADERS += algorithms.h \
           ambient.h \
           audioio.h \
           buffer.h \
           bulktransfer.h \
           config.h \
           control.h \
		   convert.h \
           diceroller.h \
		   effectconfig.h \
           facilitymanager.h \
           filewatcher.h \
		   focus.h \
           headtracker.h \
           infopanel.h \
           mainwindow.h \
		   marker.h \
		   messages.h \
		   npcviewer.h \
           painterface.h \
           playercomlink.h \
		   roomacoustics.h \
           setup.h \
		   sketchpad.h \
           soundfx.h \
           streammanager.h \
		   tableselector.h \
		   teamselector.h \
		   tools.h \
		   types.h \
		   viewer.h \
		   visualizer.h \
           voiceeffect.h \
           vumeter.h
SOURCES += algorithms.cpp \
           ambient.cpp \
           audioio.cpp \
           buffer.cpp \
           bulktransfer.cpp \
           control.cpp \
		   convert.cpp \
           diceroller.cpp \
		   effectconfig.cpp \
           facilitymanager.cpp \
           filewatcher.cpp \
		   focus.cpp \
           headtracker.cpp \
           infopanel.cpp \
           main.cpp \
           mainwindow.cpp \
		   marker.cpp \
		   messages.cpp \
		   npcviewer.cpp \
           painterface.c \
           playercomlink.cpp \
		   roomacoustics.cpp \
           setup.cpp \
		   sketchpad.cpp \
           soundfx.cpp \
           streammanager.cpp \
		   tableselector.cpp \
		   teamselector.cpp \
		   tools.cpp \
		   viewer.cpp \
		   visualizer.cpp \
           voiceeffect.cpp \
           vumeter.cpp
           
           
