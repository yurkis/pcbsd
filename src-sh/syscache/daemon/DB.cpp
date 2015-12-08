#include "DB.h"
#include <QJsonDocument>
#include <QJsonObject>

/* === Representation of Database ====
  HASH -> Info: [JailList, RepoList] 
	       Categories: [Jails/<jail name>/, Repos/<repoID>]
  
  Jail Info Category -> Info: [JID, jailIP, jailPath, RepoID, hasUpdates, updateLog, pkgList, lastSyncTimeStamp (internal)]
				Categories: [pkg/<local pkg origin>/]
				
  Repo Info Category -> Info: [pkgList, lastSyncTimeStamp (internal)]
				  Categories: [pkg/<remote pkg origin>/]
				  
  Local Pkg Info: [name, origin, description, comment, version, maintainer, website, 
		license, size, arch, isOrphan, isLocked, message, timestamp, dependencies, reverse dependencies,
		categories, files, options, users, groups, annotations]
		
  Remote Pkg Info: [name, origin, version, maintainer, comment, description, website, arch,
		licence, size, message, dependencies, reverse dependencies, categories, options,
		annotations]


  EXAMPLE: 
  To fetch the name of a pkg on the repository for a jail:
  
  (Input string variables: myjail and mypkg)
  QString repoID = HASH->value("Jail/"+myjail+"/RepoID","");
  QString name = HASH->value("Repo/"+repoID+"/"+mypkg+"/name","");
  
*/

#define LISTDELIMITER QString("::::")
#define LINEBREAK QString("<LINEBREAK>")
#define LOCALSYSTEM QString("**LOCALSYSTEM**")
#define REBOOT_FLAG QString("/tmp/.rebootRequired")
#define UPDATE_FLAG_CHECK QString("pgrep -F /tmp/.updateInProgress") //returns 0 if active
#define PKG_REPO_FLAG QString("-r pcbsd-major ")

DB::DB(QObject *parent) : QObject(parent){
  HASH = new QHash<QString, QString>;
  SYNC = new Syncer(0, HASH);
	connect(SYNC, SIGNAL(finishedLocal()), this, SLOT(localSyncFinished()) );
	connect(SYNC, SIGNAL(finishedRemote()), this, SLOT(remoteSyncFinished()) );
	connect(SYNC, SIGNAL(finishedPBI()), this, SLOT(pbiSyncFinished()) );
	connect(SYNC, SIGNAL(finishedSystem()), this, SLOT(systemSyncFinished()) );
	connect(SYNC, SIGNAL(finishedJails()), this, SLOT(jailSyncFinished()) );
  syncThread = new QThread;
	SYNC->moveToThread(syncThread);
	syncThread->start(QThread::LowPriority); //don't slow down normal system usage for a sync
  chkTime = new QTimer(this);
	chkTime->setInterval(300000); // 5 minute delay for sync on changes
	chkTime->setSingleShot(true);
	connect(chkTime, SIGNAL(timeout()), this, SLOT(kickoffSync()) );
  maxTime = new QTimer(this);
	maxTime->setInterval(24*60*60*1000); // re-sync every 24 hours
	connect(maxTime, SIGNAL(timeout()), this, SLOT(kickoffSync()) ); 
  watcher = new QFileSystemWatcher(this);
    connect(watcher, SIGNAL(directoryChanged(QString)), this, SLOT(watcherChange(QString)) );
    connect(watcher, SIGNAL(fileChanged(QString)), this, SLOT(watcherChange(QString)) );
  //Setup the watcher to look for the pc-systemflag flags
  if(!QFile::exists("/tmp/.pcbsdflags")){ QProcess::startDetached("pc-systemflag CHECKDIR"); }
  locrun = remrun = pbirun = jrun = sysrun = false;
}

DB::~DB(){
  if(syncThread->isRunning()){ syncThread->quit(); syncThread->wait();}//make sure the sync gets stopped appropriately
  delete HASH;
  delete SYNC;
  delete syncThread;
}

// ===============
//    PUBLIC
// ===============
void DB::startSync(){
  // --reset watched directories
  if(!watcher->directories().isEmpty()){ watcher->removePaths( watcher->directories() ); }
  watcher->addPath("/var/db/pkg"); //local system pkg database should always be watched
  watcher->addPath("/tmp/.pcbsdflags"); //local PC-BSD system flags
  watcher->addPath("/var/db/pbi/index"); //local PBI index directory
  QTimer::singleShot(0,this, SLOT(kickoffSync()));
}

void DB::shutDown(){
  HASH->clear();
}

QString DB::fetchInfo(QStringList request, bool noncli){
  if(HASH->isEmpty()){ 
    startSync();
    QCoreApplication::processEvents();
    pausems(200); //wait 1/5 second for sync to start up
  }
	
  QString hashkey, searchterm, searchjail;
  QStringList pkglist;
  int searchmin, searchfilter;
  bool sortnames = false;
  //qDebug() << "Request:" << request << request.length();
  //Determine the internal hash key for the particular request
  if(request.length()==1){
    if(request[0]=="help"){ return fetchHelpInfo().join(LISTDELIMITER); }
    if(request[0]=="startsync"){ 
      if( kickoffSync() ){
	writeToLog("User Sync Request...");
      }
      return "Starting Sync...";
    }
    else if(request[0]=="hasupdates"){ hashkey = "System/hasUpdates"; }
    else if(request[0]=="needsreboot"){ return (QFile::exists(REBOOT_FLAG) ? "true": "false"); }
    else if(request[0]=="isupdating"){ return ( (QProcess::execute(UPDATE_FLAG_CHECK)==0) ? "true": "false"); }
    else if(request[0]=="updatelog"){ hashkey = "System/updateLog"; }
    else if(request[0]=="hasmajorupdates"){ hashkey = "System/hasMajorUpdates"; }
    else if(request[0]=="majorupdatelog"){ hashkey = "System/majorUpdateDetails"; }
    else if(request[0]=="hassecurityupdates"){ hashkey = "System/hasSecurityUpdates"; }
    else if(request[0]=="securityupdatelog"){ hashkey = "System/securityUpdateDetails"; }
    else if(request[0]=="haspcbsdupdates"){ hashkey = "System/hasPCBSDUpdates"; }
    else if(request[0]=="pcbsdupdatelog"){ hashkey = "System/pcbsdUpdateDetails"; }
    
  }else if(request.length()>1 && request[0]=="cage-summary"){
    //Simplification routine - assemble the inputs
    pkglist = request.mid(1); //elements 1+ are cages
    hashkey="PBI/CAGES/"; //just to cause the request to wait for sync to finish if needed
    
  }else if(request.length()==2){
    if(request[0]=="help"){ return fetchHelpInfo(request[1]).join(LISTDELIMITER); }
    if(request[0]=="jail"){
      if(request[1]=="list"){ hashkey = "JailList"; }
      else if(request[1]=="stoppedcages"){ hashkey = "JailCages"; }
      else if(request[1]=="runningcages"){ hashkey = "JailCagesRunning"; }
      else if(request[1]=="stoppedlist"){ hashkey = "StoppedJailList"; }
    }
    
  }else if(request.length()>2 && request[1]=="app-summary"){
    //Simplification routine - assemble the inputs
    pkglist = request.mid(2); //elements 2+ are pkgs
    searchjail = request[0];
    if(searchjail=="#system"){ searchjail = LOCALSYSTEM; }
    hashkey="Repos/"; //just to cause the request to wait for sync to finish if needed
    
  }else if(request.length()==3){
    if(request[0]=="jail"){
      hashkey = "Jails/"+request[1];
      if( static_cast<QStringList>(HASH->keys() ).filter(hashkey+"/").isEmpty() ){ hashkey.clear(); } //invalid jail
      else if(request[2]=="id"){ hashkey.append("/JID"); }
      else if(request[2]=="ip"){ hashkey.append("/jailIP"); }
      else if(request[2]=="path"){ hashkey.append("/jailPath"); }
      else if(request[2]=="all"){ hashkey.append("/iocage-all"); }
      else{ hashkey.append("/"+request[2]); }
    }else if(request[0]=="pkg"){
      if(request[1]=="search"){
	hashkey="Repos/"; //just to cause the request to wait for sync to finish if needed
	searchterm = request[2];
	searchjail=LOCALSYSTEM;
	searchmin = 10;
      }else if(request[1]=="#system"){ hashkey="Jails/"+LOCALSYSTEM+"/"; }
      else{ hashkey="Jails/"+request[1]+"/"; }
      if(hashkey.startsWith("Jails/")){
        if(request[2]=="installedlist"){ hashkey.append("pkgList"); sortnames=true;}
        else if(request[2]=="hasupdates"){ hashkey.append("hasUpdates"); }
        else if(request[2]=="updatemessage"){ hashkey.append("updateLog"); }
        else if(request[2]=="remotelist"){ hashkey="Repos/"+HASH->value(hashkey+"RepoID")+"/pkgList"; }
        else{ hashkey.clear(); }
      }
    }else if(request[0]=="pbi"){
      if(request[1]=="list"){
        if(request[2]=="allapps"){ hashkey="PBI/pbiList"; sortnames=true;}
	else if(request[2]=="cages"){ hashkey = "PBI/CAGES/list"; sortnames=true; }
	else if(request[2]=="graphicalapps"){ hashkey="PBI/graphicalAppList"; sortnames=true;}
	else if(request[2]=="textapps"){ hashkey="PBI/textAppList"; sortnames=true;}
	else if(request[2]=="serverapps"){ hashkey="PBI/serverAppList"; sortnames=true;}
	else if(request[2]=="allcats"){ hashkey = "PBI/catList"; }
	else if(request[2]=="graphicalcats"){ hashkey = "PBI/graphicalCatList"; }
	else if(request[2]=="textcats"){ hashkey = "PBI/textCatList"; }
	else if(request[2]=="servercats"){ hashkey = "PBI/serverCatList"; }
	else if(request[2]=="new"){ hashkey = "PBI/newappList"; sortnames=true;}
	else if(request[2]=="highlighted"){ hashkey = "PBI/highappList"; sortnames=true;}
	else if(request[2]=="recommended"){ hashkey = "PBI/recappList"; sortnames=true;}
      }else if(request[1]=="search"){
	hashkey="PBI/"; //just to cause the request to wait for sync to finish if needed
	searchterm = request[2];
	searchjail = "pbi";
	searchmin = 10;
	searchfilter=0; //all
      }		
    }
    
  }else if(request.length()==4){
    if(request[0]=="pbi"){
      if(request[1]=="app"){
        hashkey = "PBI/"+request[2]+"/"+request[3]; //pkg origin and variable
      }else if(request[1]=="cage"){
	hashkey = "PBI/CAGES/"+request[2]+"/"+request[3]; //pkg origin and variable
	//qDebug() << "Cage Request:" << hashkey;
      }else if(request[1]=="cat"){
	hashkey = "PBI/cats/"+request[2]+"/"+request[3]; //pkg origin and variable
      }else if(request[1]=="search"){
	hashkey="PBI/"; //just to cause the request to wait for sync to finish if needed
	searchterm = request[2];
	searchjail = "pbi";
	searchmin = 10;
	if(request[3]=="graphical"){ searchfilter=1; }
	else if(request[3]=="server"){ searchfilter=2;}
	else if(request[3]=="text"){ searchfilter=3;}
	else if(request[3]=="notgraphical"){ searchfilter=-1;}
	else if(request[3]=="notserver"){ searchfilter=-2;}
	else if(request[3]=="nottext"){ searchfilter=-3;}
	else{ searchfilter=0; }//all
      }
    }else if(request[0]=="pkg"){
      if(request[1]=="search"){
	hashkey="Repos/"; //just to cause the request to wait for sync to finish if needed
	searchterm = request[2];
	searchjail = request[3];
	if(searchjail=="#system"){searchjail=LOCALSYSTEM;}
	searchmin = 10;
      }
    }
    
  }else if(request.length()==5){
    if(request[0]=="pkg"){
      if(request[1]=="search"){
	hashkey="Repos/"; //just to cause the request to wait for sync to finish if needed
	searchterm = request[2];
	searchjail = request[3];
	if(searchjail=="#system"){searchjail=LOCALSYSTEM;}
	bool ok;
	searchmin = request[4].toInt(&ok);
	if(!ok){searchmin = 10;}
      }
      else if(request[1]=="#system"){ hashkey="Jails/"+LOCALSYSTEM+"/"; }
      else{ hashkey="Jails/"+request[1]+"/"; }	
      if(hashkey.startsWith("Jails/")){
        if(request[2]=="local"){
          hashkey.append("pkg/"+request[3]+"/"+request[4]); // "pkg/<origin>/<variable>"
        }else if(request[2]=="remote"){
	  hashkey="Repos/"+HASH->value(hashkey+"RepoID")+"/pkg/"+request[3]+"/"+request[4];
        }
      }
    }else if(request[0]=="pbi"){
      if(request[1]=="search"){
	hashkey="PBI/"; //just to cause the request to wait for sync to finish if needed
	searchterm = request[2];
	searchjail = "pbi";
	if(request[3]=="graphical"){ searchfilter=1; }
	else if(request[3]=="server"){ searchfilter=2;}
	else if(request[3]=="text"){ searchfilter=3;}
	else if(request[3]=="notgraphical"){ searchfilter=-1;}
	else if(request[3]=="notserver"){ searchfilter=-2;}
	else if(request[3]=="nottext"){ searchfilter=-3;}
	else{ searchfilter=0; }//all
	bool ok;
	searchmin = request[4].toInt(&ok);
	if(!ok){searchmin = 10;}
      }
    }
  }
  //qDebug() << "Request Key:" << hashkey;
  //Now fetch/return the info
  QString val;
  if(hashkey.isEmpty()){ val = "[ERROR] Invalid Information request: \""+request.join(" ")+"\""; }
  else{
    validateHash(hashkey);
    //Check if a sync is running and wait a moment until it is done
    while(isRunning(hashkey)){
	if(noncli){ return "[BUSY]"; }
	pausems(500);  //re-check every 500 ms
    }
    //Now check for info availability
    if(!searchterm.isEmpty()){
      val = doSearch(searchterm,searchjail, searchmin, searchfilter).join(LISTDELIMITER);
    }else if(!pkglist.isEmpty() && hashkey=="PBI/CAGES/"){
      val = FetchCageSummaries(pkglist).join(LINEBREAK);
      return val; //Skip the LISTDELIMITER/empty checks below - this output is highly formatted
    }else if(!pkglist.isEmpty() && !searchjail.isEmpty()){
      val = FetchAppSummaries(pkglist, searchjail).join(LINEBREAK);
      return val; //Skip the LISTDELIMITER/empty checks below - this output is highly formatted
    }else if(!HASH->contains(hashkey)){ val = "[ERROR] Information not available"; }
    else{
      val = HASH->value(hashkey,"");
      if(sortnames && !val.isEmpty()){ val = sortByName(val.split(LISTDELIMITER)).join(LISTDELIMITER); }
    }
    if(!noncli){ val.replace(LISTDELIMITER, ", "); } //for CLI requests, put lists in a comma-delimited order
    if(val.isEmpty()){ val = " "; } //make sure it has a blank space at the minimum
  }
  return val;
}

