/****************************************************/
/* File: code.h                                     */
/* Code emitting utilities for the TINY compiler    */
/* and interface to the TM machine                  */
/****************************************************/

#ifndef _CODE_H_
#define _CODE_H_

/* pc = program counter  */
#define  pc 7

/* mp = "内存指针" 点
 * 内存最顶端 (用于临时存储)
 */
#define  mp 6

/* gp = "全局指针" 点
 * 在内存的最下面(全局)
 * 存储变量
 */
#define gp 5

/* 累加器 */
#define  ac 0

/* 第二个累加器 */
#define  ac1 1

/* 用于产生一个注释性语句 */
void emitComment( char * c );

/* 用于“寄存器间”和“寄存器到地址”汇编代码的中间代码的emit */
void emit( char * op, int r, int d, int s, char *c);

/* 跳过“几个”中间代码，作为之后要回填的地址
 * 也可以返回当前的代码的位置
 */
int emitSkip( int howMany);

/* 将需要回填的地址赋给loc
 */
void emitBackup( int loc);

/* Procedure emitRestore restores the current 
 * code position to the highest previously
 * unemitted position
 */
void emitRestore(void);

/* Procedure emitRM_Abs 传递一个绝对地址引用 
 * 到相对引用处
 * op = 指令
 * r = 目标寄存器
 * a = 绝对地址
 * c = a comment to be printed if TraceCode is TRUE
 */
void emitRM_Abs( char *op, int r, int a, char * c);

#endif
