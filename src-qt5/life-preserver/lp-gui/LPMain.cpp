#include "LPMain.h"
#include "ui_LPMain.h"
#include <unistd.h>

LPMain::LPMain(QWidget *parent) : QMainWindow(parent), ui(new Ui::LPMain){
  ui->setupUi(this); //load the Qt-designer UI file
  //Initialize the auto-refresh timer
  timer = new QTimer(this);
	timer->setSingleShot(true);
	timer->setInterval(60000); // 1 minute timer
	connect(timer, SIGNAL(timeout()), this, SLOT(updateTabs()) );
  //Initialize the system watcher
  watcher = new QFileSystemWatcher(this);
    //Make sure the lpreserver log directory exists and watch it
    if(!QFile::exists("/var/log/lpreserver")){
      qDebug() << "Creating the lpreserver log directory (/var/log/lpreserver)";
      QDir dir;
      dir.mkpath("/var/log/lpreserver");
    }
    watcher->addPath("/var/log/lpreserver/");
  WorkThread = new QThread();
  WORKER = new BackgroundWorker();
    WORKER->moveToThread(WorkThread);
    connect(this, SIGNAL(loadSnaps(LPDataset*)), WORKER, SLOT(loadSnapshotInfo(LPDataset*)) );
    WorkThread->start();
  //Initialize the waitbox pointer
  waitBox = 0;
  //Initialize the classic dialog pointer
  classicDLG = 0;
  //Create the basic/advanced view options
  viewBasic = new QRadioButton(tr("Basic"), ui->menuView);
	QWidgetAction *WABasic = new QWidgetAction(this); WABasic->setDefaultWidget(viewBasic);
	ui->menuView->addAction(WABasic);
  viewAdvanced = new QRadioButton(tr("Advanced"), ui->menuView);
	QWidgetAction *WAAdv = new QWidgetAction(this); WAAdv->setDefaultWidget(viewAdvanced);
	ui->menuView->addAction(WAAdv);
  connect(viewBasic, SIGNAL(toggled(bool)), this, SLOT(viewChanged()) );
  //Now set the default view type
  settings = new QSettings(QSettings::UserScope, "PC-BSD", "Life-Preserver-GUI", this);
  bool basicMode = settings->value("viewmode", true).toBool(); //basic by default
  if(basicMode){ viewBasic->setChecked(true); } //will automatically call the "viewChanged" function
  else{ viewAdvanced->setChecked(true); } //will automatically call the "viewChanged" function
  //Create the filesystem model and tie it to the treewidget
  fsModel = new QFileSystemModel(this);
	fsModel->setReadOnly(true);
	ui->treeView->setModel(fsModel);
  //Connect the UI to all the functions
  connect(ui->tool_refresh, SIGNAL(clicked()), this, SLOT(updatePoolList()) );
  connect(ui->combo_pools, SIGNAL(currentIndexChanged(int)), this, SLOT(updateTabs()) );
  connect(ui->combo_datasets, SIGNAL(currentIndexChanged(int)), this, SLOT(updateDataset()) );
  connect(ui->slider_snapshots, SIGNAL(valueChanged(int)), this, SLOT(updateSnapshot()) );
  connect(ui->push_prevsnap, SIGNAL(clicked()), this, SLOT(prevSnapshot()) );
  connect(ui->push_nextsnap, SIGNAL(clicked()), this, SLOT(nextSnapshot()) );
  connect(ui->check_hidden, SIGNAL(stateChanged(int)), this, SLOT(setFileVisibility()) );
  connect(ui->push_restore, SIGNAL(clicked()), this, SLOT(restoreFiles()) );
  connect(ui->push_configure, SIGNAL(clicked()), this, SLOT(openConfigGUI()) );
  //Connect the Menu buttons
  connect(ui->menuManage_Pool, SIGNAL(triggered(QAction*)), this, SLOT(menuAddPool(QAction*)) );
  connect(ui->menuUnmanage_Pool, SIGNAL(triggered(QAction*)), this, SLOT(menuRemovePool(QAction*)) );
  //connect(ui->menuEnable_Offsite_Backups, SIGNAL(triggered(QAction*)), this, SLOT(menuSetupISCSI(QAction*)) );
  connect(ui->action_SaveKeyToUSB, SIGNAL(triggered()), this, SLOT(menuSaveSSHKey()) );
  connect(ui->actionClose_Window, SIGNAL(triggered()), this, SLOT(menuCloseWindow()) );
  connect(ui->menuCompress_Home_Dir, SIGNAL(triggered(QAction*)), this, SLOT(menuCompressHomeDir(QAction*)) );
  connect(ui->actionExtract_Home_Dir, SIGNAL(triggered()), this, SLOT(menuExtractHomeDir()) );
  //connect(ui->actionAdd_Disk, SIGNAL(triggered()), this, SLOT(menuAddDisk()) );
  //connect(ui->menuRemove_Disk, SIGNAL(triggered(QAction*)), this, SLOT(menuRemoveDisk(QAction*)) );
  //connect(ui->menuSet_Disk_Offline, SIGNAL(triggered(QAction*)), this, SLOT(menuOfflineDisk(QAction*)) );
  //connect(ui->menuSet_Disk_Online, SIGNAL(triggered(QAction*)), this, SLOT(menuOnlineDisk(QAction*)) );
  connect(ui->action_startScrub, SIGNAL(triggered()), this, SLOT(menuStartScrub()) );
  connect(ui->action_stopScrub, SIGNAL(triggered()), this, SLOT(menuStopScrub()) );
  connect(ui->action_newSnapshot, SIGNAL(triggered()), this, SLOT(menuNewSnapshot()) );
  connect(ui->menuDelete_Snapshot, SIGNAL(triggered(QAction*)), this, SLOT(menuRemoveSnapshot(QAction*)) );
  connect(ui->menuStart_Replication, SIGNAL(triggered(QAction*)), this, SLOT(menuStartReplication(QAction*)) );
  connect(ui->menuInit_Replications, SIGNAL(triggered(QAction*)), this, SLOT(menuInitReplication(QAction*)) );
  connect(ui->menuReset_Replication_Password, SIGNAL(triggered(QAction*)), this, SLOT(menuResetReplicationPassword(QAction*)) );
  //Update the interface
  QTimer::singleShot(0,this,SLOT(updatePoolList()) );
  
  //Make sure the status tab is shown initially
  ui->tabWidget->setCurrentWidget(ui->tab_status);
  //Now connect the watcher to the update slot
  connect(watcher, SIGNAL(directoryChanged(QString)), this, SLOT(autoRefresh()) );
  //Connect the worker process to the update routine
  connect(WORKER, SIGNAL(SnapshotsLoaded()), this, SLOT(updateSnapshots()) );
}

