
#include "maingui.h"

maingui::maingui(QWidget *parent) : QWidget(parent)
{
    setFixedSize(1000,150);
 	//create camera a window show labe
    camlabela 	 	 = new QLabel(tr("Camera_A"));
    heiwidlabela	 = new QLabel(tr("H * W:")); 
    bitdeplabela 	 = new QLabel(tr("Bit Depth:"));
    bayordlabela 	 = new QLabel(tr("Bayer Order:"));
    positionlabelc 	 = new QLabel(tr("position :"));
    brightnesslabela = new QLabel(tr("brightness :")); 
    rawlabela		 = new QLabel(tr("raw :")); 

	//create camera a a place to write param
    heiwidstulabela	    = new QLabel;
    bitdepstulabela     = new QLabel;
    bayordstulabela     = new QLabel;
    positionstulabela   = new QLabel;
    brightnessstulabela = new QLabel;
    rawstulabela        = new QLabel;

	camlabelc 		 = new QLabel(tr("Camera_C")); 
    heiwidlabelc	 = new QLabel(tr("H * W:"));
    bitdeplabelc	 = new QLabel(tr("Bit Depth:")); 
    bayordlabelc     = new QLabel(tr("Bayer Order:"));
    positionlabela   = new QLabel(tr("position :"));
    brightnesslabelc = new QLabel(tr("brightness :"));
   	rawlabelc		 = new QLabel(tr("raw :"));

    heiwidstulabelc		= new QLabel;
    bitdepstulabelc     = new QLabel;
    bayordstulabelc		= new QLabel;
    positionstulabelc	= new QLabel;
    brightnessstulabelc	= new QLabel;
    rawstulabelc		= new QLabel;
	//add device mode slector
	device_mode 		= new QLabel(tr("MODE:"));
	devmodcombox 		= new QComboBox;
	
	devmodcombox->addItem(QWidget::tr("Senosr_mode0"));
	devmodcombox->addItem(QWidget::tr("Senosr_mode1"));
	devmodcombox->addItem(QWidget::tr("Senosr_mode2"));
	devmodcombox->addItem(QWidget::tr("Disk_mode"));
	devmodcombox->addItem(QWidget::tr("Burning_mode"));
	devmodcombox->setCurrentIndex(0);

	//create camera control button
    opencambut    = new QPushButton(tr("Open Camera"));
	closecambut   = new QPushButton(tr("Close Camera"));
    streamonbut   = new QPushButton(tr("Stream ON"));
	streamoffbut  = new QPushButton(tr("Stream OFF"));;


    spacer1 = new QSpacerItem(0,20,QSizePolicy::Minimum,QSizePolicy::Expanding);
    spacer2 = new QSpacerItem(0,20,QSizePolicy::Minimum,QSizePolicy::Expanding);

    QGridLayout *mainlayout = new QGridLayout;

    mainlayout->addWidget(camlabela,0,0);
    mainlayout->addWidget(heiwidlabela, 1, 0);
    mainlayout->addWidget(bitdeplabela, 2, 0);
    mainlayout->addWidget(bayordlabela, 3, 0);

    mainlayout->addWidget(heiwidstulabela, 1, 1);
    mainlayout->addWidget(bitdepstulabela, 2, 1);
    mainlayout->addWidget(bayordstulabela, 3, 1);
	
    mainlayout->addWidget(positionlabela,1,2);
    mainlayout->addWidget(brightnesslabela,2,2);
    mainlayout->addWidget(rawlabela,3,2);

    mainlayout->addWidget(positionstulabela,1,3);
    mainlayout->addWidget(brightnessstulabela,2,3);
    mainlayout->addWidget(rawstulabela,3,3);

    mainlayout->addWidget(camlabelc,0,4);
    mainlayout->addWidget(heiwidlabelc, 1, 4);
    mainlayout->addWidget(bitdeplabelc, 2, 4);
    mainlayout->addWidget(bayordlabelc, 3, 4);

    mainlayout->addWidget(heiwidstulabelc, 1, 5);
    mainlayout->addWidget(bitdepstulabelc, 2, 5);
    mainlayout->addWidget(bayordstulabelc, 3, 5);

    mainlayout->addWidget(positionlabelc,1,6);
    mainlayout->addWidget(brightnesslabelc,2,6);
    mainlayout->addWidget(rawlabelc,3,6);

    mainlayout->addWidget(positionstulabelc,1,7);
    mainlayout->addWidget(brightnessstulabelc,2,7);
    mainlayout->addWidget(rawstulabelc,3,7);

    mainlayout->addWidget(opencambut,1,8);
	mainlayout->addWidget(streamoffbut,2,8);
    mainlayout->addWidget(closecambut,3,8);

    mainlayout->addWidget(streamonbut,1,10);
	mainlayout->addWidget(device_mode,3,10);
	mainlayout->addWidget(devmodcombox,3,11);
    setLayout(mainlayout);

    connect(opencambut, SIGNAL(clicked()),this, SLOT(opencamslo()));
    connect(streamonbut,SIGNAL(clicked()),this,SLOT(streamonslo()));
	connect(streamoffbut,SIGNAL(clicked()),this,SLOT(streamoffslo()));
    connect(closecambut, SIGNAL(clicked()),this, SLOT(closecamslo()));
	connect(devmodcombox, SIGNAL(currentIndexChanged(const QString &)), this, SLOT(sentmodeinfo(const QString &)));
}

