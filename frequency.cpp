//
//   Frequency classes for PMSDR
//
//   Keeps memory of the state of the oscillators
//
//   
//   (C) 2008,2009,2010 A.Montefusco iw0hdv
//
#include <stdio.h>
#include <stdlib.h>
#include "dttsp.h"
#include "frequency.h"


Frequency :: ~Frequency ()
{

}

FrequencyExtOsc :: FrequencyExtOsc ( int sampleRate, int startFreq, int fLocMax, int fLocMin):
    Frequency(),
    fExtOscMin_(fLocMin), 
    fExtOscMax_(fLocMax), 
    sampleRate_(sampleRate)
{
    set ( startFreq);

}

FrequencyExtOsc :: ~FrequencyExtOsc ()
{

}
       

void FrequencyExtOsc :: set ( int newF )
{
    if ( newF < fExtOscMax_  && newF > fExtOscMin_ ) {
        frequency_ = newF;
    } else {
        frequency_ = (fExtOscMax_ - fExtOscMin_) / 2;
    }
    fSwOsc_    = - (sampleRate_ / 4) ;  // minus to indicate up conversion
    fExtOsc_   = newF + fSwOsc_;

    changeExtOsc ();
    changeSwOsc ();
}

int  FrequencyExtOsc :: get ( )  const
{
    return frequency_;

}

void FrequencyExtOsc :: change ( int df )
{
    if (df == 0) { return ; }

    //
    // check if we have enough room to move the sw oscillator only
    //

    if ( ( abs (fSwOsc_) + abs (df) > (sampleRate_ / 2) )
                           ||
         ( abs (fSwOsc_) - abs (df) < (sampleRate_ / 8)  )
    ) {

        set ( frequency_ + df );

    } else {

        frequency_ += df;

        // in range, move the sw oscillator only
        fSwOsc_ -= df;
        changeSwOsc ();
    }
}


void FrequencyExtOsc :: changeDcOsc ()
{
    fprintf (stderr, ">>> Empty\n");
}

void FrequencyExtOsc :: changeExtOsc ()
{
    fprintf (stderr, ">>> External Oscillator: %d\n", fExtOsc_);
}

void FrequencyExtOsc :: changeSwOsc ()
{
    fprintf (stderr, ">>> Internal Oscillator: %d\n", fSwOsc_);
}


//
// Down Converter class
//


float FrequencyExtOscWithDc :: bands [][2] = {
          { 150E6, 130E6 },   // VHF II
          { 140E6, 100E6 },   // VHF I
          { 110E6,  90E6 },   // FM WB
          { 35E6,      0 },   // HF, downconverter off
          { 0,         0 },
        };

FrequencyExtOscWithDc :: FrequencyExtOscWithDc ( int sampleRate, int startFreq,
                                                 int fLocMax, int fLocMin,
                                                 int fDcOscMin, int fDcOscMax):
    Frequency(),
    fExtOscMin_(fLocMin), 
    fExtOscMax_(fLocMax), 
    fDcOscMin_(fDcOscMin), 
    fDcOscMax_(fDcOscMax), 
    sampleRate_(sampleRate),
    fDcOsc_  (0)
{
    set ( startFreq);
}

FrequencyExtOscWithDc :: ~FrequencyExtOscWithDc ()
{

}
       
void FrequencyExtOscWithDc :: set ( int newF )
{
    int newFrx = newF;
    int n;
    float ndcf = 0;

    // select the downconverter frequency
    for (n=0; !(bands[n][0] == 0 && bands[n][1] == 0); ++n) {
        if (newF >= bands[n][0]) {
            int indx = (n==0) ? 0 : n-1;
            ndcf =  bands[indx][1];
            break;
        }
    }
    if (ndcf != fDcOsc_) {
        fDcOsc_ = ndcf;
        if (fDcOsc_ > 0) changeDcOsc ();
    } 

    if (fDcOsc_ != 0) {
        newFrx = newF - fDcOsc_;
    }

    {
        if ( newFrx < fExtOscMax_  && newFrx > fExtOscMin_ ) {
            frequency_ = newFrx;
        } else {
            frequency_ = (fExtOscMax_ - fExtOscMin_) / 2;
        }
        fSwOsc_    = - (sampleRate_ / 4) ;  // minus to indicate up conversion
        fExtOsc_   = newFrx + fSwOsc_;

        changeExtOsc ();
        changeSwOsc ();
    }
}

int  FrequencyExtOscWithDc :: get ( )  const
{
    return frequency_ + fDcOsc_;
}

void FrequencyExtOscWithDc :: change ( int df )
{
    if (df == 0) { return ; }

    //
    // check if we have enough room to move the sw oscillator only
    //

    if ( ( abs (fSwOsc_) + abs (df) > (sampleRate_ / 2) )
                           ||
         ( abs (fSwOsc_) - abs (df) < (sampleRate_ / 8)  )
    ) {

        set ( get() + df );

    } else {

        frequency_ += df;

        // in range, move the sw oscillator only
        fSwOsc_ -= df;
        changeSwOsc ();
    }
}

