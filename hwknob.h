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
   
  Qt3Knob (): fd(-1) {} ;
  virtual ~Qt3Knob () {}
  int getFd () const { return fd; }
  bool isActive (void) const { return (fd != -1); }

  // to be implemented in derived classes
  virtual int processEvent(void) = 0;
  virtual void postProcessEvent (void) = 0 ;

  static Qt3Knob *Create (void);

protected:
  int fd;
};

class Qt3KnobFifo: public Qt3Knob {

public:

   Qt3KnobFifo (const char *pszFifo);
   ~Qt3KnobFifo () {} 
   int processEvent(void);
   void postProcessEvent(void);
   
private:
   FILE *pF;
};




class Qt3KnobIoctl: public Qt3Knob {

public:

   Qt3KnobIoctl (const char *pszPort);
   ~Qt3KnobIoctl () {} 
   int processEvent(void);
   void postProcessEvent(void);
   
private:
   void activate (void);

private:
   int counter; 
   int irqc;
   int nAck;
   int nFault;
   int Busy; 
   int busy;
   int acking;
   int ready;
   int direction;         //  0 .... unknown
                          // -1 .... ccw
                          //  1 .... cw
   char ch;
   unsigned char status;
};


class Main_Widget;

class HwKnobWidget: public QWidget {
   Q_OBJECT

public:
   HwKnobWidget (Qt3Knob *, QWidget *parent = 0, const char *pszWidgetName = 0);
   void setViewer (Main_Widget *p);

public slots:
   void hwKnobRotated ();

private:
   Qt3Knob     *pQt3Knob;
   Main_Widget *pMw;
};

#endif

