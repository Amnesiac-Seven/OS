; haribote-ipl
; TAB=4

CYLS	EQU		10				; EQU相当于C程序言中的define

		ORG		0x7c00			; 0x00007c00-0x00007dff ：??区内容的装?地址

; 以下?段是?准FAT12格式???用的代?

		JMP		entry
		DB		0x90
		DB		"HARIBOTE"		; ??区的名称可以是任意的字符串（8字?）
		DW		512				; ?个扇区（sector）的大小（必?是512字?）
		DB		1				; 簇（cluster）的大小（必?是1个扇区）
		DW		1				; FAT的起始位置（一般从第一个扇区?始）
		DB		2				; FAT的个数（必?是2）
		DW		224				; 根目的大小（一般成224）
		DW		2880			; 磁?的大小（必?是2880扇区）
		DB		0xf0			; 磁?的位置（必?是0xf0）
		DW		9				; FAT的扇区（必?是9扇区）
		DW		18				; 1个磁道（track）有几个扇区（必?是18）
		DW		2				; 磁?数（必?是2）
		DD		0				; 不使用分区，必?是0
		DD		2880			; 重写一次磁?大小
		DB		0,0,0x29		; 意?不明，固定
		DD		0xffffffff		; （可能是）卷号
		DB		"HARIBOTEOS "	; 磁?的名称（11字?）
		DB		"FAT12   "		; 磁?格式名称（8字?）
		RESB	18				; 先空出18字?

; 程序核心

entry:
		MOV		AX,0			; 初始化寄存器
		MOV		SS,AX
		MOV		SP,0x7c00
		MOV		DS,AX

; 写磁?

		MOV		AX,0x0820
		MOV		ES,AX
		MOV		CH,0			; 柱面0
		MOV		DH,0			; 磁?0
		MOV		CL,2			; 扇区2
readloop:
		MOV		SI,0			; ??失?次数的寄存器
retry:							; 如果出???，??5次，如果都失?就打印??信息
		MOV		AH,0x02			; AH=0x02 : 
		MOV		AL,1			; 1个扇区
		MOV		BX,0
		MOV		DL,0x00			; A??器
		INT		0x13			; ?用磁?BIOS  0x13函数包含了ES至DL的数据
		JNC		next			; 没出?的?跳?到next
		ADD		SI,1			; 往SI加1
		CMP		SI,5			; 比?SI与5
		JAE		error			; SI >= 5?，跳?到error
		MOV		AH,0x00
		MOV		DL,0x00			; A??器
		INT		0x13			; 重置??器
		JMP		retry
next:
		MOV		AX,ES			; 把内存地址后移0x20，由于??是512字?,[ES:BX]中BX位16位，所以ES+=512/16
		ADD		AX,0x0020
		MOV		ES,AX			; 因?没有ADD ES,0x020指令，所以?里稍微?个弯
		ADD		CL,1			; 往CL里加1
		CMP		CL,18			; 比?CL与18
		JBE		readloop		; 如果CL <= 18 跳?至readloop
		MOV		CL,1
		ADD		DH,1
		CMP		DH,2
		JB		readloop		; 如果DH < 2，?跳?到readloop
		MOV		DH,0
		ADD		CH,1
		CMP		CH,CYLS
		JB		readloop		; 如果CH < CYLS，?跳?到readloop

; 読み終わったのでharibote.sysを実行だ！

		MOV		[0x0ff0],CH		; IPLがどこまで読んだのかをメモ
		JMP		0xc200			; ?本P79?已?明0xc200的由来

error:
		MOV		SI,msg
putloop:
		MOV		AL,[SI]
		ADD		SI,1			; ?SI加1
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e			; ?示一个文字
		MOV		BX,15			; 指定字符?色
		INT		0x10			; ?用??BIOS 0x10
		JMP		putloop
fin:
		HLT						; ?CPU停止，等待指令
		JMP		fin				; 无限循?
msg:
		DB		0x0a, 0x0a		; ?行2次
		DB		"load error"
		DB		0x0a			; ?行
		DB		0

		RESB	0x7dfe-$		; 0x7dfeまでを0x00で埋める命令 0x7dfe前所有空位置填充0x00令

		DB		0x55, 0xaa
