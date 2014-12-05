#include <QApplication>
#include <qtranslator.h>
#include <qlocale.h>
#include <qtsingleapplication.h>
#include <QDebug>
#include <QMessageBox>
//#include <QSplashScreen>
#include <QProcess>
#include <QFile>

#include "MainUI.h"
//#include "migrateUI.h"

#include <unistd.h>
#include <sys/types.h>

//#include "../config.h"
#ifndef PREFIX
#define PREFIX QString("/usr/local")
#endif

int main( int argc, char ** argv )
{
    //Check for root permissions
    if( getuid() == 0){
      qDebug() << "pc-softwaremanager must not be started as root!";
      //Try to get the username
      QString user = getenv("SUDO_USER");
      if(user=="root"){ return 1; } //can't do anything (probably started from CLI)
      //Try to see if the desktop entry needs to be fixed
      QString path = "/usr/home/"+user+"/Desktop/appcafe.desktop";
      QApplication a(argc, argv);
      if(QFile::exists(path)){
	QFile::remove(path);
	QFile::copy("/usr/local/share/applications/softmanager.desktop", path);
	QProcess::execute("chown "+user+":"+user+" "+path);
	QMessageBox::warning(0, "Please Restart AppCafe", "Please restart the AppCafe. The new AppCafe only needs to run with user permissions, and the shortcut on your desktop has been fixed to prevent this error in the future.");
      }else{
	QMessageBox::warning(0, "Invalid User", "The AppCafe should be run with user permissions." );
      }
      return 1;
    }
    qDebug() << "Starting Up the AppCafe";
    QtSingleApplication a(argc, argv);   
    if ( a.isRunning() ){
      qDebug() << " - an instance of the AppCafe is already running";
      return !(a.sendMessage("show"));
    }

    QTranslator translator;
    QLocale mylocale;
    QString langCode = mylocale.name();
    if ( ! QFile::exists( PREFIX + "/share/pcbsd/i18n/SoftwareManager_" + langCode + ".qm" ) )
      langCode.truncate(langCode.indexOf("_"));
    translator.load( QString("SoftwareManager_") + langCode, PREFIX + "/share/pcbsd/i18n/" );
    a.installTranslator( &translator );
    qDebug() << "Locale:" << langCode;

    bool debug = false;
    if(argc > 1){ debug = ( QString(argv[1])=="--debug"); }
    
      //Launch the UI
      MainUI w(debug); 
      w.show();

      QObject::connect(&a, SIGNAL(messageReceived(const QString&)), &w, SLOT(slotSingleInstance()) );
      a.connect( &a, SIGNAL( lastWindowClosed() ), &a, SLOT( quit() ) );
      return a.exec();

}
