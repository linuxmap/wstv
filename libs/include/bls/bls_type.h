#ifndef BLS_TYPE_H
#define BLS_TYPE_H

typedef struct
{
	char sps_data[32];
	char sps_size;
	char pps_data[32];
	char pps_size;
	
}BLS_SPS_PPS;

#if 1
typedef enum
{
    e_BLS_NALU_TYPE_PSLICE = 1, /*PSLICE types*/
    e_BLS_NALU_TYPE_ISLICE = 5, /*ISLICE types*/
    e_BLS_NALU_TYPE_SEI    = 6, /*SEI types*/
    e_BLS_NALU_TYPE_SPS    = 7, /*SPS types*/
    e_BLS_NALU_TYPE_PPS    = 8, /*PPS types*/
    e_BLS_NALU_TYPE_AUDIO = 100,   
    
}BLS_NALU_TYPEs;
#endif

#endif
