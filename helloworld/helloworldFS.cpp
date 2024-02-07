/* Description: HelloWorld filesystem class implementation
 * 		This class implement the core API that is related to directory and file.
 * 		In this class, we implement a very simple file system. 
 *
 */

#include "helloworldFS.h"

#include <iostream>
#include <string>

// include in one .cpp file
#include "Fuse-impl.h"

using namespace std;

static const string root_path = "/";
static const string hello_str = "Data Storage Zhang!\n";
static const string hello_path = "/helloworld";
static const string dir_path = "/dir";

int HelloFS::getattr(const char *path, struct stat *stbuf, struct fuse_file_info *)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (path == root_path) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (path == hello_path) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = hello_str.length();
	} else if (path == dir_path) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else
		res = -ENOENT;

	return res;
}

int HelloFS::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			               off_t, struct fuse_file_info *, enum fuse_readdir_flags)
{
	if (path != root_path)
		return -ENOENT;

	filler(buf, ".", NULL, 0, FUSE_FILL_DIR_PLUS);
	filler(buf, "..", NULL, 0, FUSE_FILL_DIR_PLUS);
	filler(buf, hello_path.c_str() + 1, NULL, 0, FUSE_FILL_DIR_PLUS);
	filler(buf, dir_path.c_str() + 1, NULL, 0, FUSE_FILL_DIR_PLUS);

	return 0;
}


int HelloFS::open(const char *path, struct fuse_file_info *fi)
{
	if (path != hello_path)
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}
