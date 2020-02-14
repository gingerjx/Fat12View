#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "fat_structs.h"
#include "util.h"

typedef unsigned int ut;
enum operation {CAT,GET} OPERATION;

void print_info(const param_t *prm){
  printf("About this FAT:\n------------------\n");
  printf("Bytes per sector: %5d\n", prm->bytes_per_sector);
  printf("Sectors per cluster: %2d\n", prm->sectors_per_cluster);
  printf("Size of one FAT: %6d\n", prm->table_size);
  printf("Number of Fats: %7d\n\n", prm->tables_num);
}
void print_positions(const position_t *ps){
  printf("Positions [in sectors]:\n------------------\n");
  printf("Reserved: %15d\n", 0);
  printf("First FAT: %14d\n", ps->table_pos);
  printf("Root directory: %9d\n", ps->root_pos);
  printf("Clusters area: %10d\n", ps->clusters_pos);
  printf("End of file system: %d\n\n", ps->end_pos);
}

void seek_to(position_t *pos, ut p){
  pos->actual_pos = p; fseek(pos->f, p*pos->bps, SEEK_SET); }
void seek_actual(position_t *pos){ fseek(pos->f, pos->actual_pos*pos->bps, SEEK_SET); }
void seek_table(position_t *pos){ fseek(pos->f, pos->table_pos*pos->bps, SEEK_SET); }
void seek_root(position_t *pos){ fseek(pos->f, pos->root_pos*pos->bps, SEEK_SET); }
void seek_clusters(position_t *pos){ fseek(pos->f, pos->clusters_pos*pos->bps, SEEK_SET); }

void open_fat(FAT_t *ft, const char *fn){
  position_t *ps = &ft->pos;
  ps->f = fopen(fn,"rb");
  assert( ps->f != NULL );
  assert( fread(&ft->rsv, sizeof(reserved_t), 1, ps->f) != 0 );

// position init
  param_t *pr = &ft->rsv.prms;
  ps->bps = pr->bytes_per_sector;
  ps->spc = pr->sectors_per_cluster;
  ps->epc = (ps->spc*ps->bps)/sizeof(root_dir_t);
  ps->table_pos = pr->reserved_t_size;
  ps->root_pos = ps->table_pos + pr->tables_num * pr->table_size;
  ps->actual_pos = ps->root_pos;
  ps->clusters_pos = ps->root_pos + ( sizeof(root_dir_t) * pr->root_entries ) / ps->bps;
  ps->end_pos = pr->all_sectors;
  ps->actual_pos = ps->root_pos;
  strcpy(ps->path,"/root");

// FAT table init and checking copies
  seek_table(ps);
  uint16_t alloc_size = pr->table_size * ps->bps;
  ft->table = (uint8_t *)malloc( sizeof(uint8_t)*alloc_size );
  assert( ft->table != NULL );
  assert( fread(ft->table,sizeof(uint8_t),alloc_size,ps->f) != 0 );

  uint8_t *temp_table;
  for (int i=0; i<pr->tables_num-1; ++i){
    temp_table = (uint8_t *)malloc( sizeof(uint8_t)*alloc_size );
    assert( temp_table != NULL );
    assert( fread(temp_table, sizeof(uint8_t), alloc_size, ps->f) != 0 );
    assert( memcmp(ft->table, temp_table, alloc_size) == 0 );
    free(temp_table);
  }
}

