#include "mainUI.h"
#include "ui_mainUI.h"

#include "pkgVulDialog.h"
#include "updHistoryDialog.h"
#include "eolDialog.h"
#include "branchesDialog.h"

#include <QProcess>
#include <QTimer>
#include <QMessageBox>
#include <QScrollBar>
#include <QThread>

MainUI::MainUI() : QMainWindow(), ui(new Ui::MainUI()){
  ui->setupUi(this); //load designer file
	
  InitUI(); //initialize the UI elements
	
  //Connect signals/slots
  connect(ui->push_close, SIGNAL(clicked()), this, SLOT(CloseUI()) );
  connect(ui->tool_start, SIGNAL(clicked()), this, SLOT(startUpdates()) );
  connect(ui->combo_updates, SIGNAL(currentIndexChanged(int)), this, SLOT(updateSelChange()) );
  connect(ui->group_details, SIGNAL(toggled(bool)), this, SLOT(updateDetailsChange()) );
  connect(ui->list_patches, SIGNAL(itemSelectionChanged()), this, SLOT(patchSelChange()) );
  connect(ui->tool_start_patches, SIGNAL(clicked()), this, SLOT(startPatches()) );
  connect(ui->combo_autosetting, SIGNAL(currentIndexChanged(int)), this, SLOT(autoUpChange()) );
  connect(watcher, SIGNAL(fileChanged(QString)), this, SLOT(watcherChange(QString)) );
  connect(watcher, SIGNAL(directoryChanged(QString)), this, SLOT(watcherChange(QString)) );
}

MainUI::~MainUI(){
	
}

void MainUI::slotSingleInstance(){
  this->raise();
  this->show();
}

void MainUI::InitUI(){ //initialize the UI (widgets, options, menus, current values)
  //Initialize the log file watcher
  watcher = new QFileSystemWatcher(this);
    watcher->addPath(UPDATE_LOG_FILE);
    watcher->addPath("/tmp/.pcbsdflags");
	
  ui->label_sysinfo->setText("");
  ui->tabWidget->setCurrentIndex(0); 
  //Create/set the list of auto-update options	
  QString AUval = pcbsd::Utils::getValFromPCBSDConf("AUTO_UPDATE").simplified().toLower();
  if(AUval.isEmpty()){ 
    AUval = "securitypkg"; //default value for the system
  }
  ui->combo_autosetting->clear();
  ui->combo_autosetting->addItem(tr("Everything"), "all");
  if(AUval=="all"){ ui->combo_autosetting->setCurrentIndex(0); }
  ui->combo_autosetting->addItem(tr("Security & Packages"), "securitypkg");
  if(AUval=="securitypkg"){ ui->combo_autosetting->setCurrentIndex(1); } 
  ui->combo_autosetting->addItem(tr("Security"), "security");
  if(AUval=="security"){ ui->combo_autosetting->setCurrentIndex(2); }
  ui->combo_autosetting->addItem(tr("Packages"), "pkg");
  if(AUval=="pkg"){ ui->combo_autosetting->setCurrentIndex(3); }
  ui->combo_autosetting->addItem(tr("Nothing"), "disabled");
  if(AUval=="disabled"){ ui->combo_autosetting->setCurrentIndex(4); }
  //Now show the current description
  autoUpChange(); //will also save the value to the conf file (just in case nothing set)
  
  //Make sure the details are hidden to start with
  ui->group_details->setChecked(false);
    updateDetailsChange();
  //Read the current state of the log file
  QString log = pcbsd::Utils::readTextFile(UPDATE_LOG_FILE);
  ui->text_log->setPlainText(log);
  this->setEnabled(false);
  //Now update the UI based on current system status
  UpdateUI();
}

void MainUI::ShowUpdatingNotice(){
  //Now pop up a dialog about updates starting until the update procedure is detected
  bool started = false;
  QMessageBox dlg(QMessageBox::NoIcon, tr("Please Wait"), tr("Starting update procedures..."), QMessageBox::NoButton, this, Qt::FramelessWindowHint);
  dlg.setStandardButtons(QMessageBox::NoButton); //force it again
  dlg.show();
  for(int i=0; i<200 && !started; i++){ //60 second maximum wait (200*300ms = 60000 ms)
    QApplication::processEvents();
    this->thread()->usleep(300000); //300 ms
    started = (0==QProcess::execute("pgrep -F /tmp/.updateInProgress"));
  }
  dlg.close();
  QProcess::startDetached("syscache startsync"); //re-sync the database now as well	
}

