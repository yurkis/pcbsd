#include "pdfUI.h"
#include "ui_pdfUI.h"

#include <QTimer>
#include <QScreen>
#include <QScrollBar>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPainter>
#include <QMessageBox>


int SCALEFACTOR = 4;

pdfUI::pdfUI(bool debug, QString file) : QMainWindow(), ui(new Ui::pdfUI()){
  ui->setupUi(this); //load the designer file
  DEBUG = debug;
  LOADINGFILE = false;
  PMODE = false; //Presentation mode
  DOC = 0; //initial pointer value
  SDPI = QSize(); //make sure it is empty by default
  presentationLabel = 0; //not initialized yet
  //PRINTER = new QPrinter();
  cdir = QDir::homePath(); //initial default
  upTimer = new QTimer(this);
    upTimer->setSingleShot(true);
    upTimer->setInterval(50);

  //Assemble any additional UI elements
  spin_page = new QSpinBox(this);
    spin_page->setPrefix(tr("Page")+" ");
    spin_page->setSingleStep(1); //one page at a time
    spin_page->setStatusTip(tr("Change the current page"));
    spin_page->setFocusPolicy(Qt::ClickFocus);
  ui->toolBar->insertWidget(ui->actionNext, spin_page); //insert between actionPrev/actionNext

  QWidget *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->toolBar->insertWidget(ui->actionStarttimer, spacer);
	
  combo_scale = new QComboBox(this);
    combo_scale->setStatusTip(tr("Change the scaling of the document output"));
    combo_scale->setFocusPolicy(Qt::NoFocus);
    ui->toolBar->insertWidget(ui->actionStarttimer, combo_scale);
    //Now populate the combobox with different scaling options (user data is integer codes for scaling set)
    combo_scale->addItem(tr("Fit to Width"), -1);
    combo_scale->addItem(tr("200%"), 200);
    combo_scale->addItem(tr("180%"), 180);
    combo_scale->addItem(tr("160%"), 160);
    combo_scale->addItem(tr("140%"), 140);
    combo_scale->addItem(tr("120%"), 120);
    combo_scale->addItem(tr("100%"), 100);
    combo_scale->addItem(tr("80%"), 80);
    combo_scale->addItem(tr("60%"), 60);
    combo_scale->addItem(tr("40%"), 40);
    combo_scale->addItem(tr("20%"), 20);
    combo_scale->addItem(tr("Fit to Height"), -2);

  spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->toolBar->insertWidget(ui->actionStarttimer, spacer);

  prevPageDnS = new QShortcut(QKeySequence(tr("Down")), this);
  nextPageUpS = new QShortcut(QKeySequence(tr("Up")), this);
  prevPageLS = new QShortcut(QKeySequence(tr("Left")), this);
  nextPageRS = new QShortcut(QKeySequence(tr("Right")), this);
  zoomInS = new QShortcut(QKeySequence(tr("Ctrl+Up")), this);
  zoomOutS = new QShortcut(QKeySequence(tr("Ctrl+Down")), this);
  
  //Setup any signals/slots
  connect(spin_page, SIGNAL(valueChanged(int)), this, SLOT(PageChanged()) );
  connect(combo_scale, SIGNAL(currentIndexChanged(int)),this, SLOT(PageChanged()) );
  connect(spin_page, SIGNAL(editingFinished()), this, SLOT(ShowPage()) );
  connect(upTimer, SIGNAL(timeout()), this, SLOT(ShowPage()) );
  connect(ui->actionPrev, SIGNAL(triggered()), this, SLOT(pageDown()));
  connect(ui->actionNext, SIGNAL(triggered()), this, SLOT(pageUp()));
  connect(ui->actionClose, SIGNAL(triggered()), this, SLOT(close()) );
  connect(ui->actionOpen_File, SIGNAL(triggered()), this, SLOT(OpenNewFile()) );
  connect(prevPageDnS, SIGNAL(activated()), this, SLOT( pageDown() ) );
  connect(nextPageUpS, SIGNAL(activated()), this, SLOT( pageUp() ) );
  connect(prevPageLS, SIGNAL(activated()), this, SLOT( pageDown() ) );
  connect(nextPageRS, SIGNAL(activated()), this, SLOT( pageUp() ) );
  connect(zoomInS, SIGNAL(activated()), this, SLOT( zoomUp() ) );
  connect(zoomOutS, SIGNAL(activated()), this, SLOT( zoomDown() ) );

  QApplication::setApplicationName(tr("PDF Viewer") );
  this->setWindowTitle(tr("PDF Viewer") );
  //Disable the UI elements until a document is loaded
  ui->actionPrev->setEnabled(false);
  ui->actionNext->setEnabled(false);
  spin_page->setEnabled(false);
  combo_scale->setEnabled(false);
  
  //Load the input file as necessary
  if(!file.isEmpty()){
    if( OpenPDF(file) ){
      //Update the UI appearance	    
      QTimer::singleShot(10, this, SLOT(ShowPage()));
    }
  }

  ui->actionStop_Presentation->setEnabled(PMODE);
  ui->menuStart_Presentation->setEnabled(!PMODE && DOC!=0);
  ui->actionPrint->setEnabled(DOC!=0);
  ui->actionPrint_Preview->setEnabled(DOC!=0);
  
  //Disable anything not finished yet
  ui->actionStarttimer->setVisible(false);
  ui->actionTimer->setVisible(false);
}

