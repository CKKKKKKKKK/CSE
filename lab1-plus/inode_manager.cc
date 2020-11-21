#include "inode_manager.h"
#include <x86intrin.h>
#include <stdio.h>
#include <stdlib.h>

#define func_num 18
// #define CYCLE
int call_count[func_num] = {0};
unsigned long long max_cycle[func_num] = {0};
unsigned long long min_cycle[func_num] = {0};
FILE * fp;

unsigned long long get_current_time()
{
  return __rdtsc();
}

void calculate_cycle(int func_no, unsigned long long start)
{
  unsigned long long end = _rdtsc();
  unsigned long long cycles = end - start;
  call_count[func_no]++;
  if (cycles > max_cycle[func_no])
  {
    max_cycle[func_no] = cycles;
  }
  if (min_cycle[func_no] == 0 || cycles < min_cycle[func_no])
  {
    min_cycle[func_no] = cycles;
  }
  fp = fopen("inode_manager.txt", "a");
  fprintf(fp, "%s%d %s %d %s %s %lld  %s %lld\n", "Function", func_no, "called", call_count[func_no], "times", "Max cycle:", max_cycle[func_no], "Min cycle:", min_cycle[func_no]);
  fclose(fp);
}

//----------------------------------------------------
#define DIRECT
// #define LOOP_UNROLL

// disk layer -----------------------------------------

disk::disk() 
{
  #ifdef CYCLE
    unsigned long long start = get_current_time();
  #endif
  bzero(blocks, sizeof(blocks));
  #ifdef CYCLE
    calculate_cycle(0, start);
  #endif
}

void
disk::read_block(blockid_t id, char *buf)
{
  #ifdef CYCLE
    unsigned long long start = get_current_time();
  #endif
  if(id < 0 || id >= BLOCK_NUM )
  {
    printf("read_block:block id %d out of range",id);
    return;
  }
  if(buf == NULL)
  {
    printf("read_block:parameter buf is NULL");
    return;
  }
  memcpy(buf,blocks[id - 1],BLOCK_SIZE);
  #ifdef CYCLE
    calculate_cycle(1, start);
  #endif
}

void
disk::write_block(blockid_t id, const char *buf)
{
  #ifdef CYCLE
    unsigned long long start = get_current_time();
  #endif
  if(id < 0 || id >= BLOCK_NUM )
  {
    printf("read_block:block id %d out of range",id);
    return;
  }
  if(buf == NULL)
  {
    printf("read_block:parameter buf is NULL");
    return;
  }
  memcpy(blocks[id - 1],buf,BLOCK_SIZE);
  #ifdef CYCLE
    calculate_cycle(2, start);
  #endif
}

// block layer -----------------------------------------

// Allocate a free disk block.
blockid_t block_manager::alloc_block()
{
  /*
   * your code goes here.
   * note: you should mark the corresponding bit in block bitmap when alloc.
   * you need to think about which block you can start to be allocated.
   */
  #ifdef CYCLE
    unsigned long long start_time = get_current_time();
  #endif
  //One block of bitmap
  char bitmap[BLOCK_SIZE];
  //Block number of one block containing bitmap
  int bitmap_bnum,hover;
  //Flag of having found an available free block
  bool found = false;
  //Traverse from the block right after the inode table
  blockid_t start = IBLOCK(INODE_NUM, sb.nblocks) + 1;
  for(bitmap_bnum = BBLOCK(start);bitmap_bnum <= BBLOCK(BLOCK_NUM);++bitmap_bnum)
  {
    d->read_block(bitmap_bnum,bitmap);
    for(hover = 0;hover < BPB;++hover)
    {
      char bit = bitmap[hover / 8] & (0x1 << (7 - hover % 8));
      if(bit == 0)
      {
        found = true;
        //Set the corresponding bit to 1
        bitmap[hover / 8] = bitmap[hover / 8] | (0x1 << (7 - hover % 8));
        d->write_block(bitmap_bnum,bitmap);
        break;
      } 
    }
    if(found)
    {
      blockid_t bnum = (bitmap_bnum - BBLOCK(1)) * BPB + hover + 1;
      return bnum; 
    }
    else
    {
      printf("alloc_block: no free block available");
    }
  }
  #ifdef CYCLE
    calculate_cycle(3, start_time);
  #endif
  return 0;
}

