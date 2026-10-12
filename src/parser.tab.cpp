/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 6 "parser.y"

#include <cstdio>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>

extern nlohmann::ordered_json g_output;
extern int yylex();
extern int yylineno;
void yyerror(const char *msg);

int g_semanticErrorCount = 0; 
static void reportSemanticError(const std::string &msg) {
    std::fprintf(stderr, "Semantic error at line %d: %s\n", yylineno, msg.c_str());
    g_semanticErrorCount++;
}

static std::string typeJsonToName(const nlohmann::ordered_json &t) {
    std::string kind = t.value("type", "");
    if (kind == "builtin-type") return t.value("type-value", "unknown");
    if (kind == "pointer-type" && t.contains("pointer-to")) {
        std::string inner = typeJsonToName(t["pointer-to"]);
        return inner == "unknown" ? "unknown" : inner + "*";
    }
    return "unknown";
}

class SymbolTable {
public:
    SymbolTable() { pushScope(); } 

    void pushScope() { scopes.push_back({}); }
    void popScope() { if (!scopes.empty()) scopes.pop_back(); }
    void declare(const std::string &name, const nlohmann::ordered_json &type, bool isArray = false) {
        if (scopes.empty()) return;
        scopes.back()[name] = type;
        scopes.back()[name]["__array"] = isArray; 
    }
    bool isDeclared(const std::string &name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
            if (it->find(name) != it->end()) return true;
        return false;
    }
    bool isArray(const std::string &name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) return found->second.value("__array", false);
        }
        return false;
    }
    std::string lookupTypeValue(const std::string &name) {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return typeJsonToName(found->second);
            }
        }
        return "unknown";
    }
private:
    std::vector<std::unordered_map<std::string, nlohmann::ordered_json>> scopes;
};

static SymbolTable g_symbolTable;
static std::unordered_map<std::string, nlohmann::ordered_json> g_functionReturnTypes;

static int g_structDepth = 0;

static std::string inferSimpleType(nlohmann::ordered_json *e) {
    std::string t = e->value("type", "");
    if (t == "literal-constant") {
        std::string ct = e->value("constant-type", "");
        if (ct == "integer") return "int";
        if (ct == "real") return "double";
        if (ct == "string") return "string";
        return "unknown";
    }
    if (t == "variable-reference") {
        return g_symbolTable.lookupTypeValue(e->value("variable-name", ""));
    }
    if (e->contains("result-type")) {
        const nlohmann::ordered_json &rt = (*e)["result-type"];
        if (rt.is_string()) return rt.get<std::string>();
        if (rt.is_object()) return typeJsonToName(rt);
    }
    if (e->contains("assignment-type") && (*e)["assignment-type"].is_string())
        return (*e)["assignment-type"].get<std::string>();
    return "unknown";
}

static bool isIntegerTypeName(const std::string &t) {
    return t == "int" || t == "long" || t == "short" || t == "char" || t == "bool" || t == "boolean" ||
        t == "unsigned int" || t == "unsigned long" || t == "unsigned short" || t == "unsigned char";
}

static bool isRealTypeName(const std::string &t) {
    return t == "double" || t == "float";
}

static nlohmann::ordered_json nameToTypeJson(const std::string &n) {
    if (n.empty() || n == "unknown") return "unknown";
    if (n.back() == '*')
        return {{"type","pointer-type"},{"pointer-to", nameToTypeJson(n.substr(0, n.size()-1))}};
    return {{"type","builtin-type"},{"type-value", n}};
}

static std::string pointeeName(const std::string &n) {
    if (!n.empty() && n.back() == '*') return n.substr(0, n.size()-1);
    return "unknown";
}


static std::string inferOperandType(nlohmann::ordered_json *e) {
    std::string t = inferSimpleType(e);
    if (t != "unknown" && e->value("type","") == "variable-reference" &&
        g_symbolTable.isArray(e->value("variable-name","")))
        return t + "*";
    return t;
}

static bool isPointerLikeTypeName(const std::string &t) {
    return t == "string" || (!t.empty() && t.back() == '*');
}

static std::string promoteArithmeticType(const std::string &leftType, const std::string &rightType, char op) {
    if (leftType == "unknown" || rightType == "unknown") return "unknown"; 
    bool lp = isPointerLikeTypeName(leftType), rp = isPointerLikeTypeName(rightType);
    if (lp || rp) {
        
        if (op == '+' && lp && !rp && isIntegerTypeName(rightType)) return leftType;
        if (op == '+' && rp && !lp && isIntegerTypeName(leftType)) return rightType;
        if (op == '-' && lp && !rp && isIntegerTypeName(rightType)) return leftType;
        if (op == '-' && lp && rp) return "int"; 
        reportSemanticError(std::string("invalid operands to binary operator '") + op + "': '" + leftType + "' and '" + rightType + "'");
        return "unknown";
    }
    if (leftType == "double" || rightType == "double") return "double";
    if (leftType == "float" || rightType == "float") return "float";
    if (isIntegerTypeName(leftType) && isIntegerTypeName(rightType)) return "int";
    reportSemanticError("operands of type '" + leftType + "' and '" + rightType + "' are not arithmetic");
    return "unknown";
}

static std::string unifyBranchTypes(const std::string &a, const std::string &b, nlohmann::ordered_json *ea, nlohmann::ordered_json *eb) {
    if (a == "unknown" || b == "unknown") return "unknown";
    if (a == b) return a;
    bool ap = isPointerLikeTypeName(a), bp = isPointerLikeTypeName(b);
    if (!ap && !bp) return promoteArithmeticType(a, b, '+');
    auto isZero = [](nlohmann::ordered_json *e) {
        return e->value("type","") == "literal-constant" && e->value("constant-value", -1) == 0;
    };
    if (ap && isZero(eb)) return a;
    if (bp && isZero(ea)) return b;
    reportSemanticError("incompatible operand types in conditional expression: '" + a + "' and '" + b + "'");
    return "unknown";
}

static std::string computeModuloType(const std::string &leftType, const std::string &rightType) {
    if (isRealTypeName(leftType) || isRealTypeName(rightType)) {
        reportSemanticError("operator% requires integral operands, got '" +
                             leftType + "' and '" + rightType + "'");
        return "unknown";
    }
    if (leftType == "unknown" || rightType == "unknown") return "unknown";
    if (isIntegerTypeName(leftType) && isIntegerTypeName(rightType)) return "int";
    reportSemanticError("operator% requires integral operands, got '" +
                         leftType + "' and '" + rightType + "'");
    return "unknown";
}

static bool isUnsignedTypeName(const std::string &t) {
    return t.rfind("unsigned", 0) == 0;
}

static void markLvalueIfVariableRef(nlohmann::ordered_json &node) {
    if (node.value("type", "") == "variable-reference") {
        node["variable-usage"] = "lvalue";
    }
}

static void stampDeclaratorType(nlohmann::ordered_json &d, const nlohmann::ordered_json &type) {
    bool isArrayDecl = d.contains("__index_ranges");
    if (isArrayDecl) {
        nlohmann::ordered_json arrayType = {
            {"type", "array-type"},
            {"name", d.value("variable-name", "")},
            {"element-type", type},
            {"index-ranges", d["__index_ranges"]}
        };
        d.erase("__index_ranges");
        d["variable-type"] = arrayType;
    } else {
        d["variable-type"] = type;
    }
    g_symbolTable.declare(d.value("variable-name", ""), type, isArrayDecl);
}

static nlohmann::ordered_json *buildVarDecl(bool isGlobal, bool isStatic, bool isConst,
                                     nlohmann::ordered_json *type,
                                     nlohmann::ordered_json *declarators) {
    for (auto &d : *declarators) {
        stampDeclaratorType(d, *type);
    }
    nlohmann::ordered_json *result = new nlohmann::ordered_json({
        {"type", "variables-declaration"},
        {"global", isGlobal},
        {"static", isStatic},
        {"const", isConst},
        {"declarators", *declarators}
    });
    delete type;
    delete declarators;
    return result;
}

static void markLastReturn(nlohmann::ordered_json *statements) {
    for (auto &s : *statements) {
        if (s.value("type", "") == "return-statement") {
            s["the-last-statement"] = false;
        }
    }
    if (!statements->empty()) {
        auto &last = statements->back();
        if (last.value("type", "") == "return-statement") {
            last["the-last-statement"] = true;
        }
    }
}

static void splitMembers(nlohmann::ordered_json *body, nlohmann::ordered_json &fields,
                          nlohmann::ordered_json &methods, nlohmann::ordered_json &friends) {
    for (auto &m : *body) {
        std::string t = m.value("type", "");
        if (t == "subroutine-declaration") {
            methods.push_back(m);
        } else if (t == "friend-declaration") {
            friends.push_back(m.value("friend-name", ""));
        } else {
            fields.push_back(m);
        }
    }
}

static bool computePassControl(const nlohmann::ordered_json &variants) {
    for (const auto &variant : variants) {
        const nlohmann::ordered_json &body = variant.at("variant-body");
        const nlohmann::ordered_json &stmts = body.at("statements");
        bool endsInBreak = !stmts.empty() &&
            stmts.back().value("type", "") == "break-statement";
        if (!endsInBreak) return true;
    }
    return false;
}

