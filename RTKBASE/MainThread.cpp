// RTKBASE is a GUI interface for RTKLIB made for the Raspberry pî and a touchscreen  
//   Copyright (C) 2016  
//   David ENSG PPMD-2016 (first rtkbase release)
//   Francklin N'guyen van <francklin2@wanadoo.fr>
//   Sylvain Poulain <sylvain.poulain@giscan.com>
//   Vladimir ENSG PPMD-2017 (editable configuration)
//   Saif Aati ENSG PPMD-2018 (post processing)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.



#include <QThread>
#include <QMutex>
#include "MainThread.h"
#include <stdlib.h>
#include "vt.h"
#include <iostream>
#include <fstream>
#include <string>
#include <QList>
#include <QTextStream>
#include <QFile>

using namespace std;

extern "C" int mainRTKlib(int argc, char **argv);
extern "C" int ouvreVT(int argc, char **argv);
extern "C" void affichestatus(vt_t *vt);
extern "C" void affichesat(vt_t *vt);
extern "C" void afficheNaviData(vt_t *vt);
extern "C" void afficheStream(vt_t *vt);
extern "C" int fermeVT();

 QString Proj_x;
 QString Proj_y;
 QString Proj_z;



MainThread ::MainThread(QObject* parent):
QThread(parent)
{
    stopped = false;

    // Chargement des paramètres de sauvegarde :
    QFile sauveOptionFile(QString("../RTKBASE/data/SauveOptions"));
    QTextStream flux(&sauveOptionFile);
    sauveOptionFile.open(QFile::ReadOnly|QFile::Truncate);
    QString line;
    QStringList liste;
    while(!flux.atEnd())
    {
        line = flux.readLine();
        qDebug()<<line;
        liste<<line;
    }
    sauveOptionFile.close();

    for(int i=0;i<liste.length();i++)
    {
        QStringList decomp = liste[i].remove("=").split(" ",QString::SkipEmptyParts);
        if(decomp[0]=="filepath")  _filePath = decomp[1];
        else if(decomp[0]=="pointname") _pointName = decomp[1];
        else if(decomp[0]=="nummeas") _numOfMeasures = decomp[1].toInt();
        else if(decomp[0]=="cyclen") _cycleLength = decomp[1].toFloat();
        else if(decomp[0]=="oldpoint") _addMeasures = decomp[1].toInt();
        else if(decomp[0]=="EPSG") _EPSG = decomp[1];
        else if(decomp[0]=="AddGeoid") _AddGeoid = decomp[1].toInt();
        else if(decomp[0]=="GeoidPath") _filePath_Geoid = decomp[1];

        else std::cout<<"Le paramètre <"<<decomp[0].toStdString()<<"> n'a pas été reconnu"<<std::endl;
    }
}

MainThread::~MainThread()
{
     exit();
     wait();
}
void MainThread::setArgcArgv(std::vector<string> args)
{
    argc=args.size();
    argv=new char*[argc];
    int i=0;
    for (auto &s : args)
    {
        char *arg = new char[s.size() + 1];
        copy(s.begin(), s.end(), arg);
        arg[s.size()] = '\0';
        argv[i]=arg;
        i++;
    }
}


void MainThread::afficheRtkrcvsatellite(int i)
{
    cout<<"le chiffre est le: "<<i<<endl;
    if (i==11) m_choix=2;
    if (i==12) m_choix=1;
    if (i==13) m_choix=3;
    if (i==14) affichesat(&vt);
    if (i==15) m_choix=5;
    if (i==16) m_choix=7;
}

void MainThread::stop()
{
    qDebug()<<"-----------------on est avant la fermeture de VT-------------------";
    fermeVT();
    qDebug()<<"-----------------on est après la fermeture de VT-------------------";

    /*problèmes de droits en écriture dans le dossier???*/

    /*create folder if folder doesn't exist*/
    QString Folder="PointsFiles";
    QDir dir(Folder);
    if(!dir.exists())
    {
        dir.mkpath(".");
    }

    QString path=QDir::currentPath();
    QFile::copy(NomFichier,path+"/"+Folder+"/"+NomFichier);
    QFile::remove(NomFichier);

    sleep(3);
    stopped = true; // après ne retourne pas dans le run()...
    emit etatFermetureVT(true);
}

