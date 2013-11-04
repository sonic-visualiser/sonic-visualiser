#ifndef IMAFENCODER_H
#define IMAFENCODER_H

//***********************************************************//
//      Interactive Music Audio Format (IMAF) ENCODER		 //
//						Version 2.0							 //
//															 //
//                Eugenio Oñate Hospital                     //
//	    Costantino Taglialatela & Jesus Corral Garcìa        //
//															 //
//   Copyright (c) 2013 Centre for Digital Music (C4DM)		 //
//	Queen Mary University of London. All rights reserved.    //
//***********************************************************//
//				      IM_AF Encoder.h						 //
//***********************************************************//


/* for FILE typedef, */
#include <stdio.h>

#define maxtracks 8 //change this value to support more than 8 audio tracks. This value was 6 before I changed it
#define maxgroups 2
#define maxpreset 10
#define maxrules 10
#define maxfilters 3 //Max number of Filters for an EQ preset
#define maxdynamic 2 //Max number of Dynamic Volume changes
#define num_ch 2	 //Number of channel outputs (STEREO)

typedef long long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

//typedef struct nametrack { // Stores the different titles of the tracks
//    char title[20];
//}nametrack[maxtracks];

typedef struct FileTypeBox
{
    u32 size;
    u32 type;          // ftyp
    u32 major_brand;   // brand identifier
    u32 minor_version; // informative integer for the mirror version
    u32 compatible_brands[2]; //list of brands
}FileTypeBox;

