#include <alsa/asoundlib.h>
#include <stdio.h>
#include "Transport.h"
#include <sys/mman.h>
#include <termios.h>

#include <python2.5/Python.h>

extern "C" {
extern void inittransport();
}
int main(int argc, char *argv[])
{
    FILE *fp;
    int fd;
    struct termios portset;
    const char *device = "/dev/ttyS0";
 

    mlockall( MCL_CURRENT);
    mlockall( MCL_FUTURE);
   
    /* Set up io port correctly, and open it... */
    fd = open(device , O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
	    fprintf(stderr, "ERROR: open for %s failed.\n",device);
	    exit(1);
    }
    tcgetattr(fd, &portset);
    cfmakeraw(&portset);
    cfsetospeed(&portset, B19200);
    tcsetattr(fd, TCSANOW, &portset);
    close(fd);
    

    // setup python
    Py_SetProgramName(argv[0]);
    argv[0] = "driver.py";
    Py_Initialize();
    PySys_SetArgv(argc, argv);

    /* initialize our transport module */
    inittransport();
    
    // launch the main script
    fp = fopen("/home/drue/james/core/python/driver.py", "r");
    PyRun_SimpleFile(fp, "driver.py");
    fclose(fp);
    // we're done
    return 0;
    
}