QStringList DB::fetchHelpInfo(QString subsystem){
  //Note: This help info is highly formatted for the non-CLI usage (JSON array output)
  // Every entry in the list will show up as an element in the output JSON array
  QStringList info;
  if(subsystem=="jail"){
    info << "\"jail list\": List all running jails by name";
    info << "\"jail stoppedlist\": List all stopped jails by name";
    info << "\"jail runningcages\": List all installed/running pbicages by origin and iocage ID (<origin> <ID>)";
    info << "\"jail stoppedcages\": List all installed/stopped pbicages by origin and iocage ID (<origin> <ID>)";
    info << "\"jail <jailname> <info>\": Get information about a particular jail";
    info << "Possible Info requests: ";
    info << "id: 	[RUNNING ONLY] Get the current jail ID # (JID)";
    info << "ip: 	[RUNNING ONLY] Get the current jail IP address";
    info << "path:	[RUNNING ONLY] Get the jail directory path on host system";
    info << "all:	Return the raw iocage information list for the jail (all properties)";
    info << "WID:	Get the iocage jail ID #";
    info << "tag:	Get the iocage jail tag";
    info << "installed: Get the origin of the installed pbicage";
    info << "ipv4: Get the jail ipv4 address setting";
    info << "alias-ipv4:	Get the jail ipv4 alias setting";
    info << "bridge-ipv4:	Get the jail ipv4 bridge setting";
    info << "alias-bridge-ipv4:	Get the jail ipv4 bridge setting for the alias";
    info << "defaultrouter-ipv4:	The ipv4 address setting for the default gateway";
    info << "ipv6:	Get the jail ipv6 address setting";
    info << "alias-ipv6:	Get the jail ipv6 alias setting";
    info << "bridge-ipv6:	Get the jail ipv6 bridge setting";
    info << "alias-bridge-ipv6:	Get the jail ipv6 bridge setting for the alias";
    info << "defaultrouter-ipv6:	The ipv6 address setting for the default gateway";
    info << "autostart:	[true/false] Jail is set to start automatically on boot";
    info << "vnet:	[Disabled/Enabled]";
    info << "type:	Get the type of jail [portjail, traditional, linux]";
    info << "hasupdates: [true/false] Return whether the jail has updates available";

  }else if(subsystem=="pbi"){
    info << "\"pbi list <infolist>\":";
    info << "Possible Info Lists: ";
    info << "\"allapps\":	List all applications by pkg origin";
    info << "\"serverapps\":	List all server applications by pkg origin";
    info << "\"textapps\":	List all text applications by pkg origin";
    info << "\"graphicalapps\":	List all graphical applications by pkg origin";
    info << "\"allcats\":	List all categories";
    info << "\"servercats\":	List all categories that contain a server application";
    info << "\"textcats\":	List all categories that contain a text application";
    info << "\"graphicalcats\":	List all categories that contain a graphical application";
    info << "\"cages\":		List all known cages by origin";
    info << "\"pbi app <pkg origin> <info>\":";
    info << "Possible Application Information: ";
    info << "\"author\": 	Package author";
    info << "\"category\":	Primary category where this package belongs";
    info << "\"confdir\":	Local path to configuration directory";
    info << "\"dependencies\":	List of *additional* dependencies by pkg origin";
    info << "\"origin\": 	Package/port origin";
    info << "\"plugins\":	List of optional plugins by pkg origin";
    info << "\"rating\":	Current rating (0.00 to 5.00)";
    info << "\"relatedapps\":	List of similar applications by pkg origin";
    info << "\"screenshots\":	List of screenshot URLs";
    info << "\"type\":	Primary category where this package belongs";
    info << "\"tags\":	List of search tags for application";
    info << "\"comment\":	(pkg override) Short package summary";
    info << "\"description\":	(pkg override) Full package description";
    info << "\"license\":	(pkg override) List of licences for the application";
    info << "\"maintainer\":	(pkg override) Maintainer email address";
    info << "\"name\": 	(pkg override) Package name";
    info << "\"options\":	(pkg override) List of compile-time options used";
    info << "\"website\":	(pkg override) Application website URL";
    info << "\"pbi cage <origin> <info>\":";
    info << "Possible Cage Information: ";
    info << "\"icon\":	Icon file path";
    info << "\"name\":	Name of the cage";
    info << "\"description\":	Application description";
    info << "\"arch\":	Architecture of the cage";
    info << "\"fbsdver\":	FreeBSD version used";
    info << "\"git\":	GIT path for the cage";
    info << "\"gitbranch\":	GIT granch used";
    info << "\"screenshots\":	List of screenshot URLs";
    info << "\"tags\":	List of search tags";
    info << "\"website\":	Website URL";
    info << "\"pbi cat <pkg category> <info>\":";
    info << "Possible Category Information:";
    info << "\"comment\": 	Short description of the category contents";
    info << "\"icon\":	Icon file path for the category";
    info << "\"name\":	Display name to use for the category (Ex: Desktop Utilities)";
    info << "\"origin\":	pkg name of the category (Ex: deskutils)";
    info << "\"pbi search <search term> [filter] [minimum results]\":";
    info << "Possible Filters: ";
    info << "\"all\" (default): No filtering";
    info << "\"graphical\": Only search for graphical applications";
    info << "\"server\": Only search for server applications";
    info << "\"text\": Only search for text applications";
    info << "\"notgraphical\": Do not search graphical applications";
    info << "\"notserver\": Do not search server applications";
    info << "\"nottext\": Do not search text applications";
  
  }else if(subsystem=="pkg"){
    info << "Note: <jail> = \"#system\" or name of running jail";
    info << "\"pkg <jail> remotelist\": List all available packages by origin";
    info << "\"pkg <jail> installedlist\": List all installed packages by origin";
    info << "\"pkg <jail> hasupdates\": (true/false) Package updates are available";
    info << "\"pkg <jail> updatemessage\": Full log message from the check for updates";
    info << "\"pkg search <search term> [<jail>] [minimum results]\": Perform a search and attempt to return the pkg origins for the first [minimum] results.";
    info << "\"pkg <jail> <local or remote> <pkg origin> <info>\"";
    info << "Possible <info> requests: ";
    info << "origin: 	Package/port origin";
    info << "name: 	Package name";
    info << "version:	Package version";
    info << "maintainer:	Maintainer email address";
    info << "comment:	Short package summary";
    info << "description:	Full package description";
    info << "website:	Application website URL";
    info << "size:	Human-readable package size (installed or to download)";
    info << "arch:	System architecture the package was built for";
    info << "timestamp:	[local only] Date/Time the package was installed (epoch time?)";
    info << "message:	Special message included with the package";
    info << "isOrphan:	[local only] Is the package orphaned? (true/false)";
    info << "isLocked:	[local only] Is the package version locked? (true/false)";
    info << "dependencies:	List of dependencies by pkg origin";
    info << "rdependencies:	List of reverse dependencies by pkg origin";
    info << "categories:	List of categories where this package belongs";
    info << "files:	[local only] List of files installed by this package";
    info << "options:	List of compile-time options used to build the package";
    info << "license:	List of licences the application is available under";
    info << "users:	[local-only] List of users that were created for this package";
    info << "groups:	[local-only] List of groups that were created for this package";

  }else if(subsystem=="search"){
    info << "\"<pkg | pbi> search <search term> [<pkg jail>/<pbi filter>] [result minimum]\"";
    info << "This allows the user to retrieve a list of pkg origins corresponding to the given search term.";
    info << "Default Values for optional inputs:";
    info << "<pkg jail> -> \"#system\"";
    info << "<pbi filter> -> \"all\"";
    info << "<result minimum> -> 10";
    info << "Notes: ";
    info << "1) Each search is performed case-insensitive, with the next highest search priority group added to the end of the list as long as the number of matches is less than the requested minimum.";
    info << "2) Each search priority/group is arranged alphabetically by name independently of the other groups.";
    info << "3) Each package origin will always appear in the highest priority group possible with no duplicates later in the output.";
    info << "Search matching groups/priority is: ";
        info << "1) Exact Name match";
        info << "2) Partial Name match (name begins with search term)";
        info << "3) Partial Name match (search term anywhere in name)";
        info << "4) Comment match (search term in comment for pkg)";
        info << "5) Description match (search term in description for pkg)";
    info << "Initial Filtering: ";
      info << "For packages, it always searches the entire list of available/remote packages for that particular jail";
      info << "For PBI's the possible filters are: ";
      info << "\"all\"";
      info << "\"graphical\"";
      info << "\"server\"";
      info << "\"text\"";
      info << "\"notgraphical\" (I.E. Show both server and text apps)";
      info << "\"notserver\" (I.E. Show both graphical and text apps)";
      info << "\"nottext\" (I.E. Show both graphical and server apps)";
	  
  }else{
    info << "syscache: Interface to retrieve system information from the syscache daemon based on lists of database requests.";
    info << "\"startsync\": Manually start a system information sync (usually unnecessary)";
    info << "\"needsreboot\": [true/false] See whether the system needs to reboot to finish updates";
    info << "\"isupdating\": [true/false] See whether the system is currently performing updates";
    info << "\"hasupdates\": [true/false] See whether any system updates are available";
    info << "\"updatelog\": Raw text output from the check for system updates";
    info << "\"hasmajorupdates\": [true/false] See whether major FreeBSD system updates are available";
    info << "\"majorupdatelog\": Details about the major update(s)";
    info << "\"hassecurityupdates\": [true/false] See whether FreeBSD security updates are available";
    info << "\"securityupdatelog\": Details about any security update(s)";
    info << "\"haspcbsdupdates\": [true/false] See whether any special PC-BSD hotfixes are available";
    info << "\"pcbsdupdatelog\": Details about any PC-BSD hotfixes";
    info << "\"help [jail | pkg | pbi | search]\": Information about DB requests for that subsystem";
    info << "\"<jail> app-summary <pkg origin 1>  < pkg origin 2> [etc..]\": Returns (one per pkg/line): [<pkg origin>, <name>, <version>, <icon>, <rating>, <type>, <comment>, <confdir>, <installed>, <canremove>]";
    info << "\"cage-summary <origin 1> <origin 2> [etc..]\": Returns (one per origin/line): [<origin>, <name>, <icon>, <arch>, <fbsdver>]";
  }
  return info;
}