void FrequencyExtOscWithDc :: changeDcOsc ()
{
    fprintf (stderr, ">>>%s: Downconverter oscillator: %d \n", __PRETTY_FUNCTION__, fDcOsc_);
}

void FrequencyExtOscWithDc :: changeExtOsc ()
{
    fprintf (stderr, ">>>%s: External Oscillator: %d\n", __PRETTY_FUNCTION__, fExtOsc_);
}

void FrequencyExtOscWithDc :: changeSwOsc ()
{
    fprintf (stderr, ">>>%s: Internal Oscillator: %d\n", __PRETTY_FUNCTION__, fSwOsc_);
}


FrequencyPMSDR :: FrequencyPMSDR( int sampleRate, FILE *pPmSdrFile, FILE *pDttSpFile ):
    FrequencyExtOsc (sampleRate, 7050000, 500*1E6, 1E5 ),
    pPmsdr (pPmSdrFile),
    pDttsp (pDttSpFile)
{

}

FrequencyPMSDR :: ~FrequencyPMSDR ()
{
}

void FrequencyPMSDR :: changeExtOsc ()
{
    printf(">>>>>>>>>>>> %s: new Ext Osc: %d\n", __FUNCTION__, fExtOsc_ );

    if ( pPmsdr != NULL) {
        fprintf ( pPmsdr, "f %d\n", fExtOsc_ );
        fflush  ( pPmsdr );
        printf("************ %s: sent to PMSDR: [%d]\n", __FUNCTION__, fExtOsc_);
    }
}

void FrequencyPMSDR :: changeSwOsc ()
{
    printf(">>>>>>>>>>>> %s: new Sw Osc: %d\n", __FUNCTION__, fSwOsc_ );
    if ( pDttsp != NULL ) {
        fprintf( pDttsp, "setOsc %d\n", fSwOsc_ );
        fflush ( pDttsp );
    }
}




FrequencyPMSDRudp :: FrequencyPMSDRudp ( int sampleRate, FILE *pPmSdrFile, DttSPcmd *pCmd ):
    FrequencyExtOsc (sampleRate, 7050000, 500*1E6, 1E5 ),
    pPmsdr (pPmSdrFile),
    cp (pCmd)
{
    const char *cmd = "dfilter 0\n";   // bypass
    printf(">>>>>>>>>>>> %s\n", __PRETTY_FUNCTION__ );

    if ( pPmsdr != NULL) {
        fprintf ( pPmsdr, "%s", cmd);
        fflush  ( pPmsdr );
        printf("************ %s: sent to PMSDR: %s", __PRETTY_FUNCTION__, cmd);

        cmd = "fd 100000000";
        fprintf ( pPmsdr, "%s", cmd);
        fflush  ( pPmsdr );
        printf("************ %s: sent to PMSDR: %s", __PRETTY_FUNCTION__, cmd);

    } 

}

FrequencyPMSDRudp :: ~FrequencyPMSDRudp ()
{  
  printf(">>>>>>>>>>>> %s\n", __PRETTY_FUNCTION__ );

  if ( pPmsdr != NULL) {
      char cmd[BUFSIZ];

      sprintf (cmd, "%s", "quit\n");
      fprintf ( pPmsdr, "%s", cmd);
      fflush  ( pPmsdr );
      printf("************ %s: sent to PMSDR: %s", __PRETTY_FUNCTION__, cmd);
  } 
}

void FrequencyPMSDRudp :: changeExtOsc ()
{
    printf(">>>>>>>>>>>> %s: new Ext Osc: %d\n", __PRETTY_FUNCTION__, fExtOsc_ );

    if ( pPmsdr != NULL) {
        fprintf ( pPmsdr, "f %d\n", fExtOsc_ );
        fflush  ( pPmsdr );
        printf("************ %s: sent to PMSDR: [%d]\n", __FUNCTION__, fExtOsc_);
    }
}

void FrequencyPMSDRudp :: changeSwOsc ()
{
    printf(">>>>>>>>>>>> %s: new Sw Osc: %d\n", __FUNCTION__, fSwOsc_ );
    if ( cp != NULL ) {
        //char szBuf [BUFSIZ];

        //sprintf (szBuf, "setOsc %d\n", fSwOsc_ );

        //send_command (cp, szBuf);
        int rc = cp->sendCommand ( "setOsc %d\n", fSwOsc_ );

        if (rc < 0) {
            fprintf (stderr, "%s: sendCommand rc: %d\n", __FUNCTION__, rc);
        }
    }
}




FrequencyPMSDRwithDcUdp :: FrequencyPMSDRwithDcUdp ( int sampleRate, FILE *pPmSdrFile, DttSPcmd *pCmd ):
    FrequencyExtOscWithDc (sampleRate, 7050000, 500*1E6, 1E5, 35E6 ),
    pPmsdr (pPmSdrFile),
    cp (pCmd)
{
}

