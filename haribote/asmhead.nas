; haribote-os boot asm
; TAB=4

[INSTRSET "i486p"]

VBEMODE EQU		0x105			;1024 x  768 x 8bit
; 画面模式号码如下
;	0x100 :  640 x  400 x 8bit
;	0x101 :  640 x  480 x 8bit
;	0x103 :  800 x  600 x 8bit
;	0x105 : 1024 x  768 x 8bit
;	0x107 : 1280 x 1024 x 8bit

BOTPAK	EQU		0x00280000		; bootpackのロード先
DSKCAC	EQU		0x00100000		; ディスクキャッシュの場所
DSKCAC0	EQU		0x00008000		; ディスクキャッシュの場所（リアルモード）

;有关BOOT_INFO
CYLS	EQU		0x0ff0			;设定启动区
LEDS	EQU		0x0ff1
VMODE	EQU		0x0ff2			;关于色彩数目的信息，颜色位数
SCRNX	EQU		0x0ff4			;分辨率的x
SCRNY	EQU		0x0ff6			;分辨率的y
VRAM	EQU		0x0ff8			;图像缓冲区的开始地址

		ORG 	0xc200			;程序要装载到的位置，书本P79页已说明0xc200的由来
		
; 确认VBE是否存在
		MOV 	AX,0x9000
		MOV 	ES,AX
		MOV 	DI,0
		MOV 	AX,0x4f00
		INT 	0x10
		CMP 	AX,0x004f
		JNE 	scrn320
		
; 检查VBE的版本
		MOV 	AX,[ES:DI+4]
		CMP 	AX,0x0200
		JB 		scrn320 		; if (AX < 0x0200) goto scrn320
		
; 取得画面模式信息
		MOV		CX,VBEMODE
		MOV		AX,0x4f01
		INT		0x10
		CMP		AX,0x004f
		JNE		scrn320
		
; 画面模式信息的确认
		CMP		BYTE [ES:DI+0x19],8
		JNE		scrn320
		CMP		BYTE [ES:DI+0x1b],4
		JNE		scrn320
		MOV		AX,[ES:DI+0x00]
		AND		AX,0x0080
		JZ		scrn320			; 模式属性的bit7是0，所以放弃

; 画面模式的切换
		MOV		BX,VBEMODE+0x4000
		MOV		AX,0x4f02
		INT		0x10
		MOV		BYTE [VMODE],8	; 记下画面模式（参考C语言）
		MOV		AX,[ES:DI+0x12]
		MOV		[SCRNX],AX
		MOV		AX,[ES:DI+0x14]
		MOV		[SCRNY],AX
		MOV		EAX,[ES:DI+0x28]
		MOV		[VRAM],EAX
		JMP		keystatus

scrn320:
		MOV 	AL,0x13			;VGA显卡，320*200*8彩色
		MOV		AH,0x00
		INT		0x10
		MOV		BYTE [VMODE],8	;记录画面模式
		MOV		WORD [SCRNX],320;两个字节
		MOV		WORD [SCRNY],200;两个字节
		MOV		DWORD [VRAM],0x000a0000;VRAM是用来显示画面的内存，地址0xa0000～0xaffff的64KB
		
		
;用BIOS取得键盘上各种LED指示灯的状态
keystatus:
		MOV		AH,0x02
		INT		0x16			;keyboard BIOS
		MOV		[LEDS],AL
		
		;下面5行代码的意思为io_out(PIC0_IMR, 0xff);io_out(PIC1_IMR, 0xff);io_cli();
		;禁止主PIC的全部中断;禁止从PIC的全部中断;禁止CPU级别的中断
		MOV		AL,0xff		
		OUT		0x21,AL
		NOP
		OUT		0xa1,AL
		CLI
		
		; 为了让CPU能够访问1MB以上的内存空间，设定A20GATE
		; 0x64 0x60端口分别为命令端口和数据端口
		CALL	waitkbdout
		MOV		AL,0xd1
		OUT		0x64,AL
		CALL	waitkbdout
		MOV		AL,0xdf			; enable A20
		OUT		0x60,AL
		CALL	waitkbdout

[INSTRSET "i486p"]				; “想要使用486指令”的叙述
		LGDT	[GDTR0]			; 设定临时GDT
		MOV		EAX,CR0
		AND		EAX,0x7fffffff	; 设bit31为0（为了禁止分页）
		OR		EAX,0x00000001	; 设bit0为1（为了切换到保护模式）
		MOV		CR0,EAX
		JMP		pipelineflush
		
pipelineflush:
		MOV		AX,1*8			; 可读写的段 32bit
		MOV		DS,AX
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX
		
; bootpack的转送
		;相当于memcpy(bootpack, BOTPAK, 512*1024/4);
		MOV		ESI,bootpack	; 转送源
		MOV		EDI,BOTPAK		; 转送目的地
		MOV		ECX,512*1024/4
		CALL	memcpy
		
; 磁盘数据最终转送到它本来的位置去
; 首先从启动扇区开始
		;相当于memcpy(0x7c00, DSKCAC, 512/4 );
		MOV		ESI,0x7c00		; 转送源
		MOV		EDI,DSKCAC		; 转送目的地
		MOV		ECX,512/4
		CALL	memcpy
		
		;相当于memcpy(DSKCAC0+512, DSKCAC+512, cyls * 512*18*2/4 - 512/4);
		MOV		ESI,DSKCAC0+512	; 转送源
		MOV		EDI,DSKCAC+512	; 转送目的地
		MOV		ECX,0
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4	; IMUL乘法，从柱面数变换为字节数/4
		SUB		ECX,512/4		; SUB减法，减去 IPL
		CALL	memcpy
		
		; bootpack的启动
		MOV		EBX,BOTPAK		;0x00280000
		MOV		ECX,[EBX+16]	;0x11a8
		ADD		ECX,3			
		SHR		ECX,2			; ECX /= 4;	
		JZ		skip			; 没有要转送的东西时
		MOV		ESI,[EBX+20]	; 转送源0x10c8
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]	; 转送目的地0x00310000
		CALL	memcpy
		
skip:
		MOV		ESP,[EBX+12]	; 栈初始值
		JMP		DWORD 2*8:0x0000001b; far模式EIP=0x1b, CS=2*8
		
waitkbdout:
		IN		AL,0x64
		AND		AL,0x02
		JNZ		waitkbdout
		RET
		
memcpy:							; 与C语言中memcpy函数实现相同的功能
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy
		RET
		
		ALIGNB	16
GDT0:
		RESB	8
		DW		0xffff,0x0000,0x9200,0x00cf ; 可以读写的段（segment）32bit
		DW		0xffff,0x0000,0x9a28,0x0047 ; 可以执行的段（segment）32bit（bootpack用）
		
		DW 		0
		
GDTR0:
		DW 		8*3-1
		DD		GDT0
		
		ALIGNB  16

bootpack: