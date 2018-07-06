#include "mv2_hostkit.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MV2_HostKit w;
    w.show();
    return a.exec();
}