LPMain::~LPMain(){
  WorkThread->exit(0);
  delete WorkThread;
}

// ==============
//      PUBLIC SLOTS
// ==============
void LPMain::slotSingleInstance(){
  this->raise();
  this->show();
}

// ==============
//          PRIVATE
// ==============
void LPMain::showErrorDialog(QString title, QString message, QString errors){
  QMessageBox MB(QMessageBox::Warning, title, message, QMessageBox::Ok, this);
    MB.setDetailedText(errors);
    MB.exec();
}

void LPMain::showWaitBox(QString message){
  if(waitBox == 0){
    qDebug() << "New Wait Box";
    waitBox = new QMessageBox(QMessageBox::NoIcon, tr("Please Wait"), message, QMessageBox::NoButton, this);
    waitBox->setStandardButtons(QMessageBox::NoButton);
    waitBox->setWindowModality(Qt::WindowModal);
  }else{
    qDebug() << "Update Wait Box:" << message;
    try{
      waitBox->setText(message);
    }catch(...){
      waitBox = 0; //reset flag if necessary
      showWaitBox(message);
      return;
    }
  }
  if(!waitBox->isVisible()){ waitBox->show(); waitBox->raise(); }
  QCoreApplication::processEvents();
}

void LPMain::hideWaitBox(){
  if(waitBox != 0){
    if(waitBox->isVisible()){ waitBox->hide(); }
  }
	
}

// ==============
//     PRIVATE SLOTS
// ==============
void LPMain::updatePoolList(){
  //Get the currently selected pool (if there is one)
  qDebug() << "Update Pool List";
  QString cPool;
  QStringList cpoolList;
  if(ui->combo_pools->currentIndex() != -1){ cPool = ui->combo_pools->currentText(); }
  for(int i=0; i<ui->combo_pools->count(); i++){
    cpoolList << ui->combo_pools->itemText(i);
  }
  //Get the list of managed pools
  qDebug() << "[DEBUG] Fetching list of pools";
  QStringList pools = LPBackend::listDatasets();
  QStringList poolsAvail = LPBackend::listPossibleDatasets();
  for(int i=0; i<pools.length(); i++){
    if(!cpoolList.contains(pools[i])){ cPool = pools[i]; break; } //new managed pool, activate this one instead
  }
  //Now put the lists into the UI
  poolSelected = false; //disable for the moment while changing lists
  ui->combo_pools->clear();
  if(!pools.isEmpty()){ ui->combo_pools->addItems(pools); }
  //Now set the currently selected pools
  qDebug() << "[DEBUG] Pool list:" << pools;
  if(pools.length() > 0){
    poolSelected=true;	  
    int index = pools.indexOf(cPool);
    if(index < 0){ ui->combo_pools->setCurrentIndex(0); }
    else{ ui->combo_pools->setCurrentIndex(index); }
  }else{
    //No managed pools
    poolSelected=false;
    ui->combo_pools->addItem("No Managed Pools!");
    ui->combo_pools->setCurrentIndex(0);
    //Reset to Basic View
    viewBasic->setChecked(true);
  }
  qDebug() << "[DEBUG] Update pool menu options";
  //Now update the add/remove pool menu's
  ui->menuManage_Pool->clear();
  for( int i=0; i<poolsAvail.length(); i++){
    if(pools.contains(poolsAvail[i])){ continue; } //already managed
    ui->menuManage_Pool->addAction(poolsAvail[i]);
  }
  ui->menuManage_Pool->setEnabled( !ui->menuManage_Pool->isEmpty() );
  ui->menuUnmanage_Pool->clear();
  //ui->menuEnable_Offsite_Backups->clear();
  for( int i=0; i<pools.length(); i++){
    ui->menuUnmanage_Pool->addAction(pools[i]);
    //ui->menuEnable_Offsite_Backups->addAction(pools[i]);
  }
  ui->menuUnmanage_Pool->setEnabled( !ui->menuUnmanage_Pool->isEmpty() );
  //ui->menuEnable_Offsite_Backups->setEnabled( !ui->menuEnable_Offsite_Backups->isEmpty() );
  qDebug() << "[DEBUG] Update user menus";
  //Now update the user's that are available for home-dir packaging
  QDir hdir("/usr/home");
  QStringList users = hdir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
  ui->menuCompress_Home_Dir->clear();
  for(int i=0; i<users.length(); i++){
    ui->menuCompress_Home_Dir->addAction(users[i]);
  }
  //Now update the interface appropriately
  ui->combo_pools->setEnabled(poolSelected);
  qDebug() << "[DEBUG] Finished updatePoolList()";
  updateTabs();
}

void LPMain::viewChanged(){
  ui->menuView->hide();
  ui->menubar->clear();
  settings->setValue("viewmode", viewBasic->isChecked()); //save value for later
  if(viewBasic->isChecked()){
    ui->menubar->addMenu(ui->menuFile);
    ui->menubar->addMenu(ui->menuView);
    ui->menubar->addMenu(ui->menuClassic_Backups);
  }else{
    ui->menubar->addMenu(ui->menuFile);
    ui->menubar->addMenu(ui->menuView);
    ui->menubar->addMenu(ui->menuClassic_Backups);
    ui->menubar->addMenu(ui->menuSnapshots);
    ui->menubar->addMenu(ui->menuDisks);
  }
}

