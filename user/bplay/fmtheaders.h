#ifndef _FMTHEADERS_H
#define _FMTHEADERS_H	1

#include <sys/types.h>
#include <inttypes.h>

#ifdef __GNUC__
# define PACKED(x)      __attribute__((packed)) x
#else
# define PACKED(x)	x
#endif

/* Definitions for .VOC files */

#define VOC_MAGIC	"Creative Voice File\032"

#define DATALEN(bp)	((u_int32_t)(bp.BlockLen[0]) | \
                         ((u_int32_t)(bp.BlockLen[1]) << 8) | \
                         ((u_int32_t)(bp.BlockLen[2]) << 16) )

typedef struct vochead {
  u_int8_t  Magic[20];	/* must be VOC_MAGIC */
  u_int16_t BlockOffset;	/* Offset to first block from top of file */
  u_int16_t Version;	/* VOC-file version */
  u_int16_t IDCode;	/* complement of version + 0x1234 */
} vochead;

typedef struct blockTC {
  u_int8_t  BlockID;
  u_int8_t  BlockLen[3];	/* low, mid, high byte of length of rest of block */
} blockTC;

typedef struct blockT1 {
  u_int8_t  TimeConstant;
  u_int8_t  PackMethod;
} blockT1;

typedef struct blockT8 {
  u_int16_t TimeConstant;
  u_int8_t  PackMethod;
  u_int8_t  VoiceMode;
} blockT8;

typedef struct blockT9 {
  u_int   SamplesPerSec;
  u_int8_t  BitsPerSample;
  u_int8_t  Channels;
  u_int16_t Format;
  u_int8_t   reserved[4];
} blockT9;
  


/* Definitions for Microsoft WAVE format */

/* it's in chunks like .voc and AMIGA iff, but my source say there
   are in only in this combination, so I combined them in one header;
   it works on all WAVE-file I have
*/
typedef struct wavhead {
  u_int32_t main_chunk;		/* 'RIFF' */
  u_int32_t length;		/* Length of rest of file */
  u_int32_t chunk_type;		/* 'WAVE' */

  u_int32_t sub_chunk;		/* 'fmt ' */
  u_int32_t sc_len;		/* length of sub_chunk, =16 (rest of chunk) */
  u_int16_t	format;		/* should be 1 for PCM-code */
  u_int16_t	modus;		/* 1 Mono, 2 Stereo */
  u_int32_t sample_fq;		/* frequence of sample */
  u_int32_t byte_p_sec;
  u_int16_t	byte_p_spl;	/* samplesize; 1 or 2 bytes */
  u_int16_t	bit_p_spl;	/* 8, 12 or 16 bit */ 

  u_int32_t data_chunk;		/* 'data' */
  u_int32_t data_length;	/* samplecount (lenth of rest of block?)*/
} wavhead;

#endif