void MainThread::run()
{
    while (!stopped)
    {
        vt.state=0;
        std::ofstream r("infosouverture.txt");

        ouvreVT(argc,argv);
        sleep(2);

        /*create file for saving last point*/
        std::ofstream p("essaisauvegarde.txt");

        while(1)
        {
        if (m_choix==1) etatstatut();
        if (m_choix==2) etatsatellite();
        if (m_choix==3) etatNaviData();
        if (m_choix==5) etatStream();
        if (m_choix==6) sauvegardedansfichier(_filePath,_pointName,_numOfMeasures,_cycleLength,_addMeasures,_EPSG,_AddGeoid,_filePath_Geoid);
        if (m_choix==7) stop();
        }

    }
    stopped = false;
    qDebug()<<"from MainThread: "<<currentThreadId();
}


void MainThread::etatsatellite()
{
    std::ofstream o("essaiprojet.txt");
    affichesat(&vt);

    /*Open, read line by line and then close file*/
    int i=1;
    QStringList list;
    QString fileName = "essaiprojet.txt";
    QFile fichier(fileName);
    fichier.open(QIODevice::ReadOnly | QIODevice::Text);
    /*Check if file is opened*/
    QTextStream flux(&fichier);
    QString ligne;
    while(! flux.atEnd())
    {
        ligne = flux.readLine();
        //traitement de la ligne
        qDebug()<<ligne;
        list<<ligne;
        i=i+1;
    }
    qDebug()<<"nombre de lignes = "<<i-1;
    qDebug()<<list;
    fichier.close();
    emit emitdonneesSatellites(list,i-1);

    sleep(1);
    remove("essaiprojet.txt");
}

void MainThread::etatstatut()
{
    /*create buffer file*/
    std::ofstream o("essaiprojet.txt");

    /*Launch function of rtkrcv.c relative to the solution to print*/
    affichestatus(&vt);

    /*Open, read line by line and then close file*/
    int i=1;
    QStringList list;
    QString fileName = "essaiprojet.txt";
    QFile fichier(fileName);
    fichier.open(QIODevice::ReadOnly | QIODevice::Text);
    //---------verifier ouverture fichier......
    QTextStream flux(&fichier);
    QString ligne;
    while(! flux.atEnd())
    {
        ligne = flux.readLine();
        //traitement de la ligne
        qDebug()<<ligne;
        list<<ligne;
        i=i+1;
    }
    fichier.close();

    /*Save last position. This position will be used in Base Mode*/
    std::ofstream q("sauvegardepourbase.txt");
    QFile fichierlastpositionforbase("sauvegardepourbase.txt");
    fichierlastpositionforbase.open(QIODevice::Append | QIODevice::Text);
    QTextStream out1(&fichierlastpositionforbase);
    //out1<<"essai";
    if(i>1)
    {
        out1<<list[4]<<endl;
        out1<<list[5]<<endl;
        out1<<list[6]<<endl;
    }
    fichierlastpositionforbase.close();

    emit emitdonneesStatus(list);
    sleep(1);// on constate que dans cette partie, le sleep bloque la GUI, ie on n'est plus vraiment dans un Thread mais dans une fonction normale...

    /*Delete File*/
    remove("essaiprojet.txt");

}

void MainThread::etatNaviData()
{
    /*Launch function of rtkrcv.c relative to the solution to print*/
    afficheNaviData(&vt);

    sleep(1);
}


void MainThread::etatStream()
{
    std::ofstream o("essaiprojet.txt");
    /*Launch function of rtkrcv.c relative to the solution to print*/
    afficheStream(&vt);

    /*Open, read line by line and then close file*/
    int i=1;
    QStringList list;
    QString fileName = "essaiprojet.txt";
    QFile fichier(fileName);
    fichier.open(QIODevice::ReadOnly | QIODevice::Text);
    //---------verifier ouverture fichier......
    QTextStream flux(&fichier);
    QString ligne;
    while(! flux.atEnd())
    {
        ligne = flux.readLine();
        //traitement de la ligne
        list<<ligne;
        i=i+1;
    }
    fichier.close();
    emit emitdonneesStream(list,i-1);

    sleep(1);
    remove("essaiprojet.txt");
}

void MainThread::saveposition()
{
    qDebug()<<"le bouton save fonctionne";
    emit save(1);
    m_choix=6;
    //sauvegardedansfichier();
}

