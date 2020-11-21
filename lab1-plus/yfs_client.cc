// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <x86intrin.h>


#define func_num_yfs 25
// #define CYCLE
int call_count_yfs[func_num_yfs] = {0};
unsigned long long max_cycle_yfs[func_num_yfs] = {0};
unsigned long long min_cycle_yfs[func_num_yfs] = {0};
FILE * fp_yfs;

unsigned long long get_current_time_yfs()
{
  return __rdtsc();
}

void calculate_cycle_yfs(int func_no, unsigned long long start)
{
  unsigned long long end = _rdtsc();
  unsigned long long cycles = end - start;
  call_count_yfs[func_no]++;
  if (cycles > max_cycle_yfs[func_no])
  {
    max_cycle_yfs[func_no] = cycles;
  }
  if (min_cycle_yfs[func_no] == 0 || cycles < min_cycle_yfs[func_no])
  {
    min_cycle_yfs[func_no] = cycles;
  }
  fp_yfs = fopen("yfs_client.txt", "a");
  fprintf(fp_yfs, "%s%d %s %d %s %s %lld  %s %lld\n", "Function", func_no, "called", call_count_yfs[func_no], "times", "Max cycle:", max_cycle_yfs[func_no], "Min cycle:", min_cycle_yfs[func_no]);
  fclose(fp_yfs);
}

//--------------------------------------------------------------
// #define CACHE
//--------------------------------------------------------------
// #define PRINTF
//--------------------------------------------------------------
#define TYPE_CACHE
//--------------------------------------------------------------
yfs_client::yfs_client()
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    ec = new extent_client();
  #ifdef CYCLE
    calculate_cycle_yfs(0, start);
  #endif
}

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    ec = new extent_client();
    if (ec->put(1, "") != extent_protocol::OK)
    {
        #ifdef PRINTF
            printf("error init root dir\n");
        #endif
    }
  #ifdef CYCLE
    calculate_cycle_yfs(1, start);
  #endif
}

yfs_client::inum
yfs_client::n2i(std::string n)
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
  #ifdef CYCLE
    calculate_cycle_yfs(2, start);
  #endif
    return finum;
}

bool yfs_client::add_entry_and_save(inum parent, const char *name, inum inum) 
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    std::list<dirent> entries;

    if (readdir(parent, entries) != OK) 
    {
        #ifdef PRINTF
            printf("add_entry_and_save: fail to read directory entries\n");
        #endif
        #ifdef CYCLE
            calculate_cycle_yfs(3, start);
        #endif
        return false;
    }

    dirent entry;
    entry.name = name;
    entry.inum = inum;
    entries.push_back(entry);

    if (writedir(parent, entries) != OK) 
    {
        #ifdef PRINTF
            printf("add_entry_and_save: fail to write directory entries\n");
        #endif
        #ifdef CYCLE
            calculate_cycle_yfs(3, start);
        #endif
        return false;
    }
  #ifdef CYCLE
    calculate_cycle_yfs(3, start);
  #endif
    return true;
}

bool yfs_client::has_duplicate(inum parent, const char *name) {
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    
    bool exist;
    inum old_inum;

    if (lookup(parent, name, exist, old_inum) != extent_protocol::OK) {
        #ifdef PRINTF
            printf("has_duplicate: fail to perform lookup\n");
        #endif
        #ifdef CYCLE
            calculate_cycle_yfs(4, start);
        #endif
        return true; // use true to forbid further action
    }
  #ifdef CYCLE
    calculate_cycle_yfs(4, start);
  #endif
    return exist;
}

std::string
yfs_client::filename(inum inum)
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    std::ostringstream ost;
    ost << inum;
  #ifdef CYCLE
    calculate_cycle_yfs(5, start);
  #endif
    return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    extent_protocol::attr a;

    #ifdef CACHE
    if (inode_attr.count(inum) > 0)
    {
        a = inode_attr[inum];
        if (a.type == extent_protocol::T_FILE)
        {
            return true;
        }
        else
        {
            return false;
        }        
    }
    #endif

    #ifdef TYPE_CACHE
        if (inode_type.count(inum) > 0)
        {
            if (inode_type[inum] == extent_protocol::T_FILE)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    #endif


    if (ec->getattr(inum, a) != extent_protocol::OK) {
        #ifdef CACHE
            inode_attr.insert({inum, a});
        #endif
        #ifdef TYPE_CACHE
            inode_type[inum] = (extent_protocol::types)a.type;
        #endif
        #ifdef PRINTF
        printf("error getting attr\n");
        #endif
        #ifdef CYCLE
        calculate_cycle_yfs(6, start);
        #endif
        return false;
    }

    if (a.type == extent_protocol::T_FILE) {
        #ifdef PRINTF
        printf("isfile: %lld is a file\n", inum);
        #endif
        #ifdef CYCLE
        calculate_cycle_yfs(6, start);
        #endif
        return true;
    }
    #ifdef PRINTF 
        printf("isfile: %lld is a dir\n", inum);
    #endif
  #ifdef CYCLE
    calculate_cycle_yfs(6, start);
  #endif
    return false;
}
/** Your code here for Lab...
 * You may need to add routines such as
 * readlink, issymlink here to implement symbolic link.
 * 
 * */

