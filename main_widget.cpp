#include "main_widget.h"
#include "switches.h"
#include "modes.h"
#include "agc.h"

#include "meter1.xpm"
#include "mhz.xpm"
#include "rx.xpm"
#include "tx.xpm"
#include "logo.xpm"
#include "text.h"
#include "dttsp.h"

/* formatting note: in vi :set tab=4 */

Main_Widget::Main_Widget(QWidget *parent)
        : QWidget(parent)
{
	setFocusPolicy ( Qt::TabFocus );
	setMinimumWidth ( 650 );
	setMinimumHeight ( 300 );

	//setAttribute( Qt::WA_OpaquePaintEvent, true);
	setAttribute( Qt::WA_NoSystemBackground, true);
    setPalette(QPalette(QColor(0, 0, 0)));
    setAutoFillBackground(true);
    setMinimumWidth( 650 );
    setMinimumHeight( 300 );
	initConstants();
	slopeTuneOffset = 0;
	loadSettings();
	bin_bw = sample_rate / 4096.0;

	setWindowTitle("SDR-Shell 4.15 @ " + stationCallsign );

	font1PointSize = 14;
	font1 = new QFont ( "Andale Mono", font1PointSize, FALSE );
	font1Metrics = new QFontMetrics ( *font1 );
	
	fontlcdPointSize = 15;
	fontlcd = new QFont ( "Andald Mono", fontlcdPointSize, FALSE );
	fontlcdMetrics = new QFontMetrics ( *fontlcd );

	//printf( "Font %d %d\n", font1Metrics->ascent(), font1Metrics->descent());

	// Remediate font pathology
	for ( int i = font1PointSize; font1Metrics->ascent() > 11; i-- )
	{
		printf ( "Adjusting font... Ascent %d\n", font1Metrics->ascent() );
		font1->setPointSize ( i );
		delete font1Metrics;
		font1Metrics = new QFontMetrics ( *font1 );
	}
	
	/*for ( int i = fontlcdPointSize; fontlcdMetrics->ascent() > 18; i-- )
	{
		printf ( "Adjusting font... Ascent %d\n", fontlcdMetrics->ascent() );
		fontlcd->setPointSize ( i );
		delete fontlcdMetrics;
		fontlcdMetrics = new QFontMetrics ( *fontlcd );
	}*/

	font2PointSize = 16;
	font2 = new QFont( "Andale Mono", font2PointSize, FALSE);
    font2Metrics = new QFontMetrics( *font2 );
	QColor borderColor ( 255, 200, 55 );

	// -----------------------------------------------------------------------
	// Spectrogram

	spectrogramFrame = new QFrame ( this );

	spectrogram = new Spectrogram ( spectrogramFrame );
	connect ( spectrogram, SIGNAL ( tune1 ( int ) ),
	          this, SLOT ( spectrogramClicked ( int ) ) );
        //This seems to occasionally cause a seg fault (just above).  Rob 10/3/10
	connect ( spectrogram, SIGNAL ( tune2 ( int ) ),
	          this, SLOT ( tune ( int ) ) );
	connect ( spectrogram, SIGNAL ( movement ( int ) ),
	          this, SLOT ( f_at_mousepointer ( int ) ) );

	// -----------------------------------------------------------------------
	// Pass Band Filter Scale
	pbscale = new PassBandScale( spectrogramFrame );
	connect ( pbscale, SIGNAL ( set_lower_pb ( int ) ),
	          this, SLOT ( setLowerFilterScale ( int ) ) );
	connect ( pbscale, SIGNAL ( set_upper_pb ( int ) ),
	          this, SLOT ( setUpperFilterScale ( int ) ) );
	connect ( pbscale, SIGNAL ( movement ( int ) ),
	          this, SLOT ( displayNCO ( int ) ) );

	// -----------------------------------------------------------------------
	// Spectrum
	spectrumFrame = new Spectrum( spectrogramFrame );
	connect ( spectrumFrame, SIGNAL ( movement ( int ) ),
	          this, SLOT ( f_at_mousepointer ( int ) ) );
	connect ( spectrumFrame, SIGNAL ( tune ( int ) ),
	          this, SLOT ( spectrogramClicked ( int ) ) );
	connect ( spectrumFrame, SIGNAL ( tune2 ( int ) ),
	          this, SLOT ( tune ( int ) ) );

	// -----------------------------------------------------------------------
	// Control Frame

	// Top
	ctlFrame = new QFrame ( this, Qt::Widget );
	ctlFrame->setFrameStyle ( QFrame::StyledPanel | QFrame::Plain );
    ctlFrame->setPalette(QColor(0, 255, 0) );
    ctlFrame->setAutoFillBackground(true);

	// Bottom
	ctlFrame2 = new QFrame ( this, Qt::Widget );
	ctlFrame2->setFrameStyle ( QFrame::StyledPanel | QFrame::Plain );
    ctlFrame2->setPalette(QPalette(QColor(0, 255, 0)));
    ctlFrame2->setAutoFillBackground(true);

	// -----------------------------------------------------------------------
	// Configuration Frame

	cfgFrame = new QFrame();
	cfgFrame->setGeometry ( 50, 50, 380, 400 );
	cfgFrame->setMinimumWidth ( 380 );
	cfgFrame->setMaximumWidth ( 380 );
	cfgFrame->setMinimumHeight ( 500 );
	cfgFrame->setMaximumHeight ( 500 );
    cfgFrame->setWindowTitle("SDR-Shell : Config ");

	QFrame *cfgFrame1 = new QFrame( cfgFrame );
	QFrame *cfgFrame2 = new QFrame( cfgFrame );
	QFrame *cfgFrame3 = new QFrame( cfgFrame );
	QFrame *cfgFrame6 = new QFrame( cfgFrame );
	QFrame *cfgFrame7 = new QFrame( cfgFrame );
	QFrame *cfgFrame4 = new QFrame( cfgFrame );
	QFrame *cfgFrame5 = new QFrame( cfgFrame );

	QTabWidget *configTab = new QTabWidget ( cfgFrame );
	configTab->setFont ( *font1 );
	configTab->addTab ( cfgFrame1, "General" );
	configTab->addTab ( cfgFrame2, "IQ" );
	configTab->addTab ( cfgFrame3, "Spectrum" );
	configTab->addTab ( cfgFrame6, "IF" );
	configTab->addTab ( cfgFrame7, "TX" );
	configTab->addTab ( cfgFrame4, "Help" );
	configTab->addTab ( cfgFrame5, "About" );
	configTab->setGeometry ( 2, 2,
	                         cfgFrame->width() - 4,
	                         cfgFrame->height() - 2 );

	// Station Callsign
	QGroupBox *cfgCallBox = new QGroupBox ( cfgFrame1 );
	cfgCallBox->setTitle ( "Station Callsign" );
	cfgCallBox->setGeometry( 5, 5, 330, 45 );

	cfgCallInput = new QLineEdit ( cfgCallBox );
	cfgCallInput->setGeometry ( 10, 18, 120, 20 );
	cfgCallInput->setText ( stationCallsign );

	QPushButton *updateCallButton = new QPushButton ( cfgCallBox );
	updateCallButton->setText ( "Update" );
	updateCallButton->setGeometry ( 140, 18, 70, 20 );
	connect ( updateCallButton, SIGNAL ( pressed() ),
	          this, SLOT ( updateCallsign() ) );

	// Local Oscilator Frequency in Hertz
	QGroupBox *cfgLOFreqBox = new QGroupBox ( cfgFrame1 );
	cfgLOFreqBox->setTitle ( "LO Frequency (Hz)" );
	cfgLOFreqBox->setGeometry( 5, 53, 330, 90 );

	cfgLOFreqInput = new QLineEdit ( cfgLOFreqBox );
	cfgLOFreqInput->setGeometry( 10, 37, 120, 20 );
	cfgLOFreqInput->setText ( rx_f_string );
    cfgLOFreqInput->setEnabled(true);

	updateLOFreqButton = new QPushButton( cfgLOFreqBox );
	updateLOFreqButton->setText ( "Update" );
	updateLOFreqButton->setGeometry( 140, 37, 70, 20 );
    updateLOFreqButton->setEnabled(true);
	connect ( updateLOFreqButton, SIGNAL ( pressed() ),
	          this, SLOT ( updateLOFreq() ) );

	// Use USBSoftrock
	QGroupBox *cfgUSBBox = new QGroupBox ( cfgFrame1 );
	cfgUSBBox->setTitle ( "USBSoftrock Control" );
    cfgUSBBox->setGeometry ( 5, 143, 330, 90 );

	QRadioButton *cfgUseUSBsoftrock = new QRadioButton ( cfgUSBBox );
	cfgUseUSBsoftrock->setText ( "Use usbsoftrock via UDP" );
	cfgUseUSBsoftrock->setGeometry ( 25, 18, 200, 20 );
    cfgUseUSBsoftrock->setAutoExclusive(false);
	connect ( cfgUseUSBsoftrock, SIGNAL ( toggled ( bool ) ),
	          this, SLOT ( updateUseUSBsoftrock ( bool ) ) );
	if( !rock_bound)
		cfgUseUSBsoftrock->setChecked (true);

	// USBSoftrock 5/4 tuning for dual-conversion
	QRadioButton *cfgDualConversion = new QRadioButton ( cfgUSBBox );
	cfgDualConversion->setText ( "5/4 Tuning" );
	cfgDualConversion->setGeometry ( 25, 36, 200, 20 );
    cfgDualConversion->setAutoExclusive(false);
	connect ( cfgDualConversion, SIGNAL ( toggled ( bool ) ),
	          this, SLOT ( updateDualConversion ( bool ) ) );
	if( dualConversion)
		cfgDualConversion->setChecked (true);

	// USB-tune offset
	cfgTuneOffsetInput = new QLineEdit ( cfgUSBBox );
	cfgTuneOffsetInput->setGeometry( 25, 60, 120, 20 );
	TuneOffset_string.sprintf("%d", tuneCenter);
	cfgTuneOffsetInput->setText ( TuneOffset_string );
    cfgTuneOffsetInput->setEnabled(true);

	updateTuneOffsetButton = new QPushButton( cfgUSBBox );
	updateTuneOffsetButton->setText ( "Update" );
	updateTuneOffsetButton->setGeometry( 150, 60, 70, 20 );
    updateTuneOffsetButton->setEnabled(true);
	connect ( updateTuneOffsetButton, SIGNAL ( pressed() ),
	          this, SLOT ( updateTuneOffset() ) );

	// Spectrum and S-Meter calibration
	QGroupBox *calibrationBox = new QGroupBox ( cfgFrame1 );
	calibrationBox->setTitle ( "Calibration" );
    //calibrationBox->setGeometry( 5, 209, 330, 45 );
    calibrationBox->setGeometry( 5, 250, 330, 45 );
	///calibrationBox->setFrameStyle ( QFrame::StyledPanel | QFrame::Plain );

	QLabel *specCalLabel = new QLabel ( calibrationBox );
	specCalLabel->setText ( "Spectrum: " );
	specCalLabel->setGeometry ( 10, 18, 70, 20 );
	specCalLabel->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );

	specCalSpinBox = new QSpinBox ( calibrationBox );
	specCalSpinBox->setGeometry ( 80, 18, 70, 20 );
	specCalSpinBox->setMinimum( 0 );
	specCalSpinBox->setMaximum( 100 );
	specCalSpinBox->setValue ( ( int ) specCal );
	connect ( specCalSpinBox, SIGNAL ( valueChanged ( int ) ),
	          this, SLOT ( calibrateSpec ( int ) ) );

	QLabel *metrCalLabel = new QLabel ( calibrationBox );
	metrCalLabel->setText ( "S-Meter: " );
	metrCalLabel->setGeometry ( 175, 18, 70, 20 );
	metrCalLabel->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );

	metrCalSpinBox = new QSpinBox ( calibrationBox );
	metrCalSpinBox->setGeometry ( 245, 18, 70, 20 );
	metrCalSpinBox->setMinimum( 0 );
	metrCalSpinBox->setMaximum( 100 );
	metrCalSpinBox->setValue ( ( int ) metrCal );
	connect ( metrCalSpinBox, SIGNAL ( valueChanged ( int ) ),
	          this, SLOT ( calibrateMetr ( int ) ) );

	// IQ Phase Calibration
	QGroupBox *cfgIQCal = new QGroupBox ( cfgFrame2 );
	cfgIQCal->setTitle ( "RX IQ Calibration" );
	cfgIQCal->setGeometry( 5, 5, 330, 45 );

#if 0
	QLabel *cfgIQPhaseLabel = new QLabel ( cfgIQCal );
	cfgIQPhaseLabel->setText ( "IQ Phase: " );
	cfgIQPhaseLabel->setGeometry ( 10, 18, 70, 20 );
	cfgIQPhaseLabel->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );

	cfgIQPhaseInput = new QSpinBox ( cfgIQCal );
	cfgIQPhaseInput->setGeometry ( 80, 18, 70, 20 );
	cfgIQPhaseInput->setMinimum( -1000 );
	cfgIQPhaseInput->setMaximum( 1000 );
	cfgIQPhaseInput->setValue ( iqPhase );
	connect ( cfgIQPhaseInput, SIGNAL ( valueChanged ( int ) ),
	          this, SLOT ( updateIQPhase ( int ) ) );
#endif

	// IQ Gain Calibration
	QLabel *cfgIQGainLabel = new QLabel ( cfgIQCal );
	cfgIQGainLabel->setText ( "IQ Gain: " );
	cfgIQGainLabel->setGeometry ( 175, 18, 70, 20 );
	cfgIQGainLabel->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );

	cfgIQGainInput = new QSpinBox ( cfgIQCal );
	cfgIQGainInput->setGeometry ( 245, 18, 70, 20 );
	cfgIQGainInput->setMinimum( -1000 );
	cfgIQGainInput->setMaximum( 1000 );
	cfgIQGainInput->setValue ( iqGain );
	connect ( cfgIQGainInput, SIGNAL ( valueChanged ( int ) ),
	          this, SLOT ( updateIQGain ( int ) ) );

	// Transmit IQgain, IQphase
	QGroupBox *cfgTxIQCal = new QGroupBox ( cfgFrame2 );
	cfgTxIQCal->setTitle ( "TX IQ Calibration" );
	cfgTxIQCal->setGeometry ( 5, 50, 340, 45 );

#if 0
	QLabel *cfgTxIQPhaseLabel = new QLabel ( cfgTxIQCal );
	cfgTxIQPhaseLabel->setText ( "IQ Phase: " );
	cfgTxIQPhaseLabel->setGeometry ( 10, 18, 70, 20 );
	cfgTxIQPhaseLabel->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );

	cfgTxIQPhaseInput = new QSpinBox ( cfgTxIQCal );
	cfgTxIQPhaseInput->setGeometry ( 80, 18, 70, 20 );
	cfgTxIQPhaseInput->setMinimum ( -1000 );
	cfgTxIQPhaseInput->setMaximum ( 1000 );
	cfgTxIQPhaseInput->setValue ( txIQPhase );
	connect ( cfgTxIQPhaseInput, SIGNAL ( valueChanged ( int ) ),
	          this, SLOT ( updateTxIQPhase ( int ) ) );