pdfUI::~pdfUI(){
  //Clean up any open document
  if(DOC!=0){
    delete DOC;
  }
  if(presentationLabel!=0){
    delete presentationLabel;
  }
}

// =================
//     PRIVATE FUNCTIONS
// =================

bool pdfUI::OpenPDF(QString filepath){
  LOADINGFILE = true;
 int pages = 0;
  //Verify that the file exists
	
  //Load it using Poppler
  Poppler::Document *TEMPDOC = Poppler::Document::load(filepath);
  if(TEMPDOC == 0 || TEMPDOC->isLocked()){ 
    //error loading the file	
    QMessageBox::warning(this, tr("Error loading file"), tr("There was an error trying to open the file. Is it a PDF document?") );
    LOADINGFILE = false;
    return false;
  }
  if(DOC!=0){ delete DOC; } //clean out the old document
  DOC = TEMPDOC; //good file - go ahead and use it
  pageimage = -1;
  pageImages.clear();
  //Save the dir this file is from for later
  cdir = filepath.section("/",0,-2);
  if(DEBUG){ qDebug() << "New cdir:" << cdir; }
  //Grab info about the document and update the widget
  pages = DOC->numPages();
  QString label= filepath.section("/",-1); //use the filename
  this->setWindowTitle(label);
  ui->actionPrint->setEnabled(DOC!=0);
  ui->actionPrint_Preview->setEnabled(DOC!=0);
  spin_page->setEnabled(pages!=1);
  combo_scale->setEnabled(DOC!=0);
  ui->actionStop_Presentation->setEnabled(false);
  ui->menuStart_Presentation->setEnabled(DOC!=0);
  
  //Update the available/current pages
  spin_page->setRange(1,pages);
  spin_page->setValue(1);
  spin_page->setSuffix( " "+ QString(tr("of %1")).arg( QString::number(spin_page->maximum()) ) );
  QApplication::processEvents(); //make sure to throw away the events from these changes
  LOADINGFILE = false;
  return true;
}

QImage pdfUI::OpenPage(int page, bool loadonly){
  bool needload = !pageImages.contains(page);
  qDebug() <<"Open Page:" << page;
  if(needload){
    //Now load the image for this page and show it
    qDebug() << " - Loading Page:" << page;
    Poppler::Page *DOCPAGE = DOC->page(page);
    if(DOCPAGE==0){
      //Error - could not load page

    }else{
      if(!SDPI.isValid()){
        ScreenChanged(); //get the screen DPI
      }
      pageImages.insert(page, DOCPAGE->renderToImage(SDPI.width(), SDPI.height()) ); //use the automatic settings (full page, default resolution)
      delete DOCPAGE; //done with the page structure
    }
  }
  if(loadonly){ 
    return QImage();
  }else{
    return pageImages.value(page,QImage());
  }
}

