#ifndef MAINGUI_H
#define MAINGUI_H

#include<QWidget>
#include<QWidget>
#include<QLabel>
#include<QGridLayout>
#include<QSpinBox>
#include<QPushButton>
#include<QComboBox>
#include<QSpacerItem>
#include<QKeyEvent>
#include<QKeySequence>

class maingui : public QWidget
{
    Q_OBJECT
public:
    explicit maingui(QWidget *parent = 0);

private:
    QLabel *camlabela;
    QLabel *heiwidlabela;
    QLabel *bitdeplabela;
    QLabel *bayordlabela;
    QLabel *positionlabela;
    QLabel *brightnesslabela;
    QLabel *rawlabela;
    QLabel *heiwidstulabela;
    QLabel *bitdepstulabela;
    QLabel *bayordstulabela;
    QLabel *positionstulabela;
    QLabel *brightnessstulabela;
    QLabel *rawstulabela;
 
    QLabel *camlabelc;
    QLabel *heiwidlabelc;
    QLabel *bitdeplabelc;
    QLabel *bayordlabelc;
    QLabel *positionlabelc;
    QLabel *brightnesslabelc;
    QLabel *rawlabelc;
    QLabel *heiwidstulabelc;
    QLabel *bitdepstulabelc;
    QLabel *bayordstulabelc;
    QLabel *positionstulabelc;
    QLabel *brightnessstulabelc;
    QLabel *rawstulabelc;

	QLabel 	  *device_mode;
	QComboBox *devmodcombox;
 
    QGridLayout *mainlayout;
    QSpacerItem *spacer1;
    QSpacerItem *spacer2;
	
    QPushButton *opencambut;
    QPushButton *closecambut;
    QPushButton *streamonbut;
	QPushButton *streamoffbut;
	
signals:
	void opencamsig(void);
	void closecamsig(void);
	void streamonsig(void);
	void streamoffsig(void);
	void sentmodeinfosig(int);

public slots:
	void opencamslo(void);
	void closecamslo(void); 
	void streamonslo(void);
	void streamoffslo(void);
	void sentmodeinfo(const QString &);
	void setinfoa(int w,int h,int bit,int bayeroder,int x,int y,int brightness,int raw ); 
	void setinfoc(int w,int h,int bit,int bayeroder,int x,int y,int brightness,int raw ); 
};
#endif 
