; haribote-ipl
; TAB=4

CYLS	EQU		9				; EQU�൱��C�����е�define

		ORG		0x7c00			; 0x00007c00-0x00007dff  ָ���������ʼ��ַ

; ���µļ������ڱ�׼FAT12��ʽ������

		JMP		entry
		DB		0x90
		DB		"HARIBOTE"		; �����������ƿ�����������ַ�����8�ֽڣ�
		DW		512				; ÿ��������sector���Ĵ�С������Ϊ512�ֽڣ�
		DB		1				; �أ�cluster���Ĵ�С������Ϊ1��������
		DW		1				; FAT����ʼλ�ã�һ��ӵ�һ��������ʼ��
		DB		2				; FAT�ĸ���������Ϊ2��
		DW		224				; ��Ŀ¼�Ĵ�С��һ�����224�
		DW		2880			; �ô��̵Ĵ�С��������80*18*2=2880������
		DB		0xf0			; ���̵����ࣨ������0xf0��
		DW		9				;  FAT�ĳ��ȣ�������9������
		DW		18				; 1���ŵ���track���м���������������18��
		DW		2				; ��ͷ����������2��
		DD		0				; ��ʹ�÷�����������0
		DD		2880			; ��дһ�δ��̴�С
		DB		0,0,0x29		; ���岻�����̶�
		DD		0xffffffff		; �������ǣ�������
		DB		"HARIBOTEOS "	; ���̵����ƣ�11�ֽڣ�
		DB		"FAT12   "		; ���̸�ʽ���ƣ�8�ֽڣ�
		RESB	18				; �ȿճ�18�ֽ�

; �������

entry:
		MOV		AX,0			; ��ʼ���Ĵ���
		MOV		SS,AX
		MOV		SP,0x7c00
		MOV		DS,AX

; ��ȡ����

		MOV		AX,0x0820
		MOV		ES,AX
		MOV		CH,0			; ����0
		MOV		DH,0			; ��ͷ0
		MOV		CL,2			; ����2
		MOV		BX,18*2*CYLS-1	; ��Ҫ��ȡ����������
		CALL	readfast		; 

; ��ȡ����������haribote.sys��

		MOV		BYTE [0x0ff0],CYLS	; ��¼IPLʵ�ʶ�ȡ�˶�������
		JMP		0xc200

error:
		MOV		SI,msg
putloop:
		MOV		AL,[SI]
		ADD		SI,1			; ��SI��1
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e			; ��ʾһ������
		MOV		BX,15			; ָ���ַ���ɫ
		INT		0x10			; �����Կ�BIOS 0x10
		JMP		putloop
fin:
		HLT						; ��CPUֹͣ���ȴ�ָ��
		JMP		fin				; ����ѭ��
msg:
		DB		0x0a, 0x0a		; ����2��
		DB		"load error"
		DB		0x0a			; ����
		DB		0

readfast:						; ʹ��AL����һ���Զ�ȡ���� �Ӵ˿�ʼ
; ES:��ȡ��ַ, CH:����, DH:��ͷ, CL:����, BX:ʣ�����ȡ������, AL:��¼ÿ�ζ�ȡ��������, AX:��ʼֵ0x0820=2080
; �ۺϵ�ǰ�ŵ�ʣ�����ȡ������(19-CL)��ʣ����BX�����أ�ȷ��ÿ�ζ�ȡ��������AL
		MOV		AX,ES			; <ͨ��ES����AL�����ֵ>
		SHL		AX,3			; AH=(AX*2^3)/2^8=AX/32 ��AX����32�����������AH��SHL������λָ�
		AND		AH,0x7f			; AH &= 0x7f 	        AH��AH����128���õ�������512*128=64K��
		MOV		AL,128			; AL = 128 - AH;  	    AH��AH����128���õ�������512*128=64K��
		SUB		AL,AH

		MOV		AH,BL			; <ͨ��BX����AL�����ֵ������AH>
		CMP		BH,0			; if (BH != 0) { AH = 18; } BH>0��ʾ���ٻ���256����������ȡ
		JE		.skip1
		MOV		AH,18
.skip1:							; BH == 0 ����ȡ�������Ѳ���256��
		CMP		AL,AH			; if (AL > AH) { AL = AH; }
		JBE		.skip2
		MOV		AL,AH
.skip2:							; AL <= AH
		MOV		AH,19			; <ͨ��CL����AL�����ֵ������AH>
		SUB		AH,CL			; AH = 19 - CL ��ǰ���滹ʣ����ȡ��������
		CMP		AL,AH			; if (AL > AH) { AL = AH; }
		JBE		.skip3
		MOV		AL,AH
		
.skip3:

		PUSH	BX
		MOV		SI,0			; ����ʧ�ܴ����ļĴ���
retry:
		MOV		AH,0x02			; AH=0x02 : ��ȡ����
		MOV		BX,0
		MOV		DL,0x00			; A������
		PUSH	ES
		PUSH	DX
		PUSH	CX
		PUSH	AX
		INT		0x13			; ���ô���BIOS
		JNC		next			; û����ʱ��ת��next
		ADD		SI,1			; ��SI��1
		CMP		SI,5			; �Ƚ�SI��5
		JAE		error			; SI >= 5ʱ����ת��error
		MOV		AH,0x00
		MOV		DL,0x00			; A������
		INT		0x13			; ����������
		POP		AX
		POP		CX
		POP		DX
		POP		ES
		JMP		retry
next:
		POP		AX
		POP		CX
		POP		DX
		POP		BX				; ��ӦPUSH	ES,��ESֵ����������BX
		SHR		BX,5			; BX����5��(��Ҫ����Ϊ�μĴ����ļ���ģʽ[ES:BX] ES*0x20+BX)
		MOV		AH,0
		ADD		BX,AX			; BX += AL;
		SHL		BX,5			; Ȼ��������
		MOV		ES,BX			; ES=BX=BX<<5=(BX+AL)<<5=(BX+AL)*0x20=(ES+AL)*0x20  EX += AL * 0x20;
		POP		BX
		SUB		BX,AX
		JZ		.ret
		ADD		CL,AL			; CL+AL
		CMP		CL,18			; �ж�CL�Ƿ����18
		JBE		readfast		; CL <= 18 ����ת��readfast
		MOV		CL,1
		ADD		DH,1
		CMP		DH,2
		JB		readfast		; DH < 2 ����ת��readfast
		MOV		DH,0
		ADD		CH,1
		JMP		readfast
.ret:
		RET
		
		RESB	0x7dfe-$		; 0x7dfe�����뵱ǰλ��ȫ�����0x00

		DB		0x55, 0xaa
