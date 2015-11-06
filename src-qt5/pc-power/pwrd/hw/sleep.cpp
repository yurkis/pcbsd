#include "sleep.h"

#include <dev/acpica/acpiio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

static const char* const ACPIDEV = "/dev/acpi";

bool acpi_sleep(QString state)
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
