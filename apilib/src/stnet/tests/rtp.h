/* RTP protocol definitions */
#include <endian.h>

/*
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           timestamp                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           synchronization source (SSRC) identifier            |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |            contributing source (CSRC) identifiers             |
   |                             ....                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */

#if __BYTE_ORDER == __LITTLE_ENDIAN
typedef struct STNETi_RTP_Header_s
{
    unsigned                         Version         : 2;
    unsigned                         Padding         : 1;
    unsigned                         Extension       : 1;
    unsigned                         CSRCCount       : 4;
    unsigned                         Marker          : 1;
    unsigned                         PayloadType     : 7;
    unsigned                         SequenceNumber  : 16;
    unsigned long                    TimeStamp	     : 32;
    unsigned long		     SSRC	     : 32;
} RTP_Header_t;

typedef struct {
	unsigned	profile_use	: 16;
	unsigned	length		: 16;
	char		*ext_data;	/* length bytes */
} RTP_Header_ext_t;

typedef struct {
	char	*pad_more;	/* padding_size - 3 bytes */
	char	pad_bytes[3];
	char	padding_size;
} RTP_Padding_t;


#else

typedef struct STNETi_RTP_Header_s
{
    /* BYTE 3 & 4 */
    U32                         SequenceNumber  : 16;
    /* BYTE 2 */
    U32                         PayloadType     : 7;
    U32                         Marker          : 1;
    /* BYTE 1 */
    U32                         CSRCCount       : 4;
    U32                         Extension       : 1;
    U32                         Padding         : 1;
    U32                         Version         : 2;

    unsigned long                         TimeStamp;
    unsigned long			        SSRC;
} RTP_Header_t;

typedef struct {
	unsigned	length		: 16;
	unsigned	profile_use	: 16;
	char		*ext_data;	/* length bytes */
} RTP_Header_ext_t;

typedef struct {
	char	*pad_more;	/* padding_size - 3 bytes */
	char	padding_size;
	char	pad_bytes[3];
} RTP_Padding_t;

#endif


typedef struct {
	RTP_Header_t		hdr;
	unsigned long		CSRC[15];	/* Optional, up to 15 */
	RTP_Header_ext_t	hdrx;		/* Optional */
	char			*payload;
	RTP_Padding_t		pad;		/* Optional */
} RTP_Packet_t;
