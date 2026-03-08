#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

void print_err(const char *path) {
    printf("%s [error opening dir]\n", path);
    printf("\n0 directories, 0 files\n");
}

int count_char(const char *path, const char key) {
  int cnt = 0;
  for (int i = 0; i < strlen(path); i++)
    if (*(path + i) == key)
      cnt++;
  return cnt;
}

void ls(char* root, char key, int* dir_cnt, int* file_cnt) {
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;
  char *path = root;

  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "ls: cannot stat %s\n", path);
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
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      printf("%s %d\n", buf, count_char(buf, key));
      ls(buf, key, dir_cnt, file_cnt);
    }
    break;
  }
  close(fd);
}

void mp0(char *root, char key) {
  // TODO: implement mp0
  // Hint: Use pipe, fork, and walk
  int fd;
  struct stat st;
  char *path = root;
  int p[2];
  if ((fd = open(path, O_RDONLY)) < 0) {
    // fprintf(2, "ls: cannot open %s\n", path);
    print_err(path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    // fprintf(2, "ls: cannot stat %s\n", path);
    print_err(path);
    close(fd);
    return;
  }

  if (st.type != T_DIR) {
    print_err(path);
    return;
  }
  close(fd);
  printf("%s %d\n", root, count_char(path, key));
  pipe(p);
  if(fork() == 0){
    close(p[0]);
    int dir_cnt = -1;
    int file_cnt = 0;
    ls(path, key, &dir_cnt, &file_cnt);
    write(p[1], &dir_cnt, sizeof(int));
    write(p[1], &file_cnt, sizeof(int));
    close(p[1]);
    exit(0);
  }
  close(p[1]);
  int dir_cnt = 0;
  int file_cnt = 0;
  wait(0);
  read(p[0], &dir_cnt, sizeof(int));
  read(p[0], &file_cnt, sizeof(int));
  close(p[0]);
  printf("\n%d directories, %d files\n", dir_cnt, file_cnt); // placeholder
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("usage: mp0 <root_directory> <key>\n");
    exit(1);
  }
  mp0(argv[1], *argv[2]);
  exit(0);
}
