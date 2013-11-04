//***********************************************************//
//      Interactive Music Audio Format (IMAF) ENCODER		 //
//						Version 2.0							 //
//															 //
//                Eugenio Oñate Hospital                     //
//	   Jesús Corral García & Costantino Taglialatela          //
//															 //
//   Copyright (c) 2013 Centre for Digital Music (C4DM)		 //
//	Queen Mary University of London. All rights reserved.    //
//***********************************************************//
//						  main.c							 //
//***********************************************************//

//File input/output
#include <stdio.h>
//Standard library: numeric conversion, memory allocation...
#include <stdlib.h>
//Operations with strings
#include <string.h>
//Get the creation time: clock
#include <time.h>
#include "IMAFencoder.h"
//Qt libraries
#include <QTextStream>
#include <QMessageBox>
#include <QByteArray>


/*Prototype*/
void filetypebx(FileTypeBox *ftyp);
int mdatbox(MediaDataBox *mdat, int, FILE *imf, FILE *song, FILE *text, int, int, u32);
void moovheaderbox(MovieBox *moov, int, int, int, int, int, int, int);
int trackstructure(MovieBox *moov, int, int, int, int,const char *name);
int samplecontainer(MovieBox *moov, int, int, const char *name);
int sampledescription(MovieBox *moov, int);
int presetcontainer(MovieBox *moov, int, int *vol_values, int type, int fade_in);
int rulescontainer(MovieBox *moov, int SelRuleType, int SelRule_PAR1, int SelRule_PAR2,
                   int MixRuleType, int MixRule_PAR1, int MixRule_PAR2, int MixRule_PAR3, int MixRule_PAR4);
void writemoovbox(MovieBox moov, int numtrack,int totaltracks, FILE *imf, FILE *text);
int readTrack(MovieBox *moov, int,const char *name);

// Timed Text Functions
int trackstructure_text (MovieBox *moov, int numtrack, int clock, int durationTrack, int sizemdat,const char *textfile,FILE *text,int totaltracks);
int samplecontainer_text(MovieBox *moov, int numtrack, int sizemdat, const char *textfiles,int totaltracks);
int aux (FILE *text,int num,int num1,int num2,int num3,int *sal);
int getTextSize (FILE *text);
// Group and Preset functions
int groupcontainer(MovieBox *moov, int *group_tracks, int grp_vol, QString grp_name,
                   QString grp_description, int totaltracks);
void writepresets(MovieBox moov, int numtrack,int totaltracks, FILE *imf); // NOT YET USED

// MetaData and JPEG Image functions
int metacontainer (MetaBox *meta);
u32 getImageSize(const char *imag);
void insertImage (MetaBox *meta, FILE **imf, u32 imagesize, const char *imagedir);
void writemetabox(MetaBox meta, FILE *imf);

int bytereverse(int num);
int bytereverse16(int num);
int size_phrase[1000];//the total number of bytes in a phrase including modifiers
int phrases;//the number of phrases in the whole text


// Qt Widget for the "Exit Window"
class ExitWindow : public QWidget
{
 public:
     ExitWindow();
 private:
     QMessageBox *file_created;
};

ExitWindow::ExitWindow()
{
    file_created = new QMessageBox();
    file_created->setFixedSize(400,200); // doesn't seem to work
    file_created->setWindowTitle("IM AF Encoder");
    file_created->setText("IM AF file successfully created!");
    file_created->show();
}

int mainIMAFencoder (int totaltracks, QString files_path[maxtracks],QString outimaf,
                     QString picturefile, QString textfile, int vol_values[maxtracks], bool HasImageFile,
                     int SelRuleType, int SelRule_PAR1, int SelRule_PAR2,
                     int MixRuleType, int MixRule_PAR1, int MixRule_PAR2, int MixRule_PAR3, int MixRule_PAR4,
                     int group_tracks[maxtracks], int group_volume, QString group_name, QString group_description,
                     int pres_type, int fade_in)
{
    //Variables
    FileTypeBox ftyp;
    MediaDataBox mdat;
    MovieBox moov;
    MetaBox meta;

    //Media Data Box - Contains the audio
    FILE *song; //MP3
    u32 sizeTRAK = 0;
    int numtr;
    //Output File
    FILE *imf; //IMA
    int numtrack, sizemdat, durationTrack;
    //Image
    u32 imagesize;
    QTextStream out (stdout);
    //Timed-Text
    FILE *text;
    int sizetext;
    const char *c_str1[8];//change this value to support more than 8 audio tracks
    const char *c_str2;

    //Groups, Presets, Rules and Metadata boxes sizes
    u32 sizeGRCO, sizePRCO, sizeRUCO, sizeMETA;

    /* Obtain current time as seconds elapsed since the Epoch. */
    time_t clock = time(NULL);


    //INPUT: Image
    if (HasImageFile){
        c_str2= picturefile.toStdString().c_str(); //convert QString to const char
        imagesize = getImageSize(c_str2);//calculate the size of the jpeg
        } else { //if there is no image, the size is 0
        imagesize = 0;
    }

    //INPUT: Timed-Text
    c_str2= textfile.toStdString().c_str(); //convert Qstring to const char
    text = fopen(c_str2, "rb");
    sizetext= getTextSize (text); //calculate the size of the text
    if((text)==NULL) {
            // do something
        }

    //Create OUTPUT file (.ima)
    imf = fopen (outimaf.toStdString().c_str(),"wb");

    //Define the File Type Box
    filetypebx(&ftyp);
    fwrite(&ftyp, sizeof(FileTypeBox),1, imf);
    //AUDIO
    //Put the tracks in Media Data Box
    for (numtr=0; numtr<totaltracks; numtr++) {
        c_str1[numtr]= files_path[numtr].toStdString().c_str(); //convert Qstring to const char
        song = fopen(c_str1[numtr], "rb");

        //Extract the samples from the audio file and the text
        sizemdat = mdatbox(&mdat, totaltracks, imf, song, text, numtr, sizetext, imagesize);//sizemdat is the size of one audio track

        fclose(song); //Close the audio file
    }//close for

    //For each track write track information
    durationTrack = (sizemdat*8)/128;

    for (numtrack = 0; numtrack < totaltracks; numtrack++) {
        c_str1[numtrack]= files_path[numtrack].toStdString().c_str(); //convert QString to const char
        sizeTRAK =  trackstructure(&moov, numtrack, clock, durationTrack,sizemdat,c_str1[numtrack])+ sizeTRAK;
    }
//At this point, the variable sizeTRAK will be the sum of the size of the box ´trak´ in all the audio tracks.
//The size of the box trak of the text track will be added after.

    if (HasImageFile){
        //Meta
        sizeMETA = metacontainer(&meta);

        //Read the image from the JPEG file and write it in the IMAF file
        c_str2= picturefile.toStdString().c_str(); //convert Qstring to const char
        insertImage(&meta, &imf, imagesize,c_str2);
    }

    //Groups
    sizeGRCO = groupcontainer(&moov, group_tracks, group_volume, group_name, group_description, totaltracks); //Creates the group, returns the size of the box

    //Presets
    sizePRCO = presetcontainer(&moov, totaltracks, vol_values, pres_type, fade_in); // Creates the preset, returns the size of the box.

    //Rules
    sizeRUCO = rulescontainer(&moov, SelRuleType, SelRule_PAR1, SelRule_PAR2,
                              MixRuleType, MixRule_PAR1, MixRule_PAR2, MixRule_PAR3, MixRule_PAR4); // Creates the rules, returns the size of the box.

    //Text track
    c_str2= textfile.toStdString().c_str(); //convert Qstring to const char
    sizeTRAK = trackstructure_text (&moov, numtrack, clock, durationTrack, sizemdat, c_str2, text, totaltracks) + sizeTRAK;

    //Movie Header - Overall declarations
    moovheaderbox(&moov, clock, sizeTRAK, sizePRCO, totaltracks, durationTrack, sizeRUCO, sizeGRCO); // -> enter sizeGRCO instead of 0

    //Writes the movie box into the file
    writemoovbox(moov, numtrack, totaltracks, imf, text);

    //Writes the meta box into the IMAF file
    if (HasImageFile){
        writemetabox(meta,imf);
    }

    //Close Files and show exit dialog window
    fclose(imf);
    fclose (text);
    ExitWindow *window = new ExitWindow();
}



void filetypebx(FileTypeBox *ftyp){
    int swap;

    swap = bytereverse(24);// 24 is the size of the box "ftyp"
    ftyp->size = swap;
    swap = bytereverse('ftyp');
    ftyp->type = swap;
    swap = bytereverse('im02');  // Conformance point 3 (see ISO/IEC 23000-12:2010/FDAM 1:2011(E))
    ftyp->major_brand = swap;
    ftyp->minor_version = 0;
    swap = bytereverse('im02');  // Conformance point 3 (see ISO/IEC 23000-12:2010/FDAM 1:2011(E))
    ftyp->compatible_brands[0] = swap;
    swap = bytereverse('isom');
    ftyp->compatible_brands[1] = swap;
}

int mdatbox(MediaDataBox *mdat, int totaltracks, FILE *imf, FILE *song, FILE *text, int numtr, int sizetext, u32 imagesize){

    int d=0, cnt=0, j, find = 0, sizestring = 0, i = 0,cnt2=0,highlight_end_time=0;
    int  dat = 0, dat1 = 0, dat2 = 0, dat3 = 0,k=0;
    u32 size = 0, swap, sizeMDAT = 0;
    unsigned char c1=0,c2=0,numwords=0,initposition[3000],endposition[3000];

    //Positionate the pointer at the end of the file to know the size of it
    fseek(song, 0, SEEK_END);
    size = ftell(song);
    //Positionate the pointer at first
    fseek(song, 0, SEEK_SET);

    initposition[0]=0; // this array saves the position of the first letter of a word in a phrase
    phrases=0;// this variable saves the number of phrases in the whole text

    //Find the header of the first frame (the beginning), when find it d=1 and jump out the loop.
    // The header is 32 bytes. We find in groups of 8 bytes
    // Contemplate all possible options of headers
    while (d == 0) {
        find = 0;
        fread(&dat, sizeof(unsigned char), 1, song);
        cnt++;

        if (dat == 0xFF) {
            cnt++;                                              // cnt : stores the position of the pointer.
            fread(&dat1, sizeof(unsigned char), 1, song);
            cnt++;
            fread(&dat2, sizeof(unsigned char), 1, song);
            cnt++;
            fread(&dat3, sizeof(unsigned char), 1, song);
            if (dat1 == 0xFB && dat2 == 146 && dat3 == 64 ) {
                find = 1;                                       // find: if the header is found
                d=1;                                            // d: jump out the loop
            }
            if (dat1 == 0xFB && dat2 == 146 && dat3 == 96 ) {
                d=1;
                find = 1;
            }
            if (dat1 == 0xFB && dat2 == 144 && dat3 == 64 ) {
                find = 1;
                d=1;
            }
            if (dat1 == 0xFB && dat2 == 144 && dat3 == 96 ) {
                find = 1;
                d=1;
            }
            if (dat1 == 0xFB && dat2 == 146 && dat3 == 100 ) {
                d=1;
                find = 1;
            }
            if (dat1 == 0xFB && dat2 == 144 && dat3 == 100 ) {
                find = 1;
                d=1;
            }
            if (dat1 == 0xFA && dat2 == 146 && dat3 == 64 ) {
                find = 1;
                d=1;
            }
            if (dat1 == 0xFA && dat2 == 146 && dat3 == 96 ) {
                d=1;
                find = 1;
            }
            if (dat1 == 0xFA && dat2 == 144 && dat3 == 64 ) {
                find = 1;
                d=1;
            }
            if (dat1 == 0xFA && dat2 == 144 && dat3 == 96 ) {
                find = 1;
                d=1;
            }
            if (dat1 == 0xFA && dat2 == 146 && dat3 == 100 ) {
                d=1;
                find = 1;
            }
            if (dat1 == 0xFA && dat2 == 144 && dat3 == 100 ) {
                find = 1;
                d=1;
            }
            if (find == 0) {
                fseek(song, -3, SEEK_CUR); // if 3 last bytes in the header are not correct
                cnt = cnt - 3;
            }
        } // close if
        if (cnt == size) { //if we have readen all the bytes in the audio file
            d = 1;
        }
    }//close while
    size =  size - (cnt - 4);       // Calculate the size of the samples. size = pos. end of file - pos. first header.
    if (numtr == 0) { //if it is the first audio track the code writes the mdat size
        sizeMDAT = size*totaltracks + 8 + sizetext + imagesize;    // size of the whole media box -> INCLUDING IMAGE and TEXT SIZE.The text size does not include modifiers
        swap = bytereverse(sizeMDAT);
        fwrite(&swap, sizeof(u32), 1, imf);
        swap = bytereverse('mdat');
        mdat->type = swap;
        fwrite(&mdat->type, sizeof(u32), 1, imf);
    }
    fseek(song, cnt - 4, SEEK_SET);
    for (j=0; j<size; j++) {                //read all the samples of one track and writes them in the IM AF file
        fread(&mdat->data, sizeof(char), 1, song);
        fwrite(&mdat->data, sizeof(char), 1, imf);
    }

    // copy the text in the 3gp to the ima and add the text modifiers

    cnt=0;// the total number of bytes of the whole text including modifiers
    fseek(text,0,SEEK_CUR);
    sizestring=0;//number of bytes of a phrase (without modifiers)
    initposition[0]=0;// this array saves the initial position of a word
    if(numtr==totaltracks-1){  // writes the text after the samples of all the audio tracks
        j=0;cnt=0;
        while(j<sizetext){ //this loop reads the whole text
            cnt2=0;//the total number of bytes of a phrase including the modifiers
            fread(&c1,sizeof(char),1,text);
            fread(&c2,sizeof(char),1,text);
            fwrite(&c1,sizeof(char),1,imf);//two bytes of sizestring
            fwrite(&c2,sizeof(char),1,imf);
            sizestring = (c1<<8) | (c2);//sizestring is the size of one phrase contained in 3gp
            j=j+2;
            cnt=cnt+2;
            cnt2=cnt2+2;
            phrases++;//the number of phrases in the whole text
            numwords=0;// the number of words in a phrase
            initposition[0]=0;//this array saves the first position of a word in a phrase
            endposition[0]=0;// this array saves the last position of a word in a phrase
            k=0;//the index for endposition array and initposition array
            for(i=0;i< sizestring;i++){
                fread(&mdat->data,sizeof(char),1,text);
                fwrite(&mdat->data,sizeof(char),1,imf);
                j++;
                cnt=cnt+1;
                cnt2=cnt2+1;
                if(mdat->data==0x20){ //a blank space separates two words. 0x20 = blank space

                    numwords++;
                    endposition[k]=i;
                    initposition[k+1]=i+1;
                    k++;
                }


            } //close for
            endposition[k]=sizestring-1;//saves the last position of the last word in a phrase
            numwords++;


    mdat->data=0x00;
    fwrite(&mdat->data,sizeof(char),1,imf);//hclr size
    fwrite(&mdat->data,sizeof(char),1,imf);//hclr size
    fwrite(&mdat->data,sizeof(char),1,imf);//hclr size
    mdat->data=0x0C;
    fwrite(&mdat->data,sizeof(char),1,imf); //hclr size
    fwrite("h",sizeof(char),1,imf);
    fwrite("c",sizeof(char),1,imf);
    fwrite("l",sizeof(char),1,imf);
    fwrite("r",sizeof(char),1,imf);
    mdat->data=0xFF;
    fwrite(&mdat->data,sizeof(char),1,imf);//highlight color rgba
    mdat->data=0x62;
    fwrite(&mdat->data,sizeof(char),1,imf);//highlight color rgba
    mdat->data=0x04;
    fwrite(&mdat->data,sizeof(char),1,imf);//highlight color rgba
    mdat->data=0xFF;
    fwrite(&mdat->data,sizeof(char),1,imf);//highlight color rgba
    mdat->data=0x00;
    fwrite(&mdat->data,sizeof(char),1,imf); //krok size
    mdat->data=0x00;
    fwrite(&mdat->data,sizeof(char),1,imf);//krok size
    mdat->data=0x00;
    fwrite(&mdat->data,sizeof(char),1,imf);//krok size
    mdat->data=14+(8*numwords);
    fwrite(&mdat->data,sizeof(char),1,imf); //krok size
    fwrite("k",sizeof(char),1,imf);
    fwrite("r",sizeof(char),1,imf);
    fwrite("o",sizeof(char),1,imf);
    fwrite("k",sizeof(char),1,imf);
    mdat->data=0x00;
    fwrite(&mdat->data,sizeof(char),1,imf);//highlight-start-time
    mdat->data=0x00;
    fwrite(&mdat->data,sizeof(char),1,imf);//highlight-start-time
    mdat->data=0x00;
    fwrite(&mdat->data,sizeof(char),1,imf);//highlight-start-time
    mdat->data=0x00;
    fwrite(&mdat->data,sizeof(char),1,imf);//highlight-start-time
    mdat->data=0x00;
    fwrite(&mdat->data,sizeof(char),1,imf);//entry-count
    mdat->data=numwords;
    fwrite(&mdat->data,sizeof(char),1,imf);//entry-count

    for(i=0;i<numwords;i++){

        mdat->data=0x00;
        fwrite(&mdat->data,sizeof(char),1,imf);//highlight-end-time
        mdat->data=0x00;
        fwrite(&mdat->data,sizeof(char),1,imf);//highlight-end-time


        if(i==numwords-1){ //if it is the last word in a phrase we put this value
            mdat->data=0xFF;
            fwrite(&mdat->data,sizeof(char),1,imf);//highlight-end-time
            mdat->data=0xFF;
            fwrite(&mdat->data,sizeof(char),1,imf);//highlight-end-time

        }

        else{   //if it is not the last word in a phrase

            highlight_end_time = (i+1)*(11/numwords);//change the value '11' in order to increase o decrease the speed of highlight
            mdat->data= highlight_end_time;
            fwrite(&mdat->data,sizeof(char),1,imf);//highlight-end-time
            mdat->data=0xFF;
            fwrite(&mdat->data,sizeof(char),1,imf);//highlight-end-time


        }

        mdat->data=0x00;
        fwrite(&mdat->data,sizeof(char),1,imf);//startcharoffset
        mdat->data=initposition[i];
        fwrite(&mdat->data,sizeof(char),1,imf);//startcharoffset
        mdat->data=0x00;
        fwrite(&mdat->data,sizeof(char),1,imf);//endcharoffset
        mdat->data=endposition[i]+1;
        fwrite(&mdat->data,sizeof(char),1,imf);//endcharoffset

    }//close for

    cnt=cnt+26+(numwords*8);//cnt is the number of bytes of the whole text including modifiers
    cnt2=cnt2+26+(numwords*8); //cnt2 is the number of bytes of a phrase including the modifiers

    size_phrase[phrases-1]=cnt2 ;

    } //close while

        sizeMDAT = size*totaltracks + 8 + cnt + imagesize; // size value must include image and text sizes
        swap = bytereverse(sizeMDAT);
        fseek(imf,-(sizeMDAT-imagesize),SEEK_CUR); //overwrittes sizeMDAT with the total size (Image is yet to be written in the file at this point)
        fwrite(&swap, sizeof(u32), 1, imf);
        fseek(imf,(sizeMDAT-imagesize)-4,SEEK_CUR);	// (Image is yet to be written in the file at this point)
    } // close if (numtr==totaltracks-1)

    fclose(song);

    return size;
}

