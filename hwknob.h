//
//   Hardware knob class
//
//   Interface incremental encoder via parallel port 
//
//   
//   A.Montefusco iw0hdv
//
#if !defined __HWKNOB_H__
#define      __HWKNOB_H__

#include <qwidget.h>

class Qt3Knob {

public:

   Qt3Knob (const char *pszPort);

   void activate (void);

   bool isActive (void) const;

   int processEvent(void);

   int getFd () const { return fd; }

private:
   int fd;
   int counter; 
   int irqc;
   int nAck;
   int nFault;
   int Busy; 
   int busy;
   int acking;
   int ready;
   char ch;
   unsigned char status;
};


class Main_Widget;

class HwKnobWidget: public QWidget {
   Q_OBJECT
public:
   HwKnobWidget (const char *pszPort, QWidget *parent = 0, const char *pszWidgetName = 0);
   void setViewer (Main_Widget *p);

public slots:
   void hwKnobRotated ();

private:
   Qt3Knob     *pQt3Knob;
   Main_Widget *pMw;
};

#endif

