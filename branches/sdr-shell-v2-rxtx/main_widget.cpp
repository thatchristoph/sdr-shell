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

Main_Widget::Main_Widget ( QWidget *parent, const char *name )
		: QWidget ( parent, name )
{
	setFocusPolicy ( QWidget::TabFocus );
	setPaletteBackgroundColor ( QColor ( 0, 0, 0 ) );
	setMinimumWidth ( 650 );
	setMinimumHeight ( 300 );

	initConstants();
	slopeTuneOffset = 0;
	loadSettings();

	setCaption ( "SDR-Shell rxtx.5 @ " + stationCallsign );

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

	/*QFont font2 ( "Bitstream Vera Sans", 10, FALSE );
	QFontMetrics fm2 ( font2 ); */
	QColor borderColor ( 255, 200, 55 );

	// -----------------------------------------------------------------------
	// Spectrogram

	spectrogramFrame = new QFrame ( this );
	spectrogramFrame->setFrameStyle ( QFrame::StyledPanel | QFrame::Plain );
	spectrogramFrame->setPaletteForegroundColor ( borderColor );

	spectrogram = new Spectrogram ( spectrogramFrame );
	spectrogram->setPaletteBackgroundColor ( QColor ( 20, 20, 50 ) );
	connect ( spectrogram, SIGNAL ( tune1 ( int ) ),
	          this, SLOT ( spectrogramClicked ( int ) ) );
	connect ( spectrogram, SIGNAL ( tune2 ( int ) ),
	          this, SLOT ( tune ( int ) ) );
	connect ( spectrogram, SIGNAL ( plot ( int ) ),
	          this, SLOT ( plotSpectrum ( int ) ) );
	connect ( spectrogram, SIGNAL ( movement ( int ) ),
	          this, SLOT ( f_at_mousepointer ( int ) ) );

	// -----------------------------------------------------------------------
	// Pass Band Filter Scale
	pbscale = new PassBandScale ( spectrogramFrame,
	                              "pbscale", Qt::WResizeNoErase );
	pbscale->setPaletteBackgroundColor ( QColor ( 100, 0, 0 ) );
	connect ( pbscale, SIGNAL ( set_lower_pb ( int ) ),
	          this, SLOT ( setLowerFilterScale ( int ) ) );
	connect ( pbscale, SIGNAL ( set_upper_pb ( int ) ),
	          this, SLOT ( setUpperFilterScale ( int ) ) );
	connect ( pbscale, SIGNAL ( movement ( int ) ),
	          this, SLOT ( displayNCO ( int ) ) );

	// -----------------------------------------------------------------------
	// Spectrum
	spectrumFrame = new Spectrum ( spectrogramFrame, "SpectrumFrame",
	                               Qt::WResizeNoErase );
	spectrumFrame->setPaletteBackgroundColor ( QColor ( 0, 0, 0 ) );
	connect ( spectrumFrame, SIGNAL ( movement ( int ) ),
	          this, SLOT ( f_at_mousepointer ( int ) ) );
	connect ( spectrumFrame, SIGNAL ( tune ( int ) ),
	          this, SLOT ( spectrogramClicked ( int ) ) );
	connect ( spectrumFrame, SIGNAL ( tune2 ( int ) ),
	          this, SLOT ( tune ( int ) ) );

	// -----------------------------------------------------------------------
	// Control Frame

	// Top
	ctlFrame = new QFrame ( this );
	ctlFrame->setFrameStyle ( QFrame::StyledPanel | QFrame::Plain );
	ctlFrame->setPaletteForegroundColor ( borderColor );
	ctlFrame->setBackgroundColor ( QColor ( 0, 15, 150 ) );

	// Top Filler
	QFrame *ctlFrame1 = new QFrame ( ctlFrame );
	ctlFrame1->setPaletteForegroundColor ( QColor ( 255, 255, 255 ) );
	ctlFrame1->setPaletteBackgroundColor ( QColor ( 0, 0, 0 ) );

	// Bottom
	ctlFrame2 = new QFrame ( this );
	ctlFrame2->setFrameStyle ( QFrame::StyledPanel | QFrame::Plain );
	ctlFrame2->setPaletteForegroundColor ( borderColor );
	ctlFrame2->setPaletteBackgroundColor ( QColor ( 255, 200, 0 ) );

	// -----------------------------------------------------------------------
	// Configuration Frame

	cfgFrame = new QFrame();
	cfgFrame->setGeometry ( 50, 50, 380, 400 );
	cfgFrame->setMinimumWidth ( 380 );
	cfgFrame->setMaximumWidth ( 380 );
	cfgFrame->setMinimumHeight ( 500 );
	cfgFrame->setMaximumHeight ( 500 );
	cfgFrame->setCaption ( "SDR-Shell : Config" );

	QFrame *cfgFrame1 = new QFrame();
	QFrame *cfgFrame2 = new QFrame();
	QFrame *cfgFrame3 = new QFrame();
	QFrame *cfgFrame6 = new QFrame();
	QFrame *cfgFrame7 = new QFrame();
	QFrame *cfgFrame4 = new QFrame();
	QFrame *cfgFrame5 = new QFrame();

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
	cfgCallBox->setGeometry ( 5, 5, 340, 45 );

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
	cfgLOFreqBox->setGeometry ( 5, 53, 340, 45 );

	cfgLOFreqInput = new QLineEdit ( cfgLOFreqBox );
	cfgLOFreqInput->setGeometry ( 10, 18, 120, 20 );
	cfgLOFreqInput->setText ( rx_f_string );


	QPushButton *updateLOFreqButton = new QPushButton ( cfgLOFreqBox );
	updateLOFreqButton->setText ( "Update" );
	updateLOFreqButton->setGeometry ( 140, 18, 70, 20 );
	connect ( updateLOFreqButton, SIGNAL ( pressed() ),
	          this, SLOT ( updateLOFreq() ) );

	// Use USBSoftrock
	QGroupBox *cfgUSBBox = new QGroupBox ( cfgFrame1 );
	cfgUSBBox->setTitle ( "USBSoftrock Control" );
	cfgUSBBox->setGeometry ( 5, 101, 340, 65 );

	QRadioButton *cfgUseUSBsoftrock = new QRadioButton ( cfgUSBBox );
	cfgUseUSBsoftrock->setText ( "Use usbsoftrock via UDP" );
	cfgUseUSBsoftrock->setGeometry ( 25, 18, 200, 20 );
	connect ( cfgUseUSBsoftrock, SIGNAL ( toggled ( bool ) ),
	          this, SLOT ( updateUseUSBsoftrock ( bool ) ) );
	cfgUseUSBsoftrock->setChecked ( !rock_bound );

	// USBSoftrock 5/4 tuning for dual-conversion
	QRadioButton *cfgDualConversion = new QRadioButton ( cfgUSBBox );
	cfgDualConversion->setText ( "5/4 Tuning" );
	cfgDualConversion->setGeometry ( 25, 36, 200, 20 );
	connect ( cfgDualConversion, SIGNAL ( toggled ( bool ) ),
	          this, SLOT ( updateDualConversion ( bool ) ) );
	cfgDualConversion->setChecked ( dualConversion );

	// Spectrum and S-Meter calibration
	QGroupBox *calibrationBox = new QGroupBox ( cfgFrame1 );
	calibrationBox->setTitle ( "Calibration" );
	calibrationBox->setGeometry ( 5, 175, 340, 45 );

	QLabel *specCalLabel = new QLabel ( calibrationBox );
	specCalLabel->setText ( "Spectrum: " );
	specCalLabel->setGeometry ( 10, 18, 70, 20 );
	specCalLabel->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );

	specCalSpinBox = new QSpinBox ( calibrationBox );
	specCalSpinBox->setGeometry ( 80, 18, 70, 20 );
	specCalSpinBox->setMinValue ( 0 );
	specCalSpinBox->setMaxValue ( 100 );
	specCalSpinBox->setValue ( ( int ) specCal );
	connect ( specCalSpinBox, SIGNAL ( valueChanged ( int ) ),
	          this, SLOT ( calibrateSpec ( int ) ) );

	QLabel *metrCalLabel = new QLabel ( calibrationBox );
	metrCalLabel->setText ( "S-Meter: " );
	metrCalLabel->setGeometry ( 175, 18, 70, 20 );
	metrCalLabel->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );

	metrCalSpinBox = new QSpinBox ( calibrationBox );
	metrCalSpinBox->setGeometry ( 245, 18, 70, 20 );
	metrCalSpinBox->setMinValue ( 0 );
	metrCalSpinBox->setMaxValue ( 100 );
	metrCalSpinBox->setValue ( ( int ) metrCal );
	connect ( metrCalSpinBox, SIGNAL ( valueChanged ( int ) ),
	          this, SLOT ( calibrateMetr ( int ) ) );

	// IQ Phase Calibration
	QGroupBox *cfgIQCal = new QGroupBox ( cfgFrame2 );
	cfgIQCal->setTitle ( "RX IQ Calibration" );
	cfgIQCal->setGeometry ( 5, 5, 340, 45 );

	QLabel *cfgIQPhaseLabel = new QLabel ( cfgIQCal );
	cfgIQPhaseLabel->setText ( "IQ Phase: " );
	cfgIQPhaseLabel->setGeometry ( 10, 18, 70, 20 );
	cfgIQPhaseLabel->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );

	cfgIQPhaseInput = new QSpinBox ( cfgIQCal );
	cfgIQPhaseInput->setGeometry ( 80, 18, 70, 20 );
	cfgIQPhaseInput->setMinValue ( -1000 );
	cfgIQPhaseInput->setMaxValue ( 1000 );
	cfgIQPhaseInput->setValue ( iqPhase );
	connect ( cfgIQPhaseInput, SIGNAL ( valueChanged ( int ) ),
	          this, SLOT ( updateIQPhase ( int ) ) );

	// IQ Gain Calibration
	QLabel *cfgIQGainLabel = new QLabel ( cfgIQCal );
	cfgIQGainLabel->setText ( "IQ Gain: " );
	cfgIQGainLabel->setGeometry ( 175, 18, 70, 20 );
	cfgIQGainLabel->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );

	cfgIQGainInput = new QSpinBox ( cfgIQCal );
	cfgIQGainInput->setGeometry ( 245, 18, 70, 20 );
	cfgIQGainInput->setMinValue ( -1000 );
	cfgIQGainInput->setMaxValue ( 1000 );
	cfgIQGainInput->setValue ( iqGain );
	connect ( cfgIQGainInput, SIGNAL ( valueChanged ( int ) ),
	          this, SLOT ( updateIQGain ( int ) ) );

	// Transmit IQgain, IQphase
	QGroupBox *cfgTxIQCal = new QGroupBox ( cfgFrame2 );
	cfgTxIQCal->setTitle ( "TX IQ Calibration" );
	cfgTxIQCal->setGeometry ( 5, 50, 340, 45 );

	QLabel *cfgTxIQPhaseLabel = new QLabel ( cfgTxIQCal );
	cfgTxIQPhaseLabel->setText ( "IQ Phase: " );
	cfgTxIQPhaseLabel->setGeometry ( 10, 18, 70, 20 );
	cfgTxIQPhaseLabel->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );

	cfgTxIQPhaseInput = new QSpinBox ( cfgTxIQCal );
	cfgTxIQPhaseInput->setGeometry ( 80, 18, 70, 20 );
	cfgTxIQPhaseInput->setMinValue ( -1000 );
	cfgTxIQPhaseInput->setMaxValue ( 1000 );
	cfgTxIQPhaseInput->setValue ( txIQPhase );
	connect ( cfgTxIQPhaseInput, SIGNAL ( valueChanged ( int ) ),
	          this, SLOT ( updateTxIQPhase ( int ) ) );

	// IQ Gain Calibration
	QLabel *cfgTxIQGainLabel = new QLabel ( cfgTxIQCal );
	cfgTxIQGainLabel->setText ( "IQ Gain: " );
	cfgTxIQGainLabel->setGeometry ( 175, 18, 70, 20 );
	cfgTxIQGainLabel->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );

	cfgTxIQGainInput = new QSpinBox ( cfgTxIQCal );
	cfgTxIQGainInput->setGeometry ( 245, 18, 70, 20 );
	cfgTxIQGainInput->setMinValue ( -1000 );
	cfgTxIQGainInput->setMaxValue ( 1000 );
	cfgTxIQGainInput->setValue ( txIQGain );
	connect ( cfgTxIQGainInput, SIGNAL ( valueChanged ( int ) ),
	          this, SLOT ( updateTxIQGain ( int ) ) );

	// Polyphase FFT
	QButtonGroup *cfgPolyFFTGroup = new QButtonGroup ( cfgFrame3 );
	cfgPolyFFTGroup->setTitle ( "Polyphase FFT" );
	cfgPolyFFTGroup->setGeometry ( 5, 5, 340, 45 );
	cfgPolyFFTGroup->setRadioButtonExclusive ( true );
	connect ( cfgPolyFFTGroup, SIGNAL ( clicked ( int ) ),
	          this, SLOT ( setPolyFFT ( int ) ) );

	QRadioButton *polyFFT_off_button = new QRadioButton ( cfgPolyFFTGroup );
	polyFFT_off_button->setText ( "Off" );
	polyFFT_off_button->setGeometry ( 10, 18, 70, 20 );

	QRadioButton *polyFFT_on_button = new QRadioButton ( cfgPolyFFTGroup );
	polyFFT_on_button->setText ( "On" );
	polyFFT_on_button->setGeometry ( 160, 18, 70, 19 );

	cfgPolyFFTGroup->setButton ( polyphaseFFT );

	// Spectrum Type
	QButtonGroup *cfgSpecTypeGroup = new QButtonGroup ( cfgFrame3 );
	cfgSpecTypeGroup->setTitle ( "Spectrum Type" );
	cfgSpecTypeGroup->setGeometry ( 5, 53, 340, 45 );
	cfgSpecTypeGroup->setRadioButtonExclusive ( true );
	connect ( cfgSpecTypeGroup, SIGNAL ( clicked ( int ) ),
	          this, SLOT ( setSpectrumType ( int ) ) );

	QRadioButton *preFilter_button = new QRadioButton ( cfgSpecTypeGroup );
	preFilter_button->setText ( "Pre Filter" );
	preFilter_button->setGeometry ( 10, 18, 100, 20 );

	QRadioButton *postFilter_button = new QRadioButton ( cfgSpecTypeGroup );
	postFilter_button->setText ( "Post Filter" );
	postFilter_button->setGeometry ( 160, 18, 100, 20 );

	cfgSpecTypeGroup->setButton ( spectrumType );

	// FFT Windowing Function
	QButtonGroup *cfgFFTWindowGroup = new QButtonGroup ( cfgFrame3 );
	cfgFFTWindowGroup->setTitle ( "FFT Windowing Function" );
	cfgFFTWindowGroup->setGeometry ( 5, 101, 340, 162 );
	cfgFFTWindowGroup->setRadioButtonExclusive ( true );
	connect ( cfgFFTWindowGroup, SIGNAL ( clicked ( int ) ),
	          this, SLOT ( setFFTWindow ( int ) ) );

	QRadioButton *fftWindow_0 = new QRadioButton ( cfgFFTWindowGroup );
	fftWindow_0->setText ( "Rectangular" );
	fftWindow_0->setGeometry ( 10, 18, 140, 19 );

	QRadioButton *fftWindow_1 = new QRadioButton ( cfgFFTWindowGroup );
	fftWindow_1->setText ( "Hanning" );
	fftWindow_1->setGeometry ( 10, 38, 140, 19 );

	QRadioButton *fftWindow_2 = new QRadioButton ( cfgFFTWindowGroup );
	fftWindow_2->setText ( "Welch" );
	fftWindow_2->setGeometry ( 10, 58, 140, 19 );

	QRadioButton *fftWindow_3 = new QRadioButton ( cfgFFTWindowGroup );
	fftWindow_3->setText ( "Parzen" );
	fftWindow_3->setGeometry ( 10, 78, 140, 19 );

	QRadioButton *fftWindow_4 = new QRadioButton ( cfgFFTWindowGroup );
	fftWindow_4->setText ( "Bartlett" );
	fftWindow_4->setGeometry ( 10, 98, 140, 19 );

	QRadioButton *fftWindow_5 = new QRadioButton ( cfgFFTWindowGroup );
	fftWindow_5->setText ( "Hamming" );
	fftWindow_5->setGeometry ( 10, 118, 140, 19 );

	QRadioButton *fftWindow_6 = new QRadioButton ( cfgFFTWindowGroup );
	fftWindow_6->setText ( "Blackman 2" );
	fftWindow_6->setGeometry ( 10, 138, 140, 19 );

	QRadioButton *fftWindow_7 = new QRadioButton ( cfgFFTWindowGroup );
	fftWindow_7->setText ( "Blackman 3" );
	fftWindow_7->setGeometry ( 160, 18, 140, 19 );

	QRadioButton *fftWindow_8 = new QRadioButton ( cfgFFTWindowGroup );
	fftWindow_8->setText ( "Blackman 4" );
	fftWindow_8->setGeometry ( 160, 38, 140, 19 );

	QRadioButton *fftWindow_9 = new QRadioButton ( cfgFFTWindowGroup );
	fftWindow_9->setText ( "Exponential" );
	fftWindow_9->setGeometry ( 160, 58, 140, 19 );

	QRadioButton *fftWindow_10 = new QRadioButton ( cfgFFTWindowGroup );
	fftWindow_10->setText ( "Riemann" );
	fftWindow_10->setGeometry ( 160, 78, 140, 19 );

	QRadioButton *fftWindow_11 = new QRadioButton ( cfgFFTWindowGroup );
	fftWindow_11->setText ( "Blackman-Harris" );
	fftWindow_11->setGeometry ( 160, 98, 140, 19 );

	QRadioButton *fftWindow_12 = new QRadioButton ( cfgFFTWindowGroup );
	fftWindow_12->setText ( "Nuttall" );
	fftWindow_12->setGeometry ( 160, 118, 140, 19 );

	cfgFFTWindowGroup->setButton ( fftWindow );

	// IF Settings
	QRadioButton *cfgIFAligned = new QRadioButton ( cfgFrame6 );
	cfgIFAligned->setText ( "Use IF Mode" );
	cfgIFAligned->setGeometry ( 25, 155, 340, 45 );
	connect ( cfgIFAligned, SIGNAL ( toggled ( bool ) ),
	          this, SLOT ( setIF ( bool ) ) );
	cfgIFAligned->setChecked ( useIF );

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
	cfgSlopeLowOffsetInput->setMinValue ( 0 );
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
	cfgSlopeHighOffsetInput->setMinValue ( 0 );
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
	cfgUSBOffsetInput->setMinValue ( 0 );
	cfgUSBOffsetInput->setMaxValue ( 9999 );
	cfgUSBOffsetInput->setValue ( usbOffset );
	connect ( cfgUSBOffsetInput, SIGNAL ( valueChanged ( int ) ),
	          this, SLOT ( updateUSBOffset ( int ) ) );

	QLabel *cfgLSBOffsetLabel = new QLabel ( cfgIFreqBox );
	cfgLSBOffsetLabel->setText ( "LSB Offset" );
	cfgLSBOffsetLabel->setGeometry ( 10, 83, 70, 20 );
	cfgLSBOffsetLabel->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );

	cfgLSBOffsetInput = new QSpinBox ( cfgIFreqBox );
	cfgLSBOffsetInput->setGeometry ( 90, 83, 70, 20 );
	cfgLSBOffsetInput->setMinValue ( 0 );
	cfgLSBOffsetInput->setMaxValue ( 9999 );
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
	cfgUseHamlib->setGeometry ( 5, 100, 150, 45 );
	cfgUseHamlib->setChecked ( useHamlib );
	connect ( cfgUseHamlib, SIGNAL ( toggled ( bool ) ),
	          this, SLOT ( setHamlib ( bool ) ) );
	connect ( this, SIGNAL ( toggleHamlibButton ( bool ) ), 
		  cfgUseHamlib, SLOT ( setChecked ( bool ))); 
	
	QRadioButton *cfgMuteXmitHamlib = new QRadioButton ( cfgHamlibBox );
	cfgMuteXmitHamlib->setText ( "Mute in Xmit" );
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
	cfgTxMicGainInput->setMinValue ( -255 );
	cfgTxMicGainInput->setMaxValue ( 255 );
	cfgTxMicGainInput->setValue ( micGain );
	connect ( cfgTxMicGainInput, SIGNAL ( valueChanged ( int ) ),
	          this, SLOT ( updateTxMicGain ( int ) ) );

	QLabel *cfgTxOutputGainLabel = new QLabel ( cfgTxGain );
	cfgTxOutputGainLabel->setText ( "Out Gain: " );
	cfgTxOutputGainLabel->setGeometry ( 175, 18, 70, 20 );
	cfgTxOutputGainLabel->setAlignment ( Qt::AlignRight | Qt::AlignVCenter );

	cfgTxOutputGainInput = new QSpinBox ( cfgTxGain );
	cfgTxOutputGainInput->setGeometry ( 245, 18, 70, 20 );
	cfgTxOutputGainInput->setMinValue ( -255 );
	cfgTxOutputGainInput->setMaxValue ( 255 );
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
	aboutText->setReadOnly ( true );
	aboutText->append (
	    QString ( "<center><br>SDR-Shell Version rxtx.4<br>" ) +
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
	signalFrame->setPaletteBackgroundPixmap ( *meter1_pix );

	for ( int i = 0; i < 34; i++ )
	{
		signalBargraph[i] = new QFrame ( signalFrame );
		signalBargraph[i]->setPaletteBackgroundColor ( QColor ( 50, 50, 50 ) );
		signalBargraph[i]->setGeometry ( 3 + 4 * i, 3, 3, 9 );
		signalBargraph[i]->show();
	}

	signal_dBm = new QLabel ( this, "-174", signalFrame );
	signal_dBm->setFont ( *font1 );
	signal_dBm->setPaletteForegroundColor ( QColor ( 255, 255, 255 ) );
	signal_dBm->setPaletteBackgroundColor ( QColor ( 0, 0, 0 ) );

	QLabel *dBmLabel = new QLabel ( this, " dBm", signalFrame );
	dBmLabel->setFont ( *font1 );
	dBmLabel->setPaletteForegroundColor ( QColor ( 255, 255, 255 ) );
	dBmLabel->setPaletteBackgroundColor ( QColor ( 0, 0, 0 ) );

	/*lcd = new QLCDNumber ( 12, ctlFrame );
	lcd->setFrameStyle ( QFrame::NoFrame );
	lcd->setSegmentStyle ( QLCDNumber::Filled );
	lcd->setPaletteForegroundColor ( QColor ( 255, 255, 255 ) );
	lcd->setPaletteBackgroundColor ( QColor ( 0, 0, 0 ) );*/
	
	lcd = new QLabel ( this, "", ctlFrame );
	lcd->setFont ( *fontlcd);
	lcd->setPaletteForegroundColor ( QColor (255, 255, 255 ) );
	lcd->setPaletteBackgroundColor  ( QColor (0, 0, 0 ) );
	lcd->setFrameStyle ( QFrame::NoFrame );

	step_1Hz_frame = new QFrame ( ctlFrame );
	step_1Hz_frame->setGeometry ( 318, 29, 11, 1 );

	step_10Hz_frame = new QFrame ( ctlFrame );
	step_10Hz_frame->setGeometry ( 305, 29, 11, 1 );

	step_100Hz_frame = new QFrame ( ctlFrame );
	step_100Hz_frame->setGeometry ( 292, 29, 11, 1 );

	step_1000Hz_frame = new QFrame ( ctlFrame );
	step_1000Hz_frame->setGeometry ( 279, 29, 11, 1 );
	
	step_10000Hz_frame = new QFrame ( ctlFrame );
	step_10000Hz_frame->setGeometry ( 266, 29, 11, 1 );
	
	step_100000Hz_frame = new QFrame ( ctlFrame );
	step_100000Hz_frame->setGeometry ( 253, 29, 11, 1 );
	
	step_1000000Hz_frame = new QFrame ( ctlFrame );
	step_1000000Hz_frame->setGeometry ( 234, 29, 11, 1 );
	
	step_10000000Hz_frame = new QFrame ( ctlFrame );
	step_10000000Hz_frame->setGeometry ( 221, 29, 11, 1 );
	
	step_100000000Hz_frame = new QFrame ( ctlFrame );
	step_100000000Hz_frame->setGeometry ( 208, 29, 11, 1 );

	rxPix = new QPixmap ( rx_xpm );
	txPix = new QPixmap ( tx_xpm );

	trxFrame = new QFrame ( ctlFrame );
	trxFrame->setPaletteBackgroundPixmap ( *rxPix );
	TRX_label = new Varilabel ( trxFrame );
	TRX_label->setGeometry ( 0, 0, 19, 11 );
	connect ( TRX_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( toggle_TX ( int ) ) );

	QPixmap *mhz_pix = new QPixmap ( mhz_xpm );
	QFrame *mhzFrame = new QFrame ( ctlFrame );
	mhzFrame->setPaletteBackgroundPixmap ( *mhz_pix );
	signal_dBm->setGeometry ( 140, 2, 35, 12 );

	// -----------------------------------------------------------------------
	// Mode Frame

	QFrame *modeFrame = new QFrame ( ctlFrame );
	modeFrame->setBackgroundColor ( QColor ( 100, 0, 0 ) );

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

	QFrame *swFrame = new QFrame ( ctlFrame );
	swFrame->setBackgroundColor ( QColor ( 0, 80, 0 ) );

	QPixmap nr_pix ( nr_xpm );
	NR_label = new Varilabel ( swFrame );
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
	MUTE_label->setBackgroundColor ( QColor ( 0, 0, 0 ) );
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
	QFrame *bFrame = new QFrame ( ctlFrame );
	bFrame->setBackgroundColor ( QColor ( 0, 80, 0 ) );

	QPixmap up_pix ( up_xpm );
	UP_label = new Varilabel ( bFrame );
	UP_label->setPixmap ( up_pix );
	UP_label->setGeometry ( 3, 3, 27, 11 );
	connect ( UP_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( band_UP ( int ) ) );

	//QPixmap tx_pix ( tx_xpm );
	//TX_label = new Varilabel ( bFrame );
	//TX_label->setPixmap ( tx_pix );
	//TX_label->setGeometry ( 33, 3, 27, 11 );
	//connect ( TX_label, SIGNAL ( mouseRelease ( int ) ),
	//          this, SLOT ( toggle_TX ( int ) ) );

	QPixmap down_pix ( down_xpm );
	DOWN_label = new Varilabel ( bFrame );
	DOWN_label->setPixmap ( down_pix );
	DOWN_label->setGeometry ( 3, 17, 27, 11 );
	connect ( DOWN_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( band_DOWN ( int ) ) );

	// -----------------------------------------------------------------------
	// Sub Mode Frame
	QFrame *subFrame = new QFrame ( ctlFrame );
	subFrame->setBackgroundColor ( QColor ( 0, 0, 180 ) );

	QPixmap rit_pix ( rit_xpm );
	RIT_label = new Varilabel ( subFrame );
	RIT_label->setPixmap ( rit_pix );
	RIT_label->setGeometry ( 3, 3, 27, 11 );
	connect ( RIT_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( toggle_RIT ( int ) ) );

	QPixmap split_pix ( split_xpm );
	SPLIT_label = new Varilabel ( subFrame );
	SPLIT_label->setPixmap ( split_pix );
	SPLIT_label->setGeometry ( 3, 17, 27, 11 );
	connect ( SPLIT_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( toggle_SPLIT ( int ) ) );

	// -----------------------------------------------------------------------
	// RIT / Split Value
	rit = new QLabel ( this, "", ctlFrame );
	rit->setFont ( *fontlcd);
	rit->setPaletteForegroundColor ( QColor (255, 255, 255 ) );
	rit->setPaletteBackgroundColor  ( QColor (0, 0, 0 ) );
	rit->setFrameStyle ( QFrame::NoFrame );

	// -----------------------------------------------------------------------
	// Memory Cells

	for (int i=0; i<NUM_MEMS; i++) {
	  char buffer[256];
	  f_cell[i] = new MemoryCell(ctlFrame2);
	  f_cell[i]->setFont(*font1);
	  f_cell[i]->setPaletteForegroundColor(QColor(255, 255, 255));
	  f_cell[i]->setPaletteBackgroundColor (QColor(0, 0, 0));
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
	M_label->setPaletteForegroundColor ( QColor ( 255, 255, 255 ) );
	M_label->setPaletteBackgroundColor ( QColor ( 0, 0, 0 ) );
	M_label->setGeometry (
	    169,
	    1,
	    font1Metrics->maxWidth() * 12,
	    15 );
	M_label->setAlignment ( Qt::AlignHCenter | Qt::AlignVCenter );

	for (int i=0; i<NUM_BANDMEMS; i++) {
	  band_cell[i] = new MemoryCell();
	  band_cell[i]->setID(i+1);
	}
	band = 1;

	loadMemoryCells();

	// -----------------------------------------------------------------------
	// Spectrogram Color Aperture

	CA_label = new QLabel ( ctlFrame2 );
	CA_label->setFont ( *font1 );
	CA_label->setPaletteForegroundColor ( QColor ( 255, 100, 100 ) );
	CA_label->setPaletteBackgroundColor ( QColor ( 0, 0, 0 ) );
	CA_label->setGeometry (
	    M_label->x() + M_label->width() + 1,
	    1,
	    font1Metrics->maxWidth() * 16,
	    15 );
	CA_label->setAlignment ( Qt::AlignHCenter | Qt::AlignVCenter );


	// -----------------------------------------------------------------------
	// AGC

	AGC_label = new QLabel ( ctlFrame2 );
	AGC_label->setFont ( *font1 );
	AGC_label->setText ( " AGC" );
	AGC_label->setPaletteForegroundColor ( QColor ( 0, 180, 255 ) );
	AGC_label->setPaletteBackgroundColor ( QColor ( 0, 0, 0 ) );
	AGC_label->setGeometry (
	    CA_label->x() + CA_label->width() + 1,
	    1,
	    font1Metrics->maxWidth() * 5 + 50,
	    15 );
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
	AGC_L_label->setGeometry ( font1Metrics->maxWidth() * 5, 2, 9, 11 );
	connect ( AGC_L_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( setAGC ( int ) ) );

	QPixmap agc_s_pix ( agc_s_xpm );
	AGC_S_label = new Varilabel ( AGC_label );
	AGC_S_label->setLabel ( 2 );
	AGC_S_label->setPixmap ( agc_s_pix );
	AGC_S_label->setGeometry ( AGC_L_label->x() + 11, 2, 9, 11 );
	connect ( AGC_S_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( setAGC ( int ) ) );

	QPixmap agc_m_pix ( agc_m_xpm );
	AGC_M_label = new Varilabel ( AGC_label );
	AGC_M_label->setLabel ( 3 );
	AGC_M_label->setPixmap ( agc_m_pix );
	AGC_M_label->setGeometry ( AGC_S_label->x() + 11, 2, 9, 11 );
	connect ( AGC_M_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( setAGC ( int ) ) );

	QPixmap agc_f_pix ( agc_f_xpm );
	AGC_F_label = new Varilabel ( AGC_label );
	AGC_F_label->setLabel ( 4 );
	AGC_F_label->setPixmap ( agc_f_pix );
	AGC_F_label->setGeometry ( AGC_M_label->x() + 11, 2, 9, 11 );
	connect ( AGC_F_label, SIGNAL ( mouseRelease ( int ) ),
	          this, SLOT ( setAGC ( int ) ) );

	// -----------------------------------------------------------------------
	// Spacer for filling up empty space

	Spacer_label = new QLabel ( ctlFrame2 );
	Spacer_label->setFont ( *font1 );
	Spacer_label->setPaletteForegroundColor ( QColor ( 100,255,100 ) );
	Spacer_label->setPaletteBackgroundColor ( QColor ( 0, 0, 0 ) );
	Spacer_label->setAlignment ( Qt::AlignHCenter | Qt::AlignVCenter );

	f_label = new Varilabel ( ctlFrame );
	f_label->setFont ( *font1 );
	f_label->setPaletteForegroundColor ( QColor ( 255,255,25 ) );
	f_label->setPaletteBackgroundColor ( QColor ( 0, 0, 0 ) );
	f_label->setGeometry ( 173, 48, 85, 14 );

	CFG_label = new Varilabel ( ctlFrame2 );
	CFG_label->setFont ( *font1 );
	CFG_label->setText ( "Help & CFG" );
	CFG_label->setPaletteForegroundColor ( QColor ( 255,255,255 ) );
	CFG_label->setPaletteBackgroundColor ( QColor ( 0, 0, 0 ) );
	CFG_label->setAlignment ( Qt::AlignHCenter | Qt::AlignVCenter );
	connect ( CFG_label, SIGNAL ( mouseRelease ( int ) ), this, SLOT ( setCfg ( int ) ) );

	CPU_label = new QLabel ( ctlFrame2 );
	CPU_label->setFont ( *font1 );
	CPU_label->setPaletteForegroundColor ( QColor ( 0, 180, 255 ) );
	CPU_label->setPaletteBackgroundColor ( QColor ( 0, 0, 0 ) );
	CPU_label->setAlignment ( Qt::AlignHCenter | Qt::AlignVCenter );

	ctlFrame1->setGeometry ( 1, 1, 641, 31 );
	signalFrame->setGeometry ( 1, 3, 170, 27 );
	dBmLabel->setGeometry ( 140, 14, 35, 12 );
	lcd->setGeometry ( 170, 3, 170, 27 );
	trxFrame->setGeometry ( 337, 4, 19, 11 );
	mhzFrame->setGeometry ( 337, 19, 19, 8 );

	bFrame->setGeometry ( 363, 1, 33, 31 );
	//modeFrame->setGeometry ( 363, 1, 99, 31 );
	modeFrame->setGeometry ( 397, 1, 99, 31 );
	//swFrame->setGeometry ( 463, 1, 93, 31 );
	swFrame->setGeometry ( 497, 1, 93, 31 );
	subFrame->setGeometry ( 590, 1, 33, 31 );
	rit->setGeometry ( 625, 1, 130, 31 );

	logoFrame = new QFrame ( ctlFrame );
	logoFrame->setBackgroundColor ( QColor ( 0, 0, 0 ) );

	QPixmap logo_pix ( logo_xpm );
	logoLabel = new QLabel ( logoFrame );
	logoLabel->setPixmap ( logo_pix );
	logoLabel->setBackgroundColor ( QColor ( 0, 0, 0 ) );

	setRxFrequency( 1 );	// >> !rock_bound
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
	setCA_label();
	setTuneStep ( 0 );
	setTheme ( 2 );
	setPolyFFT ( polyphaseFFT );
	setFFTWindow ( fftWindow );
	setSpectrumType ( spectrumType );
	setAGC ( agcType );
	setTxIQGain();
	setTxIQPhase();
	setTxGain( 0 );
	setTxGain( 1 );
	processorLoad();
	if ( useHamlib )
	{
		initHamlib();
	}
	setMode(mode, FALSE, TRUE);

	QTimer *cpuTimer = new QTimer ( this );
	connect ( cpuTimer, SIGNAL ( timeout() ), this, SLOT ( processorLoad() ) );
	cpuTimer->start ( 5000, FALSE );

	QTimer *meterTimer = new QTimer ( this );
	connect ( meterTimer, SIGNAL ( timeout() ), this, SLOT ( readMeter() ) );
	meterTimer->start ( 100, FALSE );

	QTimer *fftTimer = new QTimer ( this );
	connect ( fftTimer, SIGNAL ( timeout() ), this, SLOT ( readSpectrum() ) );
	fftTimer->start ( 100, FALSE );
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
			setPaletteBackgroundColor ( QColor ( 0, 0, 0 ) );
			break;
		case 2:
			setPaletteBackgroundColor ( QColor ( 0, 0, 0 ) );
			break;
		default:
			break;
	}
}