void LPMain::updateTabs(){
  static bool updating = false;
  if(updating){ return; } //prevent double-taps on this function
  updating = true;
  QApplication::processEvents();
  //qDebug() << "Update Tabs" << poolSelected;
  qDebug() << "[DEBUG] start updateTabs():" << poolSelected;
  viewChanged();
  ui->tabWidget->setCurrentWidget(ui->tab_status);
  ui->tabWidget->setEnabled(poolSelected);
  ui->menuView->setEnabled(poolSelected);	
  ui->menuDisks->setEnabled(poolSelected); 
  ui->menuSnapshots->setEnabled(poolSelected);
  ui->push_configure->setVisible(poolSelected);
  ui->action_SaveKeyToUSB->setEnabled(poolSelected);
  //No Pool selected (yet)
    ui->label_numdisks->clear();
    ui->label_latestsnapshot->clear();
    ui->label_status->clear();
	  ui->label_errorstat->setVisible(false);
	  ui->label_runningstat->setVisible(false);
	  ui->label_finishedstat->setVisible(false);
  if(poolSelected){
    showWaitBox(tr("Loading Information"));
    qDebug() << "[DEBUG] loadPoolData:" << ui->combo_pools->currentText();
    POOLDATA = LPGUtils::loadPoolData(ui->combo_pools->currentText());
    qDebug() << "[DEBUG] loaded data";
    hideWaitBox();
    //Now list the status information
    ui->label_status->setText(POOLDATA.poolStatus);
    ui->label_numdisks->setText( QString::number(POOLDATA.harddisks.length()) );
    ui->label_latestsnapshot->setText(POOLDATA.latestSnapshot);
    qDebug() << "[DEBUG] Latest Snapshot:" << POOLDATA.latestSnapshot;
    if(POOLDATA.finishedStatus.isEmpty()){ ui->label_finishedstat->setVisible(false); }
    else{
      ui->label_finishedstat->setText(POOLDATA.finishedStatus);
      ui->label_finishedstat->setVisible(true);
    }
    if(POOLDATA.runningStatus.isEmpty()){ 
      ui->label_runningstat->setVisible(false);
      ui->action_startScrub->setEnabled(true);
      ui->action_stopScrub->setEnabled(false);
    }else{
      ui->label_runningstat->setText(POOLDATA.runningStatus);
      ui->label_runningstat->setVisible(true);
      ui->action_startScrub->setEnabled(false); //Something already running
      ui->action_stopScrub->setEnabled(POOLDATA.runningStatus.contains("scrub", Qt::CaseInsensitive));
    }	    
    if(POOLDATA.errorStatus.isEmpty()){ ui->label_errorstat->setVisible(false); }
    else{
      ui->label_errorstat->setText(POOLDATA.errorStatus);
      ui->label_errorstat->setVisible(true);
    }
    
    //Update the replication/disk menus
    QStringList repHosts = POOLDATA.repHost;
    ui->menuStart_Replication->clear();
    ui->menuInit_Replications->clear();
    ui->menuReset_Replication_Password->clear();
    for(int i=0; i<repHosts.length(); i++){
      ui->menuStart_Replication->addAction( repHosts[i] );
      ui->menuInit_Replications->addAction( repHosts[i] );
      ui->menuReset_Replication_Password->addAction( repHosts[i] );
    }
    ui->menuStart_Replication->setEnabled( !ui->menuStart_Replication->isEmpty() );
    ui->menuInit_Replications->setEnabled( !ui->menuInit_Replications->isEmpty() );
    ui->menuReset_Replication_Password->setEnabled( !ui->menuReset_Replication_Password->isEmpty() );
    //Now update the disk menu items
    /*ui->menuRemove_Disk->clear();
    ui->menuSet_Disk_Offline->clear();
    ui->menuSet_Disk_Online->clear();
    for(int i=0; i<POOLDATA.harddisks.length(); i++){
      ui->menuRemove_Disk->addAction(POOLDATA.harddisks[i]);
      if(POOLDATA.harddiskStatus[i] == "OFFLINE"){
        ui->menuSet_Disk_Online->addAction(POOLDATA.harddisks[i]);
      }else{
	ui->menuSet_Disk_Offline->addAction(POOLDATA.harddisks[i]);      
      }
    }
    ui->menuRemove_Disk->setEnabled(!ui->menuRemove_Disk->isEmpty());
    ui->menuSet_Disk_Offline->setEnabled(!ui->menuSet_Disk_Offline->isEmpty());
    ui->menuSet_Disk_Online->setEnabled(!ui->menuSet_Disk_Online->isEmpty());*/
    
    //Now list the data restore options
    QString cds = ui->combo_datasets->currentText();
    ui->combo_datasets->clear();
    
    ui->menuDelete_Snapshot->clear();
    
    emit loadSnaps(&POOLDATA); //kickoff the snapshot loading in the background
  }
  QApplication::processEvents();
  updating = false;
}
    
