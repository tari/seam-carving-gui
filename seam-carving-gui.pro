# You can make this with
# qmake seam-carving-gui.pro

TEMPLATE = app
#Becaus the c files have c++ like sytax
QMAKE_CC = g++ 
TARGET = SeamCarvingGui
RC_FILE = seam-carving-gui.rc

win32{
  #In windows, we build a static binary, so we declare the jpeg plugin here
  QTPLUGIN=qjpeg
  DEFINES += STATIC_PLUGINS
}

# Incluce CAIR as backend
include(cair/cair.pri)	

# Input
HEADERS += mainwindow.h 

FORMS += resizewidget.ui

SOURCES += main.cpp \
           mainwindow.cpp 