typedef struct MovieBox //extends Box('moov')
{
    u32 size;
    u32 type;       // moov

    struct MovieHeaderBox
    {
        u32 size;
        u32 type; // mvhd
        u32 version; // version + flag
        u32 creation_time;
        u32 modification_time;
        u32 timescale; // specifies the time-scale
        u32 duration;
        u32 rate;  // typically 1.0
        u16 volume; // typically full volume
        u16 reserved; // =0
        u32 reserved2[2]; //=0
        u32 matrix[9]; // information matrix for video (u,v,w)
        u32 pre_defined[6]; // =0
        u32 next_track_ID; //non zero value for the next track ID
    }MovieHeaderBox;

   struct TrackBox
    {
        u32 size;
        u32 type;
        struct TrackHeaderBox
        {
            u32 size;
            u32 type;
            u32 version; // version + flag
            u32 creation_time;
            u32 modification_time;
            u32 track_ID;
            u32 reserved; // =0
            u32 duration;
            u32 reserved2[2]; // =0
            u16 layer; // =0  // for video
            u16 alternate_group; // =0
            u16 volume; // full volume is 1 = 0x0100
            u16 reserved3;// =0
            u32 matrix[9]; // for video
            u32 width; // video
            u32 height; // video
        }TrackHeaderBox;

        struct MediaBox // extends Box('mdia')
        {
            u32 size;
            u32 type;
            struct MediaHeaderBox // extends FullBox('mdhd', version,0)
            {
                u32 size;
                u32 type;
                u32 version; // version + flag
                u32 creation_time;
                u32 modification_time;
                u32 timescale;
                u32 duration;
                u16 language; // [pad,5x3] = 16 bits and pad = 0
                u16 pre_defined; // =0
            }MediaHeaderBox;
           struct HandlerBox  // extends FullBox('hdlr')
            {
                u32 size;
                u32 type;
                u32 version; // version = 0 + flag
                u32 pre_defined; // =0
                u32 handler_type; // = 'soun' for audio track, text or hint
                u32 reserved[3]; // =0
                unsigned char data[5]; // Does not work! only 4 bytes

            }HandlerBox;
             struct MediaInformationBox //extends Box('minf')
            {
                u32 size;
                u32 type;
                // smhd in sound track only!!
               struct SoundMediaHeaderBox //extends FullBox('smhd')
                {
                    u32 size;
                    u32 type;
                    u32 version;
                    u16 balance; // =0 place mono tracks in stereo. 0 is center
                    u16 reserved; // =0
                }SoundMediaHeaderBox;
               struct NullMediaHeaderBox //extends FullBox('nmhd')
                {
                    u32 size;
                    u32 type;
                    u32 flags;
                }NullMediaHeaderBox;
               struct DataInformationBox //extends Box('dinf')
                {
                    u32 size;
                    u32 type;
                    struct DataReferenceBox
                    {
                        u32 size;
                        u32 type;
                        u32 flags;
                        u32 entry_count; // counts the actual entries.
                        struct DataEntryUrlBox //extends FullBox('url', version=0, flags)
                        {
                            u32 size;
                            u32 type;
                            u32 flags;
                        }DataEntryUrlBox;
                    }DataReferenceBox;
                }DataInformationBox;
                struct SampleTableBox // extends Box('stbl')
                {
                    u32 size;
                    u32 type;
                    struct TimeToSampleBox{
                        u32 size;
                        u32 type;
                        u32 version;
                        u32 entry_count;
                        u32 sample_count[3000];
                        u32 sample_delta[3000];
                    }TimeToSampleBox;
                    struct SampleDescriptionBox // stsd
                    {
                        u32 size;
                        u32 type;
                        u32 version;
                        u32 entry_count; // = 1 number of entries
                    //    unsigned char esds[88];
                        struct TextSampleEntry{
                            u32 size;
                            u32 type; //tx3g
                            u32 a;
                            u32 b;
                            u32 displayFlags;
                            u8 horizontaljustification;
                            u8 verticaljustification;
                            u8 backgroundcolorrgba[4];
                            u16 top;
                            u16 left;
                            u16 bottom;
                            u16 right;
                            //StyleRecord
                            u16 startChar;
                            u16 endChar;
                            u16 fontID;
                            u8 facestyleflags;
                            u8 fontsize;
                            u8 textcolorrgba[4];
                            struct FontTableBoX{
                                u32 size;
                                u32 type;
                                u16 entrycount;
                                u16 fontID;
                                u8 fontnamelenght;
                                u8 font[5]; //Serif
                            }FontTableBox;
                        }TextSampleEntry;
                        struct AudioSampleEntry{
                            u32 size;
                            u32 type;   //mp4a
                            char reserved[6];
                            u16 data_reference_index; // = 1
                            u32 reserved2[2];
                            u16 channelcount; // = 2
                            u16 samplesize; // = 16
                            u32 reserved3;
                            u32 samplerate; // 44100 << 16
                     //       unsigned char esds[81];
                            struct ESbox{
                                u32 size;
                                u32 type;
                                u32 version;
                                struct ES_Descriptor{
                                    unsigned char tag;
                                    unsigned char length;
                                    u16 ES_ID;
                                    unsigned char mix;
                                    struct DecoderConfigDescriptor{
                                        unsigned char tag;
                                        unsigned char length;
                                        unsigned char objectProfileInd;
                                        u32 mix;
                                        u32 maxBitRate;
                                        u32 avgBitrate;
                                     /*   struct DecoderSpecificInfo{
                                            unsigned char tag;
                                            unsigned length;
                                           // unsigned char decSpecificInfosize;
                                            unsigned char decSpecificInfoData[2];
                                        }DecoderSpecificInfo;
                                   */ }DecoderConfigDescriptor;
                                    struct SLConfigDescriptor{
                                        unsigned char tag;
                                        unsigned char length;
                                        unsigned char predifined;
                                    }SLConfigDescriptor;
                                  }ES_Descriptor;
                            }ESbox;
                        }AudioSampleEntry;
                    }SampleDescriptionBox;
                    struct SampleSizeBox{
                        u32 size;
                        u32 type;
                        u32 version;
                        u32 sample_size; // =0
                        u32 sample_count;
                        u32 entry_size[9000];
                    }SampleSizeBox;
                    struct SampleToChunk{
                        u32 size;
                        u32 type;
                        u32 version;
                        u32 entry_count;
                        u32 first_chunk;
                        u32 samples_per_chunk;
                        u32 sample_description_index;
                    }SampleToChunk;
                    struct ChunkOffsetBox{
                        u32 size;
                        u32 type;
                        u32 version;
                        u32 entry_count;
                        u32 chunk_offset[maxtracks];
                    }ChunkOffsetBox;
                }SampleTableBox;
            }MediaInformationBox;
        }MediaBox;
    }TrackBox[maxtracks]; // max 10 tracks

    struct PresetContainerBox // extends Box('prco')
    {
        u32 size;
        u32 type;
        unsigned char num_preset;
        unsigned char default_preset_ID;
        struct PresetBox //extends FullBox('prst',version=0,flags)
        {
            u32 size;
            u32 type;
            u32 flags;
            unsigned char preset_ID;
            unsigned char num_preset_elements;
            struct presElemId{
                u32 preset_element_ID;
            }presElemId[maxtracks];
            unsigned char preset_type;
            unsigned char preset_global_volume;

            // if (preset_type == 0) || (preset_type == 8) - Static track volume preset
            struct StaticTrackVolume{
                struct presVolumElem{
                    u8 preset_volume_element;
                    struct EQ{ // if preset_type == 8 (with EQ)
                        u8 num_eq_filters;
                        struct Filter{
                            u8 filter_type;
                            u16 filter_reference_frequency;
                            u8 filter_gain;
                            u8 filter_bandwidth;
                        }Filter[maxfilters];
                    }EQ;
                }presVolumElem[maxtracks];
            }StaticTrackVolume;

            // if (preset_type == 1) || (preset_type == 9) - Static object volume preset
            struct StaticObjectVolume{
                struct InputCH{
                    u8 num_input_channel;
                }InputCH[maxtracks];
                u8 output_channel_type;
                struct presElVol_1{
                    struct Input{
                        struct Output{
                            u8 preset_volume_element;
                        }Output[num_ch];
                        struct EQ_1{ // if preset_type == 9 (with EQ)
                            u8 num_eq_filters;
                            struct Filter_1{
                                u8 filter_type;
                                u16 filter_reference_frequency;
                                u8 filter_gain;
                                u8 filter_bandwidth;
                            }Filter[maxfilters];
                        }EQ;
                    }Input[num_ch];
                }presElVol[maxtracks];
            }StaticObjectVolume;

            // if (preset_type == 2) || (preset_type == 10) - Dynamic track volume preset
            struct DynamicTrackVolume{
                u16 num_updates;
                struct DynamicChange{
                    u16 updated_sample_number;
                    struct presVolumElem_2{
                        u8 preset_volume_element;
                        struct EQ_2{ // if preset_type == 10 (with EQ)
                            u8 num_eq_filters;
                            struct Filter_2{
                                u8 filter_type;
                                u16 filter_reference_frequency;
                                u8 filter_gain;
                                u8 filter_bandwidth;
                            }Filter[maxfilters];
                        }EQ;
                    }presVolumElem[maxtracks];
                }DynamicChange[maxdynamic];
            }DynamicTrackVolume;

            // if (preset_type == 3) || (preset_type == 11) - Dynamic object volume preset
            struct DynamicObjectVolume{
                u16 num_updates;
                struct InputCH_3{
                    u8 num_input_channel;
                }InputCH[maxtracks];
                u8 output_channel_type;
                struct DynamicChange_3{
                    u16 updated_sample_number;
                    struct presElVol{
                        struct Input_3{
                            struct Output_3{
                                u8 preset_volume_element;
                            }Output[num_ch];
                            struct EQ_3{ // if preset_type == 11 (with EQ)
                                u8 num_eq_filters;
                                struct Filter_3{
                                    u8 filter_type;
                                    u16 filter_reference_frequency;
                                    u8 filter_gain;
                                    u8 filter_bandwidth;
                                }Filter[maxfilters];
                            }EQ;
                        }Input[num_ch];
                    }presElVol[maxtracks];
                }DynamicChange[maxdynamic];
            }DynamicObjectVolume;

            // if (preset_type == 4) || (preset_type == 12) - Dynamic track approximated volume preset
            struct DynamicTrackApproxVolume{
                u16 num_updates;
                struct DynamicChange_4{
                    u16 start_sample_number;
                    u16 duration_update;
                    struct presElVol_4{
                        u8 end_preset_volume_element;
                        struct EQ_4{ // if preset_type == 12 (with EQ)
                            u8 num_eq_filters;
                            struct Filter_4{
                                u8 filter_type;
                                u16 filter_reference_frequency;
                                u8 end_filter_gain;
                                u8 filter_bandwidth;
                            }Filter[maxfilters];
                        }EQ;
                    }presElVol[maxtracks];
                }DynamicChange[maxdynamic];
            }DynamicTrackApproxVolume;

            // if (preset_type == 5) || (preset_type == 13) - Dynamic object approximated volume preset
            // THIS STRUCTURE GIVES STACK OVERFLOW PROBLEMS - MORE STACK SIZE NEEDED -> Needs investigation
            struct DynamicObjectApproxVolume{
                u16 num_updates;
                struct InputCH_5{
                    u8 num_input_channel;
                }InputCH[maxtracks];
                u8 output_channel_type;
                struct DynamicChange_5{
                    u16 start_sample_number;
                    u16 duration_update;
                    struct presElVol_5{
                        struct Input_5{
                            struct Output_5{
                                u8 preset_volume_element;
                            }Output[num_ch];
                            struct EQ_5{ // if preset_type == 11 (with EQ)
                                u8 num_eq_filters;
                                struct Filter_5{
                                    u8 filter_type;
                                    u16 filter_reference_frequency;
                                    u8 end_filter_gain;
                                    u8 filter_bandwidth;
                                }Filter[maxfilters];
                            }EQ;
                        }Input[num_ch];
                    }presElVol[maxtracks];
                }DynamicChange[maxdynamic];
            }DynamicObjectApproxVolume;

            char preset_name[50];

        }PresetBox[maxpreset];

    }PresetContainerBox;

    struct RulesContainer{
        u32 size;
        u32 type;
        u16 num_selection_rules;
        u16 num_mixing_rules;
        struct SelectionRules{
            u32 size;
            u32 type;
            u32 version;
            u16 selection_rule_ID;
            unsigned char selection_rule_type;
            u32 element_ID;
            // Only for Min/Max Rule
            // if (selection_rule_type==0)
                u16 min_num_elements;
                u16 max_num_elements;
            // Only for Exclusion and Implication Rules
            // if (selection_rule_type==1 || selection_rule_type==3)
                u32 key_element_ID;
            char rule_description[20];
        }SelectionRules;
        struct MixingRules{
            u32 size;
            u32 type;
            u32 version;
            u16 mixing_rule_ID;
            unsigned char mixing_type;
            u32 element_ID;
            u16 min_volume;
            u16 max_volume;
            u32 key_elem_ID;
            char mix_description[17];
        }MixingRules;
    }RulesContainer;
    struct GroupContainerBox{ //extends Box('grco')
        u32 size; // = 10 + sizeGRUP
        u32 type;
        u16 num_groups;
        struct GroupBox{ // extends FullBox('grup')
        u32 size; // = 21 + 22 + 32 (+2 if group_activation_mode = 2)
        u32 type;
        u32 version;
        u32 group_ID;
        u16 num_elements;
        struct groupElemId{
            u32 element_ID;
        }groupElemId[maxtracks];
        unsigned char group_activation_mode;
        u16 group_activation_elements_number;
        u16 group_reference_volume;
        char group_name[22];
        char group_description[32];
        }GroupBox[maxgroups];
    }GroupContainerBox;
}MovieBox;

