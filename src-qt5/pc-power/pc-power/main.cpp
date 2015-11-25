#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    if (argc>1)
    {
        if (QString(argv[1]).trimmed() == "settings")
        {
            w.show();
        }
    }



    return a.exec();
}
