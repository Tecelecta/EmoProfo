TEMPLATE = \
#app
lib

CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

TARGET = \
#a.out \
#b.out \
#c.out \
pilib

SOURCES += \
#CV src
	#cam/cv.cpp \
	#cam/alt1.cpp \
	cam/piccap.cpp \
	#cam/test_main.cpp \
#stk src
	#mic/play_test.cpp \
	#mic/find_dev.cpp \
	mic/voicap.cpp \
	#mic/voi_test.cpp
#transmission facility
	#trans/client.cpp \
	trans/trans.cpp \
	#trans/dummy_srv.cpp \
	trans/pilib.cpp \
	#trans/transtest.cpp
    trans/pvlib.cpp


HEADERS += \
        cam/piccap.h \
		mic/voicap.h \
		trans/trans.h \
		trans/Pilib.h \
		trans/PVlib.h

INCLUDEPATH += \
		/usr/local/include/opencv2 \
		/usr/local/include/opencv \
		/usr/local/include/ \
		/home/tecelecta/Applications/jdk1.8.0_121/include/ \
		/home/tecelecta/Applications/jdk1.8.0_121/include/linux/

LIBS += /usr/local/lib/libopencv_* \
		/usr/local/lib/libstk* \
		/usr/lib/x86_64-linux-gnu/libcurl.so \
		-pthread

QMAKE_CXXFLAGS += -pthread
