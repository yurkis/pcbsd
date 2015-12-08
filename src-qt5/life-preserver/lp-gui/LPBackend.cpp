#include "LPBackend.h"
#include <QInputDialog>
#include <QObject>
#include <pcbsd-utils.h>

// ==============
//     Informational
// ==============
QStringList LPBackend::listPossibleDatasets(){
  QString cmd = "zpool list -H -o name";
  QStringList out = LPBackend::getCmdOutput(cmd);
  //Now process the output (one dataset per line - no headers)
  QStringList list;
  for(int i=0; i<out.length(); i++){
    QString ds = out[i].section("/",0,0).simplified();
    if(!ds.isEmpty()){ list << ds; }
  }
  list.removeDuplicates();
   
  return list;	
}

QStringList LPBackend::listPoolDatasets(QString pool){
  QString cmd = "zfs list -H -o name";
  QStringList out = LPBackend::getCmdOutput(cmd).filter(pool);
  //qDebug() << "out - " << out;
  //Now process the output (one dataset per line - no headers)
  QStringList list;
  for(int i=0; i<out.length(); i++){
    if(out[i].startsWith(pool+"/") && !out[i].contains("/ROOT/") ){ list << out[i].simplified(); }
  }
  //qDebug() << "list - " << list;
  list.removeDuplicates();
  
  return list;		
}

QStringList LPBackend::listDatasets(){
  QString cmd = "lpreserver listcron snap";
  QStringList out = LPBackend::getCmdOutput(cmd);
  //Now process the output
  QStringList list;
  for(int i=0; i<out.length(); i++){
    //skip the first two lines  and any other headers
    if(out[i].simplified().isEmpty() || out[i].startsWith("----") || (i < out.length()-1 && out[i+1].startsWith("----") ) ){ continue; }
    QString ds = out[i].section(" - ",0,0).simplified();
    if(!ds.isEmpty() && ds!=out[i]){ list << ds; }
  }

  return list;
}

QStringList LPBackend::listScrubs(){
  QString cmd = "lpreserver listcron scrub";
  QStringList out = LPBackend::getCmdOutput(cmd);
  //Now process the output
  QStringList list;
  for(int i=0; i<out.length(); i++){ 
    //skip the first two lines  and any other headers
    if(out[i].simplified().isEmpty() || out[i].startsWith("----") || (i < out.length()-1 && out[i+1].startsWith("----") ) ){ continue; }
    QString ds = out[i].section(" - ",0,0).simplified();
    if(!ds.isEmpty() && ds!=out[i]){ list << ds; }
  }
   
  return list;
}

QStringList LPBackend::listDatasetSubsets(QString dataset){
  QString cmd = "zfs list -H -t filesystem -o name,mountpoint,mounted";
  QStringList out = LPBackend::getCmdOutput(cmd);
  //Now process the output (one dataset per line - no headers)
  QStringList list;
  for(int i=0; i<out.length(); i++){
    if(out[i].startsWith(dataset+"/")){
      if(out[i].section("\t",2,2,QString::SectionSkipEmpty).simplified() == "yes"){
        QString ds = out[i].section("\t",1,1).simplified(); //save the mountpoint
        if(!ds.isEmpty()){ list << ds; }
      }
    }
  }
  list.removeDuplicates();	
   
  return list;
}

QStringList LPBackend::listSnapshots(QString dsmountpoint){
  //List all the snapshots available for the given dataset mountpoint
  QDir dir(dsmountpoint+"/.zfs/snapshot");
  QStringList list;
  if(dir.exists()){
    list = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time | QDir::Reversed);
  }
  return list;
}

QStringList LPBackend::listLPSnapshots(QString dataset, QStringList &comments){
  QString cmd = "lpreserver listsnap "+dataset;
  QStringList out = LPBackend::getCmdOutput(cmd);
  //Now process the output
  QStringList list;
  for(int i=0; i<out.length(); i++){ //oldest ->newest
    if(out[i].startsWith(dataset+"@")){
      QString snap = out[i].simplified().section(" ", 0, 0).section("@",1,3).section(" ",0,0).simplified();
      QString comment = out[i].simplified().section(" ", 1, -1);
      if(!snap.isEmpty()){ list << snap; comments << comment; }
    }
  }
   
  return list;	
}

