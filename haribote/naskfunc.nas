;naskfunc
;TAB=4

[FORMAT "WCOFF"]		; 制作目标文件的模式
[INSTRSET "i486p"]		; 使用486命令
[BITS 32]				; 制作32位模式用的机械语言
[FILE "naskfunc.nas"]	; 源文件名信息

	GLOBAL	_io_hlt, _io_cli, _io_sti, _io_stihlt
	GLOBAL	_io_in8,  _io_in16,  _io_in32
	GLOBAL	_io_out8, _io_out16, _io_out32
	GLOBAL	_io_load_eflags, _io_store_eflags
	GLOBAL	_load_gdtr, _load_idtr
	GLOBAL	_load_tr
	GLOBAL	_load_cr0, _store_cr0
	GLOBAL	_asm_inthandler0c, _asm_inthandler0d
	GLOBAL	_asm_inthandler20, _asm_inthandler21
	GLOBAL	_asm_inthandler27, _asm_inthandler2c
	GLOBAL	_asm_end_app, _memtest_sub
	GLOBAL	_farjmp, _farcall
	GLOBAL	_asm_hrb_api, _start_app
	EXTERN	_inthandler0c, _inthandler0d
	EXTERN	_inthandler20, _inthandler21
	EXTERN	_inthandler27, _inthandler2c
	EXTERN	_hrb_api

;以下是实际的函数
[SECTION .text]				; 目标文件中写了这些之后再写程序

_io_hlt:					; void io_hlt(void);
		HLT					; 待机
		RET

_io_cli:					; void io_cli(void);
		CLI					; 中断标志置0，禁止中断
		RET
		
_io_sti:					;void io_sti(void);
		STI					; 中断标志置1
		RET
		
_io_stihlt:					;void io_stihlt(void);
		STI
		HLT					; 待机
		RET
		
_io_in8:					; int io_in8(int port);
		MOV		EDX,[ESP+4]	; port端口
		MOV		EAX,0
		IN		AL,DX		;从DX端口读取1个字节(8位)数据到AL
		RET
		
_io_in16: 					; int io_in16(int port);
		MOV 	EDX,[ESP+4] ; port端口
		MOV 	EAX,0
		IN 		AX,DX		;读取2个字节(16位)，即从DX端口读取1个字节数据到AL，从DX+1端口读数据到AH
		RET
_io_in32: 					; int io_in32(int port);
		MOV 	EDX,[ESP+4] ; port端口
		IN 		EAX,DX		; 从DX到DX+3端口连续读取4个字节数据到EAX
		RET	
;port=0x0020，data=0x61   第一个数字port的存放地址：[ESP + 4]，第二个数字data的存放地址：[ESP + 8]
_io_out8:					; void io_out8(int port,int data);
		MOV		EDX,[ESP+4]	; port端口 	EDX=0x0020
		MOV		AL,[ESP+8]	; 数据		AL =0x01
		OUT		DX,AL		; 将数据AL写入DX端口
		RET
		
_io_out16: 				
		MOV 	EDX,[ESP+4] ; port端口
		MOV 	EAX,[ESP+8]	; 数据
		OUT 	DX,AX		; 将数据写入DX及其相邻的2个端口
		RET
		
_io_out32: 				
		MOV 	EDX,[ESP+4] ; port端口
		MOV 	EAX,[ESP+8]	; 数据
		OUT 	DX,EAX		; 将数据写入DX及其相邻的4个端口
		RET	
	
_io_load_eflags:			;  int io_load_eflags(void);函数的返回值为 EAX
		PUSHFD				; 指 PUSH EFLAGS 入栈
		POP		EAX			; 弹出的值代入EAX
		RET

_io_store_eflags:			;  void io_store_eflags(int eflags);
		MOV		EAX,[ESP+4]
		PUSH	EAX
		POPFD				; 指 POP EFLAGS
		RET

