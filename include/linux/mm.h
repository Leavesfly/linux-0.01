#ifndef _MM_H
#define _MM_H

/* 页面大小 */
#define PAGE_SIZE 4096

/* 函数声明 */
extern unsigned long get_free_page(void);		/* 获取空闲页面 */
extern unsigned long put_page(unsigned long page,unsigned long address); /* 放置页面 */
extern void free_page(unsigned long addr);		/* 释放页面 */

#endif