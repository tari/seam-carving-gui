# You can make this with
# qmake seam-carving-gui.pro

TEMPLATE = app
TARGET = SeamCarvingGui
RC_FILE = seam-carving-gui.rc
CONFIG += release

#Because the c files have c++ like sytax
!win32:QMAKE_CC = g++ 

macx{
  CONFIG += x86 ppc
  ICON = g.icns
}

# Incluce CAIR as backend
include(cair/cair.pri)	

# Input
HEADERS += mainwindow.h 

FORMS += resizewidget.ui

SOURCES += main.cpp \
           mainwindow.cpp 