bool 
yfs_client::isdir(inum inum) 
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    extent_protocol::attr a;

    #ifdef CACHE
    if (inode_attr.count(inum) > 0)
    {
        a = inode_attr[inum];
        if (a.type == extent_protocol::T_DIR)
        {
            return true;
        }
        else
        {
            return false;
        }        
    }
    #endif

    #ifdef TYPE_CACHE
        if (inode_type.count(inum) > 0)
        {
            if (inode_type[inum] == extent_protocol::T_DIR)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    #endif


    if (ec->getattr(inum, a) != extent_protocol::OK) {
        #ifdef CACHE
            inode_attr.insert({inum, a});
        #endif
        #ifdef TYPE_CACHE
            inode_type[inum] = (extent_protocol::types)a.type;
        #endif
        #ifdef PRINTF
            printf("isdir: error getting attr\n");
        #endif
        #ifdef CYCLE
            calculate_cycle_yfs(7, start);
        #endif
        return false;
    }

    if (a.type == extent_protocol::T_DIR) {
        #ifdef PRINTF
            printf("isdir: %lld is a dir\n", inum);
        #endif
        #ifdef CYCLE
            calculate_cycle_yfs(7, start);
        #endif
        return true;
    }
  #ifdef CYCLE
    calculate_cycle_yfs(7, start);
  #endif
    return false;
}


int
yfs_client::getfile(inum inum, fileinfo &fin)
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    int r = OK;
    #ifdef PRINTF
        printf("getfile %016llx\n", inum);
    #endif
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    #ifdef PRINTF
        printf("getfile %016llx -> sz %llu\n", inum, fin.size);
    #endif

release:
  #ifdef CYCLE
    calculate_cycle_yfs(8, start);
  #endif
    return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    int r = OK;
    #ifdef PRINTF
        printf("getdir %016llx\n", inum);
    #endif
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

release:
  #ifdef CYCLE
    calculate_cycle_yfs(9, start);
  #endif
    return r;
}

int yfs_client::getlink(inum inum, fileinfo& sin) 
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    int r = OK;
    #ifdef PRINTF
        printf("getlink %016llx\n", inum);
    #endif
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) 
    {
        r = IOERR;
        goto release;
    }

    sin.atime = a.atime;
    sin.mtime = a.mtime;
    sin.ctime = a.ctime;
    sin.size  = a.size;
    #ifdef PRINTF
        printf("getlink %016llx -> sz %llu\n", inum, sin.size);
    #endif

release:
  #ifdef CYCLE
    calculate_cycle_yfs(10, start);
  #endif
    return r;
}

#define EXT_RPC(xx) do { \
    if ((xx) != extent_protocol::OK) { \
        printf("EXT_RPC Error: %s:%d \n", __FILE__, __LINE__); \
        r = IOERR; \
        goto release; \
    } \
} while (0)

// Only support set size of attr
int
yfs_client::setattr(inum ino, size_t size)
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    int r = OK;

    /*
     * your code goes here.
     * note: get the content of inode ino, and modify its content
     * according to the size (<, =, or >) content length.
     */
    std::string content;
    if(ec->get(ino, content) != extent_protocol::OK)
    {
        #ifdef PRINTF
            printf("setattr: fail to read content\n");
        #endif
        #ifdef CYCLE
            calculate_cycle_yfs(11, start);
        #endif
        return IOERR;
    }
    if(size == content.size())
    {
        #ifdef CYCLE
            calculate_cycle_yfs(11, start);
        #endif
        return OK;
    }
    content.resize(size);
    if(ec->put(ino,content) != extent_protocol::OK)
    {
        #ifdef PRINTF
            printf("setattr: fail to write content\n");
        #endif
        #ifdef CYCLE
            calculate_cycle_yfs(11, start);
        #endif
        return IOERR;
    }
  #ifdef CYCLE
    calculate_cycle_yfs(11, start);
  #endif
    return r;
}

