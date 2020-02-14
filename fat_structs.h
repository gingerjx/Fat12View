#ifndef FAT_STRUCTS_H__
#define FAT_STRUCTS_H__
#include <stdint.h>
#define FILENAME_NUM 8
#define EXT_NUM 3
#define MAX_PATH 100

#define FILENAME_NUM 8
#define EXT_NUM 3
#define MAX_PATH 100

typedef struct __attribute__((packed)) {       //0-35 bytes of FAT
  uint8_t assemb[3];
  char name[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_t_size;
  uint8_t tables_num;
  uint16_t root_entries;
  uint16_t all_sectors;
  uint8_t media_type;
  uint16_t table_size;
  uint16_t sector_per_track;
  uint16_t num_of_head;
  uint32_t num_of_sec_bef;
  uint32_t num_of_sec_v2;
} param_t;

typedef struct  __attribute__((packed)) {       //reserved_area
  param_t prms;
  uint8_t bios;
  uint8_t not_used;
  uint8_t val_next_three;
  uint16_t vol_ser_num[2];
  char vol_lab[11];
  char fat_type[8];
  uint8_t not_used_v2[448];
  uint16_t signature_value;
} reserved_t;

typedef struct __attribute__((packed)) {    //root_area
  char fname[FILENAME_NUM + EXT_NUM];
  uint8_t attributes;
  uint8_t not_used;
  uint8_t create_time;      //tenths of seconds
  uint16_t create_time_v2;  //hour, minute, seconds
  uint16_t create_date;
  uint16_t access_date;
  uint16_t HO_first_cluster;
  uint16_t mod_time;
  uint16_t mod_date;
  uint16_t LO_first_cluster;
  uint32_t f_size;          //0 for dirs, in bytes
} root_dir_t;



#endif