void print_directory(FAT_t *ft){
  if ( ft->pos.actual_pos == ft->pos.root_pos ){    //print root
    print_root(ft); return; }
// print another directory
  uint16_t first_cluster = sec2cl(ft->pos.actual_pos,&ft->pos);
  int next_cluster = index_value(first_cluster, ft);
  while(1){
      print_dir(ft);
    if ( next_cluster == 0x0001 || 0xfff7 ) break;
      seek_to(&ft->pos, cl2sec(next_cluster, &ft->pos) );
    if( next_cluster >= 0xfff8 ) break;
  }
  seek_to(&ft->pos, cl2sec(first_cluster,&ft->pos) );
}
void print_root(FAT_t *ft){
  root_dir_t entry;
  const param_t *pr = &ft->rsv.prms;
  for (int i=0; i<pr->root_entries; ++i){
    assert( fread(&entry,sizeof(root_dir_t),1,ft->pos.f) != 0 );
    if ( entry.fname[0] == 0x0 ) break;
    else if ( entry.fname[0] >= 0xE5 || entry.fname[0] < 0x0 ) continue;
    else if ( is_lfn(entry.attributes) || is_volume_label(entry.attributes) ) continue;
    print_entry(&entry);
  }
  seek_root(&ft->pos);
}
void print_dir(FAT_t *ft){
  root_dir_t entry;
  for (int i=0; i<ft->pos.epc; ++i){
    assert( fread(&entry,sizeof(root_dir_t),1,ft->pos.f) != 0 );
    if ( entry.fname[0] == 0x0 ) break;
    else if ( entry.fname[0] >= 0xE5 || entry.fname[0] < 0x0 ) continue;
    else if ( is_lfn(entry.attributes) || is_volume_label(entry.attributes) ) continue;
    print_entry(&entry);
  }
  return;
}
void print_entry(root_dir_t *rt){
  print_mod_date(rt->mod_date);
  print_mod_time(rt->mod_time);
  if ( is_dir(rt->attributes) ) printf(" <DIRECTORY>  ");
  else printf(" %11d  ", rt->f_size );
  print_name(rt->fname);
  printf("\n");
}
void print_name(char *nm){
  int i=0;
  while(i < FILENAME_NUM && nm[i] != ' ')
    printf("%c",nm[i++]);
  if (nm[FILENAME_NUM] == ' ') return;
  i = FILENAME_NUM-1;
  printf(".");
  while(i++ != FILENAME_NUM + EXT_NUM)
    printf("%c",nm[i]);
}
void print_mod_date(uint16_t date){
  uint8_t day = date & 31;
  uint8_t month = (date >> 5 ) & 15;
  uint16_t year = ( (date >> 9)  & 127 )  +  1980;
  printf(" %02u-%02u-%04u  ", day, month, year);
}
void print_mod_time(uint16_t time){
  uint8_t second = (time & 31) * 2;
  uint8_t minute = (time >> 5) & 63;
  uint16_t hour = (time>> 11) & 31;
  printf(" %02d:%02d:%02d  ", hour, minute, second);
}

int is_read_only(uint8_t attr) { return (attr & 0x01); }
int is_hidden(uint8_t attr) { return (attr & 0x02); }
int is_system_file(uint8_t attr) { return (attr & 0x04); }
int is_volume_label(uint8_t attr) { return (attr & 0x08); }
int is_lfn(uint8_t attr) { return (attr & 0x0f); }
int is_dir(uint8_t attr) { return (attr & 0x10); }
int is_archive(uint8_t attr) { return (attr & 0x20); }

uint16_t sec2cl(uint16_t sector, const position_t *pos){
  return (sector - pos->clusters_pos)/pos->spc + 2;
}
uint16_t cl2sec(uint16_t cluster, const position_t *pos){
  return (cluster-2)*pos->spc + pos->clusters_pos;
}
uint16_t index_value(uint16_t cluster, const FAT_t *ft){
  if ( cluster % 2 == 0 ){
    int L_idx = (3*cluster)/2;
    int R_idx = L_idx + 1;
    uint16_t ret = ( ( (uint16_t)( ft->table[R_idx] & 0x0f ) ) << 8 ) | ft->table[L_idx];
    return ret;
  } else{
    int L_idx = cluster + (cluster-3)/2 + 1;
    int R_idx = L_idx + 1;
    uint16_t ret = ( (uint16_t)(ft->table[R_idx]) << 4 ) | ( ( ft->table[L_idx] >> 4) & 0x0f );
    return ret;
  }
}