; limit=0x0000ffff   addr=0x00270000  存放道ESP的顺序为ffff0000 00007200
_load_gdtr:					; void load_gdtr(int limit, int addr);
		MOV		AX,[ESP+4]	; limit 存放的是段上限   limit的低16位写进AX寄存器，即AX=0xffff
		MOV		[ESP+6],AX	; 将ESP寄存器中的第3 4位赋值为第1 2位数据  ESP:ffffffff 00007200
		LGDT	[ESP+6]		; 将0x00270000ffff装入GDTR寄存器
		RET

_load_idtr:					; void load_idtr(int limit, int addr);参考_load_gdtr函数
		MOV		AX,[ESP+4]	; limit
		MOV		[ESP+6],AX
		LIDT	[ESP+6]
		RET

_load_tr: 					; void load_tr(int tr);向TR寄存器中写入tr值
		LTR 	[ESP+4] ; tr
		RET

_load_cr0:						; int load_cr0(void)
		MOV 	EAX,CR0
		RET
		
_store_cr0: 					; void store_cr0(int cr0)
		MOV 	EAX,[ESP+4]
		MOV 	CR0,EAX
		RET
;栈异常
_asm_inthandler0c:
		STI
		PUSH 	ES
		PUSH 	DS
		PUSHAD
		MOV 	EAX,ESP
		PUSH 	EAX
		MOV 	AX,SS
		MOV 	DS,AX
		MOV 	ES,AX
		CALL 	_inthandler0c
		CMP 	EAX,0 			; 只有这里不同
		JNE 	_asm_end_app	; 只有这里不同
		POP 	EAX
		POPAD
		POP 	DS
		POP 	ES
		ADD 	ESP,4 			; 在INT 0x0d中需要这句
		IRETD
;一般性保护异常
_asm_inthandler0d:
		STI
		PUSH 	ES
		PUSH 	DS
		PUSHAD
		MOV 	EAX,ESP
		PUSH 	EAX
		MOV 	AX,SS
		MOV 	DS,AX
		MOV 	ES,AX
		CALL 	_inthandler0d
		CMP 	EAX,0 			; 只有这里不同
		JNE 	_asm_end_app	; 只有这里不同
		POP 	EAX
		POPAD
		POP 	DS
		POP 	ES
		ADD 	ESP,4 			; 在INT 0x0d中需要这句
		IRETD

;计时器中断
_asm_inthandler20:
		PUSH 	ES
		PUSH 	DS
		PUSHAD
		MOV 	EAX,ESP
		PUSH 	EAX
		MOV 	AX,SS
		MOV 	DS,AX
		MOV 	ES,AX
		CALL 	_inthandler20
		POP 	EAX
		POPAD
		POP 	DS
		POP 	ES
		IRETD
		
;INT 0x21=33 为键盘	
_asm_inthandler21:
		PUSH 	ES
		PUSH 	DS
		PUSHAD
		MOV 	EAX,ESP
		PUSH 	EAX
		MOV 	AX,SS
		MOV 	DS,AX
		MOV 	ES,AX
		CALL 	_inthandler21
		POP 	EAX
		POPAD
		POP 	DS
		POP 	ES
		IRETD
		
;INT 0x27=39 	
_asm_inthandler27:
		PUSH 	ES
		PUSH 	DS
		PUSHAD
		MOV 	EAX,ESP
		PUSH 	EAX
		MOV 	AX,SS
		MOV 	DS,AX
		MOV 	ES,AX
		CALL 	_inthandler27
		POP 	EAX
		POPAD
		POP 	DS
		POP 	ES
		IRETD
		
;INT 0x2c=44 为PS/2鼠标	
_asm_inthandler2c:
		PUSH 	ES
		PUSH 	DS
		PUSHAD
		MOV 	EAX,ESP
		PUSH 	EAX
		MOV 	AX,SS
		MOV 	DS,AX
		MOV 	ES,AX
		CALL 	_inthandler2c
		POP 	EAX
		POPAD
		POP 	DS
		POP 	ES
		IRETD