void block_manager::free_block(uint32_t id)
{
  /* 
   * your code goes here.
   * note: you should unmark the corresponding bit in the block bitmap when free.
   */
  #ifdef CYCLE
    unsigned long long start = get_current_time();
  #endif
  if(id <= 0 || id > BLOCK_NUM)
  {
    printf("free_block:block id %d out of range",id);
    return;
  }
  using_blocks[id] = 0;
  #ifdef CYCLE
    calculate_cycle(4, start);
  #endif
  return;
}

// The layout of disk should be like this:
// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|
block_manager::block_manager()
{
  #ifdef CYCLE
    unsigned long long start = get_current_time();
  #endif
  d = new disk();

  // format the disk
  sb.size = BLOCK_SIZE * BLOCK_NUM;
  sb.nblocks = BLOCK_NUM;
  sb.ninodes = INODE_NUM;

  // mark superblock, bitmap, inode table as used
  char buf[BLOCK_SIZE];
  memset(buf, ~0, BLOCK_SIZE);
  int last_bnum = IBLOCK(INODE_NUM, sb.nblocks);

  // blocks in bitmap to be filled with all ones
  for (int bitmap_bnum = BBLOCK(1); bitmap_bnum < BBLOCK(last_bnum); ++bitmap_bnum) 
  {
    d->write_block(bitmap_bnum, buf);
  }

    // the last block in bitmap to partially fill with ones
    // set whole bytes to ones
    memset(buf, 0,  BLOCK_SIZE);
    int remaining_bits_num = last_bnum - (BBLOCK(last_bnum) - BBLOCK(1)) * BPB;
    memset(buf, ~0, remaining_bits_num / 8);

    // set trailing bits to ones
    char last_byte = 0;

    for (int pos = 0; pos < remaining_bits_num % 8; ++pos) 
    {
        last_byte = last_byte | ((char)1 << (7 - pos));
    }
    buf[remaining_bits_num / 8] = last_byte;

    // write last block
    d->write_block(BBLOCK(last_bnum), buf);
  #ifdef CYCLE
    calculate_cycle(5, start);
  #endif
}

void
block_manager::read_block(uint32_t id, char *buf)
{
  #ifdef CYCLE
    unsigned long long start = get_current_time();
  #endif
  d->read_block(id, buf);
  #ifdef CYCLE
    calculate_cycle(6, start);
  #endif
}

void
block_manager::write_block(uint32_t id, const char *buf)
{
  #ifdef CYCLE
    unsigned long long start = get_current_time();
  #endif
  d->write_block(id, buf);
  #ifdef CYCLE
    calculate_cycle(7, start);
  #endif
}

// inode layer -----------------------------------------

inode_manager::inode_manager()
{
  #ifdef CYCLE
    unsigned long long start = get_current_time();
  #endif
  bm = new block_manager();
  uint32_t root_dir = alloc_inode(extent_protocol::T_DIR);
  if (root_dir != 1) {
    printf("\tim: error! alloc first inode %d, should be 1\n", root_dir);
    exit(0);
  }
  #ifdef CYCLE
    calculate_cycle(8, start);
  #endif
}

