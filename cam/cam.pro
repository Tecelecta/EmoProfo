TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
	#cv.cpp \
	#alt1.cpp \
    piccap.cpp \
    test_main.cpp \
    voicap.cpp

HEADERS += \
    piccap.h \
    voicap.h

INCLUDEPATH += /usr/local/include/opencv2 \
				/usr/local/include/opencv \
				/usr/local/include

LIBS += /usr/local/lib/libopencv_* \
		/usr/local/lib/libstk.so
