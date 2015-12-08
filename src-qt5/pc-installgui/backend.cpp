#include <QApplication>
#include <QFile>

#include "backend.h"

using namespace Scripts;

void Backend::createErrorReport()
{
  QString line;

  QProcess p;
  QString prog = "xterm";
  QStringList args;
  args << "-e" << "/root/save-logs.sh";
  p.start(prog, args);
  if (p.waitForFinished()) {
  }
}

void Backend::setupSSHKeys(QString Host, QString User, QString Port)
{
  QString line;
  QString PCSYSINSTALL;
  if ( QFile::exists("/root/pc-sysinstall/pc-sysinstall") )  
     PCSYSINSTALL = "/root/pc-sysinstall/pc-sysinstall";
  else  
     PCSYSINSTALL = "/usr/local/sbin/pc-sysinstall";

  QProcess p;
  QString prog = "xterm";
  QStringList args;
  args << "-e" << PCSYSINSTALL << "setup-ssh-keys" << User << Host << Port;
  p.start(prog, args);
  if (p.waitForFinished()) {
  }
}

void Backend::enableNic(QString Nic, QString IP, QString NetMask, QString DNS, QString Gate, bool fetchMirrors, QString IPv6, QString IPv6Gate, QString IPv6DNS)
{
    QString line;
    QString Mirrors;
    if ( fetchMirrors )
      Mirrors = "ON";
    else
      Mirrors = "OFF";

    Process p(QStringList() << "enable-net" << Nic << IP << NetMask << DNS << Gate << Mirrors << IPv6 << IPv6Gate << IPv6DNS);
    while(p.state() == QProcess::Starting || p.state() == QProcess::Running)
 	QCoreApplication::processEvents();

    while (p.canReadLine()) {
      qDebug() << "Enable Nic: " << p.readLine().simplified();
    }
}

QStringList Backend::timezones()
{
    QStringList _zones;
    QString line;

    Process p(QStringList() << "list-tzones");

    if (p.waitForFinished()) {
        while (p.canReadLine()) {
            line = p.readLine();
            line = line.simplified();
            line.truncate(65);
            _zones.append(line);
        }
    }
    return _zones;
}

QStringList Backend::languages()
{
    QStringList _languages;
    QString code, desc, line;

    QFile mFile;
    mFile.setFileName("/usr/local/share/pc-sysinstall/conf/avail-langs");
    if ( ! mFile.open(QIODevice::ReadOnly | QIODevice::Text))
       return QStringList();

    // Read in the meta-file for categories
    QTextStream in(&mFile);
    in.setCodec("UTF-8");
    while ( !in.atEnd() ) {
       line = in.readLine();
       code = line;
       code.truncate(line.indexOf(" "));
       desc = line.remove(0, line.indexOf(" "));
        _languages.append(desc.simplified() + " - (" + code.simplified() + ")");
    }
    mFile.close();
    return _languages;
}

QStringList Backend::keyModels()
{
    QStringList _models;
    QString code, desc, line;

    Process p(QStringList() << "xkeyboard-models");

    if (p.waitForFinished()) {
        while (p.canReadLine()) {
            line = p.readLine();
            code = line;
            code.truncate(line.indexOf(" "));
            desc = line.remove(0, line.indexOf(" "));
            _models.append(desc.simplified() + " - (" + code.simplified() + ")");
        }
    }
    return _models;
}

QStringList Backend::keyLayouts()
{
    QStringList _layouts;
    QString code, desc, line;

    Process p(QStringList() << "xkeyboard-layouts");

    if (p.waitForFinished()) {
        while (p.canReadLine()) {
            line = p.readLine();
            code = line;
            code.truncate(line.indexOf(" "));
            desc = line.remove(0, line.indexOf(" "));
            desc.replace("(", "'");
            desc.replace(")", "'");
            _layouts.append(desc.simplified() + " - (" + code.simplified() + ")");
        }
    }
    return _layouts;
}

