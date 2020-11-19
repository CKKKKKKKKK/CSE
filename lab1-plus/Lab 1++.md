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



## 2.