void LPMain::updateSnapshots(){
    qDebug() << "Snapshot data Available";
    QStringList dslist = POOLDATA.subsets();
    dslist.sort();
    //Now move the home directories to the top of the list
    int moved = 0;
    for(int i=0; i<dslist.length(); i++){  //make sure it stays in alphabetical order
      if(dslist[i].startsWith("/usr/home/")){
        dslist.move(i,moved);
	moved++; 
	i--; //make sure to not miss any items from moving
      }
    }
    ui->combo_datasets->addItems(dslist);
    int dsin = dslist.indexOf(cds);
    if(dsin >= 0){ ui->combo_datasets->setCurrentIndex(dsin); }
    else if( !dslist.isEmpty() ){ ui->combo_datasets->setCurrentIndex(0); }
    else{ ui->combo_datasets->addItem(tr("No datasets available")); }
    //NOTE: this automatically calls the "updateDataset()" function
    
    //Now update the snapshot removal menu list
    //QStringList snapComments;
    QStringList snaps = POOLDATA.allSnapshots(); //LPBackend::listLPSnapshots(ui->combo_pools->currentText(), snapComments);
    ui->menuDelete_Snapshot->clear();
    for(int i=0; i<snaps.length(); i++){
       QString comment = POOLDATA.snapshotComment(snaps[i]).simplified();
	if(comment.isEmpty()){ ui->menuDelete_Snapshot->addAction(snaps[i]); }
	else{ ui->menuDelete_Snapshot->addAction(snaps[i] + " (" + comment + ")" ); }
    }
    ui->menuDelete_Snapshot->setEnabled( !ui->menuDelete_Snapshot->isEmpty() );

}

void LPMain::updateDataset(){
  //Update the snapshots for the currently selected dataset
  QString cds = ui->combo_datasets->currentText();
  if(POOLDATA.subsets().indexOf(cds) >= 0){
    QStringList snaps = POOLDATA.snapshots(cds);
      qDebug() << "Update Dataset";
      ui->slider_snapshots->setEnabled(true);
      ui->slider_snapshots->setMinimum(0);
      int max = snaps.length() -1;
      if(max < 0){ max = 0; ui->slider_snapshots->setEnabled(false); }
      ui->slider_snapshots->setMaximum(max);
      ui->slider_snapshots->setValue(max); //most recent snapshot
      int interval = 1; //one tick per snapshot
      if( max > 20 ){ interval = max / 20; } //show 20 ticks
      ui->slider_snapshots->setTickInterval(interval);

      updateSnapshot();
  }else{
    ui->slider_snapshots->setEnabled(false);
    ui->label_snapshot->clear();
    ui->push_nextsnap->setEnabled(false);
    ui->push_prevsnap->setEnabled(false);
  }
	
}

void LPMain::updateSnapshot(){
  int sval = ui->slider_snapshots->value();
  QStringList snaps = POOLDATA.snapshots(ui->combo_datasets->currentText());
  //qDebug() << "Update Snapshot";
  //Update the previous/next buttons
  if(sval == ui->slider_snapshots->minimum() ){ ui->push_prevsnap->setEnabled(false); }
  else{ ui->push_prevsnap->setEnabled(true); }
  if(sval == ui->slider_snapshots->maximum() ){ ui->push_nextsnap->setEnabled(false); }
  else{ ui->push_nextsnap->setEnabled(true); }
  //Now update the snapshot viewer
  if(snaps.isEmpty()){ ui->label_snapshot->clear(); ui->slider_snapshots->setEnabled(false); }
  else{
    QString snap = snaps.at(sval);
    QString path = ui->combo_datasets->currentText() + "/.zfs/snapshot/"+snap;
    //qDebug() << "Snapshot path:" << path;
    QString comment = POOLDATA.snapshotComment(snap).simplified();
    if(comment.isEmpty()){ ui->label_snapshot->setText(snap); }
    else{ ui->label_snapshot->setText(snap+" ("+comment+")"); }
    //Now update the snapshot view
    ui->treeView->setRootIndex( fsModel->setRootPath(path) );
    
  }
}

void LPMain::nextSnapshot(){
  ui->slider_snapshots->setValue( ui->slider_snapshots->value()+1 );
}

void LPMain::prevSnapshot(){
  ui->slider_snapshots->setValue( ui->slider_snapshots->value()-1 );
}

void LPMain::setFileVisibility(){
  if(ui->check_hidden->isChecked()){
    fsModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden );
  }else{
    fsModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot );
  }
}

void LPMain::restoreFiles(){
  QModelIndexList sel = ui->treeView->selectionModel()->selectedIndexes();

  //The treeView will return one index per column/line, not one per file
  QStringList oldfiles;	
  for(int i=0; i<sel.length(); i++){ oldfiles << fsModel->filePath(sel[i]); }
  oldfiles.removeDuplicates();
  
  QStringList errors, newfiles;
  //Loop over the entire selection and revert all of them
  for(int i=0; i<oldfiles.length(); i++){
    QString filePath = oldfiles[i];
    qDebug() << " Restore file(s):" << filePath;
    QFileInfo info(filePath);	
    QString destDir = filePath;
	destDir.remove("/.zfs/snapshot/"+ui->label_snapshot->text().section("(",0,0).simplified());
	destDir.chop( filePath.section("/",-1).size()+1 ); //get rid of the filename at the end
	while(!QFile::exists(destDir)){ destDir.chop( destDir.section("/",-1).size() +1); }
    QString newFilePath = destDir+"/"+LPGUtils::generateReversionFileName(filePath, destDir);
  //qDebug() << "Destination:" << newFilePath;
  //Perform the reversion(s)
    if( info.isDir() ){
      //Is a directory
      showWaitBox( QString(tr("Restoring Directory: %1")).arg(newFilePath) );
      QStringList tmperrors = LPGUtils::revertDir(filePath, newFilePath);
      if(!tmperrors.isEmpty()){
        qDebug() << "Failed Reversions:" << tmperrors;
        errors << tmperrors;
      }
    }else{
      //Just a single file
      showWaitBox( QString(tr("Restoring file: %1")).arg(newFilePath) );
      bool ok = LPGUtils::revertFile(filePath, newFilePath);
      if( !ok ){
        qDebug() << "Failed Reversion:" << newFilePath;
	errors << newFilePath;
      }
    }	 
  } //end loop over files/dirs to revert    
  
  //Now show the message box about any errors
  hideWaitBox();
  if(errors.isEmpty()){
    showErrorDialog(tr("Reversion Error"), tr("Some file(s) could not be restored from the snapshot."), errors.join("\n") );
  }else{
    QMessageBox::information(this,tr("Restore Successful"),tr("The file(s) were succesfully restored") );
  }
}

