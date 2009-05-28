/****************************************************************************
** $Id: qt/desktop.cpp   3.3.7   edited Aug 31 2005 $
**
** Copyright (C) 2008 A.Montefusco.  All rights reserved.
**
** 
** 
**
*****************************************************************************/


#include <qapplication.h>
#include <qfile.h>

#include <qtextstream.h>

#include <qptrlist.h>
#include <qstringlist.h>

//#include <stdio.h>
#include <iostream>
#include <limits.h>

#include "stations.h"


EibiStation::EibiStation (const char *fileName)
{
    std::cerr    << "**************** " << __FUNCTION__ << std::endl;

    QFile file (fileName);
    if (!file.open(IO_ReadOnly)) {
       std::cerr << "**************** Unable to open file [" << fileName << "]" << std::endl ;
       return;
    }
    QTextStream in(&file);
    int n = 0;

    while (!in.atEnd()) {
       QString line = in.readLine();
   //    std::cout << line << std::endl;

       QStringList tokList = QStringList::split(";", line, TRUE);

    ///std::cout << (const char *)tokList[0] 
    ///          << " -- "
    ///          << (const char *)tokList[1] 
    ///          << " -- "
    ///          << (const char *)tokList[4] 
    ///          << std::endl;
       int fKHz; 

       if ( sscanf ((const char *)tokList[0], "%d", &fKHz ) == 1 ) {

          //
          // parse the time range
          //
          int st, et;
          if ( tokList[1] == 0 || sscanf ((const char *)tokList[1], "%d-%d", &st, &et ) != 2 ) {
              st = 0;
              et = 24;
          } else {
              st = st / 100;
              et = et / 100;
          }

          Station *ps =  new Station(tokList[4], fKHz, tokList[3], st, et);

          if ( mapSf.find (fKHz) == mapSf.end() ) {

              QPtrList<Station> qpl;
              qpl.append (ps);

              mapSf[fKHz] = qpl;

          } else {

              QMap<int, QPtrList<Station> >::iterator i = mapSf.find (fKHz);

              (*i).append (ps);

          }

          if ( mapSfn.contains(tokList[4]) )
             ; // do nothing because a record for a station with the same name and frequency is already registered
          else {
             //mapSf.insert (fKHz, ps);
             mapSfn.insert (tokList[4], ps );
          }          
          n++;
       }
    }
}

QPtrList<Station> EibiStation::getStationInFreqRange (int xFreq, unsigned int rangeInKHz, int fTime)
{
    //
    // xFreq it the input frequency
    //
    int newF = xFreq / 1000;   // freq in KHz
    int maxF = newF + rangeInKHz; 
    int minF = newF - rangeInKHz;
    

    QPtrList <Station> ll;
    int fDiff = INT_MAX;

    // search into the db for 

    for (int x = minF ; x <= maxF; ++x) {

        QMap<int, QPtrList<Station> >::Iterator i = mapSf.find(x);

        if ( i != mapSf.end()) {
            QPtrList<Station> sl = *i;

            Station *xl;

            for ( xl = sl.first() ; xl ; xl = sl.next() ) {

                if ( xl ) {
                   int currDiff = abs(newF - xl->getFreq());

                   std::cerr << xl->getFreq()      << " - " << xl->getId()      << " - " 
                             << xl->getStartTime() << " - " << xl->getEndTime() << std::endl;

                   if ( currDiff <= fDiff ) {
                      fDiff = currDiff;

                      if ( fTime ) {

                          // compute the current GMT time (hour)
                          time_t  rawtime;
                          tm     *ptm;

                          time ( &rawtime );

                          ptm = gmtime ( &rawtime );

                          if (ptm) {
                              if ( ( (ptm->tm_hour >= xl->getStartTime()) && (ptm->tm_hour <= xl->getEndTime()) ) ) {

                                  std::cerr << xl->getFreq()      << " * " << xl->getId()      << " * " 
                                            << xl->getStartTime() << " * " << xl->getEndTime() << std::endl;

                                  ll.append (xl);
                              } 
                          }

                      }  else {
                          std::cerr << xl->getFreq() << " - " << xl->getId() << std::endl;
                          ll.append (xl);
                      }
                   }
                } else {
                   std::cout << "No station at " << x << std::endl;
                }

            }
        }

    }
    return ll;
}


#if defined __TEST_MODULE__

QMultiMap<int, Station *> mapSf;


//
// The program starts here.
//

int main( int argc, char **argv )
{
    QApplication app( argc, argv );

    EibiStation estat;

    std::cout << "Stations read: " << estat.getStationCount() << std::endl;
    
    while ( ! std::cin.eof() ) {
        int xFreq;  

        std::cin >> xFreq ;
        std::cout << xFreq << std::endl;

        QList<Station *> sl = estat.getStationinFreqRange (xFreq, 6);
        if (! sl.empty() ) {

          QListIterator<Station *> i(sl);
          while (i.hasNext()) {
             Station *ps = i.next();

             std::cout << ps->getFreq()
                       << " -- "
                       << (const char *) (ps->getId().toLatin1())
                       << std::endl; 
  
          }
        }
    }
}

#endif

#if 0


//
// The program starts here.
//

int main( int argc, char **argv )
{
    QApplication app( argc, argv );


    QFile file("eibi.csv");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
       std::cerr << "Unable to open file" <<  std::endl ;
       return 1;
    }
    QTextStream in(&file);
    int n = 0;

    while (!in.atEnd()) {
       QString line = in.readLine();
       //std::cout << (const char *)line.toLatin1() << std::endl;

       QStringList list1 = line.split(";");

       std::cout << (const char *)list1[0].toLatin1() 
                 << " -- "
                 << (const char *)list1[1].toLatin1() 
                 << " -- "
                 << (const char *)list1[4].toLatin1() 
                 << std::endl;
       int fKHz; 

       if ( sscanf ((const char *)list1[0].toLatin1(), "%d", &fKHz ) == 1 ) {
          mapSf.insert(fKHz, new Station(list1[4], fKHz));
          n++;
       }
    }
    std::cout << "Stations read: " << n << std::endl;


    int xFreq;
    while ( ! std::cin.eof() ) {


    std::cin >> xFreq ;

    std::cout << xFreq << std::endl;

    //
    // xFreq it the input frequency
    //
    int newF = xFreq / 1000;   // freq in KHz
    int maxF = newF + 6; 
    int minF = newF - 6;
    

    QList <Station *> ll;
    int fDiff = INT_MAX;

    // search into the db for 

    for (int x = minF ; x <= maxF; ++x) {

        QList <Station *> xl = mapSf.values(x);

        if (! xl.empty()) {
           int currDiff = abs(newF - (xl.at(0))->getFreq());
           if ( currDiff < fDiff ) {
              fDiff = currDiff;

              for (int j=0; j<xl.size(); ++j) ll.insert (0, xl.at(j));
           }
        } else {
           std::cout << "No station at " << x << std::endl;
        }
    }

    if (! ll.empty() ) {

       QListIterator<Station *> i(ll);
       while (i.hasNext()) {
          Station *ps = i.next();

          std::cout << ps->getFreq()
                    << " -- "
                    << (const char *) (ps->getId().toLatin1())
                    << std::endl; 
  
       }
           

    }

    }

    

    return 0;
}

#endif

