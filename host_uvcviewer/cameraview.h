#ifndef CameraView_H
#define CameraView_H
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QGraphicsLineItem>


class CameraView: public QGraphicsView
{
    Q_OBJECT
public:
    explicit CameraView(QWidget *parent = 0);
        ~CameraView();
signals:
    void  sendpostion(int ,int );
private slots:
    void display(QImage* img);
protected:
     virtual void wheelEvent(QWheelEvent* event);
     virtual void mousePressEvent(QMouseEvent *event);
private:
    QGraphicsPixmapItem *pix;
    QGraphicsScene *QScene;
    QGraphicsLineItem *hline;
    QGraphicsLineItem *sline;
};
#endif
