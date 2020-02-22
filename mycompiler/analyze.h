/****************************************************/
/* File: analyze.h                                  */
/* Semantic analyzer interface for TINY compiler    */
/****************************************************/

#ifndef _ANALYZE_H_
#define _ANALYZE_H_

/* buildSymtab函数：前序遍历语法树生成符号表 */
void buildSymtab(TreeNode *);

/* typeCheck函数：后序遍历语法树生成类型检查程序 */
void typeCheck(TreeNode *);

#endif
