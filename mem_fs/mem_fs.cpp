// See  FUSE:  example/hello.c

#include "memoryFS.h"

int main(int argc, char *argv[])
{

  MemoryFS fs;

  int status = fs.run(argc, argv);

  return status;
}
