/****************************************************/
/* File: cgen.c                                     */
/* The code generator implementation                */
/* for the TINY compiler                            */
/* (generates code for the TM machine)              */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "code.h"
#include "cgen.h"

static char buffer[1000];

#define ofpFO 0
#define retFO -1
#define initFO -2

static int globalOffset = 0;
static int localOffset = initFO;

/* numOfParams is the number of parameters in current frame */
static int numOfParams = 0;

/* isInFunc is the flag that shows if current node
   is in a function block. This flag is used when
   calculating localOffset of a function declaration.
*/
static int isInFunc = FALSE;

/* mainFuncLoc is the location of main() function */
static int mainFuncLoc = 0;

/* prototype for internal recursive code generator */
static void cGen (TreeNode * tree);

/* list的节点类型，若为变量/数组变量/参数，那么返回该节点符号的偏移量 */
static int getBlockOffset(TreeNode *list) {
  int offset = 0;

  if (list == NULL) {
    //offset = 0;
  } else if (list->nodekind == DeclK) {
    /* local variable declaration */
    TreeNode *node = list;
    while (node != NULL) {
      switch (node->kind.decl) {
      case VarK:
        ++offset;
        break;
      case ArrVarK:
        offset += node->attr.arr.size;
        break;
      default:
        break;
      }
      node = node->sibling;
    }
  } else if (list->nodekind == ParamK) {
    /* parameter declaration */
    TreeNode *node = list;
    while (node != NULL) {
      ++offset;
      node = node->sibling;
    }
  }

  return offset;
}

/* 函数体的中间代码生成 */
static void genStmt( TreeNode * tree )
{ TreeNode * p1, * p2, * p3;
  int savedLoc1,savedLoc2,currentLoc;
  int loc;
  int offset;
  /*分为stmtK类型的几种情况，递归调用，从分析树里将CompoundStatement分为更小的子句*/
  switch (tree->kind.stmt) {

      case CompK:
        if (TraceCode) emitComment("Start compound");

        p1 = tree->child[0];
        p2 = tree->child[1];

        /* update localOffset with the offset derived from declarations */
        offset = getBlockOffset(p1);
        localOffset -= offset;

        /* push scope */
        sc_push(tree->attr.scope);

        /*递归求比较表达式的中间代码*/
        cGen(p2);

        /* pop scope */
        sc_pop();

        /* restore localOffset */
        localOffset -= offset;

        if (TraceCode) emitComment("End compound");

        break;

      case IfK:
        if (TraceCode) emitComment("Start if");

        p1 = tree->child[0];
        p2 = tree->child[1];
        p3 = tree->child[2];

        /* 递归得到if后面的表达式的中间代码 */
        cGen(p1);

        /* 得到假出口的位置 */
        savedLoc1 = emitSkip(1);
        emitComment("if: jump to else belongs here");

        /* 形成then后面的表达式的中间代码 */
        cGen(p2);

        /* 得到正规表达式的出口位置 */
        savedLoc2 = emitSkip(1);
        emitComment("if: jump to end belongs here");

        /* 当前四元式的位置 */
        currentLoc = emitSkip(0);
        /* 得到需要回填的位置是假出口 */
        emitBackup(savedLoc1);
        emitRM_Abs("JEQ",ac,currentLoc,"if: jmp to else");
        emitRestore();
        /* 递归得到else后面的表达式的中间代码 */
        cGen(p3);
        currentLoc = emitSkip(0);
        emitBackup(savedLoc2);
        /* 得到需要回填的位置是整个表达式的出口 */
        emitRM_Abs("LDA",pc,currentLoc,"jmp to end");
        emitRestore();
        if (TraceCode)  emitComment("End if");

        break; /* select_k */

      case IterK:
        if (TraceCode) emitComment("Start iter.");

        p1 = tree->child[0];
        p2 = tree->child[1];

        /* while的开头的位置，用于循环回跳 */
        savedLoc1 = emitSkip(0);
        emitComment("while: jump after body comes back here");

        /* 递归生成循环的条件表达式的中间代码 */
        cGen(p1);

        /* while循环结束后的跳转的位置 */
        savedLoc2 = emitSkip(1);
        emitComment("while: jump to end belongs here");

        /* 递归生成while循环内的代码的中间代码 */
        cGen(p2);
        emitRM_Abs("LDA",pc,savedLoc1,"while: jmp back to test");
        /* backpatch */
        currentLoc = emitSkip(0);
        emitBackup(savedLoc2);
        emitRM_Abs("JEQ",ac,currentLoc,"while: jmp to end");
        emitRestore();

        if (TraceCode)  emitComment("End iter.");

        break; /* iter_k */

      case RetK:
        if (TraceCode) emitComment("-> return");

        p1 = tree->child[0];

        /* 递归生成返回里的表达式的中间代码 */
        cGen(p1);
        emitRM("LD",pc,retFO,mp,"return: to caller");

        if (TraceCode) emitComment("<- return");

        break; /* return_k */
      default:
         break;
    }
} /* genStmt */