int samplecontainer(MovieBox *moov, int numtrack, int sizemdat, const char *name){

    u32 sizeSTSD, sizeSTSZ, swap, num_samples, dat=0;
    u32 sizetime, sizeSTTS; //Time to Sample Box
    u32 sizeSTSC = 28;      //Sample to Chunck
    u32 sizeSTCO = 20;      //Chunck offset
    u32 sizeSTBL;   //Sample Table Box //

    //Sample Description Box//
    sizeSTSD = sampledescription(moov, numtrack);

    //Sample size box//
    swap = bytereverse('stsz');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleSizeBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleSizeBox.version = 0;
    //Read Track: Frame size and Decoder Times
    num_samples = readTrack(moov, numtrack, name);
    sizeSTSZ = num_samples*4 + 20;
    swap = bytereverse(sizeSTSZ);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleSizeBox.size = swap;

    //Time To Sample Box//
    sizetime = bytereverse(moov->TrackBox[numtrack].MediaBox.MediaInformationBox.
                          SampleTableBox.TimeToSampleBox.entry_count);
    sizeSTTS = 16 + sizetime*4*2;
    swap = bytereverse(sizeSTTS);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.size = swap;
    swap = bytereverse('stts');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.version = 0;

    //Sample To Chunk//
    swap = bytereverse(sizeSTSC);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.size = swap;
    swap = bytereverse('stsc');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.version = 0;
    swap = bytereverse(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.entry_count = swap;
    swap = bytereverse(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.first_chunk = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.samples_per_chunk = moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleSizeBox.sample_count;
    swap = bytereverse(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.sample_description_index = swap;

    //Chunk Offset Box//
    swap = bytereverse(sizeSTCO);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    ChunkOffsetBox.size = swap;
    swap = bytereverse('stco');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    ChunkOffsetBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    ChunkOffsetBox.version = 0;
    swap = bytereverse(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    ChunkOffsetBox.entry_count = swap;
    dat = 32 + sizemdat*numtrack;
    swap = bytereverse(dat);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    ChunkOffsetBox.chunk_offset[numtrack] = swap;

    //Sample Table Box //
    sizeSTBL = 8 + sizeSTSD + sizeSTSZ + sizeSTSC + sizeSTCO + sizeSTTS;
    swap = bytereverse(sizeSTBL);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.size = swap;
    swap = bytereverse('stbl');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.type =swap;

    return  sizeSTBL;
}

int sampledescription(MovieBox *moov, int numtrack){

    u32 swap, sizeESD = 35;

    u32 sizeMP4a;    //Audio Sample Entry//
    u32 sizeSTSD;   //Sample description box //


    swap = bytereverse(sizeESD);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.size = swap;
    swap = bytereverse('esds');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.version = 0;

    //ES Descriptor//
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.tag = 3;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.length = 21;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.ES_ID = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.mix = 0;

    //Decoder config descriptor//
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.
    DecoderConfigDescriptor.tag = 4;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.
    DecoderConfigDescriptor.length = 13;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.
    DecoderConfigDescriptor.objectProfileInd = 0x6B;
    swap = bytereverse(0x150036B0);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.
    DecoderConfigDescriptor.mix = swap;
    swap = bytereverse(128);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.
    DecoderConfigDescriptor.maxBitRate = swap;
    swap = bytereverse(128);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.
    DecoderConfigDescriptor.avgBitrate = swap;

    //SLConfig Descriptor//
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.
    SLConfigDescriptor.tag = 6;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.
    SLConfigDescriptor.length = 1;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.
    SLConfigDescriptor.predifined = 2;

    //Audio Sample Entry//
    sizeMP4a = 36 + sizeESD;
    swap = bytereverse(sizeMP4a);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.size = swap;
    swap = bytereverse('mp4a');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.type =swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.reserved[0] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.reserved[1] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.reserved[2] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.reserved[3] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.reserved[4] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.reserved[5] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.data_reference_index = bytereverse16(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.reserved2[0] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.reserved2[1] = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.channelcount = 512;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.samplesize = 4096; // 16 bits
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.reserved3 = 0;
    swap = 44100 << 16;
    swap = bytereverse(swap);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.AudioSampleEntry.samplerate = swap;

    //Sample description box //
    sizeSTSD = 16 + sizeMP4a;
    swap = bytereverse(sizeSTSD);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.size = swap;
    swap = bytereverse('stsd');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.version = 0;
    swap = bytereverse(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.entry_count = swap;

    return sizeSTSD;
}

int readTrack (MovieBox *moov, int numtrack,const char *name){

    int t=0,k=1, l =0;

    FILE *song;
    int d=0, cnt = 0, i=0, j=0, cnt2 = 0, find = 0, swap, num_entr = 0;
    int  dat = 0, dat1 = 0, dat2 = 0, dat3 = 0, num_frame = 0, end =0, pos = 0;
    u32 size[9000];

    //Open the audio file with the name introduced by the user
    song = fopen (name,"rb");
    if (song == NULL) {
        printf("Error opening input file\n");
        system("pause");
        exit(1);
    }
    //Calculate the size of the track
    fseek(song, 0, SEEK_END);
    end = ftell(song);
    fseek(song, 0, SEEK_SET);
    d=0, i=0;
    //Search for each frame one by one, and extratcs the information
    while (d == 0) {
        find = 0;
        fread(&dat, sizeof(unsigned char), 1, song);
        cnt++;

        if (dat == 0xFF) {
            cnt++;
            fread(&dat1, sizeof(unsigned char), 1, song);
            cnt++;
            fread(&dat2, sizeof(unsigned char), 1, song);
            cnt++;
            fread(&dat3, sizeof(unsigned char), 1, song);
            if (dat1 == 0xFB && dat2 == 146 && dat3 == 64 ) {
                pos = cnt - 4;                      //Pos of the beginning of the frame
                size[num_frame] = pos - cnt2;     //Size of one frame
                cnt2 = pos;                         //Pos of the next frame
                find = 1;
                num_frame ++;                     //Number of frames
            }
            if (dat1 == 0xFB && dat2 == 146 && dat3 == 96 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (dat1 == 0xFB && dat2 == 144 && dat3 == 64 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (dat1 == 0xFB && dat2 == 144 && dat3 == 96 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (dat1 == 0xFB && dat2 == 146 && dat3 == 100 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (dat1 == 0xFB && dat2 == 144 && dat3 == 100 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (dat1 == 0xFA && dat2 == 146 && dat3 == 64 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (dat1 == 0xFA && dat2 == 146 && dat3 == 96 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (dat1 == 0xFA && dat2 == 144 && dat3 == 64 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (dat1 == 0xFA && dat2 == 144 && dat3 == 96 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (dat1 == 0xFA && dat2 == 146 && dat3 == 100 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (dat1 == 0xFA && dat2 == 144 && dat3 == 100 ) {
                pos = cnt - 4;
                size[num_frame] = pos - cnt2;
                cnt2 = pos;
                find = 1;
                num_frame ++;
            }
            if (find == 0) { //In case it does not find the header.
                             //It keeps reading next data without jump any position
                fseek(song, -3, SEEK_CUR);
                cnt = cnt - 3;
            }
        }

        if (cnt == end) {
            pos = cnt;
            size[num_frame] = pos - cnt2;
            d = 1;
        }
    }

    //Save Samples size//
    swap = bytereverse(num_frame);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleSizeBox.sample_count = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleSizeBox.sample_size = 0;

    for (i=0; i< num_frame; i++) {
        swap = bytereverse(size[i+1]);
        moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
        SampleSizeBox.entry_size[i] = swap;
    }

    //Save Decoding Times//
    //Writes manually the duration of each frame.
    //Follows the following structure:
    //  7 frames of 26 ms
    //  1 frame  of 27 ms
    //      ...
    // And each 13 rows it writes
    //  8 frames of 26 ms
    //  1 frame  of 27 ms
    //It is done for adjusting the different durations of each frame.
    //                  as they vary between 26.125 ms and 26.075 ms

    swap = bytereverse(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.sample_count[0] = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.sample_delta[0] =0;
    // int t=0,k=1, l =0;
    num_entr = 1;
    j = 0;
    for (i = 1; i< num_frame; i++) {
        if (j == 8 && l == 0) {
            swap = bytereverse(7);
            moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.sample_count[num_entr] = swap;
            swap = bytereverse(26);
            moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.sample_delta[num_entr] =swap;
            num_entr ++;

            swap = bytereverse(1);
            moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.sample_count[num_entr] = swap;
            swap = bytereverse(27);
            moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.sample_delta[num_entr] =swap;
            num_entr++;
            j=0;
            dat = i;
            if (k == 6 && t == 0) {
                l = 1;
                t = 1;
                k = 1;
            }
            if (k == 6 && t ==1) {
                l = 1;
                k = 1;
            }
            k++;
        }

        if (j == 9 && l == 1) {

            swap = bytereverse(8);
            moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.sample_count[num_entr] = swap;
            swap = bytereverse(26);
            moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.sample_delta[num_entr] =swap;
            num_entr ++;

            swap = bytereverse(1);
            moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.sample_count[num_entr] = swap;
            swap = bytereverse(27);
            moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.sample_delta[num_entr] =swap;
            num_entr++;
            j=0;
            dat = i;
            l = 0;
        }
        j++;
    }

    dat = num_frame - dat;

    swap = bytereverse(dat);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.sample_count[num_entr] = swap;
    swap = bytereverse(26);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.sample_delta[num_entr] =swap;
    num_entr++;
    swap = bytereverse(num_entr);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.entry_count = swap;

    fclose(song);
    return num_frame;

}

int trackstructure (MovieBox *moov, int numtrack, int clock,
                    int durationTrack, int sizemdat,const char *name){
    int swap;

    int sizeSTBL;  //Sample Table Box
    u32 sizeURL;   //Data Entry Url Box
    u32 sizeDREF;  //Data Reference
    u32 sizeSMHD;  //Sound Header Box
    u32 sizeDINF;  //Data information Box
    u32 sizeMINF;  //Media Information Box//
    u32 sizeHDLR;  //Handler Box//
    u32 sizeMDHD; //Media Header Box//
    u32 sizeMDIA; //Media Box//
    u32 sizeTKHD; //Track Header//
    u32 sizeTRAK; //Track container

    //Sample Table Box
    sizeSTBL = 0;
    sizeSTBL = samplecontainer(moov, numtrack,sizemdat, name);

    //Data Entry Url Box
    sizeURL = 12;
    swap = bytereverse(sizeURL);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.DataEntryUrlBox.size = swap;
    swap = bytereverse('url ');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.DataEntryUrlBox.type = swap;
    swap = bytereverse(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.DataEntryUrlBox.flags = swap; // =1 Track in same file as movie atom.

    //Data Reference
    sizeDREF = sizeURL+ 16;
    swap = bytereverse(sizeDREF);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.size = swap;
    swap = bytereverse('dref');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.flags = 0;
    swap = bytereverse(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.entry_count = swap;

    //Data information Box//
    sizeDINF = sizeDREF + 8;
    swap = bytereverse(sizeDINF);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.size = swap;
    swap = bytereverse('dinf');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.type = swap;

    //Sound Header Box //
    sizeSMHD = 16;
    swap = bytereverse(sizeSMHD);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SoundMediaHeaderBox.size = swap;
    swap = bytereverse('smhd');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SoundMediaHeaderBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SoundMediaHeaderBox.version = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SoundMediaHeaderBox.balance = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SoundMediaHeaderBox.reserved = 0;

    //Media Information Box//
    sizeMINF = sizeDINF + sizeSMHD + sizeSTBL + 8;
    swap = bytereverse(sizeMINF);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.size = swap;
    swap = bytereverse('minf');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.type = swap;

    //Handler Box//
    sizeHDLR = 37;
    swap = bytereverse(sizeHDLR);
    moov->TrackBox[numtrack].MediaBox.HandlerBox.size = swap;
    swap = bytereverse('hdlr');
    moov->TrackBox[numtrack].MediaBox.HandlerBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.version = 0;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.pre_defined = 0;
    swap = bytereverse('soun');
    moov->TrackBox[numtrack].MediaBox.HandlerBox.handler_type = swap;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.reserved[0] = 0;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.reserved[1] = 0;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.reserved[2] = 0;
    //swap = bytereverse('soun');
    //moov->TrackBox[numtrack].MediaBox.HandlerBox.data = swap;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.data[0] = 's';
    moov->TrackBox[numtrack].MediaBox.HandlerBox.data[1] = 'o';
    moov->TrackBox[numtrack].MediaBox.HandlerBox.data[2] = 'u';
    moov->TrackBox[numtrack].MediaBox.HandlerBox.data[3] = 'n';
    moov->TrackBox[numtrack].MediaBox.HandlerBox.data[4] = '\0';

    //Media Header Box//
    sizeMDHD = 32;
    swap = bytereverse(sizeMDHD);
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.size = swap;
    swap = bytereverse('mdhd');
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.version = 0;
    swap = bytereverse(clock);
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.creation_time = swap;
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.modification_time = swap;
    swap = bytereverse(1000);
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.timescale = swap;
    swap = bytereverse(durationTrack);
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.duration = swap;
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.language = 0xC455;
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.pre_defined = 0;

    //Media Box//
    sizeMDIA = sizeMDHD + sizeHDLR + sizeMINF + 8;
    swap = bytereverse(sizeMDIA);
    moov->TrackBox[numtrack].MediaBox.size = swap;
    swap = bytereverse('mdia');
    moov->TrackBox[numtrack].MediaBox.type = swap;

    //Track Header//
    sizeTKHD = 92;
    swap = bytereverse (sizeTKHD);
    moov->TrackBox[numtrack].TrackHeaderBox.size = swap;
    swap = bytereverse ('tkhd');
    moov->TrackBox[numtrack].TrackHeaderBox.type = swap ;
    swap = bytereverse (0x00000006);
    moov->TrackBox[numtrack].TrackHeaderBox.version = swap;
    swap = bytereverse (clock);
    moov->TrackBox[numtrack].TrackHeaderBox.creation_time = swap;
    moov->TrackBox[numtrack].TrackHeaderBox.modification_time = swap;
    swap = bytereverse (numtrack+1);
    moov->TrackBox[numtrack].TrackHeaderBox.track_ID = swap; //From 0x00000001 - 0x7FFFFFFF (dec 2147483647)
    moov->TrackBox[numtrack].TrackHeaderBox.reserved = 0;
    swap = bytereverse (durationTrack);
    moov->TrackBox[numtrack].TrackHeaderBox.duration = swap;
    moov->TrackBox[numtrack].TrackHeaderBox.reserved2[0] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.reserved2[1] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.layer = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.alternate_group = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.volume = 0x1;
    moov->TrackBox[numtrack].TrackHeaderBox.reserved3 = 0;
    swap = bytereverse (0x00010000);
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[0] = swap;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[1] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[2] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[3] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[4] = swap;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[5] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[6] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[7] = 0;
    swap = bytereverse(0x40000000);
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[8] = swap;
    moov->TrackBox[numtrack].TrackHeaderBox.width = 0; //just for video
    moov->TrackBox[numtrack].TrackHeaderBox.height = 0; //just for video

    //Track container
    sizeTRAK = sizeTKHD + sizeMDIA + 8;
    swap = bytereverse (sizeTRAK); // Size of one track
    moov->TrackBox[numtrack].size = swap;
    swap = bytereverse ('trak');
    moov->TrackBox[numtrack].type = swap;
    return sizeTRAK;

}

int groupcontainer(MovieBox *moov, int *group_tracks, int grp_vol, QString grp_name,
                   QString grp_description, int totaltracks) {

    int i, j, k, numgroups, sizeCont;
    int numel = 0;
    int sizeBox = 0;
    int tempsize = 0;
    int active, activenum, addsize;

// ADDED FOR SV
    for (j=0; j<totaltracks; j++){
        if (group_tracks[j]==1){
            numel++;
        }
    }

    if (numel==0){
        numgroups = 0;
      }else{
        numgroups = 1;
    }
//

    moov->GroupContainerBox.num_groups = bytereverse16(numgroups);

    for (i=0; i<numgroups; i++){
        addsize = 0;
        tempsize = 0;
        moov->GroupContainerBox.GroupBox[i].group_ID = bytereverse(2147483649+i+1); // group_ID shall be represented from 0x80000000 to 0xFFFFFFFF
        strcpy(moov->GroupContainerBox.GroupBox[i].group_name, grp_name.toStdString().c_str());
        strcpy(moov->GroupContainerBox.GroupBox[i].group_description, grp_description.toStdString().c_str());

//        numel = 0;   // uncomment for more than one group and remove initial "for" with numel
        k = 0;
        for (j=0; j<totaltracks; j++){
            if (group_tracks[j]==1){
                moov->GroupContainerBox.GroupBox[i].groupElemId[k].element_ID = bytereverse(j+1);
//                numel++;  // uncomment for more than one group and remove initial "for" with numel
                addsize += 4;
                k++;
            }
        }
        moov->GroupContainerBox.GroupBox[i].num_elements = bytereverse16(numel);

//        printf("Choose the activation mode: ");
//        printf("Activation mode\n");
//		printf(" - Switch on the MINIMUN number of elements (0, if no Min/Max rule) [0]\n");
//		printf(" - Switch on the MAXIMUM number of elements (All tracks, if no Min/Max rule) [1]\n");
//		printf(" - Switch on the defined number of elements (ONLY IF there is Min/Max rule) [2]\n");
//        scanf("%d",&active);
//        fflush(stdin);

        active = 1; // ADDED FOR SV, All tracks enabled

        moov->GroupContainerBox.GroupBox[i].group_activation_mode = active;
        activenum = 0;
        moov->GroupContainerBox.GroupBox[i].group_activation_elements_number = activenum;
        if (active==2){
            printf("Activation elements number: ");
            scanf("%d",&activenum);
            moov->GroupContainerBox.GroupBox[i].group_activation_elements_number = bytereverse16(activenum);
            addsize += 2;
        }

        moov->GroupContainerBox.GroupBox[i].group_reference_volume = bytereverse16(grp_vol*256/100); // define 8.8 fixed point

        tempsize = 75 + addsize;  // ---> before was 66 + addsize;
        moov->GroupContainerBox.GroupBox[i].size = bytereverse(tempsize);
        moov->GroupContainerBox.GroupBox[i].type = bytereverse('grup');
        moov->GroupContainerBox.GroupBox[i].version = bytereverse(0x02); // flags = Display enable, Edit disable

        sizeBox += tempsize;

    } // close for (numgroups)

    sizeCont = sizeBox + 10;

    moov->GroupContainerBox.size = bytereverse(sizeCont);
    moov->GroupContainerBox.type = bytereverse('grco');

    return sizeCont;
}

int presetcontainer(MovieBox *moov, int totaltracks, int *vol_values, int prestype, int fade_in){

    int temp, i, j, k, m, t, vol;
    int numpres, eq;
    //char name[50];
    u32 sizePRST = 0;
    u32 sizePRCO = 0;
    u32 sizeEQ;
    u32 addsize = 0;
    int updates = 0;

    numpres = 1; // ADDED FOR SV

     //Preset Box//
    for (i=0; i<numpres; i++) {
//        printf("\nPRESET LIST:\n");
//        printf(" - Static track volume [0]\n");
//        printf(" - Static object volume [1]\n");
//        printf(" - Dynamic track volume [2]\n");
//        printf(" - Dynamic object volume [3]\n");
//        printf(" - Dynamic track approximated volume [4]\n");
//        printf(" - Dynamic object approximated volume [5]\n");
//        printf(" - Static track volume with Equalization [6]\n"); // count preset_type from 8
//        printf(" - Static object volume with Equalization [7]\n");
//        printf(" - Dynamic track volume with Equalization [8]\n");
//        printf(" - Dynamic object volume with Equalization [9]\n");
//        printf(" - Dynamic track approximated with Equalization [10]\n");
//        printf(" - Dynamic object approximated with Equalization [11]\n");
//        printf("\nPlease make your choice: ");
//        scanf("%d", &prestype);
//        fflush(stdin);

        //PresetBOX
        // Box size specified in respective case
        moov->PresetContainerBox.PresetBox[i].type = bytereverse('prst');
        moov->PresetContainerBox.PresetBox[i].flags = bytereverse(0x02); // Display Enable Edit Disable
        moov->PresetContainerBox.PresetBox[i].preset_ID = i+1;
        moov->PresetContainerBox.PresetBox[i].num_preset_elements = totaltracks; // All the tracks are involved in the preset
        for (j=0; j<totaltracks; j++) {
            moov->PresetContainerBox.PresetBox[i].presElemId[j].preset_element_ID = bytereverse(j+1);
        }
        moov->PresetContainerBox.PresetBox[i].preset_global_volume = 100;

//        prestype = 0; // ADDED FOR SV

        switch (prestype) {
            case 0: moov->PresetContainerBox.PresetBox[i].preset_type = 0;
                    strcpy(moov->PresetContainerBox.PresetBox[i].preset_name, "Static track volume preset");

                    for (j=0; j<totaltracks; j++) {
                        vol = vol_values[j]*2;
                        moov->PresetContainerBox.PresetBox[i].StaticTrackVolume.presVolumElem[j].preset_volume_element = vol/2; //*0.02
                    }

                    addsize = totaltracks;
                    break;
            case 1: moov->PresetContainerBox.PresetBox[i].preset_type = 1;
                    strcpy(moov->PresetContainerBox.PresetBox[i].preset_name, "Static object volume preset");

                    for (j=0; j<totaltracks; j++) {
                        moov->PresetContainerBox.PresetBox[i].StaticObjectVolume.InputCH[j].num_input_channel = num_ch;
                    }
                    moov->PresetContainerBox.PresetBox[i].StaticObjectVolume.output_channel_type = 1; // STEREO (2 output channels)
                    for (j=0; j<totaltracks; j++){
                        for (k=0; k<num_ch; k++){
                            for (m=0; m<num_ch; m++){
                                moov->PresetContainerBox.PresetBox[i].StaticObjectVolume.presElVol[j].
                                    Input[k].Output[m].preset_volume_element = (100-(20*m))/2; // INPUT BY USER
                            }
                        }
                    }

                    addsize = totaltracks + 1+ totaltracks*num_ch*num_ch;
                    break;
            case 2: moov->PresetContainerBox.PresetBox[i].preset_type = 2;
                    strcpy(moov->PresetContainerBox.PresetBox[i].preset_name, "Dynamic track volume preset");

                    updates = 2; // volume changes
                    moov->PresetContainerBox.PresetBox[i].DynamicTrackVolume.num_updates = bytereverse16(updates);
                    for (j=0; j<updates; j++){ // INPUT BY USER
                        moov->PresetContainerBox.PresetBox[i].DynamicTrackVolume.
                            DynamicChange[j].updated_sample_number = bytereverse16(100+(j*100)); // *0.026 = time in seconds
                        for (k=0; k<totaltracks; k++){
                            moov->PresetContainerBox.PresetBox[i].DynamicTrackVolume.
                                DynamicChange[j].presVolumElem[k].preset_volume_element = (50*j)/2; // INPUT BY USER
                        }
                    }

                    addsize = 2 + updates*(2 + totaltracks);
                    break;
            case 3: moov->PresetContainerBox.PresetBox[i].preset_type = 3;
                    strcpy(moov->PresetContainerBox.PresetBox[i].preset_name, "Dynamic object volume preset");

                    updates = 2; // volume changes
                    moov->PresetContainerBox.PresetBox[i].DynamicObjectVolume.num_updates = bytereverse16(updates); // INPUT BY USER (maybe...)
                    for (j=0; j<totaltracks; j++) {
                        moov->PresetContainerBox.PresetBox[i].DynamicObjectVolume.InputCH[j].num_input_channel = 2;
                    }
                    moov->PresetContainerBox.PresetBox[i].DynamicObjectVolume.output_channel_type = 1; // STEREO (2 output channels)
                    for (j=0; j<updates; j++){ // INPUT BY USER
                        moov->PresetContainerBox.PresetBox[i].DynamicObjectVolume.
                            DynamicChange[j].updated_sample_number = bytereverse16(0+(j*500)); // *0.026 = time in seconds
                        for (k=0; k<totaltracks; k++){
                            for (m=0; m<num_ch; m++){
                                for (t=0; t<num_ch; t++){
                                    moov->PresetContainerBox.PresetBox[i].DynamicObjectVolume.DynamicChange[j].
                                        presElVol[k].Input[m].Output[t].preset_volume_element = (25*(j+1)); // INPUT BY USER
                                }
                            }
                        }
                    }

                    addsize = 2 + totaltracks + 1 + updates*(2 + totaltracks*num_ch*num_ch);
                    break;
            case 4: moov->PresetContainerBox.PresetBox[i].preset_type = 4;
                    strcpy(moov->PresetContainerBox.PresetBox[i].preset_name, "Dynamic track approximated volume");

                    updates = 2; // volume changes
                    moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.num_updates = bytereverse16(updates);
                    for (j=0; j<updates; j++){
                        moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                            DynamicChange[j].start_sample_number = bytereverse16(100); // *0.026 = time in seconds - INPUT BY USER
                        moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                            DynamicChange[j].duration_update = bytereverse16(500); // *0.026 = time in seconds -INPUT BY USER
                        for (k=0; k<totaltracks; k++){
                            if (fade_in){
                                moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.DynamicChange[j].
                                    presElVol[k].end_preset_volume_element = (50*j); // Fade IN
                              } else {
                                moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.DynamicChange[j].
                                    presElVol[k].end_preset_volume_element = (100-(100*j))/2; // Fade OUT
                            }
                        }
                    }

                    /* // some code for test
                    moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.DynamicChange[2].start_sample_number = bytereverse16(1100);
                    moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.DynamicChange[2].duration_update = bytereverse16(250);
                    moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.DynamicChange[2].presElVol[0].end_preset_volume_element = 50;
                    moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.DynamicChange[2].presElVol[1].end_preset_volume_element = 50;
                    moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.DynamicChange[3].start_sample_number = bytereverse16(1100);
                    moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.DynamicChange[3].duration_update = bytereverse16(250);
                    moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.DynamicChange[3].presElVol[0].end_preset_volume_element = 1;
                    moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.DynamicChange[3].presElVol[1].end_preset_volume_element = 1;
                    */

                    addsize = 2 + updates*(2 + 2 + totaltracks);
                    break;
            case 5: moov->PresetContainerBox.PresetBox[i].preset_type = 5; // NOT YET IMPLEMENTED INTO THE PLAYER!
                    strcpy(moov->PresetContainerBox.PresetBox[i].preset_name, "Dynamic object approximated volume");

                    printf("\n\nTHIS PRESET IS NOT YET IMPLEMENTED INTO THE PLAYER!!! PLAYER MAY CRASH WITH THIS FILE!!!\n\n");

                    updates = 2; // volume changes
                    moov->PresetContainerBox.PresetBox[i].DynamicObjectApproxVolume.num_updates = bytereverse16(updates);
                    for (j=0; j<totaltracks; j++) {
                        moov->PresetContainerBox.PresetBox[i].DynamicObjectApproxVolume.InputCH[j].num_input_channel = 2;
                    }
                    moov->PresetContainerBox.PresetBox[i].DynamicObjectApproxVolume.output_channel_type = 1; // STEREO (2 output channels)
                    for (j=0; j<updates; j++){
                        moov->PresetContainerBox.PresetBox[i].DynamicObjectApproxVolume.
                            DynamicChange[j].start_sample_number = bytereverse16(100); // *0.026 = time in seconds - INPUT BY USER
                        moov->PresetContainerBox.PresetBox[i].DynamicObjectApproxVolume.
                            DynamicChange[j].duration_update = bytereverse16(250); // *0.026 = time in seconds - INPUT BY USER
                        for (k=0; k<totaltracks; k++){
                            for (m=0; m<num_ch; m++){
                                for (t=0; t<num_ch; t++){
                                    moov->PresetContainerBox.PresetBox[i].DynamicObjectApproxVolume.DynamicChange[j].
                                        presElVol[k].Input[m].Output[t].preset_volume_element = (100*j)/2; // INPUT BY USER
                                }
                            }
                        }
                    }

                    addsize = 2 + totaltracks + 1 + updates*( 2 + 2 + totaltracks*num_ch*num_ch);
                    break;
            case 6: moov->PresetContainerBox.PresetBox[i].preset_type = 8;
                    strcpy(moov->PresetContainerBox.PresetBox[i].preset_name, "Static track volume with Equalization");

                    eq = 0;	addsize = 0;
                    for (j=0; j<totaltracks; j++) {
                        //printf("Enter volume for %s : ",namet[j].title);
                        scanf("%d",&vol);
                        fflush(stdin);
                        moov->PresetContainerBox.PresetBox[i].StaticTrackVolume.presVolumElem[j].preset_volume_element = vol/2; //*0.02

                        // Equalization
                        printf("EQ Filter on this element? [1] Yes - [0] No : ");
                        scanf("%d",&eq);
                        fflush(stdin);

                        if (eq == 1){
                            moov->PresetContainerBox.PresetBox[i].StaticTrackVolume.presVolumElem[j].EQ.num_eq_filters = 1;
                            for (k=0; k<moov->PresetContainerBox.PresetBox[i].StaticTrackVolume.presVolumElem[j].EQ.num_eq_filters; k++){
                                moov->PresetContainerBox.PresetBox[i].StaticTrackVolume.presVolumElem[j].EQ.Filter[k].filter_type = 4; // HPF
                                moov->PresetContainerBox.PresetBox[i].StaticTrackVolume.presVolumElem[j].EQ.Filter[k].
                                    filter_reference_frequency = bytereverse16(5000); // 10kHz
                                moov->PresetContainerBox.PresetBox[i].StaticTrackVolume.presVolumElem[j].EQ.Filter[k].filter_gain = -10;
                                moov->PresetContainerBox.PresetBox[i].StaticTrackVolume.presVolumElem[j].EQ.Filter[k].filter_bandwidth = 4;
                                addsize += 5;
                            } //close for
                        }else{
                            moov->PresetContainerBox.PresetBox[i].StaticTrackVolume.presVolumElem[j].EQ.num_eq_filters = 0;
                        } //close if/else
                    } //close for

                    addsize += totaltracks + totaltracks; //add preset_volume and num_eq_filters size
                    break;
            case 7: moov->PresetContainerBox.PresetBox[i].preset_type = 9; // NOT YET IMPLEMENTED INTO THE PLAYER!
                    strcpy(moov->PresetContainerBox.PresetBox[i].preset_name, "Static object volume with Equalization");
                    printf("\n\nTHIS PRESET IS NOT YET IMPLEMENTED INTO THE PLAYER!!! PLAYER MAY CRASH WITH THIS FILE!!!\n\n");
                    break;
            case 8: moov->PresetContainerBox.PresetBox[i].preset_type = 10; // NOT YET IMPLEMENTED INTO THE PLAYER!
                    strcpy(moov->PresetContainerBox.PresetBox[i].preset_name, "Dynamic track volume with Equalization");
                    printf("\n\nTHIS PRESET IS NOT YET IMPLEMENTED INTO THE PLAYER!!! PLAYER MAY CRASH WITH THIS FILE!!!\n\n");
                    break;
            case 9: moov->PresetContainerBox.PresetBox[i].preset_type = 11; // NOT YET IMPLEMENTED INTO THE PLAYER!
                    strcpy(moov->PresetContainerBox.PresetBox[i].preset_name, "Dynamic object volume with Equalization");
                    break;
            case 10: moov->PresetContainerBox.PresetBox[i].preset_type = 12; // NOT YET IMPLEMENTED INTO THE PLAYER!
                     strcpy(moov->PresetContainerBox.PresetBox[i].preset_name, "Dynamic track approximated with Equalization");
                     printf("\n\nTHIS PRESET IS NOT YET IMPLEMENTED INTO THE PLAYER!!! PLAYER MAY CRASH WITH THIS FILE!!!\n\n");

                     eq = 0; addsize = 0;
                     updates = 2; // volume changes
                     moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.num_updates = bytereverse16(updates);
                     for (j=0; j<updates; j++){
                         moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                             DynamicChange[j].start_sample_number = bytereverse16(100); // *0.026 = time in seconds - INPUT BY USER
                         moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                             DynamicChange[j].duration_update = bytereverse16(500); // *0.026 = time in seconds -INPUT BY USER
                         for (k=0; k<totaltracks; k++){
                             moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.DynamicChange[j].
                                 presElVol[k].end_preset_volume_element = (50*j); // Fade IN, change in (100-(100*j))/2 for Fade OUT -INPUT BY USER

                             // Equalization
                             //printf("EQ Filter on %s ?  [1] Yes - [0] No : ",namet[k].title);
                             scanf("%d",&eq);
                             fflush(stdin);

                             if (eq == 1){
                                 moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                                     DynamicChange[j].presElVol[k].EQ.num_eq_filters = 1;
                                 for (t=0; t<moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                                     DynamicChange[j].presElVol[k].EQ.num_eq_filters; t++){
                                         moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                                             DynamicChange[j].presElVol[k].EQ.Filter[t].filter_type = 4; // HPF
                                         moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                                             DynamicChange[j].presElVol[k].EQ.Filter[t].filter_reference_frequency = bytereverse16(5000); // 10kHz
                                         moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                                             DynamicChange[j].presElVol[k].EQ.Filter[t].end_filter_gain = -10;
                                         moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                                             DynamicChange[j].presElVol[k].EQ.Filter[t].filter_bandwidth = 4;
                                         addsize += 5;
                                 } //close for
                             }else{
                                 moov->PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                                     DynamicChange[j].presElVol[k].EQ.num_eq_filters = 0;
                             } //close if/else
                         } //close for (k)
                     } //close for (j)

                     addsize += 2 + updates*(2 + 2 + totaltracks*(1+1));
                     break;
            case 11: moov->PresetContainerBox.PresetBox[i].preset_type = 13; // NOT YET IMPLEMENTED INTO THE PLAYER!
                     strcpy(moov->PresetContainerBox.PresetBox[i].preset_name, "Dynamic object approximated with Equalization");
                     printf("\n\nTHIS PRESET IS NOT YET IMPLEMENTED INTO THE PLAYER!!! PLAYER MAY CRASH WITH THIS FILE!!!\n\n");
                     break;
            default: printf("ERROR in PRESET CONTAINER");
                     system("pause");
                     exit(1);
                     break;

        } //close switch

    temp = 16 + 50 + 4*totaltracks + addsize; // size PresetBox[i]
    moov->PresetContainerBox.PresetBox[i].size = bytereverse(temp);

    sizePRST += temp; // size of all Preset Boxes

    } //close for

    //Preset Container//
    sizePRCO += sizePRST + 10;
    moov->PresetContainerBox.size = bytereverse(sizePRCO);
    moov->PresetContainerBox.type = bytereverse('prco');
    moov->PresetContainerBox.default_preset_ID = 1; // Indicates initial preset activated.
    moov->PresetContainerBox.num_preset = numpres;

    return sizePRCO;

} //close function

int rulescontainer(MovieBox *moov, int SelRuleType, int SelRule_PAR1, int SelRule_PAR2,
                   int MixRuleType, int MixRule_PAR1, int MixRule_PAR2,
                   int MixRule_PAR3, int MixRule_PAR4) {

    int swap;
    u32 add_size = 0; //for SelectionRulesBox
    u32 sizeRUSC, sizeRUCO, sizeRUMX;


    //Selection Rules
    moov->RulesContainer.num_selection_rules = bytereverse16(1);
    moov->RulesContainer.SelectionRules.selection_rule_ID = bytereverse16(1);

    switch (SelRuleType) {

        case 0: moov->RulesContainer.SelectionRules.selection_rule_type = 0;
                moov->RulesContainer.SelectionRules.element_ID = bytereverse(2147483649+1); // Must be the same ID of the group
                moov->RulesContainer.SelectionRules.min_num_elements = bytereverse16(SelRule_PAR1);
                moov->RulesContainer.SelectionRules.max_num_elements = bytereverse16(SelRule_PAR2);
                strcpy(moov->RulesContainer.SelectionRules.rule_description,"Min/Max Rule");
                add_size = 4;
                break;
        case 1: moov->RulesContainer.SelectionRules.selection_rule_type = 1;
                moov->RulesContainer.SelectionRules.element_ID = bytereverse(SelRule_PAR1);
                moov->RulesContainer.SelectionRules.key_element_ID =bytereverse(SelRule_PAR2);
                strcpy(moov->RulesContainer.SelectionRules.rule_description,"Exclusion Rule");
                add_size = 4;
                break;
        case 2: moov->RulesContainer.SelectionRules.selection_rule_type = 2;
                moov->RulesContainer.SelectionRules.element_ID = bytereverse(SelRule_PAR1);
                strcpy(moov->RulesContainer.SelectionRules.rule_description,"Not mute Rule");
                add_size = 0;
                break;
        case 3: moov->RulesContainer.SelectionRules.selection_rule_type = 3;
                moov->RulesContainer.SelectionRules.element_ID = bytereverse(SelRule_PAR1);
                moov->RulesContainer.SelectionRules.key_element_ID =bytereverse(SelRule_PAR2);
                strcpy(moov->RulesContainer.SelectionRules.rule_description,"Implication Rule");
                add_size = 4;
                break;
        default: printf("ERROR in RULES CONTAINER/Selection Rules");
                 system("pause");
                 break;
    }

    sizeRUSC = 19 + 20 + add_size;
    moov->RulesContainer.SelectionRules.size = bytereverse(sizeRUSC);
    moov->RulesContainer.SelectionRules.type = bytereverse('rusc');
    moov->RulesContainer.SelectionRules.version = 0;

    //Mixing Rule
    moov->RulesContainer.num_mixing_rules = bytereverse16(1);
    moov->RulesContainer.MixingRules.mixing_rule_ID = bytereverse16(1);

    switch (MixRuleType) {

        case 0: moov->RulesContainer.MixingRules.mixing_type = 0;
                moov->RulesContainer.MixingRules.element_ID = bytereverse(MixRule_PAR1);
                moov->RulesContainer.MixingRules.key_elem_ID = bytereverse(MixRule_PAR2);
                strcpy(moov->RulesContainer.MixingRules.mix_description, "Equivalence rule");
                break;
        case 1: moov->RulesContainer.MixingRules.mixing_type = 1;
                moov->RulesContainer.MixingRules.element_ID = bytereverse(MixRule_PAR2);
                moov->RulesContainer.MixingRules.key_elem_ID = bytereverse(MixRule_PAR1);
                strcpy(moov->RulesContainer.MixingRules.mix_description, "Upper rule");
                break;
        case 2: moov->RulesContainer.MixingRules.mixing_type = 2;
                moov->RulesContainer.MixingRules.element_ID = bytereverse(MixRule_PAR2);
                moov->RulesContainer.MixingRules.key_elem_ID = bytereverse(MixRule_PAR1);
                strcpy(moov->RulesContainer.MixingRules.mix_description, "Lower rule");
                break;
        case 3: moov->RulesContainer.MixingRules.mixing_type = 3;
                moov->RulesContainer.MixingRules.element_ID = bytereverse(MixRule_PAR1);
                moov->RulesContainer.MixingRules.min_volume = bytereverse16(1 + MixRule_PAR3*2.5); // 8.8 fixed point
                moov->RulesContainer.MixingRules.max_volume = bytereverse16(1 + MixRule_PAR4*2.5); // 8.8 fixed point
                strcpy(moov->RulesContainer.MixingRules.mix_description, "Limit rule");
                break;
        default: printf("ERROR in RULES CONTAINER/Mixing Rules");
                 system("pause");
                 exit(1);
                 break;
    }

    sizeRUMX = 23 + 17;
    moov->RulesContainer.MixingRules.size = bytereverse(sizeRUMX);
    moov->RulesContainer.MixingRules.type = bytereverse('rumx');
    moov->RulesContainer.MixingRules.version = 0;

    //Rule container
    sizeRUCO = 12 + sizeRUSC + sizeRUMX;
    swap = bytereverse(sizeRUCO);
    moov->RulesContainer.size = swap;
    swap = bytereverse('ruco');
    moov->RulesContainer.type = swap;

    return sizeRUCO;

} // close function

void moovheaderbox (MovieBox *moov, int clock, int sizeTRAK, int sizePRCO, int totaltracks,
                    int durationTrack, int sizeRUCO, int sizeGRCO) {
    int swap;
    u32 sizeMOOV; //MovieBox

    //MovieHeader
    u32 sizeMVHD = 108;
    swap = bytereverse (sizeMVHD);
    moov->MovieHeaderBox.size = swap;
    swap = bytereverse ('mvhd');
    moov->MovieHeaderBox.type = swap;
    moov->MovieHeaderBox.version = 0;
    swap = bytereverse (clock);
    moov->MovieHeaderBox.creation_time = swap;
    moov->MovieHeaderBox.modification_time = swap;
    swap = bytereverse (1000);
    moov->MovieHeaderBox.timescale = swap;
    swap = bytereverse (durationTrack);
    moov->MovieHeaderBox.duration = swap;
    swap = bytereverse (0x00010000);
    moov->MovieHeaderBox.rate = swap;
    swap = bytereverse (1);
    moov->MovieHeaderBox.volume = 1;
    moov->MovieHeaderBox.reserved=0;
    moov->MovieHeaderBox.reserved2[0] = 0;
    moov->MovieHeaderBox.reserved2[1] = 0;
    swap = bytereverse (0x00010000);
    moov->MovieHeaderBox.matrix[0] = swap;
    moov->MovieHeaderBox.matrix[1] = 0;
    moov->MovieHeaderBox.matrix[2] = 0;
    moov->MovieHeaderBox.matrix[3] = 0;
    moov->MovieHeaderBox.matrix[4] = swap;
    moov->MovieHeaderBox.matrix[5] = 0;
    moov->MovieHeaderBox.matrix[6] = 0;
    moov->MovieHeaderBox.matrix[7] = 0;
    swap = bytereverse (0x40000000);
    moov->MovieHeaderBox.matrix[8] = 0x40000000;
    moov->MovieHeaderBox.pre_defined[0] = 0;
    moov->MovieHeaderBox.pre_defined[1] = 0;
    moov->MovieHeaderBox.pre_defined[2] = 0;
    moov->MovieHeaderBox.pre_defined[3] = 0;
    moov->MovieHeaderBox.pre_defined[4] = 0;
    moov->MovieHeaderBox.pre_defined[5] = 0;
    swap = bytereverse (totaltracks + 1);
    moov->MovieHeaderBox.next_track_ID = swap;

    //MovieBox
    sizeMOOV = sizeMVHD + sizeTRAK + sizePRCO + sizeRUCO + sizeGRCO + 8;
    swap = bytereverse (sizeMOOV); //Size movie: Taking into account number tracks
    moov->size = swap;
    swap = bytereverse ('moov');
    moov->type = swap;
}


void writemoovbox(MovieBox moov, int numtrack,int totaltracks, FILE *imf, FILE *text)
{
    int i, j, k, m, t, swap, pos, temp, type;
    int cnt = 0, cnt2 = 0, d = 0, dat = 0, dat1 = 0, dat2 = 0, dat3 = 0, size = 0;
    u16 numgroups, numel;


    //Write movie box//
    fwrite(&moov.size, sizeof(u32), 1, imf);
    fwrite(&moov.type, sizeof(u32), 1, imf);
    //Movie header//
    fwrite(&moov.MovieHeaderBox, sizeof(moov.MovieHeaderBox), 1, imf);
    //Track container//
    for (numtrack = 0; numtrack < totaltracks; numtrack++) {
        fwrite(&moov.TrackBox[numtrack].size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].type, sizeof(u32), 1, imf);
        //Trck header//
        fwrite(&moov.TrackBox[numtrack].TrackHeaderBox, sizeof(moov.TrackBox[numtrack].TrackHeaderBox), 1, imf);
        //Media Box//
        fwrite(&moov.TrackBox[numtrack].MediaBox.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.type, sizeof(u32), 1, imf);
        //Media Header//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaHeaderBox,
               sizeof(moov.TrackBox[numtrack].MediaBox.MediaHeaderBox), 1, imf);
        //Handler Box//
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.type, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.version, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.pre_defined, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.handler_type, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.reserved[0], sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.reserved[1], sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.reserved[2], sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.data[0], sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.data[1], sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.data[2], sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.data[3], sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.data[4], sizeof(unsigned char), 1, imf);
        //Media inforamtion box//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.type, sizeof(u32), 1, imf);
        //Sound media header//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SoundMediaHeaderBox,
               sizeof(moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SoundMediaHeaderBox), 1, imf);
        //Data reference//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox,
               sizeof(moov.TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox), 1, imf);
        //Sample table box//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.type, sizeof(u32), 1, imf);

        //Time to sample box//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               TimeToSampleBox.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               TimeToSampleBox.type, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               TimeToSampleBox.version, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               TimeToSampleBox.entry_count, sizeof(u32), 1, imf);

        swap = bytereverse(moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                          TimeToSampleBox.entry_count);
        pos = swap;

        for (i=0; i<pos; i++) {

            fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                   TimeToSampleBox.sample_count[i], sizeof(u32), 1, imf);
            fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                   TimeToSampleBox.sample_delta[i], sizeof(u32), 1, imf);
        }

        //Sample description box//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.type, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                SampleDescriptionBox.version, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                SampleDescriptionBox.entry_count, sizeof(u32), 1, imf);
        //Audio Sample entry//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                SampleDescriptionBox.AudioSampleEntry.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.type, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.reserved[0], sizeof(unsigned char), 6, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.data_reference_index, sizeof(u16), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.reserved2[0], sizeof(u32), 2, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.channelcount, sizeof(u16), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.samplesize, sizeof(u16), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.reserved3, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.samplerate, sizeof(u32), 1, imf);
        //ESDBox//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.type, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.version, sizeof(u32), 1, imf);
        //ES Descriptor//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.tag
               , sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.length
               , sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.ES_ID
               , sizeof(u16), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.mix
               , sizeof(unsigned char), 1, imf);
        //Decoder Config//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.DecoderConfigDescriptor.
               tag, sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.DecoderConfigDescriptor.
               length, sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.DecoderConfigDescriptor.
               objectProfileInd, sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.DecoderConfigDescriptor.
               mix, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.DecoderConfigDescriptor.
               maxBitRate, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.DecoderConfigDescriptor.
               avgBitrate, sizeof(u32), 1, imf);
/*        //DecoderSpecificInfo//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.DecoderConfigDescriptor.
               DecoderSpecificInfo.tag, sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.DecoderConfigDescriptor.
               DecoderSpecificInfo.length, sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.DecoderConfigDescriptor.
               DecoderSpecificInfo.decSpecificInfoData[0], sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.DecoderConfigDescriptor.
               DecoderSpecificInfo.decSpecificInfoData[1], sizeof(unsigned char), 1, imf);
  */      //SLConfig//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.SLConfigDescriptor.
               tag, sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.SLConfigDescriptor.
               length, sizeof(unsigned char), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleDescriptionBox.AudioSampleEntry.ESbox.ES_Descriptor.SLConfigDescriptor.
               predifined, sizeof(unsigned char), 1, imf);


        //Sample Size box//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleSizeBox.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleSizeBox.type, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleSizeBox.version, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleSizeBox.sample_size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleSizeBox.sample_count, sizeof(u32), 1, imf);
        swap = bytereverse(moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                          SampleSizeBox.sample_count);
        for(i=0; i<swap; i++){
            fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                   SampleSizeBox.entry_size[i], sizeof(u32), 1, imf);
        }

        //Sample to chunk box//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleToChunk.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleToChunk.type, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleToChunk.version, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleToChunk.entry_count, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleToChunk.first_chunk, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleToChunk.samples_per_chunk, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               SampleToChunk.sample_description_index, sizeof(u32), 1, imf);

        //Chunk offset//
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               ChunkOffsetBox.size, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               ChunkOffsetBox.type, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               ChunkOffsetBox.version, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               ChunkOffsetBox.entry_count, sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
               ChunkOffsetBox.chunk_offset[numtrack], sizeof(u32), 1, imf);
    }

    //Group Container
    fwrite(&moov.GroupContainerBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.GroupContainerBox.type, sizeof(u32), 1, imf);
    fwrite(&moov.GroupContainerBox.num_groups, sizeof(u16), 1, imf);
    //Group Box
    numgroups = bytereverse16(moov.GroupContainerBox.num_groups);
    for(i=0; i<numgroups; i++){
        fwrite(&moov.GroupContainerBox.GroupBox[i].size, sizeof(u32), 1, imf);
        fwrite(&moov.GroupContainerBox.GroupBox[i].type, sizeof(u32), 1, imf);
        fwrite(&moov.GroupContainerBox.GroupBox[i].version, sizeof(u32), 1, imf);
        fwrite(&moov.GroupContainerBox.GroupBox[i].group_ID, sizeof(u32), 1, imf);
        fwrite(&moov.GroupContainerBox.GroupBox[i].num_elements, sizeof(u16), 1, imf);
        numel = bytereverse16(moov.GroupContainerBox.GroupBox[i].num_elements);
        for(j=0; j<numel; j++){
            fwrite(&moov.GroupContainerBox.GroupBox[i].groupElemId[j].element_ID, sizeof(u32), 1, imf);
        }
        fwrite(&moov.GroupContainerBox.GroupBox[i].group_activation_mode, sizeof(unsigned char), 1, imf);
        if(moov.GroupContainerBox.GroupBox[i].group_activation_mode==2){
            fwrite(&moov.GroupContainerBox.GroupBox[i].group_activation_elements_number, sizeof(u16), 1, imf);
        }
        fwrite(&moov.GroupContainerBox.GroupBox[i].group_reference_volume, sizeof(u16), 1, imf);
        for (j=0; j<22; j++) {
            fwrite(&moov.GroupContainerBox.GroupBox[i].group_name[j], sizeof(char), 1, imf);
        }
        for (j=0; j<32; j++) {
            fwrite(&moov.GroupContainerBox.GroupBox[i].group_description[j], sizeof(char), 1, imf);
        }

    }

    //PresetContainerBox
    fwrite(&moov.PresetContainerBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.PresetContainerBox.type, sizeof(u32), 1, imf);
    fwrite(&moov.PresetContainerBox.num_preset, sizeof(unsigned char), 1, imf);
    fwrite(&moov.PresetContainerBox.default_preset_ID, sizeof(unsigned char), 1, imf);

    //Preset Box
    for (i=0; i<moov.PresetContainerBox.num_preset; i++) {
        fwrite(&moov.PresetContainerBox.PresetBox[i].size, sizeof(u32), 1, imf);
        fwrite(&moov.PresetContainerBox.PresetBox[i].type, sizeof(u32), 1, imf);
        fwrite(&moov.PresetContainerBox.PresetBox[i].flags, sizeof(u32), 1, imf);
        fwrite(&moov.PresetContainerBox.PresetBox[i].preset_ID, sizeof(unsigned char), 1, imf);
        fwrite(&moov.PresetContainerBox.PresetBox[i].num_preset_elements, sizeof(unsigned char), 1, imf);
        for (j=0; j< moov.PresetContainerBox.PresetBox[i].num_preset_elements; j++) {
            fwrite(&moov.PresetContainerBox.PresetBox[i].presElemId[j].
                   preset_element_ID, sizeof(u32), 1, imf);
        }
        fwrite(&moov.PresetContainerBox.PresetBox[i].preset_type , sizeof(unsigned char), 1, imf);
        fwrite(&moov.PresetContainerBox.PresetBox[i].preset_global_volume, sizeof(unsigned char), 1, imf);

        type = moov.PresetContainerBox.PresetBox[i].preset_type;

        switch(type){

            case 0: for (j=0; j< moov.PresetContainerBox.PresetBox[i].num_preset_elements; j++) {
                        fwrite(&moov.PresetContainerBox.PresetBox[i].StaticTrackVolume.presVolumElem[j].
                            preset_volume_element,sizeof(unsigned char), 1, imf);
                    }
                    break;
            case 1: for (j=0; j<num_ch; j++){
                        fwrite(&moov.PresetContainerBox.PresetBox[i].StaticObjectVolume.InputCH[j].
                            num_input_channel,sizeof(unsigned char), 1, imf);
                    }
                    fwrite(&moov.PresetContainerBox.PresetBox[i].StaticObjectVolume.output_channel_type, sizeof(unsigned char), 1, imf);
                    for (j=0; j<totaltracks; j++){
                        for (k=0; k<num_ch; k++){
                            for (m=0; m<num_ch; m++){
                                fwrite(&moov.PresetContainerBox.PresetBox[i].StaticObjectVolume.presElVol[j].
                                    Input[k].Output[m].preset_volume_element, sizeof(unsigned char), 1, imf);
                            }
                        }
                    }
                    break;
            case 2: fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicTrackVolume.num_updates, sizeof(u16), 1, imf);
                    temp = bytereverse16(moov.PresetContainerBox.PresetBox[i].DynamicTrackVolume.num_updates);
                    for (j=0; j<temp; j++){
                        fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicTrackVolume.
                            DynamicChange[j].updated_sample_number, sizeof(u16), 1, imf);
                        for (k=0; k<totaltracks; k++){
                            fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicTrackVolume.DynamicChange[j].
                                presVolumElem[k].preset_volume_element, sizeof(unsigned char), 1, imf);
                        }
                    }
                    break;
            case 3: fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicObjectVolume.num_updates, sizeof(u16), 1, imf);
                    for (j=0; j<totaltracks; j++) {
                        fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicObjectVolume.InputCH[j].num_input_channel, sizeof(u8), 1, imf);
                    }
                    fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicObjectVolume.output_channel_type, sizeof(u8), 1, imf);
                    temp = bytereverse16(moov.PresetContainerBox.PresetBox[i].DynamicObjectVolume.num_updates);
                    for (j=0; j<temp; j++){
                        fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicObjectVolume.
                            DynamicChange[j].updated_sample_number, sizeof(u16), 1, imf);
                        for (k=0; k<totaltracks; k++){
                            for (m=0; m<num_ch; m++){
                                for (t=0; t<num_ch; t++){
                                    fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicObjectVolume.DynamicChange[j].
                                        presElVol[k].Input[m].Output[t].preset_volume_element, sizeof(u8), 1, imf);
                                }
                            }
                        }
                    }
                    break;
            case 4: fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.num_updates, sizeof(u16), 1, imf);
                    temp = bytereverse16(moov.PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.num_updates);
                    for (j=0; j<temp; j++){
                        fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                            DynamicChange[j].start_sample_number, sizeof(u16), 1, imf);
                        fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                            DynamicChange[j].duration_update, sizeof(u16), 1, imf);
                        for (k=0; k<totaltracks; k++){
                            fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.DynamicChange[j].
                                presElVol[k].end_preset_volume_element, sizeof(unsigned char), 1, imf);
                        }
                    }
                    break;
            case 5: // NOT YET IMPLEMENTED INTO THE PLAYER!
                    printf("\n\nTHIS PRESET IS NOT YET IMPLEMENTED INTO THE PLAYER!!! PLAYER MAY CRASH WITH THIS FILE!!!\n\n");
                    fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicObjectApproxVolume.num_updates, sizeof(u16), 1, imf);
                    for (j=0; j<totaltracks; j++) {
                        fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicObjectApproxVolume.InputCH[j].num_input_channel, sizeof(u8), 1, imf);
                    }
                    fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicObjectApproxVolume.output_channel_type, sizeof(u8), 1, imf);
                    temp = bytereverse16(moov.PresetContainerBox.PresetBox[i].DynamicObjectApproxVolume.num_updates);
                    for (j=0; j<temp; j++){
                        fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicObjectApproxVolume.
                            DynamicChange[j].start_sample_number, sizeof(u16), 1, imf);
                        fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicObjectApproxVolume.
                            DynamicChange[j].duration_update, sizeof(u16), 1, imf);
                        for (k=0; k<totaltracks; k++){
                            for (m=0; m<num_ch; m++){
                                for (t=0; t<num_ch; t++){
                                    fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicObjectApproxVolume.DynamicChange[j].
                                        presElVol[k].Input[m].Output[t].preset_volume_element, sizeof(u8), 1, imf);
                                }
                            }
                        }
                    }
                    break;
            case 6: printf("\nERROR WRITING PRESET CONTAINER IN OUTPUT FILE - Not valid case (6)\n");
                    system("pause");
                    exit(1);
                    break;
            case 7: printf("\nERROR WRITING PRESET CONTAINER IN OUTPUT FILE - Not valid case (7)\n");
                    system("pause");
                    exit(1);
                    break;
            case 8: for (j=0; j< moov.PresetContainerBox.PresetBox[i].num_preset_elements; j++) {
                        fwrite(&moov.PresetContainerBox.PresetBox[i].StaticTrackVolume.presVolumElem[j].
                            preset_volume_element,sizeof(unsigned char), 1, imf);
                        fwrite(&moov.PresetContainerBox.PresetBox[i].StaticTrackVolume.presVolumElem[j].EQ.num_eq_filters, sizeof(u8), 1, imf);
                        //EQ
                        if (moov.PresetContainerBox.PresetBox[i].StaticTrackVolume.presVolumElem[j].EQ.num_eq_filters != 0) {
                            for (k=0; k<moov.PresetContainerBox.PresetBox[i].StaticTrackVolume.presVolumElem[j].EQ.num_eq_filters; k++){
                                fwrite(&moov.PresetContainerBox.PresetBox[i].StaticTrackVolume.
                                    presVolumElem[j].EQ.Filter[k].filter_type, sizeof(u8), 1, imf);
                                fwrite(&moov.PresetContainerBox.PresetBox[i].StaticTrackVolume.
                                    presVolumElem[j].EQ.Filter[k].filter_reference_frequency, sizeof(u16), 1, imf);
                                fwrite(&moov.PresetContainerBox.PresetBox[i].StaticTrackVolume.
                                    presVolumElem[j].EQ.Filter[k].filter_gain, sizeof(u8), 1, imf);
                                fwrite(&moov.PresetContainerBox.PresetBox[i].StaticTrackVolume.
                                    presVolumElem[j].EQ.Filter[k].filter_bandwidth, sizeof(u8), 1, imf);
                            } //close for
                        } //close if
                    }
                    break;
            case 9: // NOT YET IMPLEMENTED INTO THE PLAYER!
                    printf("\n\nTHIS PRESET IS NOT YET IMPLEMENTED INTO THE PLAYER!!! PLAYER MAY CRASH WITH THIS FILE!!!\n\n");
                    break;
            case 10:// NOT YET IMPLEMENTED INTO THE PLAYER!
                    printf("\n\nTHIS PRESET IS NOT YET IMPLEMENTED INTO THE PLAYER!!! PLAYER MAY CRASH WITH THIS FILE!!!\n\n");
                    break;
            case 11:// NOT YET IMPLEMENTED INTO THE PLAYER!
                    printf("\n\nTHIS PRESET IS NOT YET IMPLEMENTED INTO THE PLAYER!!! PLAYER MAY CRASH WITH THIS FILE!!!\n\n");
                    break;
            case 12:// NOT YET IMPLEMENTED INTO THE PLAYER!
                    printf("\n\nTHIS PRESET IS NOT YET IMPLEMENTED INTO THE PLAYER!!! PLAYER MAY CRASH WITH THIS FILE!!!\n\n");
                    fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.num_updates, sizeof(u16), 1, imf);
                    temp = bytereverse16(moov.PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.num_updates);
                    for (j=0; j<temp; j++){
                        fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                            DynamicChange[j].start_sample_number, sizeof(u16), 1, imf);
                        fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                            DynamicChange[j].duration_update, sizeof(u16), 1, imf);
                        for (k=0; k<totaltracks; k++){
                            fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.DynamicChange[j].
                                presElVol[k].end_preset_volume_element, sizeof(u8), 1, imf);
                            //EQ
                            fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                                     DynamicChange[j].presElVol[k].EQ.num_eq_filters, sizeof(u8), 1, imf);
                            if(moov.PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                                     DynamicChange[j].presElVol[k].EQ.num_eq_filters != 0){
                                for (t=0; t<moov.PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                                     DynamicChange[j].presElVol[k].EQ.num_eq_filters; t++){
                                    fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                                     DynamicChange[j].presElVol[k].EQ.Filter[t].filter_type, sizeof(u8), 1, imf);
                                    fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                                     DynamicChange[j].presElVol[k].EQ.Filter[t].filter_reference_frequency, sizeof(u16), 1, imf);
                                    fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                                     DynamicChange[j].presElVol[k].EQ.Filter[t].end_filter_gain, sizeof(u8), 1, imf);
                                    fwrite(&moov.PresetContainerBox.PresetBox[i].DynamicTrackApproxVolume.
                                     DynamicChange[j].presElVol[k].EQ.Filter[t].filter_bandwidth, sizeof(u8), 1, imf);
                                } // close for (t)
                            }//close if

                        }// close for (k)
                    } //close for (j)
                    break;
            case 13:// NOT YET IMPLEMENTED INTO THE PLAYER!
                    printf("\n\nTHIS PRESET IS NOT YET IMPLEMENTED INTO THE PLAYER!!! PLAYER MAY CRASH WITH THIS FILE!!!\n\n");
                    break;
            default: printf("\nERROR WRITING PRESET CONTAINER IN OUTPUT FILE - Not valid case (default)\n");
                     system("pause");
                     exit(1);
                     break;

        } // close switch

        for (j=0; j<50; j++) {
            fwrite(&moov.PresetContainerBox.PresetBox[i].preset_name[j], sizeof(char), 1, imf);
        }

    } // close for


    //Rules Container//
    fwrite(&moov.RulesContainer.size, sizeof(u32), 1, imf);
    fwrite(&moov.RulesContainer.type, sizeof(u32), 1, imf);
    fwrite(&moov.RulesContainer.num_selection_rules, sizeof(u16), 1, imf);
    fwrite(&moov.RulesContainer.num_mixing_rules, sizeof(u16), 1, imf);
    //Selection Rules//
    fwrite(&moov.RulesContainer.SelectionRules.size, sizeof(u32), 1, imf);
    fwrite(&moov.RulesContainer.SelectionRules.type, sizeof(u32), 1, imf);
    fwrite(&moov.RulesContainer.SelectionRules.version, sizeof(u32), 1, imf);
    fwrite(&moov.RulesContainer.SelectionRules.selection_rule_ID, sizeof(u16), 1, imf);
    fwrite(&moov.RulesContainer.SelectionRules.selection_rule_type, sizeof(unsigned char), 1, imf);
    fwrite(&moov.RulesContainer.SelectionRules.element_ID, sizeof(u32), 1, imf);

    swap = moov.RulesContainer.SelectionRules.selection_rule_type;
    if ( swap==0 ) {
        fwrite(&moov.RulesContainer.SelectionRules.min_num_elements, sizeof(u16), 1, imf);
        fwrite(&moov.RulesContainer.SelectionRules.max_num_elements, sizeof(u16), 1, imf);
    }
    if ( swap==1 || swap ==3 ){
        fwrite(&moov.RulesContainer.SelectionRules.key_element_ID, sizeof(u32), 1, imf);
    }

    for(i=0; i<20; i++){
        fwrite(&moov.RulesContainer.SelectionRules.rule_description[i], sizeof(char), 1, imf);
    }

    //Mixing Rules//
    fwrite(&moov.RulesContainer.MixingRules.size, sizeof(u32), 1, imf);
    fwrite(&moov.RulesContainer.MixingRules.type, sizeof(u32), 1, imf);
    fwrite(&moov.RulesContainer.MixingRules.version, sizeof(u32), 1, imf);
    fwrite(&moov.RulesContainer.MixingRules.mixing_rule_ID, sizeof(u16), 1, imf);
    fwrite(&moov.RulesContainer.MixingRules.mixing_type,sizeof(unsigned char), 1, imf);
    fwrite(&moov.RulesContainer.MixingRules.element_ID, sizeof(u32), 1, imf);

    swap = moov.RulesContainer.MixingRules.mixing_type;
    if( swap==3 ){
        fwrite(&moov.RulesContainer.MixingRules.min_volume, sizeof(u16), 1, imf);
        fwrite(&moov.RulesContainer.MixingRules.max_volume, sizeof(u16), 1, imf);
        } else {
        fwrite(&moov.RulesContainer.MixingRules.key_elem_ID, sizeof(u32), 1, imf);
    }
    for(i=0; i<17; i++){
        fwrite(&moov.RulesContainer.MixingRules.mix_description[i],
               sizeof(char), 1, imf);
    }

    //TIMED TEXT
    fwrite(&moov.TrackBox[numtrack].size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].type, sizeof(u32), 1, imf);
    //Track header//
    fwrite(&moov.TrackBox[numtrack].TrackHeaderBox,
            sizeof(moov.TrackBox[numtrack].TrackHeaderBox), 1, imf);
    //Media Box//
    fwrite(&moov.TrackBox[numtrack].MediaBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.type, sizeof(u32), 1, imf);
    //Media Header//
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaHeaderBox,
            sizeof(moov.TrackBox[numtrack].MediaBox.MediaHeaderBox), 1, imf);
    //Handler Box//
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.type, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.version, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.pre_defined, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.handler_type, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.reserved[0], sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.reserved[1], sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.reserved[2], sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.data[0], sizeof(unsigned char), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.data[1], sizeof(unsigned char), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.data[2], sizeof(unsigned char), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.data[3], sizeof(unsigned char), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.HandlerBox.data[4], sizeof(unsigned char), 1, imf);
    //Media information box//
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.type, sizeof(u32), 1, imf);
    //Null media header//
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.NullMediaHeaderBox.size,
            sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.NullMediaHeaderBox.type,
            sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.NullMediaHeaderBox.flags,
            sizeof(u32), 1, imf);
    //Data Information
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.size,sizeof(u32),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.type,sizeof(u32),1,imf);
    //Data Reference
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.DataReferenceBox.size,sizeof(u32),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.DataReferenceBox.type,sizeof(u32),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.DataReferenceBox.flags,sizeof(u32),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.DataReferenceBox.entry_count,sizeof(u32),1,imf);
    //Data Entry URL
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.DataReferenceBox.DataEntryUrlBox.size,sizeof(u32),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.DataReferenceBox.DataEntryUrlBox.type,sizeof(u32),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.DataReferenceBox.DataEntryUrlBox.flags,sizeof(u32),1,imf);
    //Sample table box//
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            type, sizeof(u32), 1, imf);
    //Time to sample box//
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.type, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.version, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            TimeToSampleBox.entry_count, sizeof(u32), 1, imf);
    swap = bytereverse(moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                        TimeToSampleBox.entry_count);
    pos = swap;

    for (i=0; i<pos; i++) {
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                TimeToSampleBox.sample_count[i], sizeof(u32), 1, imf);
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                TimeToSampleBox.sample_delta[i], sizeof(u32), 1, imf);
    }

    //Sample description box//
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.type, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.version, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleDescriptionBox.entry_count, sizeof(u32), 1, imf);
    //tx3g
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.size,sizeof(u32),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.type,sizeof(u32),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.a,sizeof(u32),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.b,sizeof(u32),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.displayFlags,sizeof(u32),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.horizontaljustification,sizeof(u8),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.verticaljustification,sizeof(u8),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.backgroundcolorrgba[0],sizeof(u8),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.backgroundcolorrgba[1],sizeof(u8),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.backgroundcolorrgba[2],sizeof(u8),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.backgroundcolorrgba[3],sizeof(u8),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.top,sizeof(u16),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.left,sizeof(u16),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.bottom,sizeof(u16),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.right,sizeof(u16),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.startChar,sizeof(u16),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.endChar,sizeof(u16),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.fontID,sizeof(u16),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.facestyleflags,sizeof(u8),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.fontsize,sizeof(u8),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.textcolorrgba[0],sizeof(u8),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.textcolorrgba[1],sizeof(u8),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.textcolorrgba[2],sizeof(u8),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.textcolorrgba[3],sizeof(u8),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.FontTableBox.size,sizeof(u32),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.FontTableBox.type,sizeof(u32),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.FontTableBox.entrycount,sizeof(u16),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.FontTableBox.fontID,sizeof(u16),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.FontTableBox.fontnamelenght,sizeof(u8),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.FontTableBox.font[0],sizeof(u8),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.FontTableBox.font[1],sizeof(u8),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.FontTableBox.font[2],sizeof(u8),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.FontTableBox.font[3],sizeof(u8),1,imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.
        SampleTableBox.SampleDescriptionBox.TextSampleEntry.FontTableBox.font[4],sizeof(u8),1,imf);
    //Sample Size box//
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleSizeBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleSizeBox.type, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleSizeBox.version, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleSizeBox.sample_size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleSizeBox.sample_count, sizeof(u32), 1, imf);
    swap = bytereverse(moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                        SampleSizeBox.sample_count);

    for(i=0; i<swap; i++){
        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                SampleSizeBox.entry_size[i], sizeof(u32), 1, imf);
    }

    //Sample to chunk box//
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleToChunk.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleToChunk.type, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleToChunk.version, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleToChunk.entry_count, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleToChunk.first_chunk, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleToChunk.samples_per_chunk, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            SampleToChunk.sample_description_index, sizeof(u32), 1, imf);

    //Chunk offset//
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            ChunkOffsetBox.size, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            ChunkOffsetBox.type, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            ChunkOffsetBox.version, sizeof(u32), 1, imf);
    fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            ChunkOffsetBox.entry_count, sizeof(u32), 1, imf);
    swap=0x00;
    fwrite(&swap,sizeof(u32),1,imf);
    for (i=0;i<bytereverse(moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
            ChunkOffsetBox.entry_count);i++){

        fwrite(&moov.TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
                ChunkOffsetBox.chunk_offset[i], sizeof(u32), 1, imf);
    }

    //Song Image//
    /* Go to function "writemeta" */

}


