# Lab 1++: Tune Your FS!

姓名：程可

学号：518021910095



## 1. 初始性能差距

Native file system:

```powershell
root@ics:/mnt/hgfs/lab-cse/lab1# ./fxmark/bin/fxmark --type=YFS --root=./native --ncore=1 --duration=5
# ncpu secs works works/sec 
1 5.097093 3968.000000 778.482951
```

YFS:

```powershell
root@ics:/mnt/hgfs/lab-cse/lab1# ./fxmark/bin/fxmark --type=YFS --root=./yfs1 --ncore=1 --duration=5
# ncpu secs works works/sec 
1 6.199136 512.000000 82.592155
```



## 2. 寻找性能瓶颈

​	为了寻找性能瓶颈，我调用了_rdtsc()函数来测量某一个函数的微观的CPU cycle数，测量方法如下：

```c++
#define func_num 18
#define CYCLE
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
  if (min_cycle[func_num] == 0 || cycles < min_cycle[func_num])
  {
    min_cycle[func_no] = cycles;
  }
  fp = fopen("cycle.txt", "a");
  fprintf(fp, "%s%d %s %d %s %s %lld  %s %lld\n", "Function", func_no, "called", call_count[func_no], "times", "Max cycle:", max_cycle[func_no], "Min cycle:", min_cycle[func_no]);

  fclose(fp);
}
```

​	测量的时候只要将

```c++
#ifdef CYCLE
unsigned long long start = get_current_time();
#endif
```

​	加在被测试代码片段的开头， 将

```
#ifdef CYCLE
calculate_cycle(0, start);
#endif
```

​	加在被测试代码片段的结束即可测试。



### inode_manager.cc

| 函数名                       | 被调用次数 | 最大cycle数 | 最小cycle数 |
| ---------------------------- | ---------- | ----------- | ----------- |
| disk::disk                   | 1          | 25697838    | 25697838    |
| disk::read_block             | 7184       | 26331       | 535         |
| disk::write_block            | 3977       | 25660       | 315         |
| block_manager::alloc_block   | 0          | /           | /           |
| block_manager::free_block    | 1152       | 26232       | 2735        |
| block_manager::block_manager | 1          | 30166784    | 30166784    |
| block_manager::read_block    | 6031       | 12435752    | 626982      |
| block_manager::write_block   | 2823       | 32497106    | 1113395     |
| inode_manager::inode_manager | 1          | 60321564    | 60321564    |
| inode_manager::alloc_inode   | 130        | 29127416    | 8156378     |
| inode_manager::free_inode    | 128        | 40844874    | 9532312     |
| inode_manager::get_inode     | 3592       | 17004523    | 2183364     |
| inode_manager::put_inode     | 1540       | 41429798    | 4739086     |
| inode_manager::read_file     | 1027       | 51937976    | 9201138     |
| inode_manager::write_file    | 385        | 187636712   | 14446537    |
| inode_manager::getattr       | 1924       | 33136153    | 3311680     |
| inode_manager::remove_file   | 128        | 69029921    | 23563854    |



### yfs_client.cc

| 函数名                         | 被调用次数 | 最大cycle数 | 最小cycle数 |
| ------------------------------ | ---------- | ----------- | ----------- |
| yfs_client::yfs_client         | 1          | 96045415    | 96045415    |
| yfs_client::yfs_client         | 0          | /           | /           |
| yfs_client::n2i                | 0          | /           | /           |
| yfs_client::add_entry_and_save | 129        | 61130823    | 38497621    |
| yfs_client::has_duplicate      | 0          | /           | /           |
| yfs_client::filename           | 0          | /           | /           |
| yfs_client::isfile             | 1154       | 31183082    | 5468489     |
| yfs_client::isdir              | 257        | 13888232    | 5401466     |
| yfs_client::getfile            | 256        | 39805603    | 5587524     |
| yfs_client::getdir             | 257        | 25884116    | 5226918     |
| yfs_client::getlink            | 0          | /           | /           |
| yfs_client::setattr            | 0          | /           | /           |
| yfs_client::create             | 128        | 109172280   | 91354277    |
| yfs_client::mkdir              | 1          | 64896681    | 64896681    |
| yfs_client::lookup             | 642        | 55766159    | 15092110    |
| yfs_client::readdir            | 899        | 54444302    | 12821005    |
| yfs_client::writedir           | 257        | 39362426    | 14467083    |
| yfs_client::read               | 0          | /           | /           |
| yfs_client::write              | 128        | 172767198   | 99782224    |
| yfs_client::readlink           | 0          | /           | /           |
| yfs_client::symlink            | 0          | /           | /           |
| yfs_client::unlink             | 128        | 132078204   | 79125227    |