#line 326 "parser.tab.cpp"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "parser.tab.hpp"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_TOK_INTEGER_LITERAL = 3,        /* TOK_INTEGER_LITERAL  */
  YYSYMBOL_TOK_REAL_LITERAL = 4,           /* TOK_REAL_LITERAL  */
  YYSYMBOL_TOK_STRING_LITERAL = 5,         /* TOK_STRING_LITERAL  */
  YYSYMBOL_TOK_IDENTIFIER = 6,             /* TOK_IDENTIFIER  */
  YYSYMBOL_TOK_BREAK = 7,                  /* TOK_BREAK  */
  YYSYMBOL_TOK_CASE = 8,                   /* TOK_CASE  */
  YYSYMBOL_TOK_CLASS = 9,                  /* TOK_CLASS  */
  YYSYMBOL_TOK_CONST = 10,                 /* TOK_CONST  */
  YYSYMBOL_TOK_CONTINUE = 11,              /* TOK_CONTINUE  */
  YYSYMBOL_TOK_DEFAULT = 12,               /* TOK_DEFAULT  */
  YYSYMBOL_TOK_DELETE = 13,                /* TOK_DELETE  */
  YYSYMBOL_TOK_DO = 14,                    /* TOK_DO  */
  YYSYMBOL_TOK_ELSE = 15,                  /* TOK_ELSE  */
  YYSYMBOL_TOK_ENUM = 16,                  /* TOK_ENUM  */
  YYSYMBOL_TOK_FOR = 17,                   /* TOK_FOR  */
  YYSYMBOL_TOK_FRIEND = 18,                /* TOK_FRIEND  */
  YYSYMBOL_TOK_GOTO = 19,                  /* TOK_GOTO  */
  YYSYMBOL_TOK_IF = 20,                    /* TOK_IF  */
  YYSYMBOL_TOK_NEW = 21,                   /* TOK_NEW  */
  YYSYMBOL_TOK_RETURN = 22,                /* TOK_RETURN  */
  YYSYMBOL_TOK_STATIC = 23,                /* TOK_STATIC  */
  YYSYMBOL_TOK_STATIC_CAST = 24,           /* TOK_STATIC_CAST  */
  YYSYMBOL_TOK_STRUCT = 25,                /* TOK_STRUCT  */
  YYSYMBOL_TOK_SWITCH = 26,                /* TOK_SWITCH  */
  YYSYMBOL_TOK_TYPEDEF = 27,               /* TOK_TYPEDEF  */
  YYSYMBOL_TOK_UNION = 28,                 /* TOK_UNION  */
  YYSYMBOL_TOK_WHILE = 29,                 /* TOK_WHILE  */
  YYSYMBOL_TOK_KW_INT = 30,                /* TOK_KW_INT  */
  YYSYMBOL_TOK_KW_DOUBLE = 31,             /* TOK_KW_DOUBLE  */
  YYSYMBOL_TOK_KW_FLOAT = 32,              /* TOK_KW_FLOAT  */
  YYSYMBOL_TOK_KW_CHAR = 33,               /* TOK_KW_CHAR  */
  YYSYMBOL_TOK_KW_BOOL = 34,               /* TOK_KW_BOOL  */
  YYSYMBOL_TOK_KW_VOID = 35,               /* TOK_KW_VOID  */
  YYSYMBOL_TOK_KW_LONG = 36,               /* TOK_KW_LONG  */
  YYSYMBOL_TOK_KW_SHORT = 37,              /* TOK_KW_SHORT  */
  YYSYMBOL_TOK_UNSIGNED = 38,              /* TOK_UNSIGNED  */
  YYSYMBOL_TOK_LPAREN = 39,                /* TOK_LPAREN  */
  YYSYMBOL_TOK_RPAREN = 40,                /* TOK_RPAREN  */
  YYSYMBOL_TOK_LBRACE = 41,                /* TOK_LBRACE  */
  YYSYMBOL_TOK_RBRACE = 42,                /* TOK_RBRACE  */
  YYSYMBOL_TOK_LBRACKET = 43,              /* TOK_LBRACKET  */
  YYSYMBOL_TOK_RBRACKET = 44,              /* TOK_RBRACKET  */
  YYSYMBOL_TOK_SEMICOLON = 45,             /* TOK_SEMICOLON  */
  YYSYMBOL_TOK_COLON = 46,                 /* TOK_COLON  */
  YYSYMBOL_TOK_COMMA = 47,                 /* TOK_COMMA  */
  YYSYMBOL_TOK_QUESTION = 48,              /* TOK_QUESTION  */
  YYSYMBOL_TOK_ASSIGN = 49,                /* TOK_ASSIGN  */
  YYSYMBOL_TOK_EQ = 50,                    /* TOK_EQ  */
  YYSYMBOL_TOK_PLUS = 51,                  /* TOK_PLUS  */
  YYSYMBOL_TOK_PLUS_ASSIGN = 52,           /* TOK_PLUS_ASSIGN  */
  YYSYMBOL_TOK_INCREMENT = 53,             /* TOK_INCREMENT  */
  YYSYMBOL_TOK_MINUS = 54,                 /* TOK_MINUS  */
  YYSYMBOL_TOK_MINUS_ASSIGN = 55,          /* TOK_MINUS_ASSIGN  */
  YYSYMBOL_TOK_DECREMENT = 56,             /* TOK_DECREMENT  */
  YYSYMBOL_TOK_STAR = 57,                  /* TOK_STAR  */
  YYSYMBOL_TOK_STAR_ASSIGN = 58,           /* TOK_STAR_ASSIGN  */
  YYSYMBOL_TOK_SLASH = 59,                 /* TOK_SLASH  */
  YYSYMBOL_TOK_SLASH_ASSIGN = 60,          /* TOK_SLASH_ASSIGN  */
  YYSYMBOL_TOK_PERCENT = 61,               /* TOK_PERCENT  */
  YYSYMBOL_TOK_PERCENT_ASSIGN = 62,        /* TOK_PERCENT_ASSIGN  */
  YYSYMBOL_TOK_AMP = 63,                   /* TOK_AMP  */
  YYSYMBOL_TOK_AMP_ASSIGN = 64,            /* TOK_AMP_ASSIGN  */
  YYSYMBOL_TOK_LOGICAL_AND = 65,           /* TOK_LOGICAL_AND  */
  YYSYMBOL_TOK_PIPE = 66,                  /* TOK_PIPE  */
  YYSYMBOL_TOK_PIPE_ASSIGN = 67,           /* TOK_PIPE_ASSIGN  */
  YYSYMBOL_TOK_LOGICAL_OR = 68,            /* TOK_LOGICAL_OR  */
  YYSYMBOL_TOK_CARET = 69,                 /* TOK_CARET  */
  YYSYMBOL_TOK_CARET_ASSIGN = 70,          /* TOK_CARET_ASSIGN  */
  YYSYMBOL_TOK_TILDE = 71,                 /* TOK_TILDE  */
  YYSYMBOL_TOK_LOGICAL_NOT = 72,           /* TOK_LOGICAL_NOT  */
  YYSYMBOL_TOK_NE = 73,                    /* TOK_NE  */
  YYSYMBOL_TOK_LT = 74,                    /* TOK_LT  */
  YYSYMBOL_TOK_LE = 75,                    /* TOK_LE  */
  YYSYMBOL_TOK_SHL = 76,                   /* TOK_SHL  */
  YYSYMBOL_TOK_SHL_ASSIGN = 77,            /* TOK_SHL_ASSIGN  */
  YYSYMBOL_TOK_GT = 78,                    /* TOK_GT  */
  YYSYMBOL_TOK_GE = 79,                    /* TOK_GE  */
  YYSYMBOL_TOK_SHR = 80,                   /* TOK_SHR  */
  YYSYMBOL_TOK_SHR_ASSIGN = 81,            /* TOK_SHR_ASSIGN  */
  YYSYMBOL_UNARY_PREC = 82,                /* UNARY_PREC  */
  YYSYMBOL_YYACCEPT = 83,                  /* $accept  */
  YYSYMBOL_program = 84,                   /* program  */
  YYSYMBOL_F_STATEMENTS = 85,              /* F_STATEMENTS  */
  YYSYMBOL_F_STATEMENT = 86,               /* F_STATEMENT  */
  YYSYMBOL_87_1 = 87,                      /* $@1  */
  YYSYMBOL_88_2 = 88,                      /* $@2  */
  YYSYMBOL_89_3 = 89,                      /* $@3  */
  YYSYMBOL_90_4 = 90,                      /* $@4  */
  YYSYMBOL_STATEMENTS = 91,                /* STATEMENTS  */
  YYSYMBOL_STATEMENT = 92,                 /* STATEMENT  */
  YYSYMBOL_93_5 = 93,                      /* $@5  */
  YYSYMBOL_94_6 = 94,                      /* $@6  */
  YYSYMBOL_95_7 = 95,                      /* $@7  */
  YYSYMBOL_96_8 = 96,                      /* $@8  */
  YYSYMBOL_97_9 = 97,                      /* $@9  */
  YYSYMBOL_CHOOSE_VARIANTS = 98,           /* CHOOSE_VARIANTS  */
  YYSYMBOL_CHOOSE_VARIANT = 99,            /* CHOOSE_VARIANT  */
  YYSYMBOL_TYPE_SPEC = 100,                /* TYPE_SPEC  */
  YYSYMBOL_BASE_LIST_OPT = 101,            /* BASE_LIST_OPT  */
  YYSYMBOL_BASE_LIST = 102,                /* BASE_LIST  */
  YYSYMBOL_ENUM_VALUE_LIST = 103,          /* ENUM_VALUE_LIST  */
  YYSYMBOL_ENUM_VALUE = 104,               /* ENUM_VALUE  */
  YYSYMBOL_PARAMS = 105,                   /* PARAMS  */
  YYSYMBOL_PARAM_LIST = 106,               /* PARAM_LIST  */
  YYSYMBOL_PARAM = 107,                    /* PARAM  */
  YYSYMBOL_DECLARATOR_LIST = 108,          /* DECLARATOR_LIST  */
  YYSYMBOL_DECLARATOR = 109,               /* DECLARATOR  */
  YYSYMBOL_STATIC_OPT = 110,               /* STATIC_OPT  */
  YYSYMBOL_CONST_OPT = 111,                /* CONST_OPT  */
  YYSYMBOL_FOR_INIT = 112,                 /* FOR_INIT  */
  YYSYMBOL_expr = 113,                     /* expr  */
  YYSYMBOL_assign_expr = 114,              /* assign_expr  */
  YYSYMBOL_INDEX_SUFFIX_LIST = 115,        /* INDEX_SUFFIX_LIST  */
  YYSYMBOL_ARG_LIST_OPT = 116,             /* ARG_LIST_OPT  */
  YYSYMBOL_ARG_LIST = 117                  /* ARG_LIST  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1127

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  83
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  35
/* YYNRULES -- Number of rules.  */
#define YYNRULES  149
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  335

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   337


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   336,   336,   344,   347,   356,   355,   381,   381,   402,
     402,   423,   423,   443,   447,   451,   455,   463,   471,   477,
     478,   486,   493,   493,   501,   510,   519,   529,   541,   553,
     553,   570,   578,   585,   588,   591,   598,   607,   611,   619,
     619,   640,   640,   661,   661,   681,   693,   694,   703,   716,
     733,   734,   735,   736,   737,   738,   739,   740,   742,   743,
     744,   745,   746,   748,   759,   760,   764,   769,   777,   782,
     790,   795,   804,   805,   809,   814,   822,   830,   835,   843,
     853,   863,   874,   888,   889,   893,   894,   898,   905,   920,
     921,   933,   940,   947,   955,   966,   982,   990,   998,  1007,
    1024,  1032,  1040,  1053,  1061,  1070,  1079,  1088,  1097,  1111,
    1114,  1118,  1122,  1126,  1130,  1134,  1138,  1142,  1146,  1150,
    1154,  1158,  1162,  1166,  1176,  1183,  1191,  1199,  1207,  1215,
    1228,  1239,  1247,  1255,  1263,  1274,  1282,  1297,  1304,  1313,
    1321,  1329,  1339,  1347,  1359,  1364,  1373,  1374,  1378,  1383
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "TOK_INTEGER_LITERAL",
  "TOK_REAL_LITERAL", "TOK_STRING_LITERAL", "TOK_IDENTIFIER", "TOK_BREAK",
  "TOK_CASE", "TOK_CLASS", "TOK_CONST", "TOK_CONTINUE", "TOK_DEFAULT",
  "TOK_DELETE", "TOK_DO", "TOK_ELSE", "TOK_ENUM", "TOK_FOR", "TOK_FRIEND",
  "TOK_GOTO", "TOK_IF", "TOK_NEW", "TOK_RETURN", "TOK_STATIC",
  "TOK_STATIC_CAST", "TOK_STRUCT", "TOK_SWITCH", "TOK_TYPEDEF",
  "TOK_UNION", "TOK_WHILE", "TOK_KW_INT", "TOK_KW_DOUBLE", "TOK_KW_FLOAT",
  "TOK_KW_CHAR", "TOK_KW_BOOL", "TOK_KW_VOID", "TOK_KW_LONG",
  "TOK_KW_SHORT", "TOK_UNSIGNED", "TOK_LPAREN", "TOK_RPAREN", "TOK_LBRACE",
  "TOK_RBRACE", "TOK_LBRACKET", "TOK_RBRACKET", "TOK_SEMICOLON",
  "TOK_COLON", "TOK_COMMA", "TOK_QUESTION", "TOK_ASSIGN", "TOK_EQ",
  "TOK_PLUS", "TOK_PLUS_ASSIGN", "TOK_INCREMENT", "TOK_MINUS",
  "TOK_MINUS_ASSIGN", "TOK_DECREMENT", "TOK_STAR", "TOK_STAR_ASSIGN",
  "TOK_SLASH", "TOK_SLASH_ASSIGN", "TOK_PERCENT", "TOK_PERCENT_ASSIGN",
  "TOK_AMP", "TOK_AMP_ASSIGN", "TOK_LOGICAL_AND", "TOK_PIPE",
  "TOK_PIPE_ASSIGN", "TOK_LOGICAL_OR", "TOK_CARET", "TOK_CARET_ASSIGN",
  "TOK_TILDE", "TOK_LOGICAL_NOT", "TOK_NE", "TOK_LT", "TOK_LE", "TOK_SHL",
  "TOK_SHL_ASSIGN", "TOK_GT", "TOK_GE", "TOK_SHR", "TOK_SHR_ASSIGN",
  "UNARY_PREC", "$accept", "program", "F_STATEMENTS", "F_STATEMENT", "$@1",
  "$@2", "$@3", "$@4", "STATEMENTS", "STATEMENT", "$@5", "$@6", "$@7",
  "$@8", "$@9", "CHOOSE_VARIANTS", "CHOOSE_VARIANT", "TYPE_SPEC",
  "BASE_LIST_OPT", "BASE_LIST", "ENUM_VALUE_LIST", "ENUM_VALUE", "PARAMS",
  "PARAM_LIST", "PARAM", "DECLARATOR_LIST", "DECLARATOR", "STATIC_OPT",
  "CONST_OPT", "FOR_INIT", "expr", "assign_expr", "INDEX_SUFFIX_LIST",
  "ARG_LIST_OPT", "ARG_LIST", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-264)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-50)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    -264,    20,   602,  -264,    43,    53,    90,  -264,    80,  1079,
      95,  -264,    67,   123,   130,   173,   178,  -264,  -264,  -264,
    -264,  -264,  -264,  -264,  -264,    76,     5,   123,    15,   170,
    -264,  1079,   181,   175,   189,   169,   177,  -264,  -264,  -264,
    -264,   188,  -264,   190,   185,  -264,     3,  -264,   192,  -264,
     186,    -3,  -264,  -264,  -264,  1079,  -264,  -264,  -264,    59,
     230,   117,  -264,   234,  -264,   735,   197,   189,     7,   206,
     200,  -264,  -264,   201,   735,   735,   209,   207,  -264,  -264,
       6,  -264,   252,  -264,  -264,  -264,   212,   202,  1079,   642,
     735,   735,   735,   735,   735,   735,   735,   735,   760,  -264,
    -264,   -18,  -264,   215,  1079,   260,   217,    14,   760,   760,
    1079,   735,  -264,   218,   735,   208,   -15,     4,   -23,    27,
     -15,   -15,   -15,   -15,   -15,   -15,   -15,   -15,   735,   735,
     735,   735,   735,  -264,   735,   735,  -264,   735,   735,   735,
     735,   735,   735,   735,   735,   735,   735,   735,   735,   735,
     735,   735,   735,   735,   735,   735,   735,   735,   735,   735,
     207,  -264,  -264,   219,  -264,  -264,   735,   214,    72,  -264,
     760,   227,   224,   735,   735,   735,   735,  -264,   156,   760,
     -24,   316,   760,   316,   760,   -15,   760,   -15,   799,   -15,
     799,  1047,   799,   945,   979,   799,   911,  1013,   838,   -24,
     124,   124,   726,   838,   124,   124,   726,   838,  -264,   760,
     231,  -264,  -264,   735,   -15,   241,    89,   -15,   735,  -264,
     760,  -264,  -264,   877,   447,    39,   237,   278,   244,   587,
     280,   251,   286,   254,   679,   289,   257,  1079,   291,   259,
    -264,  -264,  -264,    67,   125,   587,  -264,   123,  -264,   270,
     263,  -264,   256,   735,  -264,   143,   123,   735,    16,   264,
     735,  -264,  1079,  -264,  -264,   265,   268,   189,   704,  -264,
      42,  -264,   267,    60,   272,  -264,    70,   517,     7,  -264,
     735,   118,    67,   735,   146,   587,  -264,   281,  -264,  -264,
     587,  -264,   147,  -264,    71,   292,  1079,   151,  -264,   310,
    -264,  -264,   329,  -264,  -264,   399,   294,  -264,     7,   735,
     587,   469,    34,   295,   296,  -264,   154,   121,  -264,   297,
     735,   285,  -264,  -264,  -264,  -264,  -264,   587,  -264,   174,
    -264,  -264,  -264,   307,   377
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       3,     0,    83,     1,     0,     0,     0,    84,     0,     0,
       0,     4,    85,    64,     0,     0,     0,    50,    51,    52,
      53,    54,    55,    56,    57,    58,     0,    64,     0,     0,
      86,     0,     0,     0,     0,     0,     0,    59,    60,    62,
      61,     0,    63,     0,     0,    11,     0,    66,    65,     9,
      70,     0,    68,    13,    14,    72,     7,    17,     3,    79,
       0,     0,    77,     0,     3,     0,     0,     0,     0,     0,
      73,    74,     3,    83,     0,     0,     0,    82,    80,    18,
       0,    67,    83,    91,    92,    93,    94,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    71,    16,
      69,    79,    76,     0,     0,    83,     0,     0,    89,    81,
      72,     0,    78,     0,   146,     0,   142,   139,     0,     0,
     109,   104,   108,   106,   136,   137,   122,   118,     0,     0,
       0,     0,     0,   105,     0,     0,   107,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      98,    15,    75,     0,    12,   144,     0,     0,     0,    10,
     148,     0,   147,     0,   146,     0,     0,    96,     0,   125,
     110,    99,   126,   100,   127,   101,   128,   102,   129,   103,
     130,   119,   131,   116,   120,   132,   117,   121,   133,   115,
     111,   112,   124,   135,   113,   114,   123,   134,     8,    90,
       0,   145,    95,     0,   141,     0,     0,    97,     0,    19,
     149,   140,   138,   143,    83,    94,     0,     0,     0,    83,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      22,     6,    20,    85,     0,    83,    33,    64,    34,     0,
       0,    29,     0,     0,    32,     0,    64,     0,     0,     0,
       0,    19,     0,    21,    36,     0,     0,     0,    83,    35,
       0,    31,     0,     0,     0,    43,     0,    83,     0,    41,
       0,     0,    85,     0,     0,    83,    39,     0,    45,     3,
      83,    23,     0,     3,     0,     0,     0,     0,    87,    25,
       3,    46,    83,    27,    37,    83,     0,    38,     0,     0,
      83,    83,     0,     0,     0,    28,     0,     0,    24,     0,
       0,     0,    26,    47,    44,    42,    88,    83,    40,     0,
      19,    30,    19,    83,    83
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -264,  -264,   -56,  -264,  -264,  -264,  -264,  -264,  -251,  -222,
    -264,  -264,  -264,  -264,  -264,  -264,  -264,    -5,   -22,  -264,
      77,   276,   240,  -264,   247,  -263,   -44,    -2,  -229,  -264,
     -71,     0,   -53,   179,  -264
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     1,     2,    11,    76,    72,    64,    58,   224,   242,
     261,   268,   300,   293,   289,   312,   323,    68,    33,    48,
      51,    52,    69,    70,    71,    61,    62,   243,    31,   283,
     244,   108,   160,   171,   172
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      12,    26,    73,   107,    28,    43,    77,   249,    82,    59,
     277,    41,   101,   101,   262,   292,   105,   176,   119,    74,
       3,    44,   274,   264,   102,    74,    46,   131,    74,   133,
     134,    75,   136,   137,    42,   139,   112,   141,   133,    66,
     168,   136,   320,   174,    67,   316,   321,   175,    77,    13,
     152,   153,   154,   296,   156,   157,   158,   178,   165,    14,
      42,   166,    42,   299,    42,    98,    60,   177,   303,    60,
      60,    12,    42,    42,   166,   109,   322,    30,   114,   333,
      12,   334,   285,   117,   118,   245,    27,   116,   318,   166,
     120,   121,   122,   123,   124,   125,   126,   127,    -5,    15,
     287,    29,    74,    12,   216,   331,    37,   166,    75,    38,
     290,   306,    39,    40,   170,    16,   211,   166,   166,   166,
      17,    18,    19,    20,    21,    22,    23,    24,    25,   179,
     180,   181,   182,   222,   183,   184,   166,   185,   186,   187,
     188,   189,   190,   191,   192,   193,   194,   195,   196,   197,
     198,   199,   200,   201,   202,   203,   204,   205,   206,   207,
     295,   327,    79,   255,    80,    67,   209,    74,   166,    32,
     263,    34,   166,   214,   170,   131,   217,   133,   134,    35,
     136,   137,   270,   139,    36,   141,   273,    47,   271,   276,
     166,   298,   304,   166,    80,    50,   309,   284,   166,   326,
     154,    80,   218,   166,   158,    83,    84,    85,    86,   294,
       4,    45,   297,   220,    53,    87,    49,     5,   223,     6,
     332,   166,    54,    88,     7,   265,     8,    55,     9,    10,
      57,    56,   258,   302,   272,    65,    78,   305,   317,    63,
      81,    89,    99,   106,   311,   115,   103,   104,   110,   329,
     111,   114,   173,    90,   210,    91,    92,   278,    93,    94,
     161,     4,   164,   169,   208,    95,   282,   212,     5,     4,
       6,   213,   219,    96,    97,     7,     5,     8,     6,     9,
      10,   221,   246,     7,   247,     8,   250,     9,    10,   248,
     251,   308,   252,   253,   113,   256,   257,   259,   260,   266,
      12,   269,   163,    12,   267,   275,   279,   280,   286,    12,
      83,    84,    85,   225,   226,   -49,   227,   288,   228,   -49,
      87,   229,   301,   230,   231,   310,   232,   233,    88,   234,
       7,   330,   235,   236,   237,   238,   239,   307,     4,   315,
     324,   325,   328,   100,   281,     5,    89,     6,   240,   -49,
     167,   162,     7,   215,     8,     0,     9,    10,    90,    74,
      91,    92,     0,    93,    94,     0,     0,     0,     0,   133,
      95,   313,   136,   137,     0,   139,     0,   141,    96,    97,
      83,    84,    85,   225,   226,   -48,   227,     0,   228,   -48,
      87,   229,     0,   230,   231,     0,   232,   233,    88,   234,
       7,     0,   235,   236,   237,   238,   239,     0,     4,     0,
       0,     0,     0,     0,     0,     5,    89,     6,   240,   -48,
       0,     0,     7,     0,     8,     0,     9,    10,    90,     0,
      91,    92,     0,    93,    94,     0,     0,     0,     0,     0,
      95,   314,     0,     0,     0,     0,     0,     0,    96,    97,
      83,    84,    85,   225,   226,     0,   227,     0,   228,     0,
      87,   229,     0,   230,   231,     0,   232,   233,    88,   234,
       7,     0,   235,   236,   237,   238,   239,     0,     4,     0,
       0,     0,     0,     0,     0,     5,    89,     6,   240,   241,
       0,     0,     7,     0,     8,     0,     9,    10,    90,     0,
      91,    92,     0,    93,    94,     0,     0,     0,     0,     0,
      95,   319,     0,     0,     0,     0,     0,     0,    96,    97,
      83,    84,    85,   225,   226,     0,   227,     0,   228,     0,
      87,   229,     0,   230,   231,     0,   232,   233,    88,   234,
       7,     0,   235,   236,   237,   238,   239,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    89,     0,   240,   291,
       0,     0,     0,     0,     0,     0,     0,     0,    90,     0,
      91,    92,     0,    93,    94,     0,     0,     0,     0,     0,
      95,     0,     0,     0,     0,     0,     0,     0,    96,    97,
      83,    84,    85,   225,   226,     0,   227,     0,   228,     0,
      87,   229,    -2,   230,   231,     0,   232,   233,    88,   234,
       7,     4,   235,   236,   237,   238,   239,     0,     5,     0,
       6,     0,     0,     0,     0,     7,    89,     8,   240,     9,
      10,     0,     0,     0,     0,     0,     0,     0,    90,     0,
      91,    92,     0,    93,    94,    83,    84,    85,    86,     0,
      95,     0,     0,     0,     0,    87,     0,     0,    96,    97,
       0,     0,     0,    88,     0,     0,     0,     0,     0,     0,
       0,     0,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    89,    83,    84,    85,    86,     0,     0,     0,     0,
       0,     0,    87,    90,     0,    91,    92,     0,    93,    94,
      88,     0,     0,     0,     0,    95,     0,    83,    84,    85,
      86,     0,     0,    96,    97,     0,     0,    87,    89,     0,
       0,     0,     0,     0,   254,    88,     0,     7,     0,     0,
      90,     0,    91,    92,     0,    93,    94,     0,    83,    84,
      85,    86,    95,    89,     0,     0,     0,     0,    87,     0,
      96,    97,     0,     0,     0,    90,    88,    91,    92,     0,
      93,    94,     0,     0,     0,     0,     0,    95,     0,    74,
       0,     0,     0,     0,    89,    96,    97,   131,     0,   133,
     134,     0,   136,   137,     0,   139,    90,   141,    91,    92,
       0,    93,    94,     0,     0,     0,     0,     0,    95,     0,
       0,     0,     0,    74,     0,     0,    96,    97,   128,   129,
     130,   131,   132,   133,   134,   135,   136,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,     0,     0,   151,   152,   153,   154,   155,   156,   157,
     158,   159,    74,     0,     0,     0,     0,   128,     0,   130,
     131,     0,   133,   134,     0,   136,   137,     0,   139,   140,
     141,   142,   143,   144,   145,   146,   147,   148,   149,   150,
       0,     0,   151,   152,   153,   154,   155,   156,   157,   158,
     159,    74,     0,     0,     0,     0,   128,     0,   130,   131,
       0,   133,   134,     0,   136,   137,     0,   139,     0,   141,
       0,   143,     0,   145,   146,     0,   148,   149,   150,     0,
       0,   151,   152,   153,   154,   155,   156,   157,   158,   159,
      74,     0,     0,     0,     0,   128,     0,   130,   131,     0,
     133,   134,     0,   136,   137,     0,   139,     0,   141,     0,
     143,     0,   145,   146,     0,   148,   149,     0,     0,     0,
     151,   152,   153,   154,    74,   156,   157,   158,     0,     0,
       0,   130,   131,     0,   133,   134,     0,   136,   137,     0,
     139,     0,   141,     0,   143,     0,   145,   146,     0,     0,
     149,     0,     0,     0,   151,   152,   153,   154,    74,   156,
     157,   158,     0,     0,     0,   130,   131,     0,   133,   134,
       0,   136,   137,     0,   139,     0,   141,     0,   143,     0,
       0,   146,     0,     0,   149,     0,     0,     0,   151,   152,
     153,   154,    74,   156,   157,   158,     0,     0,     0,   130,
     131,     0,   133,   134,     0,   136,   137,     0,   139,     0,
     141,     0,   143,     0,     0,     0,     0,     0,   149,     0,
       0,     0,   151,   152,   153,   154,    74,   156,   157,   158,
       0,     0,     0,   130,   131,     0,   133,   134,     0,   136,
     137,     0,   139,     0,   141,     0,   143,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   151,   152,   153,   154,
      74,   156,   157,   158,     0,     0,     0,   130,   131,     0,
     133,   134,     0,   136,   137,     0,   139,     0,   141,    17,
      18,    19,    20,    21,    22,    23,    24,    25,     0,     0,
     151,   152,   153,   154,     0,   156,   157,   158
};

