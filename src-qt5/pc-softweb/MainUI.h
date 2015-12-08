#ifndef _PCBSD_APPCAFE_MAIN_WEB_UI_H
#define _PCBSD_APPCAFE_MAIN_WEB_UI_H

#include <QMainWindow>
#include <QWebView>
#include <QUrl>
#include <QProcess>
#include <QDebug>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QStatusBar>
#include <QToolBar>
#include <QAction>
#include <QWebHistory>
#include <QFile>
#include <QTextStream>
#include <QNetworkReply>
#include <QMenu>
#include <QToolButton>
#include <QFrame>
#include <QLineEdit>
#include <QToolButton>
#include <QShortcut>

#include "configDlg.h"
#include <pcbsd-utils.h>

#define BASEWEBURL QString("http://127.0.0.1:<port>")
#define LOCALUI QString("&AppCafeUI=true")

class MainUI : public QMainWindow{
	Q_OBJECT
public:
	MainUI(bool debugmode = false);
	~MainUI();

private:
	bool DEBUG;
	QString baseURL;
	QWebView *webview;
	QProgressBar *progressBar;
	QMenu *listMenu;
	QToolButton *listB;
	QAction *backA, *forA, *refA, *stopA, *progA;
	QFrame *group_search;
	QLineEdit *line_search;
	QToolButton *tool_search;
	QString lastsearch;
	QShortcut *ctrlF, *esc;
        QShortcut *ctrlQ, *Close;
	
	bool sameUrls(QUrl U1, QUrl U2);
	bool actionUrl(QUrl U1);

private slots:
	void slotSingleInstance();
	void LinkClicked(const QUrl &url);
	void PageStartLoading();
	void PageLoadProgress(int);
	void PageDoneLoading(bool);
	void authenticate(QNetworkReply*);
	void StatusTextChanged(const QString&);
	void loadHomePage();

	//Button Actions
	void GoBack();
	void GoForward();
	void GoRefresh();
	void GoStop();
	void GoClose();
	void GoConfigure();
	void GoSearch();
	void openSearch();
	void closeSearch();

	//Special Functionality
	void Save_pkglist();
};

#endif