/* Procedure genExp generates code at an expression node */
static void genExp( TreeNode * tree, int lhs )
{ int loc;
  int varOffset, baseReg;
  int numOfArgs;
  TreeNode * p1, * p2;
  switch (tree->kind.exp) {
    case AssignK:
      /* 赋值语句的中间代码的生成 */
      if (TraceCode) emitComment("-> assign");

      p1 = tree->child[0];
      p2 = tree->child[1];

      /* generate code for ac = address of lhs */
      genExp(p1, TRUE);
      /* generate code to push lhs */
      emitRM("ST", ac, localOffset--, mp, "assign: push left (address)");

      /* generate code for ac = rhs */
      cGen(p2);
      /* now load lhs */
      emitRM("LD", ac1, ++localOffset, mp, "assign: load left (address)");

      emitRM("ST", ac, 0, ac1, "assign: store value");

      if (TraceCode) emitComment("<- assign");
      break; /* assign_k */

    case OpK:
      /* 运算或者布尔表达式的中间代码的生成 */
      if (TraceCode) emitComment("-> Op");

      p1 = tree->child[0];
      p2 = tree->child[1];

      /* gen code for ac = left arg */
      cGen(p1);
      /* gen code to push left operand */
      emitRM("ST",ac,localOffset--,mp,"op: push left");

      /* gen code for ac = right operand */
      cGen(p2);
      /* now load left operand */
      emitRM("LD",ac1,++localOffset,mp,"op: load left");

      switch (tree->attr.op) {
        case PLUS:
          emitRO("ADD",ac,ac1,ac,"op +");
          break;
        case MINUS:
          emitRO("SUB",ac,ac1,ac,"op -");
          break;
        case TIMES:
          emitRO("MUL",ac,ac1,ac,"op *");
          break;
        case OVER:
          emitRO("DIV",ac,ac1,ac,"op /");
          break;
        case LT:
          emitRO("SUB",ac,ac1,ac,"op <");
          emitRM("JLT",ac,2,pc,"br if true");
          emitRM("LDC",ac,0,ac,"false case");
          emitRM("LDA",pc,1,pc,"unconditional jmp");
          emitRM("LDC",ac,1,ac,"true case");
          break;
        case LTEQ:
          emitRO("SUB",ac,ac1,ac,"op <=");
          emitRM("JLE",ac,2,pc,"br if true");
          emitRM("LDC",ac,0,ac,"false case");
          emitRM("LDA",pc,1,pc,"unconditional jmp");
          emitRM("LDC",ac,1,ac,"true case");
          break;
        case GT:
          emitRO("SUB",ac,ac1,ac,"op >");
          emitRM("JGT",ac,2,pc,"br if true");
          emitRM("LDC",ac,0,ac,"false case");
          emitRM("LDA",pc,1,pc,"unconditional jmp");
          emitRM("LDC",ac,1,ac,"true case");
          break;
        case GTEQ:
          emitRO("SUB",ac,ac1,ac,"op >=");
          emitRM("JGE",ac,2,pc,"br if true");
          emitRM("LDC",ac,0,ac,"false case");
          emitRM("LDA",pc,1,pc,"unconditional jmp");
          emitRM("LDC",ac,1,ac,"true case");
          break;
        case EQ:
          emitRO("SUB",ac,ac1,ac,"op ==");
          emitRM("JEQ",ac,2,pc,"br if true");
          emitRM("LDC",ac,0,ac,"false case");
          emitRM("LDA",pc,1,pc,"unconditional jmp");
          emitRM("LDC",ac,1,ac,"true case");
          break;
        case NEQ:
          emitRO("SUB",ac,ac1,ac,"op !=");
          emitRM("JNE",ac,2,pc,"br if true");
          emitRM("LDC",ac,0,ac,"false case");
          emitRM("LDA",pc,1,pc,"unconditional jmp");
          emitRM("LDC",ac,1,ac,"true case");
          break;
        default:
          emitComment("BUG: Unknown operator");
          break;
      } /* case op */

      if (TraceCode)  emitComment("<- Op");

      break; /* OpK */

    case ConstK:
      /* 常数的中间代码的生成 */
      if (TraceCode) emitComment("-> Const") ;
      /* gen code to load integer constant using LDC */
      emitRM("LDC",ac,tree->attr.val,0,"load const");
      if (TraceCode)  emitComment("<- Const") ;
      break; /* ConstK */

    case IdK:
    case ArrIdK:
      /*数组的中间代码的生成*/
      if (TraceCode) {
        sprintf(buffer, "-> Id (%s)", tree->attr.name);
        emitComment(buffer);
      }

      /* 得到数组的基地址 */
      loc = st_lookup_top(tree->attr.name);
      if (loc >= 0)
        varOffset = initFO - loc;
      else
        varOffset = -(st_lookup(tree->attr.name));

      /* generate code to load varOffset */
      emitRM("LDC", ac, varOffset, 0, "id: load varOffset");

      if (tree->kind.exp == ArrIdK) {
        /* kind of node is for array id */

        if (loc >= 0 && loc < numOfParams) {

          /* 生成压入基地址的中间代码 */
          emitRO("ADD", ac, mp, ac, "id: load the memory address of base address of array to ac");
          emitRO("LD", ac, 0, ac, "id: load the base address of array to ac");
        } else {
          /* 若是全局变量 */

          /* 形成地址的中间代码 */
          if (loc >= 0)
            /* 若在当前栈帧里找到 */
            emitRO("ADD", ac, mp, ac, "id: calculate the address");
          else
            /* 若在全局作用域内找到 */
            emitRO("ADD", ac, gp, ac, "id: calculate the address");
        }

        /* 形成本地偏移的中间代码 */
        emitRM("ST", ac, localOffset--, mp, "id: push base address");

        /* 递归形成索引表达式的中间代码 */
        p1 = tree->child[0];
        cGen(p1);
        /* gen code to get correct varOffset */
        emitRM("LD", ac1, ++localOffset, mp, "id: pop base address");
        emitRO("SUB", ac, ac1, ac, "id: calculate element address with index");
      } else {
        /* kind of node is for non-array id */

        /* generate code for address */
        if (loc >= 0)
          /* symbol found in current frame */
          emitRO("ADD", ac, mp, ac, "id: calculate the address");
        else
          /* symbol found in global scope */
          emitRO("ADD", ac, gp, ac, "id: calculate the address");
      }

      if (lhs) {
        emitRM("LDA", ac, 0, ac, "load id address");
      } else {
        emitRM("LD", ac, 0, ac, "load id value");
      }

      if (TraceCode)  emitComment("<- Id");

      break; /* IdK, ArrIdK */

    case CallK:
      /* 生成调用的中间代码 */
      if (TraceCode) emitComment("Start Call");

      /* init */
      numOfArgs = 0;

      p1 = tree->child[0];

      /* for each argument */
      while (p1 != NULL) {
        /* 递归生成参数表达式的中间代码 */
        if (p1->type == IntegerArray)
          genExp(p1, TRUE);
        else
          genExp(p1, FALSE);

        /* 生成参数压栈的中间代码 */
        emitRM("ST", ac, localOffset + initFO - (numOfArgs++), mp,
            "call: push argument");

        p1 = p1->sibling;
      }

      if (strcmp(tree->attr.name, "input") == 0) {
        /* generate code for input() function */
        emitRO("IN",ac,0,0,"read integer value");
      } else if (strcmp(tree->attr.name, "output") == 0) {
        /* generate code for output(arg) function */
        /* generate code for value to write */
        emitRM("LD", ac, localOffset + initFO, mp, "load arg to ac");
        /* now output it */
        emitRO("OUT",ac,0,0,"write ac");
      } else {
        /* generate code to store current mp */
        emitRM("ST", mp, localOffset + ofpFO, mp, "call: store current mp");
        /* generate code to push new frame */
        emitRM("LDA", mp, localOffset, mp, "call: push new frame");
        /* generate code to save return in ac */
        emitRM("LDA", ac, 1, pc, "call: save return in ac");

        /* generate code to relative-jump to function entry */
        loc = -(st_lookup(tree->attr.name));
        emitRM("LD", pc, loc, gp, "call: relative jump to function entry");

        /* generate code to pop current frame */
        emitRM("LD", mp, ofpFO, mp, "call: pop current frame");
      }

      if (TraceCode)  emitComment("End Call");

      break; /* CallK */

    default:
      break;
  }
} /* genExp */