int cmd_cd(const char *dir_path, FAT_t *ft){
  char name[FILENAME_NUM+1];
  const char *path;
  ut start_pos = ft->pos.actual_pos;
  char start_path[MAX_PATH+1];
  strcpy(start_path,ft->pos.path);

  if ( strcmp(dir_path,"/root") == 0){              //go to root
    ft->pos.actual_pos = ft->pos.root_pos;
    seek_root(&ft->pos);
    ft->pos.path[5] = '\0';
    return 1;
  }
  if ( dir_path[0] == '.' && dir_path[1] == '.'){   // go to precedent
    int i=0;
    while( dir_path[i] == '.' && dir_path[i+1] == '.' ){
      move_to_precedent(ft);
      i += 3;
      if ( dir_path[i] == '\0' || dir_path[i+1] == '\0' ) return 1;
    }
    char new_path[MAX_PATH + 1];
    new_path[0] = '.';
    strcpy(new_path+1,dir_path+i-1);
    return cmd_cd(new_path,ft);
  }

  if ( dir_path[0] == '.' && dir_path[1] == '/' ){   // relative path
    path = dir_path+2; }
  else if (dir_path[0] == '/' ){                                             // absolute path
    ft->pos.path[5] = '\0';
    path = dir_path + 6;
    seek_root(&ft->pos);
  } else{
      printf("Cannot find \'%s\' directory\n", dir_path);
      return 0;
  }

  int result;
  while( *path != ' ' && *path != '\0' && *path != '.'){
    path = extract_name(path,name);
    result = move_to_dir(name,ft);
    if ( result == 0 ){
      printf("Cannot find \'%s\' directory\n", dir_path);
      strcpy(ft->pos.path,start_path);
      seek_to(&ft->pos,start_pos);
      return 0;
    }
  }

  return 1;
}
void move_to_precedent(FAT_t *ft){
  char precedent_path[MAX_PATH+1];
  if (ft->pos.actual_pos == ft->pos.root_pos)
    return;
  for (int i=0; ft->pos.path[i] ; ++i)
    precedent_path[i] = tolower(ft->pos.path[i]);
  for (int i=strlen(precedent_path); i>=0; --i){
    if (precedent_path[i] == '/'){
      precedent_path[i] = '\0';
      break;
    }
  }
  cmd_cd(precedent_path,ft);
  return;
}
const char *extract_name(const char *path, char *nm){
  int i=0;
  while( path[i] != '/' && path[i] != ' ' && path[i] != '\0' ){
    nm[i] = path[i];
    i++;
  }
  nm[i] = '\0';
  if ( path[i] == '\0' ) return path + i;
  return path + i + 1;
}
int move_to_dir(const char *dir_nm, FAT_t *ft){
  ut start_pos = ft->pos.actual_pos;
  if ( ft->pos.actual_pos == ft->pos.root_pos ){      // search in root directory
      if ( look_for_dir(ft->rsv.prms.root_entries,dir_nm,ft) == 0 )
        return 1;
      seek_to(&ft->pos,start_pos);
      return 0;
  }
// search in some directory
  uint16_t first_cluster = sec2cl(ft->pos.actual_pos,&ft->pos);
  int next_cluster = index_value(first_cluster, ft);
  ut entries_per_cluster = (ft->pos.spc * ft->pos.bps)/sizeof(root_dir_t);
  int founded = look_for_dir(entries_per_cluster,dir_nm,ft);

  if ( founded == 0 ) return 1;
  else if ( founded == 2 ){ seek_to(&ft->pos,start_pos); return 0; }

  while(1){
    if ( !(next_cluster >= 0x002 && next_cluster<= 0xff6) )
      break;
    seek_to(&ft->pos, cl2sec(next_cluster, &ft->pos) );
    founded = look_for_dir(entries_per_cluster,dir_nm,ft);
    if ( founded == 0 ) return 1;
    else if ( founded == 2 ) break;
    next_cluster = index_value(next_cluster, ft);
  }

  seek_to(&ft->pos,start_pos);
  return 0;
}
int look_for_dir(ut entries_num, const char *dir_nm, FAT_t *ft){
  root_dir_t entry;
  char name[FILENAME_NUM+1];

  for (int i=0; i<entries_num; ++i){
    assert( fread(&entry,sizeof(root_dir_t),1,ft->pos.f) != 0 );
    if ( entry.fname[0] == 0x0 ) return 2;
    else if ( entry.fname[0] >= 0xE5 || entry.fname[0] < 0x0 ) continue;
    else if ( is_lfn(entry.attributes) || is_volume_label(entry.attributes) ) continue;

    if ( names_match( get_dir_name(&entry,name), dir_nm ) ){
      if ( strlen(ft->pos.path) >= MAX_PATH-1 ){
        printf("Path is too long\n"); return 0; }

      strcat(ft->pos.path,"/");
      ut max_append = MAX_PATH - strlen(ft->pos.path);
      strncat(ft->pos.path, get_dir_name(&entry,name), max_append);

      ft->pos.actual_pos = cl2sec(entry.LO_first_cluster,&ft->pos);
      seek_to(&ft->pos, ft->pos.actual_pos );

      return 0;
    }
  }

  return 1;
}
char *get_dir_name(const root_dir_t *rt, char *nm){
  if ( rt->fname[FILENAME_NUM] != ' ' )
    nm[0] = '\0';
  else{
    int i=0;
    for (; i<FILENAME_NUM; ++i){
      if ( rt->fname[i] == ' ' ) break;
      nm[i] = rt->fname[i];
    }
    nm[i] = '\0';
  }
  return nm;
}
int names_match(const char *s1, const char *s2){
  if ( strlen(s1) != strlen(s2) )
    return 0;
  int len = strlen(s1);
  for (int i=0; i<len; ++i){
    if ( tolower(s1[i]) != tolower(s2[i]) )
      return 0;
  }
  return 1;
}