static const yytype_int16 yycheck[] =
{
       2,     6,    58,    74,     9,    27,    59,   229,    64,     6,
     261,     6,     6,     6,   243,   278,    72,    40,    89,    43,
       0,     6,     6,   245,    68,    43,    31,    51,    43,    53,
      54,    49,    56,    57,    57,    59,    80,    61,    53,    42,
     111,    56,     8,    39,    47,   308,    12,    43,   101,     6,
      74,    75,    76,   282,    78,    79,    80,   128,    44,     6,
      57,    47,    57,   285,    57,    65,    63,    40,   290,    63,
      63,    73,    57,    57,    47,    75,    42,    10,    39,   330,
      82,   332,    40,    88,    89,    46,     6,    87,   310,    47,
      90,    91,    92,    93,    94,    95,    96,    97,    39,     9,
      40,     6,    43,   105,   175,   327,    30,    47,    49,    33,
      40,    40,    36,    37,   114,    25,    44,    47,    47,    47,
      30,    31,    32,    33,    34,    35,    36,    37,    38,   129,
     130,   131,   132,    44,   134,   135,    47,   137,   138,   139,
     140,   141,   142,   143,   144,   145,   146,   147,   148,   149,
     150,   151,   152,   153,   154,   155,   156,   157,   158,   159,
      42,    40,    45,   234,    47,    47,   166,    43,    47,    46,
      45,    41,    47,   173,   174,    51,   176,    53,    54,     6,
      56,    57,   253,    59,     6,    61,   257,     6,    45,   260,
      47,    45,    45,    47,    47,     6,    45,   268,    47,    45,
      76,    47,    46,    47,    80,     3,     4,     5,     6,   280,
       9,    41,   283,   213,    45,    13,    41,    16,   218,    18,
      46,    47,    45,    21,    23,   247,    25,    39,    27,    28,
      45,    41,   237,   289,   256,    49,     6,   293,   309,    47,
       6,    39,    45,    42,   300,    43,    40,    47,    39,   320,
      43,    39,    44,    51,    40,    53,    54,   262,    56,    57,
      45,     9,    45,    45,    45,    63,   268,    40,    16,     9,
      18,    47,    41,    71,    72,    23,    16,    25,    18,    27,
      28,    40,    45,    23,     6,    25,     6,    27,    28,    45,
      39,   296,     6,    39,    42,     6,    39,     6,    39,    29,
     302,    45,    42,   305,    41,    41,    41,    39,    41,   311,
       3,     4,     5,     6,     7,     8,     9,    45,    11,    12,
      13,    14,    41,    16,    17,    15,    19,    20,    21,    22,
      23,    46,    25,    26,    27,    28,    29,    45,     9,    45,
      45,    45,    45,    67,   267,    16,    39,    18,    41,    42,
     110,   104,    23,   174,    25,    -1,    27,    28,    51,    43,
      53,    54,    -1,    56,    57,    -1,    -1,    -1,    -1,    53,
      63,    42,    56,    57,    -1,    59,    -1,    61,    71,    72,
       3,     4,     5,     6,     7,     8,     9,    -1,    11,    12,
      13,    14,    -1,    16,    17,    -1,    19,    20,    21,    22,
      23,    -1,    25,    26,    27,    28,    29,    -1,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    16,    39,    18,    41,    42,
      -1,    -1,    23,    -1,    25,    -1,    27,    28,    51,    -1,
      53,    54,    -1,    56,    57,    -1,    -1,    -1,    -1,    -1,
      63,    42,    -1,    -1,    -1,    -1,    -1,    -1,    71,    72,
       3,     4,     5,     6,     7,    -1,     9,    -1,    11,    -1,
      13,    14,    -1,    16,    17,    -1,    19,    20,    21,    22,
      23,    -1,    25,    26,    27,    28,    29,    -1,     9,    -1,
      -1,    -1,    -1,    -1,    -1,    16,    39,    18,    41,    42,
      -1,    -1,    23,    -1,    25,    -1,    27,    28,    51,    -1,
      53,    54,    -1,    56,    57,    -1,    -1,    -1,    -1,    -1,
      63,    42,    -1,    -1,    -1,    -1,    -1,    -1,    71,    72,
       3,     4,     5,     6,     7,    -1,     9,    -1,    11,    -1,
      13,    14,    -1,    16,    17,    -1,    19,    20,    21,    22,
      23,    -1,    25,    26,    27,    28,    29,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    39,    -1,    41,    42,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    51,    -1,
      53,    54,    -1,    56,    57,    -1,    -1,    -1,    -1,    -1,
      63,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    71,    72,
       3,     4,     5,     6,     7,    -1,     9,    -1,    11,    -1,
      13,    14,     0,    16,    17,    -1,    19,    20,    21,    22,
      23,     9,    25,    26,    27,    28,    29,    -1,    16,    -1,
      18,    -1,    -1,    -1,    -1,    23,    39,    25,    41,    27,
      28,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    51,    -1,
      53,    54,    -1,    56,    57,     3,     4,     5,     6,    -1,
      63,    -1,    -1,    -1,    -1,    13,    -1,    -1,    71,    72,
      -1,    -1,    -1,    21,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,     3,     4,     5,     6,    -1,    -1,    -1,    -1,
      -1,    -1,    13,    51,    -1,    53,    54,    -1,    56,    57,
      21,    -1,    -1,    -1,    -1,    63,    -1,     3,     4,     5,
       6,    -1,    -1,    71,    72,    -1,    -1,    13,    39,    -1,
      -1,    -1,    -1,    -1,    45,    21,    -1,    23,    -1,    -1,
      51,    -1,    53,    54,    -1,    56,    57,    -1,     3,     4,
       5,     6,    63,    39,    -1,    -1,    -1,    -1,    13,    -1,
      71,    72,    -1,    -1,    -1,    51,    21,    53,    54,    -1,
      56,    57,    -1,    -1,    -1,    -1,    -1,    63,    -1,    43,
      -1,    -1,    -1,    -1,    39,    71,    72,    51,    -1,    53,
      54,    -1,    56,    57,    -1,    59,    51,    61,    53,    54,
      -1,    56,    57,    -1,    -1,    -1,    -1,    -1,    63,    -1,
      -1,    -1,    -1,    43,    -1,    -1,    71,    72,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    -1,    -1,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    43,    -1,    -1,    -1,    -1,    48,    -1,    50,
      51,    -1,    53,    54,    -1,    56,    57,    -1,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      -1,    -1,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    43,    -1,    -1,    -1,    -1,    48,    -1,    50,    51,
      -1,    53,    54,    -1,    56,    57,    -1,    59,    -1,    61,
      -1,    63,    -1,    65,    66,    -1,    68,    69,    70,    -1,
      -1,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      43,    -1,    -1,    -1,    -1,    48,    -1,    50,    51,    -1,
      53,    54,    -1,    56,    57,    -1,    59,    -1,    61,    -1,
      63,    -1,    65,    66,    -1,    68,    69,    -1,    -1,    -1,
      73,    74,    75,    76,    43,    78,    79,    80,    -1,    -1,
      -1,    50,    51,    -1,    53,    54,    -1,    56,    57,    -1,
      59,    -1,    61,    -1,    63,    -1,    65,    66,    -1,    -1,
      69,    -1,    -1,    -1,    73,    74,    75,    76,    43,    78,
      79,    80,    -1,    -1,    -1,    50,    51,    -1,    53,    54,
      -1,    56,    57,    -1,    59,    -1,    61,    -1,    63,    -1,
      -1,    66,    -1,    -1,    69,    -1,    -1,    -1,    73,    74,
      75,    76,    43,    78,    79,    80,    -1,    -1,    -1,    50,
      51,    -1,    53,    54,    -1,    56,    57,    -1,    59,    -1,
      61,    -1,    63,    -1,    -1,    -1,    -1,    -1,    69,    -1,
      -1,    -1,    73,    74,    75,    76,    43,    78,    79,    80,
      -1,    -1,    -1,    50,    51,    -1,    53,    54,    -1,    56,
      57,    -1,    59,    -1,    61,    -1,    63,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    73,    74,    75,    76,
      43,    78,    79,    80,    -1,    -1,    -1,    50,    51,    -1,
      53,    54,    -1,    56,    57,    -1,    59,    -1,    61,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    -1,    -1,
      73,    74,    75,    76,    -1,    78,    79,    80
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,    84,    85,     0,     9,    16,    18,    23,    25,    27,
      28,    86,   110,     6,     6,     9,    25,    30,    31,    32,
      33,    34,    35,    36,    37,    38,   100,     6,   100,     6,
      10,   111,    46,   101,    41,     6,     6,    30,    33,    36,
      37,     6,    57,   101,     6,    41,   100,     6,   102,    41,
       6,   103,   104,    45,    45,    39,    41,    45,    90,     6,
      63,   108,   109,    47,    89,    49,    42,    47,   100,   105,
     106,   107,    88,    85,    43,    49,    87,   115,     6,    45,
      47,     6,    85,     3,     4,     5,     6,    13,    21,    39,
      51,    53,    54,    56,    57,    63,    71,    72,   114,    45,
     104,     6,   109,    40,    47,    85,    42,   113,   114,   114,
      39,    43,   109,    42,    39,    43,   114,   100,   100,   113,
     114,   114,   114,   114,   114,   114,   114,   114,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    73,    74,    75,    76,    77,    78,    79,    80,    81,
     115,    45,   107,    42,    45,    44,    47,   105,   113,    45,
     114,   116,   117,    44,    39,    43,    40,    40,   113,   114,
     114,   114,   114,   114,   114,   114,   114,   114,   114,   114,
     114,   114,   114,   114,   114,   114,   114,   114,   114,   114,
     114,   114,   114,   114,   114,   114,   114,   114,    45,   114,
      40,    44,    40,    47,   114,   116,   113,   114,    46,    41,
     114,    40,    44,   114,    91,     6,     7,     9,    11,    14,
      16,    17,    19,    20,    22,    25,    26,    27,    28,    29,
      41,    42,    92,   110,   113,    46,    45,     6,    45,    92,
       6,    39,     6,    39,    45,   113,     6,    39,   100,     6,
      39,    93,   111,    45,    92,   101,    29,    41,    94,    45,
     113,    45,   101,   113,     6,    41,   113,    91,   100,    41,
      39,   103,   110,   112,   113,    40,    41,    40,    45,    97,
      40,    42,   108,    96,   113,    42,   111,   113,    45,    92,
      95,    41,    85,    92,    45,    85,    40,    45,   100,    45,
      15,    85,    98,    42,    42,    45,   108,   113,    92,    42,
       8,    12,    42,    99,    45,    45,    45,    40,    45,   113,
      46,    92,    46,    91,    91
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr1[] =
{
       0,    83,    84,    85,    85,    87,    86,    88,    86,    89,
      86,    90,    86,    86,    86,    86,    86,    86,    86,    91,
      91,    92,    93,    92,    92,    92,    92,    92,    92,    94,
      92,    92,    92,    92,    92,    92,    92,    92,    92,    95,
      92,    96,    92,    97,    92,    92,    98,    98,    99,    99,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   101,   101,   102,   102,   103,   103,
     104,   104,   105,   105,   106,   106,   107,   108,   108,   109,
     109,   109,   109,   110,   110,   111,   111,   112,   112,   113,
     113,   114,   114,   114,   114,   114,   114,   114,   114,   114,
     114,   114,   114,   114,   114,   114,   114,   114,   114,   114,
     114,   114,   114,   114,   114,   114,   114,   114,   114,   114,
     114,   114,   114,   114,   114,   114,   114,   114,   114,   114,
     114,   114,   114,   114,   114,   114,   114,   114,   114,   114,
     114,   114,   114,   114,   115,   115,   116,   116,   117,   117
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     0,     2,     0,    11,     0,     8,     0,
       8,     0,     7,     4,     4,     7,     6,     4,     5,     0,
       2,     2,     0,     4,     7,     5,     7,     5,     7,     0,
       9,     3,     2,     2,     2,     3,     3,     5,     6,     0,
       8,     0,     8,     0,     7,     4,     0,     2,     4,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       2,     2,     2,     2,     0,     2,     1,     3,     1,     3,
       1,     3,     0,     1,     1,     3,     2,     1,     3,     1,
       2,     3,     2,     0,     1,     0,     1,     2,     5,     1,
       3,     1,     1,     1,     1,     4,     3,     4,     2,     3,
       3,     3,     3,     3,     2,     2,     2,     2,     2,     2,
       3,     3,     3,     3,     3,     3,     3,     3,     2,     3,
       3,     3,     2,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     2,     2,     5,     2,
       5,     4,     2,     5,     3,     4,     0,     1,     1,     3
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* program: F_STATEMENTS  */
#line 336 "parser.y"
                 {
        g_output = *(yyvsp[0].expval);
        delete (yyvsp[0].expval);
    }
#line 1821 "parser.tab.cpp"
    break;

  case 3: /* F_STATEMENTS: %empty  */
#line 344 "parser.y"
     {
        (yyval.expval) = new nlohmann::ordered_json(nlohmann::ordered_json::array());
    }
#line 1829 "parser.tab.cpp"
    break;

  case 4: /* F_STATEMENTS: F_STATEMENTS F_STATEMENT  */
#line 347 "parser.y"
                               {
        (yyvsp[-1].expval)->push_back(*(yyvsp[0].expval));
        (yyval.expval) = (yyvsp[-1].expval);
        delete (yyvsp[0].expval);
    }
#line 1839 "parser.tab.cpp"
    break;

  case 5: /* $@1: %empty  */
#line 356 "parser.y"
    {
        g_symbolTable.pushScope();
        g_functionReturnTypes[*(yyvsp[0].sval)] = *(yyvsp[-1].expval);
    }
#line 1848 "parser.tab.cpp"
    break;

  case 6: /* F_STATEMENT: STATIC_OPT CONST_OPT TYPE_SPEC TOK_IDENTIFIER $@1 TOK_LPAREN PARAMS TOK_RPAREN TOK_LBRACE STATEMENTS TOK_RBRACE  */
#line 361 "parser.y"
                                     {
        g_symbolTable.popScope();
        markLastReturn((yyvsp[-1].expval));
        nlohmann::ordered_json body = {
            {"type", "compound-statement"},
            {"statements", *(yyvsp[-1].expval)}
        };
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "subroutine-declaration"},
            {"subroutine-name", *(yyvsp[-7].sval)},
            {"result-type", *(yyvsp[-8].expval)},
            {"arguments-declarators", *(yyvsp[-4].expval)},
            {"constant-section", nlohmann::ordered_json::array()},
            {"type-section", nlohmann::ordered_json::array()},
            {"variables-section", nlohmann::ordered_json::array()},
            {"subroutines-section", nlohmann::ordered_json::array()},
            {"subroutine-body", body}
        });
        delete (yyvsp[-8].expval); delete (yyvsp[-7].sval); delete (yyvsp[-4].expval); delete (yyvsp[-1].expval);
    }
