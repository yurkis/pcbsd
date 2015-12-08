#ifndef _PCBSD_PDF_VIEWER_H
#define _PCBSD_PDF_VIEWER_H
#include <QMainWindow>
#include <QSpinBox>
#include <QWidgetAction>
#include <QTimer>
#include <QResizeEvent>
#include <QComboBox>
#include <QLabel>
#include <QHash>
#include <QPrinter>
#include <QShortcut>

#include <poppler-qt5.h>

namespace Ui{
	class pdfUI;
};

class pdfUI : public QMainWindow{
	Q_OBJECT
public:
	pdfUI(bool debug = false, QString file = "");
	~pdfUI();



private:
	Ui::pdfUI *ui;
	Poppler::Document *DOC;
	bool DEBUG, LOADINGFILE, PMODE;
	QTimer *upTimer;
	QSize SDPI, PDPI; //current screen/presentation DPI
	QHash<int,QImage> pageImages; //the list of all loaded pages (instant read later)
	//QPrinter *PRINTER;
	int pageimage; //The page number for the saved image
	QString cdir; //the directory that the current file is exists in (conveniance)
	QLabel *presentationLabel;
	//Keyboard shortcuts
	QShortcut *prevPageDnS, *nextPageUpS, *prevPageLS, *nextPageRS, *zoomInS, *zoomOutS;

	//Additional display widgets (could not be added via Qt Designer)
	QSpinBox *spin_page;
	QComboBox *combo_scale;

	//The main functions using the Poppler library for reading the file
	bool OpenPDF(QString filepath);
	QImage OpenPage(int page, bool loadonly = false);

	int CurrentPage(){ //converts from the "display" number (1...) to the index (0...)
	  return (spin_page->value()-1);
	}
	
	QScreen *getScreen(bool current, bool &cancelled);
	void startPresentation(bool atStart);
	
private slots:
	//PDF Viewing Functions
	void ShowPage(int page = -1); //Also uses Poppler to pull the particular page from the file
	void PageChanged(); //for streamlining the number of calls from rapidly changing page numbers
	void ScreenChanged();
	void PreLoadPages(); //run this in the background to get the next page(s) ready

	void zoomUp();
	void zoomDown();
	void pageUp();
	void pageDown();

	//UI Interaction Functions
	void OpenNewFile();
	void endPresentation();


	void on_actionStart_at_Beginning_triggered(){
	  startPresentation(true);
	}
	void on_actionAt_Current_Page_triggered(){
	  startPresentation(false);
	}
	//Printing support functions
	void on_actionPrint_triggered();
	void on_actionPrint_Preview_triggered();
	void paintOnPrinter(QPrinter *PRINTER);
	


protected:
	void resizeEvent(QResizeEvent *event){
	  //Use the event in the QMainWindow first
	  QMainWindow::resizeEvent(event);
	  //Now refresh the page
	  if(combo_scale->currentData().toInt() < 0){
	    //Current scaling based on window size - reload the page
	    if(upTimer->isActive()){ upTimer->stop(); }
	    upTimer->start();
          }
	}
	
	void keyPressEvent(QKeyEvent *event){
	  //See if this is one of the special hotkeys and act appropriately 
	  // NOTE: Some of this is duplicated with the QShortcut definitions (for non-presentation mode)
	  //  This routine does not always work for the main window viewer due to differing widget focus policies
	  if( event->key()==Qt::Key_Escape || event->key()==Qt::Key_Backspace){
	    endPresentation();
	  }else if(event->key()==Qt::Key_Right || event->key()==Qt::Key_Down || event->key()==Qt::Key_Space){
	    ShowPage( CurrentPage()+1 );
	  }else if(event->key()==Qt::Key_Left || event->key()==Qt::Key_Up){
	    ShowPage( CurrentPage()-1 );
	  }else if(event->key()==Qt::Key_Home){
	    ShowPage(0); //go to the first page
	  }else if(event->key()==Qt::Key_End){
	    ShowPage( spin_page->maximum()-1 ); //go to the last page
	  }else if(event->key()==Qt::Key_F11){
	    if(PMODE){ endPresentation(); }
	    else{ startPresentation(true); }
	  }else{
	    QMainWindow::keyPressEvent(event);
	  }
	}
};

#endif
