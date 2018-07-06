#include "cameraview.h"
#include<opencv/cv.h>
#include<opencv2/highgui/highgui.hpp>

CameraView::CameraView(QWidget *parent):QGraphicsView(parent)
{
    QScene = new QGraphicsScene(this);
    setScene(QScene);
    pix = new QGraphicsPixmapItem();
    QScene->addItem(pix);
    hline=new QGraphicsLineItem ;
    sline=new QGraphicsLineItem;
    QScene->addItem(hline);
    QScene->addItem(sline);
    hline->setPen(QPen(Qt::black, 4, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin));
    sline->setPen(QPen(Qt::black, 4, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin));
}
CameraView::~CameraView()
{

}
void CameraView::display(QImage* img)
{
    pix->setPixmap(QPixmap::fromImage(*img));
    cvWaitKey(0);
}
void CameraView::wheelEvent(QWheelEvent* event)
{
    //Scale the view ie. do the zoom
    double scaleFactor = 1.15; //How fast we zoom
    if(event->delta() > 0) {
        //Zoom in
        scale(scaleFactor, scaleFactor);
    } else {
        //Zooming out
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }
    QTransform qtf = transform() ;
    if (qtf.m11()>1)
    {
        scale(1.0/qtf.m11(), 1.0/qtf.m11());
    }
}
void CameraView::mousePressEvent(QMouseEvent *event)
{
    if(event->button()==Qt::LeftButton)
    {
        QPointF mouseclick= pix->mapFromScene(mapToScene(event->pos()));
        emit sendpostion(mouseclick.x(),mouseclick.y());
    }

    if(event->button()==Qt::RightButton)
    {
        QPointF mouseclick= pix->mapFromScene(mapToScene(event->pos()));
        hline->setLine(0,mouseclick.y(),2000,mouseclick.y());
        sline->setLine(mouseclick.x(),0,mouseclick.x(),2000);
    }

    if(event->button()==Qt::MidButton)
    {
        QPointF mouseclick= pix->mapFromScene(mapToScene(event->pos()));
        hline->setLine(0,mouseclick.y(),0,mouseclick.y());
        sline->setLine(mouseclick.x(),0,mouseclick.x(),0);
    }
}