// ========
//   PRIVATE
// ========
//Search the hash for matches
QStringList DB::doSearch(QString srch, QString jail, int findmin, int filter){
  //Filter Note: [0=all, 1=graphical, -1=!graphical, 2=server, -2=!server, 3=text, -3=!text]
  QStringList out, raw;
  QString prefix;
  if(jail.toLower()=="pbi"){
    //Get the initial list by filter
    switch(filter){
      case 1:
	raw = HASH->value("PBI/graphicalAppList").split(LISTDELIMITER);
        //qDebug() << "Search Graphical apps:" << raw.length();
        break;
      case 2:
	raw = HASH->value("PBI/serverAppList","").split(LISTDELIMITER);
        //qDebug() << "Search Server apps:" << raw.length();
        break;
      case 3:
	raw = HASH->value("PBI/textAppList","").split(LISTDELIMITER);
        //qDebug() << "Search Text apps:" << raw.length();
        break;
      case -1:
	raw = HASH->value("PBI/serverAppList","").split(LISTDELIMITER);
        raw << HASH->value("PBI/textAppList","").split(LISTDELIMITER);
        //qDebug() << "Search non-Graphical apps:" << raw.length();
        break;
      case -2:
	raw = HASH->value("PBI/graphicalAppList","").split(LISTDELIMITER);
        raw << HASH->value("PBI/textAppList","").split(LISTDELIMITER);
        //qDebug() << "Search non-Server apps:" << raw.length();
        break;
      case -3:
	raw = HASH->value("PBI/serverAppList","").split(LISTDELIMITER);
        raw << HASH->value("PBI/graphicalAppList","").split(LISTDELIMITER);
        //qDebug() << "Search non-Text apps:" << raw.length();
        break;
      default:
	raw = HASH->value("PBI/pbiList","").split(LISTDELIMITER);
        //qDebug() << "Search All apps:" << raw.length();
        break;      
    }
    prefix = "PBI/";
  }else{
    //pkg search - no type filter available
    prefix = "Repos/"+HASH->value("Jails/"+jail+"/RepoID","")+"/";
    raw = HASH->value(prefix+"pkgList","").split(LISTDELIMITER);
    prefix.append("pkg/");
    //qDebug() << "Pkg Search:" << prefix << raw.length();
  }
  //qDebug() << "Search For Term:" << srch << raw.length();
  //Now perform the search on the raw list
  if(!raw.isEmpty()){
    QStringList found;
    QStringList exact;
    // - name
    found = raw.filter("/"+srch, Qt::CaseInsensitive);
    if(!found.isEmpty()){
      //Also check for an exact name match and pull that out
      for(int i=0; i<found.length(); i++){
        if(found[i].endsWith("/"+srch, Qt::CaseInsensitive)){ exact << found.takeAt(i); i--;}
      }
      found = sortByName(found);
    }
    // - If not enough matches, also loop through and look for tag/description matches
    if( (found.length()+exact.length()) < findmin){
      QStringList words = srch.split(" ");
      QStringList tagM, sumM, descM, nameM; //tag/summary/desc/name matches
      for(int i=0; i<raw.length(); i++){
        if(exact.contains(raw[i]) || found.contains(raw[i])){ continue; } //already found
	//Also account for multiple-work searches
	//  - Names
	QString tmp = HASH->value(prefix+raw[i]+"/name","");
	if(tmp.contains(srch, Qt::CaseInsensitive)){ nameM << "100::::"+raw[i]; }
	else if(words.length()>1){
	  int wrdcount = 0;
	  for(int j=0; j<words.length(); j++){
	    if(tmp.contains(words[j], Qt::CaseInsensitive)){ wrdcount++; }
	  }
	  if(wrdcount>0){nameM << QString::number(wrdcount)+"::::"+raw[i]; }
	}
	//  - Tags
	tmp = HASH->value(prefix+raw[i]+"/tags","");
	if(tmp.contains(srch, Qt::CaseInsensitive)){ tagM << "100::::"+raw[i]; }
	else if(words.length()>1){
	  int wrdcount = 0;
	  for(int j=0; j<words.length(); j++){
	    if(tmp.contains(words[j], Qt::CaseInsensitive)){ wrdcount++; }
	  }
	  if(wrdcount>0){tagM << QString::number(wrdcount)+"::::"+raw[i]; }
	}
	//  - Comment
	tmp = HASH->value(prefix+raw[i]+"/comment","");
	if(tmp.contains(srch, Qt::CaseInsensitive)){ sumM << "100::::"+raw[i]; }
	else if(words.length()>1){
	  int wrdcount = 0;
	  for(int j=0; j<words.length(); j++){
	    if(tmp.contains(words[j], Qt::CaseInsensitive)){ wrdcount++; }
	  }
	  if(wrdcount>0){sumM << QString::number(wrdcount)+"::::"+raw[i]; }
	}
	//  - Description
	tmp = HASH->value(prefix+raw[i]+"/description","");
	if(tmp.contains(srch, Qt::CaseInsensitive)){ descM << "100::::"+raw[i]; }
	else if(words.length()>1){
	  int wrdcount = 0;
	  for(int j=0; j<words.length(); j++){
	    if(tmp.contains(words[j], Qt::CaseInsensitive)){ wrdcount++; }
	  }
	  if(wrdcount>0){descM << QString::number(wrdcount)+"::::"+raw[i]; }
	}
	/*if(HASH->value(prefix+raw[i]+"/name","").contains(srch, Qt::CaseInsensitive) ){ nameM << "100::::"+raw[i]; }
	else if(HASH->value(prefix+raw[i]+"/tags","").contains(srch, Qt::CaseInsensitive)){ tagM << "100::::"+raw[i]; }
	else if(HASH->value(prefix+raw[i]+"/comment","").contains(srch, Qt::CaseInsensitive) ){ sumM << "100::::"+raw[i]; }
	else if(HASH->value(prefix+raw[i]+"/description","").contains(srch, Qt::CaseInsensitive) ){ descM << "100::::"+raw[i]; }
	*/
      }
      // - Now add them to the found list by priority (tags > summary > description)
      found << sortByName(nameM, true);
      if( (found.length()+exact.length())<findmin){ found << sortByName(tagM, true); }
      if( (found.length()+exact.length())<findmin){ found << sortByName(sumM,true); }
      if( (found.length()+exact.length())<findmin){ found << sortByName(descM,true); }
    }
    //Sort the found list by name
    //Add the exact matches back to the top of the output list
    if(!exact.isEmpty()){ out << exact; }
    out << found;
  }
  //Make sure we don't return duplicate results
  out.removeDuplicates();
  return out;
}


//Sort a list of pkg origins by name
QStringList DB::sortByName(QStringList origins, bool haspriority){
  if(haspriority){
    //This is another recursive layer for sorting, origins should be "<priority number>::::<origin>"
    QStringList orgs = origins;
    origins.clear();
    orgs.sort();
    while( !orgs.isEmpty() ){
      QStringList priority = orgs.filter(orgs[0].section("::::",0,0)+"::::");
      for(int j=0; j<priority.length(); j++){
        orgs.removeAll(priority[j]);
	priority[j] = priority[j].section("::::",1,50); //now return it to just the origin (strip off the priority)
      }
      if(!priority.isEmpty()){
        origins << sortByName(priority, false); //now do the name sorting
      }
    }
  }else{
    QStringList names  = origins;
    for(int i=0; i<origins.length(); i++){ origins[i] = origins[i].section("/",-1)+":::"+origins[i]; }
    origins.sort();
    for(int i=0; i<origins.length(); i++){ origins[i] = origins[i].section(":::",1,1); }
  }
  return origins;
}