QScreen* pdfUI::getScreen(bool current, bool &cancelled){
  //Note: the "cancelled" boolian is actually an output - not an input
  QList<QScreen*> screens = QApplication::screens();
  cancelled = false;
  if(screens.length() ==1){ return screens[0]; } //only one option
  //Multiple screens available - figure it out
  if(current){
    //Just return the screen the window is currently on
    for(int i=0; i<screens.length(); i++){
      if(screens[i]->geometry().contains( this->mapToGlobal(this->pos()) )){
        return screens[i];
      }
    }
    //If it gets this far, there was an error and it should just return the primary screen
    return QApplication::primaryScreen();
  }else{
    //Ask the user to select a screen (for presentations, etc..)
    QStringList names;
    for(int i=0; i<screens.length(); i++){
      QString screensize = QString::number(screens[i]->size().width())+"x"+QString::number(screens[i]->size().height());
       names << QString(tr("%1 (%2)")).arg(screens[i]->name(), screensize);
    }
    bool ok = false;
    QString sel = QInputDialog::getItem(this, tr("Select Screen"), tr("Screen:"), names, 0, false, &ok);
    cancelled = !ok;
    if(!ok){ return screens[0]; } //cancelled - just return the first one 
    int index = names.indexOf(sel);
    if(index < 0){ return screens[0]; } //error - should never happen though
    return screens[index]; //return the selected screen
  }
}

