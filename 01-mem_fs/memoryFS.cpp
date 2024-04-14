/* Description: Memory filesystem class implementation
 * 		This class implement the core API that is related to directory and file.
 * 		In this class, we implement a very simple file system. 
 *
 */

#include "memoryFS.h"

#include <syslog.h>

#include <iostream>
#include <string>
#include <map>
#include <bitset>
#include <vector>

// include in one .cpp file
#include "Fuse-impl.h"

using namespace std;

static const string root_path = "/";

#define DATA_SPACE_LEN (256)
static map<string, inode*> files;
static vector<char*> data_space;
static bitset<DATA_SPACE_LEN> data_bitmap;

/* 初始化的时候被调用 */
void* MemoryFS::init(struct fuse_conn_info *conn, fuse_config*)
{
	syslog(LOG_NOTICE, "init\n");
	for (int i = 0; i < DATA_SPACE_LEN; i++) {
	    char* block = new char[1024];
	    memset(block, 0, 1024);
	    data_space.push_back(block);
	    data_bitmap.reset(i);
	}

	return 0;
}

int MemoryFS::getattr(const char *path, struct stat *stbuf, struct fuse_file_info *)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (path == root_path) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (files.find(path) != files.end()) {
		auto inode = files.find(path);
		stbuf->st_mode = S_IFREG | 0755;
		stbuf->st_nlink = 1;
		stbuf->st_size = inode->second->size;
		syslog(LOG_NOTICE, "getattr file: %s %d\n", path);
	} else {
		res = -ENOENT;
		syslog(LOG_NOTICE, "getattr error: %s\n", path);
	}

	syslog(LOG_NOTICE, "getattr: %s %d\n", path, res);
	return res;
}

int MemoryFS::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			               off_t, struct fuse_file_info *, enum fuse_readdir_flags)
{
	syslog(LOG_NOTICE, "readdir: %s\n", path);

	filler(buf, ".", NULL, 0, FUSE_FILL_DIR_PLUS);
	filler(buf, "..", NULL, 0, FUSE_FILL_DIR_PLUS);
	for (const auto& [key, value] : files) {
	    filler(buf, key.c_str() + 1, NULL, 0, FUSE_FILL_DIR_PLUS);
	}

	return 0;
}

int MemoryFS::create(const char *name, mode_t mode, struct fuse_file_info *)
{
	syslog(LOG_NOTICE, "Create: %s\n", name);
	inode *i = new inode();
	memset(i, 0, sizeof(inode));
	files[name] = i;
	return 0;
}

int MemoryFS::open(const char *name, struct fuse_file_info *)
{
	int status = 0;
	auto inode = files.find(name);
	if (inode == files.end()) {
	    status = -ENOENT;
	}

	syslog(LOG_NOTICE, "Open: %s %d\n", name, status);
	return 0;
}
int MemoryFS::write(const char *name, const char *buf, size_t buf_size, off_t offset, struct fuse_file_info *)
{
	auto inode = files.find(name);
	char* content = nullptr;
	short index = 0;
	int status = 0;

	if ( inode == files.end() ) {
	    status = -ENOENT;
	    goto OUT;
	}

	if ( inode->second->size ) {
	    index = inode->second->data_index;
	} else {
            for (int i = 0; i < DATA_SPACE_LEN; i++) {
    	        if (!data_bitmap[i]) {
                    index = i;
		    data_bitmap.set(i);
    	            break;
                }
    	   }
	}

	content = data_space[index];
	memcpy(content, buf, buf_size);
	inode->second->data_index = index;
	inode->second->size = buf_size;

OUT:
	syslog(LOG_NOTICE, "Write: %s %s %d\n", name, buf, buf_size);
	return buf_size;
}

int MemoryFS::read(const char *name, char *buf, size_t buf_size, off_t offset, struct fuse_file_info *)
{
	auto inode = files.find(name);
	char* content = nullptr;
	short index = 0;
	short file_size = 0;
	int status = 0;

	if ( inode == files.end() ) {
	    status = -ENOENT;
	    goto OUT;
	}

	index = inode->second->data_index;
	file_size = inode->second->size;
	content = data_space[index];
	memcpy(buf, content, file_size);

OUT:
	syslog(LOG_NOTICE, "Read: %s C %s B %s %d %d %d\n", name, content, buf, index, file_size, buf_size);
	return file_size;
}