int cmd_cat(const char *fn, FAT_t *ft){
    OPERATION = CAT;
    return cmd_cd_or_get(fn,ft);
}
int cmd_get(const char *fn, FAT_t *ft){
  OPERATION = GET;
  return cmd_cd_or_get(fn,ft);
}
int cmd_cd_or_get(const char *fn, FAT_t *ft){
  ut start_pos = ft->pos.actual_pos;
  char actual_path[MAX_PATH+1];
  strcpy(actual_path,ft->pos.path);

  char name[FILENAME_NUM+EXT_NUM+1];      //find file in path
  find_file(fn,ft,name);
  if( name == NULL ){
    seek_to(&ft->pos,start_pos);
    strcpy(ft->pos.path,actual_path);
    return 0;
  }

  if ( start_pos == ft->pos.root_pos ){   //check in root
    int res = look_for_file(ft->rsv.prms.root_entries, name, ft);
    seek_to(&ft->pos,start_pos);
    strcpy(ft->pos.path,actual_path);
    if ( res != 0 ){
      printf("There's no file \'%s\' in current directory \'%s\'\n", fn, ft->pos.path );
      return 0;
    }
    return 1;
  }

// check in another directory TODO
  int entries_num = start_pos == ft->pos.root_pos ? ft->rsv.prms.root_entries : ft->pos.epc;
  uint16_t first_cluster = sec2cl(ft->pos.actual_pos,&ft->pos);
  int next_cluster = index_value(first_cluster, ft);
  int founded = look_for_file(entries_num, name, ft);

  if ( founded == 2 ){
    printf("There's no file \'%s\' in current directory \'%s\'\n", fn, ft->pos.path );
    strcpy(ft->pos.path,actual_path);
    seek_to(&ft->pos,start_pos);
    return 0;
  }
  if ( founded == 0 ) {
    strcpy(ft->pos.path,actual_path);
    seek_to(&ft->pos,start_pos);
    return 1;
  }

  while(1){
    if ( !(next_cluster >= 0x002 && next_cluster<= 0xff6) )
      break;
    seek_to(&ft->pos, cl2sec(next_cluster, &ft->pos) );
    founded = look_for_file(entries_num, name, ft);
    if ( founded == 2 ) break;
    if ( founded == 0 ) {
      strcpy(ft->pos.path,actual_path);
      seek_to(&ft->pos,start_pos);
      return 1;
    }
    next_cluster = index_value(next_cluster, ft);
  }

  printf("There's no file \'%s\' in current directory \'%s\'\n", fn, ft->pos.path );
  strcpy(ft->pos.path,actual_path);
  seek_to(&ft->pos,start_pos);
  return 0;
}
char *find_file(const char *fn, FAT_t *ft, char *name){
  if ( strchr(fn,' ') )
    return NULL;
  if ( fn[0] != '.' && fn[0] != '/' ){
    strcpy(name,fn);
    return name;
  }

  char go_to[MAX_PATH+1];
  strcpy(go_to,fn);
  ut fn_size = -1;
  for (int i=strlen(fn); i>=0; --i, fn_size++){
    if ( fn[i] == '/' )
      break;
  }

  ut shift = strlen(fn) - fn_size;
  strcpy(name,fn+shift);
  go_to[shift] = '\0';
  if ( cmd_cd(go_to,ft) )
    name = NULL;

  return name;
}
int look_for_file(ut entries_num, const char *fn, FAT_t *ft){
  root_dir_t entry;
  char name[FILENAME_NUM+EXT_NUM+1];
  for (int i=0; i<entries_num; ++i){
    assert( fread(&entry,sizeof(root_dir_t),1,ft->pos.f) != 0 );
    if ( entry.fname[0] == 0x0 ) return 2;
    else if ( entry.fname[0] >= 0xE5 || entry.fname[0] < 0x0 ) continue;
    else if ( is_lfn(entry.attributes) || is_volume_label(entry.attributes) ) continue;
    if ( entry.f_size == 0 ) continue;


    if ( names_match( get_file_name(&entry,name), fn ) ){
        file_operation(&entry,ft);
        return 0;
    }
  }

  return 1;
}
char *get_file_name(const root_dir_t *rt, char *nm){
  int i=0;
  for (; i<FILENAME_NUM; ++i){
    if ( rt->fname[i] == ' ' ) break;
    nm[i] = rt->fname[i];
  }
  nm[i++] = '.';
  for (int j=0; j<EXT_NUM; ++j){
    if (rt->fname[FILENAME_NUM+j] != ' ')
      nm[i++] = rt->fname[FILENAME_NUM+j];
  }
  if ( rt->fname[FILENAME_NUM] == ' ' ){
    nm[FILENAME_NUM] = '\0';
    return nm;
  }
  nm[i] = '\0';
  return nm;
}
void file_operation(const root_dir_t *rt, FAT_t *ft){
  ut first_cluster =  rt->LO_first_cluster;
  ut next_cluster = index_value(first_cluster, ft);
  ut cluster_size = ft->pos.spc * ft->pos.bps;
  ut file_size = rt->f_size;
  ut read_size = 0;
  FILE *f;

  if ( OPERATION == GET ){
    char fn[FILENAME_NUM+EXT_NUM+1];
    get_file_name(rt,fn);
    f = fopen(fn,"w");
    assert( f != NULL );
  }

  seek_to(&ft->pos, cl2sec(first_cluster,&ft->pos) );
  if ( OPERATION == CAT ) print_cluster(cluster_size, file_size, ft);
  if ( OPERATION == GET ) copy_file(f,cluster_size, file_size, ft);
  read_size += cluster_size;

  while(1){
    if ( read_size >= file_size ) break;
    if ( !(next_cluster >= 0x002 && next_cluster<= 0xff6) )
      break;
    seek_to(&ft->pos, cl2sec(next_cluster, &ft->pos) );
    if ( OPERATION == CAT ) print_cluster(cluster_size, file_size-read_size, ft);
    if ( OPERATION == GET ) copy_file(f,cluster_size, file_size, ft);
    read_size += cluster_size;
    next_cluster = index_value(next_cluster, ft);
  }

  if ( OPERATION == GET )
    fclose(f);
}
void print_cluster(ut max_size, ut to_read, FAT_t *ft ){
  ut len = max_size > to_read ? max_size : to_read;
  for (int i=0; i<len; ++i)
    printf("%c", fgetc(ft->pos.f) );
}
void copy_file(FILE *f, ut max_size, ut to_read, FAT_t *ft ){
  ut len = max_size > to_read ? max_size : to_read;
  char c;
  for (int i=0; i<len; ++i){
    c = fgetc(ft->pos.f);
    fwrite(&c,sizeof(char),1,f);
  }
}