void LPMain::openConfigGUI(){
  qDebug() << "Open Configuration UI";
  QString ds = ui->combo_pools->currentText();
  if(ds.isEmpty()){ return; }
  qDebug() << "Opening Config UI:";
  LPConfig CFG(this);
  qDebug() << " - Loading Datasets";
  CFG.loadDataset(ds, LPBackend::listReplicationTargets().contains(ds), LPBackend::listScrubs().contains(ds));
  qDebug() << " - Exec";
  CFG.exec();
  //Now check for return values and update appropriately
  bool change = false;
  if(CFG.localChanged){
    ui->statusbar->showMessage(QString(tr("Configuring dataset: %1")).arg(ds),0);
    qDebug() << "Settings up local snapshots:" << ds << "Frequency:" << CFG.localSchedule << "Total Snaps:" << CFG.localSnapshots;
    LPBackend::setupDataset(ds, CFG.localSchedule, CFG.localSnapshots);
    ui->statusbar->clearMessage();
    change = true;
  }

  if(CFG.scrubChanged){
    change = true;
    if(CFG.isScrubSched){
      ui->statusbar->showMessage(QString(tr("Configuring scrub: %1")).arg(ds),0);
      qDebug() << "Settings up scrub:" << ds << "Frequency:" << CFG.scrubSchedule << "Day:" << CFG.scrubDay << "Time:" << CFG.scrubTime;
      LPBackend::setupScrub(ds, CFG.scrubTime, CFG.scrubDay, CFG.scrubSchedule);
    }else{
      ui->statusbar->showMessage(QString(tr("Removing scrub: %1")).arg(ds),0);
      qDebug() << "Removing Scrub:" << ds;
      LPBackend::removeScrub(ds);
    }
    ui->statusbar->clearMessage();
  }

  if(CFG.remoteChanged){
    change = true;
    ui->statusbar->showMessage(QString(tr("Configuring replication settings: %1")).arg(ds),0);
    QApplication::processEvents();
      //First setup any existing/new replication hosts
      for(int i=0; i<CFG.remoteHosts.length(); i++){
        qDebug() << "Setting up Replication:" << ds << "Host:" << CFG.remoteHosts[i].host() << " Frequency:" << CFG.remoteHosts[i].freq();
	if(CFG.remoteHosts[i].dataset()=="ISCSI"){ qDebug() <<" - skipping ISCSI host"; continue; }
	if(CFG.newHosts.contains(CFG.remoteHosts[i].host())){
	  //This is a brand new host - setup the SSH key first
	  bool ok = LPBackend::setupSSHKey(CFG.remoteHosts[i].host(), CFG.remoteHosts[i].user(), CFG.remoteHosts[i].port());
	  if(!ok){ continue; } //cancelled for some reason - move on to the next host
	  else{ QMessageBox::information(this,tr("Reminder"),tr("Don't forget to save your SSH key to a USB stick so that you can restore your system from the remote host later!!")); }
	}
        LPBackend::setupReplication(ds, CFG.remoteHosts[i]);
      }
      //Now remove any old hosts
      for(int i=0; i<CFG.remHosts.length(); i++){
        ui->statusbar->showMessage(QString(tr("Removing replication: %1, Host: %2")).arg(ds, CFG.remHosts[i]),0);
	QApplication::processEvents();
        qDebug() << "Removing Replication:" << ds << "Host:" << CFG.remHosts[i];
        LPBackend::removeReplication(ds, CFG.remHosts[i]);
      }
  }
  ui->statusbar->clearMessage();

  //Now update the UI if appropriate
  if(change){
    updateTabs();
  }	
}

void LPMain::autoRefresh(){
  //This slot makes sure that the GUI is not refreshed too frequently
  if(!timer->isActive()){
    timer->start(); //start countdown to GUI refresh 
  }	  
}

// -----------------------------------------------
//   MENU SLOTS
// -----------------------------------------------
// ==== File Menu ====
void LPMain::menuAddPool(QAction *act){
  QString dataset = act->text();
  qDebug() << "Start Wizard for new managing pool:" << dataset;
  LPWizard wiz(this);
  wiz.setDataset(dataset);
  wiz.exec();
  //See if the wizard was cancelled or not
  if(!wiz.cancelled){
    ui->statusbar->showMessage(QString(tr("Enabling dataset management: %1")).arg(dataset),0);
    //run the proper commands to get the dataset enabled
    qDebug() << "Setup Snapshots:" << dataset << " Frequency:" << wiz.localTime;
    if( LPBackend::setupDataset(dataset, wiz.localTime, wiz.totalSnapshots) ){
      /*if(wiz.enableReplication){
      	 qDebug() << "Setting up replication:" << dataset << " Frequency:" << wiz.remoteTime;
	 LPBackend::setupReplication(dataset, wiz.remoteHost, wiz.remoteUser, wiz.remotePort, wiz.remoteDataset, wiz.remoteTime);     
	 QMessageBox::information(this,tr("Reminder"),tr("Don't forget to save your SSH key to a USB stick so that you can restore your system from the remote host later!!"));
      }*/
      if(wiz.enableScrub){
      qDebug() << "Settings up scrub:" << dataset << "Frequency:" << wiz.scrubSchedule << "Day:" << wiz.scrubDay << "Time:" << wiz.scrubTime;
      LPBackend::setupScrub(dataset, wiz.scrubTime, wiz.scrubDay, wiz.scrubSchedule);
      }
    }
    ui->statusbar->clearMessage();
    //Now update the list of pools
    updatePoolList();
    if(wiz.openAdvancedConfig){
      QTimer::singleShot(100,this, SLOT(openConfigGUI()) );
    }
  }	
}

