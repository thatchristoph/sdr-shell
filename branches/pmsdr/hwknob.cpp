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



Qt3Knob :: Qt3Knob (const char *pszPort):
   fd(-1),
   counter(0)
{
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

void Qt3Knob :: activate (void)
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

bool Qt3Knob :: isActive (void) const
{
	return (fd != -1);
}

int Qt3Knob :: processEvent(void)
{
	int delta = 0;

	counter = counter + 1;
	//printf ("counter: %d\n",counter);
    // read the pin 11, /BUSY
    ioctl (fd, PPRSTATUS, &status);

    //printf ("BB: %04X STATUS: %02X COUNTER: %d ..... ", busy, status, counter);
    if ( status & 0x80 ) --delta;
                   else  ++delta;
    //printf ("%d        \n", delta);  fflush (stdout); 

    /* We are now marked as busy. */
      
    /* Fetch the data. */
    ioctl (fd, PPRDATA, &ch);

    /* Clear the interrupt. */
    ioctl (fd, PPCLRIRQ, &irqc);
    if (irqc > 1) fprintf (stderr, "Arghh! Missed %d interrupt%s!\n", irqc - 1, irqc == 2 ? "s" : "");

    /* Ack it. */
    ioctl (fd, PPWCONTROL, &acking);
    //usleep (2);
    ioctl (fd, PPWCONTROL, &busy);

    //printf ("--->%02X\n", ch);

    return delta;
}




HwKnobWidget :: HwKnobWidget (const char *pszPort, QWidget *parent, const char *pszWidgetName)
	: QWidget(parent, pszWidgetName),
      pQt3Knob(0),
      pMw(0)
{
    pQt3Knob = new Qt3Knob(pszPort);
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

      pQt3Knob->activate();

      pMw->tune( delta );

      printf ("%s: DELTA: %d\n", __FUNCTION__, delta); fflush(stdout);
   }
}


