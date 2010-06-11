/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <zlib.h>
#include <time.h>
#include "mz-zip.h"
#include "mz-attachment.h"

static int
init_z_stream (z_stream *stream)
{
    memset(stream, 0, sizeof(*stream));
    return deflateInit2(stream,
                        1, /*  We always use fast compression */
                        Z_DEFLATED,
                        -14, /* zip on Linux seems to use -14. */
                        9, /* Use maximum memory for optimal speed.*/
                        Z_DEFAULT_STRATEGY);

}

unsigned int
mz_zip_compress_in_memory (const char *data,
                           unsigned int data_length,
                           char **compressed_data,
                           int *guessed_data_type)
{
#define BUFFER_SIZE 4096
    z_stream zlib_stream;
    int ret;
    unsigned int compressed_data_length = 0;

    ret = init_z_stream(&zlib_stream);

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

    *guessed_data_type = zlib_stream.data_type;

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
    unsigned short msdos_time, msdos_date;
    unsigned short filename_length;
    uLong crc;
    MzZipHeader *header;

    header = malloc(sizeof(*header));
    if (!header)
        return NULL;

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

    header->extra_field_length[0] = 0;
    header->extra_field_length[1] = 0;

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

    record = malloc(sizeof(*record));
    if (!record)
        return NULL;

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

    record->extra_field_length[0] = 0;
    record->extra_field_length[1] = 0;

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

#define GET_16BIT_VALUE(x) (((x[0]) & 0xff) | (((x[1]) << 8)))
#define GET_32BIT_VALUE(x) (((x[0]) & 0xff) | (((x[1]) << 8)) | (((x[2] << 16)) | (((x[3]) << 24))))

MzZipEndOfCentralDirectoryRecord *
mz_zip_create_end_of_central_directory_record (unsigned int central_directory_records_length,
                                               unsigned int central_directory_record_start_pos)
{
    MzZipEndOfCentralDirectoryRecord *record;

    record = malloc(sizeof(*record));
    if (!record)
        return NULL;

    record->signature[0] = 0x50;
    record->signature[1] = 0x4b;
    record->signature[2] = 0x05;
    record->signature[3] = 0x06;

    record->num_disk[0] = 0;
    record->num_disk[1] = 0;

    record->start_disk_num[0] = 0;
    record->start_disk_num[1] = 0;

    record->total_disk_num[0] = 0x01;
    record->total_disk_num[1] = 0;

    record->total_entry_num[0] = 0x01;
    record->total_entry_num[1] = 0;

    record->entry_size[0] = central_directory_records_length & 0xff;
    record->entry_size[1] = (central_directory_records_length >> 8) & 0xff;
    record->entry_size[2] = (central_directory_records_length >> 16) & 0xff;
    record->entry_size[3] = (central_directory_records_length >> 24) & 0xff;

    record->offset[0] = central_directory_record_start_pos & 0xff;
    record->offset[1] = (central_directory_record_start_pos >> 8) & 0xff;
    record->offset[2] = (central_directory_record_start_pos >> 16) & 0xff;
    record->offset[3] = (central_directory_record_start_pos >> 24) & 0xff;

    record->comment_length[0] = 0;
    record->comment_length[1] = 0;

    return record;
}

static bool
_write_data (int fd, void *data, size_t data_size, ssize_t *written_size)
{
    *written_size = write(fd, data, data_size);
    return (*written_size == data_size);
}

static MzZipHeader *
create_zip_header_from_attachment (MzAttachment *attachment)
{
    return mz_zip_create_header(attachment->filename,
                                attachment->data,
                                attachment->data_length,
                                attachment->last_modified_time, /* Should set current time? */
                                0); /* compressed_size will be replaced later. */
}

MzZipHeader *
_compress_attachment (int fd,
                      z_stream *zlib_stream,
                      MzAttachment *attachment,
                      unsigned int *compressed_size)
{
    unsigned int compressed_data_length = 0;
    MzZipHeader *header = NULL;
    ssize_t written_bytes;
    off_t header_pos = 0;
    bool success = false;
    int ret = Z_OK;

    header = create_zip_header_from_attachment(attachment);
    if (!header)
        return NULL;

    /* We need current file postition to store compressed size later. */
    header_pos = lseek(fd, 0, SEEK_CUR);
    if (!_write_data(fd, header, sizeof(*header), &written_bytes))
        goto end;
    if (!_write_data(fd, (void*)attachment->filename, strlen(attachment->filename), &written_bytes))
        goto end;

    zlib_stream->next_in = (Bytef*)attachment->data;
    zlib_stream->avail_in = attachment->data_length;

    while (ret  == Z_OK || ret == Z_STREAM_END) {
        char compressed_data[BUFFER_SIZE];
        unsigned int compressed_bytes;

        zlib_stream->next_out = (Bytef*)compressed_data;
        zlib_stream->avail_out = BUFFER_SIZE;

        ret = deflate(zlib_stream, Z_FINISH);

        compressed_bytes = BUFFER_SIZE - zlib_stream->avail_out;
        if (!_write_data(fd, compressed_data, compressed_bytes, &written_bytes))
            goto end;

        compressed_data_length += written_bytes;
        if (ret == Z_STREAM_END)
            break;
    }

    lseek(fd, header_pos + offsetof(MzZipHeader, compressed_size), SEEK_SET);
    if (!_write_data(fd, &compressed_data_length, sizeof(compressed_data_length), &written_bytes))
        goto end;

    memcpy(&header->compressed_size, &compressed_data_length, sizeof(compressed_data_length));
    lseek(fd, 0, SEEK_END);

    success = true;
end:

    if (!success) {
        if (header)
            free(header);
        header = NULL;
    }

    return header;
}

unsigned int
mz_zip_compress_attachments (int fd, MzList *attachments)
{
    z_stream zlib_stream;
    int ret;
    bool success = false;
    ssize_t written_bytes;
    unsigned int compressed_data_length = 0;
    unsigned int central_records_length = 0;
    off_t central_record_start_pos = 0;
    MzList *headers = NULL;
    MzList *central_records = NULL;
    MzList *node;
    MzZipEndOfCentralDirectoryRecord *end_of_record = NULL;
    MzList *first_attachments;

    if (!attachments)
        return -1;

    /* skip mail body */
    attachments = mz_list_next(attachments);
    if (!attachments)
        return -1;

    first_attachments = attachments;
    ret = init_z_stream(&zlib_stream);
    if (ret != Z_OK)
        return -1;

    while (attachments) {
        MzZipHeader *header;
        MzZipCentralDirectoryRecord *central_record = NULL;
        MzAttachment *attachment = attachments->data;

        header = _compress_attachment(fd, &zlib_stream, attachment, &compressed_data_length);
        if (!header)
            goto end;

        central_record = mz_zip_create_central_directory_record(attachment->filename,
                                                                header,
                                                                attachment->file_attributes,
                                                                zlib_stream.data_type);
        if (!central_record)
            goto end;

        headers = mz_list_append(headers, header);
        central_records = mz_list_append(central_records, central_record);
        attachments = mz_list_next(attachments);
        if (attachments)
            deflateReset(&zlib_stream);
    }

    central_record_start_pos = lseek(fd, 0, SEEK_CUR);

    for (attachments = first_attachments, node = central_records;
         node;
         node = mz_list_next(node), attachments = mz_list_next(attachments)) {
        MzZipCentralDirectoryRecord *record = central_records->data;
        MzAttachment *attachment = attachments->data;
        const char *filename = attachment->filename;

        if (!_write_data(fd, record, sizeof(*record), &written_bytes))
            goto end;
        if (!_write_data(fd, (void*)filename, strlen(filename), &written_bytes))
            goto end;
        central_records_length += sizeof(*record) +
                                  GET_16BIT_VALUE(record->filename_length) +
                                  GET_16BIT_VALUE(record->extra_field_length);
    }

    end_of_record = mz_zip_create_end_of_central_directory_record(central_records_length,
                                                                  central_record_start_pos);
    if (!end_of_record)
        goto end;
    if (!_write_data(fd, end_of_record, sizeof(*end_of_record), &written_bytes))
        goto end;

    success = true;

end:
    if (headers)
        mz_list_free_with_free_func(headers, free);
    if (central_records)
        mz_list_free_with_free_func(central_records, free);
    if (end_of_record)
        free(end_of_record);

    deflateEnd(&zlib_stream);

    return success ? compressed_data_length : -1;
}