QStringList LPBackend::listReplicationTargets(){
  QString cmd = "lpreserver replicate list";
  QStringList out = LPBackend::getCmdOutput(cmd);
  //Now process the output
  QStringList list;
  for(int i=0; i<out.length(); i++){
    if(out[i].contains("->")){
      QString ds = out[i].section("->",0,0).simplified();
      if(!ds.isEmpty()){ list << ds; }
    }
  }
   
  return list;		
}

QStringList LPBackend::listCurrentStatus(){
  QString cmd = "lpreserver status";
  QStringList out = LPBackend::getCmdOutput(cmd);
  //Now process the output	
  QStringList list;
  for(int i=2; i<out.length(); i++){ //first 2 lines are headers
    //Format: <dataset> -> <replicationTarget> - <lastsnapshot | NONE> - <lastreplication | NONE>
    if(out[i].isEmpty() || !out[i].contains(" -> ") ){ continue; }
    QString ds  = out[i].section(" -> ",0,0).simplified();
    QString target = out[i].section(" -> ",1,1).section(" - ",0,0).simplified();
    QString snap = out[i].section(" - ",1,1).simplified();
    QString rep = out[i].section(" - ",2,2).simplified();
    if(snap == "NONE"){ snap = "-"; }
    if(rep == "NONE"){ rep = "-"; }
    if(target =="NONE"){ target = "-"; }
    list << ds +":::"+ snap+":::"+rep+":::"+target;
  }
   
  return list;
}

// ==================
//    Dataset Management
// ==================
bool LPBackend::setupDataset(QString dataset, int time, int numToKeep){
  //Configure inputs
  QString freq;
  if(time == -60){ freq = "hourly"; }
  else if(time == -30){ freq = "30min"; }
  else if(time == -10){ freq = "10min"; }
  else if(time == -5){ freq = "5min"; }
  else if(time >= 0 && time < 24){ freq = "daily@"+QString::number(time); }
  else{ freq = "auto"; }
  
  //Create the command
  QString cmd = "lpreserver cronsnap "+dataset+" start "+freq;
  if(freq != "auto"){ cmd.append(" "+QString::number(numToKeep) ); } //also add the number to keep
  //qDebug() << "Lpreserver Command:" << cmd;
  int ret = LPBackend::runCmd(cmd);
   
  return (ret == 0);
}

bool LPBackend::removeDataset(QString dataset){
  QString cmd = "lpreserver cronsnap "+dataset+" stop";
  int ret = LPBackend::runCmd(cmd);
   
  return (ret == 0);
}

bool LPBackend::datasetInfo(QString dataset, int& time, int& numToKeep){
  QString cmd = "lpreserver listcron snap";
  QStringList out = LPBackend::getCmdOutput(cmd);
  //Now process the output
  bool ok = false;
  for(int i=0; i<out.length(); i++){
    if(out[i].section(" - ",0,0).simplified() == dataset){
      //Get time schedule (in integer format)
      QString sch = out[i].section(" - ",1,1).simplified();
      if(sch.startsWith("daily@")){ time = sch.section("@",1,1).simplified().toInt(); }
      else if(sch=="5min"){time = -5;}
      else if(sch=="10min"){time = -10;}
      else if(sch=="30min"){time = -30;}
      else if(sch=="hourly"){ time = -60; } //hourly
      else{ time = -999; } //auto
      //Get total snapshots
      numToKeep = out[i].section("- total:",1,1).simplified().toInt();
      ok=true;
      break;
    }
  }
  //qDebug() << "lpreserver cronsnap:\n" << out << QString::number(time) << QString::number(numToKeep);
   
  return ok;
}