QStringList DB::FetchAppSummaries(QStringList pkgs, QString jail){
  //Returns (one per pkg): INFO=<pkg origin>::::<name>::::<version>::::<icon>::::<rating>::::<comment>
  //First sort out the jail info (same for all pkgs)
  QString pkgRprefix = "Repos/"+HASH->value("Jails/"+jail+"/RepoID", "")+"/pkg/"; // remote pkg prefix
  QString pkgLprefix = "Jails/"+jail+"/pkg/"; //local pkg prefix
  QStringList installed = HASH->value("Jails/"+jail+"/pkgList","").split(LISTDELIMITER); //
  //Now fill the output
  QStringList out;
//qDebug() << "Summary Request:" << pkgs << pkgRprefix << pkgLprefix;
  for(int i=0; i<pkgs.length(); i++){
    QString orig, name, ver, ico, rate, comm, type, conf, inst, canrm;
    orig = pkgs[i];
    canrm = "false";
    //Pkg Info
    if(installed.contains(pkgs[i]) ){
      //Use the locally-installed info
      name = HASH->value(pkgLprefix+orig+"/name");
      ver = HASH->value(pkgLprefix+orig+"/version");
      comm = HASH->value(pkgLprefix+orig+"/comment");
      inst = "true";
      QString rdep = HASH->value(pkgLprefix+orig+"/rdependencies","");
      if( rdep.isEmpty() ){
	canrm = "true";
      }
    }else{
      //Use the remotely-available info
      name = HASH->value(pkgRprefix+orig+"/name");
      ver = HASH->value(pkgRprefix+orig+"/version");
      comm = HASH->value(pkgRprefix+orig+"/comment");
      inst = "false";
    }
    //PBI Info
    if(HASH->contains("PBI/"+pkgs[i]+"/origin")){
      //Only overwrite the pkg name/comment if the PBI info is not empty
      QString tmp = HASH->value("PBI/"+pkgs[i]+"/name","");
      if(!tmp.isEmpty()){ name = tmp; }
      tmp = HASH->value("PBI/"+pkgs[i]+"/comment","");
      if(!tmp.isEmpty()){ comm = tmp; }
      //Icon/Rating only come from PBI info
      ico = HASH->value("PBI/"+pkgs[i]+"/icon","");
      if(!QFile::exists(ico)){ ico.clear(); } //don't output an invalid icon location
      rate = HASH->value("PBI/"+pkgs[i]+"/rating","");
      type = HASH->value("PBI/"+pkgs[i]+"/type","");
      conf = HASH->value("PBI/"+pkgs[i]+"/confdir","");
    }
    out << orig+"::::"+name+"::::"+ver+"::::"+ico+"::::"+rate+"::::"+type+"::::"+comm+"::::"+conf+"::::"+inst+"::::"+canrm;
  }
  //qDebug() << "Output:" << out;
  return out;
}

QStringList DB::FetchCageSummaries(QStringList pkgs){
  QString prefix = "PBI/CAGES/";
  QStringList out;
  for(int i=0; i<pkgs.length(); i++){
    if( HASH->contains(prefix+pkgs[i]+"/icon") ){
      QStringList info;
      //Now assemble the information (in order)
      info << pkgs[i]; //origin first
      info << HASH->value(prefix+pkgs[i]+"/name");
      info << HASH->value(prefix+pkgs[i]+"/icon");
      info << HASH->value(prefix+pkgs[i]+"/arch");
      info << HASH->value(prefix+pkgs[i]+"/fbsdver");
      out << info.join("::::");
    }
  }
  return out;
}

//Check that the DB Hash is filled for the requested field
void DB::validateHash(QString key){
  static qint64 lastCheck = 0;
  qint64 now = QDateTime::currentMSecsSinceEpoch();
  if( (now - 300000) < lastCheck){ return; } //Only check once every 5 minutes
  //Just check the overarching DB field to ensure a sync has been run successfully (and not currently running)
  // - This does not check the particular/individual field for availability
  //Only check the main fields that might be internet-connection-dependant
  QString chk = key.section("/",0,0)+"/";
  if(chk.contains("JailList")){ return; } //skip this validation for lists of jails (this *can* be empty)
  if(key.contains("/pkg/")){ chk = key.section("/pkg/",0,0)+"/pkg/"; } //Make this jail/ID specific
  if( QStringList(HASH->keys()).filter(chk).isEmpty() && !sysrun){
    writeToLog("Empty Hash Detected: Starting Sync...");
    writeToLog("Check: " + chk+ "\nKeys: " + QStringList(HASH->keys()).filter(chk).join(", ") );
    kickoffSync(); 
  }
  lastCheck = now; //save this for later
}

//Internal pause/syncing functions
bool DB::isRunning(QString key){
  if(!sysrun && !jrun){ return false; } //no sync going on - all info available
  //A sync is running - check if the current key falls into a section not finished yet
  if(key.startsWith("Jails/")){ return locrun; } //local sync running
  else if(key.startsWith("Repos/")){ return remrun; } //remote sync running
  else if(key.startsWith("PBI/")){ return pbirun; } //pbi sync running
  else if(key.startsWith("System/")){ return false; } //system sync running 
     //Note: Don't stop for system calls because freebsd-update can take *forever* to finish.
     //  Let the system calls go through and get nothing, to let it proceed to other requests
  else{ return jrun; }
  //sysrun not used (yet)
}

void DB::pausems(int ms){
  //pause the calling function for a few milliseconds
  QTime time = QTime::currentTime().addMSecs(ms);
  int udiv = ms*100; //cut into 10 parts for checks (microseconds)
  while(QTime::currentTime() < time){
    QCoreApplication::processEvents();
     QObject().thread()->usleep(udiv);
  }
}

void DB::writeToLog(QString message){
  QFile file("/var/log/pc-syscache.log");
    if(file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append) ){
      QTextStream out(&file);
	out << message +"\n";
      file.close();
    }
}
// ============
//   PRIVATE SLOTS
// ============
void DB::watcherChange(QString change){
  //Tons of these signals while syncing
    //   - so use a QTimer to compress them all down to a single call (within a short time frame)
  bool now = false;
  //Check if this is the special flag to resync now
  if(change.startsWith("/tmp/.pcbsdflags")){
    QDir dir("/tmp/.pcbsdflags");
     QFileInfoList list = dir.entryInfoList(QStringList() << "syscache-sync-*", QDir::Files | QDir::NoDotAndDotDot, QDir::Time);
     QDateTime ctime = QDateTime::currentDateTime().addSecs(-1); // go back 1 seconds
     for(int i=0; i<list.length(); i++){
       if(list[i].created() > ctime || list[i].lastModified() > ctime){ now = true; break; }
     }
  }

  if(change.contains("/var/db/pkg") && locrun){ return; } //Local sync running - ignore these for the moment (local pkg info routine can cause pings)
  QString log = "Watcher Ping: "+change+" -> Sync "+ (now ? "Now": "in 5 Min");
  writeToLog(log);
  if(!now){
    //General pkg/system change - use the timer before resync
   if(chkTime->isActive()){ chkTime->stop(); } //reset back to full time
    chkTime->start();
  }else{
    //Special pc-systemflag change: resync now
    kickoffSync();
  }
  
}

bool DB::kickoffSync(){
  if(sysrun){ return false; } //already running a sync (sysrun is the last one to be finished)
  if( QProcess::execute(UPDATE_FLAG_CHECK)==0 ){ return false; } //in the middle of updates - no syncing
  writeToLog("Starting Sync: "+QDateTime::currentDateTime().toString(Qt::ISODate) );
  locrun = remrun = pbirun = jrun = sysrun = true; //switch all the flags to running
  //if(!syncThread->isRunning()){ syncThread->start(); } //make sure the other thread is running
  QTimer::singleShot(0,SYNC, SLOT(performSync()));
  return true;
}

void DB::jailSyncFinished(){ 
  jrun = false; 
  writeToLog(" - Jail Sync Finished:"+QDateTime::currentDateTime().toString(Qt::ISODate));
  //Also reset the list of watched jails
  QStringList jails = watcher->directories().filter("/var/db/pkg");
  jails.removeAll("/var/db/pkg"); //don't remove the local pkg dir - just the jails
  if(!jails.isEmpty()){ watcher->removePaths(jails); }
  jails = HASH->value("JailList").split(LISTDELIMITER);
  for(int i=0; i<jails.length(); i++){
    //qDebug() << "Start Watching Jail:" << jails[i];
    watcher->addPath(HASH->value("Jails/"+jails[i]+"/jailPath")+"/var/db/pkg"); //watch this jail's pkg database
  }
  //qDebug() << "Watcher paths:" << watcher->directories();
}

//****************************************
//    SYNCER CLASS
//****************************************

Syncer::Syncer(QObject *parent, QHash<QString,QString> *hash) : QObject(parent){
  HASH = hash;
  longProc = new QProcess(this);
    longProc->setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    longProc->setProcessChannelMode(QProcess::MergedChannels);   
    connect(longProc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(LongProcFinished(int, QProcess::ExitStatus)) );
  applianceMode = false;
  //Check if the system/appcafe is running in appliance mode (for FreeNAS, etc)
  QStringList chk = readFile("/usr/local/etc/appcafe.conf").filter("mode").filter("=").filter("appliance");
  for(int i=0; i<chk.length(); i++){
    //Just verify that this line is not commented out (filtering above ensures this list is extremely small or empty)
    if(chk[i].section(";",0,0).section("=",1,1).simplified()=="appliance"){ applianceMode = true; break;}
  }
}

Syncer::~Syncer(){
  stopping = true;
  if(longProc->state() != QProcess::NotRunning){
    longProc->kill();
  }
}

//===============
//   PRIVATE
//===============
//System Command functions 
QStringList Syncer::sysCmd(QString cmd){ // ensures only 1 running at a time (for things like pkg)
  //static running = false;
  //while(running){ QThread::msleep(200); } //wait until the current command finishes
  //running=true;
  QStringList out = directSysCmd(cmd);
  //running=false;
  return out;
}

QStringList Syncer::directSysCmd(QString cmd){ //run command immediately
   QProcess p;  
   //Make sure we use the system environment to properly read system variables, etc.
   p.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
   //Merge the output channels to retrieve all output possible
   p.setProcessChannelMode(QProcess::MergedChannels);
   p.start(cmd, QIODevice::ReadOnly);
   //QTimer time(this);
    //time.setSingleShot(true);
    //time.start(5000); //5 second timeout
   QString tmp;
   while(p.state()==QProcess::Starting || p.state() == QProcess::Running){
     /*if(!time.isActive()){
       p.terminate(); //hung process - kill it
     }*/
     if( !p.waitForFinished(500) ){ //1/2 second timeout check
       QString tmp2 = p.readAllStandardOutput(); //this is just any new output - not the full thing
       //qDebug() << "tmp1:" << tmp;
       //qDebug() << "tmp2:" << tmp2;
       if(tmp2.isEmpty() && tmp.simplified().endsWith("]:")){
        //Interactive prompt? kill the process.
	p.terminate(); return QStringList();
       }
       tmp.append(tmp2);
     }
     QCoreApplication::processEvents();
     if(stopping){break;}
   }
   tmp.append(p.readAllStandardOutput());
   //if(time.isActive()){ time.stop(); }
   if(stopping){ p.terminate(); return QStringList(); }
   //QString tmp = p.readAllStandardOutput();
   p.close();
   if(tmp.contains("database is locked", Qt::CaseInsensitive)){
     return directSysCmd(cmd); //try again - in case the pkg database is currently locked
   }else{
     if(tmp.endsWith("\n")){ tmp.chop(1); }
     return tmp.split("\n");
   }
}

