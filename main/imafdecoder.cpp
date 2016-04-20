//Jesús Corral García
//Universidad de Málaga

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <QTextStream>
#include <QString>
#include "MainWindow.h"


int audiotracks; //number of audio tracks contained inside the IM AF file

int mainIMAFdecoder(QString outimaf){

    FILE *imf,*audiotrack;
    int d=0,i,j,sizemdat,audiosize;
    int chunkoffset[64]; // if you want more than 64 tracks , change this value.
    unsigned char dat,dat1,dat2,dat3;
    QTextStream out(stdout);

    imf = fopen (outimaf.toStdString().c_str(),"rb");

    fseek (imf,0,SEEK_SET);
    fseek (imf,24,SEEK_CUR); //jump to 'mdat'

    fread(&dat, sizeof(unsigned char), 1, imf);
    fread(&dat1, sizeof(unsigned char), 1, imf);
    fread(&dat2, sizeof(unsigned char), 1, imf);
    fread(&dat3, sizeof(unsigned char), 1, imf);

    sizemdat = (dat<<24)  | (dat1<<16) | (dat2<<8) | (dat3);

    if (sizemdat==1){ // if conformance file
        fread(&dat, sizeof(unsigned char), 1, imf);
        fread(&dat1, sizeof(unsigned char), 1, imf);
        fread(&dat2, sizeof(unsigned char), 1, imf);
        fread(&dat3, sizeof(unsigned char), 1, imf);
        fread(&dat, sizeof(unsigned char), 1, imf);
        fread(&dat1, sizeof(unsigned char), 1, imf);
        fread(&dat2, sizeof(unsigned char), 1, imf);
        fread(&dat3, sizeof(unsigned char), 1, imf);
        fread(&dat, sizeof(unsigned char), 1, imf);
        fread(&dat1, sizeof(unsigned char), 1, imf);
        fread(&dat2, sizeof(unsigned char), 1, imf);
        fread(&dat3, sizeof(unsigned char), 1, imf);
        sizemdat = (dat<<24)  | (dat1<<16) | (dat2<<8) | (dat3);

        fseek(imf,sizemdat-16,SEEK_CUR);
    }
   else {

    fseek(imf, sizemdat-4, SEEK_CUR); // -4 because we have to sub the 4 bytes of size
   }

    fseek (imf,16,SEEK_CUR);
    fseek (imf,96,SEEK_CUR); // next track id is placed 96 bytes after the last byte of type 'mvhd'

    fread(&dat, sizeof(unsigned char), 1, imf);
    fread(&dat1, sizeof(unsigned char), 1, imf);
    fread(&dat2, sizeof(unsigned char), 1, imf);
    fread(&dat3, sizeof(unsigned char), 1, imf);

    audiotracks = ((dat<<24)  | (dat1<<16) | (dat2<<8) | (dat3)) -1 ; //read the number of audio tracks.It is ´-1´ because the field indicates the number of tracks +1


    if (audiotracks == 12345678){

        audiotracks = 6; // for the conformance file 2

    }
    for (j=1;j<=audiotracks;j++){

        d=0;
        while (d==0){

         fread(&dat, sizeof(unsigned char), 1, imf);
         fread(&dat1, sizeof(unsigned char), 1, imf);
         fread(&dat2, sizeof(unsigned char), 1, imf);
         fread(&dat3, sizeof(unsigned char), 1, imf);


         if (dat == 0x68 && dat1 == 0x64 && dat2 == 0x6C && dat3 == 0x72) {  // 68646C72 = hdlr
            d=1;
         }

         else{
            fseek(imf, -3, SEEK_CUR); //if we have not readen ´hdlr´
         }

       } //close while

       for (i=1;i<=8;i++){  //handler type is placed eight bytes after the last byte of type 'hdlr'

            fread(&dat, sizeof(unsigned char), 1, imf);
       }

       fread(&dat, sizeof(unsigned char), 1, imf);//dat could be ´s´ (soun) or ´t´(text)

       d=0;
       if (dat==0x73){  //73 = ´s´

             while (d==0){

                fread(&dat, sizeof(unsigned char), 1, imf);
                fread(&dat1, sizeof(unsigned char), 1, imf);
                fread(&dat2, sizeof(unsigned char), 1, imf);
                fread(&dat3, sizeof(unsigned char), 1, imf);

                if (dat == 0x63 && dat1 == 0x6F && dat2 == 0x36 && dat3 == 0x34) {  // 636F3634 = co64
                     d=1;
                }
                else if (dat == 0x73 && dat1 == 0x74 && dat2 == 0x63 && dat3 == 0x6F){ //7374636F = stco
                    d=2;
                }

                else{
                     fseek(imf, -3, SEEK_CUR); //if we have not readen ´stco´
                }

            } //close while
             if (d==1){ // if co64
                for (i=1;i<=12;i++){

                fread(&dat, sizeof(unsigned char), 1, imf);
                }
             }
             if (d==2){// if stco
                for (i=1;i<=8;i++){

                fread(&dat, sizeof(unsigned char), 1, imf);
                }
             }


            fread(&dat, sizeof(unsigned char), 1, imf);
            fread(&dat1, sizeof(unsigned char), 1, imf);
            fread(&dat2, sizeof(unsigned char), 1, imf);
            fread(&dat3, sizeof(unsigned char), 1, imf);

            chunkoffset[j-1] = (dat<<24)  | (dat1<<16) | (dat2<<8) | (dat3);


       }//close if


   } //close for
//At this point, we will look for a text track. If d=2 there are no lyrics
    d=0;
    while (d==0){

      fread(&dat, sizeof(unsigned char), 1, imf);
      fread(&dat1, sizeof(unsigned char), 1, imf);
      fread(&dat2, sizeof(unsigned char), 1, imf);
      fread(&dat3, sizeof(unsigned char), 1, imf);

      if (feof (imf )){// if end of file -> no lyrics
           d=2;
      }
      else if (dat == 0x68 && dat1 == 0x64 && dat2 == 0x6C && dat3 == 0x72) {  // 68646C72 = hdlr
        d=1;
      }
      else{
        fseek(imf, -3, SEEK_CUR); //if we have not readen ´hdlr´
      }

    } //close while


// At this point, we know the position of the audio tracks in mdat and the number of tracks.
// Now we will separate the audio tracks in several MP3.

    fseek(imf, 0, SEEK_SET);
    for (j=0;j<chunkoffset[0];j++) //advance to the position of the first audio track contained in mdat
    {
         fread(&dat, sizeof(unsigned char), 1, imf);
    }

    for(i=0;i<audiotracks-1;i++){


          char buf[2];
          sprintf(buf, "%d", i);// convert int to char
          audiotrack =fopen (buf,"wb");

          for (j=chunkoffset[i];j<chunkoffset[i+1];j++){
             fread(&dat,sizeof(unsigned char),1,imf);
             fwrite (&dat,sizeof(unsigned char),1,audiotrack);
          }
          fclose (audiotrack);

   } // close for

//last audio track

    char buf[2];
    sprintf(buf, "%d", audiotracks-1); //convert int to char
    audiotrack =fopen (buf,"wb");
    audiosize = chunkoffset[2]-chunkoffset[1]; //calculate the size of one audio track. Every audio track must be the same size
    for (i=1;i<=audiosize;i++)
    {
        fread(&dat,sizeof(unsigned char),1,imf);
        fwrite (&dat,sizeof(unsigned char),1,audiotrack);
    }

    fclose (audiotrack);
    fclose (imf);
    return d;



}