/* Create a new file.
* Return its inum. */
uint32_t
inode_manager::alloc_inode(uint32_t type)
{
  /* 
   * your code goes here.
   * note: the normal inode block should begin from the 2nd inode block.
   * the 1st is used for root_dir, see inode_manager::inode_manager().
   */
  #ifdef CYCLE
    unsigned long long start = get_current_time();
  #endif
  struct inode *ino = NULL;
  uint32_t inum = 1;
  unsigned int now = (unsigned int)time(NULL);
  char buf[BLOCK_SIZE];

  //Traverse inode table
  for(inum = 1;inum <= INODE_NUM;++inum)
  {
    bm->read_block(IBLOCK(inum,bm->sb.nblocks),buf);
    ino = (struct inode *)buf + (inum - 1) % IPB;
    if(ino->type == 0)
    {
      break;
    }
  }
  if(inum > INODE_NUM)
  {
    printf("alloc_inode: no empty inode available\n");
    return 0;
  }

  ino->atime = now;
  ino->ctime = now;
  ino->mtime = now;
  ino->type = type;
  ino->size = 0;
  //Write inode back to inode table
  bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
  #ifdef CYCLE
    calculate_cycle(9, start);
  #endif
  return inum;
}

void
inode_manager::free_inode(uint32_t inum)
{
  /* 
   * your code goes here.
   * note: you need to check if the inode is already a freed one;
   * if not, clear it, and remember to write back to disk.
   */
  #ifdef CYCLE
    unsigned long long start = get_current_time();
  #endif
  struct inode *ino;
  if((ino = get_inode(inum)) == NULL)
  {
    printf("free_inode:free a freed inode");
    return;
  }
  ino->type = 0;
  put_inode(inum, ino);
  free(ino);
  #ifdef CYCLE
    calculate_cycle(10, start);
  #endif
  return;
}

/* Return an inode structure by inum, NULL otherwise.
 * Caller should release the memory. */
struct inode* 
inode_manager::get_inode(uint32_t inum)
{
  #ifdef CYCLE
    unsigned long long start = get_current_time();
  #endif
  struct inode *ino, *ino_disk;
  char buf[BLOCK_SIZE];

  printf("\tim: get_inode %d\n", inum);

  if (inum < 0 || inum >= INODE_NUM) {
    printf("\tim: inum out of range\n");
    return NULL;
  }

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  // printf("%s:%d\n", __FILE__, __LINE__);

  ino_disk = (struct inode*)buf + inum%IPB;
  if (ino_disk->type == 0) {
    printf("\tim: inode not exist\n");
    return NULL;
  }

  ino = (struct inode*)malloc(sizeof(struct inode));
  *ino = *ino_disk;
  #ifdef CYCLE
    calculate_cycle(11, start);
  #endif
  return ino;
}