void Syncer::UpdatePkgDB(QString jail){
  if(jail!=LOCALSYSTEM){ 
    if(HASH->value("Jails/"+jail+"/haspkg")=="true"){ directSysCmd("pkg -j "+HASH->value("Jails/"+jail+"/JID")+" update"); }
  }else{ directSysCmd("pkg update"); }
}

QStringList Syncer::readFile(QString filepath){
  QStringList out;
  QFile file(filepath);
  if(file.exists()){
    if(file.open(QIODevice::ReadOnly | QIODevice::Text)){
	QTextStream fout(&file);
	while(!fout.atEnd()){
	  out << fout.readLine();
	}
	file.close();
    }
  }
  return out;
}

//Internal Hash maintenance functions
void Syncer::clearRepo(QString repo){
  //Remove All Repo specific info
  QStringList pkeys = HASH->keys();
    pkeys = pkeys.filter("Repos/"+repo+"/");
  for(int i=0; i<pkeys.length(); i++){ HASH->remove(pkeys[i]); }	
}

void Syncer::clearJail(QString jail){
  //Remove All Jail specific info
  QStringList pkeys = HASH->keys();
    pkeys = pkeys.filter("Jails/"+jail+"/");
  for(int i=0; i<pkeys.length(); i++){ HASH->remove(pkeys[i]); }
}

void Syncer::clearLocalPkg(QString pkgprefix){
  QStringList pkeys = HASH->keys();
    pkeys = pkeys.filter(pkgprefix);
  for(int i=0; i<pkeys.length(); i++){ HASH->remove(pkeys[i]); }
}

void Syncer::clearPbi(){
  QStringList pkeys = HASH->keys();
    pkeys = pkeys.filter("PBI/");
  for(int i=0; i<pkeys.length(); i++){ HASH->remove(pkeys[i]); }	
}

bool Syncer::needsLocalSync(QString jail){
  //Checks the pkg database file for modification since the last sync
  if(applianceMode){ return false; } //never sync pkg info for appliances
  if(!HASH->contains("Jails/"+jail+"/lastSyncTimeStamp")){ return true; }
  else{
    //Previously synced - look at the DB modification time
    
    if(jail==LOCALSYSTEM){ 
      QString path = "/var/db/pkg/local.sqlite";
      qint64 mod = QFileInfo(path).lastModified().toMSecsSinceEpoch();
      qint64 stamp = HASH->value("Jails/"+jail+"/lastSyncTimeStamp","").toLongLong();
      if(mod > stamp){ return true; }//was it modified after the last sync?
      //Otherwise check if the installed pkg list if different (sometimes timestamps don't get updated properly on files)
      return (HASH->value("Jails/"+jail+"/pkgList","") != directSysCmd("pkg query -a %o").join(LISTDELIMITER) );
    }else{
      //This is inside a jail - need different method
      QString path = HASH->value("Jails/"+jail+"/jailPath","") + "/var/db/pkg/local.sqlite";
      if( (HASH->value("Jails/"+jail+"/haspkg") != "true") || !QFile::exists(path) ){ return false; }
      qint64 mod = QFileInfo(path).lastModified().toMSecsSinceEpoch();
      qint64 stamp = HASH->value("Jails/"+jail+"/lastSyncTimeStamp","").toLongLong();
      if(mod > stamp){ return true; }//was it modified after the last sync?
      //Otherwise check if the installed pkg list if different (sometimes timestamps don't get updated properly on files)
      return (HASH->value("Jails/"+jail+"/pkgList","") != directSysCmd("pkg -j "+HASH->value("Jails/"+jail+"/JID","")+" query -a %o").join(LISTDELIMITER) );
    }
  }
}

bool Syncer::needsRemoteSync(QString jail){
  if(applianceMode){ return false; } //never sync pkg info for appliances
  //Checks the pkg repo files for changes since the last sync
  if( (jail!=LOCALSYSTEM) && HASH->value("Jails/"+jail+"/haspkg") != "true" ){ return false; } //pkg not installed
  else if(!HASH->contains("Jails/"+jail+"/RepoID")){ return true; } //no repoID yet
  else if(HASH->value("Jails/"+jail+"/RepoID") != generateRepoID(jail) ){ return true; } //repoID changed
  else if( !HASH->contains("Repos/"+HASH->value("Jails/"+jail+"/RepoID")+"/lastSyncTimeStamp") ){ return true; } //Repo Never synced
  else{
    QDir pkgdb( HASH->value("Jails/"+jail+"/jailPath","")+"/var/db/pkg" );
    QFileInfoList repos = pkgdb.entryInfoList(QStringList() << "repo-*.sqlite");
    qint64 stamp = HASH->value("Repos/"+HASH->value("Jails/"+jail+"/RepoID")+"/lastSyncTimeStamp").toLongLong();
    for(int i=0; i<repos.length(); i++){
      //check each repo database for recent changes
      if(repos[i].lastModified().toMSecsSinceEpoch() > stamp){ return true; }
    }
    return false;
  }
}

bool Syncer::needsPbiSync(){
  //Check the PBI index to see if it needs to be resynced
  if(!HASH->contains("PBI/lastSyncTimeStamp")){ return true; }
  else{
    qint64 mod = QFileInfo("/var/db/pbi/index/PBI-INDEX").lastModified().toMSecsSinceEpoch();
    qint64 stamp = HASH->value("PBI/lastSyncTimeStamp").toLongLong();
    qint64 mod2 = QFileInfo("/var/db/pbi/cage-index/CAGE-INDEX").lastModified().toMSecsSinceEpoch();
    qint64 dayago = QDateTime::currentDateTime().addDays(-1).toMSecsSinceEpoch();
    return (mod > stamp || mod2 > stamp || stamp < dayago );
  }
  
}

bool Syncer::needsSysSync(){
  if(applianceMode){ return false; } //never sync freebsd-update info for appliances
  //Check how long since the last check the
  if(longProc->state() != QProcess::NotRunning){ return false; } //currently running
  if(!HASH->contains("System/lastSyncTimeStamp")){ return true; }
  else{
    qint64 stamp = HASH->value("System/lastSyncTimeStamp").toLongLong();
    qint64 dayago = QDateTime::currentDateTime().addDays(-1).toMSecsSinceEpoch();
    return ( stamp < dayago );
  }
}

QString Syncer::generateRepoID(QString jail){
  QString cmd = "pkg -v -v";
  if(jail!=LOCALSYSTEM){ cmd = "pkg -j "+HASH->value("Jails/"+jail+"/JID")+" -v -v"; }
  QStringList urls = directSysCmd(cmd).filter(" url ");
  QString ID;
  for(int i=0; i<urls.length(); i++){
    ID.append( urls[i].section(" : ",1,50).simplified() );
  }
  ID.remove("\"");
  //qDebug() << "RepoID: "<< jail << ID;
  return ID;
}

//===============
//  PRIVATE SLOTS
//===============

//General Sync Functions
void Syncer::performSync(){
  stopping = false;
  qDebug() << "Syncing system information";
  //First do the operations that can potentially lock the pkg database first, but are fast
  if(stopping){ return; }
  if(HASH->isEmpty()){
    qDebug() << " - First Run: Updating pkg repo database:" << QDateTime::currentDateTime().toString(Qt::ISODate);;
    directSysCmd("pkg update -f"); //make sure this is finished before doing anything else in the syncer
    if(stopping){ return; }
  }
  qDebug() << " - Starting Jail Sync:" << QDateTime::currentDateTime().toString(Qt::ISODate);
  syncJailInfo();
  emit finishedJails();
  qDebug() << "   - Jails done";
  if(stopping){ return; }
  qDebug() << " - Starting Local Pkg Sync";
  syncPkgLocal();
  emit finishedLocal();
  qDebug() << "   - Local done";
  if(stopping){ return; }
  //Now Load the PBI database (more useful, will not lock system usage, and is fast)
  qDebug() << " - Starting PBI Sync";
  syncPbi();
  qDebug() << "   - PBI done";
  emit finishedPBI();
  //Now do all the remote pkg info retrieval (won't lock the pkg database in 1.3.x?)
   // Note: This can take a little while
  qDebug() << " - Starting Remote Pkg Sync";
  syncPkgRemote();
  qDebug() << "   - Remote done";
  emit finishedRemote();
  if(stopping){ return; }
  //Now check for overall system updates (not done yet)
  qDebug() << " - Starting System Sync";
  syncSysStatus();
  emit finishedSystem();
  qDebug() << "  - Finished all syncs";
}

