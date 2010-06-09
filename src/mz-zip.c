/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#include <string.h>
#include <zlib.h>
#include <time.h>
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

static void
convert_time_to_msdos_time_and_date (time_t time,
                                              unsigned short *msdos_time,
                                              unsigned short *msdos_date)
{
    struct tm tm;
    localtime_r(&time, &tm);

    *msdos_time = (tm.tm_hour << 11) | (tm.tm_min << 5) | ((tm.tm_sec + 1) >> 1);
    *msdos_date = ((tm.tm_year - 80) << 9) | ((tm.tm_mon  + 1) << 5) | tm.tm_mday;
}

MzZipHeader *
mz_zip_create_header (const char *filename,
                      const char *data,
                      unsigned int data_length,
                      time_t last_modified_time,
                      unsigned int compressed_size)
{
    unsigned short extra_field_length;
    unsigned short msdos_time, msdos_date;
    unsigned short filename_length;
    uLong crc;
    MzZipHeader *header;

    header = malloc(sizeof(*header));

    header->signature[0] = 0x50;
    header->signature[1] = 0x4b;
    header->signature[2] = 0x03;
    header->signature[3] = 0x04;

    header->need_version[0] = 0x14;
    header->need_version[1] = 0x00;

    header->flags[0] = 0x04; /* Fast compression mode */
    header->flags[1] = 0x00;

    header->compression_method[0] = Z_DEFLATED & 0xff;
    header->compression_method[1] = (Z_DEFLATED >> 8) & 0xff;

    convert_time_to_msdos_time_and_date(last_modified_time, &msdos_time, &msdos_date);

    header->last_modified_time[0] = msdos_time & 0xff;
    header->last_modified_time[1] = (msdos_time >> 8) & 0xff;

    header->last_modified_date[0] = msdos_date & 0xff;
    header->last_modified_date[1] = (msdos_date >> 8) & 0xff;

    crc = crc32(0, NULL, 0);
    crc = crc32(crc, (unsigned char*)data, data_length);
    header->crc[0] = crc & 0xff;
    header->crc[1] = (crc >> 8) & 0xff;
    header->crc[2] = (crc >> 16) & 0xff;
    header->crc[3] = (crc >> 24) & 0xff;

    header->compressed_size[0] = compressed_size & 0xff;
    header->compressed_size[1] = (compressed_size >> 8) & 0xff;
    header->compressed_size[2] = (compressed_size >> 16) & 0xff;
    header->compressed_size[3] = (compressed_size >> 24) & 0xff;

    header->uncompressed_size[0] = data_length & 0xff;
    header->uncompressed_size[1] = (data_length >> 8) & 0xff;
    header->uncompressed_size[2] = (data_length >> 16) & 0xff;
    header->uncompressed_size[3] = (data_length >> 24) & 0xff;

    filename_length = strlen(filename);
    header->filename_length[0] = filename_length & 0xff;
    header->filename_length[1] = (filename_length >> 8) & 0xff;

    extra_field_length = 28;
    header->extra_field_length[0] = extra_field_length & 0xff;
    header->extra_field_length[1] = (extra_field_length >> 8) & 0xff;

    return header;
}

MzZipCentralDirectoryRecord *
mz_zip_create_central_directory_record (const char *filename,
                                        MzZipHeader *header,
                                        int file_attributes,
                                        int data_type)
{
    MzZipCentralDirectoryRecord *record;
    void *dest, *src;
    unsigned short extra_field_length;

    record = malloc(sizeof(*record));

    record->signature[0] = 0x50;
    record->signature[1] = 0x4b;
    record->signature[2] = 0x01;
    record->signature[3] = 0x02;

    record->made_version[0] = 0x1e;
    record->made_version[1] = 0x03;

    dest = record;
    dest += 6;
    src = header;
    src += 4;
    memcpy(dest, src, sizeof(*header) - 4);

    extra_field_length = 24;
    record->extra_field_length[0] = extra_field_length & 0xff;
    record->extra_field_length[1] = (extra_field_length >> 8) & 0xff;

    record->file_comment_length[0] = 0;
    record->file_comment_length[1] = 0;

    record->start_disk_num[0] = 0;
    record->start_disk_num[1] = 0;

    record->internal_file_attributes[0] = data_type & 0xff;
    record->internal_file_attributes[1] = (data_type >> 8) & 0xff;

    record->external_file_attributes[0] = file_attributes & 0xff;
    record->external_file_attributes[1] = (file_attributes >> 8) & 0xff;
    record->external_file_attributes[2] = (file_attributes >> 16) & 0xff;
    record->external_file_attributes[3] = (file_attributes >> 24) & 0xff;

    record->header_offset[0] = 0;
    record->header_offset[1] = 0;
    record->header_offset[2] = 0;
    record->header_offset[3] = 0;

    return record;
}

