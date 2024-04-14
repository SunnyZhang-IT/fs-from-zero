// Hello filesystem class definition

#ifndef __HELLOFS_H_
#define __HELLOFS_H_

#include "Fuse.h"

#include "Fuse-impl.h"

class HelloWorldFS : public Fusepp::Fuse<HelloWorldFS>
{
public:
  HelloWorldFS() {}

  ~HelloWorldFS() {}

  static int getattr (const char *, struct stat *, struct fuse_file_info *);

  static int readdir(const char *path, void *buf,
                     fuse_fill_dir_t filler,
                     off_t offset, struct fuse_file_info *fi,
                     enum fuse_readdir_flags);
  
};

#endif