int
yfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out)
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    int r = OK;

    /*
     * your code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    bool found = false;
    inum ino;
    std::string buf;
    lookup(parent, name, found, ino);
    if(found)
    {
        #ifdef CYCLE
            calculate_cycle_yfs(12, start);
        #endif
        return EXIST;
    }
    if(ec->create(extent_protocol::T_FILE, ino_out) != extent_protocol::OK)
    {
        #ifdef CYCLE
            calculate_cycle_yfs(12, start);
        #endif
        return IOERR;
    }
    if (add_entry_and_save(parent, name, ino_out) == false) 
    {
        #ifdef CYCLE
            calculate_cycle_yfs(12, start);
        #endif
        return IOERR;
    }
  #ifdef CYCLE
    calculate_cycle_yfs(12, start);
  #endif
  #ifdef TYPE_CACHE
    inode_type[ino_out] = extent_protocol::T_FILE;
  #endif
    return r;
}

int
yfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    int r = OK;

    /*
     * your code goes here.
     * note: lookup is what you need to check if directory exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    bool found = false;
    inum ino;
    std::string buf;
    lookup(parent, name, found, ino);
    if(found)
    {
        #ifdef CYCLE
            calculate_cycle_yfs(13, start);
        #endif
        return EXIST;
    }
    if(ec->create(extent_protocol::T_DIR, ino_out) != extent_protocol::OK)
    {
        #ifdef PRINTF
            printf("mkdir: fail to create directory\n");
        #endif
        #ifdef CYCLE
            calculate_cycle_yfs(13, start);
        #endif
        return IOERR;
    }
    if(add_entry_and_save(parent, name, ino_out) == false)
    {
        #ifdef CYCLE
            calculate_cycle_yfs(13, start);
        #endif
        return IOERR;
    }
  #ifdef CYCLE
    calculate_cycle_yfs(13, start);
  #endif
    #ifdef TYPE_CACHE
        inode_type[ino_out] = extent_protocol::T_DIR;
    #endif

    return r;
}

int
yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    int r = OK;

    /*
     * your code goes here.
     * note: lookup file from parent dir according to name;
     * you should design the format of directory content.
     */
    std::list<dirent> file_list;
    if(readdir(parent,file_list) != OK)
    {
        found = false;
        #ifdef CYCLE
            calculate_cycle_yfs(14, start);
        #endif
        return IOERR;
    }
    for(std::list<dirent>::iterator it = file_list.begin(); it != file_list.end(); ++it)
    {
        if(it->name == std::string(name))
        {
            found = true;
            ino_out = it->inum;
            #ifdef CYCLE
                calculate_cycle_yfs(14, start);
            #endif
            return r;
        }
    }
    found = false;
  #ifdef CYCLE
    calculate_cycle_yfs(14, start);
  #endif
    return r;
}

int
yfs_client::readdir(inum dir, std::list<dirent> &list)
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    int r = OK;

    /*
     * your code goes here.
     * note: you should parse the dirctory content using your defined format,
     * and push the dirents to the list.
     */
    std::string buf;
    if (ec->get(dir, buf) != extent_protocol::OK) 
    {
        #ifdef CYCLE
            calculate_cycle_yfs(15, start);
        #endif
        return IOERR;
    }
    // traverse directory content
   list.clear();
   std::istringstream ist(buf);
   dirent entry;
   while(std::getline(ist, entry.name, '\0'))
   {
       ist >> entry.inum;
       list.push_back(entry);
   }
  #ifdef CYCLE
    calculate_cycle_yfs(15, start);
  #endif
    return r;
}

int yfs_client::writedir(inum dir, std::list<dirent>& entries) 
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    // prepare content
    std::ostringstream ost;

    for (std::list<dirent>::iterator it = entries.begin(); it != entries.end(); ++it) 
    {
        ost << it->name;
        ost.put('\0');
        ost << it->inum;
    }

    // write to file
    if (ec->put(dir, ost.str()) != extent_protocol::OK) 
    {
        #ifdef CYCLE
            calculate_cycle_yfs(16, start);
        #endif
        return IOERR;
    }
  #ifdef CYCLE
    calculate_cycle_yfs(16, start);
  #endif
    return OK;
}

int
yfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    int r = OK;

    /*
     * your code goes here.
     * note: read using ec->get().
     */
    std::string content;
    if(ec->get(ino, content) != extent_protocol::OK)
    {
        #ifdef PRINTF
            printf("read: fail to read file\n");
        #endif
        #ifdef CYCLE
            calculate_cycle_yfs(17, start);
        #endif
        return IOERR;
    }
    data = content.substr(off, size);
  #ifdef CYCLE
    calculate_cycle_yfs(17, start);
  #endif
    return r;
}

