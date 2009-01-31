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

        virtual void changeExtOsc () = 0;
        virtual void changeSwOsc ()  = 0;

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

        FrequencyExtOsc ( int sampleRate, int startFreq = 0, int fExtOscMax = 0, int fExtOscMin = 0 );
        virtual ~FrequencyExtOsc ();
       

        virtual void set ( int );      // absolute change
        virtual int  get ( ) const ;

        virtual void change ( int );   // relative change

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

        virtual void changeExtOsc ();
        virtual void changeSwOsc ();

    public:
        FrequencyPMSDR ( int sampleRate, FILE *pPmSdrFile, FILE *pDttSpFile );
        virtual ~FrequencyPMSDR ();
};






#endif