void
inode_manager::put_inode(uint32_t inum, struct inode *ino)
{
  #ifdef CYCLE
    unsigned long long start = get_current_time();
  #endif
  char buf[BLOCK_SIZE];
  struct inode *ino_disk;

  printf("\tim: put_inode %d\n", inum);
  if (ino == NULL)
    return;

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  ino_disk = (struct inode*)buf + inum%IPB;
  *ino_disk = *ino;
  bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
  #ifdef CYCLE
    calculate_cycle(12, start);
  #endif
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* Get all the data of a file by inum.
 * Return alloced data, should be freed by caller. */
void 
inode_manager::read_file(uint32_t inum, char **buf_out, int *size) {
  /*
   * your code goes here.
   * note: read blocks related to inode number inum,
   * and copy them to buf_Out
   */
  #ifdef CYCLE
    unsigned long long start = get_current_time();
  #endif
    struct inode *ino = get_inode(inum);
    if (ino == NULL) 
    {
        printf("read_file: inode not exist: %d\n", inum);
        return;
    }

    // alocate memory for reading
    *buf_out = (char *)malloc(ino->size);

    char block_buf[BLOCK_SIZE];
    blockid_t indirect_block_buf[BLOCK_SIZE / sizeof(blockid_t)];
    int block_num = (ino->size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // read direct entry
    #ifdef DIRECT
    for (int i = 0; i < (block_num > NDIRECT ? NDIRECT : block_num); i++) 
    {
        bm->read_block(ino->blocks[i], block_buf);

        if (i == block_num - 1) 
        { 
          // only copy partial data from last block
            memcpy(*buf_out + i * BLOCK_SIZE, block_buf, ino->size - i * BLOCK_SIZE);
        } 
        else 
        {
            memcpy(*buf_out + i * BLOCK_SIZE, block_buf, BLOCK_SIZE);
        }
    }
    #endif

    #ifdef LOOP_UNROLL
    int i;
    for (i = 0; i < (block_num > NDIRECT ? NDIRECT : block_num) - 1; i+=2) 
    {
        bm->read_block(ino->blocks[i], block_buf);

        if (i == block_num - 1) 
        { 
          // only copy partial data from last block
            memcpy(*buf_out + i * BLOCK_SIZE, block_buf, ino->size - i * BLOCK_SIZE);
        } 
        else 
        {
            memcpy(*buf_out + i * BLOCK_SIZE, block_buf, BLOCK_SIZE);
        }
        bm->read_block(ino->blocks[i + 1], block_buf);

        if (i == block_num - 2) 
        { 
          // only copy partial data from last block
            memcpy(*buf_out + (i + 1) * BLOCK_SIZE, block_buf, ino->size - (i + 1) * BLOCK_SIZE);
        } 
        else 
        {
            memcpy(*buf_out + (i + 1) * BLOCK_SIZE, block_buf, BLOCK_SIZE);
        }
    }
    for(; i < (block_num > NDIRECT ? NDIRECT : block_num); i++)
    {
        bm->read_block(ino->blocks[i], block_buf);

        if (i == block_num - 1) 
        { 
          // only copy partial data from last block
            memcpy(*buf_out + i * BLOCK_SIZE, block_buf, ino->size - i * BLOCK_SIZE);
        } 
        else 
        {
            memcpy(*buf_out + i * BLOCK_SIZE, block_buf, BLOCK_SIZE);
        }
    }
    #endif

    // read indirect entry, if needed
    if (block_num > NDIRECT) 
    {
        bm->read_block(ino->blocks[NDIRECT], (char *)indirect_block_buf);
        #ifdef DIRECT
        for (int i = 0; i < block_num - NDIRECT; i++) 
        {
            bm->read_block(indirect_block_buf[i], block_buf);

            if (i == block_num - NDIRECT - 1) 
            { 
              // only copy partial data from last block
                memcpy(*buf_out + (i + NDIRECT) * BLOCK_SIZE, block_buf, ino->size - (i + NDIRECT) * BLOCK_SIZE);
            } 
            else 
            {
                memcpy(*buf_out + (i + NDIRECT) * BLOCK_SIZE, block_buf, BLOCK_SIZE);
            }
        }
        #endif

        #ifdef LOOP_UNROLL
        int j;
        for (j = 0; j < block_num - NDIRECT - 1; j+=2) 
        {
            bm->read_block(indirect_block_buf[j], block_buf);

            if (j == block_num - NDIRECT - 1) 
            { 
              // only copy partial data from last block
                memcpy(*buf_out + (j + NDIRECT) * BLOCK_SIZE, block_buf, ino->size - (j + NDIRECT) * BLOCK_SIZE);
            } 
            else 
            {
                memcpy(*buf_out + (j + NDIRECT) * BLOCK_SIZE, block_buf, BLOCK_SIZE);
            }

            bm->read_block(indirect_block_buf[j + 1], block_buf);

            if (j == block_num - NDIRECT - 2) 
            { 
              // only copy partial data from last block
                memcpy(*buf_out + (j + 1 + NDIRECT) * BLOCK_SIZE, block_buf, ino->size - (j + 1 + NDIRECT) * BLOCK_SIZE);
            } 
            else 
            {
                memcpy(*buf_out + (j + 1 + NDIRECT) * BLOCK_SIZE, block_buf, BLOCK_SIZE);
            }
        }
        for(;j < block_num - NDIRECT;j++)
        {
            bm->read_block(indirect_block_buf[j], block_buf);

            if (j == block_num - NDIRECT - 1) 
            { 
              // only copy partial data from last block
                memcpy(*buf_out + (j + NDIRECT) * BLOCK_SIZE, block_buf, ino->size - (j + NDIRECT) * BLOCK_SIZE);
            } 
            else 
            {
                memcpy(*buf_out + (j + NDIRECT) * BLOCK_SIZE, block_buf, BLOCK_SIZE);
            }
        } 
        #endif
    }

    // report file size
    *size = ino->size;

    // update atime
    unsigned int now = (unsigned int)time(NULL);
    ino->atime = now;
    put_inode(inum, ino);
    free(ino);
  #ifdef CYCLE
    calculate_cycle(13, start);
  #endif
}

void
inode_manager::write_file(uint32_t inum, const char *buf, int size)
{
  /*
   * your code goes here.
   * note: write buf to blocks of inode inum.
   * you need to consider the situation when the size of buf 
   * is larger or smaller than the size of original inode
   */
  #ifdef CYCLE
    unsigned long long start = get_current_time();
  #endif
  struct inode *ino = get_inode(inum);
  int old_size = ino->size;
  int new_size = size;
  blockid_t indirect_buf[BLOCK_SIZE / sizeof(blockid_t)];
  int old_block_num = (old_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
  int new_block_num = (new_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
  if(new_block_num < old_block_num)
  {
    #ifdef DIRECT
    for(int i = 0;i < (new_block_num < NDIRECT ? new_block_num : NDIRECT);++i)
    {
      //If it is the last block, there may be some empty space in the last block
      if(i == (new_block_num - 1))
      {
        char padding[BLOCK_SIZE];
        bzero(padding,BLOCK_SIZE);
        memcpy(padding, buf + i * BLOCK_SIZE, size - i * BLOCK_SIZE);
        bm->write_block(ino->blocks[i], padding);
      }
      else
      {
        bm->write_block(ino->blocks[i], buf + i * BLOCK_SIZE);
      }
    }
    #endif
    #ifdef LOOP_UNROLL
    int i;
    for(i = 0;i < (new_block_num < NDIRECT ? new_block_num : NDIRECT) - 1;i+=2)
    {
      //If it is the last block, there may be some empty space in the last block
      if(i == (new_block_num - 1))
      {
        char padding[BLOCK_SIZE];
        bzero(padding,BLOCK_SIZE);
        memcpy(padding, buf + i * BLOCK_SIZE, size - i * BLOCK_SIZE);
        bm->write_block(ino->blocks[i], padding);
      }
      else
      {
        bm->write_block(ino->blocks[i], buf + i * BLOCK_SIZE);
      }
      if(i == (new_block_num - 1) - 1)
      {
        char padding[BLOCK_SIZE];
        bzero(padding,BLOCK_SIZE);
        memcpy(padding, buf + (i+1) * BLOCK_SIZE, size - (i+1) * BLOCK_SIZE);
        bm->write_block(ino->blocks[i+1], padding);
      }
      else
      {
        bm->write_block(ino->blocks[i+1], buf + (i+1) * BLOCK_SIZE);
      }
    }
    for(;i < (new_block_num < NDIRECT ? new_block_num : NDIRECT);i++)
    {
      if(i == (new_block_num - 1))
      {
        char padding[BLOCK_SIZE];
        bzero(padding,BLOCK_SIZE);
        memcpy(padding, buf + i * BLOCK_SIZE, size - i * BLOCK_SIZE);
        bm->write_block(ino->blocks[i], padding);
      }
      else
      {
        bm->write_block(ino->blocks[i], buf + i * BLOCK_SIZE);
      }
    }
    #endif
    //Write to indirect blocks entries
    if(new_block_num > NDIRECT)
    {
      bm->read_block(ino->blocks[NDIRECT], (char *)indirect_buf);
      #ifdef DIRECT
      for (int i = 0; i < new_block_num - NDIRECT; i++)
      {
        //If it is the last block, there may be some empty space in the last block
        if (i == (new_block_num - NDIRECT - 1))
        {
          char padding[BLOCK_SIZE];
          bzero(padding,BLOCK_SIZE);
          memcpy(padding, buf + (i + NDIRECT) * BLOCK_SIZE, size - (i + NDIRECT) * BLOCK_SIZE);
          bm->write_block(indirect_buf[i], padding);
        }
        else
        {
          bm->write_block(indirect_buf[i], buf + (i + NDIRECT) * BLOCK_SIZE);
        }
      }
      #endif
      #ifdef LOOP_UNROLL
      for (i = 0; i < new_block_num - NDIRECT - 1; i+=2)
      {
        //If it is the last block, there may be some empty space in the last block
        if (i == (new_block_num - NDIRECT - 1))
        {
          char padding[BLOCK_SIZE];
          bzero(padding,BLOCK_SIZE);
          memcpy(padding, buf + (i + NDIRECT) * BLOCK_SIZE, size - (i + NDIRECT) * BLOCK_SIZE);
          bm->write_block(indirect_buf[i], padding);
        }
        else
        {
          bm->write_block(indirect_buf[i], buf + (i + NDIRECT) * BLOCK_SIZE);
        }
        if (i == (new_block_num - NDIRECT - 1) - 1)
        {
          char padding[BLOCK_SIZE];
          bzero(padding,BLOCK_SIZE);
          memcpy(padding, buf + (i + 1 + NDIRECT) * BLOCK_SIZE, size - (i + 1 + NDIRECT) * BLOCK_SIZE);
          bm->write_block(indirect_buf[i + 1], padding);
        }
        else
        {
          bm->write_block(indirect_buf[i + 1], buf + (i + 1 + NDIRECT) * BLOCK_SIZE);
        }
      }
      for (;i < new_block_num - NDIRECT;i++)
      {
        if (i == (new_block_num - NDIRECT - 1))
        {
          char padding[BLOCK_SIZE];
          bzero(padding,BLOCK_SIZE);
          memcpy(padding, buf + (i + NDIRECT) * BLOCK_SIZE, size - (i + NDIRECT) * BLOCK_SIZE);
          bm->write_block(indirect_buf[i], padding);
        }
        else
        {
          bm->write_block(indirect_buf[i], buf + (i + NDIRECT) * BLOCK_SIZE);
        }
      }
      #endif
    }

    //Free unused direct blocks
    for(int i = new_block_num;i < (old_block_num > NDIRECT ? NDIRECT : old_block_num);++i)
    {
      bm->free_block(ino->blocks[i]);
    }

    //Free unused indirect blocks
    if(old_block_num > NDIRECT)
    {
      for (int i = (new_block_num > NDIRECT ? new_block_num - NDIRECT : 0);i < old_block_num - NDIRECT; i++)
      {
        bm->free_block(indirect_buf[i]);
      }
      if (new_block_num <= NDIRECT) 
      {
        bm->free_block(ino->blocks[NDIRECT]);
      }
    }
  }
  else
  {
    //Write to old direct blocks
    for (int i = 0; i < (old_block_num > NDIRECT ? NDIRECT : old_block_num);++i) 
    {
      bm->write_block(ino->blocks[i], buf + i * BLOCK_SIZE);
    }

    //Write to remaining direct blocks
    for (int i = old_block_num; i < (new_block_num > NDIRECT ? NDIRECT : new_block_num); ++i)
    {
      blockid_t block_num = bm->alloc_block();
      ino->blocks[i] = block_num;
      if (i == new_block_num - 1)
      {
        char padding[BLOCK_SIZE];
        bzero(padding,BLOCK_SIZE);
        memcpy(padding, buf + i * BLOCK_SIZE, size - i * BLOCK_SIZE);
        bm->write_block(ino->blocks[i], padding);
      }
      else
      {
        bm->write_block(ino->blocks[i], buf + i * BLOCK_SIZE);
      }
    }

    //Write to indirect blocks entries
    if(new_block_num > NDIRECT)
    {
      if (old_block_num <= NDIRECT)
      {
        blockid_t block_num = bm->alloc_block();
        ino->blocks[NDIRECT] = block_num;
      }
      else 
      {
        bm->read_block(ino->blocks[NDIRECT], (char *)indirect_buf);
      }
      for (int i = 0;i < (old_block_num > NDIRECT ? old_block_num - NDIRECT : 0);++i)
      {
        bm->write_block(indirect_buf[i], buf + (i + NDIRECT) * BLOCK_SIZE);
      }
       for (int i = (old_block_num > NDIRECT ? old_block_num - NDIRECT : 0); i < new_block_num - NDIRECT; ++i)
       {
          blockid_t block_num = bm->alloc_block();
          indirect_buf[i] = block_num;
          if (i == (new_block_num - NDIRECT - 1))
          {
            char padding[BLOCK_SIZE];
            bzero(padding, BLOCK_SIZE);
            memcpy(padding, buf + (i + NDIRECT) * BLOCK_SIZE,size - (i + NDIRECT) * BLOCK_SIZE);
            bm->write_block(indirect_buf[i], padding);
          }
          else
          {
            bm->write_block(indirect_buf[i], buf + (i + NDIRECT) * BLOCK_SIZE);
          }
       }
       if (new_block_num > NDIRECT) 
       {
         bm->write_block(ino->blocks[NDIRECT], (char *)indirect_buf);
       }
    }
  }

  unsigned int now = (unsigned int)time(NULL);
  ino->size  = size;
  ino->mtime = now;
  ino->ctime = now;
  put_inode(inum, ino);
  free(ino);
  #ifdef CYCLE
    calculate_cycle(14, start);
  #endif
  return;
}


void
inode_manager::getattr(uint32_t inum, extent_protocol::attr &a)
{
  /*
   * your code goes here.
   * note: get the attributes of inode inum.
   * you can refer to "struct attr" in extent_protocol.h
   */
  #ifdef CYCLE
    unsigned long long start = get_current_time();
  #endif
  struct inode *ino = get_inode(inum);
  if(ino == NULL)
  {
    printf("getattr:get attribute of a freed inode");
    return;
  }
  a.atime = ino->atime;
  a.ctime = ino->ctime;
  a.mtime = ino->mtime;
  a.size = ino->size;
  a.type = ino->type;
  free(ino);
  #ifdef CYCLE
    calculate_cycle(15, start);
  #endif
  return;
}

void
inode_manager::remove_file(uint32_t inum)
{
  /*
   * your code goes here
   * note: you need to consider about both the data block and inode of the file
   */
  #ifdef CYCLE
    unsigned long long start = get_current_time();
  #endif
  struct inode *ino = get_inode(inum);
  int block_num = (ino->size + BLOCK_SIZE - 1) / BLOCK_SIZE;
  for (int i = 0; i < (block_num > NDIRECT ? NDIRECT : block_num); ++i) 
  {
    bm->free_block(ino->blocks[i]);
  }
  if (block_num > NDIRECT) 
  {
    blockid_t indirect_buf[BLOCK_SIZE / sizeof(blockid_t)];
    bm->read_block(ino->blocks[NDIRECT], (char *)indirect_buf);
    for (int i = 0; i < block_num - NDIRECT; i++) 
    {
      bm->free_block(indirect_buf[i]);
    }
  }
  free_inode(inum);
  free(ino);
  #ifdef CYCLE
    calculate_cycle(16, start);
  #endif
  return;
}
