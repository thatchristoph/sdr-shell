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

   echo "Trapped $1"

   DTTSP_PID=`cat $VARRUN/sdr-core.pid`
   echo "Killing....... sdr-core ($DTTSP_PID)"
   kill  $DTTSP_PID

   JACKD_PID=`cat $VARRUN/jackd.pid`
   echo "Killing....... jackd ($JACKD_PID)"
   kill  $JACKD_PID


   PMSDR_PID=`cat $VARRUN/pmsdr.pid`
   echo "Killing....... pmsdr ($PMSDR_PID)"
   kill  $PMSDR_PID


   if [ ! -z "$HW_KNOB_FIFO" ]; then
      kill "$HW_KNOB_FIFO_PROGRAM_PID"
      rm -f "$HW_KNOB_FIFO"
   fi

   rm -f $PMSDR_CMDPATH

}

trap "trapped EXIT" EXIT
trap "trapped INT " INT
trap "trapped KILL" KILL

#
# Make reliable jackd - sdr-core interconnection
#
function make_connection {

   for ((i=0; i<5; i++)) {

      echo "Connecting $1 to $2 ($i)...."
      res=$($JACKC $1 $2)
      rc=$?
      if [ $rc == 0 ]; then
         return 0
      fi
      sleep 1
   }
   echo "jackd connection $1 to $2 failed after $i attempt: $res"
   exit 1
}


#
# Read the local configuration
#
. ./sdr-shell.conf


#
# Detect the sound card id from name
#

ALSAH_ID=$(aplay -l | grep "^card.*$ALSAH_NAME" | cut -f 1 -d: | cut -f 2 -d' ' | sort | uniq)

if [ -z "$ALSAH_ID" ]; then
   echo "Can't find soundcard $ALSAH_NAME"
   echo "Check your config"
   aplay -l
   exit 1
fi

ALSAH="hw:$ALSAH_ID"

echo "ALSA Sound card id: $ALSAH" 
echo "............................."

#
# Sound Card Sampling Rate
#
if [ -z "$DEFRATE" ]; then
   echo "Can't find DEFRATE definition."
   exit 1
fi

export SDR_DEFRATE="$DEFRATE"


JACKD_PARAM=" -r -dalsa -d$ALSAH -r$SDR_DEFRATE "


##############################################################################

VARRUN=/tmp/

##############################################################################
# Setup the PMSDR environment variables
export PMSDR_CMDPATH=/tmp/PMSDRcommands

##############################################################################
# Test if we have the needed executables and directories
if [ ! -x $JACKD ]; then
  echo "Can't find $JACKD executable."
  exit 1
fi

if [ ! -d $DTTSP ]; then 
  echo "Can't find directory $DTTSP"
  exit 1
fi

DTTSP_EXEC=$DTTSP/sdr-core

if [ ! -x $DTTSP_EXEC ]; then 
  echo "Can't find $DTTSP_EXEC executable."
  exit 1
fi

if [ ! -x $JACKC ]; then
  echo "Can't find $JACKC"
  exit 1
fi

if [ ! -x $PMSDR ]; then
  echo "Can't find $PMSDR executable."
  exit 1
fi

# If we have RT capability, get the realtime module ready
if [ $REALTIME ]; then
  echo "Configuring realtime module"
  JACKD="$JACKD -R "
  rmmod capability
  rmmod commoncap
  modprobe realcap any=1 allcaps=1
fi
  

##########################################################################
# Create FIFOs if needed
#

if [ ! -p $PMSDR_CMDPATH ]; then
   mkfifo $PMSDR_CMDPATH
fi

##########################################################################
# Sanity check
#
if [ ! -p $PMSDR_CMDPATH ]; then
   echo "Error while creating $PMSDR_CMDPATH fifo"
   exit 1
fi

##########################################################################
# Make sure jack and dttsp are not already running
TMP=`ps -ef | grep jackd | grep -v grep | wc -l`
if [ ! $TMP == 0 ]; then
  echo "jackd is already running. Stopping it..."
  killall jackd
fi

TMP=`ps -ef | grep sdr-core | grep -v grep | wc -l`
if [ ! $TMP == 0 ]; then
  echo "sdr-core is already running. Stopping it..."
  killall sdr-core
fi

##########################################################################
# Start jackd
echo ">>>> Starting jack: $JACKD $JACKD_PARAM $JACKD_CUSTOM_PARAM ."

pasuspender -- $JACKD $JACKD_PARAM $JACKD_CUSTOM_PARAM &
#$JACKD $JACKD_PARAM $JACKD_CUSTOM_PARAM &

JACKD_PID=$!
if [ $JACKD_PID ] 
then
  echo $JACKD_PID > $VARRUN/jackd.pid
  echo "  Succeeded. JackD PID is $JACKD_PID"
  echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
else
  echo "  Failed"
  exit 1
fi

##########################################################################
# 
# wait for jackd startup 
#
sleep 2

##########################################################################
# Start dttsp

prev_dir=`pwd`

echo ">>>> Starting dttsp: $DTTSP_EXEC $DTTSP_PARAM..."
cd $DTTSP
$DTTSP_EXEC $DTTSP_PARAM &
DTTSP_RC=$?
DTTSP_PID=$!

if [ $DTTSP_RC == 0 ] 
then
   echo "$DTTSP_RC"
   if [ $DTTSP_PID ] 
   then
     echo $DTTSP_PID > $VARRUN/sdr-core.pid
     echo "  Succeeded. DttSP PID is $DTTSP_PID"
     echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
   else
     echo "  Failed to start $DTTSP_EXEC"
     exit 1
   fi
else
   echo "  Failed"
   exit 1
fi 

cd $prev_dir



##########################################################################
# Start the PMSDR
#
echo "> Start the pmsdr.... $PMSDR"
$PMSDR &
PMSDR_PID=$!
if [ $PMSDR_PID ] 
then
   echo $PMSDR_PID > $VARRUN/pmsdr.pid
   echo "  Succeeded. pmsdr PID is $PMSDR_PID"
else
   echo "  Failed"
   exit 1
fi

##########################################################################
# Make sure that pmsdr is running
#
TMP=`killall -CONT pmsdr ; echo $?`
if [ $TMP != 0 ]; then
   echo "pmsdr not running. Stopping..." | gmessage -center -timeout 5 -file -
   exit 1
fi



##########################################################################
#
# Connect Dttsp to jack ports
#
echo ">>>> Connecting dttsp to jack..."

make_connection sdr-$DTTSP_PID:ol  alsa_pcm:playback_1
make_connection sdr-$DTTSP_PID:or  alsa_pcm:playback_2

#
# Input channel reversed to comply with PMSDR 2.1
#
make_connection alsa_pcm:capture_1 sdr-$DTTSP_PID:il
make_connection alsa_pcm:capture_2 sdr-$DTTSP_PID:ir

##########################################################################
#
#
#
#
# Sound Card Sampling Rate
#
if [ ! -z "$HW_KNOB_FIFO" ]; then
   echo "Create FIFO for KNOB conrol"
   mkfifo $HW_KNOB_FIFO
   "$HW_KNOB_FIFO_PROGRAM" 90 > $HW_KNOB_FIFO & 
   HW_KNOB_FIFO_PROGRAM_PID=$!
fi



##########################################################################
#
# Start the real thing ....
#
$SDRSHELL

SDRSHELL_RC=$?

exit $SDRSHELL_RC