#line 1873 "parser.tab.cpp"
    break;

  case 7: /* $@2: %empty  */
#line 381 "parser.y"
                                                         {
        g_symbolTable.pushScope();
        g_structDepth++;
    }
#line 1882 "parser.tab.cpp"
    break;

  case 8: /* F_STATEMENT: TOK_STRUCT TOK_IDENTIFIER BASE_LIST_OPT TOK_LBRACE $@2 F_STATEMENTS TOK_RBRACE TOK_SEMICOLON  */
#line 385 "parser.y"
                                          {
        g_structDepth--;
        g_symbolTable.popScope();
        nlohmann::ordered_json fields = nlohmann::ordered_json::array();
        nlohmann::ordered_json methods = nlohmann::ordered_json::array();
        nlohmann::ordered_json friendsArr = nlohmann::ordered_json::array();
        splitMembers((yyvsp[-2].expval), fields, methods, friendsArr);
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "struct-type"},
            {"name", *(yyvsp[-6].sval)},
            {"base-structs", *(yyvsp[-5].expval)},
            {"fields", fields},
            {"methods", methods},
            {"friends", friendsArr}
        });
        delete (yyvsp[-6].sval); delete (yyvsp[-5].expval); delete (yyvsp[-2].expval);
    }
#line 1904 "parser.tab.cpp"
    break;

  case 9: /* $@3: %empty  */
#line 402 "parser.y"
                                                        {
        g_symbolTable.pushScope();
        g_structDepth++;
    }
#line 1913 "parser.tab.cpp"
    break;

  case 10: /* F_STATEMENT: TOK_CLASS TOK_IDENTIFIER BASE_LIST_OPT TOK_LBRACE $@3 F_STATEMENTS TOK_RBRACE TOK_SEMICOLON  */
#line 406 "parser.y"
                                          {
        g_structDepth--;
        g_symbolTable.popScope();
        nlohmann::ordered_json fields = nlohmann::ordered_json::array();
        nlohmann::ordered_json methods = nlohmann::ordered_json::array();
        nlohmann::ordered_json friendsArr = nlohmann::ordered_json::array();
        splitMembers((yyvsp[-2].expval), fields, methods, friendsArr);
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "struct-type"},
            {"name", *(yyvsp[-6].sval)},
            {"base-structs", *(yyvsp[-5].expval)},
            {"fields", fields},
            {"methods", methods},
            {"friends", friendsArr}
        });
        delete (yyvsp[-6].sval); delete (yyvsp[-5].expval); delete (yyvsp[-2].expval);
    }
#line 1935 "parser.tab.cpp"
    break;

  case 11: /* $@4: %empty  */
#line 423 "parser.y"
                                          {
        g_symbolTable.pushScope();
        g_structDepth++;
    }
#line 1944 "parser.tab.cpp"
    break;

  case 12: /* F_STATEMENT: TOK_UNION TOK_IDENTIFIER TOK_LBRACE $@4 F_STATEMENTS TOK_RBRACE TOK_SEMICOLON  */
#line 427 "parser.y"
                                          {
        g_structDepth--;
        g_symbolTable.popScope();
        nlohmann::ordered_json fields = nlohmann::ordered_json::array();
        nlohmann::ordered_json methods = nlohmann::ordered_json::array();
        nlohmann::ordered_json friendsArr = nlohmann::ordered_json::array();
        splitMembers((yyvsp[-2].expval), fields, methods, friendsArr);
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "union-type"},
            {"name", *(yyvsp[-5].sval)},
            {"fields", fields},
            {"methods", methods},
            {"friends", friendsArr}
        });
        delete (yyvsp[-5].sval); delete (yyvsp[-2].expval);
    }
#line 1965 "parser.tab.cpp"
    break;

  case 13: /* F_STATEMENT: TOK_FRIEND TOK_CLASS TOK_IDENTIFIER TOK_SEMICOLON  */
#line 443 "parser.y"
                                                        {
        (yyval.expval) = new nlohmann::ordered_json({{"type", "friend-declaration"}, {"friend-name", *(yyvsp[-1].sval)}});
        delete (yyvsp[-1].sval);
    }
#line 1974 "parser.tab.cpp"
    break;

  case 14: /* F_STATEMENT: TOK_FRIEND TOK_STRUCT TOK_IDENTIFIER TOK_SEMICOLON  */