// Function which gets the key Variants for the target layout
QStringList Backend::keyVariants(const QString &layout, QStringList &savedKeyVariants)
{
    QStringList _variants;
    QString code, desc, line;

    if ( savedKeyVariants.empty() )
    {
      Process p(QStringList() << "xkeyboard-variants");
      while(p.state() == QProcess::Starting || p.state() == QProcess::Running)
        QCoreApplication::processEvents();

      while (p.canReadLine()) {
          line = p.readLine().simplified();
          savedKeyVariants << line;
      }
    }

    for (int i = 0; i < savedKeyVariants.size(); ++i) {
       // Look for variants for this particular layout
       line = savedKeyVariants.at(i);
       if ( line.indexOf(" " + layout + ":") != -1 )
       {
         code = line.simplified();
         code.truncate(code.indexOf(" "));
         desc = line.remove(0, line.indexOf(": ") + 1);
         _variants.append(desc.simplified() + " - (" + code.simplified() + ")");
       }
    }

    return _variants;
}

QStringList Backend::bootableMedia()
{
    QStringList media;

    Process p(QStringList() << "");

    if (p.waitForFinished()) {
        while (p.canReadLine()) {
            //
        }
    }
    return media;
}


int Backend::systemMemory()
{
  int mem;
  QString tmp;
  bool ok;

  Process p(QStringList() << "sys-mem");

  if (p.waitForFinished()) {
      while (p.canReadLine()) {
          tmp = p.readLine().simplified();
      }
  }
  mem = tmp.toInt(&ok);
  qDebug() << "System Mem:" << mem;
  if ( ok )
    return mem;
  else
    return -1;
}

QString Backend::detectCountryCode()
{
    QString code;

    Process p(QStringList() << "detect-country");

    if (p.waitForFinished()) {
       code = p.readLine().simplified();
    }
    code = code.section(" ", 0, 0);
    qDebug() << "Found Country Code:" << code;
    return code;
}


QStringList Backend::networkDevices()
{
    QStringList nics;

    Process p(QStringList() << "detect-nics");

    if (p.waitForFinished()) {
        while (p.canReadLine()) {
            nics.append(p.readLine().simplified());
        }
    }
    qDebug() << "Found Nics:" << nics;
    return nics;
}

// Function which lets us run setxkbmap
void Backend::changeKbMap(QString model, QString layout, QString variant)
{
   QProcess kbp;
   QStringList args;
   QString prog;
   prog = "setxkbmap"; 
   args << "-model" << model << "-layout" << layout << "-variant" << variant;
   qDebug() << "setxkbmap:" << args;
   kbp.start(prog, args);
   kbp.waitForFinished();
}

QList<QStringList> Backend::availComponents()
{
   QList<QStringList> components;
   QStringList singleComponent;

   QString tmp, name, desc, icon, type, line;
   QString selected = "off";

   Process p(QStringList() << "list-components");

   if (p.waitForFinished()) {
       while (p.canReadLine()) {
          line = p.readLine();
          line = line.simplified();
          if ( line.indexOf("name:") != -1 )
            name = line.remove(0, line.indexOf(": ") + 2); 
          if ( line.indexOf("desc:") != -1 )
            desc = line.remove(0, line.indexOf(": ") + 2); 
          if ( line.indexOf("type:") != -1 )
            type = line.remove(0, line.indexOf(": ") + 2); 
          if ( line.indexOf("icon:") != -1 ) {
            icon = line.remove(0, line.indexOf(": ") + 2); 
            singleComponent << name << desc << icon << type << selected;
            components << singleComponent;
            qDebug() << "Found Component:" << singleComponent;
            singleComponent.clear();
          }
       }
   }

   return components;
}

