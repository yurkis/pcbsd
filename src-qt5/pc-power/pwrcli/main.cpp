#include <QCoreApplication>
#include <QTimer>
#include "pwrcli.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    PWRCLI* app = new PWRCLI(&a);
    QObject::connect(app, SIGNAL(finished()), &a, SLOT(quit()));
    QTimer::singleShot(0, app, SLOT(run()));

    return a.exec();
}

