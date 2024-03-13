/*关于GDT、IDT等descriptor table的处理*/

#include "bootpack.h"

// 初始化GDT和IDT
void init_gdtidt(void){
	struct SEGMENT_DESCRIPTOR *gdt=(struct SEGMENT_DESCRIPTOR*)ADR_GDT;// 将0x270000～0x27ffff设为GDT
	struct GATE_DESCRIPTOR *idt=(struct GATE_DESCRIPTOR*)ADR_IDT;// IDT被设为了0x26f800～0x26ffff
	int i;
	
	// 初始化GDT
	for(i=0; i<=LIMIT_GDT / 8; i++){/*LIMIT_GDT为65536位  GDT的个数为8162=LIMIT_GDT/8 */
		set_segmdesc(gdt+i, 0, 0, 0);
	}
	set_segmdesc(gdt+1, 0xffffffff, 0x00000000, AR_DATA32_RW);// 数据段
	set_segmdesc(gdt+2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);// 代码段
	load_gdtr(LIMIT_GDT, ADR_GDT);
	
	// 初始化IDT
	for(i=0; i<=LIMIT_IDT / 8; i++){/*LIMIT_IDT为2048位  IDT的个数为256=LIMIT_IDT/8 */
		set_gatedesc(idt+i, 0, 0, 0);
	}
	load_idtr(LIMIT_IDT, ADR_IDT);
	
	// 设置IDT  2*8表示的是asm_inthandler21属于哪一个段，即段号是2，
	// 乘以8是因为低3位有着别的意思，这里低3位必须是0
	set_gatedesc(idt + 0x0c, (int) asm_inthandler0c, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x0d, (int) asm_inthandler0d, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x20, (int) asm_inthandler20, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x21, (int) asm_inthandler21, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x27, (int) asm_inthandler27, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x2c, (int) asm_inthandler2c, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x40, (int) asm_hrb_api,      2 * 8, AR_INTGATE32+0x60);
	
	return;
}

// 设置GDT的相关信息
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar){
	if(limit>0xfffff){// limit需要20位
		ar|=0x8000;//修改D/B标志
		limit/=0x1000;
	}
	// 按照《深入理解linux内核》P44页
	sd->limit_low    = limit&0xffff;
	sd->base_low     = base&0xffff;
	sd->base_mid     = (base>>16)&0xff;
	sd->access_right = ar&0xff;
	sd->limit_high   = ((limit>>16)&0x0f)|((ar>>8)&0xf0);
	sd->base_high    = (base>>24)&0xff;
	return;
}
// 设置IDT的相关信息
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar){
	// 按照《深入理解linux内核》P144页
	gd->offset_low   = offset&0xffff;
	gd->selector     = selector;
	gd->dw_count     = (ar>>8)&0xff;
	gd->access_right = ar&0xff;
	gd->offset_high  = (offset>>16)&0xffff;
	return;
}