QList<QStringList> Backend::hardDrives()
{
    QList<QStringList> drives;
    QStringList drive; //its a "list" so as to also append drive information
    QStringList partition; //its a "list" so as to also append drive information

    QString size, devinfo, type;
    QString line, info, format;
    QString tmp, dev, lastslice, slice, slabel, ssize;
    bool ok;

    Process p(QStringList() << "disk-list");

    if (p.waitForFinished(90000)) {
        while (p.canReadLine()) {
            line = p.readLine();
	    if ( line.isEmpty() ) 
               continue;
            dev = line.simplified();
            dev.truncate(line.indexOf(":"));

            tmp = line.simplified().remove(0, line.indexOf(":") + 1);
            devinfo = tmp;

            // Get the disk information for this dev
            Process pp(QStringList() << "disk-info" << dev);
            if (pp.waitForFinished()) {
                while (pp.canReadLine()) {
                    info = pp.readLine().simplified();
                    if (info.indexOf("size=") == 0) size = info.replace("size=", "");
                    if (info.indexOf("type=") == 0) type = info.replace("type=", "");
                }
            }

	    // Pad the disk size a bit
	    size.toInt(&ok);
	    if ( !ok)
		continue;
	    //size.setNum(size.toInt(&ok) - 100);

            // Add this info to our list
            qDebug() << "Found Drive:" << dev << size << devinfo << type;
            drive.clear();
            drive << "DRIVE" << dev << size << devinfo << type;
            drives.append(drive);

            // Init lastslize in case this disk is completely empty
            lastslice = "s0";

            // Get the slice information for this disk
            Process ppp(QStringList() << "disk-part" << dev);
            if (ppp.waitForFinished()) {
                while (ppp.canReadLine()) {
                    info = ppp.readLine().simplified();
                    // Get the slice we are working on
                    if ( info.indexOf(dev + "s") == 0 || info.indexOf(dev + "p") == 0 ) {
                      slice = info;
                      slice.truncate(slice.indexOf("-"));
                    } else {
                      slice = "";
                    }
                     
                    if (info.indexOf(slice + "-label: ") == 0) slabel = info.replace(slice + "-label: ", "");

                    // Check if we've found the format flag
                    if (info.indexOf(dev + "-format: ") == 0) {
                      format = info.replace(dev + "-format: ", "");
                      qDebug() << "Found Disk Format: " <<  dev << " - " << format;
                      partition.clear();
                      partition << "FORMAT" << dev << format;
                      drives.append(partition);
                    }

                    // Check if we've found the new slice
                    if (info.indexOf(slice + "-sizemb: ") == 0) {
                      ssize = info.replace(slice + "-sizemb: ", "");
		      // Make sure we have a number
		      ssize.toInt(&ok);
		      if (!ok)
			continue;

		      // Pad the slice by 5MB
		      //ssize.setNum(ssize.toInt(&ok) - 5);
			
                      qDebug() << "Found Slice:" << dev << slice << slabel << ssize;
                      partition.clear();
                      partition << "SLICE" << dev << slice << ssize << slabel;   
                      drives.append(partition);
                      lastslice = slice;
                    }

                    // Check if we've found some free disk space
                    if (info.indexOf(dev + "-freemb: ") == 0) {
                      bool ok;
		      int checkSize;
                      ssize = info.replace(dev + "-freemb: ", "");
                      checkSize = ssize.toInt(&ok); 
                      if ( ok && checkSize > 100 )
                      {
                        // Figure out the next slice number, if its less than 4
                        QString freeslice;
                        tmp = lastslice;
                        tmp = tmp.remove(0, tmp.size() - 1);
                        int nextslicenum = tmp.toInt(&ok);
                        if ( ok ) {
                          if ( format == "MBR" || format == "mbr" ) {
                            nextslicenum++;
                            slice = dev + "s" + tmp.setNum(nextslicenum);
                            slabel = "Unused Space";
                            qDebug() << "Found Slice:" << dev << slice << slabel << ssize;
                            partition.clear();
                            partition << "SLICE" << dev << slice << ssize << slabel;
                            drives.append(partition);
                          } else if ( format == "GPT" || format == "gpt" ) {
                            nextslicenum++;
                            slice = dev + "p" + tmp.setNum(nextslicenum);
                            slabel = "Unused Space";
                            qDebug() << "Found Slice:" << dev << slice << slabel << ssize;
                            partition.clear();
                            partition << "SLICE" << dev << slice << ssize << slabel;
                            drives.append(partition);
 			  }
			}
		      }
                    } // End of Free Space Check
                }
            }

        }
    }
    return drives;
}