QStringList LPBackend::getDatasetExcludes(QString pool, QString type){
  //type=[snap,rep]
  type = type.toLower();
  if(type !="snap" && type != "rep"){ return QStringList(); }// error in the type
  QString filename = "/var/db/lpreserver/excludes/"+pool+"-"+type;
  QStringList list = pcbsd::Utils::readTextFile(filename).split("\n");;
  list.removeAll(""); //remove any empty lines
  return list;
}

bool LPBackend::setDatasetExcludes(QString pool, QString type, QStringList list){
  //type=[snap,rep]	
  type = type.toLower();
  if(type !="snap" && type != "rep"){ return false; }// error in the type
  QString filename = "/var/db/lpreserver/excludes/"+pool+"-"+type;
  bool ok = pcbsd::Utils::writeTextFile(filename, list.join("\n"), true); //overwrite this file
  return ok;
}

// ==================
//    Snapshop Management
// ==================
void LPBackend::newSnapshot(QString dataset, QString snapshotname, QString snapshotcomment){
  //This needs to run externally - since the snapshot is simply added to the queue, and the replication
  //   afterwards may take a long time.
  QString cmd = "lpreserver mksnap " + dataset + " " + snapshotname.replace(" ", "") + " \"" + snapshotcomment +"\"";
  QProcess::startDetached(cmd);
   
  return;
}

bool LPBackend::removeSnapshot(QString dataset, QString snapshot){
  QString cmd = "lpreserver rmsnap "+dataset +" "+snapshot;
  int ret = LPBackend::runCmd(cmd);
   
  return (ret == 0);
}

bool LPBackend::revertSnapshot(QString dataset, QString snapshot){
  QString cmd = "lpreserver revertsnap "+dataset +" "+snapshot;
  int ret  = LPBackend::runCmd(cmd);
   
  return (ret == 0);
}

// ==================
//    Scrub Management
// ==================

bool LPBackend::setupScrub(QString dataset, int time, int day, QString schedule){
  //Create the command
  QString cmd = "lpreserver  cronscrub "+dataset+" start ";
  if(schedule == "daily"){ cmd.append(schedule+"@"+QString::number(time) ); }
  else if((schedule == "weekly") || (schedule == "monthly")){
    cmd.append(schedule+"@"+QString::number(day)+"@"+QString::number(time));
  }else{
    cmd.append(QString::number(day)); //a set number of days
  }
  int ret = LPBackend::runCmd(cmd);
  qDebug() << "Lpreserver Command:" << cmd;
  return (ret == 0);
}

bool LPBackend::scrubInfo(QString dataset, int& time, int& day, QString& schedule){
  QString cmd = "lpreserver listcron scrub";
  QStringList out = LPBackend::getCmdOutput(cmd);
  //Now process the output
  bool ok = false;
  for(int i=0; i<out.length(); i++){
    if(out[i].section(" - ",0,0).simplified() == dataset){
      //Get time schedule (in integer format)
      QString sch = out[i].section(" - ",1,1).simplified();
      if(sch.startsWith("daily @ ")){
	schedule = "daily";
        day = 0;
        time = sch.section(" @ ",1,1).simplified().toInt();
      } else
      if(sch.startsWith("weekly @ ")){
	schedule = "weekly";
        day = sch.section(" @ ",1,1).simplified().toInt();
        time = sch.section(" @ ",2,2).simplified().toInt();
      } else
      if(sch.startsWith("monthly @ ")){
	schedule = "monthly";
        day = sch.section(" @ ",1,1).simplified().toInt();
        time = sch.section(" @ ",2,2).simplified().toInt();
      } 
      else{
        schedule = "days";
	day = sch.section("every",1,1).section("days",0,0).simplified().toInt();
	time = 0;
      }
      ok=true;
      break;
    }
  }

  return ok;
}

bool LPBackend::removeScrub(QString dataset){
  QString cmd = "lpreserver cronscrub "+dataset+" stop";
  int ret = LPBackend::runCmd(cmd);

  return (ret == 0);
}