#endif

	// IQ Gain Calibration
	QLabel *cfgTxIQGainLabel = new QLabel ( cfgTxIQCal );
	cfgTxIQGainLabel->setText ( "IQ Gain: " );
	cfgTxIQGainLabel->setGeometry ( 175, 18, 70, 20 );
	cfgTxIQGainLabel->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );

	cfgTxIQGainInput = new QSpinBox ( cfgTxIQCal );
	cfgTxIQGainInput->setGeometry ( 245, 18, 70, 20 );
	cfgTxIQGainInput->setMinimum ( -1000 );
	cfgTxIQGainInput->setMaximum ( 1000 );
	cfgTxIQGainInput->setValue ( txIQGain );
	connect ( cfgTxIQGainInput, SIGNAL ( valueChanged ( int ) ),
	          this, SLOT ( updateTxIQGain ( int ) ) );

	// Polyphase FFT
	QGroupBox *cfgPolyFFTGroup = new QGroupBox( cfgFrame3 );
    cfgPolyFFTGroup->setTitle("Polyphase FFT");	
	cfgPolyFFTGroup->setGeometry( 5, 5, 330, 45 );

    polyFFT_button = new QRadioButton( tr("Use Polyphase FFT"),cfgPolyFFTGroup);
    polyFFT_button->setGeometry ( 10, 18, 200, 20 );
    connect( polyFFT_button, SIGNAL(clicked()),
			 this, SLOT ( setPolyFFT () ) );
	if (polyphaseFFT)
    	polyFFT_button->setChecked(true);

	// Spectrum Type
	QGroupBox *cfgSpecTypeGroup = new QGroupBox( cfgFrame3 );
	cfgSpecTypeGroup->setTitle ( "Spectrum Type" );
	cfgSpecTypeGroup->setGeometry( 5, 53, 330, 45 );

	preFilter_button = new QRadioButton( tr("Pre Filter"), cfgSpecTypeGroup );
	preFilter_button->setGeometry ( 10, 18, 100, 20 );
	connect( preFilter_button, SIGNAL(clicked()),
			 this, SLOT(setSpectrumType()) );
	if (spectrumType == 1)
        preFilter_button->setChecked(true);	

	postFilter_button = new QRadioButton( tr("Post Filter"), cfgSpecTypeGroup );
	postFilter_button->setGeometry ( 160, 18, 100, 20 );
	connect( postFilter_button, SIGNAL(clicked()),
			 this, SLOT(setSpectrumType()) );
	if (spectrumType == 2)
        postFilter_button->setChecked(true);	


	// FFT Windowing Functionplot
	QGroupBox *cfgFFTWindowGroup = new QGroupBox( cfgFrame3 );
	cfgFFTWindowGroup->setTitle( "FFT Windowing Function" );
	cfgFFTWindowGroup->setGeometry( 5, 101, 330, 162 );

	fftWindow_0 = new QRadioButton( tr("Rectangular"), cfgFFTWindowGroup );
	fftWindow_0->setGeometry( 10, 18, 140, 19 );
	connect( fftWindow_0, SIGNAL(clicked()),
			 this, SLOT(setFFTWindow()) );
	if (fftWindow == 0)
        fftWindow_0->setChecked(true);	
			 
	fftWindow_1 = new QRadioButton( tr("Hanning"), cfgFFTWindowGroup );
	fftWindow_1->setGeometry( 10, 38, 140, 19 );
	connect( fftWindow_1, SIGNAL(clicked()),
			 this, SLOT(setFFTWindow()) );	
	if (fftWindow == 1)
        fftWindow_1->setChecked(true);	

	fftWindow_2 = new QRadioButton( tr("Welch"), cfgFFTWindowGroup );
	fftWindow_2->setGeometry( 10, 58, 140, 19 );
	connect( fftWindow_2, SIGNAL(clicked()),
			 this, SLOT(setFFTWindow()) );	
	if (fftWindow == 2)
        fftWindow_2->setChecked(true);	

	fftWindow_3 = new QRadioButton( tr("Parzen"), cfgFFTWindowGroup );
	fftWindow_3->setGeometry( 10, 78, 140, 19 );
	connect( fftWindow_3, SIGNAL(clicked()),
			 this, SLOT(setFFTWindow()) );
	if (fftWindow == 3)
        fftWindow_3->setChecked(true);	

	fftWindow_4 = new QRadioButton( tr("Bartlett"), cfgFFTWindowGroup );
	fftWindow_4->setGeometry( 10, 98, 140, 19 );
	connect( fftWindow_4, SIGNAL(clicked()),
			 this, SLOT(setFFTWindow()) );
	if (fftWindow == 4)
        fftWindow_4->setChecked(true);	

	fftWindow_5 = new QRadioButton( tr("Hamming"), cfgFFTWindowGroup );
	fftWindow_5->setGeometry( 10, 118, 140, 19 );
	connect( fftWindow_5, SIGNAL(clicked()),
			 this, SLOT(setFFTWindow()) );
	if (fftWindow == 5)
        fftWindow_5->setChecked(true);	

	fftWindow_6 = new QRadioButton( tr("Blackman 2"), cfgFFTWindowGroup );
	fftWindow_6->setGeometry( 10, 138, 140, 19 );
	connect( fftWindow_6, SIGNAL(clicked()),
			 this, SLOT(setFFTWindow()) );
	if (fftWindow == 6)
        fftWindow_6->setChecked(true);	

	fftWindow_7 = new QRadioButton( tr("Blackman 3"), cfgFFTWindowGroup );
	fftWindow_7->setGeometry( 160, 18, 140, 19 );
	connect( fftWindow_7, SIGNAL(clicked()),
			 this, SLOT(setFFTWindow()) );
	if (fftWindow == 7)
        fftWindow_7->setChecked(true);	

	fftWindow_8 = new QRadioButton( tr("Blackman 4"), cfgFFTWindowGroup );
	fftWindow_8->setGeometry( 160, 38, 140, 19 );
	connect( fftWindow_8, SIGNAL(clicked()),
			 this, SLOT(setFFTWindow()) );
	if (fftWindow == 8)
        fftWindow_8->setChecked(true);	

	fftWindow_9 = new QRadioButton( tr("Exponential"), cfgFFTWindowGroup );
	fftWindow_9->setGeometry( 160, 58, 140, 19 );
	connect( fftWindow_9, SIGNAL(clicked()),
			 this, SLOT(setFFTWindow()) );
	if (fftWindow == 9)
        fftWindow_9->setChecked(true);	

	fftWindow_10 = new QRadioButton( tr("Riemann"), cfgFFTWindowGroup );
	fftWindow_10->setGeometry( 160, 78, 140, 19 );
	connect( fftWindow_10, SIGNAL(clicked()),
			 this, SLOT(setFFTWindow()) );
	if (fftWindow == 10)
        fftWindow_10->setChecked(true);	

	fftWindow_11 = new QRadioButton( tr("Blackman-Harris"), cfgFFTWindowGroup );
	fftWindow_11->setGeometry( 160, 98, 140, 19 );
	connect( fftWindow_11, SIGNAL(clicked()),
			 this, SLOT(setFFTWindow()) );
	if (fftWindow == 11)
        fftWindow_11->setChecked(true);	

	fftWindow_12 = new QRadioButton( tr("Nuttall"), cfgFFTWindowGroup );
	fftWindow_12->setGeometry( 160, 118, 140, 19 );
	connect( fftWindow_12, SIGNAL(clicked()),
			 this, SLOT(setFFTWindow()) );
	if (fftWindow == 12)
        fftWindow_12->setChecked(true);	


	// IF Settings
	QGroupBox *cfgIfBox = new QGroupBox ( cfgFrame6 );
	cfgIfBox->setTitle ( "IF Setup" );
	cfgIfBox->setGeometry ( 5, 160, 340, 50 );

	QRadioButton *cfgIFAligned = new QRadioButton ( cfgIfBox );
	cfgIFAligned->setText ( "Use IF Mode" );
	cfgIFAligned->setGeometry ( 5, 5, 340, 45 );
    cfgDualConversion->setAutoExclusive(false);
	connect ( cfgIFAligned, SIGNAL ( toggled ( bool ) ),
	          this, SLOT ( setIF ( bool ) ) );
	if (useIF)
		cfgIFAligned->setChecked (true);

	QGroupBox *cfgIFreqBox = new QGroupBox ( cfgFrame6 );
	cfgIFreqBox->setTitle ( "IF Frequencies (Hz)" );
	cfgIFreqBox->setGeometry ( 5, 353, 340, 110 );

	cfgIFreqInput = new QLineEdit ( cfgIFreqBox );
	cfgIFreqInput->setGeometry ( 10, 25, 120, 20 );
	cfgIFreqInput->setText ( rx_if_string );

	QPushButton *updateIFreqButton = new QPushButton ( cfgIFreqBox );
	updateIFreqButton->setText ( "Update" );
	updateIFreqButton->setGeometry ( 140, 25, 70, 20 );
	connect ( updateIFreqButton, SIGNAL ( pressed() ),
	          this, SLOT ( updateIFreq() ) );

	QGroupBox *cfgSlopeTuneBox = new QGroupBox ( cfgFrame6 );
	cfgSlopeTuneBox->setTitle ( "Slope Tuning Offsets (Hz)" );
	cfgSlopeTuneBox->setGeometry ( 5, 195, 340, 150 );

	QLabel *cfgSlopeLowOffsetLabel = new QLabel ( cfgSlopeTuneBox );
	cfgSlopeLowOffsetLabel->setText ( "Low Offset" );
	cfgSlopeLowOffsetLabel->setGeometry ( 10, 28, 120, 20 );
	cfgSlopeLowOffsetLabel->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );

	cfgSlopeLowOffsetInput = new QSpinBox ( cfgSlopeTuneBox );
	cfgSlopeLowOffsetInput->setGeometry ( 180, 28, 70, 20 );
	cfgSlopeLowOffsetInput->setMinimum ( 0 );
	cfgSlopeLowOffsetInput->setMaxValue ( 9999 );
	cfgSlopeLowOffsetInput->setValue ( slopeLowOffset );
	connect ( cfgSlopeLowOffsetInput, SIGNAL ( valueChanged ( int ) ),
	          this, SLOT ( updateSlopeLowOffset ( int ) ) );

	QLabel *cfgSlopeHighOffsetLabel = new QLabel ( cfgSlopeTuneBox );
	cfgSlopeHighOffsetLabel->setText ( "High Offset" );
	cfgSlopeHighOffsetLabel->setGeometry ( 10, 58, 120, 20 );
	cfgSlopeHighOffsetLabel->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );

	cfgSlopeHighOffsetInput = new QSpinBox ( cfgSlopeTuneBox );
	cfgSlopeHighOffsetInput->setGeometry ( 180, 58, 70, 20 );
	cfgSlopeHighOffsetInput->setMinimum ( 0 );
	cfgSlopeHighOffsetInput->setMaxValue ( 9999 );
	cfgSlopeHighOffsetInput->setValue ( slopeHighOffset );
	connect ( cfgSlopeHighOffsetInput, SIGNAL ( valueChanged ( int ) ),
	          this, SLOT ( updateSlopeHighOffset ( int ) ) );
	
	QRadioButton *cfgUseSlopeTune = new QRadioButton ( cfgSlopeTuneBox );
	cfgUseSlopeTune->setText ( "Poll Slope Tuning" );
	cfgUseSlopeTune->setGeometry ( 40, 88, 200, 45 );
	cfgUseSlopeTune->setChecked ( useSlopeTune );
	connect ( cfgUseSlopeTune, SIGNAL ( toggled ( bool ) ),
		  this, SLOT ( setSlopeTune ( bool ) ) );


	QLabel *cfgUSBOffsetLabel = new QLabel ( cfgIFreqBox );
	cfgUSBOffsetLabel->setText ( "USB Offset" );
	cfgUSBOffsetLabel->setGeometry ( 10, 53, 70, 20 );
	cfgUSBOffsetLabel->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );

	cfgUSBOffsetInput = new QSpinBox ( cfgIFreqBox );
	cfgUSBOffsetInput->setGeometry ( 90, 53, 70, 20 );
	cfgUSBOffsetInput->setMinimum ( 0 );
	cfgUSBOffsetInput->setMaximum ( 9999 );
	cfgUSBOffsetInput->setValue ( usbOffset );
	connect ( cfgUSBOffsetInput, SIGNAL ( valueChanged ( int ) ),
	          this, SLOT ( updateUSBOffset ( int ) ) );

	QLabel *cfgLSBOffsetLabel = new QLabel ( cfgIFreqBox );
	cfgLSBOffsetLabel->setText ( "LSB Offset" );
	cfgLSBOffsetLabel->setGeometry ( 10, 83, 70, 20 );
	cfgLSBOffsetLabel->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );

	cfgLSBOffsetInput = new QSpinBox ( cfgIFreqBox );
	cfgLSBOffsetInput->setGeometry ( 90, 83, 70, 20 );
	cfgLSBOffsetInput->setMinimum ( 0 );
	cfgLSBOffsetInput->setMaximum ( 9999 );
	cfgLSBOffsetInput->setValue ( lsbOffset );
	connect ( cfgLSBOffsetInput, SIGNAL ( valueChanged ( int ) ),
	          this, SLOT ( updateLSBOffset ( int ) ) );


	//Hamlib Setup
	QGroupBox *cfgHamlibBox = new QGroupBox ( cfgFrame6 );
	cfgHamlibBox->setTitle ( "Hamlib Setup" );
	cfgHamlibBox->setGeometry ( 5, 5, 340, 150 );

	cfgHamlibRigInput = new QLineEdit ( cfgHamlibBox );
	cfgHamlibRigInput->setGeometry ( 10, 18, 120, 20 );
	cfgHamlibRigInput->setText ( rigString );

	cfgRigLabel = new QLabel ( cfgHamlibBox );
	cfgRigLabel->setGeometry ( 140, 18, 180, 20 );
	cfgRigLabel->setText ( "Rig ID" );

	cfgHamlibPortInput = new QLineEdit ( cfgHamlibBox );
	cfgHamlibPortInput->setGeometry ( 10, 48, 120, 20 );
	cfgHamlibPortInput->setText ( portString );

	cfgPortLabel = new QLabel ( cfgHamlibBox );
	cfgPortLabel->setGeometry ( 140, 48, 180, 20 );
	cfgPortLabel->setText ( "Serial Port" );

	cfgHamlibSpeedInput = new QLineEdit ( cfgHamlibBox );
	cfgHamlibSpeedInput->setGeometry ( 10, 78, 120, 20 );
	cfgHamlibSpeedInput->setText ( speedString );

	cfgSpeedLabel = new QLabel ( cfgHamlibBox );
	cfgSpeedLabel->setGeometry ( 140, 78, 180, 20 );
	cfgSpeedLabel->setText ( "Baud Rate" );

	QPushButton *updateHamlibButton = new QPushButton ( cfgHamlibBox );
	updateHamlibButton->setText ( "Update Hamlib \n Parameters" );
	updateHamlibButton->setGeometry ( 230, 28, 100, 50 );
	connect ( updateHamlibButton, SIGNAL ( pressed() ),
	          this, SLOT ( updateHamlib() ) );

    QRadioButton *cfgUseHamlib = new QRadioButton ( cfgHamlibBox );
	cfgUseHamlib->setText ( "Use Hamlib" );
    cfgUseHamlib->setAutoExclusive(FALSE);
	cfgUseHamlib->setGeometry ( 5, 100, 150, 45 );
	connect ( cfgUseHamlib, SIGNAL ( toggled ( bool ) ),
	          this, SLOT ( setHamlib ( bool ) ) );
	connect ( this, SIGNAL ( toggleHamlibButton ( bool ) ), 
		  cfgUseHamlib, SLOT ( setChecked ( bool ))); 
	
    QRadioButton *cfgMuteXmitHamlib = new QRadioButton ( cfgHamlibBox );
	cfgMuteXmitHamlib->setText ( "Mute in Xmit" );
    cfgMuteXmitHamlib->setAutoExclusive(FALSE);
	cfgMuteXmitHamlib->setGeometry ( 150, 100, 180, 45 );
	cfgMuteXmitHamlib->setChecked ( useHamlib );
	connect ( cfgMuteXmitHamlib, SIGNAL ( toggled ( bool ) ),
		  this, SLOT ( setMuteXmit ( bool ) ) );

	// Transmit Tab
	// Transmit Enable
	QGroupBox *cfgTXBox = new QGroupBox ( cfgFrame7 );
	cfgTXBox->setTitle ( "Transmit" );
	cfgTXBox->setGeometry ( 5, 5, 340, 45 );

	QRadioButton *cfgTransmit = new QRadioButton ( cfgTXBox );
	cfgTransmit->setText ( "Enable transmit" );
	cfgTransmit->setGeometry ( 25, 18, 200, 20 );
	connect ( cfgTransmit, SIGNAL ( toggled ( bool ) ),
	          this, SLOT ( updateTransmit ( bool ) ) );
	cfgTransmit->setChecked ( enableTransmit );

	// TX Microphone Gain and Output Gain
	QGroupBox *cfgTxGain = new QGroupBox ( cfgFrame7 );
	cfgTxGain->setTitle ( "TX Gain" );
	cfgTxGain->setGeometry ( 5, 50, 340, 45 );

	QLabel *cfgTxMicGainLabel = new QLabel ( cfgTxGain );
	cfgTxMicGainLabel->setText ( "Mic Gain: " );
	cfgTxMicGainLabel->setGeometry ( 10, 18, 70, 20 );
	cfgTxMicGainLabel->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );

	cfgTxMicGainInput = new QSpinBox ( cfgTxGain );
	cfgTxMicGainInput->setGeometry ( 80, 18, 70, 20 );
	cfgTxMicGainInput->setMinimum ( -255 );
	cfgTxMicGainInput->setMaximum ( 255 );
	cfgTxMicGainInput->setValue ( micGain );
	connect ( cfgTxMicGainInput, SIGNAL ( valueChanged ( int ) ),
	          this, SLOT ( updateTxMicGain ( int ) ) );

	QLabel *cfgTxOutputGainLabel = new QLabel ( cfgTxGain );
	cfgTxOutputGainLabel->setText ( "Out Gain: " );
	cfgTxOutputGainLabel->setGeometry ( 175, 18, 70, 20 );
	cfgTxOutputGainLabel->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );

	cfgTxOutputGainInput = new QSpinBox ( cfgTxGain );
	cfgTxOutputGainInput->setGeometry ( 245, 18, 70, 20 );
	cfgTxOutputGainInput->setMinimum ( -255 );
	cfgTxOutputGainInput->setMaximum ( 255 );
	cfgTxOutputGainInput->setValue ( txGain );
	connect ( cfgTxOutputGainInput, SIGNAL ( valueChanged ( int ) ),
	          this, SLOT ( updateTxOutputGain ( int ) ) );

	
	// About
	QTextEdit *helpText = new QTextEdit ( cfgFrame4 );
	helpText->setGeometry ( 2, 2, 347, 462 );
	helpText->setReadOnly ( true );
	helpText->append ( helptext );

	// About
	QTextEdit *aboutText = new QTextEdit ( cfgFrame5 );
	aboutText->setGeometry ( 2, 2, 347, 262 );
	helpText->setWordWrapMode(QTextOption::WordWrap);
	helpText->setLineWrapMode(QTextEdit::WidgetWidth);
	aboutText->setReadOnly ( true );
	aboutText->append (
	    QString ( "<center><br>SDR-Shell Version 4.15<br>" ) +
	    "Copyright (c) 2006, 2007 & 2009<br>" +
	    "Edson Pereira, PU1JTE, N1VTN<br>" +
	    "ewpereira@gmail.com<br>" +
	    "Rob Frohne, KL7NA<br>" +
	    "kl7na@arrl.net<br></center>" );
	aboutText->append (
	    QString ( "This program is free software. You can redistribute " ) +
	    "it and/or modify it under the terms and conditions of the GNU " +
	    "General Public License Version 2 as published by the Free " +
	    "Software Foundation.\n\n" );

	// -----------------------------------------------------------------------
	// S meter
	QPixmap *meter1_pix = new QPixmap ( meter1_xpm );
	signalFrame = new QFrame ( ctlFrame );
    p = signalFrame->palette();
    p.setBrush(QPalette::Window, *meter1_pix);
    //?? p.setColor(QPalette::Window, Qt::black);
    signalFrame->setPalette(p); 
    signalFrame->setAutoFillBackground( true );       

	for ( int i = 0; i < 34; i++ )
	{
		signalBargraph[i] = new QFrame ( signalFrame );
        signalBargraph[i]->setPalette( QColor( 50, 50, 50 ) );
		signalBargraph[i]->setGeometry ( 3 + 4 * i, 3, 3, 9 );
		//signalBargraph[i]->setFrameStyle ( QFrame::NoFrame | QFrame::Plain );
        signalBargraph[i]->setAutoFillBackground(true);        
		signalBargraph[i]->show();
	}

    signal_dBm = new QLabel( "-174", signalFrame );    
	signal_dBm->setFont ( *font1 );
    p = signal_dBm->palette();
    p.setColor(QPalette::Window, Qt::black);
    p.setColor(QPalette::Active, QPalette::WindowText, QColor(255, 255, 255) );
    signal_dBm->setPalette(p);

    QLabel *dBmLabel = new QLabel( " dBm", signalFrame );    
	dBmLabel->setFont ( *font1 );
    p = dBmLabel->palette();
    p.setColor(QPalette::Window, Qt::black);
    p.setColor(QPalette::Active, QPalette::WindowText, QColor(255, 255, 255) );
    dBmLabel->setPalette(p);

    //Frequency display
    lcd = new QLCDNumber( 12, ctlFrame );
	lcd->setFrameStyle ( QFrame::NoFrame );
	lcd->setSegmentStyle( QLCDNumber::Filled );
    p = lcd->palette();
    //p.setColor(QPalette::Window, Qt::black);
    //p.setColor(QPalette::Active, QPalette::WindowText, QColor(255, 255, 255) );
    p.setColor(QPalette::WindowText, QColor(255, 255, 255) );
    lcd->setPalette(p);
    lcd->setAutoFillBackground(true);

	step_1Hz_frame = new QFrame ( ctlFrame );
	step_1Hz_frame->setGeometry ( 318, 29, 11, 1 );

	step_10Hz_frame = new QFrame ( ctlFrame );
	step_10Hz_frame->setGeometry ( 305, 29, 11, 1 );

	step_100Hz_frame = new QFrame ( ctlFrame );
	step_100Hz_frame->setGeometry ( 292, 29, 11, 1 );

	step_1KHz_frame = new QFrame ( ctlFrame );
	step_1KHz_frame->setGeometry ( 279, 29, 11, 1 );
	
	step_10KHz_frame = new QFrame ( ctlFrame );
	step_10KHz_frame->setGeometry ( 266, 29, 11, 1 );
	
	step_100KHz_frame = new QFrame ( ctlFrame );
	step_100KHz_frame->setGeometry ( 253, 29, 11, 1 );
	
	step_1MHz_frame = new QFrame ( ctlFrame );
	step_1MHz_frame->setGeometry ( 234, 29, 11, 1 );
	
	step_10MHz_frame = new QFrame ( ctlFrame );
	step_10MHz_frame->setGeometry ( 221, 29, 11, 1 );
	
	step_100MHz_frame = new QFrame ( ctlFrame );
	step_100MHz_frame->setGeometry ( 208, 29, 11, 1 );

    QPixmap rxPix(rx_xpm);
    QPixmap txPix(tx_xpm);

	trxFrame = new QFrame ( ctlFrame );
    trxFrame->setPalette( QColor( 0, 0, 0) );
    trxFrame->setAutoFillBackground(true);
	TRX_label = new Varilabel ( trxFrame );
	TRX_label->setGeometry ( 0, 0, 19, 11 );
    TRX_label->setPixmap(rxPix);
    TRX_label->setLabel( RX );
	connect ( TRX_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( toggle_TX ( int ) ) );

    QPixmap mhz_pix(mhz_xpm);
    QFrame *mhzFrame = new QFrame( ctlFrame );
    mhzFrame->setPalette( QColor( 0, 0, 0) );
    mhzFrame->setAutoFillBackground(true);
   
    MHz_label = new QLabel(mhzFrame);
    MHz_label->setPixmap(mhz_pix);
    MHz_label->setPalette( QColor( 0, 0, 0) );
    MHz_label->setAutoFillBackground(true);
	signal_dBm->setGeometry ( 140, 2, 35, 12 );

	// -----------------------------------------------------------------------
	// Mode Frame

    QFrame *modeFrame = new QFrame( this );
    modeFrame->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );  
    modeFrame->setPalette( QColor(100, 0, 0) );
    modeFrame->setAutoFillBackground(true);

	QPixmap lsb_pix ( lsb_xpm );
	LSB_label = new VariModelabel ( modeFrame );
	LSB_label->setPixmap ( lsb_pix );
	LSB_label->setLabel ( RIG_MODE_LSB );
	LSB_label->setGeometry ( 3, 3, 21, 11 );
	connect ( LSB_label, SIGNAL ( mouseRelease ( rmode_t, bool, bool ) ),
	          this, SLOT ( setMode ( rmode_t, bool, bool ) ) );

	QPixmap usb_pix ( usb_xpm );
	USB_label = new VariModelabel ( modeFrame );
	USB_label->setPixmap ( usb_pix );
	USB_label->setLabel ( RIG_MODE_USB );
	USB_label->setGeometry ( 27, 3, 21, 11 );
	connect ( USB_label, SIGNAL ( mouseRelease ( rmode_t, bool, bool ) ),
	          this, SLOT ( setMode ( rmode_t, bool, bool ) ) );

	QPixmap dsb_pix ( dsb_xpm );
	DSB_label = new VariModelabel ( modeFrame );
	DSB_label->setPixmap ( dsb_pix );
	DSB_label->setLabel ( RIG_MODE_DSB );
	DSB_label->setGeometry ( 51, 3, 21, 11 );
	connect ( DSB_label, SIGNAL ( mouseRelease ( rmode_t, bool, bool ) ),
	          this, SLOT ( setMode ( rmode_t, bool, bool ) ) );

	QPixmap am_pix ( am_xpm );
	AM_label = new VariModelabel ( modeFrame );
	AM_label->setPixmap ( am_pix );
	AM_label->setLabel ( RIG_MODE_AM );
	AM_label->setGeometry ( 75, 3, 21, 11 );
	connect ( AM_label, SIGNAL ( mouseRelease ( rmode_t, bool, bool ) ),
	          this, SLOT ( setMode ( rmode_t, bool, bool ) ) );

	QPixmap cwl_pix ( cwl_xpm );
	CWL_label = new VariModelabel ( modeFrame );
	CWL_label->setPixmap ( cwl_pix );
	CWL_label->setLabel ( RIG_MODE_CWR );
	CWL_label->setGeometry ( 3, 17, 21, 11 );
	connect ( CWL_label, SIGNAL ( mouseRelease ( rmode_t, bool, bool ) ),
	          this, SLOT ( setMode ( rmode_t, bool, bool ) ) );

	QPixmap cwu_pix ( cwu_xpm );
	CWU_label = new VariModelabel ( modeFrame );
	CWU_label->setPixmap ( cwu_pix );
	CWU_label->setLabel ( RIG_MODE_CW );
	CWU_label->setGeometry ( 27, 17, 21, 11 );
	connect ( CWU_label, SIGNAL ( mouseRelease ( rmode_t, bool, bool ) ),
	          this, SLOT ( setMode ( rmode_t, bool, bool ) ) );

	QPixmap sam_pix ( sam_xpm );
	SAM_label = new VariModelabel ( modeFrame );
	SAM_label->setPixmap ( sam_pix );
	SAM_label->setLabel ( RIG_MODE_SAM );
	SAM_label->setGeometry ( 51, 17, 21, 11 );
	connect ( SAM_label, SIGNAL ( mouseRelease ( rmode_t, bool, bool ) ),
	          this, SLOT ( setMode ( rmode_t, bool, bool ) ) );

	QPixmap fmn_pix ( fmn_xpm );
	FMN_label = new VariModelabel ( modeFrame );
	FMN_label->setPixmap ( fmn_pix );
	FMN_label->setLabel ( RIG_MODE_FM );
	FMN_label->setGeometry ( 75, 17, 21, 11 );
	connect ( FMN_label, SIGNAL ( mouseRelease ( rmode_t, bool, bool ) ),
	          this, SLOT ( setMode ( rmode_t, bool, bool ) ) );

	// -----------------------------------------------------------------------
	// Noise Reduction Frame

    QFrame *swFrame = new QFrame( this );
    swFrame->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );    
    swFrame->setPalette( QColor( 0, 80, 0 ) );
    swFrame->setAutoFillBackground( true );

	QPixmap nr_pix ( nr_xpm );
	NR_label = new Varilabel ( swFrame );
	//NR_label->setFrameStyle(QFrame::NoFrame );
	NR_label->setPixmap ( nr_pix );
	NR_label->setGeometry ( 3, 3, 27, 11 );
	connect ( NR_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( toggle_NR ( int ) ) );

	QPixmap anf_pix ( anf_xpm );
	ANF_label = new Varilabel ( swFrame );
	ANF_label->setPixmap ( anf_pix );
	ANF_label->setGeometry ( 33, 3, 27, 11 );
	connect ( ANF_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( toggle_ANF ( int ) ) );

	QPixmap nb_pix ( nb_xpm );
	NB_label = new Varilabel ( swFrame );
	NB_label->setPixmap ( nb_pix );
	NB_label->setGeometry ( 63, 3, 27, 11 );
	connect ( NB_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( toggle_NB ( int ) ) );

	QPixmap bin_pix ( bin_xpm );
	BIN_label = new Varilabel ( swFrame );
	BIN_label->setPixmap ( bin_pix );
	BIN_label->setGeometry ( 3, 17, 27, 11 );
	connect ( BIN_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( toggle_BIN ( int ) ) );

	QPixmap mute_pix ( mute_xpm );
	MUTE_label = new Varilabel ( swFrame );
	MUTE_label->setPixmap ( mute_pix );
	MUTE_label->setGeometry ( 33, 17, 27, 11 );
	connect ( MUTE_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( toggle_MUTE ( int ) ) );

	QPixmap spec_pix ( spec_xpm );
	SPEC_label = new Varilabel ( swFrame );
	SPEC_label->setPixmap ( spec_pix );
	SPEC_label->setGeometry ( 63, 17, 27, 11 );
	connect ( SPEC_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( toggle_SPEC ( int ) ) );

	// -----------------------------------------------------------------------
	// Band Frame
	QFrame *bFrame = new QFrame ( this );
    bFrame->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );    
    bFrame->setPalette( QColor( 0, 80, 0 ) );
    bFrame->setAutoFillBackground( true );

	QPixmap up_pix ( up_xpm );
	UP_label = new Varilabel ( bFrame );
	UP_label->setPixmap ( up_pix );
	UP_label->setGeometry ( 3, 3, 27, 11 );
   	UP_label->setPalette(QColor(0, 0, 0) );
    UP_label->setAutoFillBackground( true );
	connect ( UP_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( band_UP ( int ) ) );

	QPixmap down_pix ( down_xpm );
	DOWN_label = new Varilabel ( bFrame );
	DOWN_label->setPixmap ( down_pix );
	DOWN_label->setGeometry ( 3, 17, 27, 11 );
   	DOWN_label->setPalette(QColor(0, 0, 0) );
    DOWN_label->setAutoFillBackground( true );
	connect ( DOWN_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( band_DOWN ( int ) ) );

	// -----------------------------------------------------------------------
	// Sub Mode Frame
	QFrame *subFrame = new QFrame ( this );
    subFrame->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );    
    subFrame->setPalette( QColor( 0, 0, 100 ) );
    subFrame->setAutoFillBackground( true );

	QPixmap rit_pix ( rit_xpm );
	RIT_label = new Varilabel ( subFrame );
	RIT_label->setPixmap ( rit_pix );
	RIT_label->setGeometry ( 3, 3, 27, 11 );
   	RIT_label->setAutoFillBackground( true );
	connect ( RIT_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( toggle_RIT ( int ) ) );

	QPixmap split_pix ( split_xpm );
	SPLIT_label = new Varilabel ( subFrame );
	SPLIT_label->setPixmap ( split_pix );
	SPLIT_label->setGeometry ( 3, 17, 27, 11 );
   	SPLIT_label->setAutoFillBackground( true );
	connect ( SPLIT_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( toggle_SPLIT ( int ) ) );

	// -----------------------------------------------------------------------
	// RIT / Split Value
	rit = new QLabel ( ctlFrame );
	rit->setFont ( *fontlcd);
    p = rit->palette();
    p.setColor(QPalette::Window, Qt::black);
    p.setColor(QPalette::Active, QPalette::WindowText, QColor(255, 255, 255) );
    rit->setPalette(p);
    rit->setAutoFillBackground(true);
	//rit->setFrameStyle ( QFrame::NoFrame );

	// -----------------------------------------------------------------------
	// Memory Cells

	for (int i=0; i<NUM_MEMS; i++) {
	  char buffer[256];
	  f_cell[i] = new MemoryCell(ctlFrame2);
      f_cell[i]->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );
	  f_cell[i]->setFont(*font1);
	  f_cell[i]->setGeometry((i*21+1), 1, 20, 15);
      f_cell[i]->setPalette( QColor( 0, 0, 0 ) );
      f_cell[i]->setAutoFillBackground( true );
      p = f_cell[i]->palette();
      p.setColor(QPalette::Active, QPalette::WindowText, QColor( 255, 255, 255) );
      f_cell[i]->setPalette(p);
	  f_cell[i]->setGeometry((i*21+1), 1, 20, 15);
	  f_cell[i]->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
	  snprintf(buffer, 256, "F%d", (i+1));
	  f_cell[i]->setText (buffer);
	  f_cell[i]->setID (i+1);
	  connect(f_cell[i], SIGNAL(read(MemoryCell *)),
		    this, SLOT(readMem(MemoryCell *)));
	  connect(f_cell[i],SIGNAL(write(MemoryCell *)),
		    this, SLOT(writeMem(MemoryCell *)));
	  connect (f_cell[i], SIGNAL(display(MemoryCell *)),
		    this, SLOT(displayMem(MemoryCell *)));
	}

	M_label = new QLabel ( ctlFrame2 );
	M_label->setFont ( *font1 );
    M_label->setPalette( QColor( 0, 0, 0 ) );
    M_label->setAutoFillBackground( true );
    p = M_label->palette();
    p.setColor(QPalette::Inactive, QPalette::WindowText, QColor( 255, 255, 255) );
    M_label->setPalette(p);
	M_label->setGeometry (
	    169,
//!		f8_cell->x() + f8_cell->width(), 
	    1,
	    font1Metrics->maxWidth() * 12,
	    15 );
	M_label->setAlignment ( Qt::AlignHCenter | Qt::AlignVCenter );

	for (int i=0; i<NUM_BANDMEMS; i++) {
	  band_cell[i] = new MemoryCell();
	  band_cell[i]->setID(i+11);
	}
	band = 1;

	loadMemoryCells();

	// -----------------------------------------------------------------------
	// Spectrogram Color Aperture

	CA_label = new QLabel ( ctlFrame2 );
    CA_label->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );
	CA_label->setFont ( *font1 );
    p = CA_label->palette();
    p.setColor(QPalette::Window, Qt::black);
    p.setColor(QPalette::Active, QPalette::WindowText, QColor(255, 100, 100) );
    CA_label->setPalette(p);
    CA_label->setAutoFillBackground( true );
	CA_label->setGeometry (
	    M_label->x() + M_label->width() + 1,
		0, 
	    font1Metrics->maxWidth() * 16,
		17 );
	CA_label->setAlignment ( Qt::AlignHCenter | Qt::AlignVCenter );


	// -----------------------------------------------------------------------
	// AGC

	AGC_label = new QLabel ( ctlFrame2 );
    AGC_label->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );
	AGC_label->setFont ( *font1 );
	AGC_label->setText ( " AGC" );
    p = AGC_label->palette();
    p.setColor(QPalette::Window, Qt::black);
    p.setColor(QPalette::Active, QPalette::WindowText, QColor( 0, 180, 255 ) );
    p.setColor(QPalette::Inactive, QPalette::WindowText, QColor( 255, 180, 0 ) );
    AGC_label->setPalette(p);
    AGC_label->setAutoFillBackground( true );
	AGC_label->setGeometry (
		CA_label->x() + CA_label->width() - 1, 
		0, 
	    font1Metrics->maxWidth() * 5 + 50,
		17 );
	AGC_label->setAlignment ( Qt::AlignLeft | Qt::AlignVCenter );

	//QPixmap agc_o_pix( agc_o_xpm );
	//AGC_O_label = new Varilabel( AGC_label );
	//AGC_O_label->setLabel( 0 );
	//AGC_O_label->setPixmap( agc_o_pix );
	//AGC_O_label->setGeometry( font1Metrics->maxWidth() * 5, 2, 9, 11 );
	//connect( AGC_O_label, SIGNAL(mouseRelease(int)),
	//		 this, SLOT(setAGC(int)) );

	QPixmap agc_l_pix ( agc_l_xpm );
	AGC_L_label = new Varilabel ( AGC_label );
	AGC_L_label->setLabel ( 1 );
	AGC_L_label->setPixmap ( agc_l_pix );
    AGC_L_label->setFrameStyle( QFrame::Box | QFrame::Raised );
    AGC_L_label->setLineWidth(0);
    AGC_L_label->setMidLineWidth(0);
    AGC_L_label->setGeometry( font1Metrics->maxWidth() * 5, 3, 9, 11 );
	connect ( AGC_L_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( setAGC ( int ) ) );

	QPixmap agc_s_pix ( agc_s_xpm );
	AGC_S_label = new Varilabel ( AGC_label );
	AGC_S_label->setLabel ( 2 );
	AGC_S_label->setPixmap ( agc_s_pix );
    AGC_S_label->setGeometry( AGC_L_label->x() + 11, 3, 9, 11 );
	connect ( AGC_S_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( setAGC ( int ) ) );

	QPixmap agc_m_pix ( agc_m_xpm );
	AGC_M_label = new Varilabel ( AGC_label );
	AGC_M_label->setLabel ( 3 );
	AGC_M_label->setPixmap ( agc_m_pix );
    AGC_M_label->setGeometry( AGC_S_label->x() + 11, 3, 9, 11 );
	connect ( AGC_M_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( setAGC ( int ) ) );

	QPixmap agc_f_pix ( agc_f_xpm );
	AGC_F_label = new Varilabel ( AGC_label );
	AGC_F_label->setLabel ( 4 );
	AGC_F_label->setPixmap ( agc_f_pix );
    AGC_F_label->setGeometry( AGC_M_label->x() + 11, 3, 9, 11 );
	connect ( AGC_F_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( setAGC ( int ) ) );

	// -----------------------------------------------------------------------
	// Spacer for filling up empty space

	Spacer_label = new QLabel ( ctlFrame2 );
	Spacer_label->setFont ( *font1 );
	Spacer_label->setAlignment ( Qt::AlignHCenter | Qt::AlignVCenter );
    Spacer_label->setPalette( QColor( 0, 0, 255 ) );    
    Spacer_label->setAutoFillBackground( true );

	CFG_label = new Varilabel ( ctlFrame2 );
    CFG_label->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );
	CFG_label->setFont ( *font1 );
	CFG_label->setText ( "Help & CFG" );
    CFG_label->setPalette( QColor( 0, 0, 0 ) );
    CFG_label->setAutoFillBackground( true );
    p = CFG_label->palette();
    p.setColor(QPalette::Active, QPalette::WindowText, QColor( 255, 255, 255) );
    CFG_label->setPalette(p);   
	CFG_label->setAlignment ( Qt::AlignHCenter | Qt::AlignVCenter );
	connect ( CFG_label, SIGNAL ( mouseRelease ( int ) ), this, SLOT ( setCfg ( int ) ) );

	CPU_label = new QLabel ( ctlFrame2 );
    CPU_label->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );    
	CPU_label->setFont ( *font1 );
    CPU_label->setPalette( QColor( 0, 0, 0 ) );    
    CPU_label->setAutoFillBackground( true );    
    p = CPU_label->palette();
    p.setColor(QPalette::Active, QPalette::WindowText, QColor( 0, 180, 255) );
    CPU_label->setPalette(p);
	CPU_label->setAlignment ( Qt::AlignHCenter | Qt::AlignVCenter );

	//ctlFrame1->setGeometry ( 1, 1, 641, 31 );
	signalFrame->setGeometry ( 1, 3, 170, 27 );
	dBmLabel->setGeometry ( 140, 14, 35, 12 );
	lcd->setGeometry ( 170, 3, 170, 27 );
	trxFrame->setGeometry ( 337, 4, 19, 11 );
	mhzFrame->setGeometry ( 337, 19, 19, 8 );

	bFrame->setGeometry ( 363, 1, 33, 31 );
	modeFrame->setGeometry ( 397, 1, 99, 31 );
	swFrame->setGeometry ( 497, 1, 93, 31 );
	subFrame->setGeometry ( 590, 1, 33, 31 );
	rit->setGeometry ( 625, 1, 130, 31 );

	logoFrame = new QFrame ( ctlFrame );
    logoFrame->setPalette( QColor( 0, 0, 0 ) );
    logoFrame->setAutoFillBackground(true);

	QPixmap logo_pix ( logo_xpm );
	logoLabel = new QLabel ( logoFrame );
	logoLabel->setPixmap ( logo_pix );
    logoLabel->setPalette( QColor( 0, 0, 0 ) );
    logoLabel->setAutoFillBackground(true);

	setRxFrequency( 1 );
	if (useIF)
	{
	  setDefaultRxFrequency();
	}
	setIQGain();
	setIQPhase();
	set_NR ( NR_state );
	set_ANF ( ANF_state );
	set_NB ( NB_state );
	set_BIN ( BIN_state );
	set_SPEC ( SPEC_state );
	set_MUTE ( 0 );
	set_RIT ( 0 );
	set_SPLIT ( 0 );
   	UP_label->setPalette(QColor(0, 0, 0) );
   	DOWN_label->setPalette(QColor(0, 0, 0) );
	setCA_label();
	setTuneStep ( 0 );
	setTheme ( 2 );
        setPolyFFT ( );
	setSpectrumDefaults();
	setAGC ( agcType );
	setTxIQGain();
	setTxIQPhase();
	setTxGain( 0 );
	setTxGain( 1 );
	processorLoad();
	hScale = 1.0;
	if ( useHamlib )
	{
		initHamlib();
	}
	setMode(mode, FALSE, TRUE);

	QTimer *cpuTimer = new QTimer ( this );
	connect ( cpuTimer, SIGNAL ( timeout() ), this, SLOT ( processorLoad() ) );
    cpuTimer->start( 5000 );

	QTimer *meterTimer = new QTimer ( this );
	connect ( meterTimer, SIGNAL ( timeout() ), this, SLOT ( readMeter() ) );
    meterTimer->start( 100 );

	QTimer *fftTimer = new QTimer ( this );
	connect ( fftTimer, SIGNAL ( timeout() ), this, SLOT ( readSpectrum() ) );
    fftTimer->start( 100 );      
}