#line 447 "parser.y"
                                                         {
        (yyval.expval) = new nlohmann::ordered_json({{"type", "friend-declaration"}, {"friend-name", *(yyvsp[-1].sval)}});
        delete (yyvsp[-1].sval);
    }
#line 1983 "parser.tab.cpp"
    break;

  case 15: /* F_STATEMENT: TOK_FRIEND TYPE_SPEC TOK_IDENTIFIER TOK_LPAREN PARAMS TOK_RPAREN TOK_SEMICOLON  */
#line 451 "parser.y"
                                                                                     {
        (yyval.expval) = new nlohmann::ordered_json({{"type", "friend-declaration"}, {"friend-name", *(yyvsp[-4].sval)}});
        delete (yyvsp[-5].expval); delete (yyvsp[-4].sval); delete (yyvsp[-2].expval);
    }
#line 1992 "parser.tab.cpp"
    break;

  case 16: /* F_STATEMENT: TOK_ENUM TOK_IDENTIFIER TOK_LBRACE ENUM_VALUE_LIST TOK_RBRACE TOK_SEMICOLON  */
#line 455 "parser.y"
                                                                                  {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "enumeration-type"},
            {"name", *(yyvsp[-4].sval)},
            {"values", *(yyvsp[-2].expval)}
        });
        delete (yyvsp[-4].sval); delete (yyvsp[-2].expval);
    }
#line 2005 "parser.tab.cpp"
    break;

  case 17: /* F_STATEMENT: TOK_TYPEDEF TYPE_SPEC TOK_IDENTIFIER TOK_SEMICOLON  */
#line 463 "parser.y"
                                                         {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "typedef-type"},
            {"name", *(yyvsp[-1].sval)},
            {"old-name", *(yyvsp[-2].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[-1].sval);
    }
#line 2018 "parser.tab.cpp"
    break;

  case 18: /* F_STATEMENT: STATIC_OPT CONST_OPT TYPE_SPEC DECLARATOR_LIST TOK_SEMICOLON  */
#line 471 "parser.y"
                                                                   {
        (yyval.expval) = buildVarDecl(g_structDepth == 0, (yyvsp[-4].ival), (yyvsp[-3].ival), (yyvsp[-2].expval), (yyvsp[-1].expval));
    }
#line 2026 "parser.tab.cpp"
    break;

  case 19: /* STATEMENTS: %empty  */
#line 477 "parser.y"
    { (yyval.expval) = new nlohmann::ordered_json(nlohmann::ordered_json::array()); }
#line 2032 "parser.tab.cpp"
    break;

  case 20: /* STATEMENTS: STATEMENTS STATEMENT  */
#line 478 "parser.y"
                           {
        (yyvsp[-1].expval)->push_back(*(yyvsp[0].expval));
        (yyval.expval) = (yyvsp[-1].expval);
        delete (yyvsp[0].expval);
    }
#line 2042 "parser.tab.cpp"
    break;

  case 21: /* STATEMENT: expr TOK_SEMICOLON  */
#line 486 "parser.y"
                       {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "expression-statement"},
            {"expression", *(yyvsp[-1].expval)}
        });
        delete (yyvsp[-1].expval);
    }
#line 2054 "parser.tab.cpp"
    break;

  case 22: /* $@5: %empty  */
#line 493 "parser.y"
                 { g_symbolTable.pushScope(); }
#line 2060 "parser.tab.cpp"
    break;

  case 23: /* STATEMENT: TOK_LBRACE $@5 STATEMENTS TOK_RBRACE  */
#line 493 "parser.y"
                                                                      {
        g_symbolTable.popScope();
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "compound-statement"},
            {"statements", *(yyvsp[-1].expval)}
        });
        delete (yyvsp[-1].expval);
    }
#line 2073 "parser.tab.cpp"
    break;

  case 24: /* STATEMENT: TOK_IF TOK_LPAREN expr TOK_RPAREN STATEMENT TOK_ELSE STATEMENT  */
#line 501 "parser.y"
                                                                     {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "conditional-statement"},
            {"condition", *(yyvsp[-4].expval)},
            {"if-true", *(yyvsp[-2].expval)},
            {"if-false", *(yyvsp[0].expval)}
        });
        delete (yyvsp[-4].expval); delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 2087 "parser.tab.cpp"
    break;

  case 25: /* STATEMENT: TOK_IF TOK_LPAREN expr TOK_RPAREN STATEMENT  */
#line 510 "parser.y"
                                                  {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "conditional-statement"},
            {"condition", *(yyvsp[-2].expval)},
            {"if-true", *(yyvsp[0].expval)},
            {"if-false", nullptr}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 2101 "parser.tab.cpp"
    break;

  case 26: /* STATEMENT: TOK_SWITCH TOK_LPAREN expr TOK_RPAREN TOK_LBRACE CHOOSE_VARIANTS TOK_RBRACE  */
#line 519 "parser.y"
                                                                                  {
        bool passControl = computePassControl(*(yyvsp[-1].expval));
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "choose-statement"},
            {"pass-control", passControl},
            {"expression-to-analyze", *(yyvsp[-4].expval)},
            {"variants", *(yyvsp[-1].expval)}
        });
        delete (yyvsp[-4].expval); delete (yyvsp[-1].expval);
    }
#line 2116 "parser.tab.cpp"
    break;

  case 27: /* STATEMENT: TOK_WHILE TOK_LPAREN expr TOK_RPAREN STATEMENT  */
#line 529 "parser.y"
                                                     {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "loop-statement"},
            {"begin-condition", *(yyvsp[-2].expval)},
            {"end-condition", nullptr},
            {"loop-index", nullptr},
            {"loop-initialization", nullptr},
            {"loop-pass-to-next-iteration", nullptr},
            {"loop-body", *(yyvsp[0].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 2133 "parser.tab.cpp"
    break;

  case 28: /* STATEMENT: TOK_DO STATEMENT TOK_WHILE TOK_LPAREN expr TOK_RPAREN TOK_SEMICOLON  */
#line 541 "parser.y"
                                                                          {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "loop-statement"},
            {"begin-condition", nullptr},
            {"end-condition", *(yyvsp[-2].expval)},
            {"loop-index", nullptr},
            {"loop-initialization", nullptr},
            {"loop-pass-to-next-iteration", nullptr},
            {"loop-body", *(yyvsp[-5].expval)}
        });
        delete (yyvsp[-5].expval); delete (yyvsp[-2].expval);
    }
#line 2150 "parser.tab.cpp"
    break;

  case 29: /* $@6: %empty  */
#line 553 "parser.y"
                         {
        g_symbolTable.pushScope();
    }
#line 2158 "parser.tab.cpp"
    break;

  case 30: /* STATEMENT: TOK_FOR TOK_LPAREN $@6 FOR_INIT expr TOK_SEMICOLON expr TOK_RPAREN STATEMENT  */
#line 556 "parser.y"
                                                          {
        g_symbolTable.popScope();
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "loop-statement"},
            {"begin-condition", *(yyvsp[-4].expval)},
            {"end-condition", nullptr},
            {"loop-index", (*(yyvsp[-5].expval))["loop-init-name"]},
            {"loop-initialization", (*(yyvsp[-5].expval))["loop-init-node"]},
            {"loop-pass-to-next-iteration", *(yyvsp[-2].expval)},
            {"loop-body", *(yyvsp[0].expval)}
        });
        delete (yyvsp[-5].expval); delete (yyvsp[-4].expval); delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 2176 "parser.tab.cpp"
    break;

  case 31: /* STATEMENT: TOK_RETURN expr TOK_SEMICOLON  */
#line 570 "parser.y"
                                    {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "return-statement"},
            {"expression", *(yyvsp[-1].expval)},
            {"the-last-statement", false}
        });
        delete (yyvsp[-1].expval);
    }
#line 2189 "parser.tab.cpp"
    break;

  case 32: /* STATEMENT: TOK_RETURN TOK_SEMICOLON  */
#line 578 "parser.y"
                               {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "return-statement"},
            {"expression", nullptr},
            {"the-last-statement", false}
        });
    }
#line 2201 "parser.tab.cpp"
    break;

  case 33: /* STATEMENT: TOK_BREAK TOK_SEMICOLON  */
#line 585 "parser.y"
                              {
        (yyval.expval) = new nlohmann::ordered_json({{"type", "break-statement"}, {"label", nullptr}});
    }
#line 2209 "parser.tab.cpp"
    break;

  case 34: /* STATEMENT: TOK_CONTINUE TOK_SEMICOLON  */
#line 588 "parser.y"
                                 {
        (yyval.expval) = new nlohmann::ordered_json({{"type", "continue-statement"}, {"label", nullptr}});
    }
#line 2217 "parser.tab.cpp"
    break;

  case 35: /* STATEMENT: TOK_GOTO TOK_IDENTIFIER TOK_SEMICOLON  */
#line 591 "parser.y"
                                            {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "goto-statement"},
            {"target-label", *(yyvsp[-1].sval)}
        });
        delete (yyvsp[-1].sval);
    }
#line 2229 "parser.tab.cpp"
    break;

  case 36: /* STATEMENT: TOK_IDENTIFIER TOK_COLON STATEMENT  */
#line 598 "parser.y"
                                         {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "label"},
            {"label-name", *(yyvsp[-2].sval)},
            {"statement", *(yyvsp[0].expval)}
        });
        delete (yyvsp[-2].sval); delete (yyvsp[0].expval);
    }
#line 2242 "parser.tab.cpp"
    break;

  case 37: /* STATEMENT: STATIC_OPT CONST_OPT TYPE_SPEC DECLARATOR_LIST TOK_SEMICOLON  */
#line 607 "parser.y"
                                                                   {
        (yyval.expval) = buildVarDecl(false, (yyvsp[-4].ival), (yyvsp[-3].ival), (yyvsp[-2].expval), (yyvsp[-1].expval));
    }
#line 2250 "parser.tab.cpp"
    break;

  case 38: /* STATEMENT: TOK_ENUM TOK_IDENTIFIER TOK_LBRACE ENUM_VALUE_LIST TOK_RBRACE TOK_SEMICOLON  */
#line 611 "parser.y"
                                                                                  {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "enumeration-type"},
            {"name", *(yyvsp[-4].sval)},
            {"values", *(yyvsp[-2].expval)}
        });
        delete (yyvsp[-4].sval); delete (yyvsp[-2].expval);
    }
#line 2263 "parser.tab.cpp"
    break;

  case 39: /* $@7: %empty  */
#line 619 "parser.y"
                                                         {
        g_symbolTable.pushScope();
        g_structDepth++;
    }
#line 2272 "parser.tab.cpp"
    break;

  case 40: /* STATEMENT: TOK_STRUCT TOK_IDENTIFIER BASE_LIST_OPT TOK_LBRACE $@7 F_STATEMENTS TOK_RBRACE TOK_SEMICOLON  */
#line 623 "parser.y"
                                          {
        g_structDepth--;
        g_symbolTable.popScope();
        nlohmann::ordered_json fields = nlohmann::ordered_json::array();
        nlohmann::ordered_json methods = nlohmann::ordered_json::array();
        nlohmann::ordered_json friendsArr = nlohmann::ordered_json::array();
        splitMembers((yyvsp[-2].expval), fields, methods, friendsArr);
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "struct-type"},
            {"name", *(yyvsp[-6].sval)},
            {"base-structs", *(yyvsp[-5].expval)},
            {"fields", fields},
            {"methods", methods},
            {"friends", friendsArr}
        });
        delete (yyvsp[-6].sval); delete (yyvsp[-5].expval); delete (yyvsp[-2].expval);
    }
#line 2294 "parser.tab.cpp"
    break;

  case 41: /* $@8: %empty  */
#line 640 "parser.y"
                                                        {
        g_symbolTable.pushScope();
        g_structDepth++;
    }
#line 2303 "parser.tab.cpp"
    break;

  case 42: /* STATEMENT: TOK_CLASS TOK_IDENTIFIER BASE_LIST_OPT TOK_LBRACE $@8 F_STATEMENTS TOK_RBRACE TOK_SEMICOLON  */
#line 644 "parser.y"
                                          {  
        g_structDepth--;
        g_symbolTable.popScope();
        nlohmann::ordered_json fields = nlohmann::ordered_json::array();
        nlohmann::ordered_json methods = nlohmann::ordered_json::array();
        nlohmann::ordered_json friendsArr = nlohmann::ordered_json::array();
        splitMembers((yyvsp[-2].expval), fields, methods, friendsArr);
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "struct-type"},
            {"name", *(yyvsp[-6].sval)},
            {"base-structs", *(yyvsp[-5].expval)},
            {"fields", fields},
            {"methods", methods},
            {"friends", friendsArr}
        });
        delete (yyvsp[-6].sval); delete (yyvsp[-5].expval); delete (yyvsp[-2].expval);
    }
#line 2325 "parser.tab.cpp"
    break;

  case 43: /* $@9: %empty  */
#line 661 "parser.y"
                                          {
        g_symbolTable.pushScope();
        g_structDepth++;
    }
#line 2334 "parser.tab.cpp"
    break;

  case 44: /* STATEMENT: TOK_UNION TOK_IDENTIFIER TOK_LBRACE $@9 F_STATEMENTS TOK_RBRACE TOK_SEMICOLON  */
#line 665 "parser.y"
                                          {
        g_structDepth--;
        g_symbolTable.popScope();
        nlohmann::ordered_json fields = nlohmann::ordered_json::array();
        nlohmann::ordered_json methods = nlohmann::ordered_json::array();
        nlohmann::ordered_json friendsArr = nlohmann::ordered_json::array();
        splitMembers((yyvsp[-2].expval), fields, methods, friendsArr);
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "union-type"},
            {"name", *(yyvsp[-5].sval)},
            {"fields", fields},
            {"methods", methods},
            {"friends", friendsArr}
        });
        delete (yyvsp[-5].sval); delete (yyvsp[-2].expval);
    }
#line 2355 "parser.tab.cpp"
    break;

  case 45: /* STATEMENT: TOK_TYPEDEF TYPE_SPEC TOK_IDENTIFIER TOK_SEMICOLON  */
#line 681 "parser.y"
                                                         {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "typedef-type"},
            {"name", *(yyvsp[-1].sval)},
            {"old-name", *(yyvsp[-2].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[-1].sval);
    }
#line 2368 "parser.tab.cpp"
    break;

  case 46: /* CHOOSE_VARIANTS: %empty  */
