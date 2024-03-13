; haribote-ipl
; TAB=4

CYLS	EQU		9				; EQU相当于C语言中的define

		ORG		0x7c00			; 0x00007c00-0x00007dff  指明程序的起始地址

; 以下的记述用于标准FAT12格式的软盘

		JMP		entry
		DB		0x90
		DB		"HARIBOTE"		; 启动区的名称可以是任意的字符串（8字节）
		DW		512				; 每个扇区（sector）的大小（必须为512字节）
		DB		1				; 簇（cluster）的大小（必须为1个扇区）
		DW		1				; FAT的起始位置（一般从第一个扇区开始）
		DB		2				; FAT的个数（必须为2）
		DW		224				; 根目录的大小（一般设成224项）
		DW		2880			; 该磁盘的大小（必须是80*18*2=2880扇区）
		DB		0xf0			; 磁盘的种类（必须是0xf0）
		DW		9				;  FAT的长度（必须是9扇区）
		DW		18				; 1个磁道（track）有几个扇区（必须是18）
		DW		2				; 磁头数（必须是2）
		DD		0				; 不使用分区，必须是0
		DD		2880			; 重写一次磁盘大小
		DB		0,0,0x29		; 意义不明，固定
		DD		0xffffffff		; （可能是）卷标号码
		DB		"HARIBOTEOS "	; 磁盘的名称（11字节）
		DB		"FAT12   "		; 磁盘格式名称（8字节）
		RESB	18				; 先空出18字节

; 程序核心

entry:
		MOV		AX,0			; 初始化寄存器
		MOV		SS,AX
		MOV		SP,0x7c00
		MOV		DS,AX

; 读取磁盘

		MOV		AX,0x0820
		MOV		ES,AX
		MOV		CH,0			; 柱面0
		MOV		DH,0			; 磁头0
		MOV		CL,2			; 扇区2
		MOV		BX,18*2*CYLS-1	; 需要读取的扇区个数
		CALL	readfast		; 

; 读取结束，运行haribote.sys！

		MOV		BYTE [0x0ff0],CYLS	; 记录IPL实际读取了多少内容
		JMP		0xc200

error:
		MOV		SI,msg
putloop:
		MOV		AL,[SI]
		ADD		SI,1			; 给SI加1
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e			; 显示一个文字
		MOV		BX,15			; 指定字符颜色
		INT		0x10			; 调用显卡BIOS 0x10
		JMP		putloop
fin:
		HLT						; 让CPU停止，等待指令
		JMP		fin				; 无限循环
msg:
		DB		0x0a, 0x0a		; 换行2次
		DB		"load error"
		DB		0x0a			; 换行
		DB		0

readfast:						; 使用AL尽量一次性读取数据 从此开始
; ES:读取地址, CH:柱面, DH:磁头, CL:扇区, BX:剩余待读取扇区数, AL:记录每次读取的扇区数, AX:初始值0x0820=2080
; 综合当前磁道剩余待读取的扇区(19-CL)和剩余量BX等因素，确定每次读取的扇区数AL
		MOV		AX,ES			; <通过ES计算AL的最大值>
		SHL		AX,3			; AH=(AX*2^3)/2^8=AX/32 将AX除以32，将结果存入AH（SHL是左移位指令）
		AND		AH,0x7f			; AH &= 0x7f 	        AH是AH除以128所得的余数（512*128=64K）
		MOV		AL,128			; AL = 128 - AH;  	    AH是AH除以128所得的余数（512*128=64K）
		SUB		AL,AH

		MOV		AH,BL			; <通过BX计算AL的最大值并存入AH>
		CMP		BH,0			; if (BH != 0) { AH = 18; } BH>0表示至少还有256个扇区待读取
		JE		.skip1
		MOV		AH,18
.skip1:							; BH == 0 待读取的扇区已不足256个
		CMP		AL,AH			; if (AL > AH) { AL = AH; }
		JBE		.skip2
		MOV		AL,AH
.skip2:							; AL <= AH
		MOV		AH,19			; <通过CL计算AL的最大值并存入AH>
		SUB		AH,CL			; AH = 19 - CL 当前柱面还剩待读取的扇区数
		CMP		AL,AH			; if (AL > AH) { AL = AH; }
		JBE		.skip3
		MOV		AL,AH
		
.skip3:

		PUSH	BX
		MOV		SI,0			; 计算失败次数的寄存器
retry:
		MOV		AH,0x02			; AH=0x02 : 读取磁盘
		MOV		BX,0
		MOV		DL,0x00			; A驱动器
		PUSH	ES
		PUSH	DX
		PUSH	CX
		PUSH	AX
		INT		0x13			; 调用磁盘BIOS
		JNC		next			; 没出错时跳转到next
		ADD		SI,1			; 往SI加1
		CMP		SI,5			; 比较SI与5
		JAE		error			; SI >= 5时，跳转到error
		MOV		AH,0x00
		MOV		DL,0x00			; A驱动器
		INT		0x13			; 重置驱动器
		POP		AX
		POP		CX
		POP		DX
		POP		ES
		JMP		retry
next:
		POP		AX
		POP		CX
		POP		DX
		POP		BX				; 对应PUSH	ES,将ES值弹出并赋给BX
		SHR		BX,5			; BX右移5，(主要是因为段寄存器的计算模式[ES:BX] ES*0x20+BX)
		MOV		AH,0
		ADD		BX,AX			; BX += AL;
		SHL		BX,5			; 然后再右移
		MOV		ES,BX			; ES=BX=BX<<5=(BX+AL)<<5=(BX+AL)*0x20=(ES+AL)*0x20  EX += AL * 0x20;
		POP		BX
		SUB		BX,AX
		JZ		.ret
		ADD		CL,AL			; CL+AL
		CMP		CL,18			; 判断CL是否大于18
		JBE		readfast		; CL <= 18 则跳转到readfast
		MOV		CL,1
		ADD		DH,1
		CMP		DH,2
		JB		readfast		; DH < 2 则跳转到readfast
		MOV		DH,0
		ADD		CH,1
		JMP		readfast
.ret:
		RET
		
		RESB	0x7dfe-$		; 0x7dfe到代码当前位置全部填充0x00

		DB		0x55, 0xaa
