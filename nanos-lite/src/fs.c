#include "fs.h"
extern void ramdisk_read(void *buf, off_t offset, size_t len);
extern void ramdisk_write(const void *buf, off_t offset, size_t len);
extern size_t events_read(void *buf, size_t len);
void dispinfo_read(void* buf,off_t offset,size_t len);
void fb_write(const void *buf, off_t offset, size_t len);
typedef struct {
  char *name;
  size_t size;
  off_t disk_offset;
  off_t open_offset;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_EVENTS, FD_DISPINFO, FD_NORMAL};

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  {"stdin (note that this is not the actual stdin)", 0, 0},
  {"stdout (note that this is not the actual stdout)", 0, 0},
  {"stderr (note that this is not the actual stderr)", 0, 0},
  [FD_FB] = {"/dev/fb", 0, 0},
  [FD_EVENTS] = {"/dev/events", 0, 0},
  [FD_DISPINFO] = {"/proc/dispinfo", 128, 0},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))
size_t fs_filesz(int fd) {
  assert(fd >=0 && fd<NR_FILES);
  return file_table[fd].size;
}
off_t disk_offset(int fd) {
  assert(fd>=0 && fd<NR_FILES);
  return file_table[fd].disk_offset;
}
off_t get_open_offset(int fd) {
  assert(fd>=0 && fd<NR_FILES);
  return file_table[fd].open_offset;
}
void set_open_offset(int fd,off_t n) {
  assert(fd>=0 && fd<NR_FILES);
  assert(n>=0);
  if(n>file_table[fd].size)
    n = file_table[fd].size;
  file_table[fd].open_offset = n;
}
int fs_open(const char *pathname, int flags, int mode) {
	int i;
	//Log("the total files : %d\n", NR_FILES);
	//Log("pathname %s\n", pathname);
	for (i = 0; i < NR_FILES; i++) {
		//printf("file name: %s\n", file_table[i].name);
		if (strcmp(file_table[i].name, pathname) == 0) {
			set_open_offset(i,0);			
			return i;
		}
	}
  Log("pathname:%s\n",pathname);
	panic("this file not exist!");
	return -1;
}
int fs_close(int fd) {
  assert(fd>=0 &&fd<NR_FILES);
  return 0;
}
ssize_t fs_read(int fd, void *buf, size_t len) {
////event  
  assert(fd>=0 && fd<NR_FILES);
  if(fd < 3) {
    Log("arg invalid: fd < 3");
    return 0;
  }
  if(fd == FD_EVENTS) {
    return events_read(buf,len);
  }
  int n = fs_filesz(fd) - get_open_offset(fd);
  if(n > len)
    n = len;
  if(fd == FD_DISPINFO)
    dispinfo_read(buf,get_open_offset(fd),n);
  else
    ramdisk_read(buf,disk_offset(fd)+get_open_offset(fd),n);
  set_open_offset(fd,get_open_offset(fd)+n);
  return n;
}

ssize_t fs_write(int fd,void* buf,size_t len) {
  assert(fd>=0 && fd<NR_FILES);
	if(fd < 3 || fd == FD_DISPINFO) {
    Log("arg invalid: fd < 3 || fd == FD_DISPINFO");
    return 0;
  }
  int n = fs_filesz(fd) - get_open_offset(fd);
  if(n > len)
    n = len;
  if(fd == FD_FB)
    fb_write(buf,get_open_offset(fd),n);
  else
    ramdisk_write(buf,disk_offset(fd)+get_open_offset(fd),n);
  set_open_offset(fd,get_open_offset(fd)+n);
  return n;
}
off_t fs_lseek(int fd,off_t offset,int whence) {
  switch(whence) {
    case SEEK_SET:
      set_open_offset(fd,offset);
      return get_open_offset(fd);
    case SEEK_CUR:
      set_open_offset(fd,get_open_offset(fd)+offset);
      return get_open_offset(fd);
    case SEEK_END:
      set_open_offset(fd,fs_filesz(fd)+offset);
      return get_open_offset(fd);
    default:
      panic("Unhandled whence ID = %d",whence);
      return -1;
  }
}
void init_fs() {
  // TODO: initialize the size of /dev/fb
	extern void get_screen(int* s_width,int * s_height);
  	int width = 0,height = 0;
  	get_screen(&width,&height);
  	file_table[FD_FB].size = width*height*sizeof(uint32_t);
  	Log("set FD_DB size = %d",file_table[FD_FB].size);
}
