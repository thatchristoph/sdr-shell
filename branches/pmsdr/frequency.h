//
//   Frequency classes for PMSDR
//
//   Keeps memory of the state of the oscillators
//
//   
//   A.Montefusco iw0hdv
//
#if !defined __FREQUENCY_H__
#define      __FREQUENCY_H__


class Frequency {

    public:

        Frequency () {} ;
        virtual ~Frequency () = 0;

        virtual void set ( int ) = 0;      // absolute change
        virtual int  get ( ) const = 0;          // get current frequency

        virtual void change ( int ) = 0;   // relative change

        virtual void changeDcOsc ()  = 0;  // change Down Converter oscillator
        virtual void changeExtOsc () = 0;  // change QSD oscillator
        virtual void changeSwOsc ()  = 0;  // change DSP oscillator

#if defined TEST_MODULE
        virtual void dump ( ) = 0; 
#endif
};


class FrequencyExtOsc: public Frequency {

    private:

        int fExtOscMin_ ;
        int fExtOscMax_ ;

        int frequency_  ;  // RX frequency
        int sampleRate_ ;  // DttSP sample rate


    protected:

        int fExtOsc_    ;  // hw oscillator, can't be changed in XTALL SR
        int fSwOsc_     ;  // DttSP second conversion

        int fixedExtOsc ( void ) { return (fExtOscMin_ == fExtOscMin_) ; } ;


    public:

        FrequencyExtOsc ( int sampleRate, int startFreq = 0, int fExtOscMax = 0, int fExtOscMin = 0);
        virtual ~FrequencyExtOsc ();
       

        virtual void set ( int );      // absolute change
        virtual int  get ( ) const ;

        virtual void change ( int );   // relative change

        virtual void changeDcOsc ();
        virtual void changeExtOsc ();
        virtual void changeSwOsc ();

#if defined TEST_MODULE
        virtual void dump ();
#endif 

};


class FrequencyExtOscWithDc: public Frequency {

    private:

        int fExtOscMin_ ;
        int fExtOscMax_ ;

        int fDcOscMin_ ;
        int fDcOscMax_ ;


        int frequency_  ;  // absolute frequency to be received
        int sampleRate_ ;  // DttSP sample rate

        static float bands [128][2];

    protected:

        int fDcOsc_    ;   // hw oscillator, in PMSDR can be modified
        int fExtOsc_    ;  // hw oscillator, can't be changed in XTALL SR
        int fSwOsc_     ;  // DttSP second conversion

        int fixedExtOsc ( void ) { return (fExtOscMin_ == fExtOscMin_) ; } ;


    public:

        FrequencyExtOscWithDc ( int sampleRate, int startFreq = 0, int fExtOscMax = 0, int fExtOscMin = 0,
                                int fDcOscMin = 10000000, int fDcOscMax = 800000000);
        virtual ~FrequencyExtOscWithDc ();
       

        virtual void set ( int );      // absolute change
        virtual int  get ( ) const ;

        virtual void change ( int );   // relative change

        virtual void changeDcOsc ();
        virtual void changeExtOsc ();
        virtual void changeSwOsc ();

#if defined TEST_MODULE
        virtual void dump ();
#endif 

};



class FrequencyPMSDR: public FrequencyExtOsc {

    private:
        FILE *pPmsdr;
        FILE *pDttsp;

    protected:

        virtual void changeDcOsc () {}
        virtual void changeExtOsc ();
        virtual void changeSwOsc ();

    public:
        FrequencyPMSDR ( int sampleRate, FILE *pPmSdrFile, FILE *pDttSpFile );
        virtual ~FrequencyPMSDR ();
};


class DttSPcmd;

class FrequencyPMSDRudp: public FrequencyExtOsc {

    private:
        FILE *pPmsdr;
        DttSPcmd *cp;

    protected:

        virtual void changeDcOsc () {}
        virtual void changeExtOsc ();
        virtual void changeSwOsc ();

    public:
        FrequencyPMSDRudp ( int sampleRate, FILE *pPmSdrFile, DttSPcmd *pCmd );
        virtual ~FrequencyPMSDRudp ();
};


class FrequencyPMSDRwithDcUdp: public FrequencyExtOscWithDc {

    private:
        FILE *pPmsdr;
        DttSPcmd *cp;

    protected:

        virtual void changeDcOsc ();
        virtual void changeExtOsc ();
        virtual void changeSwOsc ();

    public:
        FrequencyPMSDRwithDcUdp ( int sampleRate, FILE *pPmSdrFile, DttSPcmd *pCmd );
        virtual ~FrequencyPMSDRwithDcUdp ();
};







#endif