void Main_Widget::paintEvent ( QPaintEvent * )
{
	updateLayout();
	plotSpectrum ( spectrum_head );
	drawPassBandScale();
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

}

void Main_Widget::loadSettings()
{
	QSettings settings;

	// SDR-Core Environment
	char *ep;
	if ( ( ep = getenv ( "SDR_DEFRATE" ) ) )
	{
		sample_rate = atoi ( ep );
		printf ( "sample_rate = %d\n", sample_rate );
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

     pCmd = new DttSPcmd ();

     pMeter = new DttSPmeter ();

     pSpectrum = new DttSPspectrum ();
     
     pUSBCmd = new USBSoftrockCmd ();
     // USB-Synth turns PTT on when it powers up.  Turn it off.
     pUSBCmd->sendCommand("set ptt off\n");
     
     pTXCmd = new DttSPTXcmd ();
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
	rx_f = settings.readEntry (
	           "/sdr-shell/rx_f", "14046000" ).toLongLong();
	rx_f_string = settings.readEntry (
	                  "/sdr-shell/rx_f", "14046000" );
	useIF = ( bool ) settings.readEntry (
	            "/sdr-shell/useIF", "0" ).toInt();
	useSlopeTune = ( bool ) settings.readEntry (
		  "/sdr-shell/useSlopeTune", "0" ).toInt();
	muteXmit = ( bool ) settings.readEntry (
			 "/sdr-shell/muteXmit", "0" ).toInt();
	rx_if = settings.readEntry (
	            "/sdr-shell/rx_if", "455000" ).toLongLong();
	rx_if_string = settings.readEntry (
	                   "/sdr-shell/rx_if", "455000" );
	rx_delta_f = settings.readEntry (
	                 "/sdr-shell/rx_delta_f", "11000" ).toInt();
	cwPitch = settings.readEntry (
			"/sdr-shell/cwPitch", "600" ).toInt();
	specApertureLow = settings.readEntry (
	                      "/sdr-shell/specApertureLow", "16" ).toInt();
	specApertureHigh = settings.readEntry (
	                       "/sdr-shell/specApertureHigh", "41" ).toInt();
	tuneStep = settings.readEntry (
	               "/sdr-shell/tuneStep", "1" ).toInt();
	stationCallsign = settings.readEntry (
	                      "/sdr-shell/stationCallsign", "nocall" );
	stationQTH = settings.readEntry (
	                 "/sdr-shell/stationQTH", "AA00aa" );
	iqGain = settings.readEntry (
	             "/sdr-shell/iqGain", "0" ).toInt();
	iqPhase = settings.readEntry (
	              "/sdr-shell/iqPhase", "0" ).toInt();
	txIQGain = settings.readEntry (
	             "/sdr-shell/txIQGain", "0" ).toInt();
	txIQPhase = settings.readEntry (
	              "/sdr-shell/txIQPhase", "0" ).toInt();
	txGain = settings.readEntry (
	             "/sdr-shell/txGain", "0" ).toInt();
	micGain = settings.readEntry (
	             "/sdr-shell/micGain", "0" ).toInt();
	enableTransmit = ( bool ) settings.readEntry (
	             "/sdr-shell/enableTransmit", "0" ).toInt();
	mode = ( rmode_t ) settings.readEntry (
	           "/sdr-shell/mode", "1" ).toInt();
	usbOffset = settings.readEntry (
	              "/sdr-shell/usbOffset", "1500" ).toInt();
	lsbOffset = settings.readEntry (
	              "/sdr-shell/lsbOffset", "1500" ).toInt();
	slopeLowOffset = settings.readEntry (
	              "/sdr-shell/slopeLowOffset", "3500" ).toInt();
	slopeHighOffset = settings.readEntry (
	              "/sdr-shell/slopeHighOffset", "3500" ).toInt();
	NR_state = settings.readEntry (
	               "/sdr-shell/NR_state", "0" ).toInt();
	ANF_state = settings.readEntry (
	                "/sdr-shell/ANF_state", "0" ).toInt();
	NB_state = settings.readEntry (
	               "/sdr-shell/NB_state", "0" ).toInt();
	BIN_state = settings.readEntry (
	                "/sdr-shell/BIN_state", "0" ).toInt();
	SPEC_state = settings.readEntry (
	                 "/sdr-shell/SPEC_state", "0" ).toInt();
	filterLine = settings.readEntry (
	                 "/sdr-shell/filterLine", "0" ).toInt();
	my_lat = settings.readEntry (
	             "/sdr-shell/my_lat", "0" ).toDouble();
	my_lon = settings.readEntry (
	             "/sdr-shell/my_lon", "0" ).toDouble();
	font1PointSize = settings.readEntry (
	                     "/sdr-shell/font1PointSize", "8" ).toInt();
	polyphaseFFT = settings.readEntry (
	                   "/sdr-shell/polyphaseFFT", "1" ).toInt();
	fftWindow = settings.readEntry (
	                "/sdr-shell/fftWindow", "11" ).toInt();
	spectrumType = settings.readEntry (
	                   "/sdr-shell/spectrumType", "0" ).toInt();
	agcType = settings.readEntry (
	              "/sdr-shell/agcType", "3" ).toInt();
	specCal = settings.readEntry (
	              "/sdr-shell/specCal", "70" ).toFloat();
	metrCal = settings.readEntry (
	              "/sdr-shell/metrCal", "40" ).toFloat();

	// Restore window geometry
	setGeometry (
	    settings.readEntry ( "/sdr-shell/g_left", "172" ).toInt(),
	    settings.readEntry ( "/sdr-shell/g_top", "172" ).toInt(),
	    settings.readEntry ( "/sdr-shell/g_width", "650" ).toInt(),
	    settings.readEntry ( "/sdr-shell/g_height", "424" ).toInt()
	);

	// Restore filter values
	USB_filter_l = settings.readEntry (
	                   "/sdr-shell/USB_filter_l", "20" ).toInt();
	USB_filter_h = settings.readEntry (
	                   "/sdr-shell/USB_filter_h", "2400" ).toInt();
	LSB_filter_l = settings.readEntry (
	                   "/sdr-shell/LSB_filter_l", "-2400" ).toInt();
	LSB_filter_h = settings.readEntry (
	                   "/sdr-shell/LSB_filter_h", "-20" ).toInt();
	DSB_filter_l = settings.readEntry (
	                   "/sdr-shell/DSB_filter_l", "-2400" ).toInt();
	DSB_filter_h = settings.readEntry (
	                   "/sdr-shell/DSB_filter_h", "2400" ).toInt();
	CWL_filter_l = settings.readEntry (
	                   "/sdr-shell/CWL_filter_l", "-500" ).toInt();
	CWL_filter_h = settings.readEntry (
	                   "/sdr-shell/CWL_filter_h", "-200" ).toInt();
	CWU_filter_l = settings.readEntry (
	                   "/sdr-shell/CWU_filter_l", "200" ).toInt();
	CWU_filter_h = settings.readEntry (
	                   "/sdr-shell/CWU_filter_h", "500" ).toInt();
	SAM_filter_l = settings.readEntry (
	                   "/sdr-shell/SAM_filter_l", "-4000" ).toInt();
	SAM_filter_h = settings.readEntry (
	                   "/sdr-shell/SAM_filter_h", "4000" ).toInt();
	FMN_filter_l = settings.readEntry (
	                   "/sdr-shell/FMN_filter_l", "-4000" ).toInt();
	FMN_filter_h = settings.readEntry (
	                   "/sdr-shell/FMN_filter_h", "4000" ).toInt();
	AM_filter_l = settings.readEntry (
	                  "/sdr-shell/AM_filter_l", "-2400" ).toInt();
	AM_filter_h = settings.readEntry (
	                  "/sdr-shell/AM_filter_h", "2400" ).toInt();
	useHamlib = ( bool ) settings.readEntry (
	            "/sdr-shell/useHamlib", "0" ).toInt();
	rigString = settings.readEntry (
	                "/sdr-shell/hamlib_rig", "1901" );
	rig = rigString.toInt();
	portString =  settings.readEntry (
	                  "/sdr-shell/hamlib_port", "localhost" );
	port = ( const char* ) portString;
	speedString = settings.readEntry (
	                  "/sdr-shell/hamlib_speed", "9600" );
	speed = speedString.toInt();

	rock_bound = ( bool ) settings.readEntry (
			  "/sdr-shell/rock_bound", "false" ).toInt();

	map_flag = 1;

	printf ( "::: Configuration loading completed\n" );
}

void Main_Widget::loadMemoryCells()
{
	QSettings settings;

	// Restore memory cells
	for(int i=0; i<NUM_MEMS; i++) {
	  char buffer[256], buffer2[256];
	  snprintf(buffer, 256, "/sdr-shell/f%d_frequency", i+1);
	  f_cell[i]->setFrequency(settings.readEntry(buffer, "0").toInt());
	  snprintf(buffer, 256, "/sdr-shell/f%d_txfrequency", i+1);
	  f_cell[i]->setTxFrequency(settings.readEntry(buffer, "0").toInt());
	  snprintf(buffer, 256, "/sdr-shell/f%d_mode", i+1);
	  f_cell[i]->setMode((rmode_t)settings.readEntry(buffer, "1").toInt());
	  snprintf(buffer, 256, "/sdr-shell/f%d_filter_l", i+1);
	  snprintf(buffer2, 256, "/sdr-shell/f%d_filter_h", i+1);
	  f_cell[i]->setFilter(settings.readEntry(buffer, "20").toInt(),
			       settings.readEntry(buffer2, "2400").toInt());
	}

	for (int i=0; i<NUM_BANDMEMS; i++) {
	  char buffer[256], buffer2[256];
	  snprintf(buffer, 256, "/sdr-shell/b%d_frequency", i+1);
	  band_cell[i]->setFrequency(settings.readEntry(buffer, "0").toInt());
	  snprintf(buffer, 256, "/sdr-shell/b%d_txfrequency", i+1);
	  band_cell[i]->setTxFrequency(settings.readEntry(buffer, "0").toInt());
	  snprintf(buffer, 256, "/sdr-shell/b%d_mode", i+1);
	  band_cell[i]->setMode((rmode_t)settings.readEntry(buffer, "1").toInt());
	  snprintf(buffer, 256, "/sdr-shell/b%d_filter_l", i+1);
	  snprintf(buffer2, 256, "/sdr-shell/b%d_filter_h", i+1);
	  band_cell[i]->setFilter(settings.readEntry(buffer, "20" ).toInt(),
			      settings.readEntry(buffer2, "2400" ).toInt() );
	}

	printf ( "::: Memory Cells loading completed\n" );
}


void Main_Widget::closeEvent ( QCloseEvent * )
{
	finish();
}

void Main_Widget::saveSettings()
{
	QSettings settings;

	printf ( "Saving settings...\n" );

	settings.setPath ( "n1vtn.org", ".qt", QSettings::User );
	//settings.writeEntry( "/sdr-shell/sample_rate", sample_rate );
	rx_f_string.setNum(rx_f);
	settings.writeEntry ( "/sdr-shell/rx_f", rx_f_string );
	rx_if_string.setNum(rx_if);
	settings.writeEntry ( "/sdr-shell/rx_if", rx_if_string );
	int intUseIF = ( int ) useIF;
	settings.writeEntry ( "/sdr-shell/useIF", intUseIF );
	int intUseSlopeTune = ( int ) useSlopeTune;
	settings.writeEntry ( "/sdr-shell/useSlopeTune", intUseSlopeTune );
	int intMuteXmit = ( int ) muteXmit;
	settings.writeEntry ( "/sdr-shell/useSlopeTune", intMuteXmit );
	settings.writeEntry ( "/sdr-shell/cwPitch", cwPitch );
	settings.writeEntry ( "/sdr-shell/rx_delta_f", rx_delta_f );
	settings.writeEntry ( "/sdr-shell/specApertureLow", specApertureLow );
	settings.writeEntry ( "/sdr-shell/specApertureHigh", specApertureHigh );
	settings.writeEntry ( "/sdr-shell/tuneStep", tuneStep );
	settings.writeEntry ( "/sdr-shell/stationCallsign", stationCallsign );
	settings.writeEntry ( "/sdr-shell/stationQTH", stationQTH );
	settings.writeEntry ( "/sdr-shell/iqGain", iqGain );
	settings.writeEntry ( "/sdr-shell/iqPhase", iqPhase );
	settings.writeEntry ( "/sdr-shell/txIQGain", txIQGain );
	settings.writeEntry ( "/sdr-shell/txIQPhase", txIQPhase );
	settings.writeEntry ( "/sdr-shell/txGain", txGain );
	settings.writeEntry ( "/sdr-shell/micGain", micGain );
	int intEnableTransmit = ( int ) enableTransmit;
	settings.writeEntry ( "/sdr-shell/enableTransmit", intEnableTransmit );
	settings.writeEntry ( "/sdr-shell/mode", ( int ) mode );
	settings.writeEntry ( "/sdr-shell/NR_state", NR_state );
	settings.writeEntry ( "/sdr-shell/ANF_state", ANF_state );
	settings.writeEntry ( "/sdr-shell/NB_state", NB_state );
	settings.writeEntry ( "/sdr-shell/BIN_state", BIN_state );
	settings.writeEntry ( "/sdr-shell/SPEC_state", SPEC_state );
	settings.writeEntry ( "/sdr-shell/filterLine", filterLine );
	settings.writeEntry ( "/sdr-shell/my_lat", my_lat );
	settings.writeEntry ( "/sdr-shell/my_lon", my_lon );
	settings.writeEntry ( "/sdr-shell/font1PointSize", font1PointSize );
	settings.writeEntry ( "/sdr-shell/polyphaseFFT", polyphaseFFT );
	settings.writeEntry ( "/sdr-shell/fftWindow", fftWindow );
	settings.writeEntry ( "/sdr-shell/spectrumType", spectrumType );
	settings.writeEntry ( "/sdr-shell/agcType", agcType );
	settings.writeEntry ( "/sdr-shell/specCal", specCal );
	settings.writeEntry ( "/sdr-shell/metrCal", metrCal );

	// Save window geometry
	settings.writeEntry ( "/sdr-shell/g_left", geometry().left() );
	settings.writeEntry ( "/sdr-shell/g_top", geometry().top() );
	settings.writeEntry ( "/sdr-shell/g_width", geometry().width() );
	settings.writeEntry ( "/sdr-shell/g_height", geometry().height() );

	// Save filter values
	settings.writeEntry ( "/sdr-shell/USB_filter_l", USB_filter_l );
	settings.writeEntry ( "/sdr-shell/USB_filter_h", USB_filter_h );
	settings.writeEntry ( "/sdr-shell/LSB_filter_l", LSB_filter_l );
	settings.writeEntry ( "/sdr-shell/LSB_filter_h", LSB_filter_h );
	settings.writeEntry ( "/sdr-shell/DSB_filter_l", DSB_filter_l );
	settings.writeEntry ( "/sdr-shell/DSB_filter_h", DSB_filter_h );
	settings.writeEntry ( "/sdr-shell/CWL_filter_l", CWL_filter_l );
	settings.writeEntry ( "/sdr-shell/CWL_filter_h", CWL_filter_h );
	settings.writeEntry ( "/sdr-shell/CWU_filter_l", CWU_filter_l );
	settings.writeEntry ( "/sdr-shell/CWU_filter_h", CWU_filter_h );
	settings.writeEntry ( "/sdr-shell/SAM_filter_l", SAM_filter_l );
	settings.writeEntry ( "/sdr-shell/SAM_filter_h", SAM_filter_h );
	settings.writeEntry ( "/sdr-shell/FMN_filter_l", FMN_filter_l );
	settings.writeEntry ( "/sdr-shell/FMN_filter_h", FMN_filter_h );
	settings.writeEntry ( "/sdr-shell/AM_filter_l", AM_filter_l );
	settings.writeEntry ( "/sdr-shell/AM_filter_h", AM_filter_h );

	// Save memory cells
	QString f_string;

	for (int i=0; i<NUM_MEMS; i++) {
	  char buffer[256];

	  snprintf(buffer, 256, "/sdr-shell/f%d_frequency", i+1);
	  f_string.sprintf("%lld", f_cell[i]->getFrequency());
	  settings.writeEntry (buffer, f_string);

	  snprintf(buffer, 256, "/sdr-shell/f%d_mode", i+1);
	  settings.writeEntry(buffer, f_cell[i]->getMode());

	  snprintf(buffer, 256, "/sdr-shell/f%d_filter_l", i+1);
	  settings.writeEntry(buffer, f_cell[i]->getFilter_l());

	  snprintf(buffer, 256, "/sdr-shell/f%d_filter_h", i+1);
	  settings.writeEntry(buffer, f_cell[i]->getFilter_h());
	}

	for (int i=0; i<NUM_BANDMEMS; i++) {
	  char buffer[256];

	  snprintf(buffer, 256, "/sdr-shell/b%d_frequency", i+1);
	  f_string.sprintf("%lld", band_cell[i]->getFrequency());
	  settings.writeEntry (buffer, f_string );

	  if (band_cell[i]->getTxFrequency() != 0) {
	    snprintf(buffer, 256, "/sdr-shell/b%d_txfrequency", i+1);
	    f_string.sprintf("%lld", band_cell[i]->getTxFrequency());
	    settings.writeEntry(buffer, f_string );
	  }

	  snprintf(buffer, 256, "/sdr-shell/b%d_mode", i+1);
	  settings.writeEntry (buffer, band_cell[i]->getMode() );


	  snprintf(buffer, 256, "/sdr-shell/b%d_filter_l", i+1);
	  settings.writeEntry(buffer, band_cell[i]->getFilter_l() );

	  snprintf(buffer, 256, "/sdr-shell/b%d_filter_h", i+1);
	  settings.writeEntry(buffer, band_cell[i]->getFilter_h() );
	}

	settings.writeEntry ( "/sdr-shell/hamlib_rig", rigString );
	settings.writeEntry ( "/sdr-shell/useHamlib", (int) useHamlib );
	settings.writeEntry ( "/sdr-shell/hamlib_port", portString );
	settings.writeEntry ( "/sdr-shell/hamlib_speed", speedString );
	settings.writeEntry ( "/sdr-shell/usbOffset", usbOffset );
	settings.writeEntry ( "/sdr-shell/lsbOffset", lsbOffset );
	settings.writeEntry ( "/sdr-shell/slopeLowOffset", slopeLowOffset );
	settings.writeEntry ( "/sdr-shell/slopeHighOffset", slopeHighOffset );

	settings.writeEntry ( "/sdr-shell/rock_bound", (int) rock_bound );
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

	switch ( e->state() )
	{
		case Qt::ShiftButton:
			switch ( e->key() )
			{
				case 82: state = RX_F; break;
				case 84: state = TX_F; break;
					//case 72: state = FILTER_H; break;
					//case 76: state = FILTER_L; break;
				default: break;
			}
			break;
		case Qt::AltButton:
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
		case 4117: // Down arrow
		case 72: // h
			//if ( tuneStep > 0 ) tuneStep--;
			setTuneStep ( -1 );
			break;
		case 4114: // Left arrow
		case 74: // j
			fprintf ( stderr, "Left arrow rx_delta_f is: %d.\n",rx_delta_f);
			if ( rock_bound )
			{
				if ( rx_delta_f < sample_rate / 2 - 2000 )  //rx_delta_f > 0 when you tune down!
					rx_delta_f = rx_delta_f + ( int ) pow ( 10, tuneStep );
				else
				  rx_delta_f =  ( sample_rate / 2 - 2000 );
			}
			else
			{
				rx_f -= rx_delta_f;
				rx_delta_f = -sample_rate/4;
				rx_f = rx_f - (int) pow ( 10, tuneStep );
				rx_f += rx_delta_f;

				// Round to tuneStep
				//rx_f = (rx_f-rx_delta_f) / (int) pow ( 10, tuneStep );
				//rx_f = rx_f * (int) pow ( 10, tuneStep ) + rx_delta_f;

				// disable RIT
				if (enableRIT)
					set_RIT( 0 );
			/*
				fprintf (stderr, "set freq %f\n", (rx_f)*1e-6 ); 
				pUSBCmd->sendCommand("set freq %f\n", (rx_f)*1e-6 ); 
			*/
			}
			setRxFrequency( 1 );	// >> !rock_bound
			break;
		case 4115: // Up arrow
		case 76: // l
			//if ( tuneStep < 3 ) tuneStep++;
			setTuneStep ( +1 );
			break;
		case 4116: // Right arrow
		case 75:  // k
			fprintf ( stderr, "Right arrow rx_delta_f is: %d.\n",rx_delta_f);
			if ( rock_bound )
			{
				if ( rx_delta_f > -( sample_rate / 2 - 2000 ) )  //rx_delta_f < 0 when you tune up!
					rx_delta_f = rx_delta_f - ( int ) pow ( 10, tuneStep );
				else
				  rx_delta_f = - ( sample_rate / 2 - 2000 );
			}
			else
			{
				rx_f -= rx_delta_f;
				rx_delta_f = -sample_rate/4;
				rx_f = rx_f + (int) pow ( 10, tuneStep );
				rx_f += rx_delta_f;

				// Round to tuneStep
				//rx_f = (rx_f-rx_delta_f) / (int) pow ( 10, tuneStep );
				//rx_f = rx_f * (int) pow ( 10, tuneStep ) + rx_delta_f;

				// disable RIT
				if (enableRIT)
					set_RIT( 0 );
			/*
				fprintf (stderr, "set freq %f\n", (rx_f)*1e-6 ); 
				pUSBCmd->sendCommand("set freq %f\n", (rx_f)*1e-6);
			*/
			}
			setRxFrequency( 1 );	// >> !rock_bound
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
			spec_width = DEFSPEC;
			once = 1;
			break;
		case '2':
			spec_width = DEFSPEC/2;
			once = 1;
			fprintf(stderr, "spec_width=%d\n", spec_width);
			break;
		case '4':
			spec_width = DEFSPEC/4;
			once = 1;
			fprintf(stderr, "spec_width=%d\n", spec_width);
			break;
		case '8':
			spec_width = DEFSPEC/8;
			once = 1;
			fprintf(stderr, "spec_width=%d\n", spec_width);
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
				if (transmit)
				{
					transmit = 0;
					pTXCmd->sendCommand ("setTRX 0\n");
					pTXCmd->sendCommand ("setRunState 0\n");
					pUSBCmd->sendCommand("set ptt off\n" );
					fprintf (stderr, "set ptt off\n");
					trxFrame->setPaletteBackgroundPixmap ( *rxPix );
					set_MUTE ( 0 );
				} else {
					transmit = 1;
					pTXCmd->sendCommand ("setRunState 2\n");
					pUSBCmd->sendCommand ("set ptt on\n" );
					pTXCmd->sendCommand ("setTRX 1\n");
					set_MUTE ( 1 );
					fprintf (stderr, "set ptt on\n");
					trxFrame->setPaletteBackgroundPixmap ( *txPix );
				}
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
	lcd->setText(text);
//	lcd->display ( text );	
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
	setRxFrequency( 1 );	// >> !rock_bound
}

//
// synth flag means set the external synth (usbsoftrock) also
//
void Main_Widget::setRxFrequency( int synth )
{
	char text[32];
	if ( !useIF )
	{
		snprintf ( text, 32, "......%11.6lf",
			( double ) ( rx_f - rx_delta_f ) / 1000000.0 );
		fprintf ( stderr, "Set the frequency: %lld - %d = %11.6lf '%s'\n",
			rx_f, rx_delta_f, 
			( rx_f - rx_delta_f) / 1000000.0, text);
		displayMutex.lock();
		lcd->setText( ( QString ) text);
		//lcd->display ( text );

		if (enableRIT) {
			tx_f_string.sprintf ("%11.0lf", ( double ) ( tx_delta_f - rx_delta_f ) );
			rit->setText( tx_f_string );
			fprintf ( stderr, "RIT %s\n", tx_f_string.ascii());
		}
		displayMutex.unlock();
	}
	fprintf ( stderr, "setOsc %d\n", rx_delta_f );
	pCmd->sendCommand ("setOsc %d %d\n", rx_delta_f, 0 );
	if (!enableRIT && !enableSPLIT) {
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
			fprintf (stderr, "set freq %f\n", (rx_f)*1e-6 ); 
			pUSBCmd->sendCommand("set freq %f\n", (rx_f)*1e-6);
		}
	}
	}
}