void writepresets(MovieBox moov, int numtrack,int totaltracks, FILE *imf) {

    //int i, j, k, m, t, temp, type;

    // this function is executed in "writemoovbox";
    // to be implemented as standalone
}

// Fill the MetaBox

int metacontainer(MetaBox *meta) {
     //int swap;

     u32 sizeMETA;
     u32 sizeHDLR = 36;
     u32 sizeDINF;
     u32 sizeDREF = 0; //16
     u32 sizeURL = 0; //12
     u32 sizeILOC = 36 - 2;
     u32 sizeIINF;
     u32 sizeINFE = 44;
     u32 sizeXML = 12 + 2000;
     char name[6] = "image";
     char type_content[18] = "application/other";
     char encoding[4] = "jpg";

     //METADATA
     char xml[2000] = "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?><Mpeg7 xmlns=\"urn:mpeg:mpeg7:schema:2001\"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:mpeg7=\"urn:mpeg:mpeg7:schema:2001\"xmlns:xml=\"http://www.w3.org/XML/1998/namespace\"xsi:schemaLocation=\"urn:mpeg:mpeg7:schema:2001 Mpeg7-2001.xsd\"><Description xsi:type=\"CreationDescriptionType\"><CreationInformation><Creation><Title type=\"songTitle\">IMAF Song</Title><Title type=\"albumTitle\">Encoder</Title><Abstract><FreeTextAnnotation>QMUL</FreeTextAnnotation></Abstract><Creator><Role href=\"urn:mpeg:mpeg7:RoleCS:2001:PERFORMER\"/><Agent xsi:type=\"PersonType\"><Name><FamilyName></FamilyName><GivenName>Frank Sinatra</GivenName></Name></Agent></Creator><CreationCoordinates><Date><TimePoint>2013</TimePoint></Date></CreationCoordinates><CopyrightString></CopyrightString></Creation><Classification><Genre href=\"urn:id3:cs:ID3genreCS:v1:12\"><Name>Electro</Name></Genre></Classification></CreationInformation></Description><Description xsi:type=\"SemanticDescriptionType\">	<Semantics>		<SemanticBase xsi:type=\"SemanticStateType\"><AttributeValuePair><Attribute><TermUse href=\"urn:mpeg:maf:cs:musicplayer:CollectionElementsCS:2007:assetNum\"/></Attribute><IntegerValue>07</IntegerValue></AttributeValuePair><AttributeValuePair><Attribute><TermUse href=\"urn:mpeg:maf:cs:musicplayer:CollectionElementsCS:2013:assetTot\"/></Attribute><IntegerValue>11</IntegerValue></AttributeValuePair></SemanticBase></Semantics></Description><Description xsi:type=\"SemanticDescriptionType\"><MediaInformation><MediaIdentification><EntityIdentifier></EntityIdentifier></MediaIdentification></MediaInformation></Description></Mpeg7>";

     sizeDINF = 8;// + sizeDREF + sizeURL;
     sizeIINF = 14 + sizeINFE;
     sizeMETA = 12 + sizeHDLR + sizeDINF + sizeILOC + sizeIINF + sizeXML;

     meta->size = bytereverse(sizeMETA);
     meta->type = bytereverse('meta');
     meta->version = 0;
         //HandlerBox
         meta->theHandler.size = bytereverse(sizeHDLR);
         meta->theHandler.type = bytereverse('hdlr');
         meta->theHandler.version = 0;
         meta->theHandler.pre_defined = 0;
         meta->theHandler.handler_type = bytereverse('meta');
         meta->theHandler.reserved[0] = 0;
         meta->theHandler.reserved[1] = 0;
         meta->theHandler.reserved[2] = 0;
         meta->theHandler.name[0] = bytereverse('m');
         meta->theHandler.name[1] = bytereverse('p');
         meta->theHandler.name[2] = bytereverse('7');
         meta->theHandler.name[3] = bytereverse('t');
         //DataInformationBox
         meta->file_locations.size = bytereverse(sizeDINF);
         meta->file_locations.type = bytereverse('dinf');
         /*
            //DataReferenceBox
            meta->file_locations.DataReferenceBox.size = bytereverse(sizeDREF);
            meta->file_locations.DataReferenceBox.type = bytereverse('dref');
            meta->file_locations.DataReferenceBox.flags = bytereverse(0); // CHECK THIS VALUE
            meta->file_locations.DataReferenceBox.entry_count = bytereverse(1);
                //DataEntryUrlBox
                meta->file_locations.DataReferenceBox.DataEntryUrlBox.size = bytereverse(sizeURL);
                meta->file_locations.DataReferenceBox.DataEntryUrlBox.type = bytereverse('url '); // 'url ' with blank space
                meta->file_locations.DataReferenceBox.DataEntryUrlBox.flags = bytereverse(0); // CHECK THIS VALUE
          */
    //item_location
    meta->item_locations.size = bytereverse(sizeILOC);
    meta->item_locations.type = bytereverse('iloc');
    meta->item_locations.version = 0;
    meta->item_locations.offset_size = 68; // 0100|0100  offset_size = 4 + lenght_size = 4
    //meta->item_locations.lenght_size = 4; //already included
    meta->item_locations.base_offset_size = 64; // 0100|0000  base_offset_size = 4 + reserved = 0
    //meta->item_locations.reserved = 0; //already included
    meta->item_locations.item_count = bytereverse16(1);
    meta->item_locations.item_ID = bytereverse16(1);
    meta->item_locations.data_reference_index = 0;
    //meta->item_locations.base_offset = bytereverse(); // corresponding to iloc_offset in insertImage function
    meta->item_locations.extent_count = bytereverse16(1);
    //meta->item_locations.extent_offset = bytereverse(); // corresponding to iloc_offset in insertImage function
    //meta->item_locations.extent_length = bytereverse(); // corresponding to imagesize in insertImage function

    //ItemInfoBox
    meta->item_infos.size = bytereverse(sizeIINF);
    meta->item_infos.type = bytereverse('iinf');
    meta->item_infos.version = 0;
    meta->item_infos.entry_count = bytereverse16(1);

        //info_entry
        meta->item_infos.info_entry.size = bytereverse(sizeINFE);
        meta->item_infos.info_entry.type = bytereverse('infe');
        meta->item_infos.info_entry.version = 0;
        meta->item_infos.info_entry.item_ID = bytereverse16(1);
        meta->item_infos.info_entry.item_protection_index = 0;
        strcpy(meta->item_infos.info_entry.item_name, name);
        strcpy(meta->item_infos.info_entry.content_type, type_content);
        strcpy(meta->item_infos.info_entry.content_encoding, encoding);

    //XMLBox (MetaData)
    meta->XMLBox.size = bytereverse(sizeXML);
    meta->XMLBox.type = bytereverse('xml ');
    meta->XMLBox.version = 0;
    strcpy(meta->XMLBox.string, xml);

    return sizeMETA;
}

