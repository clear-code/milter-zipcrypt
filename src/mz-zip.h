/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#ifndef __MZ_ZIP_H__
#define __MZ_ZIP_H__

#include "mz-list.h"

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
                                   int data_type,
                                   unsigned int header_position);
MzZipEndOfCentralDirectoryRecord *
            mz_zip_create_end_of_central_directory_record
                                  (unsigned short entry_num,
                                   unsigned int central_directory_records_length,
                                   unsigned int central_directory_record_start_pos);

unsigned int mz_zip_compress_attachments
                                  (int fd,
                                   MzList *attachments);

typedef enum
{
    MZ_ZIP_STREAM_STATUS_SUCCESS            = 0,
    MZ_ZIP_STREAM_STATUS_INVALID_HANDLE     = 1,
    MZ_ZIP_STREAM_STATUS_NO_MEMORY          = 2,
    MZ_ZIP_STREAM_STATUS_REMAIN_OUTPUT_DATA = 3,
    MZ_ZIP_STREAM_STATUS_NO_OUTPUT_SPACE    = 4,
    MZ_ZIP_STREAM_STATUS_UNKNOWN_ERROR      = 5 
} MzZipStreamStatus;

MzZipStream      *mz_zip_stream_create        (const char *password);
MzZipStreamStatus mz_zip_stream_begin_archive (MzZipStream *zip);
MzZipStreamStatus mz_zip_stream_begin_file    (MzZipStream  *zip,
                                               const char   *filename,
                                               const char   *entire_data,
                                               unsigned int  entire_data_size,
                                               char         *output_buffer,
                                               unsigned int  output_buffer_size,
                                               unsigned int *written_size);
MzZipStreamStatus mz_zip_stream_process_file_data
                                              (MzZipStream  *zip,
                                               const char   *input_buffer,
                                               unsigned int  input_buffer_size,
                                               char         *output_buffer,
                                               unsigned int  output_buffer_size,
                                               unsigned int *processed_size,
                                               unsigned int *written_size);
MzZipStreamStatus mz_zip_stream_end_file      (MzZipStream  *zip,
                                               char         *output_buffer,
                                               unsigned int  output_buffer_size,
                                               unsigned int *written_size);
MzZipStreamStatus mz_zip_stream_end_archive   (MzZipStream  *zip,
                                               char         *output_buffer,
                                               unsigned int  output_buffer_size,
                                               unsigned int *written_size);
void              mz_zip_stream_destroy       (MzZipStream  *zip);

#endif /* __MZ_ZIP_H__ */

