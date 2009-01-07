#!/usr/bin/env bash
#
# sdr_shell.sh
#
# Synopsis: 	 The script starts jack, dttsp, connects the jack ports, starts sdr-shell
#
# Version:      $Revision$
#
# Author:	    Andrea Montefusco iw0hdv
#               Excerpted from Edson Pereira startup script
#
# Usage:        ./sdr-shell.sh
#

function trapped {

   JACKD_PID=`cat $VARRUN/sdr-core.pid`
   DTTSP_PID=`cat $VARRUN/jackd.pid`
   echo "Killing....... $JACKD_PID $DTTSP_PID"
   kill  $JACKD_PID $DTTSP_PID 

   rm -f $PMSDR_CMDPATH
   exit 0
}

trap "trapped" EXIT
trap "trapped" INT
trap "trapped" KILL



#
# Read the local configuration
#
. ./sdr-shell.conf


#
# Detect the sound card id from name
#

ALSAH_ID=$(aplay -l | grep "^card.*$ALSAH_NAME" | cut -f 1 -d: | cut -f 2 -d' ')

if [ -z "$ALSAH_ID" ]; then
   echo "Can't find soundcard $ALSAH_NAME"
   echo "Check your config"
   aplay -l
   exit 1
fi

ALSAH="hw:$ALSAH_ID"

echo "ALSA Sound card id: $ALSAH" 


#
# Sound Card Sampling Rate
#
if [ -z "$DEFRATE" ]; then
   echo "Can't find DEFRATE definition"
   exit 1
fi

export SDR_DEFRATE="$DEFRATE"


JACKD_PARAM=" -R -dalsa -d$ALSAH -r$SDR_DEFRATE "

##############################################################################

VARRUN=/tmp/

##############################################################################
# Setup the DttSP Environment Variables

# Path to DttSP command FIFO
export SDR_PARMPATH=/dev/shm/SDRcommands

# Path to DttSP meter FIFO
export SDR_METERPATH=/dev/shm/SDRmeter

# Path to DttSP spectrum FIFO
export SDR_SPECPATH=/dev/shm/SDRspectrum


##############################################################################
# Setup the PMSDR environment variables
export PMSDR_CMDPATH=/tmp/PMSDRcommands

##############################################################################
# Test if we have the needed executables and directories
if [ ! -x $JACKD ]; then
  echo "Can't find $JACKD"
  exit 1
fi

if [ ! -d $DTTSP ]; then 
  echo "Can't find $DTTSP"
  exit 1
fi

if [ ! -x $JACKC ]; then
  echo "Can't find $JACKC"
  exit 1
fi

# If we have RT capability, get the realtime module ready
if [ $REALTIME ]; then
  echo "Configuring realtime module"
  JACKD="$JACKDRT -R"
  rmmod capability
  rmmod commoncap
  modprobe realcap any=1 allcaps=1
fi
  

##########################################################################
# Create FIFOs if needed
if [ ! -p $SDR_PARMPATH ]; then
   mkfifo $SDR_PARMPATH
fi

if [ ! -p $SDR_METERPATH ]; then
   mkfifo $SDR_METERPATH
fi

if [ ! -p $SDR_SPECPATH ]; then
   mkfifo $SDR_SPECPATH
fi

if [ ! -p $PMSDR_CMDPATH ]; then
   mkfifo $PMSDR_CMDPATH
fi

##########################################################################
# Sanity check
if [ ! -p $SDR_PARMPATH ]; then
   echo "Error while creating $SDR_PARMPATH fifo"
   exit 1
fi

if [ ! -p $SDR_METERPATH ]; then
   echo "Error while creating $SDR_METERPATH fifo"
   exit 1
fi

if [ ! -p $SDR_SPECPATH ]; then
   echo "Error while creating $SDR_SPECPATH fifo"
   exit 1
fi

if [ ! -p $PMSDR_CMDPATH ]; then
   echo "Error while creating $PMSDR_CMDPATH fifo"
   exit 1
fi

##########################################################################
# Make sure jack and dttsp are not already running
TMP=`ps -ef | grep jackd | grep -v grep | wc -l`
if [ ! $TMP == 0 ]; then
  echo "jackd is already running. Stopping..."
  # exit 1
fi

TMP=`ps -ef | grep sdr-core | grep -v grep | wc -l`
if [ ! $TMP == 0 ]; then
  echo "sdr-core is already running. Stopping..."
  exit 1
fi

##########################################################################
# Start jackd
echo "> Starting jack: $JACKD $JACKD_PARAM $JACKD_CUSTOM_PARAM"

$JACKD $JACKD_PARAM $JACKD_CUSTOM_PARAM &

JACKD_PID=$!
if [ $JACKD_PID ] 
then
  echo $JACKD_PID > $VARRUN/jackd.pid
  echo "  Succeeded. JackD PID is $JACKD_PID"
else
  echo "  Failed"
  exit 1
fi

##########################################################################
# Needed in some systems
sleep 1

##########################################################################
# Start dttsp
echo "> Starting dttsp: $DTTSP/sdr-core $DTTSP_PARAM..."
cd $DTTSP
$DTTSP/sdr-core $DTTSP_PARAM &
DTTSP_RC=$?
DTTSP_PID=$!

if [ $DTTSP_RC == 0 ] 
then
   echo "$DTTSP_RC"
   if [ $DTTSP_PID ] 
   then
     echo $DTTSP_PID > $VARRUN/sdr-core.pid
     echo "  Succeeded. DttSP PID is $DTTSP_PID"
   else
     echo "  Failed to start $DTTSP/sdr-core"
     exit 1
   fi
else
   echo "  Failed"
   exit 1
fi 

##########################################################################
# Needed in some systems
sleep 1

##########################################################################
# Connect the jack ports
echo "> Connecting dttsp to jack..."
sleep 2
echo "  sdr-$DTTSP_PID:ol -> alsa_pcm:playback_1"
$JACKC sdr-$DTTSP_PID:ol alsa_pcm:playback_1
echo "  sdr-$DTTSP_PID:or -> alsa_pcm:playback_2"
$JACKC sdr-$DTTSP_PID:or alsa_pcm:playback_2
echo "  alsa_pcm:capture_1 -> sdr-$DTTSP_PID:il"
$JACKC alsa_pcm:capture_1 sdr-$DTTSP_PID:il
echo "  alsa_pcm:capture_2 -> sdr-$DTTSP_PID:ir"
$JACKC alsa_pcm:capture_2 sdr-$DTTSP_PID:ir

##########################################################################
# Start the PMSDR
echo "> Start the pmsdr.... $PMSDR"
$PMSDR &
PMSDR_PID=$!
if [ $PMSDR_PID ] 
then
  echo $PMSDR_PID > $VARRUN/pmsdr.pid
  echo "  Succeeded. pmsdr PID is $PMSDR_PID"
else
  echo "  Failed"
fi


##########################################################################
# Make sure that pmsdr is running
TMP=`ps -ef | grep pmsdr | grep -v grep | wc -l`
if [ $TMP == 0 ]; then
  echo "pmsdr not running. Stopping..."
  exit 1
fi


#
# Start the real thing ....
#
$SDRSHELL

SDRSHELL_RC=$?

exit $SDRSHELL_RC