void maingui::opencamslo()
{
    emit opencamsig();
}
void maingui::streamonslo()
{
    emit streamonsig();
}

void maingui::streamoffslo()
{
    emit streamoffsig();
}

void maingui::closecamslo()
{
    emit closecamsig();
}
void maingui::sentmodeinfo(const QString &modeinfo)
{   
	int a=QString::compare(QString("Senosr_mode0"),modeinfo);     
	int b=QString::compare(QString("Senosr_mode1"),modeinfo);     
	int c=QString::compare(QString("Senosr_mode2"),modeinfo);
	int d=QString::compare(QString("Disk_mode"),modeinfo);     
	int e=QString::compare(QString("Burning_mode"),modeinfo); 
	
	if (a==0)     
		emit sentmodeinfosig(1);    
	if (b==0)    
		emit sentmodeinfosig(2);     
	if (c==0)     
		emit sentmodeinfosig(3);
	if (d==0)    
		emit sentmodeinfosig(4);  
	if (e==0)    
		emit sentmodeinfosig(5);
}

void maingui::setinfoa(int w,int h,int bit,int bayeroder,int x,int y,int brightness,int raw )
{
    QString  sbayord[9]=
    {
        QString("RGGB"),
        QString("GBRG"),
        QString("BGGR"),
        QString("GRBG"),
        QString("YUV422"),
        QString("YUV420_I420"),
        QString("Mono"),
        QString("YV12"),
        QString("NV21"),
    } ;
    QString heiwid;
	
    heiwid = QString( "%1x%2" ).arg( w ).arg( h );
    heiwidstulabela->setText(heiwid);

    bitdepstulabela->setText(QString( "RAW_%1" ).arg( bit ));

    bayordstulabela->setText(sbayord[bayeroder]);

    positionstulabela->setText(QString( "(%1,%2)" ).arg(x).arg(y));
    brightnessstulabela->setText(QString( "%1" ).arg(brightness));
    rawstulabela->setText(QString( "%1" ).arg(raw));

}
void maingui::setinfoc(int w,int h,int bit,int bayeroder,int x,int y,int brightness,int raw )
{
    QString  sbayord[9]=
    {
        QString("RGGB"),
        QString("GBRG"),
        QString("BGGR"),
        QString("GRBG"),
        QString("YUV422"),
        QString("YUV420_I420"),
        QString("Mono"),
        QString("YV12"),
        QString("NV21"),
    } ;
    QString heiwid;
	
    heiwid = QString( "%1x%2" ).arg( w ).arg( h );
    heiwidstulabelc->setText(heiwid);

    bitdepstulabelc->setText(QString( "RAW_%1" ).arg( bit ));

    bayordstulabelc->setText(sbayord[bayeroder]);

    positionstulabelc->setText(QString( "(%1,%2)" ).arg(x).arg(y));
	
    brightnessstulabelc->setText(QString( "%1" ).arg(brightness));
    rawstulabelc->setText(QString( "%1" ).arg(raw));
}