typedef struct MetaBox // extends FullBox ('meta')
{
    u32 size;
    u32 type;
    u32 version;
    struct theHandler //extends FullBox HandlerBox('hdlr')
    {
        u32 size;
        u32 type;
        u32 version; // version = 0 + flag
        u32 pre_defined; // =0
        u32 handler_type; // = 'meta' for Timed Metadata track
        u32 reserved[3]; // =0
        unsigned char name[4];
    }theHandler;
    struct file_locations //extends Box DataInformationBox('dinf')
    {
        u32 size;
        u32 type;
/*		struct DataReferenceBox2
        {
            u32 size;
            u32 type;
            u32 flags;
            u32 entry_count;   // = 1
            struct DataEntryUrlBox2 //extends FullBox('url', version=0, flags)
            {
                u32 size;
                u32 type;
                u32 flags;
            }DataEntryUrlBox;
        }DataReferenceBox; */
    }file_locations;
    struct item_locations //extends FullBox ItemLocationBox('iloc')
    {
        u32 size;
        u32 type;
        u32 version; // version = 0 + flags
        unsigned char offset_size; // = 4 bytes
        unsigned char lenght_size; // = 4 bytes
        unsigned char base_offset_size; // = 4 bytes
        unsigned char reserved; // = 0
        u16 item_count;  // = 1
        u16 item_ID;  // = 1
        u16 data_reference_index;  // = 0 (this file)
        u32 base_offset; // size=(base_offset_size*8)=4*8
        u16 extent_count;   // = 1
        u32 extent_offset; // size=(offset_size*8)=4*8
        u32 extent_length;  // size=(lenght_size*8)=4*8
    }item_locations;
    struct item_infos //extends FullBox ItemInfoBox('iinf')
    {
        u32 size;
        u32 type;
        u32 version; // version = 0 + flag
        u16 entry_count;   // = 1
        struct info_entry// extends FullBox ItemInfoEntry('infe')
        {
            u32 size;
            u32 type;
            u32 version; // = 0
            u16 item_ID; // = 1
            u16 item_protection_index; // = 0 for "unprotected"
            char item_name[6]; // name with max 5 characters
            char content_type[18]; // = 'application/other' -> 17 characters
            char content_encoding[4]; // = 'jpg' for JPEG image -> 3 characters
        }info_entry;
    }item_infos;
    struct XMLBox // extends FullBox('xml ')
    {
        u32 size;
        u32 type;
        u32 version;
        char string[2000];
    }XMLBox;
}MetaBox;

typedef struct MediaDataBox // extends Box('mdat')
{
    u32 size;
    u32 type;
    unsigned char data;
}MediaDataBox;




























#endif // IMAFENCODER_H
