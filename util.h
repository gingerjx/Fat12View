#ifndef UTIL_H__
#define UTIL_H__
#include "fat_structs.h"
#define MAX_PATH 100
#define FILENAME_NUM 8
#define EXT_NUM 3

typedef struct{
  uint16_t actual_pos;
  uint16_t table_pos;
  uint16_t root_pos;
  uint16_t clusters_pos;
  uint16_t end_pos;
  uint16_t spc;           //sector_per_cluster
  uint16_t bps;           //bytes_per_sector
  uint16_t epc;           //entries_per_cluster
  char path[MAX_PATH];
  FILE *f;
} position_t;

void print_info(const param_t *prm);
void print_positions(const position_t *ps);

void seek_to(position_t *pos, unsigned int p);  // p in secotors
void seek_actual(position_t *pos);
void seek_table(position_t *pos);
void seek_root(position_t *pos);
void seek_clusters(position_t *pos);

typedef struct{
    position_t pos;  // all about position
    reserved_t rsv;  // reserved_area
    uint8_t *table;
} FAT_t;

void open_fat(FAT_t *ft, const char *fn);

void print_directory(FAT_t *ft);
void print_dir(FAT_t *ft);
void print_root(FAT_t *ft);
void print_entry(root_dir_t *rt);
void print_name(char *nm);
void print_mod_date(uint16_t date);
void print_mod_time(uint16_t time);

int is_read_only(uint8_t attr);
int is_hidden(uint8_t attr);
int is_system_file(uint8_t attr);
int is_volume_label(uint8_t attr);
int is_lfn(uint8_t attr);
int is_dir(uint8_t attr);
int is_archive(uint8_t attr);

uint16_t sec2cl(uint16_t sector, const position_t *pos);  //  convert sector to cluster
uint16_t cl2sec(uint16_t cluster, const position_t *pos); // convert cluster to sectors
uint16_t index_value(uint16_t cluster, const FAT_t *ft);  // return value under cluster

int cmd_cd(const char *dir_path, FAT_t *ft);
void move_to_precedent(FAT_t *ft);
const char *extract_name(const char *path, char *nm); //return shifted path
int move_to_dir(const char *dir_nm, FAT_t *ft);
int look_for_dir(unsigned int entries_num, const char *dir_nm, FAT_t *ft);
char *get_dir_name(const root_dir_t *rt, char *nm);
int names_match(const char *s1, const char *s2);

int cmd_cat(const char *fn, FAT_t *ft);
int cmd_get(const char *fn, FAT_t *ft);
int cmd_cd_or_get(const char *fn, FAT_t *ft);
char *find_file(const char *fn, FAT_t *ft, char *name);
int look_for_file(unsigned int entries_num, const char *fn, FAT_t *ft);
char *get_file_name(const root_dir_t *rt, char *nm);
void file_operation(const root_dir_t *rt, FAT_t *ft);
void print_cluster(unsigned int max_size, unsigned to_read, FAT_t *ft );
void copy_file(FILE *f, unsigned int max_size, unsigned to_read, FAT_t *ft );

int cmd_zip(const char *fn_in1, const char *fn_in2, const char *fn_out, FAT_t *ft);
int look_in_directory(root_dir_t *entry, const char *fn, FAT_t *ft);
int look_for_file_zip(unsigned int entries_num, root_dir_t *entry, const char *fn, FAT_t *ft);
void zip_entries(root_dir_t *en1, root_dir_t *en2, const char *out, FAT_t *ft);
unsigned int copy_line(FILE *f, unsigned int max_size, unsigned int to_read, FAT_t *ft );

void rootinfo(FAT_t *ft);

void spaceinfo(FAT_t *ft);

int fileinfo(const char *fn, FAT_t *ft);
void print_file(const root_dir_t *en, const char *fn, FAT_t *ft);
void print_file_clusters(const root_dir_t *en, FAT_t *ft);

#endif