void pdfUI::startPresentation(bool atStart){
  if(DOC==0){ return; } //just in case
  bool cancelled = false;
  QScreen *screen = getScreen(false, cancelled); //let the user select which screen to use (if multiples)
  if(cancelled){ return;}
  int page = 0;
  if(!atStart){ page = CurrentPage(); }
  PDPI = QSize(SCALEFACTOR*screen->physicalDotsPerInchX(), SCALEFACTOR*screen->physicalDotsPerInchY());
  //Now create the full-screen window on the selected screen
  if(presentationLabel == 0){
    //Create the label and any special flags for it
    presentationLabel = new QLabel(0, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
      presentationLabel->setStyleSheet("background-color: black;");
      presentationLabel->setAlignment(Qt::AlignCenter);
  }
  //Now put the label in the proper location
  presentationLabel->setGeometry(screen->geometry());
  presentationLabel->showFullScreen();
  PMODE = true; //set this internal flag
  ui->actionStop_Presentation->setEnabled(PMODE);
  ui->menuStart_Presentation->setEnabled(!PMODE && DOC!=0);
  QApplication::processEvents();
  //Now start at the proper page
  ShowPage(page);
  this->grabKeyboard(); //Grab any keyboard events - even from the presentation window
}  

// =================
//        PRIVATE SLOTS
// =================
void pdfUI::ShowPage(int page){
  if(LOADINGFILE){ return; }
  if(page < 0){
    //Just reload the current page (size/appearance modified?)
    page = CurrentPage();
    //De-activate the keyboard focus on the spinbox
    spin_page->clearFocus();
  }
  //Check for valid document/page
  if(page<0 || DOC==0 || page >=spin_page->maximum() ){
    if(PMODE){ endPresentation(); }
    else{ ui->label_page->setPixmap(QPixmap()); }//reset back to a blank display

    return; //invalid - no document loaded or invalid page specified
  }
  QImage PAGEIMAGE = OpenPage(page);

  //Now scale the image according to the user-designations and show it
  if(!PAGEIMAGE.isNull()){
    QPixmap pix;
    int scalecode = combo_scale->currentData().toInt();
    if(scalecode == -1){
      //scale to window width
      int width = ui->scrollArea->viewport()->width() - ui->scrollArea->verticalScrollBar()->width() - 4;
      pix.convertFromImage( PAGEIMAGE.scaledToWidth(width, Qt::SmoothTransformation) );
    }else if(scalecode == -2){
      //Scale to window height
      int height = ui->scrollArea->viewport()->height() - 4;
      pix.convertFromImage( PAGEIMAGE.scaledToHeight(height, Qt::SmoothTransformation) );
    }else if(scalecode > 0 && scalecode < (SCALEFACTOR*100) ){
      //Percent scaling
	//Shrink it down by the designated percentage (remember that the image is 3x larger than it should be)
	pix.convertFromImage(PAGEIMAGE.scaled(PAGEIMAGE.size()*(scalecode/(SCALEFACTOR*100.0)), Qt::KeepAspectRatio, Qt::SmoothTransformation) );
    }
    ui->label_page->setPixmap(pix);
    if(PMODE){
      //Now show the page in the presentation window  (always sized to fit on the screen);
      //Pick the smallest dimension and scale to fit to that
      if(presentationLabel->width() > presentationLabel->height()){
	presentationLabel->setPixmap( QPixmap::fromImage( PAGEIMAGE.scaledToHeight(presentationLabel->height()-2, Qt::SmoothTransformation) ) );
      }else{
	presentationLabel->setPixmap( QPixmap::fromImage( PAGEIMAGE.scaledToWidth(presentationLabel->width()-2, Qt::SmoothTransformation) ) );      
      }
      presentationLabel->show(); //always make sure it was not hidden
    }
  }else{
    ui->label_page->setPixmap(QPixmap());
    if(PMODE){ endPresentation(); }
  }
  if(page != CurrentPage()){
    //Need to update the page number shown on the UI
    LOADINGFILE = true; //ignore the next event
    spin_page->setValue(page+1);
    spin_page->clearFocus();
    QApplication::processEvents();
    LOADINGFILE = false;
  }
  ui->actionPrev->setEnabled(page>0);
  ui->actionNext->setEnabled(page < (spin_page->maximum()-1) );
  QTimer::singleShot(10, this, SLOT(PreLoadPages()) ); 
}

void pdfUI::PageChanged(){
  //This just collects all the page change calls and streamlines them so the ShowPage function does not get spammed
  if(upTimer->isActive()){ upTimer->stop(); }
  if(!spin_page->hasFocus()){
    upTimer->start();
  }
}

void pdfUI::ScreenChanged(){
  //Get the current Screen DPI
  bool junk;
  QScreen *scrn = getScreen(true,junk); //This is the screen the window is on
  //Note: Use 4x the detected DPI so that it scales nicely without pixellation
    SDPI.setWidth(SCALEFACTOR*scrn->physicalDotsPerInchX() );
    SDPI.setHeight(SCALEFACTOR*scrn->physicalDotsPerInchY() );
  if(DEBUG){ qDebug() << "Screen DPI (x"+QString::number(SCALEFACTOR)+"):" << SDPI.width() <<SDPI.height(); }
}

void pdfUI::PreLoadPages(){
  qDebug() << "Pre-loading pages...";
  //Go through and pre-load a couple pages (previous/next)
  int cpage = CurrentPage();
  //First clean out any pages outside the current range (prev/current/next)
  QList<int> loaded = pageImages.keys(); //quick way to force one page loaded at a time (temporary?)
    for(int i=0; i<loaded.length(); i++){
      //Allow 2 pages each way to be saved for future reference
      if(loaded[i] < cpage-2 || loaded[i]>cpage+2){
	qDebug() << "Removing page from the cache:" << loaded[i];
        pageImages.remove(loaded[i]);
      }
      QApplication::processEvents();
    }
  //Now make sure that the previous/next pages are loaded (don't care about return values)
  if(cpage > 0){ OpenPage(cpage-1,true); }
  QApplication::processEvents();
  if(cpage < spin_page->maximum()-1){ OpenPage(cpage+1,true); }
}


void pdfUI::zoomUp(){
  int curr = combo_scale->currentIndex();
  if(curr < 1){ return; } //already at the top
  combo_scale->setCurrentIndex(curr-1); //change the scale
}
void pdfUI::zoomDown(){
  int curr = combo_scale->currentIndex();
  if(curr == combo_scale->count()-1){ return; } //already at the bottom
  combo_scale->setCurrentIndex(curr+1); //change the scale	
}

void pdfUI::pageUp(){
  spin_page->stepUp();
  spin_page->clearFocus();
  /*int curr = combo_scale->currentIndex();
  if(curr < 1){ return; } //already at the top
  combo_scale->setCurrentIndex(curr-1); //change the scale*/
}
void pdfUI::pageDown(){
  spin_page->stepDown();
  spin_page->clearFocus();
  /*int curr = combo_scale->currentIndex();
  if(curr == combo_scale->count()-1){ return; } //already at the bottom
  combo_scale->setCurrentIndex(curr+1); //change the scale	*/
}

void pdfUI::OpenNewFile(){
  //Ask the user to select a file
  QString filepath = QFileDialog::getOpenFileName(this, tr("Open PDF"), cdir, tr("PDF Files (*.pdf *.PDF)") );
  if(filepath.isEmpty()){ return; } //cancelled
  if( OpenPDF(filepath) ){
      //Update the UI appearance	    
      QTimer::singleShot(10, this, SLOT(ShowPage()));
    }
}

void pdfUI::endPresentation(){
  if(!PMODE){ return; } //not in presentation mode
	  
  presentationLabel->hide(); //just hide this (no need to re-create the label for future presentations)
  PDPI = QSize(); //clear this
  PMODE = false;
  ui->actionStop_Presentation->setEnabled(PMODE);
  ui->menuStart_Presentation->setEnabled(!PMODE && DOC!=0);
  this->releaseKeyboard();
}

void pdfUI::on_actionPrint_triggered(){
  QPrintDialog dlg(this);
    dlg.setOption(QAbstractPrintDialog::PrintSelection, false);

    connect(&dlg, SIGNAL(accepted(QPrinter*)), this, SLOT(paintOnPrinter(QPrinter*)));
  dlg.exec();
}

void pdfUI::on_actionPrint_Preview_triggered(){
  QPrintPreviewDialog dlg(this);
    connect(&dlg, SIGNAL(paintRequested(QPrinter*)), this, SLOT(paintOnPrinter(QPrinter*)));
  dlg.exec();
}

void pdfUI::paintOnPrinter(QPrinter *PRINTER){
  //Get page range in index-notation (0->X, not page-number notation 1->X+1)
  int fromP = PRINTER->fromPage()-1;
  int toP = PRINTER->toPage()-1;
  //Adjust the page range as necessary
  if(fromP < 0){ fromP = 0; } //start at beginning
  if(toP < 1){ toP = spin_page->maximum()-1; } //full document
  else if(toP >= spin_page->maximum()){ toP = spin_page->maximum()-1; }
  //Setup the printing variables
  QRectF size = PRINTER->pageRect(QPrinter::DevicePixel);
  QPainter painter(PRINTER);
  QMessageBox wait(QMessageBox::NoIcon, tr("Please Wait"), QString(tr("Preparing Document (%1 pages)")).arg(QString::number(toP-fromP+1)), QMessageBox::Abort, this);
    wait.setInformativeText(" "); //Make sure the window is the right size before showing it
    //wait.setStandardButtons(QMessageBox::Abort); //make sure that no buttons are used
    wait.show();
  QApplication::processEvents();
  if(PRINTER->pageOrder()==QPrinter::LastPageFirst){
    //Reverse the page order
    qDebug() << "Print Document: pages "<< fromP+1 << "to" << toP+1;
    for(int i=toP; i>=fromP; i--){
      if(!wait.isVisible()){ break; }
      wait.setInformativeText( QString(tr("Loading Page: %1")).arg(i+1) );
      QApplication::processEvents();
      //Now paint this page on the printer
      if(i!=toP){ PRINTER->newPage(); } //this is the start of the next page (not needed for first)
      painter.drawImage(0,0,OpenPage(i).scaled(size.width(), size.height(), Qt::KeepAspectRatio,Qt::SmoothTransformation) );
      QApplication::processEvents();
    }
  }else{
    qDebug() << "Print Document: pages "<< fromP+1 << "to" << toP+1;
    for(int i=fromP; i<=toP; i++){
      if(!wait.isVisible()){ break; }
      //qDebug() << " printing page:" << i+1;
      wait.setInformativeText( QString(tr("Loading Page: %1")).arg(i+1) );
      QApplication::processEvents();
      //Now paint this page on the printer
      if(i!=fromP){ PRINTER->newPage(); } //this is the start of the next page (not needed for first)
      painter.drawImage(0,0,OpenPage(i).scaled(size.width(), size.height(), Qt::KeepAspectRatio,Qt::SmoothTransformation) );
      QApplication::processEvents();
    }
  }
  wait.close();
}
