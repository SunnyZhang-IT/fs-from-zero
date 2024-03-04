// Hello filesystem class definition

#ifndef __HELLOFS_H_
#define __HELLOFS_H_

#include "Fuse.h"

#include "Fuse-impl.h"

struct inode
{
    unsigned int inode_id;
    unsigned int mode;
    unsigned short data_index;
    unsigned short size;
};

class MemoryFS : public Fusepp::Fuse<MemoryFS>
{
public:
  MemoryFS() {}

  ~MemoryFS() {}

  static void * init(struct fuse_conn_info *conn, fuse_config*);
  static int getattr (const char *, struct stat *, struct fuse_file_info *);

  static int readdir(const char *path, void *buf,
                     fuse_fill_dir_t filler,
                     off_t offset, struct fuse_file_info *fi,
                     enum fuse_readdir_flags);
  
  static int create (const char *, mode_t, struct fuse_file_info *);
  static int open(const char *name, struct fuse_file_info *);
  static int write(const char *name, const char *buf, size_t buf_size, off_t offset, struct fuse_file_info *);
  static int read(const char *name, char *buf, size_t buf_size, off_t offset, struct fuse_file_info *);
};

#endif
