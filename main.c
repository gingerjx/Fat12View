/*
  Basic implementation of fatview where you can look at your FAT12 disk image
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

void convert_to_cmd_zip(const char *cmd, FAT_t *ft);
int is_fileinfo(const char *cmd);
void print_help();

int main(int argc, char **argv){
  if ( argc != 2 ){
    printf("Pass name of disk image as command line argument\n");
    return 1;
  }
  FAT_t fat;
  open_fat(&fat, argv[1]);
  char cmd[100];
  print_info(&fat.rsv.prms);
  print_positions(&fat.pos);
  printf("Type 'help' to see available commands\n\n");
  while(1){
      printf("<%s>$ ", fat.pos.path);
      fgets(cmd,100,stdin);
      cmd[strlen(cmd)-1] = '\0';

      if ( *cmd == '\0' );
      else if ( strcmp(cmd,"exit") == 0 ) break;
      else if ( strcmp(cmd,"help") == 0 ) print_help();
      else if ( strcmp(cmd,"pwd") == 0 )
        printf("%s\n", fat.pos.path );
      else if ( cmd[0] == 'c' && cmd[1] == 'd' && cmd[2] == ' ')
        cmd_cd(cmd+3,&fat);
      else if ( strcmp(cmd,"dir") == 0 )
        print_directory(&fat);
      else if ( cmd[0] == 'c' && cmd[1] == 'a' && cmd[2] == 't' && cmd[3] == ' ' )
        cmd_cat(cmd+4,&fat);
      else if ( cmd[0] == 'g' && cmd[1] == 'e' && cmd[2] == 't' && cmd[3] == ' ' )
        cmd_get(cmd+4,&fat);
      else if ( cmd[0] == 'z' && cmd[1] == 'i' && cmd[2] == 'p' && cmd[3] == ' ' )
        convert_to_cmd_zip(cmd+4,&fat);
      else if ( strcmp(cmd,"rootinfo") == 0 )
        rootinfo(&fat);
      else if ( strcmp(cmd,"spaceinfo") == 0 )
        spaceinfo(&fat);
      else if ( is_fileinfo(cmd) )
        fileinfo(cmd+9,&fat);
      else printf("Unknown command\n");
  }
  free(fat.table);
  fclose(fat.pos.f);
  return 0;
}

void convert_to_cmd_zip(const char *cmd, FAT_t *ft){
  char path1[MAX_PATH + 1];
  char path2[MAX_PATH + 1];
  char out_name[MAX_PATH + 1];
  int i=0;

  for (; cmd[i]; ++i){
    if ( cmd[i] != ' ' )
      path1[i] = cmd[i];
    else{
      path1[i] = '\0';
      break;
    }
  }

  for (int j=0; cmd[i]; ++j){
    i++;
    if ( cmd[i] != ' ' )
      path2[j] = cmd[i];
    else{
      path2[j] = '\0';
      break;
    }
  }

  for (int j=0; cmd[i]; ++j){
    i++;
    if ( cmd[i] != ' ' )
      out_name[j] = cmd[i];
    else{
      out_name[j] = '\0';
      break;
    }
  }

  cmd_zip(path1,path2,out_name,ft);
}
int is_fileinfo(const char *cmd){
  if ( strlen(cmd) < 10 )
    return 0;
  char *temp = "fileinfo ";
  for (int i=0; i<9; ++i){
    if ( cmd[i] != temp[i] )
      return 0;
  }
  return 1;
}
void print_help(){
  printf("  'exit' - exit fatview\n");
  printf("  'pwd' - display current path\n");
  printf("  'cd' <name> - change current directory (e.g ./PO, /root/PO, ..)\n");
  printf("  'dir' - display list of files and directories of current directory\n");
  printf("  'cat' <name> - display content of given file\n");
  printf("  'get' <name> - copy given file to current directory of operating system\n");
  printf("  'zip' <name1> <name2> <name3> - zip two first files into third one and save it in current directory of operating system\n");
  printf("  'rootinfo' - display information about root directory\n");
  printf("  'spaceinfo' - display information about cluster area in given FAT12\n");
  printf("  'fileinfo' <name> - display information about given file\n");
}
