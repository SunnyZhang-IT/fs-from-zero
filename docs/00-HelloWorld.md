从今天开始，我打算挖一个比较大的坑。我计划在“数据存储张”创建一个合集，介绍如何从零开始开发一个文件系统。整个开发工作基于Ubuntu 20.04，当然其它Linux版本也没有问题。对于文件系统的开发，我们通常的理解是文件系统是一个内核模块。但是实际情况是文件系统并不一定是内核模块，比如EMC的UFS64就是一个用户态的文件系统。同时，很多开源的分布式文件系统也都是用户态的。

鉴于内核态开发门槛较高，所以我暂时不打算基于Linux内核来开发，而是借助FUSE来开发一个用户态的文件系统。通过本合集的实践，希望大家能够对本人拙著《文件系统技术内幕》中的内容能够有更加深刻的理解。其实无论是内核态的文件系统也好，用户态的文件系统也罢，其原理是相同的。

虽然暂时不打算基于内核开发，但基于用户态的内容实现完成后，如果大家对Linux内核文件系统的实现感兴趣，我也会继续基于内核实现一些功能，让大家对内核文件系统有一个比较全面的理解。

好了，言归正传，我们回到本文的正题，继续我们的文件系统之旅。在具体实现文件系统之前，我觉得有必要介绍一些基本概念，这样大家对理解后续内容会有所帮助。

# 什么是文件系统

首先我们介绍一下什么是文件系统。文件系统是一个将硬盘的线性地址转换为层级结构的软件系统，其核心是给用户呈现层级结构的目录树（如下图所示）。在文件系统中有两个非常重要的概念，一个是文件，另外一个是目录（或者文件夹）。其中目录是一个容器，可以存储文件或者目录（称为子目录）。文件是存储数据的实体，我们的数据都是以文件的形态进行存储的。文件有很多种类，比如视频文件、音频文件、Word文档和文本文件等等。