//Read the image's size
u32 getImageSize(const char *imagedir){
    u32 size;
    FILE *img;

    img = fopen(imagedir,"rb");

    //Calculate size of the picture
    fseek(img,0,SEEK_END);
    size = ftell(img);
    fclose(img);

    return size;
}
//Read the text´s size
int getTextSize (FILE *text){

     int d=0,sizetext=0,dat=0,dat1=0,dat2=0,dat3=0;

    //TEXT-FILE
    //Find mdat in text file in order to know the size of the text file
    d=0;
    while(d==0){

    fread (&dat,sizeof(unsigned char),1,text); //read a byte and saves it in dat
    fread (&dat1,sizeof(unsigned char),1,text);
    fread (&dat2,sizeof(unsigned char),1,text);
    fread (&dat3,sizeof(unsigned char),1,text);
    fseek(text,-3,SEEK_CUR);

        if(dat == 0x6D && dat1 == 0x64  && dat2 == 0x61 && dat3 == 0x74){  // 6D646174 = mdat
                    d=1;
        }

    } //close while

    fseek (text,-5,SEEK_CUR);//positionate the pointer at the first byte of size
    fread (&dat,sizeof(unsigned char),1,text); //first byte of size
    fread (&dat1,sizeof(unsigned char),1,text);//second byte of size
    fread (&dat2,sizeof(unsigned char),1,text);//third byte of size
    fread (&dat3,sizeof(unsigned char),1,text);//fourth byte of size

    sizetext = (dat<<24)  | (dat1<<16) | (dat2<<8) | (dat3);
    sizetext = sizetext-8-16; //4 bytes of size and 4 bytes of type and 16 bytes of padding

    fseek (text,20,SEEK_CUR);//Advance 20 bytes, because the pointer must advance 20 bytes to read the size of the first string.This is because of the way that Quicktime creates the 3gp

    return sizetext;


}