void MainThread::sauvegardedansfichier(QString filePath, QString pointName, int numOfMeasures, float cycleLength, bool addMeasures, QString EPSG , bool AddGeoid , QString filePath_Geoid)
{
    std::cout<<"Commençons la sauvegarde..."<<std::endl;
    if (addMeasures==false)
    {
        QFile(filePath).remove();
        m_k=0;
    }
    for(int k=0;k<numOfMeasures;k++)
    {
        QString pointNameCurrent;
        if(pointName==QString("")) pointNameCurrent=QString::number(m_k);
        else pointNameCurrent=pointName;
        /*------crée le fichier "tampon"------------------*/
        std::ofstream o("essaiprojet.txt");

        /*Launch function of rtkrcv.c relative to the solution to print*/
        affichestatus(&vt);

        /*Open, read line by line and then close file*/
        int i=1;
        QStringList list;
        QString fileName = "essaiprojet.txt";
        QFile fichier(fileName);
        fichier.open(QIODevice::ReadOnly | QIODevice::Text);
        /*check if file is opened*/
        QTextStream flux(&fichier);
        QString ligne;
        while(! flux.atEnd())
        {
            ligne = flux.readLine();
            //traitement de la ligne
            qDebug()<<ligne;
            list<<ligne;
            i=i+1;
        }
        fichier.close();
        //emit emitdonneesStatus(list);
        /*delete buffer file*/
        remove("essaiprojet.txt");
        /*----------------------------------------------*/


        QString str;
        if(m_k==0)
        {
            str=list[13];
            str.replace("/","");
            str.replace(" ","_");
            str.replace(":","");
            qDebug()<<str;
            QString z=str.mid(15);
            str.remove(z);// horaire type GDH: groupe/date/heure
            NomFichier=str+".txt";
            qDebug()<<NomFichier;
        }

  /*------Create a buffer file with WGS84 coord for cs2cs transform ------------------*/

        std::ofstream q("saveWGS84coordbuffer.txt");
          QFile WGS84coordbuffer("saveWGS84coordbuffer.txt");
          WGS84coordbuffer.open(QIODevice::Append | QIODevice::Text);
          QTextStream outWGS84(&WGS84coordbuffer);
       {
          QString X=list[4];
          X.replace(QString(","),QString("."));
                    QString Y=list[5];
          Y.replace(QString(","),QString("."));

          QString Z=list[6];
          Z.replace(QString(","),QString("."));

          outWGS84<<X<<" "<<Y<<" "<<Z<<'\n';


       WGS84coordbuffer.close();
   }

          /*-------------------------------------------------------------------------------/
                - Converte ECEF buffer file with cs2cs
          /------------------------------------------------------------------------------*/


QString EPSG1 = _EPSG;
QStringList EPSG2 = EPSG1.split(" ");
QString EPSG = EPSG2[0];
QString epsgout1 = _EPSG;
QString epsgout= epsgout1.right(4);
QString geoidpath1 = _filePath_Geoid;
QStringList geoidpath2 = geoidpath1.split("=");
QString geoidpath = geoidpath2[0];

              QProcess convert;

              QStringList param;

              if (epsgout=="4326")
              {
                if(AddGeoid==1)
                {
                param <<"+init=epsg:4326"<<"+to"<<("+init=epsg:"+epsgout)<<"+geoidgrids="+geoidpath<<"-f"<<"%.8f"<<"saveWGS84coordbuffer.txt";
                }
                else
                  {
                  param <<"+init=epsg:4326"<<"+to"<<("+init=epsg:"+epsgout)<<"-f"<<"%.8f"<<"saveWGS84coordbuffer.txt";
                  }
                }
              if ((epsgout=="2154")or(epsgout=="3942")or(epsgout=="3943")or(epsgout=="3944")or(epsgout=="3945")or(epsgout=="3946")or(epsgout=="3947")or(epsgout=="3948")or(epsgout=="3949")or(epsgout=="3950"))
              {
                  if(AddGeoid==1)
                  {
                  param <<"+init=epsg:4326"<<"+to"<<("+init=epsg:"+epsgout)<<"+geoidgrids="+geoidpath<<"saveWGS84coordbuffer.txt";
                  }
                  else
                    {
                  param <<"+init=epsg:4326"<<"+to"<<("+init=epsg:"+epsgout)<<"saveWGS84coordbuffer.txt";
                    }
                  }

               qDebug() << "param:" << param << "\n";

              convert.start("cs2cs", param);
              if (convert.waitForStarted())
              {
                 convert.waitForFinished();
                 QString output;
                 output = convert.readAllStandardOutput();

                 QString line = output;

                 qDebug() << "line:" << line << "\n";
                 QStringList list = line.split(QRegExp("\\s"));

                 double element;

                 for(int i = 0; i < list.size(); i++)
                 {
                     element = list.at(i).toDouble();
                     qDebug() << "element:" << element << "\n";
                 }


             Proj_x = (QString((list)[0]));
             Proj_y = (QString((list)[1]));
             Proj_z = (QString((list)[2]));

          }
    /*------------------------------------------------------------------------------*/



        //QFile fichiersauvegarde(NomFichier);
        QFile fichiersauvegarde(filePath);
        fichiersauvegarde.open(QIODevice::Append | QIODevice::Text);


        /*Save points in NomFichier in Folder*/

        QTextStream out(&fichiersauvegarde);


        if (epsgout=="4326")
         {
             out << "point  : "<<pointNameCurrent<<" mesure : "<<m_k<<" heure rover : "<<list[13]<<" latitude :"<<list[4] <<" longitude :"<<list[5]<<" hauteur :"<<list[6]<<" X : "<<list[1]<<" Y :"<<list[2]<<" Z : "<<list[3]<<" "<<EPSG<<" LAT : "<<Proj_y<<" LON : "<<Proj_x<<" ALT : "<<Proj_z<<'\n';
         }

        if ((epsgout=="2154")or(epsgout=="3942")or(epsgout=="3943")or(epsgout=="3944")or(epsgout=="3945")or(epsgout=="3946")or(epsgout=="3947")or(epsgout=="3948")or(epsgout=="3949")or(epsgout=="3950"))
        {
        out << "point  : "<<pointNameCurrent<<" mesure : "<<m_k<<" heure rover : "<<list[13]<<" latitude :"<<list[4] <<" longitude :"<<list[5]<<" hauteur :"<<list[6]<<" X : "<<list[1]<<" Y :"<<list[2]<<" Z : "<<list[3]<<" "<<EPSG<<" X : "<<Proj_x<<" Y : "<<Proj_y<<" ALT : "<<Proj_z<<'\n';
        }


        emit savePointNbr(pointNameCurrent.append(" ").append(QString::number(m_k)));
        fichiersauvegarde.close();

        sleep(cycleLength);

        m_k=m_k+1;
    }
    m_choix=1;//pour retour dans la boucle du Thread
    std::cout<<"On est sorti de la boucle, mais le pire est à venir !"<<std::endl;


}

