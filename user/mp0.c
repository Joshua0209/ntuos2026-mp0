#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

int count_char(const char *path, const char key) {
  int cnt = 0;
  for (int i = 0; i < strlen(path); i++)
    if (*(path + i) == key)
      cnt++;
  return cnt;
}

void walk(char *root, char key, int *dir_cnt, int *file_cnt) {
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(root, O_RDONLY)) < 0) {
    fprintf(2, "walk: cannot open %s\n", root);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "walk: cannot stat %s\n", root);
    close(fd);
    return;
  }

  switch (st.type) {
  case T_DEVICE:
  case T_FILE:
    (*file_cnt)++;
    break;

  case T_DIR:
    (*dir_cnt)++;
    if(strlen(root) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("walk: path too long\n");
      break;
    }
    strcpy(buf, root);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("walk: cannot stat %s\n", buf);
        continue;
      }
      printf("%s %d\n", buf, count_char(buf, key));
      walk(buf, key, dir_cnt, file_cnt);
    }
    break;
  }
  close(fd);
}

void mp0(char *root, char key) {
  struct stat st;
  int p[2];
  int dir_cnt = 0;
  int file_cnt = 0;

  if(stat(root, &st) < 0 || st.type != T_DIR){
    printf("%s [error opening dir]\n", root);
    printf("\n0 directories, 0 files\n");
    return;
  }
  
  printf("%s %d\n", root, count_char(root, key));
  
  pipe(p);
  if(fork() == 0){
    int child_dirs = -1;
    int child_files = 0;
    close(p[0]);
    walk(root, key, &child_dirs, &child_files);
    write(p[1], &child_dirs, sizeof(int));
    write(p[1], &child_files, sizeof(int));
    close(p[1]);
    exit(0);
  }

  close(p[1]);
  read(p[0], &dir_cnt, sizeof(int));
  read(p[0], &file_cnt, sizeof(int));
  wait(0);
  close(p[0]);
  printf("\n%d directories, %d files\n", dir_cnt, file_cnt);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("usage: mp0 <root_directory> <key>\n");
    exit(1);
  }
  mp0(argv[1], *argv[2]);
  exit(0);
}
