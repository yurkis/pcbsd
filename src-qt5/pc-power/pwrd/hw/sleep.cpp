/**************************************************************************
*   Copyright (C) 2015- by Yuri Momotyuk                                   *
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

#include "sleep.h"

#include <dev/acpica/acpiio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

static const char* const ACPIDEV = "/dev/acpi";

bool ACPISleep(QString state)
{
    static int      acpifd;

    state = state.trimmed();
    if (!state.startsWith('S')) return false;
    if (state.length()!=2) return false;
    int sleep_type = QString(state[1]).toInt();

    // Open ACPI device
    acpifd = open(ACPIDEV, O_RDWR);
    if (acpifd == -1)
        acpifd = open(ACPIDEV, O_RDONLY);
    if (acpifd == -1)
        return false;


    bool RetVal = (0 != ioctl(acpifd, ACPIIO_REQSLPSTATE, &sleep_type));

    close(acpifd);

    return RetVal;
}