#line 693 "parser.y"
     { (yyval.expval) = new nlohmann::ordered_json(nlohmann::ordered_json::array()); }
#line 2374 "parser.tab.cpp"
    break;

  case 47: /* CHOOSE_VARIANTS: CHOOSE_VARIANTS CHOOSE_VARIANT  */
#line 694 "parser.y"
                                     {
        (yyvsp[-1].expval)->push_back(*(yyvsp[0].expval));
        (yyval.expval) = (yyvsp[-1].expval);
        delete (yyvsp[0].expval);
    }
#line 2384 "parser.tab.cpp"
    break;

  case 48: /* CHOOSE_VARIANT: TOK_CASE expr TOK_COLON STATEMENTS  */
#line 703 "parser.y"
                                       {
        markLastReturn((yyvsp[0].expval));
        nlohmann::ordered_json body = {
            {"type", "compound-statement"},
            {"statements", *(yyvsp[0].expval)}
        };
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "choose-variant"},
            {"condition-label", *(yyvsp[-2].expval)},
            {"variant-body", body}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 2402 "parser.tab.cpp"
    break;

  case 49: /* CHOOSE_VARIANT: TOK_DEFAULT TOK_COLON STATEMENTS  */
#line 716 "parser.y"
                                       {
        markLastReturn((yyvsp[0].expval));
        nlohmann::ordered_json body = {
            {"type", "compound-statement"},
            {"statements", *(yyvsp[0].expval)}
        };
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "choose-variant"},
            {"condition-label", nullptr},
            {"variant-body", body}
        });
        delete (yyvsp[0].expval);
    }
#line 2420 "parser.tab.cpp"
    break;

  case 50: /* TYPE_SPEC: TOK_KW_INT  */
#line 733 "parser.y"
                  { (yyval.expval) = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","int"}}); }
#line 2426 "parser.tab.cpp"
    break;

  case 51: /* TYPE_SPEC: TOK_KW_DOUBLE  */
#line 734 "parser.y"
                    { (yyval.expval) = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","double"}}); }
#line 2432 "parser.tab.cpp"
    break;

  case 52: /* TYPE_SPEC: TOK_KW_FLOAT  */
#line 735 "parser.y"
                    { (yyval.expval) = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","float"}}); }
#line 2438 "parser.tab.cpp"
    break;

  case 53: /* TYPE_SPEC: TOK_KW_CHAR  */
#line 736 "parser.y"
                    { (yyval.expval) = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","char"}}); }
#line 2444 "parser.tab.cpp"
    break;

  case 54: /* TYPE_SPEC: TOK_KW_BOOL  */
#line 737 "parser.y"
                    { (yyval.expval) = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","bool"}}); }
#line 2450 "parser.tab.cpp"
    break;

  case 55: /* TYPE_SPEC: TOK_KW_VOID  */
#line 738 "parser.y"
                    { (yyval.expval) = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","void"}}); }
#line 2456 "parser.tab.cpp"
    break;

  case 56: /* TYPE_SPEC: TOK_KW_LONG  */
#line 739 "parser.y"
                    { (yyval.expval) = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","long"}}); }
#line 2462 "parser.tab.cpp"
    break;

  case 57: /* TYPE_SPEC: TOK_KW_SHORT  */
#line 740 "parser.y"
                    { (yyval.expval) = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","short"}}); }
#line 2468 "parser.tab.cpp"
    break;

  case 58: /* TYPE_SPEC: TOK_UNSIGNED  */
#line 742 "parser.y"
                   { (yyval.expval) = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","unsigned int"}}); }
#line 2474 "parser.tab.cpp"
    break;

  case 59: /* TYPE_SPEC: TOK_UNSIGNED TOK_KW_INT  */
#line 743 "parser.y"
                                { (yyval.expval) = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","unsigned int"}}); }
#line 2480 "parser.tab.cpp"
    break;

  case 60: /* TYPE_SPEC: TOK_UNSIGNED TOK_KW_CHAR  */
#line 744 "parser.y"
                                { (yyval.expval) = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","unsigned char"}}); }
#line 2486 "parser.tab.cpp"
    break;

  case 61: /* TYPE_SPEC: TOK_UNSIGNED TOK_KW_SHORT  */
#line 745 "parser.y"
                                { (yyval.expval) = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","unsigned short"}}); }
#line 2492 "parser.tab.cpp"
    break;

  case 62: /* TYPE_SPEC: TOK_UNSIGNED TOK_KW_LONG  */
#line 746 "parser.y"
                                { (yyval.expval) = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","unsigned long"}}); }
#line 2498 "parser.tab.cpp"
    break;

  case 63: /* TYPE_SPEC: TYPE_SPEC TOK_STAR  */
#line 748 "parser.y"
                         {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "pointer-type"},
            {"pointer-to", *(yyvsp[-1].expval)}
        });
        delete (yyvsp[-1].expval);
    }
#line 2510 "parser.tab.cpp"
    break;

  case 64: /* BASE_LIST_OPT: %empty  */
#line 759 "parser.y"
     { (yyval.expval) = new nlohmann::ordered_json(nlohmann::ordered_json::array()); }
#line 2516 "parser.tab.cpp"
    break;

  case 65: /* BASE_LIST_OPT: TOK_COLON BASE_LIST  */
#line 760 "parser.y"
                          { (yyval.expval) = (yyvsp[0].expval); }
#line 2522 "parser.tab.cpp"
    break;

  case 66: /* BASE_LIST: TOK_IDENTIFIER  */
#line 764 "parser.y"
                   {
        (yyval.expval) = new nlohmann::ordered_json(nlohmann::ordered_json::array());
        (yyval.expval)->push_back({{"type","builtin-type"},{"type-value",*(yyvsp[0].sval)}});
        delete (yyvsp[0].sval);
    }
#line 2532 "parser.tab.cpp"
    break;

  case 67: /* BASE_LIST: BASE_LIST TOK_COMMA TOK_IDENTIFIER  */
#line 769 "parser.y"
                                         {
        (yyvsp[-2].expval)->push_back({{"type","builtin-type"},{"type-value",*(yyvsp[0].sval)}});
        (yyval.expval) = (yyvsp[-2].expval);
        delete (yyvsp[0].sval);
    }
#line 2542 "parser.tab.cpp"
    break;

  case 68: /* ENUM_VALUE_LIST: ENUM_VALUE  */
#line 777 "parser.y"
               {
        (yyval.expval) = new nlohmann::ordered_json(nlohmann::ordered_json::array());
        (yyval.expval)->push_back(*(yyvsp[0].expval));
        delete (yyvsp[0].expval);
    }
#line 2552 "parser.tab.cpp"
    break;

  case 69: /* ENUM_VALUE_LIST: ENUM_VALUE_LIST TOK_COMMA ENUM_VALUE  */
#line 782 "parser.y"
                                           {
        (yyvsp[-2].expval)->push_back(*(yyvsp[0].expval));
        (yyval.expval) = (yyvsp[-2].expval);
        delete (yyvsp[0].expval);
    }
#line 2562 "parser.tab.cpp"
    break;

  case 70: /* ENUM_VALUE: TOK_IDENTIFIER  */
#line 790 "parser.y"
                   {
        g_symbolTable.declare(*(yyvsp[0].sval), {{"type","builtin-type"},{"type-value","int"}});
        (yyval.expval) = new nlohmann::ordered_json(*(yyvsp[0].sval));
        delete (yyvsp[0].sval);
    }
#line 2572 "parser.tab.cpp"
    break;

  case 71: /* ENUM_VALUE: TOK_IDENTIFIER TOK_ASSIGN assign_expr  */
#line 795 "parser.y"
                                            {
        g_symbolTable.declare(*(yyvsp[-2].sval), {{"type","builtin-type"},{"type-value","int"}});
        (yyval.expval) = new nlohmann::ordered_json(*(yyvsp[-2].sval));
        delete (yyvsp[-2].sval);
        delete (yyvsp[0].expval); 
    }
#line 2583 "parser.tab.cpp"
    break;

  case 72: /* PARAMS: %empty  */
#line 804 "parser.y"
     { (yyval.expval) = new nlohmann::ordered_json(nlohmann::ordered_json::array()); }
#line 2589 "parser.tab.cpp"
    break;

  case 73: /* PARAMS: PARAM_LIST  */
#line 805 "parser.y"
                 { (yyval.expval) = (yyvsp[0].expval); }
#line 2595 "parser.tab.cpp"
    break;

  case 74: /* PARAM_LIST: PARAM  */
#line 809 "parser.y"
          {
        (yyval.expval) = new nlohmann::ordered_json(nlohmann::ordered_json::array());
        (yyval.expval)->push_back(*(yyvsp[0].expval));
        delete (yyvsp[0].expval);
    }
#line 2605 "parser.tab.cpp"
    break;

  case 75: /* PARAM_LIST: PARAM_LIST TOK_COMMA PARAM  */
#line 814 "parser.y"
                                 {
        (yyvsp[-2].expval)->push_back(*(yyvsp[0].expval));
        (yyval.expval) = (yyvsp[-2].expval);
        delete (yyvsp[0].expval);
    }
#line 2615 "parser.tab.cpp"
    break;

  case 76: /* PARAM: TYPE_SPEC DECLARATOR  */
#line 822 "parser.y"
                         {
        stampDeclaratorType(*(yyvsp[0].expval), *(yyvsp[-1].expval));
        (yyval.expval) = (yyvsp[0].expval);
        delete (yyvsp[-1].expval);
    }
#line 2625 "parser.tab.cpp"
    break;

  case 77: /* DECLARATOR_LIST: DECLARATOR  */
#line 830 "parser.y"
               {
        (yyval.expval) = new nlohmann::ordered_json(nlohmann::ordered_json::array());
        (yyval.expval)->push_back(*(yyvsp[0].expval));
        delete (yyvsp[0].expval);
    }
#line 2635 "parser.tab.cpp"
    break;

  case 78: /* DECLARATOR_LIST: DECLARATOR_LIST TOK_COMMA DECLARATOR  */
#line 835 "parser.y"
                                           {
        (yyvsp[-2].expval)->push_back(*(yyvsp[0].expval));
        (yyval.expval) = (yyvsp[-2].expval);
        delete (yyvsp[0].expval);
    }
#line 2645 "parser.tab.cpp"
    break;

  case 79: /* DECLARATOR: TOK_IDENTIFIER  */
#line 843 "parser.y"
                   {    
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "declarator"},
            {"variable-name", *(yyvsp[0].sval)},
            {"reference", false},
            {"constructor-name", nullptr},
            {"constructor-arguments", nlohmann::ordered_json::array()}
        });
        delete (yyvsp[0].sval);
    }
#line 2660 "parser.tab.cpp"
    break;

  case 80: /* DECLARATOR: TOK_AMP TOK_IDENTIFIER  */
#line 853 "parser.y"
                             {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "declarator"},
            {"variable-name", *(yyvsp[0].sval)},
            {"reference", true},
            {"constructor-name", nullptr},
            {"constructor-arguments", nlohmann::ordered_json::array()}
        });
        delete (yyvsp[0].sval);
    }
#line 2675 "parser.tab.cpp"
    break;

  case 81: /* DECLARATOR: TOK_IDENTIFIER TOK_ASSIGN assign_expr  */
#line 863 "parser.y"
                                            {      
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "declarator"},
            {"variable-name", *(yyvsp[-2].sval)},
            {"reference", false},
            {"constructor-name", nullptr},
            {"constructor-arguments", nlohmann::ordered_json::array({*(yyvsp[0].expval)})}
        });
        delete (yyvsp[-2].sval); delete (yyvsp[0].expval);
    }
#line 2690 "parser.tab.cpp"
    break;

  case 82: /* DECLARATOR: TOK_IDENTIFIER INDEX_SUFFIX_LIST  */
#line 874 "parser.y"
                                       {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "declarator"},
            {"variable-name", *(yyvsp[-1].sval)},
            {"reference", false},
            {"constructor-name", nullptr},
            {"constructor-arguments", nlohmann::ordered_json::array()},
            {"__index_ranges", *(yyvsp[0].expval)}
        });
        delete (yyvsp[-1].sval); delete (yyvsp[0].expval);
    }
#line 2706 "parser.tab.cpp"
    break;

  case 83: /* STATIC_OPT: %empty  */
#line 888 "parser.y"
     { (yyval.ival) = 0; }
#line 2712 "parser.tab.cpp"
    break;

  case 84: /* STATIC_OPT: TOK_STATIC  */
#line 889 "parser.y"
                 { (yyval.ival) = 1; }
#line 2718 "parser.tab.cpp"
    break;

  case 85: /* CONST_OPT: %empty  */
#line 893 "parser.y"
     { (yyval.ival) = 0; }
#line 2724 "parser.tab.cpp"
    break;

  case 86: /* CONST_OPT: TOK_CONST  */
#line 894 "parser.y"
                { (yyval.ival) = 1; }
#line 2730 "parser.tab.cpp"
    break;

  case 87: /* FOR_INIT: expr TOK_SEMICOLON  */
#line 898 "parser.y"
                       {
        (yyval.expval) = new nlohmann::ordered_json({
            {"loop-init-node", *(yyvsp[-1].expval)},
            {"loop-init-name", nullptr}
        });
        delete (yyvsp[-1].expval);
    }
#line 2742 "parser.tab.cpp"
    break;

  case 88: /* FOR_INIT: STATIC_OPT CONST_OPT TYPE_SPEC DECLARATOR_LIST TOK_SEMICOLON  */
#line 905 "parser.y"
                                                                   {
        nlohmann::ordered_json *decl = buildVarDecl(false, (yyvsp[-4].ival), (yyvsp[-3].ival), (yyvsp[-2].expval), (yyvsp[-1].expval));
        nlohmann::ordered_json name = nullptr;
        if (!decl->at("declarators").empty()) {
            name = decl->at("declarators")[0].value("variable-name", "");
        }
        (yyval.expval) = new nlohmann::ordered_json({
            {"loop-init-node", *decl},
            {"loop-init-name", name}
        });
        delete decl;
    }
#line 2759 "parser.tab.cpp"
    break;

  case 89: /* expr: assign_expr  */
#line 920 "parser.y"
                { (yyval.expval) = (yyvsp[0].expval); }
#line 2765 "parser.tab.cpp"
    break;

  case 90: /* expr: expr TOK_COMMA assign_expr  */