void Main_Widget::initConstants()
{
	// Modes
	modeName[LSB] = new QString ( "LSB" );
	modeName[USB] = new QString ( "USB" );
	modeName[DSB] = new QString ( "DSB" );
	modeName[CWL] = new QString ( "CWL" );
	modeName[CWU] = new QString ( "CWU" );
	modeName[FMN] = new QString ( "FMN" );
	modeName[AM] = new QString ( "AM" );
	modeName[DIGU] = new QString ( "DIGU" );
	modeName[SPEC] = new QString ( "SPEC" );
	modeName[DIGL] = new QString ( "DIGL" );
	modeName[SAM] = new QString ( "SAM" );
	modeName[DRM] = new QString ( "DRM" );

	// S meter colors
	s_dbm[0] = -127; signalColor[0]  = new QColor ( 40, 255, 40 );
	s_dbm[1] = -124; signalColor[1]  = new QColor ( 40, 255, 40 );
	s_dbm[2] = -121; signalColor[2]  = new QColor ( 40, 255, 40 ); // S1
	s_dbm[3] = -118; signalColor[3]  = new QColor ( 40, 255, 40 );
	s_dbm[4] = -115; signalColor[4]  = new QColor ( 40, 255, 40 ); // S2
	s_dbm[5] = -112; signalColor[5]  = new QColor ( 40, 255, 40 );
	s_dbm[6] = -109; signalColor[6]  = new QColor ( 40, 255, 40 ); // S3
	s_dbm[7] = -106; signalColor[7]  = new QColor ( 40, 255, 40 );
	s_dbm[8] = -103; signalColor[8]  = new QColor ( 40, 255, 40 ); // S4
	s_dbm[9] = -100; signalColor[9]  = new QColor ( 40, 255, 40 );
	s_dbm[10] = -97; signalColor[10] = new QColor ( 40, 255, 40 ); // S5
	s_dbm[11] = -94; signalColor[11] = new QColor ( 40, 255, 40 );
	s_dbm[12] = -91; signalColor[12] = new QColor ( 40, 255, 40 ); // S6
	s_dbm[13] = -88; signalColor[13] = new QColor ( 40, 255, 40 );
	s_dbm[14] = -85; signalColor[14] = new QColor ( 40, 255, 40 ); // S7
	s_dbm[15] = -82; signalColor[15] = new QColor ( 40, 255, 40 );
	s_dbm[16] = -79; signalColor[16] = new QColor ( 40, 255, 40 ); // S8
	s_dbm[17] = -76; signalColor[17] = new QColor ( 40, 255, 40 );
	s_dbm[18] = -73; signalColor[18] = new QColor ( 40, 255, 40 ); // S9
	s_dbm[19] = -69; signalColor[19] = new QColor ( 255, 255, 40 );
	s_dbm[20] = -65; signalColor[20] = new QColor ( 255, 255, 40 );
	s_dbm[21] = -61; signalColor[21] = new QColor ( 255, 255, 40 );
	s_dbm[22] = -57; signalColor[22] = new QColor ( 255, 255, 40 );
	s_dbm[23] = -53; signalColor[23] = new QColor ( 255, 255, 40 ); // S9+20dB
	s_dbm[24] = -49; signalColor[24] = new QColor ( 255,  70, 40 );
	s_dbm[25] = -45; signalColor[25] = new QColor ( 255,  70, 40 );
	s_dbm[26] = -41; signalColor[26] = new QColor ( 255,  70, 40 );
	s_dbm[27] = -37; signalColor[27] = new QColor ( 255,  70, 40 );
	s_dbm[28] = -33; signalColor[28] = new QColor ( 255,  70, 40 ); // S9+40dB
	s_dbm[29] = -29; signalColor[29] = new QColor ( 255,  40, 40 );
	s_dbm[30] = -25; signalColor[30] = new QColor ( 255,  40, 40 );
	s_dbm[31] = -21; signalColor[31] = new QColor ( 255,  40, 40 );
	s_dbm[32] = -17; signalColor[32] = new QColor ( 255,  40, 40 );
	s_dbm[33] = -13; signalColor[33] = new QColor ( 255,  40, 40 ); // S9+60dB

	// Spectrogram colors

	spec_b[0] =  0; spec_g[0] =  0; spec_r[0] =  0;
	spec_b[1] =  9; spec_g[1] =  1; spec_r[1] =  1;
	spec_b[2] =  15; spec_g[2] =  1; spec_r[2] =  1;
	spec_b[3] =  21; spec_g[3] =  1; spec_r[3] =  1;
	spec_b[4] =  28; spec_g[4] =  1; spec_r[4] =  1;
	spec_b[5] =  34; spec_g[5] =  1; spec_r[5] =  1;
	spec_b[6] =  40; spec_g[6] =  0; spec_r[6] =  0;
	spec_b[7] =  46; spec_g[7] =  0; spec_r[7] =  0;
	spec_b[8] =  56; spec_g[8] =  0; spec_r[8] =  0;
	spec_b[9] =  62; spec_g[9] =  1; spec_r[9] =  1;
	spec_b[10] =  68; spec_g[10] =  1; spec_r[10] =  1;
	spec_b[11] =  75; spec_g[11] =  0; spec_r[11] =  0;
	spec_b[12] =  79; spec_g[12] =  0; spec_r[12] =  0;
	spec_b[13] =  84; spec_g[13] =  0; spec_r[13] =  0;
	spec_b[14] =  88; spec_g[14] =  0; spec_r[14] =  0;
	spec_b[15] =  95; spec_g[15] =  0; spec_r[15] =  0;
	spec_b[16] =  100; spec_g[16] =  3; spec_r[16] =  0;
	spec_b[17] =  104; spec_g[17] =  8; spec_r[17] =  0;
	spec_b[18] =  108; spec_g[18] =  13; spec_r[18] =  0;
	spec_b[19] =  110; spec_g[19] =  18; spec_r[19] =  0;
	spec_b[20] =  113; spec_g[20] =  23; spec_r[20] =  0;
	spec_b[21] =  115; spec_g[21] =  28; spec_r[21] =  0;
	spec_b[22] =  119; spec_g[22] =  35; spec_r[22] =  0;
	spec_b[23] =  121; spec_g[23] =  40; spec_r[23] =  0;
	spec_b[24] =  124; spec_g[24] =  45; spec_r[24] =  0;
	spec_b[25] =  124; spec_g[25] =  50; spec_r[25] =  0;
	spec_b[26] =  124; spec_g[26] =  55; spec_r[26] =  0;
	spec_b[27] =  124; spec_g[27] =  60; spec_r[27] =  0;
	spec_b[28] =  124; spec_g[28] =  65; spec_r[28] =  1;
	spec_b[29] =  124; spec_g[29] =  72; spec_r[29] =  1;
	spec_b[30] =  125; spec_g[30] =  77; spec_r[30] =  1;
	spec_b[31] =  123; spec_g[31] =  82; spec_r[31] =  1;
	spec_b[32] =  121; spec_g[32] =  87; spec_r[32] =  0;
	spec_b[33] =  119; spec_g[33] =  92; spec_r[33] =  0;
	spec_b[34] =  116; spec_g[34] =  97; spec_r[34] =  1;
	spec_b[35] =  112; spec_g[35] =  102; spec_r[35] =  1;
	spec_b[36] =  106; spec_g[36] =  110; spec_r[36] =  1;
	spec_b[37] =  101; spec_g[37] =  112; spec_r[37] =  1;
	spec_b[38] =  97; spec_g[38] =  115; spec_r[38] =  1;
	spec_b[39] =  92; spec_g[39] =  118; spec_r[39] =  1;
	spec_b[40] =  88; spec_g[40] =  121; spec_r[40] =  1;
	spec_b[41] =  80; spec_g[41] =  124; spec_r[41] =  0;
	spec_b[42] =  78; spec_g[42] =  127; spec_r[42] =  0;
	spec_b[43] =  69; spec_g[43] =  131; spec_r[43] =  0;
	spec_b[44] =  63; spec_g[44] =  134; spec_r[44] =  0;
	spec_b[45] =  57; spec_g[45] =  137; spec_r[45] =  0;
	spec_b[46] =  51; spec_g[46] =  140; spec_r[46] =  0;
	spec_b[47] =  45; spec_g[47] =  143; spec_r[47] =  1;
	spec_b[48] =  39; spec_g[48] =  146; spec_r[48] =  0;
	spec_b[49] =  33; spec_g[49] =  149; spec_r[49] =  1;
	spec_b[50] =  24; spec_g[50] =  152; spec_r[50] =  0;
	spec_b[51] =  18; spec_g[51] =  154; spec_r[51] =  7;
	spec_b[52] =  12; spec_g[52] =  156; spec_r[52] =  19;
	spec_b[53] =  6; spec_g[53] =  158; spec_r[53] =  31;
	spec_b[54] =  0; spec_g[54] =  160; spec_r[54] =  43;
	spec_b[55] =  0; spec_g[55] =  160; spec_r[55] =  55;
	spec_b[56] =  1; spec_g[56] =  160; spec_r[56] =  67;
	spec_b[57] =  1; spec_g[57] =  160; spec_r[57] =  79;
	spec_b[58] =  0; spec_g[58] =  160; spec_r[58] =  97;
	spec_b[59] =  0; spec_g[59] =  160; spec_r[59] =  109;
	spec_b[60] =  0; spec_g[60] =  159; spec_r[60] =  121;
	spec_b[61] =  0; spec_g[61] =  157; spec_r[61] =  128;
	spec_b[62] =  1; spec_g[62] =  155; spec_r[62] =  135;
	spec_b[63] =  1; spec_g[63] =  154; spec_r[63] =  142;
	spec_b[64] =  1; spec_g[64] =  152; spec_r[64] =  149;
	spec_b[65] =  1; spec_g[65] =  148; spec_r[65] =  160;
	spec_b[66] =  1; spec_g[66] =  145; spec_r[66] =  167;
	spec_b[67] =  1; spec_g[67] =  142; spec_r[67] =  174;
	spec_b[68] =  1; spec_g[68] =  139; spec_r[68] =  178;
	spec_b[69] =  1; spec_g[69] =  136; spec_r[69] =  182;
	spec_b[70] =  1; spec_g[70] =  133; spec_r[70] =  185;
	spec_b[71] =  0; spec_g[71] =  131; spec_r[71] =  188;
	spec_b[72] =  1; spec_g[72] =  126; spec_r[72] =  192;
	spec_b[73] =  1; spec_g[73] =  123; spec_r[73] =  194;
	spec_b[74] =  1; spec_g[74] =  120; spec_r[74] =  196;
	spec_b[75] =  1; spec_g[75] =  117; spec_r[75] =  198;
	spec_b[76] =  1; spec_g[76] =  112; spec_r[76] =  200;
	spec_b[77] =  1; spec_g[77] =  107; spec_r[77] =  202;
	spec_b[78] =  1; spec_g[78] =  102; spec_r[78] =  204;
	spec_b[79] =  1; spec_g[79] =  94; spec_r[79] =  206;
	spec_b[80] =  1; spec_g[80] =  89; spec_r[80] =  207;
	spec_b[81] =  0; spec_g[81] =  84; spec_r[81] =  208;
	spec_b[82] =  0; spec_g[82] =  79; spec_r[82] =  210;
	spec_b[83] =  0; spec_g[83] =  74; spec_r[83] =  212;
	spec_b[84] =  0; spec_g[84] =  69; spec_r[84] =  214;
	spec_b[85] =  1; spec_g[85] =  64; spec_r[85] =  217;
	spec_b[86] =  1; spec_g[86] =  56; spec_r[86] =  220;
	spec_b[87] =  1; spec_g[87] =  51; spec_r[87] =  222;
	spec_b[88] =  0; spec_g[88] =  46; spec_r[88] =  224;
	spec_b[89] =  2; spec_g[89] =  41; spec_r[89] =  227;
	spec_b[90] =  2; spec_g[90] =  35; spec_r[90] =  229;
	spec_b[91] =  2; spec_g[91] =  32; spec_r[91] =  231;
	spec_b[92] =  2; spec_g[92] =  29; spec_r[92] =  233;
	spec_b[93] =  10; spec_g[93] =  25; spec_r[93] =  237;
	spec_b[94] =  19; spec_g[94] =  21; spec_r[94] =  239;
	spec_b[95] =  28; spec_g[95] =  28; spec_r[95] =  241;
	spec_b[96] =  37; spec_g[96] =  37; spec_r[96] =  243;
	spec_b[97] =  46; spec_g[97] =  46; spec_r[97] =  246;
	spec_b[98] =  55; spec_g[98] =  55; spec_r[98] =  248;
	spec_b[99] =  64; spec_g[99] =  64; spec_r[99] =  250;
	spec_b[100] =  77; spec_g[100] =  77; spec_r[100] =  253;
	spec_b[101] =  86; spec_g[101] =  86; spec_r[101] =  255;
	spec_b[102] =  95; spec_g[102] =  95; spec_r[102] =  255;
	spec_b[103] =  104; spec_g[103] =  104; spec_r[103] =  255;
	spec_b[104] =  112; spec_g[104] =  112; spec_r[104] =  255;
	spec_b[105] =  121; spec_g[105] =  121; spec_r[105] =  255;
	spec_b[106] =  130; spec_g[106] =  130; spec_r[106] =  255;
	spec_b[107] =  139; spec_g[107] =  139; spec_r[107] =  255;
	spec_b[108] =  152; spec_g[108] =  152; spec_r[108] =  255;
	spec_b[109] =  161; spec_g[109] =  161; spec_r[109] =  255;
	spec_b[110] =  170; spec_g[110] =  170; spec_r[110] =  255;
	spec_b[111] =  179; spec_g[111] =  179; spec_r[111] =  255;
	spec_b[112] =  188; spec_g[112] =  188; spec_r[112] =  255;
	spec_b[113] =  197; spec_g[113] =  197; spec_r[113] =  255;
	spec_b[114] =  206; spec_g[114] =  206; spec_r[114] =  255;
	spec_b[115] =  219; spec_g[115] =  219; spec_r[115] =  255;
	spec_b[116] =  228; spec_g[116] =  228; spec_r[116] =  255;
	spec_b[117] =  237; spec_g[117] =  237; spec_r[117] =  255;
	spec_b[118] =  246; spec_g[118] =  246; spec_r[118] =  255;
	spec_b[119] =  255; spec_g[119] =  255; spec_r[119] =  255;
}