void LPMain::menuRemovePool(QAction *act){
  QString ds = act->text();
  qDebug() << "Remove Pool:" << ds;
  if(!ds.isEmpty()){
    //Verify the removal of the dataset
    if( QMessageBox::Yes == QMessageBox::question(this,tr("Verify Dataset Backup Removal"),tr("Are you sure that you wish to cancel automated snapshots and/or replication of the following dataset?")+"\n\n"+ds,QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ){	    
      //verify the removal of all the snapshots for this dataset
      QStringList snapComments;
      QStringList snaps = LPBackend::listLPSnapshots(ds, snapComments);
      if(!snaps.isEmpty()){
        if( QMessageBox::Yes == QMessageBox::question(this,tr("Verify Snapshot Deletion"),tr("Do you wish to remove the local snapshots for this dataset?")+"\n"+tr("WARNING: This is a permanant change that cannot be reversed"),QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ){
	  //Remove all the snapshots
	  ui->statusbar->showMessage(QString(tr("%1: Removing snapshots")).arg(ds),0);
	  showWaitBox(tr("Removing snapshots"));
	  for(int i=0; i<snaps.length(); i++){
	    LPBackend::removeSnapshot(ds,snaps[i]);
	  }
	  ui->statusbar->clearMessage();
        }
      }
      //Remove the dataset from life-preserver management
      if(LPBackend::listReplicationTargets().contains(ds)){ 
        ui->statusbar->showMessage(QString(tr("%1: Disabling Replication")).arg(ds),0);
	showWaitBox(tr("Disabling Replication"));
	//Need the replication host(s)
	QList<LPRepHost> rhosts = LPBackend::replicationInfo(ds);
	for(int i=0; i<rhosts.length(); i++){
	  LPBackend::removeReplication(ds,rhosts[i].host()); 
	}
	ui->statusbar->clearMessage();      
      }
      if(LPBackend::listScrubs().contains(ds)){
        ui->statusbar->showMessage(QString(tr("%1: Disabling Scrubs")).arg(ds),0);
	showWaitBox(tr("Disabling Scrubs"));
	LPBackend::removeScrub(ds);
	ui->statusbar->clearMessage();
      }
      ui->statusbar->showMessage(QString(tr("%1: Disabling Life-Preserver Management")).arg(ds),0);
      showWaitBox(tr("Removing Life Preserver Schedules"));
      LPBackend::removeDataset(ds);
      ui->statusbar->clearMessage();
      updatePoolList();
      hideWaitBox();
    }
  } //end check for empty ds

}

void LPMain::menuSaveSSHKey(){
  QString ds = ui->combo_pools->currentText();	
  qDebug() << "Save SSH Key:" << ds;
  if(ds.isEmpty()){ return; }
  //Get the local hostname
  char host[1023] = "\0";
  gethostname(host,1023);
  QString localHost = QString(host).simplified();
  qDebug() << " - hostname:" << localHost;
  //Scan for mounted USB devices
  QStringList devs = LPBackend::findValidUSBDevices();
  qDebug() << " - devs:" << devs;
  if(devs.isEmpty()){
    QMessageBox::warning(this,tr("No Valid USB Devices"), tr("No valid USB devices could be found. Please mount a FAT32 formatted USB stick and try again."));
    return;
  }
  //Ask the user which one to save the file to
  bool ok;
  QString dev = QInputDialog::getItem(this, tr("Select USB Device"), tr("Available USB Devices:"), devs,0,false,&ok);	
  if(!ok or dev.isEmpty()){ return; } //cancelled
  QString devPath = dev.section("(",0,0).simplified();
  //Now copy the file over
  ok = LPBackend::copySSHKey(devPath, localHost);
  if(ok){
    QMessageBox::information(this,tr("Success"), tr("The public SSH key file was successfully copied onto the USB device."));
  }else{
    QMessageBox::information(this,tr("Failure"), tr("The public SSH key file could not be copied onto the USB device."));
  }
}

/*void LPMain::menuSetupISCSI(QAction *act){
  QString ds = act->text(); //this is the zpool
  LPISCSIWizard dlg(this, ds);
    dlg.exec();

}*/

void LPMain::menuCloseWindow(){
  this->close();
}

// ==== Classic Backups Menu ====
void LPMain::menuCompressHomeDir(QAction* act){
  QString user = act->text();
  qDebug() << "Compress Home Dir:" << user;
  //Start up the Classic Dialog
  bool recreate = false;
  if(classicDLG == 0){ recreate = true; }
  if(recreate){
    classicDLG = new LPClassic(this);
    classicDLG->setHomeDir("/usr/home/"+user);
    classicDLG->show();
  }else if(!classicDLG->running){
    classicDLG->setHomeDir("/usr/home/"+user); //move to the alternate user dir
  }
  classicDLG->raise();
  classicDLG->show();
  /*
  //Prompt for the package name
  QString pkgName = user+"-"+QDateTime::currentDateTime().toString("yyyyMMdd-hhmm");
  bool ok;
  pkgName = QInputDialog::getText(this, tr("Package Name"), tr("Name of the package to create:"), QLineEdit::Normal, pkgName, &ok);
  if(!ok || pkgName.isEmpty() ){ return; } //cancelled
  //Now create the package
  showWaitBox(tr("Packaging home directory"));
  QString pkgPath = LPGUtils::packageHomeDir(user, pkgName);
  hideWaitBox();
  //Now inform the user of the result
  if(pkgPath.isEmpty()){
    qDebug() << "No Package created";
    QMessageBox::warning(this,tr("Package Failure"), tr("The home directory package could not be created."));
  }else{
    qDebug() << "Package created at:" << pkgPath;
    QMessageBox::information(this,tr("Package Success"), tr("The home directory package was successfully created.")+"\n\n"+pkgPath);
  }
  */
}

void LPMain::menuExtractHomeDir(){
  qDebug() << "Extract Home Dir";
  //Get the file path from the user
  QString filePath = QFileDialog::getOpenFileName(this,tr("Find Home Dir Package"), "/usr/home", tr("Home Dir Package (*.home.tar.gz)") );
  if(filePath.isEmpty() || !QFile::exists(filePath)){ return; } //cancelled
  //Now check if the user in the package is also on the system
  QString username;
  bool ok = LPGUtils::checkPackageUserPath(filePath, &username);
  if(!ok){
    QMessageBox::warning(this,tr("User Missing"),QString(tr("The user (%1) does not exist on this system. Please create this user first and then try again.")).arg(username) );
    return;
  }
  //Now extract the package
  showWaitBox(tr("Extracting Home Directory"));
  ok = LPGUtils::extractHomeDirPackage(filePath);
  hideWaitBox();
  //Now report the results
  if(ok){
    QMessageBox::information(this,tr("Package Extracted"), QString(tr("The package was successfully extracted within %1")).arg("/usr/home/"+username) );
  }else{
    QMessageBox::warning(this, tr("Package Failure"), QString(tr("The package could not be extracted within %1")).arg("/usr/home/"+username) );
  }
  
}

// ==== Disks Menu ====
/*void LPMain::menuAddDisk(){
  QString pool = ui->combo_pools->currentText();
  //Get the available disks and remove the current disks
  QStringList adisks = LPGUtils::listAvailableHardDisks();
  for(int i=0; i<POOLDATA.harddisks.length(); i++){
    adisks.removeAll( POOLDATA.harddisks[i].section("s",0,0,QString::SectionSkipEmpty) );
  }
  if(adisks.isEmpty()){
    QMessageBox::information(this,tr("Attach New Disk"), tr("No available disks could be found"));
    return;
  }
  //Find a disk that can be added to the system
  bool ok=false;
  QString disk = QInputDialog::getItem(this, tr("Attach New Disk"),tr("Detected Disks:"), adisks,0,false, &ok);
  if( !ok || disk.isEmpty() ){ return; }
  qDebug() << "Add Disk:" << disk << pool;
  showWaitBox(tr("Attaching disk"));
  ok = LPBackend::attachDisk(pool, disk);
  hideWaitBox();
  if(ok){
    QMessageBox::information(this,tr("Disk Attached"),QString(tr("Success: %1 was added to %2")).arg(disk,pool) );
    QTimer::singleShot(0,this,SLOT(updateTabs()) );
  }else{
    QMessageBox::warning(this,tr("Disk Attach Error"),QString(tr("Failure: %1 could not be attached to %2.")).arg(disk,pool) );
  }	
}

void LPMain::menuRemoveDisk(QAction *act){
  QString disk = act->text();
  QString pool = ui->combo_pools->currentText();
  //Verify action
  if(QMessageBox::Yes != QMessageBox::question(this,tr("Verify Disk Removal"),QString(tr("Are you sure that you want to remove %1 from %2?")).arg(disk,pool) + "\n\n" + tr("CAUTION: This disk can only be re-attached later as a brand new disk"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ){
    return; //cancelled
  }
  qDebug() << "Remove Disk:" << disk << pool;
  showWaitBox(tr("Detaching disk"));
  bool ok = LPBackend::detachDisk(pool, disk);
  hideWaitBox();
  if(ok){
    QMessageBox::information(this,tr("Disk Removal Success"),QString(tr("Success: %1 was removed from %2")).arg(disk, pool) );
    QTimer::singleShot(0,this,SLOT(updateTabs()) );
  }else{
    QMessageBox::warning(this,tr("Disk Removal Failure"),QString(tr("Failure: %1 could not be removed from %2 at this time.")).arg(disk, pool) );
  }
}

void LPMain::menuOfflineDisk(QAction *act){
  QString disk = act->text();
  QString pool = ui->combo_pools->currentText();
  //Verify action
  if(QMessageBox::Yes != QMessageBox::question(this,tr("Verify Disk Offline"),QString(tr("Are you sure you wish to set %1 offline?")).arg(disk), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ){
    return; //cancelled
  }
  qDebug() << "Offline Disk:" << disk << pool;
  showWaitBox(tr("Setting disk offline"));
  bool ok = LPBackend::setDiskOffline(pool, disk);
  hideWaitBox();
  if(ok){
    QMessageBox::information(this,tr("Disk Offline Success"),QString(tr("Success: %1 has been taken offline.")).arg(disk) );
    QTimer::singleShot(0,this,SLOT(updateTabs()) );
  }else{
    QMessageBox::warning(this,tr("Disk Offline Failure"),QString(tr("Failure: %1 could not be taken offline at this time.")).arg(disk) );
  }
}

void LPMain::menuOnlineDisk(QAction *act){
  QString disk = act->text();
  QString pool = ui->combo_pools->currentText();
  //Verify action
  if(QMessageBox::Yes != QMessageBox::question(this,tr("Verify Disk Online"),QString(tr("Are you sure you wish to set %1 online?")).arg(disk), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ){
    return; //cancelled
  }
  qDebug() << "Online Disk:" << disk << pool;
  showWaitBox(tr("Setting disk online"));
  bool ok = LPBackend::setDiskOnline(pool, disk);
  hideWaitBox();
  if(ok){
    QMessageBox::information(this,tr("Disk Online Success"),QString(tr("Success: %1 has been set online.")).arg(disk) );
    QTimer::singleShot(0,this,SLOT(updateTabs()) );
  }else{
    QMessageBox::warning(this,tr("Disk Online Failure"),QString(tr("Failure: %1 could not be set online at this time.")).arg(disk) );
  }
}*/

void LPMain::menuStartScrub(){
  QString pool = ui->combo_pools->currentText();
  //Verify starting a scrub
  if( QMessageBox::Yes != QMessageBox::question(this,tr("Verify Scrub"),QString(tr("Are you sure you want to start a scrub on %1?")).arg(pool) + "\n"+tr("NOTE: This may take quite a while to complete"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ){
    return; //cancelled	  
  }
  qDebug() << "Start Scrub:" << pool;
  QString cmd = "zpool scrub "+pool;
  showWaitBox(tr("Trying to start a scrub"));
  int ret = system(cmd.toUtf8());
  hideWaitBox();
  if(ret == 0){
    //Now let te user know that one has been triggered
    QMessageBox::information(this,tr("Scrub Started"),QString(tr("A scrub has just been started on %1")).arg(pool));
    QTimer::singleShot(0,this,SLOT(updateTabs()) );
  }else{
    QMessageBox::warning(this,tr("Scrub Not Started"), QString(tr("A scrub on %1 could not be started at this time.")).arg(pool) + "\n"+tr("Please wait until any current resilvering or scrubs are finished before trying again.") );
  }
}

void LPMain::menuStopScrub(){
  QString pool = ui->combo_pools->currentText();
  //Verify stopping a scrub
  if( QMessageBox::Yes != QMessageBox::question(this,tr("Verify Scrub"),QString(tr("Are you sure you want to stop the scrub on %1?")).arg(pool), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ){
    return; //cancelled	  
  }
  qDebug() << "Stop Scrub:" << pool;
  QString cmd = "zpool scrub -s "+pool;
  showWaitBox(tr("Trying to stop scrub"));
  int ret = system(cmd.toUtf8());
  hideWaitBox();
  if(ret == 0){
    //Now let te user know that one has been triggered
    QMessageBox::information(this,tr("Scrub Stopped"),QString(tr("The scrub on %1 has been stopped.")).arg(pool));
    QTimer::singleShot(0,this,SLOT(updateTabs()) );
  }else{
    QMessageBox::warning(this,tr("Scrub Not Running"), QString(tr("There was no scrub running on %1.")).arg(pool) );
  }	
}

// ==== Snapshots Menu ====
void LPMain::menuNewSnapshot(){
  qDebug() << "New Snapshot";
  QString ds = ui->combo_pools->currentText();
  if(ds.isEmpty()){return; }
  //Get the new snapshot name from the user
  bool ok;
  QString name = QInputDialog::getText(this,tr("New Snapshot Name"), tr("Snapshot Name:"), QLineEdit::Normal, tr("Name"), &ok, 0, Qt::ImhUppercaseOnly | Qt::ImhLowercaseOnly | Qt::ImhDigitsOnly );
  if(!ok || name.isEmpty()){ return; } //cancelled
  //Now get the comment
   QString comment = QInputDialog::getText (this, QObject::tr("Snapshot comment"), QObject::tr("Snapshot comment"), QLineEdit::Normal, tr("GUI Snapshot"), &ok);
   if (!ok){ comment = tr("GUI Snapshot"); }
  qDebug() << "Creating a new snapshot:" << ds << name << comment;
  //Now create the new snapshot
  LPBackend::newSnapshot(ds,name, comment);
  QMessageBox::information(this,tr("Snapshot Pending"), tr("The new snapshot creation has been added to the queue"));
  updateTabs();
}

void LPMain::menuRemoveSnapshot(QAction *act){
  QString snapshot = act->text().section(" ", 0, 0);
  QString comment = act->text().section(" ", 1, -1);
  QString pool = ui->combo_pools->currentText();
  qDebug() << "Remove Snapshot:" << snapshot;
  //verify snapshot removal
  if( QMessageBox::Yes == QMessageBox::question(this,tr("Verify Snapshot Deletion"),QString(tr("Do you wish to delete this snapshot? %1 (%2)")).arg(pool+"/"+snapshot, comment)+"\n"+tr("WARNING: This is a permanant change that cannot be reversed"),QMessageBox::Yes | QMessageBox::No, QMessageBox::No) ){
    bool ok = LPBackend::removeSnapshot(ui->combo_pools->currentText(), snapshot);
    if(ok){
      QMessageBox::information(this,tr("Snapshot Removed"),tr("The snapshot was successfully deleted"));
    }else{
      QMessageBox::information(this,tr("Snapshot Removal Failure"),tr("The snapshot removal experienced an error and it not be completed at this time."));
    }
    updateTabs();
  }
}

void LPMain::menuStartReplication(QAction* act){
  //Get the pool/host as appropriate
  QString pool = ui->combo_pools->currentText();
  QString host = act->text();
  if(host.isEmpty()){ return; } //invalid
  QString cmd = "lpreserver replicate run %1 %2";
  cmd = cmd.arg(pool, host);
  QProcess::startDetached(cmd);
  QMessageBox::information(this, tr("Replication Triggered"), tr("A replication has been queued up for this dataset"));
}

void LPMain::menuInitReplication(QAction* act){
  //Get the pool/host as appropriate
  QString pool = ui->combo_pools->currentText();
  QString host = act->text();
  if(host.isEmpty()){ return; } //invalid
  QString cmd = "lpreserver replicate init %1 %2; lpreserver replicate run %1 %2";
  cmd = cmd.arg(pool, host);
  qDebug() << "Running Command:" << cmd;
  QProcess::startDetached(cmd);
  QMessageBox::information(this, tr("Replication Triggered"), tr("A replication has been queued up for this dataset"));
}

void LPMain::menuResetReplicationPassword(QAction* act){
  QString pool = ui->combo_pools->currentText();
  QString host = act->text();
  if(host.isEmpty()){ return; } //invalid
  //Now we need to find the info about this replication host before doing anything
  QList<LPRepHost> info = LPBackend::replicationInfo(pool);
  for(int i=0; i<info.length(); i++){
    if(info[i].host()==host){
      if(info[i].dataset()=="ISCSI"){
	QMessageBox::warning(this,tr("Invalid Target"),tr("This is an ISCSI replication target and does not use an SSH password."));
      }else{
        bool ok = LPBackend::setupSSHKey(info[i].host(), info[i].user(), info[i].port());
        if(ok){ QMessageBox::information(this,tr("Reminder"),tr("Don't forget to save your SSH key to a USB stick so that you can restore your system from the remote host later!!")); }	      
      }	    
      return;
    }
  }
}