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

struct _MzZipStream
{
    z_stream zlib_stream;
    MzList *headers;
    MzList *central_directory_records;
    MzList *filenames;
    char *current_filename;
    MzZipHeader *current_header;
    unsigned int current_header_position;
    unsigned int written_header_size;
    bool written_header;
    unsigned int data_size;
    unsigned int compressed_size;
    uLong crc;
};

static int
init_z_stream (z_stream *stream)
{
    memset(stream, 0, sizeof(*stream));
    return deflateInit2(stream,
                        Z_BEST_SPEED, /*  We always use fast compression */
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
    unsigned short msdos_time = 0, msdos_date = 0;
    unsigned short filename_length;
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

    if (!data && data_length == 0) { /* for stream data */
        header->flags[0] |= (1 << 3);
        header->crc[0] = 0;
        header->crc[1] = 0;
        header->crc[2] = 0;
        header->crc[3] = 0;
    } else {
        uLong crc;
        crc = crc32(0, NULL, 0);
        crc = crc32(crc, (unsigned char*)data, data_length);
        header->crc[0] = crc & 0xff;
        header->crc[1] = (crc >> 8) & 0xff;
        header->crc[2] = (crc >> 16) & 0xff;
        header->crc[3] = (crc >> 24) & 0xff;
    }

    header->compression_method[0] = Z_DEFLATED & 0xff;
    header->compression_method[1] = (Z_DEFLATED >> 8) & 0xff;

    if (last_modified_time != 0)
        convert_time_to_msdos_time_and_date(last_modified_time, &msdos_time, &msdos_date);

    header->last_modified_time[0] = msdos_time & 0xff;
    header->last_modified_time[1] = (msdos_time >> 8) & 0xff;

    header->last_modified_date[0] = msdos_date & 0xff;
    header->last_modified_date[1] = (msdos_date >> 8) & 0xff;

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

static MzZipHeader *
mz_zip_create_stream_header (const char *filename)
{
    return mz_zip_create_header(filename,
                                NULL, 0,
                                0,
                                0);

}

MzZipCentralDirectoryRecord *
mz_zip_create_central_directory_record (const char *filename,
                                        MzZipHeader *header,
                                        int file_attributes,
                                        int data_type,
                                        unsigned int header_position)
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
    record->made_version[1] = 0x03; /* UNIX */

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

    record->header_offset[0] = header_position & 0xff;
    record->header_offset[1] = (header_position >> 8) & 0xff;
    record->header_offset[2] = (header_position >> 16) & 0xff;
    record->header_offset[3] = (header_position >> 24) & 0xff;

    return record;
}

#define GET_16BIT_VALUE(x) (((x[0]) & 0xff) | (((x[1]) << 8)))
#define GET_32BIT_VALUE(x) (((x[0]) & 0xff) | (((x[1]) << 8)) | (((x[2] << 16)) | (((x[3]) << 24))))

MzZipEndOfCentralDirectoryRecord *
mz_zip_create_end_of_central_directory_record (unsigned short entry_num,
                                               unsigned int central_directory_records_length,
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

    record->total_disk_num[0] = entry_num & 0xff;
    record->total_disk_num[1] = (entry_num >> 8) & 0xff;

    record->total_entry_num[0] = entry_num & 0xff;
    record->total_entry_num[1] = (entry_num >> 8) & 0xff;

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
                                                                zlib_stream.data_type,
                                                                0);
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

    end_of_record = mz_zip_create_end_of_central_directory_record(mz_list_length(central_records),
                                                                  central_records_length,
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

MzZipStream *
mz_zip_stream_create (void)
{
    MzZipStream *zip;

    zip = malloc(sizeof(*zip));
    if (!zip)
        return NULL;

    zip->headers = NULL;
    zip->central_directory_records = NULL;
    zip->filenames = NULL;
    zip->current_filename = NULL;
    zip->current_header = NULL;
    zip->current_header_position = 0;
    zip->written_header_size = 0;
    zip->written_header = false;
    if (init_z_stream(&zip->zlib_stream) != Z_OK) {
        free(zip);
        return NULL;
    }

    return zip;
}

MzZipStreamStatus
mz_zip_stream_begin_archive (MzZipStream *zip)
{
    /* Nothing to do here. */
    return MZ_ZIP_STREAM_STATUS_SUCCESS;
}

MzZipStreamStatus
mz_zip_stream_begin_file (MzZipStream *zip,
                          const char  *filename)
{
    MzZipHeader *header;

    if (!zip)
        return MZ_ZIP_STREAM_STATUS_INVALID_HANDLE;

    header = mz_zip_create_stream_header(filename);
    if (!header)
        return MZ_ZIP_STREAM_STATUS_NO_MEMORY;

    zip->headers = mz_list_append(zip->headers, header);
    zip->current_filename = strdup(filename);
    zip->filenames = mz_list_append(zip->filenames, zip->current_filename);
    zip->current_header = header;
    zip->written_header = false;
    zip->written_header_size = 0;
    zip->data_size = 0;
    zip->compressed_size = 0;
    zip->crc = crc32(0, NULL, 0);

    return MZ_ZIP_STREAM_STATUS_SUCCESS;
}

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif
static MzZipStreamStatus
write_header (MzZipStream *zip,
              char         *output_buffer,
              unsigned int  output_buffer_size,
              unsigned int *processed_size,
              unsigned int *written_size)
{
    unsigned int filename_length;

    *written_size = 0;
    *processed_size = 0;

    if (zip->written_header_size < sizeof(*zip->current_header)) {
        *written_size = MIN(output_buffer_size, sizeof(*zip->current_header) - zip->written_header_size);
        memcpy(output_buffer,
               zip->current_header + zip->written_header_size,
               *written_size);
        zip->written_header_size += *written_size;
    }

    filename_length = GET_16BIT_VALUE(zip->current_header->filename_length);
    if (zip->written_header_size >= sizeof(*zip->current_header)) {
        if (output_buffer_size - *written_size > 0) {
            unsigned int written_filename_size;
            unsigned int rest_filename_size;

            written_filename_size = zip->written_header_size - sizeof(*zip->current_header);
            rest_filename_size = MIN(output_buffer_size - *written_size,
                                     filename_length - written_filename_size);
            memcpy(output_buffer + *written_size,
                   zip->current_filename + written_filename_size,
                   rest_filename_size);
            *written_size += rest_filename_size;
            zip->written_header_size += rest_filename_size;
        }
    }

    if (zip->written_header_size >= sizeof(*zip->current_header) + filename_length)
        zip->written_header = true;

    return MZ_ZIP_STREAM_STATUS_SUCCESS;
}

MzZipStreamStatus
mz_zip_stream_compress_step (MzZipStream  *zip,
                             const char   *input_buffer,
                             unsigned int  input_buffer_size,
                             char         *output_buffer,
                             unsigned int  output_buffer_size,
                             unsigned int *processed_size,
                             unsigned int *written_size)
{
    int ret;

    if (!zip->written_header)
        return write_header(zip, output_buffer, output_buffer_size, processed_size, written_size);

    zip->zlib_stream.next_in = (Bytef*)input_buffer;
    zip->zlib_stream.avail_in = input_buffer_size;
    zip->zlib_stream.next_out = (Bytef*)output_buffer;
    zip->zlib_stream.avail_out = output_buffer_size;

    ret = deflate(&zip->zlib_stream, Z_FINISH);

    *written_size = output_buffer_size - zip->zlib_stream.avail_out;
    *processed_size = input_buffer_size - zip->zlib_stream.avail_in;
    zip->data_size += *processed_size;
    zip->compressed_size += *written_size;
    zip->crc = crc32(zip->crc, (unsigned char*)input_buffer, *processed_size);

    return (ret == Z_OK) ? MZ_ZIP_STREAM_STATUS_SUCCESS :
        (ret == Z_STREAM_END) ? MZ_ZIP_STREAM_STATUS_REMAIN_OUTPUT_DATA : MZ_ZIP_STREAM_STATUS_UNKNOWN_ERROR;
}

typedef struct _MzZipDataDescriptor
{
    unsigned int crc;
    unsigned int compressed_size;
    unsigned int uncompressed_size;
} MzZipDataDescriptor;

MzZipStreamStatus
mz_zip_stream_end_file (MzZipStream  *zip,
                        char         *output_buffer,
                        unsigned int  output_buffer_size,
                        unsigned int *written_size)
{
    MzZipDataDescriptor descriptor;

    if (!zip)
        return MZ_ZIP_STREAM_STATUS_INVALID_HANDLE;

    if (output_buffer_size >= sizeof(descriptor)) {
        MzZipCentralDirectoryRecord *central_record;

        descriptor.crc = zip->crc;
        descriptor.compressed_size = zip->compressed_size;
        descriptor.uncompressed_size = zip->data_size;
        memcpy(output_buffer,
               &descriptor, sizeof(descriptor));
        *written_size = sizeof(descriptor);

        memcpy((void*)zip->current_header + offsetof(MzZipHeader, crc), &descriptor, sizeof(descriptor));
        central_record = mz_zip_create_central_directory_record(zip->current_filename,
                                                                zip->current_header,
                                                                020151000000,
                                                                zip->zlib_stream.data_type,
                                                                zip->current_header_position);
        zip->current_header_position += sizeof(*zip->current_header);
        zip->current_header_position += GET_32BIT_VALUE(zip->current_header->compressed_size);
        zip->current_header_position += GET_16BIT_VALUE(zip->current_header->filename_length);
        zip->current_header_position += sizeof(descriptor);
        zip->central_directory_records = mz_list_append(zip->central_directory_records,
                                                        central_record);
        deflateReset(&zip->zlib_stream);
    } else {
        /* error? */
        return MZ_ZIP_STREAM_STATUS_NO_MEMORY;
    }

    return MZ_ZIP_STREAM_STATUS_SUCCESS;
}

MzZipStreamStatus
mz_zip_stream_end_archive (MzZipStream  *zip,
                           char         *output_buffer,
                           unsigned int  output_buffer_size,
                           unsigned int *written_size)
{
    MzList *node, *filename;
    MzZipEndOfCentralDirectoryRecord *end_of_record = NULL;
    unsigned int filenames_length = 0;
    unsigned int central_records_length = 0;
    unsigned int headers_length = 0;
    unsigned int compressed_length = 0;
    unsigned int descriptors_length = 0;

    if (!zip)
        return MZ_ZIP_STREAM_STATUS_INVALID_HANDLE;

    for (node = zip->headers; node; node = mz_list_next(node)) {
        MzZipHeader *header = node->data;
        filenames_length += GET_16BIT_VALUE(header->filename_length);
        headers_length += sizeof(MzZipHeader);
        descriptors_length += sizeof(MzZipDataDescriptor);
        compressed_length += GET_32BIT_VALUE(header->compressed_size);
    }

    central_records_length = sizeof(MzZipCentralDirectoryRecord) * mz_list_length(zip->central_directory_records);
    /* insufficient buffer size */
    /* TODO: We should output data in chunks */
    if (output_buffer_size < central_records_length + filenames_length)
        return MZ_ZIP_STREAM_STATUS_NO_MEMORY;

    /* output central directory record and end of central directory record */
    *written_size = 0;
    for (node = zip->central_directory_records, filename = zip->filenames;
         node && filename;
         node = mz_list_next(node), filename = mz_list_next(filename)) {
        unsigned int filename_length;

        MzZipCentralDirectoryRecord *record = node->data;

        memcpy(output_buffer + *written_size, record, sizeof(*record));
        *written_size += sizeof(*record);

        filename_length = GET_16BIT_VALUE(record->filename_length);
        memcpy(output_buffer + *written_size,
               filename->data, filename_length);
        *written_size += filename_length;
    }

    end_of_record = mz_zip_create_end_of_central_directory_record(mz_list_length(zip->central_directory_records),
                                                                  central_records_length + filenames_length,
                                                                  compressed_length + headers_length + filenames_length + descriptors_length);
    memcpy(output_buffer + *written_size,
           end_of_record, sizeof(*end_of_record));
    *written_size += sizeof(*end_of_record);

    return MZ_ZIP_STREAM_STATUS_SUCCESS;
}

void
mz_zip_stream_destroy (MzZipStream *zip)
{
    if (!zip)
        return;

    if (zip->headers) {
        mz_list_free(zip->headers);
        zip->headers = NULL;
    }

    if (zip->filenames) {
        mz_list_free_with_free_func(zip->filenames, free);
        zip->filenames = NULL;
    }

    deflateEnd(&zip->zlib_stream);

    free(zip);
}