// ==================
//    Replication Management
// ==================
bool LPBackend::setupReplication(QString dataset, QString remotehost, QString user, int port, QString remotedataset, int time){
  QString stime = "sync"; //synchronize on snapshot creation (default)
  if(time >= 0 && time < 24){
     stime = QString::number(time);
     // Needs 0 in front of single digits
     if ( stime.length() == 1)
        stime = "0" + stime;
  } //daily at a particular hour (24 hour notation)
  else if(time == -60){ stime = "hour"; }
  else if(time == -30){ stime = "30min"; }
  else if(time == -10){ stime = "10min"; }
  else if(time == -2){ stime = "manual"; }
  
  
  QString cmd = "lpreserver replicate add "+remotehost+" "+user+" "+ QString::number(port)+" "+dataset+" "+remotedataset+" "+stime;
  int ret = LPBackend::runCmd(cmd);
  
  return (ret == 0);
}

bool LPBackend::removeReplication(QString dataset, QString remotehost){
  QString cmd = "lpreserver replicate remove "+dataset+" "+remotehost;
  int ret = LPBackend::runCmd(cmd);	
   
  return (ret == 0);
}

QList<LPRepHost> LPBackend::replicationInfo(QString dataset){
  QString cmd = "lpreserver replicate list";
  QStringList out = LPBackend::getCmdOutput(cmd);
  //qDebug() << " -- Raw Info:" << out;
  //Now process the info
  QList<LPRepHost> repdata;
  for(int i=0; i<out.length(); i++){
    //qDebug() << " -- Line:" << out[i];
    if(out[i].contains(" -> ") && out[i].startsWith(dataset)){
      //qDebug() << " -- init container";
      LPRepHost H;
      QString data = out[i].section(" -> ",1,1);
      //qDebug() << " -- Eval Line:" << data;
      H.setUser( data.section("@",0,0) );
      H.setHost( data.section("@",1,1).section("[",0,0) );
      H.setPort( data.section("[",1,1).section("]",0,0).toInt() );
      H.setDataset( data.section(":",1,1).section(" Time",0,0) ); //could be "ISCSI" instead of a dataset
      QString synchro = data.section("Time:",1,1).simplified();
	if(synchro == "sync"){ H.setFreq(-1); }
	else if(synchro =="manual"){ H.setFreq(-2); }
	else if(synchro =="hour"){ H.setFreq(-60); }
	else if(synchro == "30min"){ H.setFreq(-30); }
	else if(synchro == "10min"){ H.setFreq(-10); }
	else{ H.setFreq(synchro.toInt()); }
      repdata << H; //Add this to the output array
    }
  }	  
  return repdata;
}

// ======================
//          SSH Key Management
// ======================
bool LPBackend::setupSSHKey(QString remoteHost, QString remoteUser, int remotePort){
  QString LPPATH = "/usr/local/share/lifePreserver";
  QString cmd = "xterm -e \""+LPPATH+"/scripts/setup-ssh-keys.sh "+remoteUser+" "+remoteHost+" "+QString::number(remotePort)+"\"";
  int ret = LPBackend::runCmd(cmd);
  return (ret == 0);
}

QStringList LPBackend::findValidUSBDevices(){
  //Return format: "<mountpoint> (<device node>")
  /*QString cmd = "mount";
  QStringList out = LPBackend::getCmdOutput(cmd);
  //Now process the output
  QStringList list;
  for(int i=0; i<out.length(); i++){
      if(out[i].startsWith("/dev/da") && out[i].contains("(msdosfs,")){
      QString mountpoint = out[i].section(" on ",1,1).section("(",0,0).simplified();
      QString devnode = out[i].section(" on ",0,0).section("/",-1).simplified();
      list << mountpoint +" ("+devnode+")";
    }
  }
  return list;*/
  //Get the list of mounted devices from pc-sysconfig
  QString ret = LPBackend::getCmdOutput("pc-sysconfig list-mounteddev").join("");
  if(ret == "[NO INFO]"){ return QStringList(); }
  QStringList devs = ret.split(", ");
  QStringList mount = LPBackend::getCmdOutput("mount");
  QStringList out;
  //Now get the mountpoints for the devices
  for(int i=0; i<mount.length(); i++){
    QString mdev = mount[i].section(" on ",0,0).section("/dev/",1,1);
    if( devs.contains(mdev) ){
      out << mount[i].section(" on ",1,1).section("(",0,0).simplified()+" ("+mdev+")";
    }
  }
  return out;
}

