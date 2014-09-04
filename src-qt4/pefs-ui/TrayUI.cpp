/**************************************************************************
*   Copyright (C) 2014 by Yuri Momotyuk                                   *
*   yurkis@pcbsd.org                                                      *
*                                                                         *
*   Permission is hereby granted, free of charge, to any person obtaining *
*   a copy of this software and associated documentation files (the       *
*   "Software"), to deal in the Software without restriction, including   *
*   without limitation the rights to use, copy, modify, merge, publish,   *
*   distribute, sublicense, and/or sell copies of the Software, and to    *
*   permit persons to whom the Software is furnished to do so, subject to *
*   the following conditions:                                             *
*                                                                         *
*   The above copyright notice and this permission notice shall be        *
*   included in all copies or substantial portions of the Software.       *
*                                                                         *
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       *
*   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    *
*   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*
*   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR     *
*   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, *
*   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR *
*   OTHER DEALINGS IN THE SOFTWARE.                                       *
***************************************************************************/

#include "TrayUI.h"


///////////////////////////////////////////////////////////////////////////////
TrayUI::TrayUI()
{
    menu = new QMenu(0);

    readConfig();
    setupMenu();
}

///////////////////////////////////////////////////////////////////////////////
TrayUI::~TrayUI()
{

}

///////////////////////////////////////////////////////////////////////////////
void TrayUI::readConfig()
{
    //TODO: implement
}

///////////////////////////////////////////////////////////////////////////////
void TrayUI::setupMenu()
{
    QAction* act = new QAction( QIcon(":/images/sysupdater.png"), tr("Settings"), this);
    act->setWhatsThis("Open settings dialog"); //system updater code
    menu->addAction(act);

    act = new QAction( QIcon(":/images/sysupdater.png"), tr("New encrypted storage"), this);
    act->setWhatsThis("New encrypted storage wizard"); //system updater code
    menu->addAction(act);

        // - Separator
    menu->addSeparator();
    menu->addSeparator();

    act = new QAction( QIcon(":/images/sysupdater.png"), tr("Exit"), this);
    act->setWhatsThis("Exit from application"); //system updater code
    menu->addAction(act);
}

///////////////////////////////////////////////////////////////////////////////
void TrayUI::slotSingleInstance()
{
    this->show();
}