#line 921 "parser.y"
                                 {   
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "sequence"},
            {"result-type", inferSimpleType((yyvsp[0].expval))},
            {"left", *(yyvsp[-2].expval)},
            {"right", *(yyvsp[0].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 2779 "parser.tab.cpp"
    break;

  case 91: /* assign_expr: TOK_INTEGER_LITERAL  */
#line 933 "parser.y"
                        {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "literal-constant"},
            {"constant-type", "integer"},
            {"constant-value", (yyvsp[0].ival)}
        });
    }
#line 2791 "parser.tab.cpp"
    break;

  case 92: /* assign_expr: TOK_REAL_LITERAL  */
#line 940 "parser.y"
                       {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "literal-constant"},
            {"constant-type", "real"},
            {"constant-value", (yyvsp[0].dval)}
        });
    }
#line 2803 "parser.tab.cpp"
    break;

  case 93: /* assign_expr: TOK_STRING_LITERAL  */
#line 947 "parser.y"
                         {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "literal-constant"},
            {"constant-type", "string"},
            {"constant-value", *(yyvsp[0].sval)}
        });
        delete (yyvsp[0].sval);
    }
#line 2816 "parser.tab.cpp"
    break;

  case 94: /* assign_expr: TOK_IDENTIFIER  */
#line 955 "parser.y"
                     {      
        if (!g_symbolTable.isDeclared(*(yyvsp[0].sval)))
            reportSemanticError("use of undeclared identifier '" + *(yyvsp[0].sval) + "'");
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "variable-reference"},
            {"variable-name", *(yyvsp[0].sval)},
            {"variable-type", g_symbolTable.lookupTypeValue(*(yyvsp[0].sval))},
            {"variable-usage", "rvalue"}
        });
        delete (yyvsp[0].sval);
    }
#line 2832 "parser.tab.cpp"
    break;

  case 95: /* assign_expr: TOK_IDENTIFIER TOK_LPAREN ARG_LIST_OPT TOK_RPAREN  */
#line 966 "parser.y"
                                                        {
        std::string resultType = "unknown";
        auto found = g_functionReturnTypes.find(*(yyvsp[-3].sval));
        if (found != g_functionReturnTypes.end()) {
            resultType = typeJsonToName(found->second);
        } else {
            reportSemanticError("call to undeclared function '" + *(yyvsp[-3].sval) + "'");
        }
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "function-call"},
            {"function-name", *(yyvsp[-3].sval)},
            {"result-type", resultType},
            {"arguments", *(yyvsp[-1].expval)}
        });
        delete (yyvsp[-3].sval); delete (yyvsp[-1].expval);
    }
#line 2853 "parser.tab.cpp"
    break;

  case 96: /* assign_expr: TOK_LPAREN expr TOK_RPAREN  */
#line 982 "parser.y"
                                 {       
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "parentheses"},
            {"result-type", inferSimpleType((yyvsp[-1].expval))},
            {"argument", *(yyvsp[-1].expval)}
        });
        delete (yyvsp[-1].expval);
    }
#line 2866 "parser.tab.cpp"
    break;

  case 97: /* assign_expr: TOK_LPAREN TYPE_SPEC TOK_RPAREN assign_expr  */
#line 990 "parser.y"
                                                                   {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "type-cast"},
            {"result-type", *(yyvsp[-2].expval)},
            {"argument", *(yyvsp[0].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 2879 "parser.tab.cpp"
    break;

  case 98: /* assign_expr: assign_expr INDEX_SUFFIX_LIST  */
#line 998 "parser.y"
                                                       {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "index"},
            {"array-expression", *(yyvsp[-1].expval)},           
            {"result-type", inferSimpleType((yyvsp[-1].expval))},
            {"indices", *(yyvsp[0].expval)}
        });
        delete (yyvsp[-1].expval); delete (yyvsp[0].expval);
    }
#line 2893 "parser.tab.cpp"
    break;

  case 99: /* assign_expr: assign_expr TOK_PLUS assign_expr  */
#line 1007 "parser.y"
                                       {
        if ((*(yyvsp[-2].expval))["type"] == "literal-constant" && (*(yyvsp[-2].expval))["constant-type"] == "string" &&
            (*(yyvsp[0].expval))["type"] == "literal-constant" && (*(yyvsp[0].expval))["constant-type"] == "string") {
            (yyval.expval) = new nlohmann::ordered_json({
                {"type", "string-concatenation"},
                {"result-type", "string"},
                {"left", *(yyvsp[-2].expval)}, {"right", *(yyvsp[0].expval)}
            });
        } else {
            (yyval.expval) = new nlohmann::ordered_json({
                {"type", "sum"},
                {"result-type", promoteArithmeticType(inferOperandType((yyvsp[-2].expval)), inferOperandType((yyvsp[0].expval)), '+')},
                {"left", *(yyvsp[-2].expval)}, {"right", *(yyvsp[0].expval)}
            });
        }
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 2915 "parser.tab.cpp"
    break;

  case 100: /* assign_expr: assign_expr TOK_MINUS assign_expr  */
#line 1024 "parser.y"
                                        {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "difference"},
            {"result-type", promoteArithmeticType(inferOperandType((yyvsp[-2].expval)), inferOperandType((yyvsp[0].expval)), '-')},
            {"left", *(yyvsp[-2].expval)}, {"right", *(yyvsp[0].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 2928 "parser.tab.cpp"
    break;

  case 101: /* assign_expr: assign_expr TOK_STAR assign_expr  */
#line 1032 "parser.y"
                                       {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "product"},
            {"result-type", promoteArithmeticType(inferOperandType((yyvsp[-2].expval)), inferOperandType((yyvsp[0].expval)), '*')},
            {"left", *(yyvsp[-2].expval)}, {"right", *(yyvsp[0].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 2941 "parser.tab.cpp"
    break;

  case 102: /* assign_expr: assign_expr TOK_SLASH assign_expr  */
#line 1040 "parser.y"
                                        {
        std::string leftType = inferOperandType((yyvsp[-2].expval));
        std::string rightType = inferOperandType((yyvsp[0].expval));
        std::string nodeType =
            (isIntegerTypeName(leftType) && isIntegerTypeName(rightType))
                ? "integer-quotient" : "quotient";
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", nodeType},
            {"result-type", promoteArithmeticType(leftType, rightType, '/')},
            {"left", *(yyvsp[-2].expval)}, {"right", *(yyvsp[0].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 2959 "parser.tab.cpp"
    break;

  case 103: /* assign_expr: assign_expr TOK_PERCENT assign_expr  */
#line 1053 "parser.y"
                                          {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "modulo"},
            {"result-type", computeModuloType(inferOperandType((yyvsp[-2].expval)), inferOperandType((yyvsp[0].expval)))},
            {"left", *(yyvsp[-2].expval)}, {"right", *(yyvsp[0].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 2972 "parser.tab.cpp"
    break;

  case 104: /* assign_expr: TOK_INCREMENT assign_expr  */
#line 1061 "parser.y"
                                                 {
        markLvalueIfVariableRef(*(yyvsp[0].expval));
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "prefix-increment"},
            {"result-type", inferSimpleType((yyvsp[0].expval))},
            {"argument", *(yyvsp[0].expval)}
        });
        delete (yyvsp[0].expval);
    }
#line 2986 "parser.tab.cpp"
    break;

  case 105: /* assign_expr: assign_expr TOK_INCREMENT  */
#line 1070 "parser.y"
                                {
        markLvalueIfVariableRef(*(yyvsp[-1].expval));
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "postfix-increment"},
            {"result-type", inferSimpleType((yyvsp[-1].expval))},
            {"argument", *(yyvsp[-1].expval)}
        });
        delete (yyvsp[-1].expval);
    }
#line 3000 "parser.tab.cpp"
    break;

  case 106: /* assign_expr: TOK_DECREMENT assign_expr  */
#line 1079 "parser.y"
                                                 {
        markLvalueIfVariableRef(*(yyvsp[0].expval));
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "prefix-decrement"},
            {"result-type", inferSimpleType((yyvsp[0].expval))},
            {"argument", *(yyvsp[0].expval)}
        });
        delete (yyvsp[0].expval);
    }
#line 3014 "parser.tab.cpp"
    break;

  case 107: /* assign_expr: assign_expr TOK_DECREMENT  */
#line 1088 "parser.y"
                                {
        markLvalueIfVariableRef(*(yyvsp[-1].expval));
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "postfix-decrement"},
            {"result-type", inferSimpleType((yyvsp[-1].expval))},
            {"argument", *(yyvsp[-1].expval)}
        });
        delete (yyvsp[-1].expval);
    }
#line 3028 "parser.tab.cpp"
    break;

  case 108: /* assign_expr: TOK_MINUS assign_expr  */
#line 1097 "parser.y"
                                             {
        nlohmann::ordered_json zero = {
            {"type", "literal-constant"},
            {"constant-type", "integer"},
            {"constant-value", 0}
        };
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", "difference"},
            {"result-type", promoteArithmeticType("int", inferSimpleType((yyvsp[0].expval)), '-')},
            {"left", zero},
            {"right", *(yyvsp[0].expval)}
        });
        delete (yyvsp[0].expval);
    }
#line 3047 "parser.tab.cpp"
    break;

  case 109: /* assign_expr: TOK_PLUS assign_expr  */
#line 1111 "parser.y"
                                            {
        (yyval.expval) = (yyvsp[0].expval);
    }
#line 3055 "parser.tab.cpp"
    break;

  case 110: /* assign_expr: assign_expr TOK_EQ assign_expr  */
