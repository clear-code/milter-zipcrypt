/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#include <string.h>
#include <zlib.h>
#include "mz-zip.h"

unsigned int
mz_zip_compress (const char *data, unsigned int data_length, char **compressed_data)
{
#define BUFFER_SIZE 4096
    z_stream zlib_stream;
    int ret;
    unsigned int compressed_data_length = 0;

    memset(&zlib_stream, 0, sizeof(zlib_stream));
    ret = deflateInit2(&zlib_stream,
                       1, /*  We always use fast compression */ 
                       Z_DEFLATED,
                       -14, /* zip on Linux seems to use -14. */
                       9, /* Use maximum memory for optimal speed.*/
                       Z_DEFAULT_STRATEGY);

    zlib_stream.next_in = (Bytef*)data;
    zlib_stream.avail_in = data_length;

    zlib_stream.next_out = (Bytef*)compressed_data;
    zlib_stream.avail_out = BUFFER_SIZE;

    while (ret  == Z_OK || ret == Z_STREAM_END) {
        unsigned int written_bytes;

        ret = deflate(&zlib_stream, Z_FINISH);

        written_bytes = BUFFER_SIZE - zlib_stream.avail_out;
        compressed_data_length += written_bytes;
        zlib_stream.next_out = (Bytef*)compressed_data + compressed_data_length;
        zlib_stream.avail_out = BUFFER_SIZE;
        if (ret == Z_STREAM_END)
            break;
    }

    deflateEnd(&zlib_stream);

    return compressed_data_length;
}

