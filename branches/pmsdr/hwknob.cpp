//
//   Hardware knob class
//
//   Interface incremental encoder via parallel port 
//
//   
//   A.Montefusco iw0hdv
//

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/ppdev.h>
#include <asm-generic/ioctl.h>
#include <linux/parport.h>

#include <qsocketnotifier.h>

#include "hwknob.h"
#include "main_widget.h"

Qt3Knob * Qt3Knob::Create (void)

{
   char *pn;
   if ((pn = getenv("HW_KNOB_PARALLEL"))) {
      fprintf (stderr, "Parallel knob\n");
      return new Qt3KnobIoctl(pn);
   }
   if ((pn = getenv("HW_KNOB_FIFO"))) {
      fprintf (stderr, "FIFO knob\n");
      return new Qt3KnobFifo(pn);
   }
   if ((pn = getenv("HW_KNOB_SERIAL"))) {
      fprintf (stderr, "SERIAL knob\n");
      return new Qt3KnobSerial(pn);
   }
   return 0;
}

Qt3KnobSerial :: Qt3KnobSerial (const char *pszSerPort):
   Qt3Knob(), value(0)
{
    if (getenv("REALTIME")) {
       int rtpri =90;
       struct sched_param param;
       printf("using realtime, priority: %d\n",rtpri);
       param.sched_priority = rtpri;
       /* enable realtime fifo scheduling */
       if(sched_setscheduler(0, SCHED_FIFO, &param)==-1){
           perror("sched_setscheduler failed");
           exit(-1);
       }
    }
    fd = open (pszSerPort, O_RDWR);
    if (fd == -1) {
       perror ("open");
       return;
    }    
    pF = fdopen (fd, "rw");
    if (pF == 0) {
       perror ("open");
       return; 
    }
}

int Qt3KnobSerial :: processEvent(void)
{
   char szBuf[BUFSIZ];
   int  delta = 0;
   int  v;

   // read data
   if (fgets (szBuf, sizeof(szBuf), pF)) {
      sscanf (szBuf, "%d", &v);
      fprintf (stderr, "%s: %d\n", __FUNCTION__, v);
      if (value != 0) {
         delta = v - value;
      }
      value = v;
   }
   return delta;
}


void Qt3KnobSerial :: postProcessEvent(void)
{}


Qt3KnobFifo :: Qt3KnobFifo (const char *pszFifo):
   Qt3Knob()
{
    int rtpri =90;
    struct sched_param param;
    printf("using realtime, priority: %d\n",rtpri);
    param.sched_priority = rtpri;
    /* enable realtime fifo scheduling */
    if(sched_setscheduler(0, SCHED_FIFO, &param)==-1){
         perror("sched_setscheduler failed");
         exit(-1);
    }

    fd = open (pszFifo, O_RDWR);
    if (fd == -1) {
       perror ("open");
       return;
    }    
    pF = fdopen (fd, "rw");
    if (pF == 0) {
       perror ("open");
       return; 
    }
}

int Qt3KnobFifo :: processEvent(void)
{
   char szBuf[BUFSIZ];
   int  delta = 0;

   // read data
   if (fgets (szBuf, sizeof(szBuf), pF)) {
      sscanf (szBuf, "%d", &delta);
      fprintf (stderr, "%s: %d\n", __FUNCTION__, delta);
   }
   return delta;
}
void Qt3KnobFifo :: postProcessEvent(void)
{}

Qt3KnobIoctl :: Qt3KnobIoctl (const char *pszPort):
   Qt3Knob(),
   counter(0),
   direction(0)
{
    int rtpri =90;
    struct sched_param param;
    printf("using realtime, priority: %d\n",rtpri);
    param.sched_priority = rtpri;
    /* enable realtime fifo scheduling */
    if(sched_setscheduler(0, SCHED_FIFO, &param)==-1){
         perror("sched_setscheduler failed");
         exit(-1);
    }

    fd = open (pszPort, O_RDWR);
    if (fd == -1) {
        perror ("open");
        return;
    }
    if (ioctl (fd, PPCLAIM)) {
        perror ("PPCLAIM");
        close (fd);
        return;
    }
    activate();    
}

void Qt3KnobIoctl :: activate (void)
{

    nAck = PARPORT_STATUS_ACK;
    nFault = PARPORT_STATUS_ERROR;
    Busy = PARPORT_STATUS_BUSY; 

    busy = nAck | nFault;
    acking = nFault;
    ready = Busy | nAck | nFault;

    /* Set up the control lines when an interrupt happens. */
    ioctl (fd, PPWCTLONIRQ, &busy);

    /* Now we're ready. */
    ioctl (fd, PPWCONTROL, &ready);
    //printf ("RR: %04X\n", ready); 

}

int Qt3KnobIoctl :: processEvent(void)
{
    int delta = 0;

    counter = counter + 1;
    //printf ("counter: %d\n",counter);
    // read the pin 11, /BUSY
    ioctl (fd, PPRSTATUS, &status);

    printf ("BB: %04X STATUS: %02X COUNTER: %d ..... ", busy, status, counter);
    if ( status & 0x80 ) ++delta;
                   else  --delta;
    //printf ("%d        \n", delta);  fflush (stdout); 

    /* We are now marked as busy. */
      
    /* Fetch the data. */
    ioctl (fd, PPRDATA, &ch);

    /* Clear the interrupt. */
    ioctl (fd, PPCLRIRQ, &irqc);
    if (irqc > 1) {
       fprintf (stderr, "Arghhhh! Missed %d interrupt%s!\n", irqc - 1, irqc == 2 ? "s" : "");
       delta = (irqc - 1) * direction;
    } else {
       direction = (delta > 0) ? 1 : -1;
    }
    /* Ack it. */
    ioctl (fd, PPWCONTROL, &acking);
    //usleep (2);
    ioctl (fd, PPWCONTROL, &busy);

    //printf ("--->%02X\n", ch);

    fprintf (stderr, "***** %d %d\n", delta, direction); fflush(stderr);
    
    return delta;
}

void Qt3KnobIoctl :: postProcessEvent(void)
{
   activate ();
}





HwKnobWidget :: HwKnobWidget (Qt3Knob *pKnob, QWidget *parent, const char *pszWidgetName)
	: QWidget(parent, pszWidgetName),
      pQt3Knob(pKnob),
      pMw(0)
{
    
}

void HwKnobWidget ::setViewer (Main_Widget *p)
{
    pMw = p;

    QSocketNotifier *pHwkSn = new QSocketNotifier( pQt3Knob->getFd(), QSocketNotifier::Read, this );
    QObject::connect( pHwkSn, SIGNAL(activated(int)), this , SLOT(hwKnobRotated()) );
}

void HwKnobWidget::hwKnobRotated( void )
{
   if (pQt3Knob && pMw) {
      int delta = pQt3Knob->processEvent();

      pQt3Knob->postProcessEvent();

      pMw->tune( delta );

      printf ("%s: DELTA: %d\n", __FUNCTION__, delta); fflush(stdout);
   }
}


