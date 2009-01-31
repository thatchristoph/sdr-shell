Instruction to setup environment and compile prerequisite for sdr-shell-pmsdr
-----------------------------------------------------------------------------


Install DttSP as found in

http://code.google.com/p/sdr-shell/wiki/HowToInstallDttSPonLinuxUbuntu804

older release, like 38, doesn't have the automeke configure script.
Do you need to issue

make clean ; make staticlib ; make 



Install gmessage:

sudo apt-get install gmessage

the launch script needs it.


Install and compile sdr-shell:

cd ~
svn checkout http://sdr-shell.googlecode.com/svn/branches/pmsdr  
cd sdr-shell-pmsdr
make


If the make is complaining on libpng reference missing please append

-lpng 

option to Makefile line 

LIBS     = $(SUBLIBS) -L/usr/share/qt3/lib -L/usr/X11R6/lib -lqt-mt -lXext -lX11 -lm -lpthread


Configure the sdr-shell.conf file according to your local setup.

gedit sdr-shell.conf


Run 

./sdr.shell.sh


If you have no audio check the jack connection with

qjackctl

