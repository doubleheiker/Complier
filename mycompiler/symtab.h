/****************************************************/
/* File: symtab.h                                   */
/* Symbol table interface for the TINY compiler     */
/* (allows only one symbol table)                   */
/****************************************************/

#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#include "globals.h"

/* SIZE is the size of the hash table */
#define SIZE 211

/* 源代码中引用变量的行号 */
typedef struct LineListRec
   { int lineno;
     struct LineListRec * next;
   } * LineList;

/* bucket中列出了
 * 每个变量，包括名称，
 * 指定的内存位置，以及
 * 行号列表，其中
 * 它出现在源代码中
 */
typedef struct BucketListRec
   { char * name;
     LineList lines;
     TreeNode *treeNode;
     int memloc ; /* memory location for variable */
     struct BucketListRec * next;
   } * BucketList;


/* 记录当前函数所在作用域的层级和它对应的符号表
 * 以及一个指针指向它的直接外层
 */
typedef struct ScopeRec
   { char * funcName;
     int nestedLevel;
     struct ScopeRec * parent;
     BucketList hashTable[SIZE]; /* the hash table */
   } * Scope;


Scope globalScope;  //全局作用域

/* st_insert函数：插入查找符号的函数及其地址到符号表中
 * 其中注意每个符号仅会将地址插入符号表一次，其他不再做插入
 */
void st_insert( char * name, int lineno, int loc, TreeNode * treeNode );

/* 函数st_lookup返回变量的内存位置，如果找不到则返回-1 */
int st_lookup ( char * name );
/* 增加新的引用变量的所在行链接到定义时该符号的lines上 */
int st_add_lineno(char * name, int lineno);
/* 从作用域栈的栈顶（最内层），依次向外层扫描是否有该符号 */
BucketList st_bucket( char * name );
/* 函数st_lookup查找符号是否在栈顶作用域内，如果找到则返回地址，反之返回-1 */
int st_lookup_top (char * name);

/* 建立该函数的作用域结构体 */
Scope sc_create(char *funcName);
Scope sc_top( void );
void sc_pop( void );
void sc_push( Scope scope );
int addLocation( void );



void printSymTab(FILE * listing);  //打印符号表到listing file

#endif
