// See  FUSE:  example/hello.c

#include "helloworldFS.h"

int main(int argc, char *argv[])
{

  HelloWorldFS fs;

  int status = fs.run(argc, argv);

  return status;
}