//Read the JPEG file

void insertImage (MetaBox *meta, FILE **imf, u32 imagesize, const char *imagedir) {

    FILE *img;
    u64 iloc_offset = 0;
    unsigned char *imgbuffer;

    img = fopen(imagedir,"rb");

    // Binary reading
    fseek(img,0,SEEK_SET);
    imgbuffer= (unsigned char*)malloc(sizeof(unsigned char)*imagesize);
    fread(imgbuffer, 1, imagesize, img);
    fclose(img);

    // Find position in the IMAF file and write the image
    iloc_offset = ftell(*imf);
    fwrite(imgbuffer,1,imagesize, *imf);

    // Set image size and offset values
    meta->item_locations.base_offset = bytereverse(iloc_offset);
    meta->item_locations.extent_length = bytereverse(imagesize);
    meta->item_locations.extent_offset = bytereverse(iloc_offset);

}


void writemetabox(MetaBox meta, FILE *imf) {

    int i=0;

    //MetaBox
    fwrite(&meta.size, sizeof(u32), 1, imf);
    fwrite(&meta.type, sizeof(u32), 1, imf);
    fwrite(&meta.version, sizeof(u32), 1, imf);
    //Handler
    fwrite(&meta.theHandler.size, sizeof(u32), 1, imf);
    fwrite(&meta.theHandler.type, sizeof(u32), 1, imf);
    fwrite(&meta.theHandler.version, sizeof(u32), 1, imf);
    fwrite(&meta.theHandler.pre_defined, sizeof(u32), 1, imf);
    fwrite(&meta.theHandler.handler_type, sizeof(u32), 1, imf);
    fwrite(&meta.theHandler.reserved[0], sizeof(u32), 1, imf);
    fwrite(&meta.theHandler.reserved[1], sizeof(u32), 1, imf);
    fwrite(&meta.theHandler.reserved[2], sizeof(u32), 1, imf);
    fwrite(&meta.theHandler.name[0], sizeof(unsigned char), 1, imf);
    fwrite(&meta.theHandler.name[1], sizeof(unsigned char), 1, imf);
    fwrite(&meta.theHandler.name[2], sizeof(unsigned char), 1, imf);
    fwrite(&meta.theHandler.name[3], sizeof(unsigned char), 1, imf);
    //Data Information Box
    fwrite(&meta.file_locations.size, sizeof(u32), 1, imf);
    fwrite(&meta.file_locations.type, sizeof(u32), 1, imf);
    /*
    //Data Reference
    fwrite(&meta.file_locations.DataReferenceBox.size, sizeof(u32), 1, imf);
    fwrite(&meta.file_locations.DataReferenceBox.type, sizeof(u32), 1, imf);
    fwrite(&meta.file_locations.DataReferenceBox.flags, sizeof(u32), 1, imf);
    fwrite(&meta.file_locations.DataReferenceBox.entry_count, sizeof(u32), 1, imf);

    fwrite(&meta.file_locations.DataReferenceBox.DataEntryUrlBox.size, sizeof(u32), 1, imf);
    fwrite(&meta.file_locations.DataReferenceBox.DataEntryUrlBox.type, sizeof(u32), 1, imf);
    fwrite(&meta.file_locations.DataReferenceBox.DataEntryUrlBox.flags, sizeof(u32), 1, imf);
    */
    //Item Location Box
    fwrite(&meta.item_locations.size, sizeof(u32), 1, imf);
    fwrite(&meta.item_locations.type, sizeof(u32), 1, imf);
    fwrite(&meta.item_locations.version, sizeof(u32), 1, imf);
    fwrite(&meta.item_locations.offset_size, sizeof(unsigned char), 1, imf);
    //fwrite(&meta.item_locations.lenght_size, sizeof(unsigned char), 1, imf);
    fwrite(&meta.item_locations.base_offset_size, sizeof(unsigned char), 1, imf);
    //fwrite(&meta.item_locations.reserved, sizeof(unsigned char), 1, imf);
    fwrite(&meta.item_locations.item_count, sizeof(u16), 1, imf);
    fwrite(&meta.item_locations.item_ID, sizeof(u16), 1, imf);
    fwrite(&meta.item_locations.data_reference_index, sizeof(u16), 1, imf);
    fwrite(&meta.item_locations.base_offset, sizeof(u32), 1, imf);
    fwrite(&meta.item_locations.extent_count, sizeof(u16), 1, imf);
    fwrite(&meta.item_locations.extent_offset, sizeof(u32), 1, imf);
    fwrite(&meta.item_locations.extent_length, sizeof(u32), 1, imf);
    //Item Info
    fwrite(&meta.item_infos.size, sizeof(u32), 1, imf);
    fwrite(&meta.item_infos.type, sizeof(u32), 1, imf);
    fwrite(&meta.item_infos.version, sizeof(u32), 1, imf);
    fwrite(&meta.item_infos.entry_count, sizeof(u16), 1, imf);
    //Info Entry
    fwrite(&meta.item_infos.info_entry.size, sizeof(u32), 1, imf);
    fwrite(&meta.item_infos.info_entry.type, sizeof(u32), 1, imf);
    fwrite(&meta.item_infos.info_entry.version, sizeof(u32), 1, imf);
    fwrite(&meta.item_infos.info_entry.item_ID, sizeof(u16), 1, imf);
    fwrite(&meta.item_infos.info_entry.item_protection_index, sizeof(u16), 1, imf);
    for (i=0; i<6; i++){
        fwrite(&meta.item_infos.info_entry.item_name[i], sizeof(char), 1, imf);
    }
    for (i=0; i<18; i++){
        fwrite(&meta.item_infos.info_entry.content_type[i], sizeof(char), 1, imf);
    }
    for (i=0; i<4; i++){
        fwrite(&meta.item_infos.info_entry.content_encoding[i], sizeof(char), 1, imf);
    }
    //XML for metadata
    fwrite(&meta.XMLBox.size, sizeof(u32), 1, imf);
    fwrite(&meta.XMLBox.type, sizeof(u32), 1, imf);
    fwrite(&meta.XMLBox.version, sizeof(u32), 1, imf);
    for(i=0; i<2000; i++){
        fwrite(&meta.XMLBox.string[i], sizeof(char), 1, imf);
    }

}

