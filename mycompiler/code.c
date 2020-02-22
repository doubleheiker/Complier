/****************************************************/
/* File: code.c                                     */
/* TM Code emitting utilities                       */
/* implementation for the TINY compiler             */
/****************************************************/

#include "globals.h"
#include "code.h"

/* 用于记录四元式的位置号 */
static int emitLoc = 0 ;

/* 记录最高的四元式记录号，用于回填，跳过，恢复 */
static int highEmitLoc = 0;

/* Procedure emitComment 输出四元式的注释
 * 用注释 c 写到中间代码的文件中
 */
void emitComment( char * c )
{ if (TraceCode) fprintf(code,"* %s\n",c);}

/* 用于“寄存器间”和“寄存器到地址”汇编代码的中间代码的emit */
void emit( char * op, int r, int d, int s, char *c)
{ fprintf(code,"%3d:  %5s  %d,%d(%d) ",emitLoc++,op,r,d,s);
  if (TraceCode) fprintf(code,"\t%s",c) ;
  fprintf(code,"\n") ;
  if (highEmitLoc < emitLoc)  highEmitLoc = emitLoc ;
} 

/* 跳过“几个”中间代码，作为之后要回填的地址
 * 也可以返回当前的代码的位置
 */
int emitSkip( int howMany)
{  int i = emitLoc;
   emitLoc += howMany ;
   if (highEmitLoc < emitLoc)  highEmitLoc = emitLoc ;
   return i;
} /* emitSkip */

/* 将需要回填的地址赋给loc
 */
void emitBackup( int loc)
{ if (loc > highEmitLoc) emitComment("BUG in emitBackup");
  emitLoc = loc ;
} /* emitBackup */

/* Procedure emitRestore restores the current 
 * code position to the highest previously
 * unemitted position
 */
void emitRestore(void)
{ emitLoc = highEmitLoc;}

/* Procedure emitRM_Abs 传递一个绝对地址引用 
 * 到相对引用处
 * op = 指令
 * r = 目标寄存器
 * a = 绝对地址
 * c = a comment to be printed if TraceCode is TRUE
 */
void emitRM_Abs( char *op, int r, int a, char * c)
{ fprintf(code,"%3d:  <%4s  %d,%d(%d) >",
               emitLoc,op,r,a-(emitLoc+1),pc);
  ++emitLoc ;
  if (TraceCode) fprintf(code,"\t%s",c) ;
  fprintf(code,"\n") ;
  if (highEmitLoc < emitLoc) highEmitLoc = emitLoc ;
} /* emitRM_Abs */
