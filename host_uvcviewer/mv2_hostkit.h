#ifndef MV2_HOSTKIT_H
#define MV2_HOSTKIT_H

#include<QMainWindow>
#include<maingui.h>
#include<QStatusBar>
#include<QSplitter>
#include"cameraview.h"
#include<opencv2/highgui/highgui.hpp>
#include<opencv/cv.h>
#include<linux/uvcvideo.h>
#include<QThread>
#include<QDateTime>
#include"mipi_tx_header.h"

class MV2_HostKit : public QMainWindow
{
    Q_OBJECT

public:
    MV2_HostKit(QWidget *parent = 0);
    void closeevent(QCloseEvent *event);
	
    ~MV2_HostKit();

private:
	QWidget *blank;
    QStatusBar *mainstatusbar;
	
    CameraView *camviea;
	CameraView *camviec;
    maingui    *mainguiwidget;

	struct uvc_xu_control_query xu_data;
    struct uvc_xu_control_query xu_mode;
    int setsensormode(int);

protected:
    void keypressevent(QKeyEvent *event);

private slots:
    void videomain();
    void teardown();
    void getpositiona(int x,int y);
    void getpositionc(int x,int y);
    void streamon(void);
	void streamoff(void);
	int  getimudata(void);
	int  set_mode(int mode);


public:
    void cv_display(void* frame, int frame_lenght);
    void mainloop(void);
	int  read_frame(void);
    int  testtimer();
	int start_videoprocess();
	
signals:
    void  displaycama(QImage*);
    void  displaycamc(QImage*);
    void  sendinfocama(int ,int ,int , int, int ,int , int, int);
    void  sendinfocamc(int ,int ,int , int, int ,int , int, int);
    void  sendimutitlesig(int);
    void  getimusig();
};
#endif 