![什么是文件系统](https://upload-images.jianshu.io/upload_images/11058170-14775ac5d80a1d47.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

# 文件系统的API

在普通用户角度，文件系统提供了一个层级结构的文件组织方式。从程序开发的角度，文件系统提供一套API来访问文件和目录。比如文件的打开、关闭、读取数据和写入数据等，目录的打开、读取（遍历）和关闭等等，具体如下所示是这些API的一个子集。对于上述API，在内核态都有对应的API来实现具体的功能。理解这些API很重要，因为FUSE正是在内核态对这些API的请求进行了截获，然后转发到用户态来处理的。

| 功能描述         | Linux API |
| ---------------- | --------- |
| 打开文件         | open      |
| 向文件写数据     | write     |
| 从文件读数据     | read      |
| 关闭文件         | close     |
| 移动文件指针位置 | lseek     |
| 获取文件属性     | stat      |
| 遍历目录         | readdir   |

# 什么是FUSE

FUSE实现了一个在用户态开发文件系统的框架，有了这个框架，我们可以在用户态开发文件系统的逻辑，而不用关心Linux内核的相关内容。从而大大降低了开发文件系统的门槛。

FUSE本身包含一个用户态的库和一个内核模块。内核态模块与VFS及其他文件系统的关系如图所示，可以理解FUSE为一个内核态的文件系统。内核态的文件系统，并不会将数据持久化，其功能是将文件系统访问请求转发到用户态。

用户态库提供了一套API，同时提供了一套接口规范，这套规范实际上是一组函数集合。基于FUSE开发文件系统就是实现FUSE定义的函数集合的某些或者全部函数。然后调用FUSE用户态库的API将实现的函数注册到内核模块中。在下图的示例中，ceph_fuse就是基于FUSE开发的一个用户态的文件系统，用于实现对CephFS的访问的。如果我们基于FUSE开发自己的文件系统，也是位于这个位置的。


![image.png](https://upload-images.jianshu.io/upload_images/11058170-160bae4f448ce1ff.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

前文已述，内核态的模块基于VFS实现了一个文件系统，可以与Ext4、NFS或者CephFS主机端的文件系统对比理解。但不同的是，当有用户请求达到该文件系统时，该文件系统不是访问磁盘或者是通过网络发送请求，而是调用用户态注册的回调函数。

如果大家对前文NFS的流程，或者CephFS的流程熟悉的话，理解FUSE工作原理就非常简单了。与NFS类比，FUSE将NFS中通过网络转化请求换成了通过函数调用（严格来说并非简单的函数调用，因为涉及内核态到用户态的转换）来转化请求。

# 实现一个不是文件系统的文件系统

今天我们将开发一个最简单的文件系统，严格来说都不是一个文件系统。这里我们只是模拟一个文件系统的层级结构。在这个文件系统中包含一个目录和一个文件，目录的名称为“dir”，文件的名称为“helloworld”。同时，按照Linux的惯例，这里还包含当前目录“.”和上一级目录“..”。该文件系统的目录结构如下图所示。

![我们实现的文件系统](https://upload-images.jianshu.io/upload_images/11058170-079df814c7010365.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

虽然这里是一个层级结构，但是这个文件系统并不能像其他文件系统那样在目录中创建文件，也不能删除文件，更不能读取文件的内容。为什么？原因很简单，因为我们并没有实现上述功能。

接下来我们来看一下这个文件系统的具体实现。所有文件都在一个名称为helloworld的目录当中，该目录中的文件与作用如下图所示。其中3个文件（Fuse.cpp、Fuse.h和Fuse-impl.h）是FUSE的C++封装，这里我们借用了James的实现，感谢他这方面的工作。另外3个C++源代码文件是本文件系统的实现，其中helloworld.cpp是主函数的源文件，其实没有太多内容，helloworldFS.cpp/h是文件系统的实现。CMakeLists.txt是cmake的源文件，用于管理整个工程。


![本工程代码结构](https://upload-images.jianshu.io/upload_images/11058170-1f47405870ea8860.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

如下是文件系统的具体实现，可以看到这里只实现了2个函数，分别是getattr和readdir。其中readdir用于读取目录项，getattr用于获取文件或者目录的详细属性。这两个函数正式命令行工具ls会调用到的命令。

```
/* Description: HelloWorld filesystem class implementation
 * 		This class implements the core API that is related to the directory and file.
 * 		In this class, we implement a very simple file system. 
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

int HelloWorldFS::getattr(const char *path, struct stat *stbuf, struct fuse_file_info *)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (path == root_path) {  //根目录的属性
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (path == hello_path) { //helloworld文件的属性
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = hello_str.length();
	} else if (path == dir_path) { //dir目录的属性
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else
		res = -ENOENT;

	return res;
}

int HelloWorldFS::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
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
```

头文件的实现更加简单，如下是头文件的源代码。在头文件中只是定义了一个文件系统类HelloWorldFS，并定义了上述两个函数。

```
// Hello filesystem class definition
#ifndef __HELLOFS_H_
#define __HELLOFS_H_

#include "Fuse.h"
#include "Fuse-impl.h"

class HelloWorldFS : public Fusepp::Fuse<HelloWorldFS>
{
public:
  HelloFS() {}
  ~HelloFS() {}

  static int getattr (const char *, struct stat *, struct fuse_file_info *);
  static int readdir(const char *path, void *buf,
                     fuse_fill_dir_t filler,
                     off_t offset, struct fuse_file_info *fi,
                     enum fuse_readdir_flags);
};

#endif
```

如下是主函数所在源文件的内容，实现也是比较简单的。这里实例化了HelloWorldFS类，并调用run函数。这个函数完成了文件系统挂载，函数注册等任务。具体涉及FUSE实现的内容，我们后面再详细介绍这方面的内容。

```
// See  FUSE:  example/hello.c

#include "helloworldFS.h"

int main(int argc, char *argv[])
{

  HelloWorldFS fs;

  int status = fs.run(argc, argv);

  return status;
}
```

如下是cmake的工程文件，用于生成Makefile文件。关于cmake的概念及使用方法可以阅读本号之前的文章，本文不在赘述相关内容。

```
cmake_minimum_required(VERSION 3.16)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++14 -D_FILE_OFFSET_BITS=64 -D__coverage -I/usr/include/fuse3 -lfuse3 -lpthread")
project(CMakeSunny
    VERSION 1.0
    DESCRIPTION "A CMake Tutorial"
    LANGUAGES CXX)

add_executable(helloworld
    helloworld.cpp
    helloworldFS.cpp
    Fuse.cpp)

target_link_libraries(helloworld -lfuse3)
```

具备上述源文件后就可以编译源文件了。首先需要在根目录创建一个名称为build子目录。然后切换到该目录执行“cmake ..”命令。成功后会生成一个Makefile。最后，我们执行一下make命令就会编译出一个可执行程序helloworld。执行如下命令就可以将我们实现的“文件系统”挂载到/mnt/test（需要提前创建）目录了。

```
./helloworld /mnt/test
```

然后我们可以通过cd命令切换到该目录，并通过ls命令查看其中的内容。这时就可以看到我们在文章一开始看到的内容。
![我们实现的文件系统](https://upload-images.jianshu.io/upload_images/11058170-079df814c7010365.png?imageMogr2/auto-orient/strip%7CimageView2/2/w/1240)

如前文所述，本文我们实现了一个假的文件系统，这个文件系统既不能创建新文件，也不能读取文件的数据，更不能向已有文件写入数据。但是通过本文，相信大家对文件系统的概念和FUSE的用法有了一个基本的了解。后面我们将以当前实现的文件系统为基础，实现一个基于内存的，可读写的文件系统。

> 注： 本文配套的源代码可以在github的SunnyZhang-IT/fs-from-zero库中找到。
>