void Main_Widget::setTheme ( int t )
{
	theme = t;
	switch ( theme )
	{
		case 1:
    setPalette( QColor( 0, 0, 0 ) );
    this->setAutoFillBackground(true);    
			break;
		case 2:
    setPalette( QColor( 0, 0, 0 ) );
    this->setAutoFillBackground(true);    
			break;
		default:
			break;
	}
}

void Main_Widget::paintEvent ( QPaintEvent * )
{
	if (updated) {
		//fprintf( stderr, "+");
		updateLayout();
		if ( SPEC_state ) {
			drawSpectrogram();			// update the spectrogram (middle) display
			plotSpectrum( spectrum_head );	// plots the spectrum display
		}
		drawPassBandScale();
	} else {
		//fprintf( stderr, "-");
	}
	updated = 0;
}

void Main_Widget::updateLayout()
{
	ctlFrame->setGeometry (
	    1,
	    1,
	    width() - 2,
	    33 );
	logoFrame->setGeometry (
	    755,
	    1,
	    ctlFrame->width() - 755,
	    31 );
	logoLabel->setGeometry (
	    logoFrame->width() - 89,
	    0,
	    88,
	    31 );
	spectrogramFrame->setGeometry (
	    1,
	    35,
	    width() - 2,
	    height() - 18 - 36 );
	spectrogram->setGeometry (
	    2,
	    2,
	    spectrogramFrame->width() - 4,
	    spectrogramFrame->height() - 4 - 120 - 15 );
	pbscale->setGeometry (
	    2,
	    spectrogram->height() + 2,
	    spectrogramFrame->width() - 4,
	    15 );
	spectrumFrame->setGeometry (
	    2,
	    spectrogramFrame->height() - 120 - 2,
	    spectrogramFrame->width() - 4,
	    120 );
	ctlFrame2->setGeometry (
	    1,
	    height() - 17 - 1,
	    width() - 2,
	    17 );
	CPU_label->setGeometry (
	    ctlFrame2->width() - font1Metrics->maxWidth() * 11 - 1,
	    1,
	    font1Metrics->maxWidth() * 11,
	    15 );
	CFG_label->setGeometry (
	    ctlFrame2->width() - CPU_label->width() -
	    font1Metrics->maxWidth() * 11 - 2,
	    1,
	    font1Metrics->maxWidth() * 11,
	    15 );
	Spacer_label->setGeometry (
	    AGC_label->x() + AGC_label->width() + 1,
	    1,
	    CFG_label->x() - ( AGC_label->x() + AGC_label->width() ) - 2,
	    15 );

	spectrum_width = int(spectrumFrame->width() * hScale);
}

void Main_Widget::loadSettings()
{
	QSettings settings("freesoftware", "sdr-shell");
	//settings.sync();
	// settings.setValue("editor/wrapMargin", 68);
	//  int margin = settings.value("editor/wrapMargin").toInt();
	//     settings.beginGroup("MainWindow");
	//     resize(settings.value("size", QSize(400, 400)).toSize());
	//     move(settings.value("pos", QPoint(200, 200)).toPoint());
	//     settings.endGroup();


	// SDR-Core Environment
	char *ep;
	if ( ( ep = getenv ( "SDR_DEFRATE" ) ) )
	{
		sample_rate = atoi ( ep );
		printf ( "sample_rate = %d\n", sample_rate );
		tuneCenter = -sample_rate / 4;
	}
	else
	{
		printf ( "Unable to get SDR_DEFRATE environment variable.\n"
		         "You can set SDR_DEFRATE as follows:\n"
		         "bash$ export SDR_DEFRATE=48000\n" );
		exit ( 1 );
	}

     // create the command, spectrum, and meter ports

     // this does a blind send to a port that's already bound,
     // so it's outbound == !inbound -> inbound = 0
     //cp = new_dttsp_port_client(DTTSP_PORT_CLIENT_COMMAND, 0);

     // these need to suck up blind sends from elsewhere,
     // so they need to be bound, hence inbound = 1
     //sp = new_dttsp_port_client(DTTSP_PORT_CLIENT_SPECTRUM, 1);
     //mp = new_dttsp_port_client(DTTSP_PORT_CLIENT_METER, 1);

	pUSBCmd = NULL;
	pTXCmd = NULL;

	if ( ( ep = getenv ( "SDR_HOST" ) ) ) {
		pCmd = new DttSPcmd (ep);

		pMeter = new DttSPmeter (ep);

		pSpectrum = new DttSPspectrum (ep);
     
		pUSBCmd = new USBSoftrockCmd (ep);
		// USB-Synth turns PTT on when it powers up.  Turn it off.
		pUSBCmd->sendCommand("set ptt off\n");

		pTXCmd = new DttSPTXcmd (ep);
	} else {
		pCmd = new DttSPcmd ();

		pMeter = new DttSPmeter ();

		pSpectrum = new DttSPspectrum ();
     
		pUSBCmd = new USBSoftrockCmd ();
		// USB-Synth turns PTT on when it powers up.  Turn it off.
		if (!rock_bound) {
			pUSBCmd->sendCommand("set ptt off\n");
		}

		pTXCmd = new DttSPTXcmd ();
	}
    // The value is the transmit port's CMD
    if ( ( ep = getenv ( "SDR_TRANSMIT" ) ) )
    {
		int port = atoi(ep);
		if (port)
			pTXCmd->setPort(port);
    }

	sdr_mode = NULL;
	if ( ( ep = getenv ( "SDR_MODE" ) ) )
	{
		sdr_mode = ep;
	}
	sdr_band = NULL;
	if ( ( ep = getenv ( "SDR_BAND" ) ) )
	{
		sdr_band = ep;
	}
	sdr_rxtx = NULL;
	if ( ( ep = getenv ( "SDR_RXTX" ) ) )
	{
		sdr_rxtx = ep;
	}

	// spectrum scaling
	spec_width = DEFSPEC;

	// Read config
	//sample_rate = settings.readEntry(
	//	"/sdr-shell/sample_rate", "48000" ).toInt();
    rx_f = settings.value( 
		"/sdr-shell/rx_f", 7055000 ).toLongLong();
    rx_f_string = settings.value( 
		"/sdr-shell/rx_f", 7055000 ).toString();
	useIF = settings.value (
	    "/sdr-shell/useIF", "0" ).toInt();
	useSlopeTune = settings.value (
		"/sdr-shell/useSlopeTune", "0" ).toInt();
	muteXmit = ( bool ) settings.value (
		"/sdr-shell/muteXmit", "0" ).toInt();
	rx_if = settings.value (
	    "/sdr-shell/rx_if", "455000" ).toLongLong();
	rx_if_string = settings.value (
	    "/sdr-shell/rx_if", "455000" ).toString();
    rx_delta_f = settings.value( 
		"/sdr-shell/rx_delta_f", 0 ).toInt();
	cwPitch = settings.value (
		"/sdr-shell/cwPitch", "600" ).toInt();
    specApertureLow = settings.value( 
		"/sdr-shell/specApertureLow", 16 ).toInt();
    specApertureHigh = settings.value( 
		"/sdr-shell/specApertureHigh", 41 ).toInt(); 
    tuneStep = settings.value( 
		"/sdr-shell/tuneStep", 1 ).toInt();
    stationCallsign = settings.value( 
		"/sdr-shell/stationCallsign", "nocall" ).toString();
	stationQTH = settings.value (
	    "/sdr-shell/stationQTH", "AA00aa" ).toString();
    iqGain = settings.value( 
		"/sdr-shell/iqGain", 0 ).toInt();
    iqPhase = settings.value( 
		"/sdr-shell/iqPhase", 0 ).toInt();
    txIQGain = settings.value( 
		"/sdr-shell/txIQGain", 0 ).toInt();
    txIQPhase = settings.value( 
		"/sdr-shell/txIQPhase", 0 ).toInt();
	txGain = settings.value (
        "/sdr-shell/txGain", "0" ).toInt();
	micGain = settings.value (
	    "/sdr-shell/micGain", "0" ).toInt();
	enableTransmit = settings.value (
	    "/sdr-shell/enableTransmit", "0" ).toInt();
	dualConversion = settings.value (
	    "/sdr-shell/dualConversion", "0" ).toInt();
    mode = (rmode_t) settings.value( 
		"/sdr-shell/mode", 1 ).toInt();
	usbOffset = settings.value (
	    "/sdr-shell/usbOffset", "1500" ).toInt();
	lsbOffset = settings.value (
	    "/sdr-shell/lsbOffset", "1500" ).toInt();
	slopeLowOffset = settings.value (
	    "/sdr-shell/slopeLowOffset", "3500" ).toInt();
	slopeHighOffset = settings.value (
	    "/sdr-shell/slopeHighOffset", "3500" ).toInt();
    NR_state = settings.value( 
		"/sdr-shell/NR_state", 0 ).toInt();
    ANF_state = settings.value( 
		"/sdr-shell/ANF_state", 0 ).toInt();
    NB_state = settings.value( 
		"/sdr-shell/NB_state", 0 ).toInt();
    BIN_state = settings.value( 
		"/sdr-shell/BIN_state", 0 ).toInt();
    SPEC_state = settings.value( 
		"/sdr-shell/SPEC_state", 1 ).toInt();
    filterLine = settings.value( 
		"/sdr-shell/filterLine", 0 ).toInt();
	my_lat = settings.value (
	    "/sdr-shell/my_lat", "0" ).toDouble();
	my_lon = settings.value (
	    "/sdr-shell/my_lon", "0" ).toDouble();
    font1PointSize = settings.value( 
		"/sdr-shell/font1PointSize", 8 ).toInt();		
    polyphaseFFT = settings.value( 
		"/sdr-shell/polyphaseFFT", 1 ).toInt();
    fftWindow = settings.value( 
		"/sdr-shell/fftWindow", 11 ).toInt();
    spectrumType = settings.value( 
		"/sdr-shell/spectrumType", 1 ).toInt();
    agcType = settings.value( 
		"/sdr-shell/agcType", 3 ).toInt();
    specCal = settings.value( 
		"/sdr-shell/specCal", 70 ).toInt();
    metrCal = settings.value( 
		"/sdr-shell/metrCal", 40 ).toInt();
//    hscaleIndex = settings.value(
//		"/sdr-shell/hScale", 3 ).toInt();
   specLineFill = settings.value(
		"/sdr-shell/specfill", false ).toBool();

    // Restore window geometry
    setGeometry (
        settings.value( "/sdr-shell/g_left", 172 ).toInt(),
        settings.value( "/sdr-shell/g_top", 172 ).toInt(),
        settings.value( "/sdr-shell/g_width", 650 ).toInt(),
        settings.value( "/sdr-shell/g_height", 424 ).toInt()
        );
    // Restore filter values
    USB_filter_l = settings.value( 
		"/sdr-shell/USB_filter_l", 20 ).toInt();
    USB_filter_h = settings.value( 
		"/sdr-shell/USB_filter_h", 2400 ).toInt();
    LSB_filter_l = settings.value( 
		"/sdr-shell/LSB_filter_l", -2400 ).toInt();    
    LSB_filter_h = settings.value( 
		"/sdr-shell/LSB_filter_h", -20 ).toInt();
    DSB_filter_l = settings.value( 
		"/sdr-shell/DSB_filter_l", -2400 ).toInt();
    DSB_filter_h = settings.value( 
		"/sdr-shell/DSB_filter_h", 2400 ).toInt();
    CWL_filter_l = settings.value( 
		"/sdr-shell/CWL_filter_l", -500 ).toInt();
    CWL_filter_h = settings.value( 
		"/sdr-shell/CWL_filter_h", -200 ).toInt();
    CWU_filter_l = settings.value( 
		"/sdr-shell/CWU_filter_l", 200 ).toInt();
    CWU_filter_h = settings.value( 
		"/sdr-shell/CWU_filter_h", 500 ).toInt();
    SAM_filter_l = settings.value( 
		"/sdr-shell/SAM_filter_l", -4000 ).toInt();
    SAM_filter_h = settings.value( 
		"/sdr-shell/SAM_filter_h", "4000" ).toInt();
    FMN_filter_l = settings.value( 
		"/sdr-shell/FMN_filter_l", -4000 ).toInt();
    FMN_filter_h = settings.value( 
		"/sdr-shell/FMN_filter_h", 4000 ).toInt();
    AM_filter_l = settings.value( 
		"/sdr-shell/AM_filter_l", -2400 ).toInt();
    AM_filter_h = settings.value( 
		"/sdr-shell/AM_filter_h", 2400 ).toInt();
	useHamlib = settings.value (
	    "/sdr-shell/useHamlib", "0" ).toInt();
	rigString = settings.value (
	    "/sdr-shell/hamlib_rig", "1901" ).toString();
	rig = rigString.toInt();
	portString =  settings.value (
	    "/sdr-shell/hamlib_port", "localhost" ).toString();
    //   hl_port =  qPrintable(settings.value (
          //  "/sdr-shell/hamlib_port", "localhost" ).toString());
	speedString = settings.value (
	    "/sdr-shell/hamlib_speed", "4800" ).toString();
	speed = speedString.toInt();
	rock_bound = ( bool ) settings.value (
			  "/sdr-shell/rock_bound", "false" ).toInt();

	map_flag = 1;

	printf ( "::: Configuration loading completed\n" );
}

