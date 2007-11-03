# CAIR backend

DEFINES += BACKEND_CAIR

SOURCES += \
	   $$PWD/CAIR.cpp 

HEADERS += \
	   $$PWD/CAIR.h \
	   $$PWD/CAIR_CML.h \

win32{
#use the pthreads lib
DEFINES += PTHREADS_WINDOWS
INCLUDEPATH += $$PWD/pthreads
HEADERS += \
	   $$PWD/pthreads/pthread.h \
	   $$PWD/pthreads/sched.h \
	   $$PWD/pthreads/semaphore.h 
LIBS += cair\pthreads\pthreadVSE2.lib
}