void MainUI::watcherChange(QString change){
  if(change==UPDATE_LOG_FILE){ 
    updateLogChanged();
    //Make sure the log file was not removed from the watcher for some reason
    if( !watcher->files().contains(UPDATE_LOG_FILE) ){ watcher->addPath(UPDATE_LOG_FILE); }
  }else{ UpdateUI(); }
}

void MainUI::UpdateUI(){ //refresh the entire UI , and system status structure
  //Parse the system status to determine the  updates that are available
  QStringList info = pcbsd::Utils::runShellCommand("syscache needsreboot isupdating hasmajorupdates hassecurityupdates haspcbsdupdates \"pkg #system hasupdates\"");
  if(info.length() < 6){
    if(0!=QProcess::execute("pgrep -F /tmp/.updateInProgress")){
      //Something went wrong with the update/syscache, restart syscache and try again
      qDebug() << "System Error - no update process running, and syscache is still stopped";
      qDebug() << " -- restarting syscache";
      QProcess::startDetached("service syscache start");
      QTimer::singleShot(100, this, SLOT(UpdateUI()) );
      return;
    }
  }	  
	
  if(info.length() < 6){ 
    //Is syscache not running?
    //QMessageBox::warning(this, tr("Syscache Not Running?"), tr("The syscache daemon does not appear to be running. It must be running on the system before this utility will function properly.")+"\n\n"+QString(tr("CLI Command:\"%1\" ")).arg("sudo service syscache start") );
    //Assume an update is running at the moment
    info.clear();
    info << "true" << "true" << "false" << "false" << "false" << "false";
    if(!QFile::exists("/tmp/.rebootRequired")){ info[0] = "false"; }
    if(0!=QProcess::execute("pgrep -F /tmp/.updateInProgress")){ info[1] = "false"; }
  }
  bool needreboot = (info[0]=="true");
  bool isupdating = (info[1]=="true");
  bool hasmajor = (info[2]=="true");
  bool hassec = (info[3]=="true");
  bool haspatch = (info[4]=="true");
  bool haspkg = (info[5]=="true");
  
  //Now make sure that the log file is being watched (in case it did not exist earlier)
  if(watcher->files().isEmpty()){
    watcher->addPath(UPDATE_LOG_FILE);
  }
  
  //Now Change the UI around based on the current status
  //int cindex = ui->tabWidget->currentIndex();
  // - First remove the special tabs from the tabWidget (add them as needed)
  for(int i=0; i<ui->tabWidget->count(); i++){
    if( (ui->tabWidget->widget(i)==ui->tab_updates && (needreboot || isupdating || !(hasmajor || hassec || haspkg)) ) \
	|| ( ui->tabWidget->widget(i)==ui->tab_patches && (needreboot || isupdating || !haspatch)) ){
      ui->tabWidget->removeTab(i);
      i--; //need to back up one value since the list got reduced
    }
  }
  // - Also hide the info label(s) at the top of the window by default
    //ui->label_sysinfo->setVisible(false);
  //System Needs Reboot
  if(info[0]=="true"){
    //ui->label_sysinfo->setVisible(true);
    ui->label_sysinfo->setText(tr("System restart required to finish updates!"));
    //if(cindex<0){ ui->tabWidget->setCurrentIndex(0); }
    //else{ ui->tabWidget->setCurrentIndex(cindex); }
    this->setEnabled(true);
    return; //None of the update tabs are available at the moment - stop here
  }
  //System currently doing updates
  if(info[1]=="true"){ 
    //ui->label_sysinfo->setVisible(true);
    ui->label_sysinfo->setText(tr("System currently performing background updates."));
    //if(cindex<0){ ui->tabWidget->setCurrentIndex(0); }
    //else{ ui->tabWidget->setCurrentIndex(cindex); }
    this->setEnabled(true);
    return; //None of the update tabs are available at the moment - stop here
  }
  //Now add in the extra tabs as necessary
  if( (hasmajor || hassec || haspkg) && ui->tabWidget->count()< (haspatch ? 4: 3) ){
    ui->tabWidget->insertTab(0,ui->tab_updates, tr("Updates Available"));
    ui->tabWidget->setCurrentIndex(0);
    //cindex++;
  }
  if(haspatch){
    ui->tabWidget->insertTab(0,ui->tab_patches, tr("PC-BSD Patches") );
    ui->tabWidget->setCurrentIndex(0);
    //cindex++;
  }
  
  if( hasmajor || hassec || haspkg || haspatch ){
    ui->label_sysinfo->setText(tr("System has updates available"));
  }else{
    ui->label_sysinfo->setText(tr("System is current up to date"));
  }
  
  //Now add in the information to the update tabs as necessary
  ui->combo_updates->clear(); //clear out the current list
  //  -- NOTE: ui->combo_updates data format: QStringList( <command to install>, <summary of update>)
  //Major updates (always do these first so they are at the top of the updates list)
  if(hasmajor){
    QStringList major = pcbsd::Utils::runShellCommand("syscache majorupdatelog").join("").split("<br>-------");
    for(int i=0; i<major.length(); i++){
      QString tag = major[i].section("TAG:",1,1).section("<br>",0,0).simplified();
      QString cmd = major[i].section("To install: \"", 1,1).section("\"",0,0);
      if( !tag.isEmpty() && !cmd.isEmpty() ){
        ui->combo_updates->addItem( QString(tr("Major OS Update: %1")).arg(tag) , QStringList() << "nice "+cmd << tr("Will install new FreeBSD version, apply security fixes, and update packages."));
      }
    }
  }
  //Now do the security + pkg update (if available - special combo command for pc-updatemanager)
  if(hassec && haspkg){
    ui->combo_updates->addItem( tr("Install Security & Package Updates"), QStringList() << "nice pc-updatemanager fbsdupdatepkgs" << tr("Will install all security fixes and update all packages") );
  }
  //Now list the security updates
  if(hassec){
    ui->combo_updates->addItem( tr("Install Security Updates"), QStringList() << "nice pc-updatemanager fbsdupdate" << tr("Will only install security updates") );
  }
  //Now list the package updates
  ui->group_details->setVisible(haspkg); //just hide this option if no pkg updates
  if(haspkg){
    ui->combo_updates->addItem( tr("Install Package Updates"), QStringList() << "nice pc-updatemanager pkgupdate" << tr("Will only update installed packages") );
    QString pkgdetails = pcbsd::Utils::runShellCommand("syscache \"pkg #system updatemessage\"").join("").replace("<br>","\n");
    ui->text_details->setPlainText(pkgdetails);
  }
  updateSelChange(); //make sure the UI is accurate
  
  //Now list the PC-BSD patches
  if(haspatch){
    ui->list_patches->clear();
    QStringList patch = pcbsd::Utils::runShellCommand("syscache pcbsdupdatelog").join("").split("<br>-----");
    for(int i=0; i<patch.length(); i++){
      QString name = patch[i].section("NAME: ",1,1).section("<br>",0,0).simplified();
      QString date = patch[i].section("DATE: ",1,1).section("<br>",0,0).simplified();
      QString details = patch[i].section("DETAILS: ",1,1).section("<br>",0,0).simplified();
      QString size = patch[i].section("SIZE: ",1,1).section("<br>",0,0).simplified();
      QString tag = patch[i].section("TAG: ",1,1).section("<br>",0,0).simplified();
      //Now add it to the list
      if(tag.isEmpty()){ continue; } //invalid update
      QListWidgetItem *it = new QListWidgetItem( QString(tr("%1 (%2)")).arg(name, date), ui->list_patches);
      it->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
      it->setCheckState(Qt::Checked); //always start out checked
      it->setToolTip(QString(tr("%1 (%2)").arg(details, size)) );
      it->setWhatsThis(tag);
      if(i==0){ ui->list_patches->setCurrentItem(it); }
    }
  }
  patchSelChange(); //make sure the UI is accurate
  
  //Make sure to select the first tab if necessary
  //if(cindex<0){ ui->tabWidget->setCurrentIndex(0); }
  //else{ ui->tabWidget->setCurrentIndex(cindex); }

  this->setEnabled(true);
}