void Main_Widget::setTxFrequency()
{
	pTXCmd->sendCommand ("setOsc %d %d\n", -rx_delta_f, 1 );
	fprintf ( stderr, "setOsc %d\n", rx_delta_f );
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
	drawPassBandScale();
}

void Main_Widget::setLowerFilterScale ( int x )
{
	float bin_bw = sample_rate/(float)spec_width;
	int stop_band;

	stop_band = ( int ) ( ( ( x - ( spectrogram->width() / 2 ) ) * bin_bw ) ) /10*10;

	if ( stop_band < *filter_h )
	{
		*filter_l = stop_band;
		setFilter();
	}
}

void Main_Widget::setUpperFilterScale ( int x )
{
	float bin_bw = sample_rate/(float)spec_width;
	int stop_band;

	stop_band = ( int ) ( ( ( x - ( spectrogram->width() / 2 ) ) * bin_bw ) ) /10*10;

	if ( stop_band > *filter_l )
	{
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
        if ( rock_bound ) 
	{
	  if ( tuneStep < 3 && step > 0 ) tuneStep++;
	  else if ( tuneStep > 0 && step < 0 ) tuneStep--;
	}
	else
	{
	  if ( tuneStep < 8 && step > 0 ) tuneStep++;
	  else if ( tuneStep > 0 && step < 0 ) tuneStep--;
	}

	switch ( tuneStep )
	{
		case 0:
			step_1Hz_frame->setPaletteBackgroundColor ( QColor ( 200, 200, 255 ) );
			step_10Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_1000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_1000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			break;
		case 1:
			step_1Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10Hz_frame->setPaletteBackgroundColor ( QColor ( 200, 200, 255 ) );
			step_100Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_1000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_1000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			break;
		case 2:
			step_1Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100Hz_frame->setPaletteBackgroundColor ( QColor ( 200, 200, 255 ) );
			step_1000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_1000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			break;
		case 3:
			step_1Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_1000Hz_frame->setPaletteBackgroundColor ( QColor ( 200, 200, 255 ) );
			step_10000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_1000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			break;
		case 4: 
			step_1Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_1000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10000Hz_frame->setPaletteBackgroundColor ( QColor ( 200, 200, 255 ) );
			step_100000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_1000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			break;
		case 5: 
			step_1Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_1000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000Hz_frame->setPaletteBackgroundColor ( QColor ( 200, 200, 255 ) );
			step_1000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			break;
		case 6: 
			step_1Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_1000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_1000000Hz_frame->setPaletteBackgroundColor ( QColor ( 200, 200, 255) );
			step_10000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			break;
		case 7: 
			step_1Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_1000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_1000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10000000Hz_frame->setPaletteBackgroundColor ( QColor ( 200, 200, 255 ) );
			step_100000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			break;
		case 8: 
			step_1Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_1000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_1000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000000Hz_frame->setPaletteBackgroundColor ( QColor ( 200, 200, 100 ) );
			break;
		default:
			step_1Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_1000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_1000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_10000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
			step_100000000Hz_frame->setPaletteBackgroundColor ( QColor ( 50, 50, 100 ) );
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

	LSB_label->setBackgroundColor ( c_off );
	USB_label->setBackgroundColor ( c_off );
	DSB_label->setBackgroundColor ( c_off );
	AM_label->setBackgroundColor ( c_off );
	CWL_label->setBackgroundColor ( c_off );
	CWU_label->setBackgroundColor ( c_off );
	SAM_label->setBackgroundColor ( c_off );
	FMN_label->setBackgroundColor ( c_off );

	switch ( mode )
	{
		case RIG_MODE_USB:
			modeStr = "USB";
			pCmd->sendCommand ("setMode %d %d\n", USB );
			pTXCmd->sendCommand ("setMode %d %d\n", USB, 1 );
			fprintf ( stderr, "setMode %d\n", USB );
			filter_l = &USB_filter_l; //20;
			filter_h = &USB_filter_h; //2400;
			USB_label->setBackgroundColor ( c_on );
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
			LSB_label->setBackgroundColor ( c_on );
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
			DSB_label->setBackgroundColor ( c_on );
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
			AM_label->setBackgroundColor ( c_on );
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
			CWL_label->setBackgroundColor ( c_on );
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
			CWU_label->setBackgroundColor ( c_on );
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
			SAM_label->setBackgroundColor ( c_on );
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
			FMN_label->setBackgroundColor ( c_on );
			if ( !displayOnly && useHamlib )
				emit changeRigMode ( RIG_MODE_FM, 12000 );
			break;
		default:
			modeStr = "Unknown";
			filter_l = &USB_filter_l; //20;
			filter_h = &USB_filter_h; //2400;
	}
	fprintf( stderr, "Set mode: %s\n", modeStr.ascii());

	// call external mode setting hook.
	if (sdr_mode != NULL) {
		QString cmd = sdr_mode;
		cmd += " ";
		cmd += modeStr;
		fprintf( stderr, "Set mode: %s\n", cmd.ascii());
		system(cmd.ascii());
	}
	setFilter();
	if ( useIF ) setDefaultRxFrequency();
	else setRxFrequency( 1 );	// >> !rock_bound
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
			//signalBargraph[j]->setPaletteBackgroundColor( *signalColor[j] );
			signalBargraph[j]->setPaletteBackgroundColor ( QColor ( 0,180,255 ) );
		else if ( j != k )
			signalBargraph[j]->setPaletteBackgroundColor ( QColor ( 50,50,50 ) );
		else if ( k != 0 )
			signalBargraph[j]->setPaletteBackgroundColor ( *signalColor[j] );
	}
}

//----------------------------------------------------------------------------
// Fetch the spectrum data from sdr-core
//
void Main_Widget::readSpectrum()
{
	int j, k, l, m, n;
	int label, stamp;
	float raw_spectrum[DEFSPEC];
	float a;

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

	drawSpectrogram();
	if ( SPEC_state ) plotSpectrum ( spectrum_head );
}

void Main_Widget::drawSpectrogram()
{
	int x, x1, x2, x3, x4;
	static int y = 0;
	int pwr;
	float pwr_range;
	float bin_bw = sample_rate/(float)spec_width;

	pwr_range = specApertureHigh - specApertureLow;

	QImage spectrogramLine ( spectrogram->width(), 1 , 32 );

	x3 = 0;
	x1 = spec_width/2 - spectrogram->width() /2;
	if (x1 < 0) {
		x3 = abs(x1);
		x1 = 0;
	}
	x2 = x1 + spectrogram->width();
	x4 = spec_width;

	if (spectrogram->width() > spec_width) {
		x2 = spec_width;
		uint rgb = qRgb ( spec_r[0], spec_g[0], spec_b[0] );
		for ( x = 0; x < spectrogram->width(); x++ ) {
			uint *p = ( uint * ) spectrogramLine.scanLine ( 0 ) + ( x );
			*p = rgb;
		}
		// spectrum from dttsp is centered at Osc.  When Osc is not zero, some of
		// the spectrum will be above the width or below zero.  dttps returns
		// sort of mirrored data.  Don't show it.
		if (rx_delta_f < 0) {
			x4 = spec_width - (abs(rx_delta_f) / bin_bw);
		}
	}

	for ( x = 0; x < spec_width; x++ )
	{
		// Compute the power (magnified)
		pwr = ( int ) ( ( spectrum[ x ] + specCal - specApertureLow ) *
		                ( 120 / pwr_range ) );

		// Sanitize
		if ( pwr > 119 ) pwr = 119;
		else if ( pwr < 0 ) pwr = 0;

		// Save power for spectrum display
		spectrum_history[y][x] = ( int ) ( spectrum[ x ] + specCal );

		// If bin is in visible range, plot it.
		if ( x >= x1 && x < x2 )
		{
			// Set the pixel color
			uint *p = ( uint * ) spectrogramLine.scanLine ( 0 ) + ( x - x1 + x3 );
			*p = qRgb ( spec_r[pwr], spec_g[pwr], spec_b[pwr] );

			// Turn on vertical filter line
			if ( filterLine &&
			        ( x - x1 + x3 ) == spectrogram->width() / 2 +
			        ( int ) ( *filter_l / ( sample_rate/(float) spec_width ) ) )
			{
				*p = qRgb ( 100, 255, 100 );
			}

			if ( filterLine &&
			        ( x - x1 + x3 ) == spectrogram->width() / 2 +
			        ( int ) ( *filter_h / ( sample_rate/(float)spec_width ) ) )
			{
				*p = qRgb ( 100, 255, 100 );
			}

			// Set the center mark
			if ( ( x - x1 + x3 ) == spectrogram->width() / 2 )
				*p = qRgb ( 255,0,0 );
			if ( x == x4 )
				*p = qRgb (0, 255, 0);
		}
	}

	if (once) {
		fprintf(stderr, "width=%d wwidth=%d x1=%d x2=%d x3=%d bin_bw=%f\n",
			spec_width, spectrogram->width(), x1, x2, x3, bin_bw);
		fprintf(stderr, "  Osc=%d bins=%.1f stop=%d x4=%d\n",
			rx_delta_f, 
			rx_delta_f / bin_bw,
			(int)(spec_width - (abs(rx_delta_f) / bin_bw)),
			x4);
	}

	spectrum_head = y;

	// Draw the spectrum line
	QPainter p;
	p.begin ( spectrogram );
	p.drawImage ( 0, y = ( y+1 ) % spectrogram->height(), spectrogramLine );

	p.setPen ( Qt::yellow );
	p.drawLine ( spectrogram->width() / 2 +
	             ( int ) ( *filter_l / ( sample_rate/(float)spec_width ) ), y+1,
	             spectrogram->width() / 2 +
	             ( int ) ( *filter_h / ( sample_rate/(float)spec_width ) ), y+1 );
	p.drawLine ( spectrogram->width() / 2 +
	             ( int ) ( *filter_l / ( sample_rate/(float)spec_width ) ), y+2,
	             spectrogram->width() / 2 +
	             ( int ) ( *filter_h / ( sample_rate/(float)spec_width ) ), y+2 );

	p.end();
}

void Main_Widget::drawPassBandScale()
{
	float bin_bw = sample_rate/(float)spec_width;
	char temp[20];
	int x1, x2;

	x1 = spectrogram->width() / 2 + ( int ) ( *filter_l / bin_bw );
	x2 = spectrogram->width() / 2 + ( int ) ( *filter_h / bin_bw );

	QPainter p;
	p.begin ( pbscale );
	p.setFont ( *font1 );

	p.eraseRect ( 0, 0, pbscale->width(), pbscale->height() );

	snprintf ( temp, 20, "%5d", *filter_l );
	p.setPen ( Qt::cyan );
	p.drawText ( x1 - 11 - font1Metrics->maxWidth() * 5,
	             font1Metrics->ascent() + 1, temp );

	// Draw pb_l arrow
	p.setPen ( Qt::yellow );
	p.drawLine ( x1 - 10, 7, x1, 7 );
	p.drawLine ( x1 - 4, 4, x1 - 1, 7 );
	p.drawLine ( x1 - 4, 10, x1 - 1, 7 );
	p.drawLine ( x1, 0, x1, 15 );

	snprintf ( temp, 20, "%-5d", *filter_h );
	p.setPen ( Qt::cyan );
	p.drawText ( x2 + 12, font1Metrics->ascent() + 1, temp );

	// Draw pb_h arrow
	p.setPen ( Qt::yellow );
	p.drawLine ( x2 + 10, 7, x2, 7 );
	p.drawLine ( x2 + 4, 4, x2 + 1, 7 );
	p.drawLine ( x2 + 4, 10, x2 + 1, 7 );
	p.drawLine ( x2, 0, x2, 15 );

	p.end();
}

void Main_Widget::plotSpectrum ( int y )
{
	float bin_bw = sample_rate/(float)spec_width;
	int x, x1, x2;
	double kHz_step;
	int f;
	char f_text[10];
	char nlabel;

	int f1, f2;

	f1 = spectrogram->width() / 2 + ( int ) ( *filter_l / bin_bw );
	f2 = spectrogram->width() / 2 + ( int ) ( *filter_h / bin_bw );

	QPixmap pix ( spectrumFrame->width(), spectrumFrame->height() );
	pix.fill ( QColor ( 0,0,0 ) );

	x1 = spec_width/2 - spectrogram->width() /2;
	if (x1 >= 0) {
		x2 = x1 + spectrogram->width();
	} else {
		x2 = spec_width + abs(x1);
	}
	kHz_step = 1000 / ( sample_rate / (float)spec_width );
	if (once) {
		fprintf(stderr, "bin_bw=%4.4f width=%d f1=%d f2=%d x1=%d x2=%d\n",
			bin_bw, spectrogram->width(), f1, f2, x1, x2);
		once = 0;
	}

	QPainter p;
	p.begin ( &pix );
	p.setFont ( *font1 );

	// Draw Passband
	p.fillRect ( f1, 0, f2-f1+1, 120, QColor ( 0, 50, 0 ) );

	// Draw Spectrum
	for ( x = 0; x < spectrumFrame->width(); x++ )
	{
		if (x + x1 < 0 || x > x2)
			continue;
		p.setPen ( QColor ( 0, 200, 0 ) );
		p.drawLine ( x, spectrumFrame->height() -
		             spectrum_history[y][x + x1],
		             x + 1, spectrumFrame->height() -
		             spectrum_history[y][x + x1 + 1] );
	}

	// Draw the 1 kHz positive marks
	nlabel = DEFSPEC / spec_width;
	f = 0;
	for ( double dx = spectrumFrame->width() /2;
	        dx <= spectrumFrame->width();
	        dx = dx + kHz_step )
	{
		p.setPen ( QColor ( 100, 100, 100 ) );
		p.drawLine ( ( int ) rint ( dx ),
		             font1Metrics->ascent(),
		             ( int ) rint ( dx ),
		             spectrumFrame->height() );
		//snprintf( f_text, 10, "%lf", (double)((rx_f + f) / 1000000.0) );
		if ((f/1000) % nlabel == 0) {
			snprintf ( f_text, 10, "%d", f );
			p.setPen ( QColor ( 255, 255, 255 ) );
			p.drawText ( ( int ) rint ( dx ) - ( font1Metrics->maxWidth() *
		                                     strlen ( f_text ) ) / 2,
		        		10,
					f_text );
		}
		f = f + 1000;
	}

	// Draw the 1 kHz negative marks
	f = -1000;
	for ( double dx = spectrumFrame->width() /2 - kHz_step;
	        dx >= 0;
	        dx = dx - kHz_step )
	{
		p.setPen ( QColor ( 100, 100, 100 ) );
		p.drawLine ( ( int ) rint ( dx ),
		             font1Metrics->ascent(),
		             ( int ) rint ( dx ),
		             spectrumFrame->height() );
		//snprintf( f_text, 10, "%lf", (double)((rx_f + f) / 1000000.0) );
		if ((f/1000) % nlabel == 0) {
			snprintf ( f_text, 10, "%d", f );
			p.setPen ( QColor ( 255, 255, 255 ) );
			//p.drawText( (int)rint(dx) + 2, 10, f_text );
			p.drawText ( ( int ) rint ( dx ) - ( font1Metrics->maxWidth() *
		                                     ( strlen ( f_text ) + 1 ) ) / 2,
		             		10,
		             		f_text );
		}
		f = f - 1000;
	}

	// Draw the dB marks
	for ( int i = 20; i < 120; i += 20 )
	{
		p.setPen ( QColor ( 100, 100, 100 ) );
		p.drawLine ( 1, i, spectrumFrame->width() - 2, i );
	}

	// Draw the color aperture lines
	QPen pen ( Qt::DotLine );
	pen.setColor ( QColor ( 255, 50, 0 ) );
	p.setPen ( pen );
	p.drawLine ( 0, ( int ) ( specApertureLow - 120 ) * -1,
	             spectrumFrame->width(), ( int ) ( specApertureLow - 120 ) * -1 );
	p.drawLine ( 0, ( int ) ( specApertureHigh - 120 ) * -1,
	             spectrumFrame->width(), ( int ) ( specApertureHigh - 120 ) * -1 );

	// Draw the dB labels
	for ( int i = 0; i < 120; i += 20 )
	{
		p.setPen ( QColor ( 255, 255, 255 ) );
		snprintf ( f_text, 10, "%d", -i - 40 );
		p.drawText ( 2, i + 19, f_text );
		snprintf ( f_text, 10, "%4d", -i - 40 );
		p.drawText ( spectrumFrame->width() -
		             font1Metrics->maxWidth() * 4 - 2, i + 19, f_text );
	}

	p.end();

	bitBlt ( spectrumFrame, 0, 0, &pix,
	         0, 0, pix.width(), pix.height(), Qt::CopyROP, true );
}

void Main_Widget::spectrogramClicked ( int x )
{
	int f;

	int f_limit = sample_rate/2 - 2000;
	if ( !useIF )  // Disable changing frequency for IF mode.  Use arrows for IF shift.
	{
		f = ( int ) ( ( sample_rate/(float)spec_width ) * ( spectrogram->width() /2 - x ) );
		
		fprintf(stderr, "The value of f is %d x is %d\n",f, x);

		if ( rock_bound )
		{
			if ( rx_delta_f >  f_limit ) rx_delta_f =  f_limit;
			if ( rx_delta_f < -f_limit ) rx_delta_f = -f_limit;
			else 
			{
				rx_delta_f = rx_delta_f + f + *filter_l + ( *filter_h - *filter_l ) / 2 ;	
			}
		}
		else
		{
	 /*	  fprintf (stderr,"Before, the rx_f sent to the usb is %f MHz. and rx_delta_f is %f KHz.",rx_f*1e-6, rx_delta_f*1e-3);
	  *	  rx_f = rx_f - rx_delta_f + sample_rate/4;
	  *	  rx_delta_f =  -sample_rate/4; //Make it not tuned to the center
	  *	  fprintf (stderr,"The rx_f sent to the usb is %f MHz.",rx_f*1e-6); */

		  // Clicking re-tunes the synth and osc.  Set osc to center.
		  rx_f -= rx_delta_f;
		  rx_delta_f =  -sample_rate/4; //Make it not tuned to the center
		  rx_f = rx_f - f - *filter_l - ( *filter_h - *filter_l ) / 2;
		  rx_f += rx_delta_f;
		}
		setRxFrequency( 1 );	// >> !rock_bound
	}
}

void Main_Widget::f_at_mousepointer ( int x )
{
	int f;
	char temp[32];

	f = ( int ) ( ( sample_rate/(float)spec_width ) * ( spectrogram->width() /2 - x ) );
	if ( !rock_bound ) f = f - sample_rate / 4 ;

	snprintf ( temp, 32, "%.6lf", ( double ) ( rx_f - f ) / 1000000.0 );
	M_label->setText ( temp );
}

void Main_Widget::tune ( int x )
{
	int f_limit = sample_rate/2 - 2000;

	// if (spectrogram->width() < spec_width)
	// f_limit = sample_rate / 2 - spectrogram->width() / 2

	// use usbsoftrock if the tuning step is large enough
	if ( x > 10000 || x < -10000 ) {
		rx_delta_f = -sample_rate/4;
		rx_f = rx_f + x;
		setRxFrequency( 1 );	// >> !rock_bound
	} else {
		rx_delta_f += x;
		if ( rx_delta_f >  f_limit ) rx_delta_f =  f_limit;
		if ( rx_delta_f < -f_limit ) rx_delta_f = -f_limit;
		setRxFrequency( 0 );
	}
}

void Main_Widget::focusInEvent ( QFocusEvent * )
{
	ctlFrame->setPaletteBackgroundColor ( QColor ( 255, 200, 0 ) );
}

void Main_Widget::focusOutEvent ( QFocusEvent * )
{
	ctlFrame->setPaletteBackgroundColor ( QColor ( 50, 50, 50 ) );
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
	if ( NR_state ) NR_label->setBackgroundColor ( QColor ( 0, 100, 200 ) );
	else NR_label->setBackgroundColor ( QColor ( 0, 0, 0 ) );

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
	if ( ANF_state ) ANF_label->setBackgroundColor ( QColor ( 0, 100, 200 ) );
	else ANF_label->setBackgroundColor ( QColor ( 0, 0, 0 ) );

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
	if ( NB_state ) NB_label->setBackgroundColor ( QColor ( 0, 100, 200 ) );
	else NB_label->setBackgroundColor ( QColor ( 0, 0, 0 ) );

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
	if ( BIN_state ) BIN_label->setBackgroundColor ( QColor ( 200, 0, 0 ) );
	else BIN_label->setBackgroundColor ( QColor ( 0, 0, 0 ) );

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
	if ( MUTE_state ) MUTE_label->setBackgroundColor ( QColor ( 200, 0, 0 ) );
	else MUTE_label->setBackgroundColor ( QColor ( 0, 0, 0 ) );

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
	if ( SPEC_state ) SPEC_label->setBackgroundColor ( QColor ( 0, 150, 50 ) );
	else SPEC_label->setBackgroundColor ( QColor ( 0, 0, 0 ) );
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
		fprintf( stderr, "Set band: %s\n", cmd.ascii());
		system(cmd.ascii());
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
		if (transmit)
		{
			transmit = 0;
			pTXCmd->sendCommand ("setTRX 0\n");
			pUSBCmd->sendCommand("set ptt off\n" );
			fprintf (stderr, "set ptt off\n");
			trxFrame->setPaletteBackgroundPixmap ( *rxPix );
			set_MUTE ( 0 );
		} else {
			// if enableSPLIT   set USBSoftrock frequency
			transmit = 1;
			pUSBCmd->sendCommand ("set ptt on\n" );
			pTXCmd->sendCommand ("setTRX 1\n");
			set_MUTE ( 1 );
			fprintf (stderr, "set ptt on\n");
			trxFrame->setPaletteBackgroundPixmap ( *txPix );
		}
	} else {
		fprintf( stderr, "Transmit is not enabled\n");
	}
}