void Syncer::syncJailInfo(){
  //Get the internal list of jails
  QStringList jails = HASH->value("JailList","").split(LISTDELIMITER);
  //Now get the current list of running jails and insert individual jail info
  QStringList jinfo = directSysCmd("jls");
  QString sysver = directSysCmd("freebsd-version").join("").section("-",0,0); //remove the "-<tag>" from the end (only need the number)
  QStringList found;
  for(int i=1; i<jinfo.length() && !stopping; i++){ //skip the header line
    jinfo[i] = jinfo[i].replace("\t"," ").simplified();
  }
  if(stopping){ return; } //catch for if the daemon is stopping
  
  //Now also fetch the list of inactive jails on the system
  QStringList info = directSysCmd("iocage list"); //"warden list -v");
  QStringList inactive;
  QStringList installedcages, runningcages;
  //qDebug() << "Warden Jail Info:" << info;
  for(int i=1; i<info.length(); i++){ //first line is header (JID, UUID, BOOT, STATE, TAG)
    if(info[i].isEmpty()){ continue; }
    QString ID = info[i].section(" ",1,1,QString::SectionSkipEmpty);
    if(ID.isEmpty()){ continue; }
    QString TAG = info[i].section(" ",4,4,QString::SectionSkipEmpty);
    if(!TAG.startsWith("pbicage-") && !TAG.startsWith("pbijail-")){ continue; } //skip this jail
    QStringList tmp = directSysCmd("iocage get all "+ID);
    //qDebug() << "iocage all "+ID+":" << tmp;
    //Create the info strings possible
    QString HOST, IPV4, AIPV4, BIPV4, ABIPV4, ROUTERIPV4, IPV6, AIPV6, BIPV6, ABIPV6, ROUTERIPV6, AUTOSTART, VNET, TYPE, RELEASE;
    HOST = ID;
    jails.removeAll(HOST);
    bool isRunning = (info[i].section(" ",3,3,QString::SectionSkipEmpty).simplified() != "down");
    //qDebug() << "IoCage Jail:" << ID << isRunning;
    for(int j=0; j<tmp.length(); j++){
      //Now iterate over all the info for this single jail
      QString val = tmp[j].section(":",1,100).simplified();
      //if(tmp[j].startsWith("hostname:")){ HOST = val; }
      //qDebug() << "Line:" << tmp[j] << val;
      if(tmp[j].startsWith("ip4_addr:")){ IPV4 = val; }
      //else if(tmp[j].startsWith("alias-ipv4:")){ AIPV4 = val; }
      //else if(tmp[j].startsWith("bridge-ipv4:")){ BIPV4 = val; }
      //else if(tmp[j].startsWith("bridge-ipv4:")){ BIPV4 = val; }
      //else if(tmp[j].startsWith("alias-bridge-ipv4:")){ ABIPV4 = val; }
      else if(tmp[j].startsWith("defaultrouter:")){ ROUTERIPV4 = val; }
      else if(tmp[j].startsWith("ip6_addr:")){ IPV6 = val; }
      //else if(tmp[j].startsWith("alias-ipv6:")){ AIPV6 = val; }
      //else if(tmp[j].startsWith("bridge-ipv6:")){ BIPV6 = val; }
      //else if(tmp[j].startsWith("alias-bridge-ipv6:")){ ABIPV6 = val; }
      else if(tmp[j].startsWith("defaultrouter6:")){ ROUTERIPV6 = val; }
      else if(tmp[j].startsWith("boot:")){ AUTOSTART = (val=="off") ? "false" : "true"; }
      else if(tmp[j].startsWith("vnet:")){ VNET = val; }
      else if(tmp[j].startsWith("type:")){ TYPE = val; }
      else if(tmp[j].startsWith("release:")) {RELEASE = val; }
    }
      QString inst = TAG.section("-",1,100); //installed cage for this jail
      //Need to replace the first "-" in the tag with a "/" (category/name format, but name might have other "-" in it)
      int catdash = inst.indexOf("-");
      if(catdash>0){ inst = inst.replace(catdash,1,"/"); }
    //Now compare the jail version with the system version (jail must be same or older)
    QString shortver = RELEASE.section("-",0,0);
    bool jnewer=false;
    for(int i=0; i<=sysver.count(".") && !jnewer; i++){
      jnewer = (sysver.section(".",i,i).toInt() < shortver.section(".",i,i).toInt());
    }
    if(jnewer){ continue; } //skip this jail - newer OS version than the system supports
      
      //Save this info into the hash
    QStringList junk = jinfo.filter(ID);
    if(!junk.isEmpty()){
      //This jail is running - add extra information
      bool haspkg = QFile::exists(junk[0].section(" ",3,3)+"/usr/local/sbin/pkg-static");
      HASH->insert("Jails/"+HOST+"/JID", junk[0].section(" ",0,0));
      HASH->insert("Jails/"+HOST+"/jailIP", junk[0].section(" ",1,1));
      HASH->insert("Jails/"+HOST+"/jailPath", junk[0].section(" ",3,3));
      HASH->insert("Jails/"+HOST+"/haspkg", haspkg ? "true": "false" );
    }else{
      HASH->insert("Jails/"+HOST+"/JID", "");
      HASH->insert("Jails/"+HOST+"/jailIP", "");
      HASH->insert("Jails/"+HOST+"/jailPath", "");
      HASH->insert("Jails/"+HOST+"/haspkg", "false" );
    }
    
      QString prefix = "Jails/"+HOST+"/";
      if(!TAG.startsWith("pbicage-")){
        if(!isRunning){ inactive << HOST+" "+TAG; } //only save inactive jails - active are already taken care of
       else{ found << HOST+" "+TAG; }
      }else{
	if(isRunning){ runningcages << inst+" "+ID; }
	else{ installedcages << inst+" "+ID; }
      }
      HASH->insert(prefix+"WID", ID); //iocage ID
      HASH->insert(prefix+"tag",TAG); //iocage tag
      HASH->insert(prefix+"installed", inst); //Installed pbicage origin
      HASH->insert(prefix+"iocage-all",tmp.join("<br>") );
      HASH->insert(prefix+"ipv4", IPV4);
      HASH->insert(prefix+"alias-ipv4", AIPV4);
      HASH->insert(prefix+"bridge-ipv4", BIPV4);
      HASH->insert(prefix+"alias-bridge-ipv4", ABIPV4);
      HASH->insert(prefix+"defaultrouter-ipv4", ROUTERIPV4);
      HASH->insert(prefix+"ipv6", IPV6);
      HASH->insert(prefix+"alias-ipv6", AIPV6);
      HASH->insert(prefix+"bridge-ipv6", BIPV6);
      HASH->insert(prefix+"alias-bridge-ipv6", ABIPV6);
      HASH->insert(prefix+"defaultrouter-ipv6", IPV6);
      HASH->insert(prefix+"autostart", AUTOSTART);
      HASH->insert(prefix+"vnet", VNET);
      HASH->insert(prefix+"type", TYPE);      

      //Now check if this jail can be updated and put that into the hash as well
      // TO-DO - iocage update check command still needs to be written
      /* It will effectively be: 
	# cd <jaildir>/root
	# git remote update
	# git status -uno | grep -q "is behind"
	*/
      //Only need the return code - 0=NoUpdates
      bool hasup = (QProcess::execute("iocage update -n "+ID)!=0);
      HASH->insert(prefix+"hasupdates", (hasup ? "true": "false") );
  }
  HASH->insert("StoppedJailList",inactive.join(LISTDELIMITER));
  HASH->insert("JailList", found.join(LISTDELIMITER));
  HASH->insert("JailCages", installedcages.join(LISTDELIMITER));
  HASH->insert("JailCagesRunning", runningcages.join(LISTDELIMITER));
  //Remove any old jails from the hash (ones that no longer exist)
  for(int i=0; i<jails.length() && !stopping; i++){ //anything left over in the list
    clearJail(jails[i]); 
  }
}

void Syncer::syncPkgLocalJail(QString jail){
  if(jail.isEmpty()){ return; }
 //Sync the local pkg information
 bool LSync = needsLocalSync(jail);
 if(LSync){
  //qDebug() << "Sync local jail info:" << jail;
  QString prefix = "Jails/"+jail+"/pkg/";
  clearLocalPkg(prefix); //clear the old info from the hash
  //Format: origin, name, version, maintainer, comment, description, website, size, arch, timestamp, message, isOrphan, isLocked
  QString cmd = "pkg query -a";
  QString opt = " PKG::%o::::%n::::%v::::%m::::%c::::%e::::%w::::%sh::::%q::::%t::::%M::::%a::::%k";
  if(jail!=LOCALSYSTEM){
    cmd.replace("pkg ", "pkg -j "+HASH->value("Jails/"+jail+"/JID")+" ");
  }
  if(stopping){ return; }
  QStringList info = directSysCmd(cmd+opt).join("\n").split("PKG::");
  if(info.length()==1){
    //Error in pkg, run the update routine to catch repo changes and try again
    UpdatePkgDB(jail);
    info = directSysCmd(cmd+opt).join("\n").split("PKG::");
  }
  bool pkgerror = info.length()<2;
  QStringList installed;
  for(int i=0; i<info.length(); i++){
    QStringList line = info[i].split("::::");
    if(line.length()<13){ continue; } //incomplete line
    installed << line[0]; //add to the list of installed pkgs
    HASH->insert(prefix+line[0]+"/origin", line[0]);
    HASH->insert(prefix+line[0]+"/name", line[1]);
    HASH->insert(prefix+line[0]+"/version", line[2]);
    HASH->insert(prefix+line[0]+"/maintainer", line[3]);
    HASH->insert(prefix+line[0]+"/comment", line[4]);
    HASH->insert(prefix+line[0]+"/description", line[5].replace("\n","<br>").section("WWW: ",0,0));
    HASH->insert(prefix+line[0]+"/website", line[6]);
    HASH->insert(prefix+line[0]+"/size", line[7]);
    HASH->insert(prefix+line[0]+"/arch", line[8]);
    HASH->insert(prefix+line[0]+"/timestamp", line[9]);
    HASH->insert(prefix+line[0]+"/message", line[10]);
    if(line[11]=="1"){ HASH->insert(prefix+line[0]+"/isOrphan", "true"); }
    else{ HASH->insert(prefix+line[0]+"/isOrphan", "false"); }
    if(line[12]=="1"){ HASH->insert(prefix+line[0]+"/isLocked", "true"); }
    else{ HASH->insert(prefix+line[0]+"/isLocked", "false"); }
  }
  //Now save the list of installed pkgs
  HASH->insert("Jails/"+jail+"/pkgList", installed.join(LISTDELIMITER));
  //qDebug() << "Jail:" << jail << " Installed pkg list:" << info.length() << installed.length();
  //Now go through the pkgs and get the more complicated/detailed info
  // -- dependency list
  if(stopping){ return; }
  if(!pkgerror){
    info = directSysCmd(cmd+" %o::::%do");
    installed.clear();
    QString orig;
    for(int i=0; i<info.length(); i++){
      if(orig!=info[i].section("::::",0,0) && !orig.isEmpty()){ 
        HASH->insert(prefix+orig+"/dependencies", installed.join(LISTDELIMITER));
        installed.clear();
      }
      orig = info[i].section("::::",0,0);
      installed << info[i].section("::::",1,1);
    }
    HASH->insert(prefix+orig+"/dependencies", installed.join(LISTDELIMITER)); //make sure to save the last one too
    // -- reverse dependency list
    if(stopping){ return; }
    info = directSysCmd(cmd+" %o::::%ro");
    installed.clear();
    orig.clear();
    for(int i=0; i<info.length(); i++){
      if(orig!=info[i].section("::::",0,0) && !orig.isEmpty()){ 
        HASH->insert(prefix+orig+"/rdependencies", installed.join(LISTDELIMITER));
        installed.clear();
      }
      orig = info[i].section("::::",0,0);
      installed << info[i].section("::::",1,1);
    }
    HASH->insert(prefix+orig+"/rdependencies", installed.join(LISTDELIMITER)); //make sure to save the last one too
    // -- categories
    if(stopping){ return; }
    info = directSysCmd(cmd+" %o::::%C");
    installed.clear();
    orig.clear();
    for(int i=0; i<info.length(); i++){
      if(orig!=info[i].section("::::",0,0) && !orig.isEmpty()){ 
        HASH->insert(prefix+orig+"/categories", installed.join(LISTDELIMITER));
        installed.clear();
      }
      orig = info[i].section("::::",0,0);
      installed << info[i].section("::::",1,1);
    }
    HASH->insert(prefix+orig+"/categories", installed.join(LISTDELIMITER)); //make sure to save the last one too
    // -- files
    if(stopping){ return; }
    info = directSysCmd(cmd+" %o::::%Fp");
    installed.clear();
    orig.clear();
    for(int i=0; i<info.length(); i++){
      if(orig!=info[i].section("::::",0,0) && !orig.isEmpty()){ 
        HASH->insert(prefix+orig+"/files", installed.join(LISTDELIMITER));
        installed.clear();
      }
      orig = info[i].section("::::",0,0);
      installed << info[i].section("::::",1,1);
    }
    HASH->insert(prefix+orig+"/files", installed.join(LISTDELIMITER)); //make sure to save the last one too
    // -- options
    if(stopping){ return; }
    info = directSysCmd(cmd+" %o::::%Ok=%Ov");
    installed.clear();
    orig.clear();
    for(int i=0; i<info.length(); i++){
      if(orig!=info[i].section("::::",0,0) && !orig.isEmpty()){ 
        HASH->insert(prefix+orig+"/options", installed.join(LISTDELIMITER));
        installed.clear();
      }
      orig = info[i].section("::::",0,0);
      installed << info[i].section("::::",1,1);
    }
    HASH->insert(prefix+orig+"/options", installed.join(LISTDELIMITER)); //make sure to save the last one too  
    // -- licenses
    if(stopping){ return; }
    info = directSysCmd(cmd+" %o::::%L");
    installed.clear();
    orig.clear();
    for(int i=0; i<info.length(); i++){
      if(orig!=info[i].section("::::",0,0) && !orig.isEmpty()){ 
        HASH->insert(prefix+orig+"/license", installed.join(LISTDELIMITER));
        installed.clear();
      }
      orig = info[i].section("::::",0,0);
      installed << info[i].section("::::",1,1);
    }
    HASH->insert(prefix+orig+"/license", installed.join(LISTDELIMITER)); //make sure to save the last one too 
    // -- users
    if(stopping){ return; }
    info = directSysCmd(cmd+" %o::::%U");
    installed.clear();
    orig.clear();
    for(int i=0; i<info.length(); i++){
      if(orig!=info[i].section("::::",0,0) && !orig.isEmpty()){ 
        HASH->insert(prefix+orig+"/users", installed.join(LISTDELIMITER));
        installed.clear();
      }
      orig = info[i].section("::::",0,0);
      installed << info[i].section("::::",1,1);
    }
    HASH->insert(prefix+orig+"/users", installed.join(LISTDELIMITER)); //make sure to save the last one too
    // -- groups
    if(stopping){ return; }
    info = directSysCmd(cmd+" %o::::%G");
    installed.clear();
    orig.clear();
    for(int i=0; i<info.length(); i++){
      if(orig!=info[i].section("::::",0,0) && !orig.isEmpty()){ 
        HASH->insert(prefix+orig+"/groups", installed.join(LISTDELIMITER));
        installed.clear();
      }
      orig = info[i].section("::::",0,0);
      installed << info[i].section("::::",1,1);
    }
    HASH->insert(prefix+orig+"/groups", installed.join(LISTDELIMITER)); //make sure to save the last one too
   } //done with local pkg sync
 }
 if(needsRemoteSync(jail) || LSync){
  //qDebug() << "Sync jail pkg update availability:" << jail;
  //Now Get jail update status/info
  if(stopping){ return; }
  QString cmd = "pkg upgrade -nU";
  if(jail!=LOCALSYSTEM){ cmd = "pkg -j "+HASH->value("Jails/"+jail+"/JID")+" upgrade -nU"; }
  QString log = directSysCmd(cmd).join("<br>");
  if(log.contains("pkg update")){ 
    UpdatePkgDB(jail); //need to update pkg database - then re-run check
    log = directSysCmd(cmd).join("<br>");
  }
  HASH->insert("Jails/"+jail+"/updateLog", log);
  if(log.contains("Your packages are up to date") ||  log.contains("pkg update") ){ HASH->insert("Jails/"+jail+"/hasUpdates", "false"); }
  else{ HASH->insert("Jails/"+jail+"/hasUpdates", "true"); }
 }
  //Now stamp the current time this jail was checked
  HASH->insert("Jails/"+jail+"/lastSyncTimeStamp", QString::number(QDateTime::currentMSecsSinceEpoch()));
}