//Update tab
void MainUI::updateSelChange(){ //update selection changed
  //Show the summary for the selected type of update
  QStringList info;
  if(ui->combo_updates->count()>0){ 
    info = ui->combo_updates->currentData().toStringList(); 
  }
  if(info.length()!=2){
    ui->tool_start->setEnabled(false);
    ui->label_upsummary->setText("");       
  }else{
    ui->tool_start->setEnabled(true);
    ui->label_upsummary->setText(info[1]);
  }
}

void MainUI::updateDetailsChange(){ //update details changed
    ui->text_details->setVisible(ui->group_details->isChecked());
}

void MainUI::startUpdates(){ //Start selected update
  QStringList up = ui->combo_updates->currentData().toStringList();
  if(up.isEmpty()){ return; } //invalid update command
  //Now launch the appropriate update
  QString cmd = up.first();
  qDebug() << "Starting Update:" << cmd;
  QProcess::startDetached(cmd);
  ShowUpdatingNotice();
  UpdateUI();
}

//Patches tab
void MainUI::patchSelChange(){
  //Display the tooltip for the item below the item as well (since it is easier to see)
  if(ui->list_patches->count() < 1){ return; } //nothing to do
  ui->label_patch_summary->setText(ui->list_patches->currentItem()->toolTip());
}