void Main_Widget::loadMemoryCells()
{
	QSettings settings("freesoftware", "sdr-shell");

	// Restore memory cells
	for(int i=0; i<NUM_MEMS; i++) {
	  char buffer[256], buffer2[256];
	  snprintf(buffer, 256, "/sdr-shell/f%d_frequency", i+1);
	  f_cell[i]->setFrequency(settings.value(buffer, "0").toInt());
	  snprintf(buffer, 256, "/sdr-shell/f%d_txfrequency", i+1);
	  f_cell[i]->setTxFrequency(settings.value(buffer, "0").toInt());
	  snprintf(buffer, 256, "/sdr-shell/f%d_mode", i+1);
	  f_cell[i]->setMode((rmode_t)settings.value(buffer, "1").toInt());
	  snprintf(buffer, 256, "/sdr-shell/f%d_filter_l", i+1);
	  snprintf(buffer2, 256, "/sdr-shell/f%d_filter_h", i+1);
	  f_cell[i]->setFilter(settings.value(buffer, "20").toInt(),
			       settings.value(buffer2, "2400").toInt());
	}

	for (int i=0; i<NUM_BANDMEMS; i++) {
	  char buffer[256], buffer2[256];
	  snprintf(buffer, 256, "/sdr-shell/b%d_frequency", i+1);
	  band_cell[i]->setFrequency(settings.value(buffer, "0").toInt());
	  snprintf(buffer, 256, "/sdr-shell/b%d_txfrequency", i+1);
	  band_cell[i]->setTxFrequency(settings.value(buffer, "0").toInt());
	  snprintf(buffer, 256, "/sdr-shell/b%d_mode", i+1);
	  band_cell[i]->setMode((rmode_t)settings.value(buffer, "1").toInt());
	  snprintf(buffer, 256, "/sdr-shell/b%d_filter_l", i+1);
	  snprintf(buffer2, 256, "/sdr-shell/b%d_filter_h", i+1);
	  band_cell[i]->setFilter(settings.value(buffer, "20" ).toInt(),
			      settings.value(buffer2, "2400" ).toInt() );
	}

	printf ( "::: Memory Cells loading completed\n" );
}


void Main_Widget::closeEvent ( QCloseEvent * )
{
	finish();
}

void Main_Widget::saveSettings()
{
	QSettings settings("freesoftware", "sdr-shell");

	printf ( "Saving settings...\n" );

	//settings.setPath ( "n1vtn.org", ".qt", QSettings::User );
	//settings.setValue( "/sdr-shell/sample_rate", sample_rate );
	rx_f_string.setNum(rx_f);
	settings.setValue ( "/sdr-shell/rx_f", rx_f_string );
	rx_if_string.setNum(rx_if);
	settings.setValue ( "/sdr-shell/rx_if", rx_if_string );
	int intUseIF = ( int ) useIF;
	settings.setValue ( "/sdr-shell/useIF", intUseIF );
	int intUseSlopeTune = ( int ) useSlopeTune;
	settings.setValue ( "/sdr-shell/useSlopeTune", intUseSlopeTune );
	int intMuteXmit = ( int ) muteXmit;
	settings.setValue ( "/sdr-shell/useSlopeTune", intMuteXmit );
	settings.setValue ( "/sdr-shell/cwPitch", cwPitch );
	settings.setValue ( "/sdr-shell/rx_delta_f", rx_delta_f );
	settings.setValue ( "/sdr-shell/specApertureLow", specApertureLow );
	settings.setValue ( "/sdr-shell/specApertureHigh", specApertureHigh );
	settings.setValue ( "/sdr-shell/tuneStep", tuneStep );
	settings.setValue ( "/sdr-shell/stationCallsign", stationCallsign );
	settings.setValue ( "/sdr-shell/stationQTH", stationQTH );
	settings.setValue ( "/sdr-shell/iqGain", iqGain );
	settings.setValue ( "/sdr-shell/iqPhase", iqPhase );
	settings.setValue ( "/sdr-shell/txIQGain", txIQGain );
	settings.setValue ( "/sdr-shell/txIQPhase", txIQPhase );
	settings.setValue ( "/sdr-shell/txGain", txGain );
	settings.setValue ( "/sdr-shell/micGain", micGain );
	int intEnableTransmit = ( int ) enableTransmit;
	settings.setValue ( "/sdr-shell/enableTransmit", intEnableTransmit );
	settings.setValue ( "/sdr-shell/dualConversion", dualConversion );
	settings.setValue ( "/sdr-shell/mode", ( int ) mode );
	settings.setValue ( "/sdr-shell/NR_state", NR_state );
	settings.setValue ( "/sdr-shell/ANF_state", ANF_state );
	settings.setValue ( "/sdr-shell/NB_state", NB_state );
	settings.setValue ( "/sdr-shell/BIN_state", BIN_state );
	settings.setValue ( "/sdr-shell/SPEC_state", SPEC_state );
	settings.setValue ( "/sdr-shell/filterLine", filterLine );
	settings.setValue ( "/sdr-shell/my_lat", my_lat );
	settings.setValue ( "/sdr-shell/my_lon", my_lon );
	settings.setValue ( "/sdr-shell/font1PointSize", font1PointSize );
	settings.setValue ( "/sdr-shell/polyphaseFFT", polyphaseFFT );
	settings.setValue ( "/sdr-shell/fftWindow", fftWindow );
	settings.setValue ( "/sdr-shell/spectrumType", spectrumType );
	settings.setValue ( "/sdr-shell/agcType", agcType );
	settings.setValue ( "/sdr-shell/specCal", specCal );
	settings.setValue ( "/sdr-shell/metrCal", metrCal );

	// Save window geometry
	settings.setValue ( "/sdr-shell/g_left", geometry().left() );
	settings.setValue ( "/sdr-shell/g_top", geometry().top() );
	settings.setValue ( "/sdr-shell/g_width", geometry().width() );
	settings.setValue ( "/sdr-shell/g_height", geometry().height() );

	// Save filter values
	settings.setValue ( "/sdr-shell/USB_filter_l", USB_filter_l );
	settings.setValue ( "/sdr-shell/USB_filter_h", USB_filter_h );
	settings.setValue ( "/sdr-shell/LSB_filter_l", LSB_filter_l );
	settings.setValue ( "/sdr-shell/LSB_filter_h", LSB_filter_h );
	settings.setValue ( "/sdr-shell/DSB_filter_l", DSB_filter_l );
	settings.setValue ( "/sdr-shell/DSB_filter_h", DSB_filter_h );
	settings.setValue ( "/sdr-shell/CWL_filter_l", CWL_filter_l );
	settings.setValue ( "/sdr-shell/CWL_filter_h", CWL_filter_h );
	settings.setValue ( "/sdr-shell/CWU_filter_l", CWU_filter_l );
	settings.setValue ( "/sdr-shell/CWU_filter_h", CWU_filter_h );
	settings.setValue ( "/sdr-shell/SAM_filter_l", SAM_filter_l );
	settings.setValue ( "/sdr-shell/SAM_filter_h", SAM_filter_h );
	settings.setValue ( "/sdr-shell/FMN_filter_l", FMN_filter_l );
	settings.setValue ( "/sdr-shell/FMN_filter_h", FMN_filter_h );
	settings.setValue ( "/sdr-shell/AM_filter_l", AM_filter_l );
	settings.setValue ( "/sdr-shell/AM_filter_h", AM_filter_h );

	// Save memory cells
	QString f_string;

	for (int i=0; i<NUM_MEMS; i++) {
	  char buffer[256];

	  snprintf(buffer, 256, "/sdr-shell/f%d_frequency", i+1);
	  f_string.sprintf("%lld", f_cell[i]->getFrequency());
	  settings.setValue (buffer, f_string);

	  snprintf(buffer, 256, "/sdr-shell/f%d_mode", i+1);
	  settings.setValue(buffer, f_cell[i]->getMode());

	  snprintf(buffer, 256, "/sdr-shell/f%d_filter_l", i+1);
	  settings.setValue(buffer, f_cell[i]->getFilter_l());

	  snprintf(buffer, 256, "/sdr-shell/f%d_filter_h", i+1);
	  settings.setValue(buffer, f_cell[i]->getFilter_h());
	}

	for (int i=0; i<NUM_BANDMEMS; i++) {
	  char buffer[256];

	  snprintf(buffer, 256, "/sdr-shell/b%d_frequency", i+1);
	  f_string.sprintf("%lld", band_cell[i]->getFrequency());
	  settings.setValue (buffer, f_string );

	  if (band_cell[i]->getTxFrequency() != 0) {
	    snprintf(buffer, 256, "/sdr-shell/b%d_txfrequency", i+1);
	    f_string.sprintf("%lld", band_cell[i]->getTxFrequency());
	    settings.setValue(buffer, f_string );
	  }

	  snprintf(buffer, 256, "/sdr-shell/b%d_mode", i+1);
	  settings.setValue (buffer, band_cell[i]->getMode() );


	  snprintf(buffer, 256, "/sdr-shell/b%d_filter_l", i+1);
	  settings.setValue(buffer, band_cell[i]->getFilter_l() );

	  snprintf(buffer, 256, "/sdr-shell/b%d_filter_h", i+1);
	  settings.setValue(buffer, band_cell[i]->getFilter_h() );
	}

	settings.setValue ( "/sdr-shell/hamlib_rig", rigString );
	settings.setValue ( "/sdr-shell/useHamlib", (int) useHamlib );
	settings.setValue ( "/sdr-shell/hamlib_port", portString );
	settings.setValue ( "/sdr-shell/hamlib_speed", speedString );
	settings.setValue ( "/sdr-shell/usbOffset", usbOffset );
	settings.setValue ( "/sdr-shell/lsbOffset", lsbOffset );
	settings.setValue ( "/sdr-shell/slopeLowOffset", slopeLowOffset );
	settings.setValue ( "/sdr-shell/slopeHighOffset", slopeHighOffset );
	settings.setValue ( "/sdr-shell/rock_bound", (int) rock_bound );
	settings.sync();
}

void Main_Widget::finish()
{
	saveSettings();
	 //
    // terminate the sdr-core
    //
    delete pSpectrum;
    delete pMeter;
    delete pCmd;
    delete pUSBCmd;
    fprintf (stderr, "sdr-shell exiting.\n");
	exit ( 0 );
}

void Main_Widget::keyPressEvent ( QKeyEvent * e )
{
	static int state = 0;

	//printf("k %d\n", e->key());

	switch ( e->key() )
	{
		 case Qt::Key_Shift:
			switch ( e->key() )
			{
				case 82: state = RX_F; break;
				case 84: state = TX_F; break;
					//case 72: state = FILTER_H; break;
					//case 76: state = FILTER_L; break;
				default: break;
			}
			break;
        case Qt::Key_Alt:
			switch ( e->key() )
			{
				case 65: setMode ( RIG_MODE_AM , FALSE, FALSE ); break; // a
				case 76: setMode ( RIG_MODE_LSB , FALSE, FALSE ); break; // l
				case 85: setMode ( RIG_MODE_USB, FALSE, FALSE ); break; // u
				default: break;
			}
			break;
		default:
			process_key ( e->key() );
			break;
	}

	//printf("Key %d %d %d\n", e->key(), e->state(), Qt::AltButton );
	switch ( state )
	{
		case RX_F: rx_cmd ( e->key() ); break;
		default: break;
	}
}

void Main_Widget::rx_cmd ( int key ) // Leave for IF shift now.
{
	switch ( key )
	{
        case Qt::Key_Down: // Down arrow
		case 72: // h
			//if ( tuneStep > 0 ) tuneStep--;
			setTuneStep ( -1 );
			break;
        case Qt::Key_Left: // Left arrow
		case 74: // j
			fprintf ( stderr, "Left arrow rx_delta_f is: %d.\n",rx_delta_f);
			if ( rock_bound )
			{
				if ( rx_delta_f < sample_rate / 2 - 2000 )  //rx_delta_f > 0 when you tune down!
					rx_delta_f = rx_delta_f + ( int ) pow ( 10, tuneStep );
				else
				  rx_delta_f =  ( sample_rate / 2 - 2000 );
				setRxFrequency( 0 );
			} else {
				rx_f -= rx_delta_f;
				rx_delta_f = tuneCenter;
				rx_f = rx_f - (int) pow ( 10, tuneStep );
				rx_f += rx_delta_f;

				// Round to tuneStep
				//rx_f = (rx_f-rx_delta_f) / (int) pow ( 10, tuneStep );
				//rx_f = rx_f * (int) pow ( 10, tuneStep ) + rx_delta_f;

				// disable RIT when tuning with the arrow keys.
				if (enableRIT)
					set_RIT( 0 );
				setRxFrequency( 1 );
			}
			break;
        case Qt::Key_Up: // Up arrow
		case 76: // l
			//if ( tuneStep < 3 ) tuneStep++;
			setTuneStep ( +1 );
			break;
        case Qt::Key_Right: // Right arrow
		case 75:  // k
			fprintf ( stderr, "Right arrow rx_delta_f is: %d.\n",rx_delta_f);
			if ( rock_bound )
			{
				if ( rx_delta_f > -( sample_rate / 2 - 2000 ) )  //rx_delta_f < 0 when you tune up!
					rx_delta_f = rx_delta_f - ( int ) pow ( 10, tuneStep );
				else
				  rx_delta_f = - ( sample_rate / 2 - 2000 );
				setRxFrequency( 0 );
			} else {
				rx_f -= rx_delta_f;
				rx_delta_f = tuneCenter;
				rx_f = rx_f + (int) pow ( 10, tuneStep );
				rx_f += rx_delta_f;

				// Round to tuneStep
				//rx_f = (rx_f-rx_delta_f) / (int) pow ( 10, tuneStep );
				//rx_f = rx_f * (int) pow ( 10, tuneStep ) + rx_delta_f;

				// reset RIT when tuning with the arrow keys
				if (enableRIT)
					set_RIT( 0 );
				setRxFrequency( 1 );
			}
			break;
		case 4118:	// Page Up
			// ^F = 4129 (ctl) 70
		case 4119:	// Page Down
			// ^B = 4129 (ctl) 66
			// spectrogram->width()
			fprintf(stderr, "width = %d\n", width());
			break;
		case 71: //g
			if (useIF)
			{
			  setDefaultRxFrequency();
			}
			break;
		case '1':
			hScale=1.0;
			break;
		case '2':
			hScale=2.0;
			break;
		case '3':
			hScale=3.0;
			break;
		case '4':
			hScale = hScale = sample_rate / (bin_bw * (geometry().width()-20)); // spectrumFrame->width());
			break;
		case '0':
			hScale = 0.25;
			break;
		case '9':
			hScale = 0.5;
			break;
		case '8':
			hScale = 0.75;
			break;
		case 4096:
			finish();
			break;
		default:
			//printf("k %d\n", key);
			break;
	}
}




void Main_Widget::process_key ( int key )
{
	int step;

	step = 10;

	switch ( key )
	{
		case 85: // U
			setFilter_l ( -1 );
			break;
		case 73: // I
			setFilter_l ( +1 );
			break;
		case 79: // O
			setFilter_h ( -1 );
			break;
		case 80: // P
			setFilter_h ( +1 );
			break;
		case 91: // [
			if ( *filter_h <= 6000 && *filter_l > step )
			{
				*filter_h -= step;
				*filter_l -= step;
			}
			setFilter();
			break;
		case 93: // [
			if ( *filter_h < 6000 && *filter_l >= step )
			{
				*filter_h += step;
				*filter_l += step;
			}
			setFilter();
			break;
		case 81: // q
			if ( iqGain > -5000 ) iqGain--;
			cfgIQGainInput->setValue ( iqGain );
			setIQGain();
			break;
		case 87: // w
			if ( iqGain < 5000 ) iqGain++;
			cfgIQGainInput->setValue ( iqGain );
			setIQGain();
			break;
		case 69: // e
			if ( iqPhase > -5000 ) iqPhase--;
			cfgIQPhaseInput->setValue ( iqPhase );
			setIQPhase();
			break;
		case 82: // r
			if ( iqPhase < 5000 ) iqPhase++;
			cfgIQPhaseInput->setValue ( iqPhase );
			setIQPhase();
			break;
		case 90: // z
			if ( specApertureLow > 0 )
				specApertureLow = specApertureLow - 1;
			setCA_label();
			break;
		case 88: // x
			if ( specApertureLow < 120 &&
			        specApertureLow < specApertureHigh - 10 )
				specApertureLow = specApertureLow + 1;
			setCA_label();
			break;
		case 67: // c
			if ( specApertureHigh > 0 &&
			        specApertureHigh > specApertureLow + 10 )
				specApertureHigh = specApertureHigh - 1;
			setCA_label();
			break;
		case 86: // v
			if ( specApertureHigh < 120 )
				specApertureHigh = specApertureHigh + 1;
			setCA_label();
			break;
		case 84: //t  toggle transmit
			if (enableTransmit)
			{
				toggle_TX(0);
#if 0
				if (transmit)
				{
					transmit = 0;
					pTXCmd->sendCommand ("setTRX 0\n");
					pTXCmd->sendCommand ("setRunState 0\n");
					pUSBCmd->sendCommand("set ptt off\n" );
					fprintf (stderr, "set ptt off\n");
//!					TRX_label->setPixmap(rxPix);
//!					TRX_label->setLabel( RX );
					set_MUTE ( 0 );
				} else {
					transmit = 1;
					pTXCmd->sendCommand ("setRunState 2\n");
					pUSBCmd->sendCommand ("set ptt on\n" );
					pTXCmd->sendCommand ("setTRX 1\n");
					set_MUTE ( 1 );
					fprintf (stderr, "set ptt on\n");
//!					TRX_label->setPixmap(txPix);
//!					TRX_label->setLabel( TX );
				}
#endif
			} else {
				fprintf( stderr, "Transmit is not enabled\n");
			}
			break;
		case 32: // space turn transmit off
			transmit = 0;
			pUSBCmd->sendCommand("set ptt off\n" );
			fprintf (stderr, "set ptt off\n");
			break;
		default:
			break;
	}
}

