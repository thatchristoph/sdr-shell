//
//   Frequency classes for PMSDR
//
//   Keeps memory of the state of the oscillators
//
//   
//   A.Montefusco iw0hdv
//
#include <stdio.h>
#include <stdlib.h>
#include "dttsp.h"
#include "frequency.h"



Frequency :: ~Frequency ()
{

}

FrequencyExtOsc :: FrequencyExtOsc ( int sampleRate, int startFreq, int fLocMax, int fLocMin ):
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



FrequencyPMSDR :: FrequencyPMSDR( int sampleRate, FILE *pPmSdrFile, FILE *pDttSpFile ):
    FrequencyExtOsc (sampleRate, 7050000, 30*1E6, 1E5 ),
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
    FrequencyExtOsc (sampleRate, 7050000, 30*1E6, 1E5 ),
    pPmsdr (pPmSdrFile),
    cp (pCmd)
{

}

FrequencyPMSDRudp :: ~FrequencyPMSDRudp ()
{
}

void FrequencyPMSDRudp :: changeExtOsc ()
{
    printf(">>>>>>>>>>>> %s: new Ext Osc: %d\n", __FUNCTION__, fExtOsc_ );

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


void FrequencyExtOsc :: changeExtOsc ()
{
    fprintf (stderr, ">>> External Oscillator: %d\n", fExtOsc_);
}

void FrequencyExtOsc :: changeSwOsc ()
{
    fprintf (stderr, ">>> Internal Oscillator: %d\n", fSwOsc_);
}


#if defined TEST_MODULE

void FrequencyExtOsc :: dump ()
{
    fprintf (stderr, "%s: %d %d %d\n", __FUNCTION__, 
             fExtOscMin_ , fExtOscMax_ , frequency_ );
}

int main (int argc, char *argv[])
{

    FrequencyExtOsc f ( 96000, 7050000, 30*1E6, 1E5 );
    f.dump();

    f.set (7050000);
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

    Frequency *pF = new FrequencyPMSDR  ( 96000, 0, 0 );
    pF->dump();

    pF->set (7050000);
    pF->dump();
}

#endif
