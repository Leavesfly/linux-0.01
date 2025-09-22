#ifndef _HEAD_H
#define _HEAD_H

/* 描述符结构定义 */
typedef struct desc_struct {
	unsigned long a,b;
} desc_table[256];

/* 全局变量声明 */
extern unsigned long pg_dir[1024];	/* 页目录 */
extern desc_table idt,gdt;		/* 中断描述符表和全局描述符表 */

/* GDT选择符定义 */
#define GDT_NUL 0	/* 空描述符 */
#define GDT_CODE 1	/* 代码段描述符 */
#define GDT_DATA 2	/* 数据段描述符 */
#define GDT_TMP 3	/* 临时描述符 */

/* LDT选择符定义 */
#define LDT_NUL 0	/* 空描述符 */
#define LDT_CODE 1	/* 代码段描述符 */
#define LDT_DATA 2	/* 数据段描述符 */

#endif