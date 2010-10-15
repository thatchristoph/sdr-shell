/*

The code contained in this file has been excerpted from DttSP port-clients.h
and adapted to c++ by A.Montefusco - IW0HDV

Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 by Frank Brickle, AB2KT and Bob McGwier, N4HY

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#if !defined __DTTSP_H__
#define      __DTTSP_H__

#define DTTSP_PORT_CLIENT_COMMAND  19001
#define DTTSP_PORT_CLIENT_SPECTRUM 19002
#define DTTSP_PORT_CLIENT_METER    19003
#define USBSOFTROCK_CLIENT_COMMAND 19004
#define DTTSP_TX_PORT_CLIENT_COMMAND  19005

#define DTTSP_PORT_CLIENT_BUFSIZE  65536


struct sockaddr_in;

class DttSP {
    
protected:
    unsigned short      port;
    struct sockaddr_in *pSa;
    int clen, flags, sock;

    char  buff [DTTSP_PORT_CLIENT_BUFSIZE];
    int   size, used;


    DttSP (int port, int inbound);

    int send_command ( char *cmdstr) ;
public:
    virtual ~DttSP ();
};



class DttSPcmd: public DttSP {

public:
    DttSPcmd (int port = DTTSP_PORT_CLIENT_COMMAND):
        DttSP (port, 0) 
        {}

    ~DttSPcmd () {}

    int sendCommand ( const char *format, ... );
};


class DttSPmeter: public DttSP {

public:
    DttSPmeter (int port = DTTSP_PORT_CLIENT_METER):
        DttSP (port, 1) 
    {}

    ~DttSPmeter () {}

    int fetch (int *label, float *data, int npts);
};


class DttSPspectrum: public DttSP {

public:
    DttSPspectrum (int port = DTTSP_PORT_CLIENT_SPECTRUM):
        DttSP (port, 1) 
    {}

    ~DttSPspectrum () {}

    int fetch ( int *tick, int *label, float *data, int npts) ;
};

class USBSoftrockCmd: public DttSP {

	public:
    
    USBSoftrockCmd (int port = USBSOFTROCK_CLIENT_COMMAND):
	    DttSP (port, 0)
	    {}

    ~USBSoftrockCmd () {}

    int sendCommand ( const char *format, ... );
};

class DttSPTXcmd: public DttSP {

public:
    DttSPTXcmd (int port = DTTSP_TX_PORT_CLIENT_COMMAND):
        DttSP (port, 0) 
        {}

    ~DttSPTXcmd () {}

    int sendCommand ( const char *format, ... );
    void off();
    void on();
    void setPort ( const int newport );
};


#endif