void Syncer::syncPkgLocal(){
  QStringList jails = HASH->value("JailList","").split(LISTDELIMITER);
  //Do the Local system first
  if(stopping){ return; }
  syncPkgLocalJail(LOCALSYSTEM);
  //Now do any running jails
  for(int i=0; i<jails.length(); i++){
    if(stopping){ return; }
    syncPkgLocalJail(jails[i]);
  }
}

void Syncer::syncPkgRemoteJail(QString jail){
  if(jail.isEmpty()){ return; }
  //Sync the local pkg information
  if(needsRemoteSync(jail)){
    QString repoID = generateRepoID(jail);
    //qDebug() << "Sync Remote Jail:" << jail << repoID;
    HASH->insert("Jails/"+jail+"/RepoID", repoID);
    //Now fetch remote pkg info for this repoID
    QString prefix = "Repos/"+repoID+"/pkg/";
    QString cmd = "pkg rquery -a " + PKG_REPO_FLAG;
    if(jail!=LOCALSYSTEM){ cmd = "pkg -j "+HASH->value("Jails/"+jail+"/JID")+" rquery -a " + PKG_REPO_FLAG; }
    QStringList info = directSysCmd(cmd+"PKG::%o::::%n::::%v::::%m::::%w::::%q::::%sh::::%c::::%e::::%M").join("\n").split("PKG::");
    if(info.length() < 3){
      qDebug() << "[ERROR] Remote info fetch for jail:" << jail<<"\n"<<info;
      return;
    }
    //qDebug() << "Info:" << info;
    //Format: origin, name, version, maintainer, website, arch, size, comment, description, message
    QStringList pkglist;
    clearRepo(repoID); //valid info found
    for(int i=0; i<info.length(); i++){
      QStringList pkg = info[i].split("::::");
      if(pkg.length()<9){ continue; } //invalid line
      pkglist << pkg[0];
      HASH->insert(prefix+pkg[0]+"/origin", pkg[0]);
      HASH->insert(prefix+pkg[0]+"/name", pkg[1]);
      HASH->insert(prefix+pkg[0]+"/version", pkg[2]);
      HASH->insert(prefix+pkg[0]+"/maintainer", pkg[3]);
      HASH->insert(prefix+pkg[0]+"/website", pkg[4]);
      HASH->insert(prefix+pkg[0]+"/arch", pkg[5]);
      HASH->insert(prefix+pkg[0]+"/size", pkg[6]);
      HASH->insert(prefix+pkg[0]+"/comment", pkg[7]);
      HASH->insert(prefix+pkg[0]+"/description", pkg[8].replace("\n","<br>").section("WWW: ",0,0));
      HASH->insert(prefix+pkg[0]+"/message", pkg[9]);
    }
    //Now save the list of installed pkgs
    HASH->insert("Repos/"+repoID+"/pkgList", pkglist.join(LISTDELIMITER));
    //Make sure that from now on the default command does not re-check for new repo files
    cmd = cmd.replace(" rquery -a ", " rquery -aU ");
    //Now go through the pkgs and get the more complicated/detailed info
    // -- dependency list
    if(stopping){ return; }
    info = directSysCmd(cmd+" %o::::%do");
    pkglist.clear();
    QString orig;
    for(int i=0; i<info.length(); i++){
      if(orig!=info[i].section("::::",0,0) && !orig.isEmpty()){ 
        HASH->insert(prefix+orig+"/dependencies", pkglist.join(LISTDELIMITER));
        pkglist.clear();
      }
      orig = info[i].section("::::",0,0);
      pkglist << info[i].section("::::",1,1);
    }
    HASH->insert(prefix+orig+"/dependencies", pkglist.join(LISTDELIMITER)); //make sure to save the last one too
    // -- reverse dependency list (DEACTIVATED - can take 5-10 minutes for needless info (use the installed rdependencies instead) )
    /*if(stopping){ return; }
    info = directSysCmd(cmd+" %o::::%ro");
    pkglist.clear();
    orig.clear();
    for(int i=0; i<info.length(); i++){
      if(orig!=info[i].section("::::",0,0) && !orig.isEmpty()){ 
        HASH->insert(prefix+orig+"/rdependencies", pkglist.join(LISTDELIMITER));
        pkglist.clear();
      }
      orig = info[i].section("::::",0,0);
      pkglist << info[i].section("::::",1,1);
    }
    HASH->insert(prefix+orig+"/rdependencies", pkglist.join(LISTDELIMITER)); //make sure to save the last one too
    */
    // -- categories
    if(stopping){ return; }
    info = directSysCmd(cmd+" %o::::%C");
    pkglist.clear();
    orig.clear();
    for(int i=0; i<info.length(); i++){
      if(orig!=info[i].section("::::",0,0) && !orig.isEmpty()){ 
        HASH->insert(prefix+orig+"/categories", pkglist.join(LISTDELIMITER));
        pkglist.clear();
      }
      orig = info[i].section("::::",0,0);
      pkglist << info[i].section("::::",1,1);
    }
    HASH->insert(prefix+orig+"/categories", pkglist.join(LISTDELIMITER)); //make sure to save the last one too
    // -- options
    if(stopping){ return; }
    info = directSysCmd(cmd+" %o::::%Ok=%Ov");
    pkglist.clear();
    orig.clear();
    for(int i=0; i<info.length(); i++){
      if(orig!=info[i].section("::::",0,0) && !orig.isEmpty()){ 
        HASH->insert(prefix+orig+"/options", pkglist.join(LISTDELIMITER));
        pkglist.clear();
      }
      orig = info[i].section("::::",0,0);
      pkglist << info[i].section("::::",1,1);
    }
    HASH->insert(prefix+orig+"/options", pkglist.join(LISTDELIMITER)); //make sure to save the last one too  
    // -- licenses
    if(stopping){ return; }
    info = directSysCmd(cmd+" %o::::%L");
    pkglist.clear();
    orig.clear();
    for(int i=0; i<info.length(); i++){
      if(orig!=info[i].section("::::",0,0) && !orig.isEmpty()){ 
        HASH->insert(prefix+orig+"/license", pkglist.join(LISTDELIMITER));
        pkglist.clear();
      }
      orig = info[i].section("::::",0,0);
      pkglist << info[i].section("::::",1,1);
    }
    HASH->insert(prefix+orig+"/license", pkglist.join(LISTDELIMITER)); //make sure to save the last one too 
  } //end sync of remote information
  //Update the timestamp for this repo
  HASH->insert("Repos/"+HASH->value("Jails/"+jail+"/RepoID")+"/lastSyncTimeStamp", QString::number(QDateTime::currentMSecsSinceEpoch()));
}

void Syncer::syncPkgRemote(){
  QStringList jails = HASH->value("JailList","").split(LISTDELIMITER);
  //Do the Local system first
  if(stopping){ return; }
  syncPkgRemoteJail(LOCALSYSTEM);
  //Now do any running jails
  for(int i=0; i<jails.length(); i++){
    if(stopping){ return; }
    syncPkgRemoteJail(jails[i]);
  }
}

