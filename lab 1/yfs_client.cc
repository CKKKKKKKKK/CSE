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

yfs_client::yfs_client()
{
    ec = new extent_client();

}

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
    ec = new extent_client();
    if (ec->put(1, "") != extent_protocol::OK)
        printf("error init root dir\n"); // XYB: init root dir
}

yfs_client::inum
yfs_client::n2i(std::string n)
{
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

bool yfs_client::add_entry_and_save(inum parent, const char *name, inum inum) 
{
    std::list<dirent> entries;

    if (readdir(parent, entries) != OK) 
    {
        printf("add_entry_and_save: fail to read directory entries\n");
        return false;
    }

    dirent entry;
    entry.name = name;
    entry.inum = inum;
    entries.push_back(entry);

    if (writedir(parent, entries) != OK) 
    {
        printf("add_entry_and_save: fail to write directory entries\n");
        return false;
    }

    return true;
}

bool yfs_client::has_duplicate(inum parent, const char *name) {
    bool exist;
    inum old_inum;

    if (lookup(parent, name, exist, old_inum) != extent_protocol::OK) {
        printf("has_duplicate: fail to perform lookup\n");

        return true; // use true to forbid further action
    }
    return exist;
}

std::string
yfs_client::filename(inum inum)
{
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_FILE) {
        printf("isfile: %lld is a file\n", inum);
        return true;
    } 
    printf("isfile: %lld is a dir\n", inum);
    return false;
}
/** Your code here for Lab...
 * You may need to add routines such as
 * readlink, issymlink here to implement symbolic link.
 * 
 * */

// bool
// yfs_client::isdir(inum inum)
// {
//     // Oops! is this still correct when you implement symlink?
//     return ! isfile(inum);
// }
bool 
yfs_client::isdir(inum inum) 
{
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("isdir: error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_DIR) {
        printf("isdir: %lld is a dir\n", inum);
        return true;
    }

    return false;
}


int
yfs_client::getfile(inum inum, fileinfo &fin)
{
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:
    return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
    int r = OK;

    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

release:
    return r;
}

int yfs_client::getlink(inum inum, fileinfo& sin) 
{
    int r = OK;

    printf("getlink %016llx\n", inum);
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
    printf("getlink %016llx -> sz %llu\n", inum, sin.size);

release:
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
    int r = OK;

    /*
     * your code goes here.
     * note: get the content of inode ino, and modify its content
     * according to the size (<, =, or >) content length.
     */
    std::string content;
    if(ec->get(ino, content) != extent_protocol::OK)
    {
        printf("setattr: fail to read content\n");
        return IOERR;
    }
    if(size == content.size())
    {
        return OK;
    }
    content.resize(size);
    if(ec->put(ino,content) != extent_protocol::OK)
    {
        printf("setattr: fail to write content\n");
        return IOERR;
    }
    return r;
}

int
yfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out)
{
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
        return EXIST;
    }
    if(ec->create(extent_protocol::T_FILE, ino_out) != extent_protocol::OK)
    {
        return IOERR;
    }
    if (add_entry_and_save(parent, name, ino_out) == false) 
    {
        return IOERR;
    }

    return r;
}

int
yfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
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
        return EXIST;
    }
    if(ec->create(extent_protocol::T_DIR, ino_out) != extent_protocol::OK)
    {
        printf("mkdir: fail to create directory\n");
        return IOERR;
    }
    if(add_entry_and_save(parent, name, ino_out) == false)
    {
        return IOERR;
    }
    return r;
}

int
yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
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
        return IOERR;
    }
    for(std::list<dirent>::iterator it = file_list.begin(); it != file_list.end(); ++it)
    {
        if(it->name == std::string(name))
        {
            found = true;
            ino_out = it->inum;
            return r;
        }
    }
    found = false;
    return r;
}

int
yfs_client::readdir(inum dir, std::list<dirent> &list)
{
    int r = OK;

    /*
     * your code goes here.
     * note: you should parse the dirctory content using your defined format,
     * and push the dirents to the list.
     */
    std::string buf;
    if (ec->get(dir, buf) != extent_protocol::OK) 
    {
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
    return r;
}

int yfs_client::writedir(inum dir, std::list<dirent>& entries) 
{
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
        return IOERR;
    }

    return OK;
}

int
yfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
    int r = OK;

    /*
     * your code goes here.
     * note: read using ec->get().
     */
    std::string content;
    if(ec->get(ino, content) != extent_protocol::OK)
    {
        printf("read: fail to read file\n");
        return IOERR;
    }
    data = content.substr(off, size);
    return r;
}

int
yfs_client::write(inum ino, size_t size, off_t off, const char *data,
        size_t &bytes_written)
{
    int r = OK;

    /*
     * your code goes here.
     * note: write using ec->put().
     * when off > length of original file, fill the holes with '\0'.
     */
    std::string content;
    if(ec->get(ino,content) != extent_protocol::OK)
    {
        printf("write: fail to read file\n");
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
        printf("write: fail to write file\n");
        return IOERR;
    }
    return r;
}

int yfs_client::readlink(inum ino, std::string& path) 
{
    // keep off invalid input
    if (ino <= 0) 
    {
        printf("readlink: invalid inode number %llu\n", ino);
        return IOERR;
    }

    // read path
    if (ec->get(ino, path) != extent_protocol::OK) 
    {
        printf("readlink: fail to read path\n");
        return IOERR;
    }

    return OK;
}

int yfs_client::symlink(inum parent, const char *link, const char *name, inum& ino_out) 
{
    // keep off invalid input
    if (parent <= 0) {
        printf("symlink: invalid inode number %llu\n", parent);
        return IOERR;
    }

    // create file first
    if (ec->create(extent_protocol::T_SLINK, ino_out) != extent_protocol::OK) 
    {
        printf("symlink: fail to create directory\n");
        return IOERR;
    }

    // write path to file
    if (ec->put(ino_out, link) != extent_protocol::OK) 
    {
        printf("symlink: fail to write link\n");
        return IOERR;
    }

    // add entry to directory
    if (add_entry_and_save(parent, name, ino_out) == false) 
    {
        printf("symlink: fail to add entry\n");
        return IOERR;
    }

    return OK;
}


int yfs_client::unlink(inum parent,const char *name)
{
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
        printf("unlink: fail to read directory\n");
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
        printf("unlink: no such file %s\n", name);
        return IOERR;
    }
    if(!isfile(it->inum))
    {
        printf("unlink: %s is not a file\n", name);
        return IOERR;
    }
    if(ec->remove(it->inum) != extent_protocol::OK)
    {
        printf("unlink: fail to remove file %s\n",name);
    }
    file_list.erase(it);
    if(writedir(parent, file_list) != OK)
    {
        printf("unlink: fail to write directory entries\n");
        return IOERR;
    }
    return r;
}

