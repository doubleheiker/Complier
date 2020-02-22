/****************************************************/
/* File: globals.h                                  */
/* Yacc/Bison Version                               */
/* Global types and vars for TINY compiler          */
/* must come before other include files             */
/****************************************************/

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/* Yacc/Bison generates internally its own values
 * for the tokens. Other files can access these values
 * by including the tab.h file generated using the
 * Yacc/Bison option -d ("generate header")
 *
 * The YYPARSER flag prevents inclusion of the tab.h
 * into the Yacc/Bison output itself
 */

#ifndef YYPARSER

/* the name of the following file may change */
#include "build/y.tab.h"

/* ENDFILE is implicitly defined by Yacc/Bison,
 * and not included in the tab.h file
 */
#define ENDFILE 0

#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

/* MAXRESERVED = the number of reserved words */
#define MAXRESERVED 6

/* Yacc/Bison generates its own integer values
 * for tokens
 */
typedef int TokenType;

extern FILE* source; /* source code text file */
extern FILE* listing; /* listing output text file */
extern FILE* code; /* code text file for TM simulator */

extern int lineno; /* source line number for listing */

/**************************************************/
/***********   Syntax tree for parsing ************/
/**************************************************/

typedef enum {StmtK,ExpK,DeclK,ParamK,TypeK} NodeKind; //{}内的语句、表达式、声明、参数、token的类型
typedef enum {CompK,IfK,IterK,RetK} StmtKind; //比较、判断、迭代、返回的类型
typedef enum {AssignK,OpK,ConstK,IdK,ArrIdK,CallK} ExpKind; //赋值、运算符、常数、符号、数组、调用的类型
typedef enum {FuncK,VarK,ArrVarK} DeclKind; //函数、变量、数组变量的类型
typedef enum {ArrParamK,NonArrParamK} ParamKind; //数组参数、非数组参数类型
typedef enum {TypeNameK} TypeKind;

/* ArrayAttr 表示数组的属性：类型、名字、大小 */
typedef struct arrayAttr {
    TokenType type;
    char * name;
    int size;
} ArrayAttr;

/* ExpType 在语法符号类型检验的时候使用，检验void、int、布尔表达式和整型数组 */
typedef enum {Void,Integer,Boolean, IntegerArray} ExpType;

#define MAXCHILDREN 3

struct ScopeRec;

/* 树节点结构：
 * 一个节点含有最多三个指向孩子节点的指针，用于指向产生式右边的非终结符节点
 * 还有一个指向兄弟节点的指针
 * 当前节点所在行数
 * 节点类型
 * 节点的具体是哪一种类型
 * 节点的属性
 * 可供检查的类型
 */
typedef struct treeNode
   { struct treeNode * child[MAXCHILDREN];
     struct treeNode * sibling;
     int lineno;
     NodeKind nodekind;
     union { StmtKind stmt;
             ExpKind exp;
             DeclKind decl;
             ParamKind param;
             TypeKind type; 
             } kind;
     union { TokenType op;
             TokenType type;
             int val;
             char * name;
             ArrayAttr arr;
             struct ScopeRec * scope; //记录当前节点的作用域
             } attr;
     ExpType type; /* for type checking of exps */
   } TreeNode;

/**************************************************/
/***********   Flags for tracing       ************/
/**************************************************/

/* EchoSource = TRUE 会使得源程序在分析过程中根据行数对listing file 进行重现 */
extern int EchoSource;

/* TraceScan = TRUE causes token information to be
 * printed to the listing file as each token is
 * recognized by the scanner
 */
extern int TraceScan;

/* TraceParse = TRUE causes the syntax tree to be
 * printed to the listing file in linearized form
 * (using indents for children)
 */
extern int TraceParse;

/* TraceAnalyze = TRUE causes symbol table inserts
 * and lookups to be reported to the listing file
 */
extern int TraceAnalyze;

/* TraceCode = TRUE causes comments to be written
 * to the TM code file as code is generated
 */
extern int TraceCode;

/* Error = TRUE prevents further passes if an error occurs */
extern int Error;
#endif