void Main_Widget::set_RIT ( int state )
{
	enableRIT = state;
	if ( enableRIT ) {
		RIT_label->setBackgroundColor ( QColor ( 0, 0, 150 ) );
		set_SPLIT( 0 );
		tx_f = rx_f;
		tx_delta_f = rx_delta_f;
	} else {
		RIT_label->setBackgroundColor ( QColor ( 0, 0, 0 ) );
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
		SPLIT_label->setBackgroundColor ( QColor ( 0, 0, 150 ) );
		set_RIT( 0 );
		tx_f_string.sprintf ("%11.6lf", 
			( double ) ( tx_f - tx_delta_f ) / 1000000.0 );
		fprintf(stderr, "set_SPLIT %s\n", tx_f_string.ascii());
		rit->setText( tx_f_string );
	} else {
		SPLIT_label->setBackgroundColor ( QColor ( 0, 0, 0 ) );
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
	} else {
		rx_delta_f = -sample_rate/4;
		rx_f = m->getFrequency();
		rx_f += rx_delta_f;
		fprintf (stderr, "set freq %f\n", (rx_f)*1e-6 ); 
		pUSBCmd->sendCommand("set freq %f\n", (rx_f)*1e-6);
		tx_f = m->getTxFrequency();
		set_RIT( 0 );	// do this before set_SPLIT, as it clears the RIT text
		if (tx_f != 0) {
			tx_delta_f = -sample_rate/4;
			tx_f += tx_delta_f;
			set_SPLIT ( TRUE );
		} else {
			set_SPLIT ( FALSE );
		}
	}
	setRxFrequency( 1 );	// >> !rock_bound
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
	pb_f = ( int ) ( ( ( x - ( spectrogram->width() / 2 ) ) * bin_bw ) ) /10*10;
	snprintf ( temp, 32, "%d", pb_f );
	M_label->setText ( temp );
}

