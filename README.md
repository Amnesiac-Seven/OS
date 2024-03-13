1、点击!cons_nt或!cons_9x文件，进入命令窗口
2、可执行如下操作
  make：像之前一样，生成一个包含操作系统内核及全部应用程序的磁盘映像
  make run：“make”后启动QEMU
  make full:将操作系统核心、apilib和应用程序全部make后生成磁盘映像
  make run_full:“make full”后“make run”
  make clean:本来clean命令是用于清除临时文件的，但由于在这个Makefile中并不生成临时文件，因此这个命令不执行任何操作
  make src_only:将生成的磁盘映像删除以释放磁盘空间
  make clean_full:对操作系统核心、apilib和应用程序全部执行“make clean”，这样将清除所有的临时文件
  make src_only_full:对操作系统核心、apilib和应用程序全部执行“make src_only”， 这样将清除所有的临时文件和最终生成物。不过      执行这个命令后，“make”和“make run”就无法使用了（用带full版本的命令代替即可），make时会消耗更多的时间
  make refresh:“make full”后“make clean_full”。从执行过“make src_only_full”的状态执行这个命令的话，就会恢复到可以直 
     接“make” 和 “make run”的状态
3、进入系统，在命令窗口中输入Makefile中的文件名可实现相关操作，后续将整理所有文件，并更新该部分内容。