void Syncer::syncSysStatus(){
  if(needsSysSync()){
    longProc->start("pc-updatemanager check");
  }
}

void Syncer::ParseSysStatus(QStringList info){
    //Save the raw output for later
    HASH->insert("System/updateLog", info.join("<br>"));
    //Determine the number/types of updates listed
    QStringList ups;
    QString cup;
    for(int i=0; i<info.length(); i++){
      if(cup.isEmpty() && info[i].contains("NAME: ") ){
	 //Starting a new update 
	 cup = info[i];     
      }else if(!cup.isEmpty()){
	//in the middle up an update - add the text
	cup.append("\n"+info[i]);
	if(info[i].contains("To install: ") || info[i].startsWith("Install: \"") ){ //last line of text for this update
	  ups << cup; //save the complete entry to the array
	  cup.clear(); //Clear it for the next one
	}
      }
    }
    //Now go through all the types of update and set flags appropriately
    // - Major system updates (10.0 -> 10.1 for example)
    QStringList tmp = ups.filter("TYPE: SYSTEMUPDATE");
    HASH->insert("System/hasMajorUpdates", !tmp.isEmpty() ? "true": "false" );
    HASH->insert("System/majorUpdateDetails", tmp.join("\n----------\n").replace("\n","<br>") );
    // - (Ignore package updates  - already taken care of with pkg details itself)
    tmp = ups.filter("TYPE: PKGUPDATE");
    for(int i=0; i<tmp.length(); i++){
      ups.removeAll(tmp[i]); //Remove these updates from the total list
    }
    // - Freebsd/security updates
    tmp = ups.filter("TYPE: SECURITYUPDATE");
    HASH->insert("System/hasSecurityUpdates", !tmp.isEmpty() ? "true": "false" );
    HASH->insert("System/securityUpdateDetails", tmp.join("\n----------\n").replace("\n","<br>") );
    // - PC-BSD patches
    tmp = ups.filter("TYPE: PATCH");
    HASH->insert("System/hasPCBSDUpdates", !tmp.isEmpty() ? "true": "false" );
    HASH->insert("System/pcbsdUpdateDetails", tmp.join("\n----------\n").replace("\n","<br>") );

    //Now save whether updates are available
    bool hasupdates = ups.length() > 0;
    HASH->insert("System/hasUpdates", hasupdates ? "true": "false" );

    //Now save the last time this was updated
    HASH->insert("System/lastSyncTimeStamp", QString::number(QDateTime::currentMSecsSinceEpoch()) );
}

void Syncer::syncPbi(){
  //Check the timestamp to see if it needs a re-sync
  if(needsPbiSync()){
    directSysCmd("pbi_updateindex"); //Make sure to update it
    clearPbi();
    QStringList info = readFile("/var/db/pbi/index/PBI-INDEX");
    if(info.length() < 5){ return; } //exit without saving a timestamp - did not get index
    QStringList pbilist, catlist;
    QStringList gcats, tcats, scats; //graphical/text/server categories
    QStringList gapps, tapps, sapps; //graphical/text/server apps
    for(int i=0; i<info.length(); i++){
      if(info[i].startsWith("PBI=")){
	//Application Information
	QStringList pbi = info[i].section("=",1,200).split("::::");
	//Line Format (7/30/14):
	// [port, name, +ports, author, website, license, app type, category, tags, 
	//      maintainer, shortdesc, fulldesc, screenshots, related, plugins, conf dir, options, rating]
	if(pbi.length()<18){ 
	  //qDebug() << "Invalid PBI Line:" << info[i];
	  //qDebug() << " - Length:" << pbi.length() << pbi;
		continue; } //incomplete line
	QString prefix = "PBI/"+pbi[0]+"/";
	pbilist << pbi[0]; //origin
	HASH->insert(prefix+"origin", pbi[0]);
	HASH->insert(prefix+"name", pbi[1]);
	HASH->insert(prefix+"dependencies", pbi[2].replace(",",LISTDELIMITER) );
	HASH->insert(prefix+"author", pbi[3]);
	HASH->insert(prefix+"website", pbi[4]);
	HASH->insert(prefix+"license", pbi[5].replace(",",LISTDELIMITER));
	HASH->insert(prefix+"type", pbi[6]);
	HASH->insert(prefix+"category", pbi[7]);
	HASH->insert(prefix+"tags", pbi[8].replace(",",LISTDELIMITER));
	HASH->insert(prefix+"maintainer", pbi[9]);
	HASH->insert(prefix+"comment", pbi[10].replace("<br>", " "));
	HASH->insert(prefix+"description", pbi[11].section("\nWWW: ",0,0) );
	HASH->insert(prefix+"screenshots", pbi[12].replace(",",LISTDELIMITER));
	HASH->insert(prefix+"relatedapps", pbi[13].replace(",",LISTDELIMITER));
	HASH->insert(prefix+"plugins", pbi[14].replace(",",LISTDELIMITER));
	HASH->insert(prefix+"confdir", "/var/db/pbi/index/"+pbi[15]);
	HASH->insert(prefix+"options", pbi[16].replace(",",LISTDELIMITER));
	HASH->insert(prefix+"rating", pbi[17]);
	HASH->insert(prefix+"icon", "/var/db/pbi/index/"+pbi[0]+"/icon.png");
	//Keep track of which category this type falls into
	if(pbi[6].toLower()=="graphical"){ gcats << pbi[0].section("/",0,0); gapps << pbi[0]; }
	else if(pbi[6].toLower()=="server"){ scats << pbi[0].section("/",0,0); sapps << pbi[0]; }
	else{ tcats << pbi[0].section("/",0,0); tapps << pbi[0]; }
	
      }else if(info[i].startsWith("Cat=")){
	//Category Information
	QStringList cat = info[i].section("=",1,50).split("::::");
	//Line Format (7/30/14): <name>, <icon>, <summary>, <freebsd category>
	if(cat.length() < 4){ continue; } //incomplete line
	QString prefix = "PBI/cats/"+cat[3]+"/";
	catlist << cat[3]; //freebsd category (origin)
	HASH->insert(prefix+"origin", cat[3]);
	HASH->insert(prefix+"name", cat[0]);
	HASH->insert(prefix+"icon", "/var/db/pbi/index/PBI-cat-icons/"+cat[1]);
	HASH->insert(prefix+"comment", cat[2]);
      }
      //Don't use the PKG= lines, since we already have the full pkg info available
    } //finished  with index lines
    //Insert the complete lists
    HASH->insert("PBI/pbiList", pbilist.join(LISTDELIMITER));
    HASH->insert("PBI/catList", catlist.join(LISTDELIMITER));
    //Now setup the category lists
    gcats.removeDuplicates(); gcats.sort();
    tcats.removeDuplicates(); tcats.sort();
    scats.removeDuplicates(); scats.sort();
    HASH->insert("PBI/graphicalCatList",gcats.join(LISTDELIMITER));
    HASH->insert("PBI/textCatList",tcats.join(LISTDELIMITER));
    HASH->insert("PBI/serverCatList",scats.join(LISTDELIMITER));
    HASH->insert("PBI/graphicalAppList",gapps.join(LISTDELIMITER));
    HASH->insert("PBI/textAppList",tapps.join(LISTDELIMITER));
    HASH->insert("PBI/serverAppList",sapps.join(LISTDELIMITER));
    //Now read/save the appcafe info as well
    info = readFile("/var/db/pbi/index/AppCafe-index");
    QStringList newapps, highapps, recapps;
    for(int i=0; i<info.length(); i++){
      //Current syntax (7/30/14): <type>=<pkg origin>::::
      if(info[i].startsWith("New=")){
        newapps << info[i].section("=",1,50).section("::::",0,0);
      }else if(info[i].startsWith("Highlight=")){
	highapps << info[i].section("=",1,50).section("::::",0,0);      
      }else if(info[i].startsWith("Recommended=")){
	recapps << info[i].section("=",1,50).section("::::",0,0);
      }
    }
    //Insert the complete lists
    HASH->insert("PBI/newappList", newapps.join(LISTDELIMITER));
    HASH->insert("PBI/highappList", highapps.join(LISTDELIMITER));
    HASH->insert("PBI/recappList", recapps.join(LISTDELIMITER));
    
    //Now get all the info from pbi-cages
    QString cprefix = "/var/db/pbi/cage-index/";
    QStringList cages = readFile(cprefix+"CAGE-INDEX");
    QStringList allcages;
    for(int i=0; i<cages.length(); i++){
      if( !QFile::exists(cprefix+cages[i]+"/MANIFEST.json")){ continue; }
      allcages << cages[i];
      //QString cinfo = readFile(cprefix+cages[i]+"/MANIFEST.json").join("\n");
      //qDebug() << "Manifest File:" << cinfo;
      QJsonDocument doc = readJsonFile(cprefix+cages[i]+"/MANIFEST.json");  /*QJsonDocument::fromBinaryData(cinfo.toLocal8Bit(),QJsonDocument::BypassValidation);*/
      //qDebug() << " - JSON Document:" << doc.isObject() << doc.isArray() << doc.isEmpty();
      QStringList dockeys;
      if(doc.isObject()){ dockeys = doc.object().keys(); }
      for(int h=0; h<dockeys.length(); h++){
	QString val = doc.object().value(dockeys[h]).toString();
	//qDebug() << " - Variable/Value:" << dockeys[h] << val;
	HASH->insert("PBI/CAGES/"+cages[i]+"/"+dockeys[h], val);
	//Note: this will automatically load any variables in the manifest into syscache (lowercase)
	//Known variables (7/23/15): arch, fbsdver, git, gitbranch, name, screenshots, tags, website
	// ==== NO LINE BREAKS IN VALUES ====
      }
      //If there is a non-empty manifest - go ahead and save the raw contents
      //qDebug() << " - Cage HASH:" << "PBI/CAGES/"+cages[i]+"/manifest";
      if(!doc.isEmpty()){ HASH->insert("PBI/CAGES/"+cages[i]+"/manifest", doc.toJson(QJsonDocument::Compact) ); }
      //Now add the description/icon
      HASH->insert("PBI/CAGES/"+cages[i]+"/description", readFile(cprefix+cages[i]+"/description").join("<br>") );
      HASH->insert("PBI/CAGES/"+cages[i]+"/icon", cprefix+cages[i]+"/icon.png");
    }
    //Now save the list of all cages
    HASH->insert("PBI/CAGES/list", allcages.join(LISTDELIMITER));
    
    //Update the timestamp
    HASH->insert("PBI/lastSyncTimeStamp", QString::number(QDateTime::currentMSecsSinceEpoch()));
  }
  
}

void Syncer::LongProcFinished(int ret, QProcess::ExitStatus status){
  qDebug() << "System Status Update Finished:" << ret << status;
  QStringList info;
     QString tmp = longProc->readAllStandardOutput();
     if(tmp.endsWith("\n")){ tmp.chop(1); }
  info = tmp.split("\n");
  //Only the system status run uses this right now, but we could add parsing/usage for other syncs here later
  ParseSysStatus(info);
}