bool LPBackend::isMounted(QString device){
  qDebug() << "Device mount check not implemented yet:" << device;
  return false;
}

bool LPBackend::unmountDevice(QString device){
  qDebug() << "Device unmounting not implemented yet:" << device;
  return false;
}

bool LPBackend::copySSHKey(QString mountPath, QString localHost){
  QString publicKey = "/root/.ssh/id_rsa";
  //copy the file onto the designated USB stick
  if(!mountPath.endsWith("/")){ mountPath.append("/"); }
  QDir lDir=mountPath + "lpreserver";
  if ( ! lDir.exists() )
     lDir.mkdir(lDir.path());

  mountPath.append("lpreserver/"+localHost+"-id_rsa");

  bool ok = QFile::copy(publicKey, mountPath);
  return ok;
}

// ======================
//        USB Device Management
// ======================
QStringList LPBackend::listDevices(){
  //Scan the system for all valid da* and ada* devices (USB/SCSI, SATA)
  //Return format: "<device node> (<device information>)"
  QDir devDir("/dev");
  QStringList devs = devDir.entryList(QStringList() << "da*"<<"ada*", QDir::System | QDir::NoSymLinks, QDir::Name);
  QStringList camOut = LPBackend::getCmdOutput("camcontrol devlist");
  QStringList output, flist;	
  for(int i=0; i<devs.length(); i++){
    flist = camOut.filter("("+devs[i]+",");
    //still need to add an additional device filter to weed out devices currently in use.
    if(!flist.isEmpty()){ output << devs[i] + " ("+flist[0].section(">",0,0).remove("<").simplified()+")"; }
  }
  return output;
}

// ======================
//        ZPOOL Disk Management
// ======================
bool LPBackend::attachDisk(QString pool, QString disk){
  if( !disk.startsWith("/dev/") ){ disk.prepend("/dev/"); } //make sure it is the full disk path
  if( !QFile::exists(disk) ){ return false; } //make sure the disk exists
  QString cmd = "lpreserver zpool attach "+pool+" "+disk;
  //Run the command
  int ret = LPBackend::runCmd(cmd);
  return (ret ==0);
}

bool LPBackend::detachDisk(QString pool, QString disk){
  QString cmd = "lpreserver zpool detach "+pool+" "+disk;
  //Run the command
  int ret = LPBackend::runCmd(cmd);
  return (ret ==0);	
}

bool LPBackend::setDiskOnline(QString pool, QString disk){
  QString cmd = "lpreserver zpool online "+pool+" "+disk;
  //Run the command
  int ret = LPBackend::runCmd(cmd);
  return (ret ==0);	
}

bool LPBackend::setDiskOffline(QString pool, QString disk){
  QString cmd = "lpreserver zpool offline "+pool+" "+disk;
  //Run the command
  int ret = LPBackend::runCmd(cmd);
  return (ret ==0);	
}

// =========================
//             UTILITY FUNCTIONS
// =========================
QStringList LPBackend::getCmdOutput(QString cmd){
  QProcess *proc = new QProcess;
  proc->setProcessChannelMode(QProcess::MergedChannels);
  proc->start(cmd);
  while(!proc->waitForFinished(300)){
    QCoreApplication::processEvents();
  }
  QStringList out = QString(proc->readAllStandardOutput()).split("\n");	
  delete proc;	
  return out;
}

int LPBackend::runCmd(QString cmd, QStringList args){
  QProcess *proc = new QProcess;
  proc->setProcessChannelMode(QProcess::MergedChannels);
  if(args.isEmpty()){	
    proc->start(cmd);
  }else{
    proc->start(cmd, args);
  }
  while(!proc->waitForFinished(300)){
    QCoreApplication::processEvents();
  }
  int ret = proc->exitCode();
  delete proc;	
  return ret;
}
