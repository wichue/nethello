// Copyright (c) 2024 The idump project authors. SPDX-License-Identifier: MIT.
// This file is part of idump(https://github.com/wichue/idump).

#ifndef SRC_UTIL_FILE_H_
#define SRC_UTIL_FILE_H_

#include <cstdio>
#include <cstdlib>
#include <string>
#include "util.h"
#include <functional>

#if defined(__linux__)
#include <limits.h>
#endif

#if defined(_WIN32)
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif // !PATH_MAX

struct dirent{
    long d_ino;              /* inode number*/
    off_t d_off;             /* offset to this dirent*/
    unsigned short d_reclen; /* length of this d_name*/
    unsigned char d_type;    /* the type of d_name*/
    char d_name[1];          /* file name (null-terminated)*/
};
typedef struct _dirdesc {
    int     dd_fd;      /** file descriptor associated with directory */
    long    dd_loc;     /** offset in current buffer */
    long    dd_size;    /** amount of data returned by getdirentries */
    char    *dd_buf;    /** data buffer */
    int     dd_len;     /** size of data buffer */
    long    dd_seek;    /** magic cookie returned by getdirentries */
    HANDLE handle;
    struct dirent *index;
} DIR;
# define __dirfd(dp)    ((dp)->dd_fd)

int mkdir(const char *path, int mode);
DIR *opendir(const char *);
int closedir(DIR *);
struct dirent *readdir(DIR *);

#endif // defined(_WIN32)

#if defined(_WIN32) || defined(_WIN64)
#define fseek64 _fseeki64
#define ftell64 _ftelli64
#else
#define fseek64 fseek
#define ftell64 ftell
#endif

namespace chw {

//创建路径
bool create_path(const char *file, unsigned int mod);

/**
 * FILE *fopen(const char *filename, const char *mode);
 * @param mode 定义了文件被打开的方式
 * "r"​：以只读方式打开文件。文件必须存在。
 * "w"​：以写入方式打开文件。如果文件不存在，则创建它；如果文件存在，则其内容被清空（即文件被截断为 0 长度）。
 * ​"a"​：以追加模式打开文件。如果文件不存在，则创建它；如果文件存在，则写入的数据被追加到文件的末尾，而不会覆盖原有内容。
 * "r+"​：以读写方式打开文件。文件必须存在。
 * "w+"​：以读写方式打开文件。如果文件不存在，则创建它；如果文件存在，则其内容被清空。
 * ​"a+"​：以读写模式打开文件用于追加。如果文件不存在，则创建它；如果文件存在，则写入的数据被追加到文件的末尾，并且可以从文件的开头读取数据。
 */

/**
 * size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
 * ptr ：指向存储读取数据的缓冲区的指针。
 * size ：每个数据单元的大小（以字节为单位）。
 * nmemb ：要读取的数据单元的数量。
 * stream ：文件指针，指向要读取的文件。
 * 
 * fread()返回成功读取的数据单元数量。如果返回值小于nmemb，则可能是遇到了文件结尾或发生了读取错误。
 */

/**
 * @brief 新建文件，目录文件夹自动生成
 * 
 * @param file 完整路经
 * @param mode 打开模式，"w"
 * @return FILE* 
 */
FILE *create_file(const char *file, const char *mode);

//判断是否为目录
bool is_dir(const char *path);

//获取路经最后的文件名
std::string path_get_file(std::string path);

//判断是否是特殊目录（. or ..）
bool is_special_dir(const char *path);

//删除目录或文件
int delete_file(const char *path);

//判断文件是否存在
bool fileExist(const char *path);

 /**
 * 加载文件内容至string
 * @param path 加载的文件路径
 * @return 文件内容
 */
std::string loadFile(const char *path);

/**
 * 保存内容至文件
 * @param data 文件内容
 * @param path 保存的文件路径
 * @return 是否保存成功
 */
bool saveFile(const std::string &data, const char *path);

/**
 * 获取父文件夹
 * @param path 路径
 * @return 文件夹
 */
std::string parentDir(const std::string &path);

/**
 * 替换"../"，获取绝对路径
 * @param path 相对路径，里面可能包含 "../"
 * @param current_path 当前目录
 * @param can_access_parent 能否访问父目录之外的目录
 * @return 替换"../"之后的路径
 */
std::string absolutePath(const std::string &path, const std::string &current_path, bool can_access_parent = false);

/**
 * 遍历文件夹下的所有文件
 * @param path 文件夹路径
 * @param cb 回调对象 ，path为绝对路径，isDir为该路径是否为文件夹，返回true代表继续扫描，否则中断
 * @param enter_subdirectory 是否进入子目录扫描
 */
void scanDir(const std::string &path, const std::function<bool(const std::string &path, bool isDir)> &cb, bool enter_subdirectory = false);

/**
 * 获取文件大小
 * @param fp 文件句柄
 * @param remain_size true:获取文件剩余未读数据大小，false:获取文件总大小
 */
uint64_t fileSize(FILE *fp, bool remain_size = false);

/**
 * 获取文件大小
 * @param path 文件路径
 * @return 文件大小
 * @warning 调用者应确保文件存在
 */
uint64_t fileSize(const char *path);

} /* namespace chw */
#endif /* SRC_UTIL_FILE_H_ */