//TIMED-TEXT's funcionts
int trackstructure_text (MovieBox *moov, int numtrack, int clock,
                    int durationTrack, int sizemdat, const char *textfile,FILE *text,int totaltracks ) { // creates the text trak structure
    int swap;
    u32 sizeURL;
    u32 sizeDREF;
    u32 sizeDINF;
    u32 sizeSMHD;
    u32 sizeMINF;
    u32 sizeHDLR;
    u32 sizeMDHD;
    u32 sizeMDIA;
    u32 sizeTKHD;
    u32 sizeTRAK;


    //Sample Table Box
    int sizeSTBL = 0;

    sizeSTBL = samplecontainer_text(moov, numtrack,sizemdat, textfile,totaltracks);

    //Data Entry Url Box
    sizeURL = 12;
    swap = bytereverse(sizeURL);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.DataEntryUrlBox.size = swap;
    swap = bytereverse('url ');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.DataEntryUrlBox.type = swap;
    swap = bytereverse(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.DataEntryUrlBox.flags = swap; // =1 Track in same file as movie atom.

    //Data Reference
    sizeDREF = sizeURL+ 16;
    swap = bytereverse(sizeDREF);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.size = swap;
    swap = bytereverse('dref');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.flags = 0;
    swap = bytereverse(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.
    DataReferenceBox.entry_count = swap;

    //Data information Box//
    sizeDINF = sizeDREF + 8;
    swap = bytereverse(sizeDINF);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.size = swap;
    swap = bytereverse('dinf');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.DataInformationBox.type = swap;


    //Null Media Header Box
    swap = bytereverse(12);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.NullMediaHeaderBox.size = swap   ;
    swap = bytereverse ('nmhd');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.NullMediaHeaderBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.NullMediaHeaderBox.flags = 0;

    //Media Information Box//
    sizeMINF = sizeDINF + sizeSTBL + 8 + 12;
    swap = bytereverse(sizeMINF);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.size = swap;
    swap = bytereverse('minf');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.type = swap;

    //Handler Box//
    sizeHDLR = 37;
    swap = bytereverse(sizeHDLR);
    moov->TrackBox[numtrack].MediaBox.HandlerBox.size = swap;
    swap = bytereverse('hdlr');
    moov->TrackBox[numtrack].MediaBox.HandlerBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.version = 0;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.pre_defined = 0;
    swap = bytereverse('text');
    moov->TrackBox[numtrack].MediaBox.HandlerBox.handler_type = swap;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.reserved[0] = 0;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.reserved[1] = 0;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.reserved[2] = 0;
    moov->TrackBox[numtrack].MediaBox.HandlerBox.data[0] = 't';
    moov->TrackBox[numtrack].MediaBox.HandlerBox.data[1] = 'e';
    moov->TrackBox[numtrack].MediaBox.HandlerBox.data[2] = 'x';
    moov->TrackBox[numtrack].MediaBox.HandlerBox.data[3] = 't';
    moov->TrackBox[numtrack].MediaBox.HandlerBox.data[4] = '\0';

    //Media Header Box//
    sizeMDHD = 32;
    swap = bytereverse(sizeMDHD);
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.size = swap;
    swap = bytereverse('mdhd');
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.version = 0;
    swap = bytereverse(clock);
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.creation_time = swap;
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.modification_time = swap;
    swap = bytereverse(1000);
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.timescale = swap;
    swap = bytereverse(durationTrack);
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.duration = swap;
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.language = 0xC455;
    moov->TrackBox[numtrack].MediaBox.MediaHeaderBox.pre_defined = 0;

    //Media Box//
    sizeMDIA = sizeMDHD + sizeHDLR + sizeMINF + 8;
    swap = bytereverse(sizeMDIA);
    moov->TrackBox[numtrack].MediaBox.size = swap;
    swap = bytereverse('mdia');
    moov->TrackBox[numtrack].MediaBox.type = swap;

    //Track Header//
    sizeTKHD = 92;
    swap = bytereverse (sizeTKHD);
    moov->TrackBox[numtrack].TrackHeaderBox.size = swap;
    swap = bytereverse ('tkhd');
    moov->TrackBox[numtrack].TrackHeaderBox.type = swap ;
    swap = bytereverse (0x00000007);
    moov->TrackBox[numtrack].TrackHeaderBox.version = swap;
    swap = bytereverse (clock);
    moov->TrackBox[numtrack].TrackHeaderBox.creation_time = swap;
    moov->TrackBox[numtrack].TrackHeaderBox.modification_time = swap;
    moov->TrackBox[numtrack].TrackHeaderBox.track_ID = bytereverse(12345678);
    moov->TrackBox[numtrack].TrackHeaderBox.reserved = 0;
    swap = bytereverse (durationTrack);
    moov->TrackBox[numtrack].TrackHeaderBox.duration = swap;
    moov->TrackBox[numtrack].TrackHeaderBox.reserved2[0] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.reserved2[1] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.layer = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.alternate_group = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.volume = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.reserved3 = 0;
    swap = bytereverse (0x00010000);
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[0] = swap;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[1] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[2] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[3] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[4] = swap;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[5] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[6] = 0;
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[7] = 0;
    swap = bytereverse(0x40000000);
    moov->TrackBox[numtrack].TrackHeaderBox.matrix[8] = swap;
    moov->TrackBox[numtrack].TrackHeaderBox.width =0;
    moov->TrackBox[numtrack].TrackHeaderBox.height = 0;

    //Track container
    sizeTRAK = sizeTKHD + sizeMDIA + 8;
    swap = bytereverse (sizeTRAK); // Size of one track
    moov->TrackBox[numtrack].size = swap;
    swap = bytereverse ('trak');
    moov->TrackBox[numtrack].type = swap;

    return sizeTRAK;
}


int aux (FILE *text,int num,int num1,int num2,int num3,int *sal) {
    int dat=0,dat1=0,dat2=0,dat3=0,size=0,d=0,cnt=0,i=0;
    fseek(text,0,SEEK_SET);
    d=0;
    while(d==0){

        fread (&dat,sizeof(unsigned char),1,text);
        cnt++;
        fread (&dat1,sizeof(unsigned char),1,text);
        cnt++;
        fread (&dat2,sizeof(unsigned char),1,text);
        cnt++;
        fread (&dat3,sizeof(unsigned char),1,text);
        cnt++;
        fseek(text, -3 ,SEEK_CUR);
        if(dat == num && dat1 == num1  && dat2 == num2 && dat3 == num3){

                  d=1;
        }

        else{

              cnt=cnt-3;
         }

    } //end while


        fseek (text,0,SEEK_SET);//positionate the pointer at first

        cnt=cnt-7;//size is located 7 bytes before type
        for (d=1;d<=cnt;d++){

            fread (&dat,sizeof(unsigned char),1,text);

        }
        fread (&dat1,sizeof(unsigned char),1,text);
        fread (&dat2,sizeof(unsigned char),1,text);
        fread (&dat3,sizeof(unsigned char),1,text);

        size  = (dat<<24)  | (dat1<<16) | (dat2<<8) | (dat3);

        for (i=1;i<=8;i++) {  //advance  sample_size
        fread (&dat,sizeof(unsigned char),1,text);
        }
        fread(&dat,1,1,text);
        fread(&dat1,1,1,text);
        fread(&dat2,1,1,text);
        fread(&dat3,1,1,text);

        *sal = (dat<<24)  | (dat1<<16) | (dat2<<8) | (dat3);


    return size;

}

int samplecontainer_text(MovieBox *moov, int numtrack, int sizemdat,const char *textfiles,int totaltracks) {

    u32 sizeSTSD, sizeSTSZ, swap,i=0;
    u32 sizeSTTS;
    u32 sizeSTSC = 0;
    u32 sizeSTCO = 0;
    u32 sizeSTBL=0;
    int dat=0,dat1=0,dat2=0,dat3=0,sal=0, samplecount=0;

    //Sample Description Box//
    FILE *text;

    text=fopen ( textfiles,"rb");
    fseek(text,0,SEEK_CUR);

    //stsd

    sizeSTSD =16+64;
    swap = bytereverse(sizeSTSD);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.size = swap;
    swap = bytereverse('stsd');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.version = 0;
    swap = bytereverse(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleDescriptionBox.entry_count = swap;


    //tx3g
    swap = bytereverse(64);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.size=swap;
    swap = bytereverse('tx3g');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.type=swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.a=0;
    swap=bytereverse(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.b=swap;
    swap = bytereverse(0x00000800);//continuous karaoke
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.displayFlags = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.horizontaljustification=0x01;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.verticaljustification=0x01;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.backgroundcolorrgba[0]= 0x00;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.backgroundcolorrgba[1]= 0x00;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.backgroundcolorrgba[2]= 0x00;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.backgroundcolorrgba[3]= 0xFF;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.top=0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.left=0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.bottom=0x6400;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.right=0x2C01;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.startChar=0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.endChar=0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.fontID=0x0100;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.facestyleflags=0x01;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.fontsize=0x0A;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.textcolorrgba[0]=0xFF;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.textcolorrgba[1]=0xFF;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.textcolorrgba[2]=0xFF;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.textcolorrgba[3]=0xFF;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.FontTableBox.size =0x12000000;
    swap =bytereverse('ftab');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.FontTableBox.type =swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.FontTableBox.entrycount=0x0100;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.FontTableBox.fontID =0x0200;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.FontTableBox.fontnamelenght =0x05;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.FontTableBox.font[0] =0x53;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.FontTableBox.font[1] =0x65;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.FontTableBox.font[2] =0x72;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.FontTableBox.font[3] =0x69;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleDescriptionBox.TextSampleEntry.FontTableBox.font[4] =0x66;

    //stsz

    sizeSTSZ=aux(text,0x73,0x74,0x73,0x7A,&sal)+4; // the function aux looks for the box stsz in 3gp file and returns the size
    swap = bytereverse('stsz');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleSizeBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleSizeBox.version = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleSizeBox.sample_size=0;

    fseek(text,0,SEEK_CUR);

    fread(&dat,1,1,text); //read sample_count
    fread(&dat1,1,1,text);
    fread(&dat2,1,1,text);
    fread(&dat3,1,1,text);

    samplecount  = (dat<<24)  | (dat1<<16) | (dat2<<8) | (dat3);

    swap= bytereverse(samplecount);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleSizeBox.sample_count=swap;

        for (i=1;i<=samplecount;i++){

            swap = bytereverse(size_phrase[i-1]);
            moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleSizeBox.entry_size[i-1]=swap;
        }

    sizeSTSZ= 20 + bytereverse(moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleSizeBox.sample_count)*4;
    swap = bytereverse(sizeSTSZ);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleSizeBox.size = swap;


    //Time To Sample Box//

    fseek (text,0,SEEK_SET);//positionate the pointer at first
    sizeSTTS=aux(text,0x73,0x74,0x74,0x73,&sal);

    swap = bytereverse(sizeSTTS);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.size = swap;
    swap = bytereverse('stts');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    TimeToSampleBox.version = 0;
    swap = bytereverse(sal);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.TimeToSampleBox.entry_count=swap;
    fseek(text,0,SEEK_CUR);
    for(i=1;i<=sal;i++){

        fread(&dat,1,1,text);//read count
        fread(&dat1,1,1,text);
        fread(&dat2,1,1,text);
        fread(&dat3,1,1,text);

        dat  = (dat<<24)  | (dat1<<16) | (dat2<<8) | (dat3);


        swap = bytereverse(dat);
        moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.TimeToSampleBox.sample_count[i-1]=swap;
        fread(&dat,1,1,text);//read delta

        fread(&dat1,1,1,text);

        fread(&dat2,1,1,text);

        fread(&dat3,1,1,text);


        dat = (dat<<24)  | (dat1<<16) | (dat2<<8) | (dat3);

        swap = bytereverse(dat/0.6);// the input text file has time_scale = 600 but the audio has time scale=1000
        moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.TimeToSampleBox.sample_delta[i-1]=swap;
    }

    //Sample To Chunk//

    dat=0;
    fseek (text,0,SEEK_SET);//positionate the pointer at first
    sizeSTSC = aux(text,0x73,0x74,0x73,0x63,&sal);
    dat=sal;
    swap = bytereverse(sizeSTSC);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.size = swap;
    swap = bytereverse('stsc');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.version = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    SampleToChunk.entry_count = bytereverse(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleToChunk.first_chunk=bytereverse(1);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleToChunk.samples_per_chunk=moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleSizeBox.sample_count;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.SampleToChunk.sample_description_index = bytereverse(1);

    //Chunk Offset Box//
    sizeSTCO=24;
    swap = bytereverse(sizeSTCO);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    ChunkOffsetBox.size = swap;
    swap = bytereverse('co64');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    ChunkOffsetBox.type = swap;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    ChunkOffsetBox.version = 0;
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    ChunkOffsetBox.entry_count = bytereverse(1);
    dat=32+sizemdat*totaltracks;
    swap=bytereverse(dat);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.
    ChunkOffsetBox.chunk_offset [0]= swap;

    //Sample Table Box //
    sizeSTBL = 8 + sizeSTSD + sizeSTSZ + sizeSTSC + sizeSTCO + sizeSTTS;
    swap = bytereverse(sizeSTBL);
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.size = swap;
    swap = bytereverse('stbl');
    moov->TrackBox[numtrack].MediaBox.MediaInformationBox.SampleTableBox.type =swap;
    return  sizeSTBL;
}

//AUX Functions for endianness

int bytereverse(int num) {
    int swapped;
    swapped = ((num>>24)&0xff) | // move byte 3 to byte 0
    ((num<<8)&0xff0000) | // move byte 1 to byte 2
    ((num>>8)&0xff00) | // move byte 2 to byte 1
    ((num<<24)&0xff000000); // byte 0 to byte 3
    return swapped;
}
int bytereverse16(int num) {
    int swapped;
    swapped = ( ((num<<8)&0xff00) | // move byte 1 to byte 2
    ((num>>8)&0x00ff) ); // byte 2 to byte 1
    return swapped;
}
