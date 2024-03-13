; haribote-ipl
; TAB=4

CYLS	EQU		10				; EQU������C���������Idefine

		ORG		0x7c00			; 0x00007c00-0x00007dff �F??����e�I��?�n��

; �ȉ�?�i��?�yFAT12�i��???�p�I��?

		JMP		entry
		DB		0x90
		DB		"HARIBOTE"		; ??��I���̉Ȑ��C�ӓI�������i8��?�j
		DW		512				; ?�����isector�j�I�召�i�K?��512��?�j
		DB		1				; �Ɓicluster�j�I�召�i�K?��1�����j
		DW		1				; FAT�I�N�n�ʒu�i��ʘ���꘢���?�n�j
		DB		2				; FAT�I�����i�K?��2�j
		DW		224				; ���ړI�召�i��ʐ�224�j
		DW		2880			; ��?�I�召�i�K?��2880���j
		DB		0xf0			; ��?�I�ʒu�i�K?��0xf0�j
		DW		9				; FAT�I���i�K?��9���j
		DW		18				; 1�������itrack�j�L�{�����i�K?��18�j
		DW		2				; ��?���i�K?��2�j
		DD		0				; �s�g�p����C�K?��0
		DD		2880			; �d�ʈꎟ��?�召
		DB		0,0,0x29		; ��?�s���C�Œ�
		DD		0xffffffff		; �i�\���j�ɍ�
		DB		"HARIBOTEOS "	; ��?�I���́i11��?�j
		DB		"FAT12   "		; ��?�i�����́i8��?�j
		RESB	18				; ���o18��?

; �����j�S

entry:
		MOV		AX,0			; ���n���񑶊�
		MOV		SS,AX
		MOV		SP,0x7c00
		MOV		DS,AX

; �ʎ�?

		MOV		AX,0x0820
		MOV		ES,AX
		MOV		CH,0			; ����0
		MOV		DH,0			; ��?0
		MOV		CL,2			; ���2
readloop:
		MOV		SI,0			; ??��?�����I�񑶊�
retry:							; �@�ʏo???�C??5���C�@�ʓs��?�A�ň�??�M��
		MOV		AH,0x02			; AH=0x02 : 
		MOV		AL,1			; 1�����
		MOV		BX,0
		MOV		DL,0x00			; A??��
		INT		0x13			; ?�p��?BIOS  0x13������ܗ�ES��DL�I����
		JNC		next			; �v�o?�I?��?��next
		ADD		SI,1			; ��SI��1
		CMP		SI,5			; ��?SI�^5
		JAE		error			; SI >= 5?�C��?��error
		MOV		AH,0x00
		MOV		DL,0x00			; A??��
		INT		0x13			; �d�u??��
		JMP		retry
next:
		MOV		AX,ES			; �c�����n���@��0x20�C�R��??��512��?,[ES:BX]��BX��16�ʁC����ES+=512/16
		ADD		AX,0x0020
		MOV		ES,AX			; ��?�v�LADD ES,0x020�w�߁C����?���c��?���^
		ADD		CL,1			; ��CL����1
		CMP		CL,18			; ��?CL�^18
		JBE		readloop		; �@��CL <= 18 ��?��readloop
		MOV		CL,1
		ADD		DH,1
		CMP		DH,2
		JB		readloop		; �@��DH < 2�C?��?��readloop
		MOV		DH,0
		ADD		CH,1
		CMP		CH,CYLS
		JB		readloop		; �@��CH < CYLS�C?��?��readloop

; �ǂݏI������̂�haribote.sys�����s���I

		MOV		[0x0ff0],CH		; IPL���ǂ��܂œǂ񂾂̂�������
		JMP		0xc200			; ?�{P79?��?��0xc200�I�R��

error:
		MOV		SI,msg
putloop:
		MOV		AL,[SI]
		ADD		SI,1			; ?SI��1
		CMP		AL,0
		JE		fin
		MOV		AH,0x0e			; ?���꘢����
		MOV		BX,15			; �w�莚��?�F
		INT		0x10			; ?�p??BIOS 0x10
		JMP		putloop
fin:
		HLT						; ?CPU��~�C���Ҏw��
		JMP		fin				; �ٌ��z?
msg:
		DB		0x0a, 0x0a		; ?�s2��
		DB		"load error"
		DB		0x0a			; ?�s
		DB		0

		RESB	0x7dfe-$		; 0x7dfe�܂ł�0x00�Ŗ��߂閽�� 0x7dfe�O���L��ʒu�U�[0x00��

		DB		0x55, 0xaa