int cmd_zip(const char *fn_in1, const char *fn_in2, const char *fn_out, FAT_t *ft){
  ut start_pos = ft->pos.actual_pos;
  char actual_path[MAX_PATH+1];
  strcpy(actual_path,ft->pos.path);
  char name[FILENAME_NUM+EXT_NUM+1];      //find files in paths
  root_dir_t en_in1, en_in2;

  find_file(fn_in1,ft,name);              //find first file
  int res = 0;
  if ( name != NULL ){
    if ( ft->pos.actual_pos == ft->pos.root_pos )
      res = look_for_file_zip(ft->rsv.prms.root_entries, &en_in1, name, ft);
    else
      res = look_in_directory(&en_in1,name,ft);
  }

  if ( res != 0 ){
    printf("Cannot find \'%s/%s\'\n", ft->pos.path, name);
    seek_to(&ft->pos,start_pos);
    strcpy(ft->pos.path,actual_path);
    return 0;
  }

  seek_to(&ft->pos,start_pos);
  strcpy(ft->pos.path,actual_path);
  find_file(fn_in2,ft,name);              //find second file
  res = 0;
  if ( name != NULL ){
    if ( ft->pos.actual_pos == ft->pos.root_pos ){
        res = look_for_file_zip(ft->rsv.prms.root_entries, &en_in2, name, ft); }
    else
      res = look_in_directory(&en_in2,name,ft);
  }

  if ( res == 0 )
    zip_entries(&en_in1,&en_in2,fn_out,ft);
  else printf("Cannot find \'%s/%s\'\n", ft->pos.path, name);
  seek_to(&ft->pos,start_pos);
  strcpy(ft->pos.path,actual_path);
  return res;
}
int look_in_directory(root_dir_t *entry, const char *fn, FAT_t *ft){
  ut first_cluster = sec2cl(ft->pos.actual_pos, &ft->pos);
  ut next_cluster = index_value(first_cluster,ft);
  int founded = look_for_file_zip(ft->pos.epc, entry, fn, ft);

  if ( founded == 2 )
    return 1;
  if ( founded == 0 )
    return 0;

  while(1){
    if ( !(next_cluster >= 0x002 && next_cluster<= 0xff6) )
      break;
    seek_to(&ft->pos, cl2sec(next_cluster, &ft->pos) );
    founded = look_for_file_zip(ft->pos.epc, entry, fn, ft);
    if ( founded == 2 )
      return 1;
    if ( founded == 0 )
      return 0;
    next_cluster = index_value(next_cluster,ft);
  }

  return 1;
}
int look_for_file_zip(ut entries_num, root_dir_t *entry, const char *fn, FAT_t *ft){
  char name[FILENAME_NUM+EXT_NUM+1];
  for (int i=0; i<entries_num; ++i){
    assert( fread(entry,sizeof(root_dir_t),1,ft->pos.f) != 0 );
    if ( entry->fname[0] == 0x0 ) return 2;
    else if ( entry->fname[0] >= 0xE5 || entry->fname[0] < 0x0 ) continue;
    else if ( is_lfn(entry->attributes) || is_volume_label(entry->attributes) ) continue;
    if ( entry->f_size == 0 ) continue;

    if ( names_match( get_file_name(entry,name), fn ) )
        return 0;
  }
  return 1;
}
void zip_entries(root_dir_t *en1, root_dir_t *en2, const char *out, FAT_t *ft){
  FILE *f = fopen(out,"w");   //to do
  assert( f != NULL );
  ut cluster1 = en1->LO_first_cluster;
  ut cluster2 = en2->LO_first_cluster;
  ut cluster_size = ft->pos.spc * ft->pos.bps;
  ut size1 =  en1->f_size;
  ut size2 =  en2->f_size;
  ut read1 = 0;
  ut read2 = 0;
  char endline = '\n';

  while(1){
    if ( read1 >= size1 && read2 >= size2 )
      break;
    if ( read1 < size1 && read1 != 0 && read1 % cluster_size == 0 )
      cluster1 = index_value(cluster1, ft);
    if ( read2 < size2 && read2 != 0 && read2 % cluster_size == 0 )
      cluster2 = index_value(cluster2, ft);

    if ( read1 < size1 ){                         //file 1
      seek_to(&ft->pos, cl2sec(cluster1,&ft->pos) );
      fseek(ft->pos.f, read1, SEEK_CUR);
      read1 += copy_line(f,cluster_size,size1-read1,ft);
      fwrite(&endline,sizeof(char),1,f);
    }

    if ( read2 < size2 ){                          //file 2
      seek_to(&ft->pos, cl2sec(cluster2,&ft->pos) );
      fseek(ft->pos.f, read2, SEEK_CUR);
      read2 += copy_line(f,cluster_size,size2-read2,ft);
      fwrite(&endline,sizeof(char),1,f);
    }
  }

  fclose(f);
}
ut copy_line(FILE *f, ut max_size, ut to_read, FAT_t *ft ){
  ut len = max_size > to_read ? max_size : to_read;
  char c;
  ut read = 0;
  for (int i=0; i<len; ++i){
    c = fgetc(ft->pos.f);
    read++;
    if ( c == '\n' ) break;
    fwrite(&c,sizeof(char),1,f);
  }
  return read;
}