void Main_Widget::setOurRxFrequency ( double frequency )
{
	char text[32];
	snprintf ( text, 32, "%11.6lf", ( double ) ( frequency ) / 1000000.0 );
	displayMutex.lock();
	lcd->display ( text );	
	displayMutex.unlock();
}

void Main_Widget::setDefaultRxFrequency ( )
{
	switch ( mode )
	{
		case RIG_MODE_USB:
			rx_delta_f = rx_if - rx_f + usbOffset + slopeTuneOffset; //1358;
			break;
		case RIG_MODE_LSB:
			rx_delta_f = rx_if -rx_f -lsbOffset + slopeTuneOffset;  //1585;  
			break;
		case RIG_MODE_CW:
			rx_delta_f = rx_if -rx_f + cwPitch + slopeTuneOffset; // This seems to be only correct with narrow filters in.
			break;
		case RIG_MODE_CWR:
			rx_delta_f = rx_if -rx_f - cwPitch + slopeTuneOffset; // This seems to be only correct with narrow filters in.
			break;
		case RIG_MODE_AM:
		case RIG_MODE_SAM:
		case RIG_MODE_DSB:
		case RIG_MODE_FM:
		default:
			rx_delta_f = rx_if - rx_f + slopeTuneOffset;
			break;
	}
	setRxFrequency( 1 );
}

//
// synth flag means set the external oscillator (via usbsoftrock), if
// configured to do so (rock_bound == 0)
//
void Main_Widget::setRxFrequency( int synth )
{
	char text[32];
	if ( !useIF )
	{
		snprintf ( text, 32, "......%11.6lf",
			( double ) ( rx_f - rx_delta_f ) / 1000000.0 );
		fprintf ( stderr, "Set the frequency (!IF): %lld - %d = %11.6lf '%s'\n",
			rx_f, rx_delta_f, 
			( rx_f - rx_delta_f) / 1000000.0, text);
		displayMutex.lock();
		lcd->display ( text );

		if (enableRIT) {
			tx_f_string.sprintf ("%11.0lf", ( double ) ( tx_delta_f - rx_delta_f ) );
			rit->setText( tx_f_string );
			fprintf ( stderr, "RIT %s\n", qPrintable(tx_f_string));
		}
		displayMutex.unlock();
	}
	fprintf ( stderr, "setOsc %d\n", rx_delta_f );
	pCmd->sendCommand ("setOsc %d %d\n", rx_delta_f, 0 );

	if (!rock_bound && !enableRIT && !enableSPLIT) {
		pTXCmd->sendCommand ("setOsc %d %d\n", -rx_delta_f, 1 );
		if ( synth ) {
			if (dualConversion) {
				fprintf (stderr, "set freq dual conversion %f %f %f\n",
					(rx_f)*1e-6, 
					((rx_f)*1e-6)/1.25,
					((rx_f)*1e-6)/1.25/4); 
				pUSBCmd->sendCommand("set freq %f\n", 
					((rx_f)*1e-6)/1.25/4);
			} else {
				fprintf (stderr, "USBsoftrock: set freq %f\n", (rx_f)*1e-6); 
				pUSBCmd->sendCommand("set freq %f\n", (rx_f)*1e-6);
			}
		}
	}
}

void Main_Widget::setTxFrequency()
{
	fprintf ( stderr, "setOsc %d\n", rx_delta_f );
	pTXCmd->sendCommand ("setOsc %d %d\n", -rx_delta_f, 1 );
}

void Main_Widget::setFilter_l ( int n )
{
	int step;

	step = 10;

	if ( *filter_l > -6000 && n == -1 )
		*filter_l -= step;

	if ( *filter_l < *filter_h - step && n == 1 )
		*filter_l += step;

	setFilter();
}

void Main_Widget::setFilter_h ( int n )
{
	int step;

	step = 10;

	if ( *filter_h < 6000  && n >= 1 )
		*filter_h += step * n;

	if ( *filter_h > *filter_l + step && n <= -1 )
		*filter_h += step * n;

	setFilter();
}

void Main_Widget::setFilter()
{
	pCmd->sendCommand ("setFilter %d %d\n", *filter_l, *filter_h );
	pTXCmd->sendCommand ("setFilter %d %d 1\n", *filter_l, *filter_h );
	fprintf ( stderr, "setFilter %d %d\n", *filter_l, *filter_h );
}

void Main_Widget::setLowerFilterScale ( int x )
{
	float bin_bw = sample_rate/(float)spec_width;
	int stop_band;

	//stop_band = ( int ) ( ( ( x - ( spectrogram->width() / 2 ) ) * bin_bw ) ) /10*10;
	stop_band = (int)(((x - (spectrogram->width() / 2)) * bin_bw) * hScale) / 10*10;

	if ( stop_band < *filter_h ) {
		*filter_l = stop_band;
		setFilter();
	}
}

void Main_Widget::setUpperFilterScale ( int x )
{
	float bin_bw = sample_rate/(float)spec_width;
	int stop_band;

	//stop_band = ( int ) ( ( ( x - ( spectrogram->width() / 2 ) ) * bin_bw ) ) /10*10;
	stop_band = (int)(((x - (spectrogram->width() / 2)) * bin_bw) * hScale) / 10*10;

	if ( stop_band > *filter_l ) {
		*filter_h = stop_band;
		setFilter();
	}
}

void Main_Widget::setCA_label()
{
	char text[20];
	snprintf ( text, 20, "CA: %4d : %4d",
	          ( int ) specApertureLow - 140,
	          ( int ) specApertureHigh - 140 );
	CA_label->setText ( text );
}

void Main_Widget::setTuneStep ( int step )
{
    if ( rock_bound ) {
	  if ( tuneStep < 3 && step > 0 ) tuneStep++;
	  else if ( tuneStep > 0 && step < 0 ) tuneStep--;
	} else {
	  if ( tuneStep < 8 && step > 0 ) tuneStep++;
	  else if ( tuneStep > 0 && step < 0 ) tuneStep--;
	}

	if (tuneStep != 0) {
		step_1Hz_frame->setPalette( QColor( 50, 50, 100 ) );
		step_1Hz_frame->setAutoFillBackground(true);
	}
	if (tuneStep != 1) {
		step_10Hz_frame->setPalette( QColor( 50, 50, 100 ) );
		step_10Hz_frame->setAutoFillBackground(true);
	}
	if (tuneStep != 2) {
		step_100Hz_frame->setPalette( QColor( 50, 50, 100 ) );
		step_100Hz_frame->setAutoFillBackground(true);
	}
	if (tuneStep != 3) {
		step_1KHz_frame->setPalette( QColor( 50, 50, 100 ) );
		step_1KHz_frame->setAutoFillBackground(true);
	}
	if (tuneStep != 4) {
		step_10KHz_frame->setPalette( QColor( 50, 50, 100 ) );
		step_10KHz_frame->setAutoFillBackground(true);
	}
	if (tuneStep != 5) {
		step_100KHz_frame->setPalette( QColor( 50, 50, 100 ) );
		step_100KHz_frame->setAutoFillBackground(true);
	}
	if (tuneStep != 6) {
		step_1MHz_frame->setPalette( QColor( 50, 50, 100 ) );
		step_1MHz_frame->setAutoFillBackground(true);
	}
	if (tuneStep != 7) {
		step_10MHz_frame->setPalette( QColor( 50, 50, 100 ) );
		step_10MHz_frame->setAutoFillBackground(true);
	}
	if (tuneStep != 8) {
		step_100MHz_frame->setPalette( QColor( 50, 50, 100 ) );
		step_100MHz_frame->setAutoFillBackground(true);
	}

	switch ( tuneStep )
	{
		case 0:
			step_1Hz_frame->setPalette( QColor( 200, 200, 255 ) );
			step_1Hz_frame->setAutoFillBackground(true);
			break;
		case 1:
			step_10Hz_frame->setPalette( QColor( 200, 200, 255 ) );
			step_10Hz_frame->setAutoFillBackground(true);
			break;
		case 2:
			step_100Hz_frame->setPalette( QColor( 200, 200, 255 ) );
			step_100Hz_frame->setAutoFillBackground(true);
			break;
		case 3:
			step_1KHz_frame->setPalette( QColor( 200, 200, 255 ) );
			step_1KHz_frame->setAutoFillBackground(true);
			break;
		case 4: 
			step_10KHz_frame->setPalette( QColor( 200, 200, 255 ) );
			step_10KHz_frame->setAutoFillBackground(true);
			break;
		case 5: 
			step_100KHz_frame->setPalette( QColor( 200, 200, 255 ) );
			step_100KHz_frame->setAutoFillBackground(true);
			break;
		case 6: 
			step_1MHz_frame->setPalette( QColor( 200, 200, 255 ) );
			step_1MHz_frame->setAutoFillBackground(true);
			break;
		case 7: 
			step_10MHz_frame->setPalette( QColor( 200, 200, 255 ) );
			step_10MHz_frame->setAutoFillBackground(true);
			break;
		case 8: 
			step_100MHz_frame->setPalette( QColor( 200, 200, 255 ) );
			step_100MHz_frame->setAutoFillBackground(true);
			break;
		default:
			break;
	}
}

void Main_Widget::setMode ( rmode_t m, bool displayOnly, bool force )
{
	QString	modeStr;

	if (mode == m && ! force) {
		fprintf(stderr, "set mode: no mode change\n");
		return;
	}
	
	mode = m;

	QColor c_off ( 0, 0, 0 );
	QColor c_on ( 200, 0, 0 );

    LSB_label->setPalette( c_off );
    LSB_label->setAutoFillBackground(true);
    USB_label->setPalette( c_off );
    USB_label->setAutoFillBackground(true);
    DSB_label->setPalette( c_off );
    DSB_label->setAutoFillBackground(true);
    AM_label->setPalette( c_off );
    AM_label->setAutoFillBackground(true);
    CWL_label->setPalette( c_off );
    CWL_label->setAutoFillBackground(true);
    CWU_label->setPalette( c_off );
    CWU_label->setAutoFillBackground(true);
    SAM_label->setPalette( c_off );
    SAM_label->setAutoFillBackground(true);
    FMN_label->setPalette( c_off );
	FMN_label->setAutoFillBackground(true);

	switch ( mode )
	{
		case RIG_MODE_USB:
			modeStr = "USB";
			pCmd->sendCommand ("setMode %d %d\n", USB );
			pTXCmd->sendCommand ("setMode %d %d\n", USB, 1 );
			fprintf ( stderr, "setMode %d\n", USB );
			filter_l = &USB_filter_l; //20;
			filter_h = &USB_filter_h; //2400;
			USB_label->setPalette( c_on );
			USB_label->setAutoFillBackground(true);
			if ( !displayOnly && useHamlib )
				//fprintf ( stderr, "emitted changeRigMode at USB. \n" );
				emit changeRigMode ( RIG_MODE_USB, 2700 );
			break;
		case RIG_MODE_LSB:
			modeStr = "LSB";
			pCmd->sendCommand ("setMode %d %d\n", LSB );
			pTXCmd->sendCommand ("setMode %d %d\n", LSB, 1 );
			fprintf ( stderr, "setMode %d\n", LSB );
			filter_l = &LSB_filter_l; //-2400;
			filter_h = &LSB_filter_h; //-20;
            LSB_label->setPalette( c_on );
            LSB_label->setAutoFillBackground(true);
			if ( !displayOnly && useHamlib )
				emit changeRigMode ( RIG_MODE_LSB, 2700 );
			break;
		case RIG_MODE_DSB:
			modeStr = "DSB";
			pCmd->sendCommand ("setMode %d %d\n", DSB );
			pTXCmd->sendCommand ("setMode %d %d\n", DSB, 1 );
			fprintf ( stderr, "setMode %d\n", DSB );
			filter_l = &DSB_filter_l; //-2400;
			filter_h = &DSB_filter_h; //2400;
            DSB_label->setPalette( c_on );
            DSB_label->setAutoFillBackground(true);            
			if ( !displayOnly && useHamlib)
				emit changeRigMode ( RIG_MODE_USB, 6000 );
			break;
		case RIG_MODE_AM:
			modeStr = "AM";
			pCmd->sendCommand ("setMode %d %d\n", AM );
			pTXCmd->sendCommand ("setMode %d %d\n", AM, 1 );
			fprintf ( stderr, "setMode %d\n", AM );
			filter_l = &AM_filter_l; //-2400;
			filter_h = &AM_filter_h; //2400;
            AM_label->setPalette( c_on );
            AM_label->setAutoFillBackground(true);            
			if ( !displayOnly && useHamlib )
				emit changeRigMode ( RIG_MODE_AM, 6000 );
			break;
		case RIG_MODE_CWR:
			modeStr = "CWR";
			pCmd->sendCommand ("setMode %d %d\n", CWL );
			pTXCmd->sendCommand ("setMode %d %d\n", CWL, 1 );
			fprintf ( stderr, "setMode %d\n", CWL );
			filter_l = &CWL_filter_l; //-500;
			filter_h = &CWL_filter_h; //-200;
            CWL_label->setPalette( c_on );
            CWL_label->setAutoFillBackground(true);            
			if ( !displayOnly && useHamlib )
				emit changeRigMode ( RIG_MODE_CWR, 500 );
			break;
		case RIG_MODE_CW:
			modeStr = "CW";
			pCmd->sendCommand ("setMode %d %d\n", CWU );
			pTXCmd->sendCommand ("setMode %d %d\n", CWU, 1 );
			fprintf ( stderr, "setMode %d\n", CWU );
			filter_l = &CWU_filter_l; //200;
			filter_h = &CWU_filter_h; //500;
            CWU_label->setPalette( c_on );
            CWU_label->setAutoFillBackground(true);            
			if ( !displayOnly && useHamlib )
				emit changeRigMode ( RIG_MODE_CW, 500 );
			break;
		case RIG_MODE_SAM:
			modeStr = "SAM";
			pCmd->sendCommand ("setMode %d %d\n", SAM );
			pTXCmd->sendCommand ("setMode %d %d\n", SAM, 1 );
			fprintf ( stderr, "setMode %d\n", SAM );
			filter_l = &SAM_filter_l; //-2400;
			filter_h = &SAM_filter_h; //2400;
            SAM_label->setPalette( c_on );
            SAM_label->setAutoFillBackground(true);            
			/*if (!displayOnly) 
			emit changeRigMode ( RIG_MODE_AM, 12000 ); */ //Some rigs can do syncronous AM, but not mine.  KL7NA  uncomment this for them.*/
			break;
		case RIG_MODE_FM:
			modeStr = "FM";
			pCmd->sendCommand ("setMode %d %d\n", FMN );
			pTXCmd->sendCommand ("setMode %d %d\n", FMN );
			fprintf ( stderr, "setMode %d\n", FMN );
			filter_l = &FMN_filter_l; //-4000;
			filter_h = &FMN_filter_h; //4000;
            FMN_label->setPalette( c_on );
            FMN_label->setAutoFillBackground(true);            
			if ( !displayOnly && useHamlib )
				emit changeRigMode ( RIG_MODE_FM, 12000 );
			break;
		default:
			modeStr = "Unknown";
			filter_l = &USB_filter_l; //20;
			filter_h = &USB_filter_h; //2400;
	}
	fprintf( stderr, "Set mode: %s\n", qPrintable(modeStr));

	// call external mode setting hook.
	if (sdr_mode != NULL) {
		QString cmd = sdr_mode;
		cmd += " ";
		cmd += modeStr;
		fprintf( stderr, "Set mode: %s\n", qPrintable(cmd));
		system(qPrintable(cmd));
	}
	setFilter();
	if ( useIF ) setDefaultRxFrequency();
	else setRxFrequency( 1 );
}


void Main_Widget::setIQGain()
{
	pCmd->sendCommand ("setcorrectIQgain %d\n", iqGain );
	fprintf ( stderr, "setcorrectIQgain %d\n", iqGain );
	// The following sets the output gain.
	pCmd->sendCommand ("setGain %d %d\n", 0,1,0 );
	//pCmd->sendCommand ("setMode %d %d\n", 0,0,0 );
	fprintf ( stderr, "setGain %d %d %d\n",0,1,0 );
	//fprintf ( stderr, "setMode %d %d %d\n",0,0,0 );
	printf ( "Set the RX Gain.\n" );
}

void Main_Widget::setIQPhase()
{
	pTXCmd->sendCommand ("setcorrectIQphase %d\n", iqPhase );
	fprintf ( stderr, "(TX)setcorrectIQphase %d\n", iqPhase );
}

void Main_Widget::setTxIQGain()
{
	pTXCmd->sendCommand ("setcorrectIQgain %d\n", txIQGain );
	fprintf ( stderr, "(TX)setcorrectIQgain %d\n", txIQGain );
}

void Main_Widget::setTxIQPhase()
{
	pTXCmd->sendCommand ("setcorrectIQphase %d %d\n", txIQPhase );
	fprintf ( stderr, "(TX)setcorrectIQphase %d\n", txIQPhase );
}

void Main_Widget::setTxGain( int inout )
{
	int gain = inout ? txGain : micGain;
	pTXCmd->sendCommand ("setGain %d %d %d\n", 1, inout, gain );
	fprintf ( stderr, "(TX)set%sGain %d %d %d\n", 
		inout ? "Output" : "Mic",
		1, inout, gain );
}


void Main_Widget::readMeter()
{
	int j, k = 0, label;
	float rxm[MAXRX * RXMETERPTS];
	char text[10];
	static double meter_peak = 0;
	static int peak_count = 0;

	pCmd->sendCommand ("reqRXMeter %d\n", getpid() );
	pMeter->fetch (&label, rxm, MAXRX * RXMETERPTS);

	if ( rxm[0] >= meter_peak )
	{
		peak_count = 0;
		meter_peak = rxm[0];
		snprintf ( text, 10, "%4d", ( int ) ( rxm[0] - metrCal ) );
		signal_dBm->setText ( text );
	}
	else
	{
		if ( peak_count++ >= 15 )
		{
			peak_count = 0;
			meter_peak = rxm[0];
		}
		else
		{
			for ( j = 0; j < 34; j++ )
			{
				if ( meter_peak - metrCal < s_dbm[j] )
				{
					k = j;
					break;
				}
			}
		}
	}

	for ( j = 0; j < 34; j++ )
	{
		if ( rxm[0] - metrCal > s_dbm[j] )
			signalBargraph[j]->setPalette( QColor( 0,180,255 ));
		else if ( j != k )
			signalBargraph[j]->setPalette( QColor( 50,50,50 ) );
		else if ( k != 0 )
			signalBargraph[j]->setPalette( *signalColor[j] );

		signalBargraph[j]->setAutoFillBackground(true);
	}
}

// Fetch the spectrum data from sdr-core
//
void Main_Widget::readSpectrum()
{
	int j, k, l, m, n;
	int label, stamp;
	float raw_spectrum[DEFSPEC];
	float a;

	//fprintf( stderr, ".");
	updated++;
	pCmd->sendCommand ("reqSpectrum %d\n", getpid() ); 

	if (spec_width == DEFSPEC) {
		pSpectrum->fetch (&stamp, &label, spectrum, DEFSPEC);
	} else {
		pSpectrum->fetch (&stamp, &label, raw_spectrum, DEFSPEC);

		// spectrum from dttsp is centered at Osc.
		// if Osc != 0, then some bins on one end are wrapped data.  Zero them.
		// this is only a visible problem when scaling the data...
#if 0
		if (rx_delta_f < 0) {
			float bin_bw = sample_rate/(float)DEFSPEC;
			a = abs(rx_delta_f) / bin_bw;
			j = DEFSPEC - (int)a;
			if (once)
				printf("Osc=%d bins=%.1f j=%d\n",
					rx_delta_f, a, j);
			for(; j < DEFSPEC; j++) {
				raw_spectrum[j] = 0;
			}
		}
#endif

		// resample spectrum to be smaller
		// use the max value of N samples
		// j = raw sample index
		// k = downsampled index
		n = DEFSPEC / spec_width;
		for (j = k = 0; j < DEFSPEC; j += n, k++) {
			a = raw_spectrum[j];

			for (l = j+1, m=1; m < n; l++, m++) {
				if (raw_spectrum[l] > a) {
					a = raw_spectrum[l];
				}
			}

			spectrum[k] = a;
		}
		for ( ; k < DEFSPEC; k++) {
			spectrum[k] = 0;
		}
		// if the dttsp Osc frequency is not zero, one end or the other has
		// wrapped data.
	}

	//drawSpectrogram();
	//if ( SPEC_state ) plotSpectrum ( spectrum_head );
	repaint();
}