void MainThread::changeSaveOptions(QStringList options)
{
    _filePath = options[0];
    if (_pointName!=options[1]) m_k=0;
    _pointName = options[1];
     _numOfMeasures = options[2].toInt();
    _cycleLength = options[3].toFloat();
    _addMeasures = options[4].toInt();
    _EPSG = options[5];
    _AddGeoid = options[6].toInt();
    _filePath_Geoid = options[7];
}

void MainThread::setSYStime()
{
    std::ofstream o("essaiprojet.txt");

    /*Launch function of rtkrcv.c relative to the solution to print*/
    affichestatus(&vt);

    /*Open, read line by line and then close file*/
    int i=1;
    QStringList list;
    QString fileName = "essaiprojet.txt";
    QFile fichier(fileName);
    fichier.open(QIODevice::ReadOnly | QIODevice::Text);
    //---------verifier ouverture fichier......
    QTextStream flux(&fichier);
    QString ligne;
    while(! flux.atEnd())
    {
        ligne = flux.readLine();
        //traitement de la ligne
        qDebug()<<ligne;
        list<<ligne;
        i=i+1;
    }
    fichier.close();

    emit (list);

    /*Delete File*/
    remove("essaiprojet.txt");



   /*Get date and time*/

     QString getGPSdatetime= list[13];
     getGPSdatetime.resize(19);


     QString getGPSdate= getGPSdatetime.left(10);
     getGPSdate.remove(QRegExp("[/]"));


     /*Format date and time to correct string for date command */

   QString getGPStime2 = getGPSdatetime;
   QString getGPStime= getGPStime2.right(8);


   /* Assemble string in good order */
//   getGPSmonthday.append(getGPStime).append(getGPSyear);
   getGPSdate.append(" ").append(getGPStime);

      /* Set Rasperry Pi system time */
       QProcess setPIdate;
       setPIdate.startDetached("sudo date +%Y%m%d%T -s \""+getGPSdate+"\"");

        /*Delete File*/
        remove("essaiprojet.txt");

}

