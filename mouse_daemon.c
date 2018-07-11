#include <X11/Xlib.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>

/* Minimum number of milliseconds to wait
   before every read.
   No interaction whit the mouse last of less then
   5 millisecond. By waiting 2 milliseconds we can
   be sure to catch any click and save 2 milliseconds of cpu time */

#define N 2

/* Right Left and Middle button code */
#define R 3
#define L 1
#define M 2


static int _XlibErrorHandler(Display *display, XErrorEvent *event);

int main(void) {

        Display *display;
        Bool result;
        Window window_returned;
        Window w;
        int root_x, root_y;
        int win_x, win_y, button;
        unsigned int mask_return;


        time_t now;
        struct tm *gmt;
        char formatted_gmt [50];

        struct timeval tv;
        struct timespec ts;

        char output[250];

        int fd;
        int n_rw;

        int fd_mouse;
        char buff[3];
        int click_considered = 0;
        /* Our process ID and Session ID */
        pid_t pid, sid;

        /* Fork off the parent process */
        pid = fork();
        if (pid < 0) {
                openlog(NULL,LOG_NDELAY | LOG_PID|LOG_CONS, LOG_DAEMON);
                syslog(LOG_INFO,"Error while forking." );
                closelog();
                exit(EXIT_FAILURE);
        }
        /* If we got a good PID, then
           we can exit the parent process. */
        if (pid > 0) {
                exit(EXIT_SUCCESS);
        }

        /* Change the file mode mask */
        umask(0);

        /* Open any logs here */

        /* Create a new SID for the child process */
        sid = setsid();
        if (sid < 0) {
                /* Log the failure */
                exit(EXIT_FAILURE);
        }

        /* Open file mouse log file */
        fd = open("/var/log/mouse.log",O_WRONLY | O_APPEND | O_CREAT, 0666);
        if(fd < 0 ) {
                openlog(NULL,LOG_NDELAY | LOG_PID|LOG_CONS, LOG_DAEMON);
                syslog(LOG_INFO,"Error while opening /var/log/mouse.log" );
                closelog();

                exit(EXIT_FAILURE);
        }
        /* Open mouse device */
        fd_mouse = open("/dev/input/mice", O_RDONLY );
        if(fd_mouse < 0) {
                printf("Error while opening /dev/input/mice\n");
                exit(0);
        }


        /* Change the current working directory */
        if ((chdir("/")) < 0) {
                /* Log the failure */
                exit(EXIT_FAILURE);
        }


        /* Close out the standard file descriptors */
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        /* Daemon-specific initialization goes here */
        ts.tv_sec = 0;
        ts.tv_nsec = N *10000000;

        /* X initialization */
        display = XOpenDisplay(NULL);
        assert(display);
        XSetErrorHandler(_XlibErrorHandler);
        w = XRootWindow(display,0);

        /* log initialization success */
        openlog(NULL,LOG_NDELAY | LOG_PID|LOG_CONS, LOG_DAEMON);
        syslog(LOG_INFO,"started." );
        closelog();


        while (1) {
                /* sleep N milliseconds */
                nanosleep(&ts,NULL);
                /* read mouse device,
                if any new info is available read block */
                read(fd_mouse,buff,sizeof(char)*3);
                /* set button code */
                button = 0;
                if((buff[0] & 0x1) != 0) button = L;
                if((buff[0] & 0x2) != 0) button = R;
                if((buff[0] & 0x4) != 0) button = M;

                /* if any button has been clicked */
                if(button) {
                    /* if current click has not been already considered:*/
                        if(!click_considered) {
                                /* mark current click as considered
                                used to not log multiple entry on mouse.log
                                beloging to the same click */
                                click_considered = 1;
                                /* get x,y position of the main monitor */
                                result = XQueryPointer(display, w, &window_returned,
                                                       &window_returned, &root_x, &root_y, &win_x, &win_y,
                                                       &mask_return);
                                if (result == True) {
                                        /*get current time */
                                        gettimeofday(&tv, NULL);
                                        now=tv.tv_sec;
                                        /* get current date */
                                        gmt = localtime(&now);
                                        /*write everything in output string */
                                        strftime ( formatted_gmt, sizeof(formatted_gmt), "%y/%m/%d,%H:%M:%S", gmt );
                                        sprintf(output,"%s.%03ld,x=%d,y=%d,b=%d\n",formatted_gmt,tv.tv_usec/1000, root_x, root_y,button );
                                        n_rw = strlen(output);
                                        /* write oputut string on mouse.log */
                                        if(write(fd,output,n_rw)< n_rw) {
                                                openlog(NULL,LOG_NDELAY | LOG_PID|LOG_CONS, LOG_DAEMON);
                                                syslog(LOG_INFO,"Error writing /var/log/mouse.log" );
                                                closelog();

                                                close(fd);
                                                XCloseDisplay(display);
                                                exit(EXIT_FAILURE);
                                        }
                                }
                        }
                }else {
                        /*If no button clicked and click_considered
                          set click as not considered
                          Used to make click_considered ready for the next click*/
                        if(click_considered) click_considered = 0;
                }

        }

        exit(EXIT_SUCCESS);
}
static int _XlibErrorHandler(Display *display, XErrorEvent *event) {

        openlog(NULL,LOG_NDELAY | LOG_PID|LOG_CONS, LOG_DAEMON);
        syslog(LOG_INFO,"An error occured detecting the mouse position\n" );
        closelog();

        return True;
}
