#include "services.h"

#include <QFile>
#include <QObject>

QList<service> Services::getServiceList(){
  //Define all the pre-set services here
  // NOTE: This assumes that all of these services are disabled by default!!
  QList<service> out;

  //SSHD
  if(QFile::exists("/etc/rc.d/sshd")){
    service S;
    S.ID = "SSHD";
    S.file = "/etc/rc.d/sshd"; //This file needs to exist to show/start this service
    S.name = QObject::tr("Enable SSH");
    S.description = QObject::tr("Allows SSH access to this system from remote systems");
    S.openPorts << "tcp 22";
    S.rcLines << "sshd_enable=YES";
    //S.cmds << ""; //optional extra commands
    out << S;
  }
  //DISABLE IPV6
  if(QFile::exists("/etc/rc.conf")){
    service S;
    S.ID = "DISABLE-IPV6";
    S.file = "/etc/rc.conf"; //This file needs to exist to show/start this service
    S.name = QObject::tr("Disable IPv6");
    S.description = QObject::tr("Disable the use of IPv6 connections");
    //S.openPorts
    S.rcLines << "ipv6_activate_all_interfaces=NO";
    //S.cmds << "service netif restart"; //optional extra commands
    out << S;
  }
	
  return out;
}

void Services::enableService(service S){
  //First open any firewall ports
  if(!S.openPorts.isEmpty()){
    QStringList lines;
    lines << "# "+S.ID;
    lines.append(S.openPorts);
    system("echo \""+lines.join("\n").toLocal8Bit()+"\n\" >> /etc/ipfw.openports");
    system("/bin/sh /etc/ipfw.rules"); //reload the firewall rules
  }
  //Now make any changes to /etc/rc.conf (for auto-start on next boot)
  if(!S.rcLines.isEmpty()){
    for(int i=0; i<S.rcLines.length(); i++){
      system("sysrc -f /etc/rc.conf "+S.rcLines[i].toLocal8Bit());
    }
  }
  //Now make any other CLI changes that are listed
  if(!S.cmds.isEmpty()){
    for(int i=0; i<S.cmds.length(); i++){
      system(S.cmds[i].toLocal8Bit());
    }
  }
  //Now startup this service for the current boot session
  system(S.file.toLocal8Bit()+" start");
}