void Main_Widget::updateCallsign()
{
	stationCallsign = cfgCallInput->text();
	setCaption ( "SDR-Shell rxtx.4 @ " + stationCallsign );
}

void Main_Widget::updateLOFreq()
{
	rx_f = cfgLOFreqInput->text().toLongLong();
	rx_f_string = cfgLOFreqInput->text();
	if ( useIF ) setDefaultRxFrequency();
	else setRxFrequency( 1 );	// >> !rock_bound
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
	port = ( const char* ) portString;
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

void Main_Widget::setPolyFFT ( int state )
{
	polyphaseFFT = state;
	pCmd->sendCommand ("setSpectrumPolyphase %d\n", state );
	fprintf ( stderr, "setSpectrumPolyphase %d\n", state );
}

void Main_Widget::setFFTWindow ( int window )
{
	fftWindow = window;
	pCmd->sendCommand ("setSpectrumWindow %d\n", window );
	fprintf ( stderr, "setSpectrumWindow %d\n", window );
}

void Main_Widget::setSpectrumType ( int type )
{
	spectrumType = type;
	pCmd->sendCommand ("setSpectrumType %d\n", type + 1 );
	fprintf ( stderr, "setSpectrumType %d\n", type + 1 );

}

void Main_Widget::setAGC ( int type )
{
	agcType = type;
	pCmd->sendCommand ("setRXAGC %d\n", type );
	fprintf ( stderr, "setRXAGC %d\n", type );

	QColor on ( 150, 50, 50 );
	QColor off ( 0, 0, 0 );

	switch ( type )
	{
		case 0:
			//AGC_O_label->setBackgroundColor( on );
			AGC_L_label->setBackgroundColor ( off );
			AGC_S_label->setBackgroundColor ( off );
			AGC_M_label->setBackgroundColor ( off );
			AGC_F_label->setBackgroundColor ( off );
			break;
		case 1:
			//AGC_O_label->setBackgroundColor( off );
			AGC_L_label->setBackgroundColor ( on );
			AGC_S_label->setBackgroundColor ( off );
			AGC_M_label->setBackgroundColor ( off );
			AGC_F_label->setBackgroundColor ( off );
			break;
		case 2:
			//AGC_O_label->setBackgroundColor( off );
			AGC_L_label->setBackgroundColor ( off );
			AGC_S_label->setBackgroundColor ( on );
			AGC_M_label->setBackgroundColor ( off );
			AGC_F_label->setBackgroundColor ( off );
			break;
		case 3:
			//AGC_O_label->setBackgroundColor( off );
			AGC_L_label->setBackgroundColor ( off );
			AGC_S_label->setBackgroundColor ( off );
			AGC_M_label->setBackgroundColor ( on );
			AGC_F_label->setBackgroundColor ( off );
			break;
		case 4:
			//AGC_O_label->setBackgroundColor( off );
			AGC_L_label->setBackgroundColor ( off );
			AGC_S_label->setBackgroundColor ( off );
			AGC_M_label->setBackgroundColor ( off );
			AGC_F_label->setBackgroundColor ( on );
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
	useIF =  value;
}

void Main_Widget::updateUseUSBsoftrock ( bool value )
{
	rock_bound =  !value;
	rx_delta_f = - sample_rate / 4 ;
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
	useHamlib =  value;
	if ( value ) initHamlib();
	else if (ourHamlibWrapper) ourHamlibWrapper->~hamlibWrapper();
	emit toggleHamlibButton ( useHamlib );
}

void Main_Widget::setMuteXmit ( bool value )
{
	muteXmit =  value;
	emit tellMuteXmit ( muteXmit );
}


void Main_Widget::initHamlib ()
{
	ourHamlibWrapper = new hamlibWrapper ( this );

	connect ( ourHamlibWrapper, SIGNAL ( nowTransmit ( int ) ), this, SLOT ( set_MUTE ( int ) ) );
	connect ( ourHamlibWrapper, SIGNAL ( newFreq ( double ) ), this, SLOT ( setOurRxFrequency ( double ) ) );
	connect ( ourHamlibWrapper, SIGNAL ( rigChangedMode ( rmode_t, bool ) ), this, SLOT ( setMode ( rmode_t, bool ) ) );
	connect ( ourHamlibWrapper, SIGNAL ( slopeLowChangedByRig ( int ) ), this, SLOT ( setSlopeLowOffset ( int ) ) );
	connect ( ourHamlibWrapper, SIGNAL ( slopeHighChangedByRig ( int ) ), this, SLOT ( setSlopeHighOffset ( int ) ) );
	connect ( ourHamlibWrapper, SIGNAL ( rigPitch ( int ) ), this, SLOT ( setCWPitch ( int ) ) );
	
	if ( ourHamlibWrapper->init ( rig, port, speed ) != RIG_OK )
	{
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

