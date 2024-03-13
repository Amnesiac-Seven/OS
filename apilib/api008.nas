[FORMAT "WCOFF"] ; 生成对象文件的模式
[INSTRSET "i486p"] ; 表示使用486兼容指令集
[BITS 32] ; 生成32位模式机器语言
[FILE "api008.nas"] ; 源文件名信息

	GLOBAL _api_initmalloc
		
[SECTION .text]
_api_initmalloc:			; void api_initmalloc(void);
	PUSH 	EBX
	MOV 	EDX,8
	MOV 	EBX,[CS:0x0020] ; 初始化地址 P494页 0x0020 (DWORD) ……malloc空间的起始地址
	MOV 	EAX,EBX 
	ADD		EAX,32*1024		;分配了32k内存
	MOV 	ECX,[CS:0x0000]
	SUB		ECX,EAX			;包括前32位数据（文件信息）以及32k内存
	INT 	0x40
	POP 	EBX
	RET