void MainUI::startPatches(){
  //Go through all the checked patchs and assemble the update command
  QStringList ups;
  for(int i=0; i<ui->list_patches->count(); i++){
    if(ui->list_patches->item(i)->checkState()==Qt::Checked){
      ups << ui->list_patches->item(i)->whatsThis();
    }
  }
  if(ups.isEmpty()){ return; } //nothing to do
  QString cmd = "nice pc-updatemanager install "+ups.join(" ");
  qDebug() << "Starting Patches:" << cmd;
  QProcess::startDetached(cmd);
  ShowUpdatingNotice();
  QTimer::singleShot(200, this, SLOT(UpdateUI()) );
}

//Log tab
void MainUI::updateLogChanged(){ //this is connected to a file watcher for changes
  //Check that the tab is visible(don't want to constantly be reading the file if not visible)
  if(ui->tabWidget->currentWidget()==ui->tab_log){
    QString log = pcbsd::Utils::readTextFile(UPDATE_LOG_FILE);
    if(log.isEmpty()){ log = pcbsd::Utils::readTextFile(UPDATE_LOG_FILE_PREVIOUS); }
    if(log.isEmpty()){ log = tr("No update logs available"); }
    //QString clog = ui->text_log->toPlainText();
    //if(clog.length() > log.length() || clog.isEmpty() ){
      //Completely different log than before - reset the entire view
      ui->text_log->setPlainText(log);
    //}else{
      //New info to the same log - just append the difference
      //ui->text_log->appendPlainText( log.remove(clog) );
    //}
    //Keep it at the bottom (the latest info)
    ui->text_log->verticalScrollBar()->setSliderPosition( ui->text_log->verticalScrollBar()->maximum() );
  }
}

//Configure tab
void MainUI::autoUpChange(){ //auto-update option changed
  QString val = ui->combo_autosetting->currentData().toString();
  //Update the summary label for this setting
  if(val=="all"){
    ui->label_autodesc->setText(tr("Auto-install all updates (including major OS upgrades)"));
  }else if(val=="security"){
    ui->label_autodesc->setText(tr("Auto-install security updates only"));	  
  }else if(val=="pkg"){
    ui->label_autodesc->setText(tr("Auto-install package updates only"));	  
  }else if(val=="securitypkg"){
    ui->label_autodesc->setText(tr("Auto-install security and package updates (recommended)"));	  
  }else if(val=="disabled"){
    ui->label_autodesc->setText(tr("Nothing is automatically updated"));	  
  }
  //Now save the value to the PC-BSD conf file
  pcbsd::Utils::setValPCBSDConf("AUTO_UPDATE",val);
}

void MainUI::on_actionVulnerabilities_triggered()
{
    PkgVulDialog* dlg = new PkgVulDialog(this);
    QTimer::singleShot(0,dlg, SLOT(setupDialog()) );
    dlg->exec();
    //
}

void MainUI::on_actionExit_triggered()
{
    close();
}

void MainUI::on_actionBase_updates_history_triggered()
{
    UpdateHistoryDialog* dlg = new UpdateHistoryDialog(this);
    dlg->execDialog();
}

void MainUI::on_actionEndOfLife_triggered()
{
    EOLDialog* dlg = new EOLDialog(this);
    QTimer::singleShot(0, dlg, SLOT(setupDialog()) );
    dlg->exec();
}

void MainUI::on_actionBranches_triggered()
{
    BranchesDialog* dlg = new BranchesDialog(this);
    QTimer::singleShot(0,dlg, SLOT(setupDialog()) );
    if (dlg->exec() == QDialog::Accepted)
    {
        QMessageBox msg(this);
        QString dst_branch = dlg->selectedBranch();
        QString info = tr("Your system will be switched to branch:")+"\n"+dst_branch;
        if (info.toUpper().indexOf("CURRENT")>=0)
        {
            info+= "\n\n"+tr("WARNING! This is an unstable version!");
        }
        msg.setText(tr("Are you sure you want to change system branch?"));
        msg.setInformativeText(info);
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msg.setDefaultButton(QMessageBox::No);
        msg.setIcon(QMessageBox::Question);
        if (msg.exec() == QMessageBox::Yes)
        {
            QString cmd = "pc-updatemanager chbranch "+dst_branch;
            qDebug() << "Starting changing branch:" << cmd;
            QProcess::startDetached(cmd);
	    this->thread()->usleep(30000); //300 milliseconds
	    QProcess::startDetached("syscache startsync");
            QTimer::singleShot(200, this, SLOT(UpdateUI()) );
        }
    }
}
