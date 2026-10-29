#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "stdint.h"
#include "uzlib.h"

/* eZ80 int is 24 bits; gzip checksums must still occupy all four bytes. */
typedef char uzlib_crc_is_32_bits[(sizeof(((struct uzlib_uncomp *)0)->checksum) == 4) ? 1 : -1];

/* A progress callback is not an input refill callback. Input is bounded by slen. */
int uzipp(uint8_t *dest, size_t dlen, const uint8_t *source, size_t slen, progres_t progres)
{
    struct uzlib_uncomp d;
    size_t remaining = dlen;
    unsigned chunksize = progres ? dlen / 10 : dlen;
    int res;
    if (!dest || !source || slen < 18) {
        return TINF_DATA_ERROR;
    }
    if (!chunksize) {
        chunksize = 1;
    }
    uzlib_uncompress_init(&d, NULL, 0);
    d.source = source;
    d.source_limit = source + slen;
    d.source_read_cb = NULL;
    res = uzlib_gzip_parse_header(&d);
    if (res != TINF_OK) {
        return res;
    }
    d.dest_start = d.dest = dest;
    do {
        unsigned chunk_len = remaining < chunksize ? remaining : chunksize;
        unsigned char *before = d.dest;
        size_t produced;
        if (progres) {
            progres(&d);
        }
        d.dest_limit = d.dest + chunk_len;
        res = uzlib_uncompress_chksum(&d);
        produced = d.dest - before;
        if (produced > remaining) {
            return TINF_DATA_ERROR;
        }
        remaining -= produced;
        if (res == TINF_DONE) {
            return remaining == 0 ? TINF_DONE : TINF_DATA_ERROR;
        }
        if (res != TINF_OK) {
            return res;
        }
        if (!produced) {
            return TINF_DATA_ERROR;
        }
        /* At an exact output boundary, one more zero-output call consumes
         * the final block marker and validates the gzip CRC/length trailer. */
    } while (1);
}

int uzip(uint8_t *dest, size_t dlen, const uint8_t *source, size_t slen)
{
    return uzipp(dest, dlen, source, slen, NULL);
}
