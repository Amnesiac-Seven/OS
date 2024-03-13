OBJS_BOOTPACK = bootpack.obj naskfunc.obj hankaku.obj graphic.obj dsctbl.obj \
		int.obj fifo.obj keyboard.obj mouse.obj memory.obj sheet.obj timer.obj \
		mtask.obj window.obj file.obj console.obj tek.obj

TOOLPATH = ../../z_tools/
INCPATH  = ../../z_tools/haribote/

MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
RULEFILE = ../haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
GOLIB    = $(TOOLPATH)golib00.exe 
COPY     = copy
DEL      = del

#默认动作
default :
	$(MAKE) ipl20.bin
	$(MAKE) haribote.sys

#文本生成规则
ipl20.bin : ipl20.nas Makefile
	$(NASK) ipl20.nas ipl20.bin ipl20.lst

asmhead.bin : asmhead.nas Makefile
	$(NASK) asmhead.nas asmhead.bin asmhead.lst
	
#使用makefont.exe从hankaku.txt生成hankaku.bin
hankaku.bin : hankaku.txt Makefile
	$(MAKEFONT) hankaku.txt hankaku.bin
	
#使用bin2obj.exe从hankaku.bin生成hankaku.obj
hankaku.obj : hankaku.bin Makefile
	$(BIN2OBJ) hankaku.bin hankaku.obj _hankaku


#使用obi2bim.exe从.obj生成.bim  3MB+64KB=3136KB
bootpack.bim : $(OBJS_BOOTPACK) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map \
		$(OBJS_BOOTPACK)

#使用bim2hrb.exe从bootpack.bim生成bootpack.hrb
bootpack.hrb : bootpack.bim Makefile
	$(BIM2HRB) bootpack.bim bootpack.hrb 0
	
#再使用copy指令将asmhead.bin与bootpack.hrb单纯结合到起来，就成了haribote.sys
haribote.sys : asmhead.bin bootpack.hrb Makefile
	copy /B asmhead.bin+bootpack.hrb haribote.sys

#cc1.exe从.c生成.gas
%.gas : %.c bootpack.h Makefile
	$(CC1) -o $*.gas $*.c

#使用gas2nask.exe从.gas生成.nas
%.nas : %.gas Makefile
	$(GAS2NASK) $*.gas $*.nas

#使用nask.exe从.nas生成.obj
%.obj : %.nas Makefile
	$(NASK) $*.nas $*.obj $*.lst

#命令
clean :
	-$(DEL) asmhead.bin
	-$(DEL) hankaku.bin
	-$(DEL) *.lst
	-$(DEL) *.obj
	-$(DEL) *.map
	-$(DEL) *.bim
	-$(DEL) *.hrb

src_only :
	$(MAKE) clean
	-$(DEL) ipl20.bin
	-$(DEL) haribote.sys
