#if !defined __STATIONS_H__
#define      __STATIONS_H__

#include <qobject.h>
#include <qstring.h>
#include <qptrlist.h>


class Station: public QObject {
public:
   Station (QString &i, int f, QString &cc, int st, int et):
     id(i),
     countryCode(cc),
     fKHz(f),
     st(st), et(et)
   {}
   int     getFreq () const { return fKHz;          }
   QString getId ()   const { return id;            }
   QString getCc ()   const { return countryCode;   }

   int     getStartTime () const { return st; }
   int     getEndTime ()   const { return et; }

private:
   QString  id;
   QString  countryCode;
   int      fKHz;
   int      st, et;
};


class EibiStation {

public:
    EibiStation (const char *fileName = "eibi.csv");

    QPtrList<Station> getStationInFreqRange (int xFreqHz, unsigned int rangeInKHz = 6, int fTime = 1);
    int               getStationCount () const { return mapSf.size(); }
private:
    QMap<int, QPtrList<Station> > mapSf;
    QMap<QString, Station *>      mapSfn;
};

#endif

