#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "stdint.h"
#include "uzlib.h"

/* produce decompressed output in chunks of this size */
/* default is to decompress byte by byte; can be any other length */
extern uint8_t  eprom;
extern uint32_t eprom_length_of_sections;

int uzipp(uint8_t *dest, size_t dlen, const uint8_t *source, size_t slen,progres_t progres)
{
    /* -- decompress data -- */
    struct uzlib_uncomp d;
    int res;
	unsigned chunksize = dlen;
	char *buffer = NULL;
	
	if(progres)
	{
		chunksize = dlen /10;
		if( chunksize)
		{
			buffer = pvPortMalloc(chunksize +1);
			if(!buffer)
				chunksize = dlen;
		}
		
	}
//	uzlib_uncompress_init(&d, buffer, buffer?chunksize:0);
	uzlib_uncompress_init(&d, NULL, 0);

    /* all 3 fields below must be initialized by user */
    d.source = source;
    d.source_limit = source + slen;
    d.source_read_cb = progres;

    res = uzlib_gzip_parse_header(&d);
	
    if (res == TINF_OK) 
	{
		
		d.dest_start = d.dest = dest;
			
		while (dlen) {
			unsigned int chunk_len = dlen < chunksize ? dlen : chunksize;
			d.source_read_cb(&d);
			d.dest_limit = d.dest + chunk_len;
			res = uzlib_uncompress_chksum(&d);
			dlen -= chunk_len;
			if (res != TINF_OK) {
				break;
			}
		}
	}
	if(buffer)
		vPortFree(buffer);
   return res;
}

int uzip(uint8_t *dest, size_t dlen, const uint8_t *source, size_t slen)
{
	return uzipp(dest, dlen, source, slen, NULL);
}