void Main_Widget::drawSpectrogram()
{
    int x, x1, x2;
    static int y = 0;
    int pwr, specval;
    float pwr_range;
    bool spec_debug = false;

    pwr_range = specApertureHigh - specApertureLow;
    if(spec_debug)
	printf("drawSpectrogram() \n");

//tf    QImage spectrogramLine( spectrogram->width() , 1, QImage::Format_RGB32 );
    QImage spectrogramLine( spectrum_width , 1, QImage::Format_RGB32 );
   
//tf    x1 = DEFSPEC/2 - spectrogram->width()/2;
//tf    x2 = x1 + spectrogram->width();
    x1 = DEFSPEC/2 - spectrum_width/2;
    x2 = x1 + spectrum_width;
    if(spec_debug)
	{
	printf("drawSpectrogram::x1 = %d\n", x1);
	printf("drawSpectrogram::x2 = %d\n", x2);
	printf("drawSpectrugram::y = %d\n", y);
	}    
    for ( x = 0; x < DEFSPEC; x++)		// for number of fft samples per line.. 
	{
        						// Compute the power (magnified)
        pwr = (int)((spectrum[ x ] + specCal - specApertureLow) * 
					(SPECFRM_V / pwr_range));
        if ( pwr > 119 ) pwr = 119;			// Sanitize the value
        else if ( pwr < 0 ) pwr = 0;
        						
	specval = (int)(spectrum[ x ] + specCal);	//Save power for spectrum display
	if(specval > 119) specval = 119;
	if(specval < 0) specval = 0;
	spectrum_history[y][x] = specval;
//tf        spectrum_history[y][x] = (int)(spectrum[ x ] + specCal);

        if ( x >= x1 && x < x2 )	// If bin is in visible range, plot it.
	    {
            uint *p = (uint *)spectrogramLine.scanLine(0) + (x - x1); // Set pixel color
            *p = qRgb( spec_r[pwr], spec_g[pwr], spec_b[pwr] );

            // Turn on vertical filter line: Note that filterLine is set to 0 by default, so 
            // no filter boundary lines will be drawn in the spectrogram.  The following two
            // if statements will fail.  (MS)
            if ( filterLine &&
                 (x - x1) == spectrogram->width() / 2 +
                 (int)(*filter_l / bin_bw) ) {
                *p = qRgb( 100, 255, 100 );
            }

            if ( filterLine &&
                 (x - x1) == spectrogram->width() / 2 +
                 (int)(*filter_h / bin_bw) ) {
                *p = qRgb( 100, 255, 100 );
            }        
                    					// Set the center mark (red)
//tf            if ( (x - x1) == spectrogram->width() / 2 )
//tf                *p = qRgb( 255,0,0 );
            if ( (x - x1) == spectrum_width / 2 )
                *p = qRgb( 255,0,0 );
        }
    }
    spectrum_head = y;
   	//////////////////////////////////////////////// Now, draw this spectrum line
    QPainter p;
    p.begin( this );
    //p.begin( spectrogramFrame );
    p.scale((1.0 / hScale), 1.0);
	y = (y+1) % spectrogram->height();
    p.drawImage( 2, y + TOPFRM_V, spectrogramLine );


    //Temporarily disable the bandpass width yellow indicator line in the waterfall display
    p.setPen( Qt::yellow );
    p.drawLine( spectrum_width / 2 + 
				((int)(*filter_l / bin_bw) + 2), y+1 + TOPFRM_V,
                spectrum_width / 2 + 
				((int)(*filter_h / bin_bw) + 2), y+1 + TOPFRM_V );
    p.drawLine( spectrum_width / 2 + 
				((int)(*filter_l / bin_bw) + 2), y+2 + TOPFRM_V,
               spectrum_width / 2 + 
				((int)(*filter_h / bin_bw) + 2), y+2 + TOPFRM_V );
    p.end();
}    

/****************************************************************************************
*	drawPassBandScale()	draws the pass-band scale on screen			*
****************************************************************************************/
void Main_Widget::drawPassBandScale()
{
//printf("drawPassBandScale() \n");
  char temp[20];
  int x1, x2;

		// x1 is the lower-BP filter location
		// x2 is the upper-BP filter location  
  x1 = spectrogram->width() / 2 + ((int)((*filter_l / bin_bw)+2) / hScale);
  x2 = spectrogram->width() / 2 + ((int)((*filter_h / bin_bw)+2) / hScale);

//printf("dPBS x1 = %d, x2 = %d\n", x1, x2);
  QPainter p;
  p.begin( this );

  p.setFont( *font1 );

  p.eraseRect( 2, spectrogram->height() + TOPFRM_V, pbscale->width()+1, 
		pbscale->height()+1 );
  p.fillRect(2, spectrogram->height() + TOPFRM_V,pbscale->width()+1,
		pbscale->height()+1,QColor(100, 0, 0));  
  sprintf( temp, "%5d", *filter_l );
  p.setPen( Qt::cyan );
  p.drawText( x1 - 11 -1 - font1Metrics->maxWidth() * 5, 
			  font1Metrics->ascent() + 1 + TOPFRM_V + spectrogram->height(), temp );
  //////////////////////////////////////////////////// Draw low-side passband arrow
  p.setPen( Qt::yellow );
  p.drawLine( x1 - 10 -1, 7 + TOPFRM_V + spectrogram->height(), 
		x1, 7 + TOPFRM_V + spectrogram->height()); //horizontal part of arrow
  p.drawLine( x1 - 4 -1, 4 + TOPFRM_V + spectrogram->height(), 
		x1 -1, 7 + TOPFRM_V + spectrogram->height() );
  p.drawLine( x1 - 4 -1, 10 + TOPFRM_V + spectrogram->height(), 
		x1 -1, 7 + TOPFRM_V + spectrogram->height()  );
  p.drawLine( x1, 0 + TOPFRM_V + spectrogram->height(), 
		x1, PBSFRM_V + TOPFRM_V + spectrogram->height() ); //vertical line 

  sprintf( temp, "%-5d", *filter_h );
  p.setPen( Qt::cyan );
  p.drawText( x2 + 12, font1Metrics->ascent() + 1 + TOPFRM_V + spectrogram->height(), temp );

  /////////////////////////////////////////////////// Draw high-side passband arrow
  p.setPen( Qt::yellow );
 p.drawLine( x2 + 10, 7 + TOPFRM_V +  spectrogram->height(), 
		x2 + 1, 7 + TOPFRM_V + spectrogram->height()); //horizontal part of arrow
  p.drawLine( x2 + 4, 4 + TOPFRM_V +  spectrogram->height(), 
		x2 + 1, 7 + TOPFRM_V + spectrogram->height() );
  p.drawLine( x2 + 4, 10 + TOPFRM_V +  spectrogram->height(), 
		x2 + 1, 7 + TOPFRM_V + spectrogram->height() );
  p.drawLine( x2, 0 + TOPFRM_V +  spectrogram->height(), 
		x2, PBSFRM_V + TOPFRM_V + spectrogram->height() ); //vertical line
  p.end();
}

/****************************************************************************************
*	plotSpectrum()		Plots the spectrum DATA on the screen			*
****************************************************************************************/
void Main_Widget::plotSpectrum( int y )
{
//printf("plotSpectrum( int y ) \n");
    int x, y1, x1, x2, spectrumFrame_width_less1;
    double kHz_step;
    int f;
    int f1, f2;
    char f_text[10];
    bool debug = false;

    spectrumFrame_width_less1 = spectrum_width - 1;

	//  Set up the dark-green BP filter swath start/stop in spectrum display
    f1 = spectrum_width / 2 + (int)(*filter_l / bin_bw) + 2;	// f1 is low filter
    f2 = spectrum_width / 2 + (int)(*filter_h / bin_bw) + 2;	// f2 is high filter

//tf    x1 = DEFSPEC/2 - spectrogram->width()/2;
//tf    x2 = x1 + spectrogram->width();
	// 4096/2 - width/2
    x1 = (DEFSPEC/2 - spectrum_width / 2) -3;
    x2 = x1 + spectrum_width;

    if(hScale > 2.0)
//tf    	kHz_step = 5000 / ( sample_rate / 4096.0 );
    	kHz_step = 5000 / bin_bw;
    else
//tf    	kHz_step = 1000 / ( sample_rate / 4096.0 );
    	kHz_step = 1000 / bin_bw;
	
    if(debug)
	{
	printf("\nStart drawspectrum debug: y = %d \n", y);
	printf("f1 = %d, f2 = %d\n",f1, f2);
	printf("%s %d, ", "spectrumFrame->width() = ", spectrumFrame->width());
	printf("%s %d \n", "spectrumFrame->height() = ", spectrumFrame->height());
	printf("%s %d, ", "spectrogram->width() = ", spectrogram->width());
	printf("%s %d \n", "spectrogram->height() = ", spectrogram->height());
	printf("%s %d \n", "spectrum_width = ", spectrum_width);
	printf("x1 = %d, x2 = %d\n", x1, x2);
	printf("sample_rate = %d \n", sample_rate);
	printf("kHz_step = %.2f Hz, ", kHz_step);     
	printf("bin_bw = %.2f Hz\n", bin_bw);
	}

    QPainter p;
    p.begin( this ); 
    p.scale( (1.0 / hScale), 1.0);  	// set the horizontal scaling value 
    if(hScale > 2.0)
		p.setFont ( *font2 );
    else
    	p.setFont ( *font1 );

	//////////////////////////////////  erase last spectrogram frame & redraw filter
    p.eraseRect( 2, spectrogram->height() + PBSFRM_V + TOPFRM_V, spectrum_width,
				spectrumFrame->height() + 6  ); // erase last spectrogram
	//////////////// next line puts in dark green area showing audio bandpass filter   
    p.fillRect( f1 , 0 + spectrogram->height() + PBSFRM_V + TOPFRM_V, f2 - f1,
				spectrumFrame->height() + 4 , QColor( 0, 50, 0 ) );
    //////////////////// Draw the 1 kHz positive marks and the vertical lines below them.
    f = 0;
    for ( double dx = spectrum_width / 2; 
		  dx <= spectrum_width; 
		  dx = dx + kHz_step ) {
        p.setPen( QColor( 100, 100, 100 ) );
        p.drawLine( (int)rint(dx) + 2, 
			font1Metrics->ascent() + spectrogram->height() + PBSFRM_V + TOPFRM_V, 
			(int)rint(dx) + 2, 
			spectrumFrame->height() + spectrogram->height() + PBSFRM_V + TOPFRM_V );
        //sprintf( f_text, "%lf", (double)((rx_f + f) / 1000000.0) );
		sprintf( f_text, "%d", f );
        p.setPen( QColor( 255, 255, 255 ) );
        if(hScale > 2.0) {
	    f = f + 5000;
            p.drawText( ((int)rint(dx) + 1) - (font2Metrics->maxWidth() * 
				 strlen( f_text )) / 2, 
			20 + spectrogram->height() + PBSFRM_V + TOPFRM_V, 
			f_text );
	    } else {
	    f = f + 1000;
            p.drawText( ((int)rint(dx) + 1) - (font1Metrics->maxWidth() * 
				 strlen( f_text )) / 2, 
			10 + spectrogram->height() + PBSFRM_V + TOPFRM_V, 
			f_text );
	    }
    }
    //////////////////// Draw the 1 kHz negative marks and the vertical lines below them.
    if(hScale > 2.0)
		f = -5000;
    else
		f = -1000;
    for ( double dx = spectrum_width / 2 - kHz_step; 
		  dx >= 0; 
		  dx = dx - kHz_step ) {
        p.setPen( QColor( 100, 100, 100 ) );
        p.drawLine( (int)rint(dx) + 2, 
			font1Metrics->ascent() + spectrogram->height() + PBSFRM_V + TOPFRM_V, 
			(int)rint(dx) + 2, 
			spectrumFrame->height() + spectrogram->height() + PBSFRM_V + TOPFRM_V );
					
        //sprintf( f_text, "%lf", (double)((rx_f + f) / 1000000.0) );
        sprintf( f_text, "%d", f );
        p.setPen( QColor( 255, 255, 255 ) );
        if(hScale > 2.0)
	    {
            p.drawText( ((int)rint(dx) + 1) - (font2Metrics->maxWidth() * 
					(strlen( f_text ) + 1)) / 2, 
			20 + spectrogram->height() + PBSFRM_V + TOPFRM_V, 
			f_text );		
	    f = f - 5000;
	    } else {
            p.drawText( ((int)rint(dx) + 1) - (font1Metrics->maxWidth() * 
					(strlen( f_text ) + 1)) / 2, 
			10 + spectrogram->height() + PBSFRM_V + TOPFRM_V, 
			f_text );		
	    f = f - 1000;
	    }
    }
    ///////////////////////////////////////////////////////////// Now, Draw the dB marks
    for ( int i = 20; i < SPECFRM_V; i += 20 ) {
        p.setPen( QColor( 100, 100, 100 ) );
        p.drawLine( 1, i + spectrogram->height() + PBSFRM_V + TOPFRM_V, spectrum_width - 2,
						 i + spectrogram->height() + PBSFRM_V + TOPFRM_V );
    }
	////////////////////////////////////////////////// Draw the color aperture lines
	QPen pen( Qt::DotLine );
	pen.setColor( QColor( 255, 50, 0 ) );
	p.setPen( pen );
	p.drawLine( 2, (int)(specApertureLow - SPECFRM_V) * -1 + spectrogram->height() + PBSFRM_V+TOPFRM_V, 
				spectrum_width, (int)(specApertureLow - SPECFRM_V) * 
					-1 + spectrogram->height() + PBSFRM_V + TOPFRM_V );
	p.drawLine( 2, (int)(specApertureHigh - SPECFRM_V) * -1 + spectrogram->height() +PBSFRM_V+TOPFRM_V,
				spectrum_width, (int)(specApertureHigh - SPECFRM_V) *
					 -1  + spectrogram->height() + PBSFRM_V + TOPFRM_V);
    //////////////////////////////////////////////////////////////// Draw the dB labels
    for ( int i = 0; i < SPECFRM_V; i += 20 )
	{
        p.setPen( QColor( 255, 255, 255 ) );
        sprintf( f_text, "%d", -i - 40 );
        p.drawText( 4, i + 19 + spectrogram->height() + PBSFRM_V + TOPFRM_V, f_text );        
        sprintf( f_text, "%4d", -i - 40 );
		if(hScale > 2) {
            p.drawText( spectrum_width - 
			font2Metrics->maxWidth() * 4 - 2,
		i + 19 + spectrogram->height() + PBSFRM_V + TOPFRM_V, f_text );

	    } else {
            p.drawText( spectrum_width - 
			font1Metrics->maxWidth() * 4 - 2,
		i + 19 + spectrogram->height() + PBSFRM_V + TOPFRM_V, f_text );
	    }
    }    
	////////////////////////////////////////// Finally, Draw the actual Spectrum data
    y1 =  spectrogram->height() + PBSFRM_V + TOPFRM_V + spectrumFrame->height();
//    p.setPen( QColor( 0, 200, 0 ) );
// tf:change color back to green
//    p.setPen( Qt::yellow );
    p.setPen( Qt::green );
	// Draw Spectrum                     + 2
    for ( x = 1; x < spectrumFrame_width_less1 ; x++ )
	{
    	if(specLineFill) {
        	p.drawLine( x, spectrogram->height() + PBSFRM_V + TOPFRM_V + SPECFRM_V,
           	         x, spectrogram->height() + PBSFRM_V + TOPFRM_V + SPECFRM_V - 
					spectrum_history[y][x + x1 + 1] );
		} else {
        	p.drawLine( x - 1, spectrogram->height() + PBSFRM_V + TOPFRM_V + SPECFRM_V - 
					spectrum_history[y][x + x1],
                    x, spectrogram->height() + PBSFRM_V + TOPFRM_V + SPECFRM_V - 
					spectrum_history[y][x + x1 + 1] );
		}
    }
	p.end();   		 
}


void Main_Widget::spectrogramClicked ( int x )
{
	int f;

	int f_limit = sample_rate/2 - 2000;
	if ( !useIF )  // Disable changing frequency for IF mode.  Use arrows for IF shift.
	{
		//f = ( int ) ( ( sample_rate/(float)spec_width ) * ( spectrogram->width() /2 - x ) );
		f = (int)(bin_bw * (spectrogram->width() / 2 - x) * hScale);
		
		fprintf(stderr, "spectrogramClicked: x = %d f = %d rx_f = %lld freq = %.6lf\n", x, f, rx_f,
			(double)( (rx_f - rx_delta_f) - f ) / 1000000.0);

		if ( rock_bound )
		{
			rx_delta_f = rx_delta_f + f + *filter_l + ( *filter_h - *filter_l ) / 2 ;	
			if ( rx_delta_f >  f_limit ) {
				rx_delta_f =  f_limit;
			} else if ( rx_delta_f < -f_limit ) {
				rx_delta_f = -f_limit;
			}
		    setRxFrequency( 0 );
		} else {
			// Clicking re-tunes the synth and osc.  Set osc to center.
			rx_f -= rx_delta_f;
			rx_delta_f =  tuneCenter;	//Make it not tuned to the center
			rx_f += rx_delta_f;

			rx_f = rx_f - f - *filter_l - ( *filter_h - *filter_l ) / 2;
			setRxFrequency( 1 );
		}
	}
}

void Main_Widget::f_at_mousepointer ( int x )
{
	int f;
	char temp[32];

	//f = ( int ) ( ( sample_rate/(float)spec_width ) * ( spectrogram->width() /2 - x ) );
	f = (int)((bin_bw) * (spectrogram->width()/2 - x) * hScale);
	//f = (int)((bin_bw) * (spectrum_width / 2 - x));

	snprintf ( temp, 32, "%.6lf", ( double ) ( (rx_f - rx_delta_f) - f ) / 1000000.0 );
	M_label->setText ( temp );
}

void Main_Widget::tune ( int x )
{
	int f_limit = sample_rate/2 - 2000;

	// if (spectrogram->width() < spec_width)
	// f_limit = sample_rate / 2 - spectrogram->width() / 2

	// use usbsoftrock if the tuning step is large enough
	if (!rock_bound && ( x > 10000 || x < -10000 )) {
		rx_delta_f = tuneCenter;
		rx_f = rx_f + x;
		setRxFrequency( 1 );
	} else {
		rx_delta_f += x;
		if ( rx_delta_f >  f_limit ) rx_delta_f =  f_limit;
		if ( rx_delta_f < -f_limit ) rx_delta_f = -f_limit;
		setRxFrequency( 0 );
	}
}

void Main_Widget::focusInEvent ( QFocusEvent * )
{
    //ctlFrame->setPalette( QColor( 255, 200, 0 ) );
    ctlFrame->setPalette( QColor( 0, 0, 0 ) );
    ctlFrame->setAutoFillBackground(true);
}

void Main_Widget::focusOutEvent ( QFocusEvent * )
{
    //ctlFrame->setPalette( QColor( 255, 200, 0 ) );
    ctlFrame->setPalette( QColor( 0, 0, 0 ) );
    ctlFrame->setAutoFillBackground(true);
}

void Main_Widget::processorLoad()
{
	char tmp[20];
	double loadavg[3];

	getloadavg ( loadavg, sizeof ( loadavg ) / sizeof ( loadavg[0] ) );
	snprintf ( tmp, 20, "CPU: %5.2f\n", loadavg[0] );
	CPU_label->setText ( tmp );
}

void Main_Widget::toggle_NR ( int )
{
	set_NR ( !NR_state );
}

void Main_Widget::set_NR ( int state )
{
	NR_state = state;
    if ( NR_state ) NR_label->setPalette( QColor( 0, 100, 200 ) );
    else NR_label->setPalette( QColor( 0, 0, 0 ) );
    NR_label->setAutoFillBackground( true );

	pCmd->sendCommand ("setNR %d\n", state );
	fprintf ( stderr, "setNR %d\n", state );

}

void Main_Widget::toggle_ANF ( int )
{
	set_ANF ( !ANF_state );
}

void Main_Widget::set_ANF ( int state )
{
	ANF_state = state;
    if ( ANF_state ) ANF_label->setPalette( QColor( 0, 100, 200 ) );
    else ANF_label->setPalette( QColor( 0, 0, 0 ) );
    ANF_label->setAutoFillBackground( true );

	pCmd->sendCommand ("setANF %d\n", ANF_state );
	fprintf ( stderr, "setANF %d\n", ANF_state );
}