FrequencyPMSDRwithDcUdp :: ~FrequencyPMSDRwithDcUdp ()
{ 
  printf(">>>>>>>>>>>> %s\n", __PRETTY_FUNCTION__ );

  if ( pPmsdr != NULL) {
      char cmd[BUFSIZ];


      if (fDcOsc_ != 0) {
          sprintf (cmd, "%s", "dfilter 0\n");
          fprintf ( pPmsdr, "%s", cmd);
          fflush  ( pPmsdr );
          printf("************ %s: sent to PMSDR: %s", __PRETTY_FUNCTION__, cmd);
      }
      sprintf (cmd, "%s", "quit\n");
      fprintf ( pPmsdr, "%s", cmd);
      fflush  ( pPmsdr );
      printf("************ %s: sent to PMSDR: %s", __PRETTY_FUNCTION__, cmd);
  } 
}


void FrequencyPMSDRwithDcUdp :: changeDcOsc ()
{
  printf(">>>>>>>>>>>> %s\n", __PRETTY_FUNCTION__ );

  if ( pPmsdr != NULL) {
      char cmd[BUFSIZ];


      if (fDcOsc_ != 0) {
          sprintf (cmd, "%s", "dfilter 3\n");
          fprintf ( pPmsdr, "%s", cmd);
          fflush  ( pPmsdr );
          printf("************ %s: sent to PMSDR: %s", __PRETTY_FUNCTION__, cmd);

          sprintf (cmd, "fd %d\n", fDcOsc_ );
          fprintf ( pPmsdr, "%s", cmd);
          fflush  ( pPmsdr );
          printf("************ %s: sent to PMSDR: %s", __PRETTY_FUNCTION__, cmd);
      }

  } 
}

void FrequencyPMSDRwithDcUdp :: changeExtOsc ()
{
    printf(">>>>>>>>>>>> %s: new Ext Osc: %d\n", __PRETTY_FUNCTION__, fExtOsc_ );

    if ( pPmsdr != NULL) {
        fprintf ( pPmsdr, "f %d\n", fExtOsc_ );
        fflush  ( pPmsdr );
        printf("************ %s: sent to PMSDR: [%d]\n", __FUNCTION__, fExtOsc_);
    }
}

void FrequencyPMSDRwithDcUdp :: changeSwOsc ()
{
    printf(">>>>>>>>>>>> %s: new Sw Osc: %d\n", __FUNCTION__, fSwOsc_ );
    if ( cp != NULL ) {
        //char szBuf [BUFSIZ];

        //sprintf (szBuf, "setOsc %d\n", fSwOsc_ );

        //send_command (cp, szBuf);
        int rc = cp->sendCommand ( "setOsc %d\n", fSwOsc_ );

        if (rc < 0) {
            fprintf (stderr, "%s: sendCommand rc: %d\n", __FUNCTION__, rc);
        }
    }
}


#if defined TEST_MODULE

//
// g++ -DTEST_MODULE frequency.cpp dttsp.cpp  -o ft
//

void FrequencyExtOsc :: dump ()
{
    fprintf (stderr, "%s: %d %d %d\n", __FUNCTION__, 
             fExtOscMin_ , fExtOscMax_ , frequency_ );
}

void FrequencyExtOscWithDc :: dump ()
{
    fprintf (stderr, "%s: %d %d %d DC: %d EXT: %d SW: %d\n", 
             __PRETTY_FUNCTION__, 
             fExtOscMin_ , fExtOscMax_ , frequency_,
             fDcOsc_, fExtOsc_, fSwOsc_ 
                     
             );
}

int main (int argc, char *argv[])
{

    FrequencyExtOsc f ( 96000, 7050000, 500*1E6, 1E5 );
    f.dump();

    f.set (30050000);
    f.dump();

    f.change ( 1000 );
    f.dump();

    f.change ( -1000 );
    f.dump();

    f.change ( -1000 );
    f.dump();

    f.change ( 50000 );
    f.dump();

    f.change ( -50000 );
    f.dump();

    f.change ( -23999 );
    f.dump();

    int x = f.get();

    fprintf (stderr, "------------------------\n");

    Frequency *pF = new FrequencyPMSDR  ( 96000, 0, 0 );
    pF->dump();

    pF->set (7050000);
    pF->dump();


    fprintf (stderr, "------------------------\n");
    FrequencyExtOscWithDc fdc (96000, 7050000, 500*1E6, 1E5, 35E6 );
    fdc.dump();

    fdc.set (139000000);   // 130.9 MHz
    fdc.dump();

    fdc.change ( 1000 );
    fdc.dump();

    fdc.change ( -1000 );
    fdc.dump();

    fdc.set (144000000);   // 144 MHz
    fdc.dump();


}

#endif
