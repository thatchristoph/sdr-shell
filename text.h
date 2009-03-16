
char *helptext = 
"<center> \
Table of Contents<br> \
----------------------------------<br> \
<br> \
S-Meter<br> \
VFO Frequency<br> \
Mode<br> \
Noise Reduction<br> \
Binaural Mode<br> \
Spectrum Mode<br> \
Spectrogram<br> \
Filter Control<br> \
Spectrum<br> \
Virtual VFOs<br> \
Automatic Gain Control (AGC)<br> \
Keystoke Chart<br> \
Mouse Commands<br> \
<br> \
----------------------------------</center><br> \
<br>\
<center><a name=\"S-Meter\">S-Meter</a><br> \
----------------------------------</center><br> \
<center>S-points for frequencies below 30 MHz:</center><br> \
<blockquote> \
<pre> \
Signal    Received  Received<br> \
Strength  Voltage   Power<br> \
--------  --------  --------<br> \
S1        0.20 uV   -121 dBm<br> \
S2        0.40 uV   -115 dBm<br> \
S3        0.79 uV   -109 dBm<br> \
S4         1.6 uV   -103 dBm<br> \
S5         3.2 uV    -97 dBm<br> \
S6         6.3 uV    -91 dBm<br> \
S7          13 uV    -85 dBm<br> \
S8          25 uV    -79 dBm<br> \
S9          50 uV    -73 dBm<br> \
S9+10dB    160 uV    -63 dBm<br> \
S9+20dB    500 uV    -53 dBm<br> \
S9+30dB    1.6 mV    -43 dBm<br> \
S9+40dB    5.0 mV    -33 dBm<br> \
S9+50dB     16 mV    -23 dBm<br> \
S9+60dB     50 mV    -13 dBm<br> \
----------------------------<br> \
</pre> \
</blockquote> \
<center> \
VFO Frequency<br> \
----------------------------------</center><br> \
The VFO frequency indicates the frequency of the Local Oscillator added \
to the SDR Numerical Controlled Oscilator (NCO). The Local Oscillator \
frequency can be set on the Configuration Frame.<br> \
<br> \
<center>Mode<br> \
----------------------------------</center><br> \
Left click on the mode label activates the mode.<br> \
<br> \
LSB: Lower Side Band<br> \
USB: Upper Side Band<br> \
DSB: Double Side Band<br> \
AM: Amplitude Modulation<br> \
CWL: Lower Side Band CW<br> \
CWU: Upper Side Band CW<br> \
SAM: Synchronous Amplitude Modulation<br> \
FM: Frequency Modulation<br> \
<br> \
<center>Noise Reduction<br> \
----------------------------------</center><br> \
NR: Noise Reduction<br> \
ANF: Automatic Notch Filter<br> \
NB: Noise Blanker<br> \
<br> \
<center>Binaural Mode<br> \
----------------------------------</center><br> \
The binaural mode introduces a phase delay between the left and right output \
audio channels. This creates a spacial sensation and in some circunstances \
can aid the comprehension of the received signal by the human brain.<br> \
<br> \
<center>Spectrum Mode<br> \
----------------------------------</center><br> \
The spectrum mode switch toggles the operation mode of the spectrum display. \
When enabled, the spectrum is constantly updated at a rate of 10 times per \
second. When disabled, right-clicking on the spectrogram frame, the stored \
spectrum at the mouse pointer is displayed.<br> \
<br> \
<center>Spectrogram<br> \
----------------------------------</center><br> \
The spectrogram displays the history of the received spectrum. Weak signals \
are displayed in dark blue and the color will become lighter as the signal \
strength increases. The color aperture for the spectrogram can be changed \
by using the keyboard keys \'z\', \'x\', \'c\', or \'v\'. The values for \
the upper and lower color apertures are displayed on the \'CA:\' label on \
the lower control frame.<br> \
<br> \
<center>Filter Control<br> \
----------------------------------</center><br> \
The filter control is located on the dark-red frame right below the \
spectrogram. The lower stopband can be selected by left-clicking on the \
filter control frame or using the \'u\' or \'i\' keyboard keys. The upper \
stop band can be selected by right-clicking on the filter frame or \
using the \'c\' or \'v\' keyboard keys<br> \
<br> \
<center>Spectrum<br> \
----------------------------------</center><br> \
The spectrum frame shows the spectrum near the received frequency. The upper \
scale is the audio frequency above and below the NCO. The vertical scale \
scale is the signal intensity in dBm. To calibrate the spectrum, please \
see the calibration section below.<br> \
<br> \
<center>Virtual VFOs<br> \
----------------------------------</center><br> \
The Virtual VFOs are located on the lower control frame and are labeled \
from \'F1\' to \'F8\'. In addition to the operating frequency, each Virtual \
VFO stores the mode, and filter settings. The frequency stored in each \
Virtual VFO can be displayed by moving the mouse cursor over each Virtual \
VFO cell.<br> \
<br> \
<center>AGC<br> \
----------------------------------</center><br> \
The AGC Control is located at the AGC label on the lower Control Frame. The \
AGC types are long (L), slow (S), medium (M), and fast (F). To select a type, \
right-click on the type letter.<br> \
<br> \
<center>Use as IF for Older Rig <br>\
----------------------------------</center><br> \
With a Softrock, you can connect into the IF of an older rig.   This software \
in conjunction with Hamlib, allows you to display the correct frequency on the \
computer.  You can use the DSP features described above with the exception that \
tuning by clicking on the spectrum will not change your frequency.  You can effect \
IF shift by using the arrow keys.  The left and right arrows shift the IF, the up and down \
arrows change the amount the IF is shifted.  The G key will center the IF again. Slope \
Tuning on the (Kenwood TS-850S anyway with a Softrock on the 455 KHz IF) is compensated \
for if you select it.  You can adjust \
the offsets for LSB and USB separately.  Mode selection can be made from the rig or the \
computer. <br> \
<br> \
<br> \
<center>Keystroke Chart <br> \
----------------------------------</center><br> \
<blockquote> \
<pre> \
Keystroke     Function<br> \
<br> \
comma         Preamp Off<br> \
period        Preamp Onn<br> \
Alt-H         Config Menus and Help Page <br> \
Down arrow    Decrease Tune Step Size<br> \
Up arrow      Increase Tune Step Size<br> \
Left arrow    Decrease frequency by Tune Step Size<br> \
Right arrow   Increase frequency by Tune Step Size<br> \
G             Return to Configured LO Frequency<br> \
j             Increase tuning frequency<br> \
k             Decrease tuning frequency<br> \
l             Increase tuning step<br> \
h             Decrease tuning step<br> \
u             Decrease lower limit of filter passband<br> \
i             Increase lower limit of filter passband<br> \
o             Decrease higher limit of filter passband<br> \
p             Increase higher limit of filter passband<br> \
q             Decrease IQ gain<br> \
w             Increase IQ gain<br> \
e             Decrease IQ phase<br> \
r             Increase IQ phase<br> \
z             Decrease spectrogram color aperture lower limit<br> \
x             Increase spectrogram color aperture lower limit<br> \
c             Decrease spectrogram color aperture higher limit<br> \
v             Increase spectrogram color aperture higher limit<br> \
ESC           Quit program<br> \
</pre> \
</blockquote> \
<br> \
<br> \
<br> \
<center>Mouse Commands <br> \
----------------------------------</center><br> \
<blockquote> \
<pre> \
Left click on upper spectrogram  Bring signal to passband<br> \
Left click + horizontal drag     Tune in 1 Hz steps<br> \
     on upper spectrogram<br> \
Right click + horizontal drag    Tune in 10 Hz steps<br> \
     on upper spectrogram<br> \
Middle click + horizontal drag   Tune in 100 Hz steps<br> \
     on upper spectrogram<br> \
Left click on NR label           Toggle Noise Reduction<br> \
Left click on ANF label          Toggle Automatic Notch Filter<br> \
Left click on NB label           Toggle Noise Blanker<br> \
Left click on BIN label          Tougle Binaural Audio output<br> \
Right click on F1-F8 labels      Read memory<br> \
Left click on F1-F8 labels       Write memory<br> \
Right click on CFG label         Open configuration window<br> \
</pre> \
</blockquote> \
<br> \
<br> \
";
