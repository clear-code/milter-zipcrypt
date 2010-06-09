/* vim: set ts=4 sts=4 nowrap ai expandtab sw=4: */
#ifndef __MZ_ZIP_H__
#define __MZ_ZIP_H__

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
    unsigned char entry_size[2];
    unsigned char offset[4];
    unsigned char comment_length[2];
};

unsigned int mz_zip_compress      (const char  *data,
                                   unsigned int data_length,
                                   char       **compressed_data);
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

#endif /* __MZ_ZIP_H__ */