void Main_Widget::toggle_NB ( int )
{
	set_NB ( !NB_state );
}

void Main_Widget::set_NB ( int state )
{
	NB_state = state;
    if ( NB_state ) NB_label->setPalette( QColor( 0, 100, 200 ) );
    else NB_label->setPalette( QColor( 0, 0, 0 ) );
    NB_label->setAutoFillBackground( true );

	pCmd->sendCommand ("setNB %d\n", NB_state );
	fprintf ( stderr, "setNB %d\n", NB_state );
}

void Main_Widget::toggle_BIN ( int )
{
	set_BIN ( !BIN_state );
}

void Main_Widget::set_BIN ( int state )
{
	BIN_state = state;
    if ( BIN_state ) BIN_label->setPalette( QColor( 200, 0, 0 ) );
    else BIN_label->setPalette( QColor( 0, 0, 0 ) );
    BIN_label->setAutoFillBackground( true );

	pCmd->sendCommand ("setBIN %d\n", BIN_state );
	fprintf ( stderr, "setBIN %d\n", BIN_state );
}

void Main_Widget::toggle_MUTE ( int )
{
	set_MUTE ( !MUTE_state );
}

void Main_Widget::set_MUTE ( int state )
{
	MUTE_state = state;
    if ( MUTE_state ) MUTE_label->setPalette( QColor( 200, 0, 0 ) );
    else MUTE_label->setPalette( QColor( 0, 0, 0 ) );
    MUTE_label->setAutoFillBackground( true );

	pCmd->sendCommand ("setRunState %d\n", MUTE_state ? 0: 2 );
	fprintf ( stderr, "setRunState %d\n", MUTE_state ? 0 : 2 );
}

void Main_Widget::toggle_SPEC ( int )
{
	set_SPEC ( !SPEC_state );
}

void Main_Widget::set_SPEC ( int state )
{
	SPEC_state = state;
    if ( SPEC_state ) SPEC_label->setPalette( QColor( 0, 150, 50 ) );
    else SPEC_label->setPalette( QColor( 0, 0, 0 ) );
    
    SPEC_label->setAutoFillBackground( true );
}

void Main_Widget::leave_band( int band )
{
	fprintf( stderr, "leave band %d\n", band );
	writeMem( band_cell[band-1] );
}

void Main_Widget::enter_band(int band)
{
	fprintf( stderr, "enter band %d\n", band );
	readMem( band_cell[band-1] );

	if (sdr_band != NULL) {
		QString cmd = sdr_band;
		QString b;
		b.sprintf ("%d", band);
		cmd += " ";
		b += " ";
		cmd += b;
		b.sprintf ("%lld", rx_f - rx_delta_f);
		cmd += " ";
		cmd += b;
		fprintf( stderr, "Set band: %s\n", qPrintable(cmd));
		system(qPrintable(cmd));
	}
}

void Main_Widget::band_UP ( int )
{
	fprintf( stderr, "Band UP\n");
	leave_band( band );
	band++;
	if (band > 10)
		band = 1;
	enter_band( band );
}

void Main_Widget::band_DOWN ( int )
{
	fprintf( stderr, "Band DOWN\n");
	leave_band( band );
	band--;
	if (band < 1)
		band = 10;
	enter_band( band );
}

void Main_Widget::setCfg ( int )
{
	cfgFrame->show();
}

void Main_Widget::toggle_TX ( int )
{
	fprintf( stderr, "Toggle TX\n");
	if (enableTransmit)
	{
		if (transmit) {
			transmit = 0;
			pTXCmd->sendCommand ("setTRX 0\n");
			pTXCmd->sendCommand ("setRunState 0\n");
			pUSBCmd->sendCommand("set ptt off\n" );
			fprintf (stderr, "set ptt off\n");
			TRX_label->setPixmap( QPixmap( rx_xpm ) );
			TRX_label->setLabel( RX );
			set_MUTE ( 0 );
		} else {
			// if enableSPLIT   set USBSoftrock frequency
			transmit = 1;
			pUSBCmd->sendCommand ("set ptt on\n" );
			pTXCmd->sendCommand ("setRunState 2\n");
			pTXCmd->sendCommand ("setTRX 1\n");
			set_MUTE ( 1 );
			fprintf (stderr, "set ptt on\n");
			TRX_label->setPixmap( QPixmap( tx_xpm ) );
			TRX_label->setLabel( TX );
		}
	} else {
		fprintf( stderr, "Transmit is not enabled\n");
	}
}

void Main_Widget::set_RIT ( int state )
{
	enableRIT = state;
	if ( enableRIT ) {
    	RIT_label->setPalette(QColor(0, 0, 200) );
		set_SPLIT( 0 );
		tx_f = rx_f;
		tx_delta_f = rx_delta_f;
	} else {
    	RIT_label->setPalette(QColor(0, 0, 0) );
		rit->setText( "" );
	}
}

void Main_Widget::toggle_RIT ( int )
{
	set_RIT(!enableRIT);
}

void Main_Widget::set_SPLIT ( int state )
{
	enableSPLIT = state;
	if ( enableSPLIT ) {
    	SPLIT_label->setPalette(QColor(0, 0, 200) );
		set_RIT( 0 );
		tx_f_string.sprintf ("%11.6lf", 
			( double ) ( tx_f - tx_delta_f ) / 1000000.0 );
		fprintf(stderr, "set_SPLIT %s\n", qPrintable(tx_f_string));
		rit->setText( tx_f_string );
	} else {
    	SPLIT_label->setPalette(QColor(0, 0, 0) );
		rit->setText( "" );
	}
}

void Main_Widget::toggle_SPLIT ( int )
{
	tx_f = rx_f;
	tx_delta_f = rx_delta_f;
	set_SPLIT(!enableSPLIT);
}


void Main_Widget::readMem ( MemoryCell *m )
{
	printf("readMem %d = %lld %lld\n", m->getID(), m->getFrequency(), m->getTxFrequency());
	setMode ( ( rmode_t ) m->getMode(), FALSE, FALSE );
	if ( rock_bound ) {
		rx_delta_f = m->getFrequency();
		if ((rx_delta_f > spec_width) || (rx_delta_f < -spec_width)) {
			fprintf(stderr, "Error: frequency outside spectrum (%d <-> %d).  Reset it to 0\n",
				rx_delta_f, spec_width);
			rx_delta_f = 0;
		}
		setRxFrequency( 0 );
	} else {
		rx_delta_f = tuneCenter;
		rx_f = m->getFrequency();
		rx_f += rx_delta_f;
		fprintf (stderr, "set freq %f\n", (rx_f)*1e-6 ); 
		pUSBCmd->sendCommand("set freq %f\n", (rx_f)*1e-6);
		tx_f = m->getTxFrequency();
		set_RIT( 0 );	// do this before set_SPLIT, as it clears the RIT text
		if (tx_f != 0) {
			tx_delta_f = tuneCenter;
			tx_f += tx_delta_f;
			set_SPLIT ( TRUE );
		} else {
			set_SPLIT ( FALSE );
		}
		setRxFrequency( 1 );
	}
	*filter_l = m->getFilter_l();
	*filter_h = m->getFilter_h();
	setFilter();
}

void Main_Widget::writeMem ( MemoryCell *m )
{
	printf("writeMem %d = (%lld - %d) %lld\n", m->getID(), rx_f, rx_delta_f,
		rx_f - rx_delta_f);
	if (rock_bound) {
		m->setFrequency ( rx_delta_f );
	} else {
		m->setFrequency ( rx_f - rx_delta_f );
		if (enableSPLIT) {
			m->setTxFrequency ( tx_f - tx_delta_f );
		} else {
			m->setTxFrequency ( 0 );
		}
	}
	m->setMode ( mode );
	m->setFilter ( *filter_l, *filter_h );
}

void Main_Widget::displayMem ( MemoryCell *m )
{
	char temp[32];
	snprintf ( temp, 32, "%lf", ( double ) ( m->getFrequency() ) / 1000000.0 );
	M_label->setText ( temp );
	printf("displayMem %d, %lld\n", m->getID(), m->getFrequency());
}

void Main_Widget::displayNCO ( int x )
{
	int pb_f;
	char temp[32];
	float bin_bw = sample_rate/(float)spec_width;
	//pb_f = ( int ) ( ( ( x - ( spectrogram->width() / 2 ) ) * bin_bw ) ) /10*10;
	pb_f = (int)(((x - (spectrogram->width() / 2)) * bin_bw) * hScale) / 10*10;  

	snprintf ( temp, 32, "%d", pb_f );
	M_label->setText ( temp );
}

void Main_Widget::updateCallsign()
{
	stationCallsign = cfgCallInput->text();
	setWindowTitle("SDR-Shell 4.15 @ " + stationCallsign );
}

void Main_Widget::updateLOFreq()
{
	rx_f = cfgLOFreqInput->text().toLongLong();
	rx_f_string = cfgLOFreqInput->text();
	if ( useIF ) setDefaultRxFrequency();
	else setRxFrequency( 1 );
}

void Main_Widget::updateTuneOffset()
{
	//fprintf(stderr, "updateTuneOffset = %s\n", qPrintable(cfgTuneOffsetInput->text()));
	tuneCenter = cfgTuneOffsetInput->text().toInt();
	fprintf(stderr, "updateTuneOffset = %d\n", tuneCenter);
	rx_f -= rx_delta_f;
	rx_delta_f =  tuneCenter;	//Make it not tuned to the center
	rx_f += rx_delta_f;
	setRxFrequency( 1 );
}

void Main_Widget::updateIFreq()
{
	rx_if = cfgIFreqInput->text().toLongLong();
	rx_if_string = cfgIFreqInput->text();
	setDefaultRxFrequency();
}
void Main_Widget::updateHamlib()
{
	portString = cfgHamlibPortInput->text();
	speedString = cfgHamlibSpeedInput->text();
	speed = speedString.toInt();
	rigString = cfgHamlibRigInput->text();
	rig = ( rig_model_t ) rigString.toInt();
}

void Main_Widget::updateIQGain ( int gain )
{
	iqGain = gain;
	setIQGain();
}

void Main_Widget::updateIQPhase ( int phase )
{
	iqPhase = phase;
	setIQPhase();
}

void Main_Widget::updateTxIQGain ( int gain )
{
	txIQGain = gain;
	setTxIQGain();
}

void Main_Widget::updateTxIQPhase ( int phase )
{
	txIQPhase = phase;
	setTxIQPhase();
}

void Main_Widget::updateTxMicGain ( int gain )
{
	micGain = gain;
	setTxGain( 0 );
}

void Main_Widget::updateTxOutputGain ( int gain )
{
	txGain = gain;
	setTxGain( 1 );
}

void Main_Widget::setPolyFFT ( )
{
	if(polyFFT_button->isChecked())
		polyphaseFFT = 1;
	else
		polyphaseFFT = 0;
        //polyphaseFFT = ~polyphaseFFT;
    pCmd->sendCommand ("setSpectrumPolyphase %d\n", polyphaseFFT );
    fprintf ( stderr, "setSpectrumPolyphase %d\n", polyphaseFFT );
}

void Main_Widget::setFFTWindow ( )
{
//	fftWindow = window;
    if ( fftWindow_0->isChecked() )
       { fftWindow = 0; }
    else 
     if ( fftWindow_1->isChecked() )
        {  fftWindow = 1; }
     else
      if ( fftWindow_2->isChecked() )
         {  fftWindow = 2; }
      else
       if ( fftWindow_3->isChecked() )
          {  fftWindow = 3; }
       else
        if ( fftWindow_4->isChecked() )
           {  fftWindow = 4; }
        else
         if ( fftWindow_5->isChecked() )
            {  fftWindow = 5; }
         else
          if ( fftWindow_6->isChecked() )
             {  fftWindow = 6; }
          else
           if ( fftWindow_7->isChecked() )
              {  fftWindow = 7; }
           else
            if ( fftWindow_8->isChecked() )
               {  fftWindow = 8; }
            else
             if ( fftWindow_9->isChecked() )
                {  fftWindow = 2; }
             else
              if ( fftWindow_10->isChecked() )
                 {  fftWindow = 10; }
              else
               if ( fftWindow_11->isChecked() )
                  {  fftWindow = 11; }
               else
                  {  fftWindow = 12; };
           
	pCmd->sendCommand ("setSpectrumWindow %d\n", fftWindow );
	fprintf ( stderr, "setSpectrumWindow %d\n", fftWindow );
}

void Main_Widget::setSpectrumType()
{
	if ( preFilter_button->isChecked() )
	   { spectrumType = 1; }		// SPEC_PRE_FILT
	else
	   { spectrumType = 2; };		// SPEC_POST_FILT
	pCmd->sendCommand ("setSpectrumType %d\n", spectrumType );
	fprintf ( stderr, "setSpectrumType %d\n", spectrumType );

}

void Main_Widget::setSpectrumDefaults()
{
	fprintf(stderr, "setSpectrumType %d\n", spectrumType );
	fprintf(stderr, "setSpectrumWindow %d\n", fftWindow );
	fprintf(stderr, "setSpectrumPolyphase %d\n", polyphaseFFT );
	pCmd->sendCommand ("setSpectrumType %d\n", spectrumType );
	pCmd->sendCommand ("setSpectrumWindow %d\n", fftWindow );
	pCmd->sendCommand ("setSpectrumPolyphase %d\n", polyphaseFFT );
}    

void Main_Widget::setAGC ( int type )
{
	agcType = type;
	pCmd->sendCommand ("setRXAGC %d\n", type );
	fprintf ( stderr, "setRXAGC %d\n", type );

	QColor on ( 150, 50, 50 );
	QColor off ( 0, 0, 0 );

    //AGC_O_label->setAutoFillBackground(true);
    AGC_L_label->setAutoFillBackground(true);
    AGC_S_label->setAutoFillBackground(true);
    AGC_M_label->setAutoFillBackground(true);
    AGC_F_label->setAutoFillBackground(true);
	switch( type ) {
	case 0:
		//AGC_O_label->setPalette( on ); 
		AGC_L_label->setPalette( off );
		AGC_S_label->setPalette( off );
		AGC_M_label->setPalette( off );
		AGC_F_label->setPalette( off );
		//AGC_O_label->setAutoFillBackground(true);
		break;
	case 1:
		//AGC_O_label->setPalette( off );
		AGC_L_label->setPalette( on ); 		 
		AGC_S_label->setPalette( off );		
		AGC_M_label->setPalette( off );		
		AGC_F_label->setPalette( off );
		
		break;
	case 2:
		//AGC_O_label->setPalette( off );
		AGC_L_label->setPalette( off );
		AGC_S_label->setPalette( on );  
		AGC_M_label->setPalette( off );
		AGC_F_label->setPalette( off );
		break;
	case 3:
		//AGC_O_label->setPalette( off );
		AGC_L_label->setPalette( off );
		AGC_S_label->setPalette( off );
		AGC_M_label->setPalette( on );  
		AGC_F_label->setPalette( off );
		break;
	case 4:
		//AGC_O_label->setPalette( off );
		AGC_L_label->setPalette( off );
		AGC_S_label->setPalette( off );
		AGC_M_label->setPalette( off );
		AGC_F_label->setPalette( on );  
		break;
	default:
		break;
	}
}

void Main_Widget::calibrateSpec ( int value )
{
	specCal = value;
}

void Main_Widget::calibrateMetr ( int value )
{
	metrCal = value;
}

void Main_Widget::setIF ( bool value )
{
fprintf(stderr, "setIF: %d\n", value);
	useIF =  value;
}

void Main_Widget::updateUseUSBsoftrock ( bool value )
{
	rock_bound = !value;
	if (value) {
		rx_f -= rx_delta_f;
		rx_delta_f = tuneCenter;
		rx_f += rx_delta_f;
	}
	fprintf ( stderr, "useUSBsoftrock: %s\n", 
		value ? "enabled" : "disabled");
}

void Main_Widget::updateDualConversion ( bool value )
{
	dualConversion =  value;
	fprintf ( stderr, "DualConversion: %s\n", 
		value ? "enabled" : "disabled");
}

void Main_Widget::updateTransmit ( bool value )
{
	enableTransmit = value;
	if ( enableTransmit ) {
		pTXCmd->on();
		setTxIQGain();
		setTxIQPhase();
		setTxGain( 0 );
		setTxGain( 1 );
		setTxFrequency();
	} else {
		pTXCmd->off();
	}
	fprintf ( stderr, "Transmit: %s\n", 
		enableTransmit ? "enabled" : "disabled");
}

void Main_Widget::setHamlib ( bool value )
{
        if ( value )
        {
            useHamlib = TRUE;
        }
        else
        {
            useHamlib = FALSE;
            if (ourHamlibWrapper) ourHamlibWrapper->~hamlibWrapper();
        }
        emit toggleHamlibButton ( useHamlib );
}

void Main_Widget::setMuteXmit ( bool value )
{
	muteXmit =  value;
	emit tellMuteXmit ( muteXmit );
}


void Main_Widget::initHamlib ()
{
        rig_errcode_e error;
	ourHamlibWrapper = new hamlibWrapper ( this );

	connect ( ourHamlibWrapper, SIGNAL ( nowTransmit ( int ) ), this, SLOT ( set_MUTE ( int ) ) );
	connect ( ourHamlibWrapper, SIGNAL ( newFreq ( double ) ), this, SLOT ( setOurRxFrequency ( double ) ) );
	connect ( ourHamlibWrapper, SIGNAL ( rigChangedMode ( rmode_t, bool ) ), this, SLOT ( setMode ( rmode_t, bool ) ) );
	connect ( ourHamlibWrapper, SIGNAL ( slopeLowChangedByRig ( int ) ), this, SLOT ( setSlopeLowOffset ( int ) ) );
	connect ( ourHamlibWrapper, SIGNAL ( slopeHighChangedByRig ( int ) ), this, SLOT ( setSlopeHighOffset ( int ) ) );
	connect ( ourHamlibWrapper, SIGNAL ( rigPitch ( int ) ), this, SLOT ( setCWPitch ( int ) ) );

       // hl_port = "localhost";  //Debugging fix.
        QByteArray myarray = portString.toAscii();
        char *hl_port = myarray.data();
        if ( (error = (rig_errcode_e)ourHamlibWrapper->init ( rig, hl_port, speed )) != RIG_OK )
	{
                fprintf( stderr, "Hamlib initialization error: %d. \n", error);
                fprintf( stderr, "Hamlib would not initialize.  Fix the hamlib set up and re-enable hamblib via the CFG option. \n");
                setHamlib ( FALSE );
	}
	emit changeSlopeTune ( useSlopeTune );
	emit tellMuteXmit ( muteXmit );
}

void Main_Widget::updateUSBOffset ( int offset )
{
	usbOffset = offset;
}

void Main_Widget::updateLSBOffset ( int offset )
{
	lsbOffset = offset;
}

void Main_Widget::setSlopeTune ( bool useslopetune)
{
	useSlopeTune = useslopetune;
	emit changeSlopeTune ( useSlopeTune );
}

void Main_Widget::updateSlopeLowOffset ( int offset )
{
	slopeLowOffset = offset;
}

void Main_Widget::updateSlopeHighOffset ( int offset )
{
	slopeHighOffset = offset;
}

void Main_Widget::setSlopeLowOffset ( int value )
{
	switch ( mode )
	{
		case RIG_MODE_USB:
			slopeTuneOffset = ( int ) 1.000 * value * slopeLowOffset / SLOPE_TUNE_MAX;
			break;
		case RIG_MODE_CW:
			slopeTuneOffset = ( int ) 1.000 * value * slopeLowOffset / SLOPE_TUNE_MAX_CW;
			break;
		default:	
		fprintf(stderr, "Hit default: value is: %d \n", value );
	}
	setDefaultRxFrequency();
}

void Main_Widget::setSlopeHighOffset ( int value )
{
	switch ( mode )
	{
		case RIG_MODE_FM:
			slopeTuneOffset = 0;
			break;
		case RIG_MODE_AM:
		case RIG_MODE_SAM:
			slopeTuneOffset = 3200 * value  / SLOPE_TUNE_MAX;  // Hardcode this for now.
			break;
		case RIG_MODE_LSB:
			slopeTuneOffset = ( int ) 1.000 * value * slopeHighOffset / SLOPE_TUNE_MAX;
			break;
		case RIG_MODE_CWR:
			slopeTuneOffset = ( int ) 1.000 * value * slopeHighOffset / SLOPE_TUNE_MAX_CW;
			break;
		default:	
			fprintf(stderr, "Hit default: value is: %d \n", value );
	}
	setDefaultRxFrequency();
}

void Main_Widget::setCWPitch ( int pitch )
{
	cwPitch = pitch;
}

