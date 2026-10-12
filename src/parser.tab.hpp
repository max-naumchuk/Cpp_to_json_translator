/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_PARSER_TAB_HPP_INCLUDED
# define YY_YY_PARSER_TAB_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif
/* "%code requires" blocks.  */
#line 1 "parser.y"

#include <string>
#include <nlohmann/json.hpp>

#line 54 "parser.tab.hpp"

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    TOK_INTEGER_LITERAL = 258,     /* TOK_INTEGER_LITERAL  */
    TOK_REAL_LITERAL = 259,        /* TOK_REAL_LITERAL  */
    TOK_STRING_LITERAL = 260,      /* TOK_STRING_LITERAL  */
    TOK_IDENTIFIER = 261,          /* TOK_IDENTIFIER  */
    TOK_BREAK = 262,               /* TOK_BREAK  */
    TOK_CASE = 263,                /* TOK_CASE  */
    TOK_CLASS = 264,               /* TOK_CLASS  */
    TOK_CONST = 265,               /* TOK_CONST  */
    TOK_CONTINUE = 266,            /* TOK_CONTINUE  */
    TOK_DEFAULT = 267,             /* TOK_DEFAULT  */
    TOK_DELETE = 268,              /* TOK_DELETE  */
    TOK_DO = 269,                  /* TOK_DO  */
    TOK_ELSE = 270,                /* TOK_ELSE  */
    TOK_ENUM = 271,                /* TOK_ENUM  */
    TOK_FOR = 272,                 /* TOK_FOR  */
    TOK_FRIEND = 273,              /* TOK_FRIEND  */
    TOK_GOTO = 274,                /* TOK_GOTO  */
    TOK_IF = 275,                  /* TOK_IF  */
    TOK_NEW = 276,                 /* TOK_NEW  */
    TOK_RETURN = 277,              /* TOK_RETURN  */
    TOK_STATIC = 278,              /* TOK_STATIC  */
    TOK_STATIC_CAST = 279,         /* TOK_STATIC_CAST  */
    TOK_STRUCT = 280,              /* TOK_STRUCT  */
    TOK_SWITCH = 281,              /* TOK_SWITCH  */
    TOK_TYPEDEF = 282,             /* TOK_TYPEDEF  */
    TOK_UNION = 283,               /* TOK_UNION  */
    TOK_WHILE = 284,               /* TOK_WHILE  */
    TOK_KW_INT = 285,              /* TOK_KW_INT  */
    TOK_KW_DOUBLE = 286,           /* TOK_KW_DOUBLE  */
    TOK_KW_FLOAT = 287,            /* TOK_KW_FLOAT  */
    TOK_KW_CHAR = 288,             /* TOK_KW_CHAR  */
    TOK_KW_BOOL = 289,             /* TOK_KW_BOOL  */
    TOK_KW_VOID = 290,             /* TOK_KW_VOID  */
    TOK_KW_LONG = 291,             /* TOK_KW_LONG  */
    TOK_KW_SHORT = 292,            /* TOK_KW_SHORT  */
    TOK_UNSIGNED = 293,            /* TOK_UNSIGNED  */
    TOK_LPAREN = 294,              /* TOK_LPAREN  */
    TOK_RPAREN = 295,              /* TOK_RPAREN  */
    TOK_LBRACE = 296,              /* TOK_LBRACE  */
    TOK_RBRACE = 297,              /* TOK_RBRACE  */
    TOK_LBRACKET = 298,            /* TOK_LBRACKET  */
    TOK_RBRACKET = 299,            /* TOK_RBRACKET  */
    TOK_SEMICOLON = 300,           /* TOK_SEMICOLON  */
    TOK_COLON = 301,               /* TOK_COLON  */
    TOK_COMMA = 302,               /* TOK_COMMA  */
    TOK_QUESTION = 303,            /* TOK_QUESTION  */
    TOK_ASSIGN = 304,              /* TOK_ASSIGN  */
    TOK_EQ = 305,                  /* TOK_EQ  */
    TOK_PLUS = 306,                /* TOK_PLUS  */
    TOK_PLUS_ASSIGN = 307,         /* TOK_PLUS_ASSIGN  */
    TOK_INCREMENT = 308,           /* TOK_INCREMENT  */
    TOK_MINUS = 309,               /* TOK_MINUS  */
    TOK_MINUS_ASSIGN = 310,        /* TOK_MINUS_ASSIGN  */
    TOK_DECREMENT = 311,           /* TOK_DECREMENT  */
    TOK_STAR = 312,                /* TOK_STAR  */
    TOK_STAR_ASSIGN = 313,         /* TOK_STAR_ASSIGN  */
    TOK_SLASH = 314,               /* TOK_SLASH  */
    TOK_SLASH_ASSIGN = 315,        /* TOK_SLASH_ASSIGN  */
    TOK_PERCENT = 316,             /* TOK_PERCENT  */
    TOK_PERCENT_ASSIGN = 317,      /* TOK_PERCENT_ASSIGN  */
    TOK_AMP = 318,                 /* TOK_AMP  */
    TOK_AMP_ASSIGN = 319,          /* TOK_AMP_ASSIGN  */
    TOK_LOGICAL_AND = 320,         /* TOK_LOGICAL_AND  */
    TOK_PIPE = 321,                /* TOK_PIPE  */
    TOK_PIPE_ASSIGN = 322,         /* TOK_PIPE_ASSIGN  */
    TOK_LOGICAL_OR = 323,          /* TOK_LOGICAL_OR  */
    TOK_CARET = 324,               /* TOK_CARET  */
    TOK_CARET_ASSIGN = 325,        /* TOK_CARET_ASSIGN  */
    TOK_TILDE = 326,               /* TOK_TILDE  */
    TOK_LOGICAL_NOT = 327,         /* TOK_LOGICAL_NOT  */
    TOK_NE = 328,                  /* TOK_NE  */
    TOK_LT = 329,                  /* TOK_LT  */
    TOK_LE = 330,                  /* TOK_LE  */
    TOK_SHL = 331,                 /* TOK_SHL  */
    TOK_SHL_ASSIGN = 332,          /* TOK_SHL_ASSIGN  */
    TOK_GT = 333,                  /* TOK_GT  */
    TOK_GE = 334,                  /* TOK_GE  */
    TOK_SHR = 335,                 /* TOK_SHR  */
    TOK_SHR_ASSIGN = 336,          /* TOK_SHR_ASSIGN  */
    UNARY_PREC = 337               /* UNARY_PREC  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 261 "parser.y"

    long            ival;
    double          dval;
    std::string    *sval;
    nlohmann::ordered_json *expval;

#line 160 "parser.tab.hpp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_PARSER_TAB_HPP_INCLUDED  */
