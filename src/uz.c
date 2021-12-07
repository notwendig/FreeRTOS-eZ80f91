#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "stdint.h"
#include "uzlib.h"

/* produce decompressed output in chunks of this size */
/* default is to decompress byte by byte; can be any other length */
extern uint8_t  eprom;
extern uint32_t eprom_length_of_sections;

int uzip(uint8_t *dest, size_t dlen, const uint8_t *source, size_t slen)
{
    /* -- decompress data -- */
    struct uzlib_uncomp d;
    int res;
	
    uzlib_uncompress_init(&d, NULL, 0);

    /* all 3 fields below must be initialized by user */
    d.source = source;
    d.source_limit = source + slen;
    d.source_read_cb = NULL;

    res = uzlib_gzip_parse_header(&d);
	
    if (res == TINF_OK) 
	{
		d.dest_start = d.dest = dest;
		d.dest_limit = d.dest + dlen;
		res = uzlib_uncompress_chksum(&d);
	}
   return res;
}
