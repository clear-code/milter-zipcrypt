/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#ifndef __MZ_ZIP_H__
#define __MZ_ZIP_H__

#include "mz-list.h"
#include <stdbool.h>

typedef struct _MzZipStream MzZipStream;

typedef struct _MzZipHeader MzZipHeader;

struct _MzZipHeader
{
    unsigned char signature[4];
    unsigned char need_version[2];
    unsigned char flags[2];
    unsigned char compression_method[2];
    unsigned char last_modified_time[2];
    unsigned char last_modified_date[2];
    unsigned char crc[4];
    unsigned char compressed_size[4];
    unsigned char uncompressed_size[4];
    unsigned char filename_length[2];

    unsigned char extra_field_length[2];
};

typedef struct _MzZipCentralDirectoryRecord MzZipCentralDirectoryRecord;

struct _MzZipCentralDirectoryRecord
{
    unsigned char signature[4];
    unsigned char made_version[2];
    unsigned char need_version[2];
    unsigned char flags[2];
    unsigned char compression_method[2];
    unsigned char last_modified_time[2];
    unsigned char last_modified_date[2];
    unsigned char crc[4];
    unsigned char compressed_size[4];
    unsigned char uncompressed_size[4];
    unsigned char filename_length[2];
    unsigned char extra_field_length[2];
    unsigned char file_comment_length[2];
    unsigned char start_disk_num[2];
    unsigned char internal_file_attributes[2];
    unsigned char external_file_attributes[4];
    unsigned char header_offset[4];
};

typedef struct _MzZipEndOfCentralDirectoryRecord MzZipEndOfCentralDirectoryRecord;

struct _MzZipEndOfCentralDirectoryRecord
{
    unsigned char signature[4];
    unsigned char num_disk[2];
    unsigned char start_disk_num[2];
    unsigned char total_disk_num[2];
    unsigned char total_entry_num[2];
    unsigned char entry_size[4];
    unsigned char offset[4];
    unsigned char comment_length[2];
};

unsigned int mz_zip_compress_in_memory
                                  (const char  *data,
                                   unsigned int data_length,
                                   char       **compressed_data,
                                   int         *guessed_data_type);
MzZipHeader *mz_zip_create_header (const char *filename,
                                   const char *data,
                                   unsigned int data_length,
                                   time_t last_modified_time,
                                   unsigned int compressed_size);

MzZipCentralDirectoryRecord *
             mz_zip_create_central_directory_record
                                  (const char *filename,
                                   MzZipHeader *header,
                                   int file_attributes,
                                   int data_type);
MzZipEndOfCentralDirectoryRecord *
            mz_zip_create_end_of_central_directory_record
                                  (unsigned int central_directory_records_length,
                                   unsigned int central_directory_record_start_pos);

unsigned int mz_zip_compress_attachments
                                  (int fd,
                                   MzList *attachments);

MzZipStream *mz_zip_stream_create              (void);
bool         mz_zip_stream_begin_archive       (MzZipStream *zip);
bool         mz_zip_stream_begin_file          (MzZipStream *zip,
                                                const char  *filename);
bool         mz_zip_stream_compress_step       (MzZipStream  *zip,
                                                const char   *input_buffer,
                                                unsigned int  input_buffer_size,
                                                char         *output_buffer,
                                                unsigned int  output_buffer_size,
                                                unsigned int *processed_size,
                                                unsigned int *written_size);
bool         mz_zip_stream_end_file            (MzZipStream  *zip,
                                                char         *output_buffer,
                                                unsigned int  output_buffer_size,
                                                unsigned int *written_size);
bool         mz_zip_stream_end_archive         (MzZipStream  *zip,
                                                char         *output_buffer,
                                                unsigned int  output_buffer_size,
                                                unsigned int *written_size);
void         mz_zip_stream_destroy             (MzZipStream  *zip);

#endif /* __MZ_ZIP_H__ */