#line 1114 "parser.y"
                                     {
        (yyval.expval) = new nlohmann::ordered_json({{"type","equals"},{"result-type","boolean"},{"left",*(yyvsp[-2].expval)},{"right",*(yyvsp[0].expval)}});
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3064 "parser.tab.cpp"
    break;

  case 111: /* assign_expr: assign_expr TOK_LT assign_expr  */
#line 1118 "parser.y"
                                     {
        (yyval.expval) = new nlohmann::ordered_json({{"type","less"},{"result-type","boolean"},{"left",*(yyvsp[-2].expval)},{"right",*(yyvsp[0].expval)}});
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3073 "parser.tab.cpp"
    break;

  case 112: /* assign_expr: assign_expr TOK_LE assign_expr  */
#line 1122 "parser.y"
                                     {
        (yyval.expval) = new nlohmann::ordered_json({{"type","less-or-equal"},{"result-type","boolean"},{"left",*(yyvsp[-2].expval)},{"right",*(yyvsp[0].expval)}});
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3082 "parser.tab.cpp"
    break;

  case 113: /* assign_expr: assign_expr TOK_GT assign_expr  */
#line 1126 "parser.y"
                                     {
        (yyval.expval) = new nlohmann::ordered_json({{"type","greater"},{"result-type","boolean"},{"left",*(yyvsp[-2].expval)},{"right",*(yyvsp[0].expval)}});
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3091 "parser.tab.cpp"
    break;

  case 114: /* assign_expr: assign_expr TOK_GE assign_expr  */
#line 1130 "parser.y"
                                     {
        (yyval.expval) = new nlohmann::ordered_json({{"type","greater-or-equal"},{"result-type","boolean"},{"left",*(yyvsp[-2].expval)},{"right",*(yyvsp[0].expval)}});
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3100 "parser.tab.cpp"
    break;

  case 115: /* assign_expr: assign_expr TOK_NE assign_expr  */
#line 1134 "parser.y"
                                     {
        (yyval.expval) = new nlohmann::ordered_json({{"type","not-equal"},{"result-type","boolean"},{"left",*(yyvsp[-2].expval)},{"right",*(yyvsp[0].expval)}});
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3109 "parser.tab.cpp"
    break;

  case 116: /* assign_expr: assign_expr TOK_LOGICAL_AND assign_expr  */
#line 1138 "parser.y"
                                              {
        (yyval.expval) = new nlohmann::ordered_json({{"type","logical-and"},{"result-type","boolean"},{"left",*(yyvsp[-2].expval)},{"right",*(yyvsp[0].expval)}});
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3118 "parser.tab.cpp"
    break;

  case 117: /* assign_expr: assign_expr TOK_LOGICAL_OR assign_expr  */
#line 1142 "parser.y"
                                             {
        (yyval.expval) = new nlohmann::ordered_json({{"type","logical-or"},{"result-type","boolean"},{"left",*(yyvsp[-2].expval)},{"right",*(yyvsp[0].expval)}});
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3127 "parser.tab.cpp"
    break;

  case 118: /* assign_expr: TOK_LOGICAL_NOT assign_expr  */
#line 1146 "parser.y"
                                                   {
        (yyval.expval) = new nlohmann::ordered_json({{"type","logical-not"},{"result-type","boolean"},{"argument",*(yyvsp[0].expval)}});
        delete (yyvsp[0].expval);
    }
#line 3136 "parser.tab.cpp"
    break;

  case 119: /* assign_expr: assign_expr TOK_AMP assign_expr  */
#line 1150 "parser.y"
                                      {
        (yyval.expval) = new nlohmann::ordered_json({{"type","bitwise-and"},{"result-type","int"},{"left",*(yyvsp[-2].expval)},{"right",*(yyvsp[0].expval)}});
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3145 "parser.tab.cpp"
    break;

  case 120: /* assign_expr: assign_expr TOK_PIPE assign_expr  */
#line 1154 "parser.y"
                                       {
        (yyval.expval) = new nlohmann::ordered_json({{"type","bitwise-or"},{"result-type","int"},{"left",*(yyvsp[-2].expval)},{"right",*(yyvsp[0].expval)}});
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3154 "parser.tab.cpp"
    break;

  case 121: /* assign_expr: assign_expr TOK_CARET assign_expr  */
#line 1158 "parser.y"
                                        {
        (yyval.expval) = new nlohmann::ordered_json({{"type","bitwise-xor"},{"result-type","int"},{"left",*(yyvsp[-2].expval)},{"right",*(yyvsp[0].expval)}});
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3163 "parser.tab.cpp"
    break;

  case 122: /* assign_expr: TOK_TILDE assign_expr  */
#line 1162 "parser.y"
                                             {
        (yyval.expval) = new nlohmann::ordered_json({{"type","bitwise-not"},{"result-type","int"},{"argument",*(yyvsp[0].expval)}});
        delete (yyvsp[0].expval);
    }
#line 3172 "parser.tab.cpp"
    break;

  case 123: /* assign_expr: assign_expr TOK_SHR assign_expr  */
#line 1166 "parser.y"
                                      {       
        std::string leftType = inferSimpleType((yyvsp[-2].expval));
        std::string nodeType = isUnsignedTypeName(leftType)
            ? "logic-bitwise-shift-right" : "arithmetic-bitwise-shift-right";
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", nodeType},{"result-type","int"},
            {"amount",*(yyvsp[0].expval)},{"argument",*(yyvsp[-2].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3187 "parser.tab.cpp"
    break;

  case 124: /* assign_expr: assign_expr TOK_SHL assign_expr  */
#line 1176 "parser.y"
                                      {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type","bitwise-shift-left"},{"result-type","int"},
            {"amount",*(yyvsp[0].expval)},{"argument",*(yyvsp[-2].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3199 "parser.tab.cpp"
    break;

  case 125: /* assign_expr: assign_expr TOK_ASSIGN assign_expr  */
#line 1183 "parser.y"
                                         {
        markLvalueIfVariableRef(*(yyvsp[-2].expval));
        (yyval.expval) = new nlohmann::ordered_json({
            {"type","assignment"},{"assignment-type",inferSimpleType((yyvsp[-2].expval))},
            {"variable",*(yyvsp[-2].expval)},{"expression",*(yyvsp[0].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3212 "parser.tab.cpp"
    break;

  case 126: /* assign_expr: assign_expr TOK_PLUS_ASSIGN assign_expr  */
#line 1191 "parser.y"
                                              {
        markLvalueIfVariableRef(*(yyvsp[-2].expval));
        (yyval.expval) = new nlohmann::ordered_json({
            {"type","assignment-sum"},{"assignment-type",inferSimpleType((yyvsp[-2].expval))},
            {"variable",*(yyvsp[-2].expval)},{"expression",*(yyvsp[0].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3225 "parser.tab.cpp"
    break;

  case 127: /* assign_expr: assign_expr TOK_MINUS_ASSIGN assign_expr  */
#line 1199 "parser.y"
                                               {
        markLvalueIfVariableRef(*(yyvsp[-2].expval));
        (yyval.expval) = new nlohmann::ordered_json({
            {"type","assignment-difference"},{"assignment-type",inferSimpleType((yyvsp[-2].expval))},
            {"variable",*(yyvsp[-2].expval)},{"expression",*(yyvsp[0].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3238 "parser.tab.cpp"
    break;

  case 128: /* assign_expr: assign_expr TOK_STAR_ASSIGN assign_expr  */
#line 1207 "parser.y"
                                              {
        markLvalueIfVariableRef(*(yyvsp[-2].expval));
        (yyval.expval) = new nlohmann::ordered_json({
            {"type","assignment-product"},{"assignment-type",inferSimpleType((yyvsp[-2].expval))},
            {"variable",*(yyvsp[-2].expval)},{"expression",*(yyvsp[0].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3251 "parser.tab.cpp"
    break;

  case 129: /* assign_expr: assign_expr TOK_SLASH_ASSIGN assign_expr  */
#line 1215 "parser.y"
                                               {
        markLvalueIfVariableRef(*(yyvsp[-2].expval));
        std::string leftType = inferSimpleType((yyvsp[-2].expval));
        std::string rightType = inferSimpleType((yyvsp[0].expval));
        std::string nodeType =
            (isIntegerTypeName(leftType) && isIntegerTypeName(rightType))
                ? "assignment-integer-quotient" : "assignment-quotient";
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", nodeType}, {"assignment-type",leftType},
            {"variable",*(yyvsp[-2].expval)},{"expression",*(yyvsp[0].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3269 "parser.tab.cpp"
    break;

  case 130: /* assign_expr: assign_expr TOK_PERCENT_ASSIGN assign_expr  */
#line 1228 "parser.y"
                                                 {
        markLvalueIfVariableRef(*(yyvsp[-2].expval));
        std::string leftType = inferSimpleType((yyvsp[-2].expval));
        std::string rightType = inferSimpleType((yyvsp[0].expval));       
        computeModuloType(leftType, rightType);
        (yyval.expval) = new nlohmann::ordered_json({
            {"type","assignment-modulo"},{"assignment-type",leftType},
            {"variable",*(yyvsp[-2].expval)},{"expression",*(yyvsp[0].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3285 "parser.tab.cpp"
    break;

  case 131: /* assign_expr: assign_expr TOK_AMP_ASSIGN assign_expr  */
#line 1239 "parser.y"
                                             {
        markLvalueIfVariableRef(*(yyvsp[-2].expval));
        (yyval.expval) = new nlohmann::ordered_json({
            {"type","assignment-bitwise-and"},{"result-type","int"},
            {"left",*(yyvsp[-2].expval)},{"right",*(yyvsp[0].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3298 "parser.tab.cpp"
    break;

  case 132: /* assign_expr: assign_expr TOK_PIPE_ASSIGN assign_expr  */
#line 1247 "parser.y"
                                              {
        markLvalueIfVariableRef(*(yyvsp[-2].expval));
        (yyval.expval) = new nlohmann::ordered_json({
            {"type","assignment-bitwise-or"},{"result-type","int"},
            {"left",*(yyvsp[-2].expval)},{"right",*(yyvsp[0].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3311 "parser.tab.cpp"
    break;

  case 133: /* assign_expr: assign_expr TOK_CARET_ASSIGN assign_expr  */
#line 1255 "parser.y"
                                               {
        markLvalueIfVariableRef(*(yyvsp[-2].expval));
        (yyval.expval) = new nlohmann::ordered_json({
            {"type","assignment-bitwise-xor"},{"result-type","int"},
            {"left",*(yyvsp[-2].expval)},{"right",*(yyvsp[0].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3324 "parser.tab.cpp"
    break;

  case 134: /* assign_expr: assign_expr TOK_SHR_ASSIGN assign_expr  */
#line 1263 "parser.y"
                                             {
        markLvalueIfVariableRef(*(yyvsp[-2].expval));
        std::string leftType = inferSimpleType((yyvsp[-2].expval));
        std::string nodeType = isUnsignedTypeName(leftType)
            ? "assignment-logic-bitwise-shift-right" : "assignment-arithmetic-bitwise-shift-right";
        (yyval.expval) = new nlohmann::ordered_json({
            {"type", nodeType},{"result-type","int"},
            {"amount",*(yyvsp[0].expval)},{"argument",*(yyvsp[-2].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3340 "parser.tab.cpp"
    break;

  case 135: /* assign_expr: assign_expr TOK_SHL_ASSIGN assign_expr  */
#line 1274 "parser.y"
                                             {
        markLvalueIfVariableRef(*(yyvsp[-2].expval));
        (yyval.expval) = new nlohmann::ordered_json({
            {"type","assignment-bitwise-shift-left"},{"result-type","int"},
            {"amount",*(yyvsp[0].expval)},{"argument",*(yyvsp[-2].expval)}
        });
        delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3353 "parser.tab.cpp"
    break;

  case 136: /* assign_expr: TOK_STAR assign_expr  */
#line 1282 "parser.y"
                                            {
        std::string argType = inferSimpleType((yyvsp[0].expval));
        std::string resultType = "unknown";
        bool argIsArray = ((yyvsp[0].expval)->value("type","") == "variable-reference") &&
                          g_symbolTable.isArray((yyvsp[0].expval)->value("variable-name",""));
        if (!argType.empty() && argType.back() == '*') {
            resultType = argType.substr(0, argType.size() - 1);   
        } else if (argIsArray) {
            resultType = argType;   
        } else if (isIntegerTypeName(argType) || isRealTypeName(argType)) {
            reportSemanticError("cannot dereference non-pointer type '" + argType + "'");
        }
        (yyval.expval) = new nlohmann::ordered_json({{"type","dereference"},{"result-type",resultType},{"argument",*(yyvsp[0].expval)}});
        delete (yyvsp[0].expval);
    }
#line 3373 "parser.tab.cpp"
    break;

  case 137: /* assign_expr: TOK_AMP assign_expr  */
#line 1297 "parser.y"
                                           {
        markLvalueIfVariableRef(*(yyvsp[0].expval));
        std::string argType = inferSimpleType((yyvsp[0].expval));
        (yyval.expval) = new nlohmann::ordered_json({{"type","address"},
            {"result-type", argType == "unknown" ? "unknown" : argType + "*"},{"argument",*(yyvsp[0].expval)}});
        delete (yyvsp[0].expval);
    }
#line 3385 "parser.tab.cpp"
    break;

  case 138: /* assign_expr: TOK_NEW TYPE_SPEC TOK_LBRACKET expr TOK_RBRACKET  */
#line 1304 "parser.y"
                                                                        {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type","dynamic-memory-allocation"},{"array-flag",true},
            {"address-hint",nullptr},{"element-type",*(yyvsp[-3].expval)},
            {"amount-of-elements",*(yyvsp[-1].expval)},{"constructor-name",nullptr},
            {"constructor-arguments",nlohmann::ordered_json::array()}
        });
        delete (yyvsp[-3].expval); delete (yyvsp[-1].expval);
    }
#line 3399 "parser.tab.cpp"
    break;

  case 139: /* assign_expr: TOK_NEW TYPE_SPEC  */
#line 1313 "parser.y"
                                         {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type","dynamic-memory-allocation"},{"array-flag",false},
            {"address-hint",nullptr},{"variable-type",*(yyvsp[0].expval)},
            {"constructor-name",nullptr},{"constructor-arguments",nlohmann::ordered_json::array()}
        });
        delete (yyvsp[0].expval);
    }
#line 3412 "parser.tab.cpp"
    break;

  case 140: /* assign_expr: TOK_NEW TYPE_SPEC TOK_LPAREN ARG_LIST_OPT TOK_RPAREN  */
#line 1321 "parser.y"
                                                                            {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type","dynamic-memory-allocation"},{"array-flag",false},
            {"address-hint",nullptr},{"variable-type",*(yyvsp[-3].expval)},
            {"constructor-name",nullptr},{"constructor-arguments",*(yyvsp[-1].expval)}
        });
        delete (yyvsp[-3].expval); delete (yyvsp[-1].expval);
    }
#line 3425 "parser.tab.cpp"
    break;

  case 141: /* assign_expr: TOK_DELETE TOK_LBRACKET TOK_RBRACKET assign_expr  */
#line 1329 "parser.y"
                                                                        {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type","dynamic-memory-deallocation"},{"array-flag",true},
            {"element-type", nameToTypeJson(pointeeName(inferSimpleType((yyvsp[0].expval))))},
            {"amount-of-elements",nullptr}, 
            {"pointer",*(yyvsp[0].expval)},{"destructor-name",nullptr},
            {"destructor-arguments",nlohmann::ordered_json::array()}
        });
        delete (yyvsp[0].expval);
    }
#line 3440 "parser.tab.cpp"
    break;

  case 142: /* assign_expr: TOK_DELETE assign_expr  */
#line 1339 "parser.y"
                                              {       
        (yyval.expval) = new nlohmann::ordered_json({
            {"type","dynamic-memory-deallocation"},{"array-flag",false},
            {"variable-type", nameToTypeJson(pointeeName(inferSimpleType((yyvsp[0].expval))))},{"pointer",*(yyvsp[0].expval)},
            {"destructor-name",nullptr},{"destructor-arguments",nlohmann::ordered_json::array()}
        });
        delete (yyvsp[0].expval);
    }
#line 3453 "parser.tab.cpp"
    break;

  case 143: /* assign_expr: assign_expr TOK_QUESTION expr TOK_COLON assign_expr  */
#line 1347 "parser.y"
                                                                             {
        (yyval.expval) = new nlohmann::ordered_json({
            {"type","conditional-expression"},
            {"result-type", unifyBranchTypes(inferSimpleType((yyvsp[-2].expval)), inferSimpleType((yyvsp[0].expval)), (yyvsp[-2].expval), (yyvsp[0].expval))},
            {"condition",*(yyvsp[-4].expval)},{"if-true",*(yyvsp[-2].expval)},{"if-false",*(yyvsp[0].expval)}
        });
        delete (yyvsp[-4].expval); delete (yyvsp[-2].expval); delete (yyvsp[0].expval);
    }
#line 3466 "parser.tab.cpp"
    break;

  case 144: /* INDEX_SUFFIX_LIST: TOK_LBRACKET expr TOK_RBRACKET  */
#line 1359 "parser.y"
                                   {
        (yyval.expval) = new nlohmann::ordered_json(nlohmann::ordered_json::array());
        (yyval.expval)->push_back(*(yyvsp[-1].expval));
        delete (yyvsp[-1].expval);
    }
#line 3476 "parser.tab.cpp"
    break;

  case 145: /* INDEX_SUFFIX_LIST: INDEX_SUFFIX_LIST TOK_LBRACKET expr TOK_RBRACKET  */
#line 1364 "parser.y"
                                                       {
        (yyvsp[-3].expval)->push_back(*(yyvsp[-1].expval));
        (yyval.expval) = (yyvsp[-3].expval);
        delete (yyvsp[-1].expval);
    }
#line 3486 "parser.tab.cpp"
    break;

  case 146: /* ARG_LIST_OPT: %empty  */
#line 1373 "parser.y"
     { (yyval.expval) = new nlohmann::ordered_json(nlohmann::ordered_json::array()); }
#line 3492 "parser.tab.cpp"
    break;

  case 147: /* ARG_LIST_OPT: ARG_LIST  */
#line 1374 "parser.y"
               { (yyval.expval) = (yyvsp[0].expval); }
#line 3498 "parser.tab.cpp"
    break;

  case 148: /* ARG_LIST: assign_expr  */
#line 1378 "parser.y"
                {
        (yyval.expval) = new nlohmann::ordered_json(nlohmann::ordered_json::array());
        (yyval.expval)->push_back(*(yyvsp[0].expval));
        delete (yyvsp[0].expval);
    }
#line 3508 "parser.tab.cpp"
    break;

  case 149: /* ARG_LIST: ARG_LIST TOK_COMMA assign_expr  */
#line 1383 "parser.y"
                                     {
        (yyvsp[-2].expval)->push_back(*(yyvsp[0].expval));
        (yyval.expval) = (yyvsp[-2].expval);
        delete (yyvsp[0].expval);
    }
#line 3518 "parser.tab.cpp"
    break;


#line 3522 "parser.tab.cpp"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 1390 "parser.y"


void yyerror(const char *msg) {
    std::fprintf(stderr, "Parse error at line %d: %s\n", yylineno, msg);
}