/* Procedure genDecl generates code at a declaration node */
static void genDecl( TreeNode * tree)
{ TreeNode * p1, * p2;
  int loadFuncLoc,jmpLoc,funcBodyLoc,nextDeclLoc;
  int loc;
  int size;

  switch (tree->kind.decl) {
  case FuncK:
    if (TraceCode) {
      sprintf(buffer, "Start Function (%s)", tree->attr.name);
      emitComment(buffer);
    }

    p1 = tree->child[1];
    p2 = tree->child[2];

    isInFunc = TRUE;

    /* generate code to store the location of func. entry */
    loc = -(st_lookup(tree->attr.name));
    loadFuncLoc = emitSkip(1);
    emitRM("ST", ac1, loc, gp, "func: store the location of func. entry");
    /* decrease global offset by 1 */
    --globalOffset;

    /* generate code to unconditionally jump to next declaration */
    jmpLoc = emitSkip(1);
    emitComment(
        "func: unconditional jump to next declaration belongs here");

    /* skip code generation to allow jump to here
       when the function was called */
    funcBodyLoc = emitSkip(0);
    emitComment("func: function body starts here");

    /* backpatch */
    emitBackup(loadFuncLoc);
    emitRM("LDC", ac1, funcBodyLoc, 0, "func: load function location");
    emitRestore();

    /* generate code to store return address */
    emitRM("ST", ac, retFO, mp, "func: store return address");

    /* calculate localOffset and numOfParams */
    localOffset = initFO;
    numOfParams = 0;
    cGen(p1);

    /* generate code for function body */
    if (strcmp(tree->attr.name, "main") == 0)
      mainFuncLoc = funcBodyLoc;

    cGen(p2);

    /* generate code to load pc with return address */
    emitRM("LD", pc, retFO, mp, "func: load pc with return address");

    /* backpatch */
    nextDeclLoc = emitSkip(0);
    emitBackup(jmpLoc);
    emitRM_Abs("LDA", pc, nextDeclLoc,
        "func: unconditional jump to next declaration");
    emitRestore();

    isInFunc = FALSE;

    if (TraceCode) {
      sprintf(buffer, "End Function (%s)", tree->attr.name);
      emitComment(buffer);
    }

    break;

  case VarK:
  case ArrVarK:
    if (TraceCode) emitComment("-> var. decl.");

    if (tree->kind.decl == ArrVarK)
      size = tree->attr.arr.size;
    else
      size = 1;

    if (isInFunc == TRUE)
      localOffset -= size;
    else
      globalOffset -= size;

    if (TraceCode) emitComment("<- var. decl.");
    break;

  default:
     break;
  }
} /* genDecl */