;调查从start地址到end地址的范围内，能够使用的内存的末尾地址
_memtest_sub: 					; unsigned int memtest_sub(unsigned int start, unsigned int end)
		PUSH 	EDI 			; （由于还要使用EBX, ESI, EDI）
		PUSH 	ESI
		PUSH 	EBX
		MOV 	ESI,0xaa55aa55 	; pat0 = 0xaa55aa55;
		MOV 	EDI,0x55aa55aa 	; pat1 = 0x55aa55aa;
		MOV 	EAX,[ESP+12+4] 	; i = start;
mts_loop:
		MOV 	EBX,EAX
		ADD 	EBX,0xffc 		; p = i + 0xffc;
		MOV 	EDX,[EBX] 		; old = *p;
		MOV 	[EBX],ESI 		; *p = pat0;
		XOR 	DWORD [EBX],0xffffffff ; *p ^= 0xffffffff;
		CMP 	EDI,[EBX] 		; if (*p != pat1) goto fin;
		JNE 	mts_fin
		XOR 	DWORD [EBX],0xffffffff ; *p ^= 0xffffffff;
		CMP 	ESI,[EBX] 		; if (*p != pat0) goto fin;
		JNE 	mts_fin
		MOV 	[EBX],EDX 		; *p = old;
		ADD 	EAX,0x1000 		; i += 0x1000;
		CMP 	EAX,[ESP+12+8] 	; if (i <= end) goto mts_loop;
		JBE 	mts_loop
		POP 	EBX
		POP 	ESI
		POP 	EDI
		RET
mts_fin:
		MOV 	[EBX],EDX 		; *p = old;
		POP 	EBX
		POP 	ESI
		POP 	EDI
		RET

_farjmp: 						; void farjmp(int eip, int cs);
		JMP 	FAR [ESP+4] 	; eip, cs
		RET
		
_farcall: 						; void farcall(int eip, int cs);
		CALL 	FAR [ESP+4] 	; eip, cs
		RET
		
;void hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax)
_asm_hrb_api:
; 为方便起见从开头就禁止中断请求
		PUSH 	DS
		PUSH 	ES
		PUSHAD 					; 用于保存的PUSH
		PUSHAD 					; 用于向hrb_api传值的PUSH
		MOV 	AX,SS
		MOV 	DS,AX 			; 将操作系统用段地址存入DS和ES
		MOV 	ES,AX
		CALL 	_hrb_api
		CMP		EAX,0 			; 当EAX不为0时程序结束
		JNE 	_asm_end_app
		ADD 	ESP,32
		POPAD
		POP 	ES
		POP 	DS
		IRETD
_asm_end_app:
		; EAX为tss.esp0的地址
		MOV 	ESP,[EAX]
		MOV 	DWORD [EAX+4],0 
		POPAD
		RET 					; 返回cmd_app
		
;void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);
;操作系统栈的ESP保存在0xfe4这个地址，以便从应用程序返回操作系统时使用
_start_app:
		PUSHAD ; 将32位寄存器的值全部保存下来
		MOV 	EAX,[ESP+36] 	; 应用程序用EIP
		MOV 	ECX,[ESP+40] 	; 应用程序用CS
		MOV 	EDX,[ESP+44] 	; 应用程序用ESP
		MOV 	EBX,[ESP+48] 	; 应用程序用DS/SS
		MOV 	EBP,[ESP+52] 	; tss.esp0的地址
		MOV 	[EBP ],ESP 		; 保存操作系统用ESP
		MOV 	[EBP+4],SS 		; 保存操作系统用SS
		MOV 	ES,BX
		MOV 	DS,BX
		MOV 	FS,BX
		MOV 	GS,BX
		; 下面调整栈，以免用RETF跳转到应用程序
		OR 		ECX,3 			; 将应用程序用段号和3进行OR运算
		OR 		EBX,3 			; 将应用程序用段号和3进行OR运算
		PUSH 	EBX 			; 应用程序的SS
		PUSH 	EDX 			; 应用程序的ESP
		PUSH 	ECX 			; 应用程序的CS
		PUSH 	EAX 			; 应用程序的EIP
		RETF
; 应用程序结束后不会回到这里
		