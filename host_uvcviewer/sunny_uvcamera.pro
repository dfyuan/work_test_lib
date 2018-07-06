#-------------------------------------------------
#
# Project created by QtCreator 2015-12-21T10:00:13
#
#-------------------------------------------------

QT  += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MV2_HostKit
TEMPLATE = app

INCLUDEPATH +=/usr/local/include/opencv
INCLUDEPATH +=/usr/local/include/opencv2

SOURCES += main.cpp\
    mv2_hostkit.cpp \
    maingui.cpp \
    cameraview.cpp \
	
HEADERS  += mv2_hostkit.h \
    maingui.h \
    cameraview.h \
	mipi_tx_header.h \
	video.h \

Debug
{
    LIBS += -L/usr/local/lib/ -lopencv_core -lopencv_highgui -lopencv_imgproc
#    LIBS += -lusb-1.0
}

Release
{
    LIBS += -L/usr/local/lib/ -lopencv_core -lopencv_highgui -lopencv_imgproc
}