/* Procedure genParam generates code at a declaration node */
static void genParam( TreeNode * tree)
{ switch (tree->kind.stmt) {

  case ArrParamK:
  case NonArrParamK:
    if (TraceCode) emitComment("-> param");
    emitComment(tree->attr.name);

    --localOffset;
    ++numOfParams;

    if (TraceCode) emitComment("<- param");

    break;

  default:
     break;
  }
} /* genParam */

/* Procedure cGen recursively generates code by
 * tree traversal
 */
static void cGen( TreeNode * tree )
{ if (tree != NULL)
  { switch (tree->nodekind) {
      case StmtK:
        genStmt(tree);
        break;
      case ExpK:
        genExp(tree, FALSE);
        break;
      case DeclK:
        genDecl(tree);
        break;
      case ParamK:
        genParam(tree);
        break;
      default:
        break;
    }
    cGen(tree->sibling);
  }
}

void genMainCall() {
  emitRM("LDC", ac, globalOffset, 0, "init: load globalOffset");
  emitRO("ADD", mp, mp, ac, "init: initialize mp with globalOffset");

  if (TraceCode) emitComment("Start Call");

  /* generate code to store current mp */
  emitRM("ST", mp, ofpFO, mp, "call: store current mp");
  /* generate code to push new frame */
  emitRM("LDA", mp, 0, mp, "call: push new frame");
  /* generate code to save return in ac */
  emitRM("LDA", ac, 1, pc, "call: save return in ac");

  /* generate code for unconditional jump to function entry */
  emitRM("LDC", pc, mainFuncLoc, 0, "call: unconditional jump to main() entry");

  /* generate code to pop current frame */
  emitRM("LD", mp, ofpFO, mp, "call: pop current frame");

  if (TraceCode) emitComment("End Call");
}

/**********************************************/
/* the primary function of the code generator */
/**********************************************/
/* Procedure codeGen generates code to a code
 * file by traversal of the syntax tree. The
 * second parameter (codefile) is the file name
 * of the code file, and is used to print the
 * file name as a comment in the code file
 */
void codeGen(TreeNode * syntaxTree, char * codefile)
{  char * s = malloc(strlen(codefile)+7);
   strcpy(s,"File: ");
   strcat(s,codefile);
   emitComment("TINY Compilation to TM Code");
   emitComment(s);
   /* generate standard prelude */
   emitComment("Standard prelude:");
   emitRM("LD",gp,0,ac,"load gp with maxaddress");
   emitRM("LDA",mp,0,gp,"copy gp to mp");
   emitRM("ST",ac,0,ac,"clear location 0");
   emitComment("End of standard prelude.");
   /* push global scope */
   sc_push(globalScope);
   /* generate code for TINY program */
   cGen(syntaxTree);
   /* pop global scope */
   sc_pop();
   /* call main() function */
   genMainCall();
   /* finish */
   emitComment("End of execution.");
   emitRO("HALT",0,0,0,"");
}