int
yfs_client::write(inum ino, size_t size, off_t off, const char *data,
        size_t &bytes_written)
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    int r = OK;

    /*
     * your code goes here.
     * note: write using ec->put().
     * when off > length of original file, fill the holes with '\0'.
     */
    std::string content;
    if(ec->get(ino,content) != extent_protocol::OK)
    {
        #ifdef PRINTF
            printf("write: fail to read file\n");
        #endif
        return IOERR;
    }
    if((unsigned)off >= content.size())
    {
        content.resize(off, '\0');
        content.append(data, size);
    }
    else
    {
        content.replace(off, off + size <= content.size() ? size : content.size() - off, data, size);
    }
    if(ec->put(ino, content) != extent_protocol::OK)
    {
        #ifdef PRINTF
            printf("write: fail to write file\n");
        #endif
        return IOERR;
    }
  #ifdef CYCLE
    calculate_cycle_yfs(18, start);
  #endif
    return r;
}

int yfs_client::readlink(inum ino, std::string& path) 
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    // keep off invalid input
    if (ino <= 0) 
    {
        #ifdef PRINTF
            printf("readlink: invalid inode number %llu\n", ino);
        #endif
        #ifdef CYCLE
            calculate_cycle_yfs(19, start);
        #endif
        return IOERR;
    }

    // read path
    if (ec->get(ino, path) != extent_protocol::OK) 
    {
        #ifdef PRINTF
            printf("readlink: fail to read path\n");
        #endif
        #ifdef CYCLE
            calculate_cycle_yfs(19, start);
        #endif
        return IOERR;
    }
  #ifdef CYCLE
    calculate_cycle_yfs(19, start);
  #endif
    return OK;
}

int yfs_client::symlink(inum parent, const char *link, const char *name, inum& ino_out) 
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    // keep off invalid input
    if (parent <= 0) {
        #ifdef PRINTF
            printf("symlink: invalid inode number %llu\n", parent);
        #endif
        #ifdef CYCLE
            calculate_cycle_yfs(20, start);
        #endif
        return IOERR;
    }

    // create file first
    if (ec->create(extent_protocol::T_SLINK, ino_out) != extent_protocol::OK) 
    {
        #ifdef PRINTF
            printf("symlink: fail to create directory\n");
        #endif
        #ifdef CYCLE
            calculate_cycle_yfs(20, start);
        #endif
        return IOERR;
    }

    // write path to file
    if (ec->put(ino_out, link) != extent_protocol::OK) 
    {
        #ifdef PRINTF
            printf("symlink: fail to write link\n");
        #endif
        #ifdef CYCLE
            calculate_cycle_yfs(20, start);
        #endif
        return IOERR;
    }

    // add entry to directory
    if (add_entry_and_save(parent, name, ino_out) == false) 
    {
        #ifdef PRINTF
            printf("symlink: fail to add entry\n");
        #endif
        #ifdef CYCLE
            calculate_cycle_yfs(20, start);
        #endif
        return IOERR;
    }
  #ifdef CYCLE
    calculate_cycle_yfs(20, start);
  #endif
    return OK;
}


int yfs_client::unlink(inum parent,const char *name)
{
  #ifdef CYCLE
    unsigned long long start = get_current_time_yfs();
  #endif
    int r = OK;

    /*
     * your code goes here.
     * note: you should remove the file using ec->remove,
     * and update the parent directory content.
     */
    std::list<dirent> file_list;
    std::list<dirent>::iterator it;
    if(readdir(parent,file_list) != OK)
    {
        #ifdef PRINTF
            printf("unlink: fail to read directory\n");
        #endif
        #ifdef CYCLE
            calculate_cycle_yfs(21, start);
        #endif
        return IOERR;
    }
    for(it = file_list.begin(); it != file_list.end(); ++it)
    {
        if(it->name == name)
        {
            break;
        }
    }
    if(it == file_list.end())
    {
        #ifdef PRINTF
            printf("unlink: no such file %s\n", name);
        #endif
        #ifdef CYCLE
            calculate_cycle_yfs(21, start);
        #endif
        return IOERR;
    }
    if(!isfile(it->inum))
    {
        #ifdef PRINTF
            printf("unlink: %s is not a file\n", name);
        #endif
        #ifdef CYCLE
            calculate_cycle_yfs(21, start);
        #endif
        return IOERR;
    }
    if(ec->remove(it->inum) != extent_protocol::OK)
    {
        #ifdef CYCLE
            calculate_cycle_yfs(21, start);
        #endif
        #ifdef PRINTF
            printf("unlink: fail to remove file %s\n",name);
        #endif
    }
    #ifdef TYPE_CACHE
        inode_type.erase(it->inum);
    #endif
    file_list.erase(it);
    if(writedir(parent, file_list) != OK)
    {
        #ifdef PRINTF
            printf("unlink: fail to write directory entries\n");
        #endif
        #ifdef CYCLE
            calculate_cycle_yfs(21, start);
        #endif
        return IOERR;
    }
  #ifdef CYCLE
    calculate_cycle_yfs(21, start);
  #endif
    return r;
}