void rootinfo(FAT_t *ft){
  ut start_pos = ft->pos.actual_pos;
  char actual_path[MAX_PATH+1];
  strcpy(actual_path,ft->pos.path);
  seek_root(&ft->pos);

  root_dir_t en;
  ut num_of_ens = 0;
  for (int i=0; i<ft->rsv.prms.root_entries; ++i){
    assert( fread(&en,sizeof(root_dir_t),1,ft->pos.f) != 0 );
    if ( en.fname[0] == 0x0 ) break;
    else if ( en.fname[0] >= 0xE5 || en.fname[0] < 0x0 ) continue;
    else if ( is_lfn(en.attributes) || is_volume_label(en.attributes) ) continue;

    num_of_ens++;
  }

  printf(" Number of entries: %u\n", num_of_ens);
  printf(" Max number of entries: %u\n", ft->rsv.prms.root_entries);
  double ss = (num_of_ens*1.0/ft->rsv.prms.root_entries);
  printf(" Root direcotry fill in %lf%c \n", ss * 100, '%' );

  seek_to(&ft->pos,start_pos);
  strcpy(ft->pos.path,actual_path);
}

void spaceinfo(FAT_t *ft){
  param_t *prm = &ft->rsv.prms;
  uint8_t left, right;
  uint16_t occupied = 0;
  uint16_t free = 0;
  uint16_t broken = 0;
  uint16_t end = 0;

  for (int i=2; i< (prm->table_size*ft->pos.bps) ; i += 2){
    if ( (ft->pos.end_pos - ft->pos.clusters_pos)/ft->pos.spc <= free + occupied + broken + end )
      break;
    left = ft->table[2*i];
    right = ft->table[2*i + 1];
    uint16_t value = ( (uint16_t)right << 8 ) + (uint16_t)left;
    if ( value == 0x000 ) free++;
    else if ( value >= 0x002 && value <= 0xff6 ) occupied++;
    else if ( value == 0xff7 ) broken++;
    else if ( value >= 0xff8 && value <= 0xfff ) end++;
  }

  printf(" Occupied clusters: %u\n", occupied);
  printf(" Free clusters: %u\n", free);
  printf(" Broken clusters: %u\n", broken);
  printf(" End clusters: %u\n", end);
  printf(" Cluster size in sectors: %u\n", ft->pos.spc);
  printf(" Cluster size in bytes: %u\n", ft->pos.spc*ft->pos.bps );
}

int fileinfo(const char *fn, FAT_t *ft){
  ut start_pos = ft->pos.actual_pos;
  char actual_path[MAX_PATH+1];
  strcpy(actual_path,ft->pos.path);
  char name[FILENAME_NUM+EXT_NUM+1];      //find files in paths
  root_dir_t en;

  find_file(fn,ft,name);              //find first file
  int res = 0;
  if ( name != NULL ){
    if ( ft->pos.actual_pos == ft->pos.root_pos )
      res = look_for_file_zip(ft->rsv.prms.root_entries, &en, name, ft);
    else
      res = look_in_directory(&en,name,ft);
  }

  if ( res != 0 )
    return 0;
  print_file(&en,name,ft);
  seek_to(&ft->pos,start_pos);
  strcpy(ft->pos.path,actual_path);
  return 1;
}
void print_file(const root_dir_t *en, const char *fn, FAT_t *ft){
  printf("Full name: %s/%s\n", ft->pos.path, fn);
    if (is_read_only(en->attributes)) printf("R+ "); else printf("R- ");
    if (is_hidden(en->attributes)) printf("H+ "); else printf("H- ");
    if (is_system_file(en->attributes)) printf("S+ "); else printf("S- ");
    if (is_volume_label(en->attributes)) printf("V+ "); else printf("V- ");
    if (is_lfn(en->attributes)) printf("L+ "); else printf("L- ");
    if (is_dir(en->attributes)) printf("D+ "); else printf("D- ");
    if (is_archive(en->attributes)) printf("A+ \n"); else printf("A- \n");
  printf("File size: %u\n", en->f_size);
  printf("Last modification: "); print_mod_date(en->mod_date); print_mod_time(en->mod_time);
  printf("\nLast access: "); print_mod_date(en->access_date); print_mod_time(en->mod_time);
  printf("\nCreated: "); print_mod_date(en->create_date); print_mod_time(en->create_time_v2);
  printf("\n");
  print_file_clusters(en,ft);
}
void print_file_clusters(const root_dir_t *en, FAT_t *ft){
  uint16_t first_cluster = en->LO_first_cluster;
  uint16_t next_cluster = index_value(en->LO_first_cluster, ft);
  uint16_t num = 1;

  printf("Clusters chain: [%u]", first_cluster);
  while (1){
    if ( !(next_cluster >= 0x002 && next_cluster<= 0xff6) )
      break;
    printf(", %u", next_cluster);
    next_cluster =  index_value(next_cluster, ft);
    num++;
  }
  printf("\nNumber of cluster: %u\n", num);
}
