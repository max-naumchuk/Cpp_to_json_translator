%code requires {
#include <string>
#include <nlohmann/json.hpp>
}

%{
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
%}

%union {
    long            ival;
    double          dval;
    std::string    *sval;
    nlohmann::ordered_json *expval;
}

%token <ival> TOK_INTEGER_LITERAL
%token <dval> TOK_REAL_LITERAL
%token <sval> TOK_STRING_LITERAL
%token <sval> TOK_IDENTIFIER

%token TOK_BREAK TOK_CASE TOK_CLASS TOK_CONST TOK_CONTINUE TOK_DEFAULT
%token TOK_DELETE TOK_DO TOK_ELSE TOK_ENUM TOK_FOR TOK_FRIEND TOK_GOTO
%token TOK_IF TOK_NEW TOK_RETURN TOK_STATIC TOK_STATIC_CAST TOK_STRUCT
%token TOK_SWITCH TOK_TYPEDEF TOK_UNION TOK_WHILE

%token TOK_KW_INT TOK_KW_DOUBLE TOK_KW_FLOAT TOK_KW_CHAR TOK_KW_BOOL
%token TOK_KW_VOID TOK_KW_LONG TOK_KW_SHORT
%token TOK_UNSIGNED

%token TOK_LPAREN TOK_RPAREN TOK_LBRACE TOK_RBRACE
%token TOK_LBRACKET TOK_RBRACKET TOK_SEMICOLON TOK_COLON TOK_COMMA
%token TOK_QUESTION

%token TOK_ASSIGN TOK_EQ
%token TOK_PLUS TOK_PLUS_ASSIGN TOK_INCREMENT
%token TOK_MINUS TOK_MINUS_ASSIGN TOK_DECREMENT
%token TOK_STAR TOK_STAR_ASSIGN
%token TOK_SLASH TOK_SLASH_ASSIGN
%token TOK_PERCENT TOK_PERCENT_ASSIGN
%token TOK_AMP TOK_AMP_ASSIGN TOK_LOGICAL_AND
%token TOK_PIPE TOK_PIPE_ASSIGN TOK_LOGICAL_OR
%token TOK_CARET TOK_CARET_ASSIGN
%token TOK_TILDE
%token TOK_LOGICAL_NOT TOK_NE
%token TOK_LT TOK_LE TOK_SHL TOK_SHL_ASSIGN
%token TOK_GT TOK_GE TOK_SHR TOK_SHR_ASSIGN

%type <expval> program
%type <expval> F_STATEMENTS F_STATEMENT
%type <expval> STATEMENTS STATEMENT
%type <expval> expr assign_expr
%type <expval> TYPE_SPEC
%type <expval> PARAMS PARAM_LIST PARAM
%type <expval> DECLARATOR_LIST DECLARATOR
%type <expval> ARG_LIST_OPT ARG_LIST
%type <expval> INDEX_SUFFIX_LIST
%type <expval> BASE_LIST_OPT BASE_LIST
%type <expval> ENUM_VALUE_LIST ENUM_VALUE
%type <expval> CHOOSE_VARIANTS CHOOSE_VARIANT
%type <expval> FOR_INIT
%type <ival> STATIC_OPT CONST_OPT

%right TOK_ASSIGN TOK_PLUS_ASSIGN TOK_MINUS_ASSIGN TOK_STAR_ASSIGN
%right TOK_SLASH_ASSIGN TOK_PERCENT_ASSIGN TOK_AMP_ASSIGN TOK_PIPE_ASSIGN
%right TOK_CARET_ASSIGN TOK_SHL_ASSIGN TOK_SHR_ASSIGN
%right TOK_QUESTION
%left TOK_LOGICAL_OR
%left TOK_LOGICAL_AND
%left TOK_PIPE
%left TOK_CARET
%left TOK_AMP
%left TOK_EQ TOK_NE
%left TOK_LT TOK_LE TOK_GT TOK_GE
%left TOK_SHL TOK_SHR
%left TOK_PLUS TOK_MINUS
%left TOK_STAR TOK_SLASH TOK_PERCENT
%precedence UNARY_PREC
%left TOK_INCREMENT TOK_DECREMENT
%right TOK_LBRACKET

%%

program:
    F_STATEMENTS {
        g_output = *$1;
        delete $1;
    }
    ;


F_STATEMENTS:
     {
        $$ = new nlohmann::ordered_json(nlohmann::ordered_json::array());
    }
    | F_STATEMENTS F_STATEMENT {
        $1->push_back(*$2);
        $$ = $1;
        delete $2;
    }
    ;

F_STATEMENT:
    STATIC_OPT CONST_OPT TYPE_SPEC TOK_IDENTIFIER
    {
        g_symbolTable.pushScope();
        g_functionReturnTypes[*$4] = *$3;
    }
    TOK_LPAREN PARAMS TOK_RPAREN
    TOK_LBRACE STATEMENTS TOK_RBRACE {
        g_symbolTable.popScope();
        markLastReturn($10);
        nlohmann::ordered_json body = {
            {"type", "compound-statement"},
            {"statements", *$10}
        };
        $$ = new nlohmann::ordered_json({
            {"type", "subroutine-declaration"},
            {"subroutine-name", *$4},
            {"result-type", *$3},
            {"arguments-declarators", *$7},
            {"constant-section", nlohmann::ordered_json::array()},
            {"type-section", nlohmann::ordered_json::array()},
            {"variables-section", nlohmann::ordered_json::array()},
            {"subroutines-section", nlohmann::ordered_json::array()},
            {"subroutine-body", body}
        });
        delete $3; delete $4; delete $7; delete $10;
    }
    | TOK_STRUCT TOK_IDENTIFIER BASE_LIST_OPT TOK_LBRACE {
        g_symbolTable.pushScope();
        g_structDepth++;
    }
    F_STATEMENTS TOK_RBRACE TOK_SEMICOLON {
        g_structDepth--;
        g_symbolTable.popScope();
        nlohmann::ordered_json fields = nlohmann::ordered_json::array();
        nlohmann::ordered_json methods = nlohmann::ordered_json::array();
        nlohmann::ordered_json friendsArr = nlohmann::ordered_json::array();
        splitMembers($6, fields, methods, friendsArr);
        $$ = new nlohmann::ordered_json({
            {"type", "struct-type"},
            {"name", *$2},
            {"base-structs", *$3},
            {"fields", fields},
            {"methods", methods},
            {"friends", friendsArr}
        });
        delete $2; delete $3; delete $6;
    }
    | TOK_CLASS TOK_IDENTIFIER BASE_LIST_OPT TOK_LBRACE {
        g_symbolTable.pushScope();
        g_structDepth++;
    }
    F_STATEMENTS TOK_RBRACE TOK_SEMICOLON {
        g_structDepth--;
        g_symbolTable.popScope();
        nlohmann::ordered_json fields = nlohmann::ordered_json::array();
        nlohmann::ordered_json methods = nlohmann::ordered_json::array();
        nlohmann::ordered_json friendsArr = nlohmann::ordered_json::array();
        splitMembers($6, fields, methods, friendsArr);
        $$ = new nlohmann::ordered_json({
            {"type", "struct-type"},
            {"name", *$2},
            {"base-structs", *$3},
            {"fields", fields},
            {"methods", methods},
            {"friends", friendsArr}
        });
        delete $2; delete $3; delete $6;
    }
    | TOK_UNION TOK_IDENTIFIER TOK_LBRACE {
        g_symbolTable.pushScope();
        g_structDepth++;
    }
    F_STATEMENTS TOK_RBRACE TOK_SEMICOLON {
        g_structDepth--;
        g_symbolTable.popScope();
        nlohmann::ordered_json fields = nlohmann::ordered_json::array();
        nlohmann::ordered_json methods = nlohmann::ordered_json::array();
        nlohmann::ordered_json friendsArr = nlohmann::ordered_json::array();
        splitMembers($5, fields, methods, friendsArr);
        $$ = new nlohmann::ordered_json({
            {"type", "union-type"},
            {"name", *$2},
            {"fields", fields},
            {"methods", methods},
            {"friends", friendsArr}
        });
        delete $2; delete $5;
    }
    | TOK_FRIEND TOK_CLASS TOK_IDENTIFIER TOK_SEMICOLON {
        $$ = new nlohmann::ordered_json({{"type", "friend-declaration"}, {"friend-name", *$3}});
        delete $3;
    }
    | TOK_FRIEND TOK_STRUCT TOK_IDENTIFIER TOK_SEMICOLON {
        $$ = new nlohmann::ordered_json({{"type", "friend-declaration"}, {"friend-name", *$3}});
        delete $3;
    }
    | TOK_FRIEND TYPE_SPEC TOK_IDENTIFIER TOK_LPAREN PARAMS TOK_RPAREN TOK_SEMICOLON {
        $$ = new nlohmann::ordered_json({{"type", "friend-declaration"}, {"friend-name", *$3}});
        delete $2; delete $3; delete $5;
    }
    | TOK_ENUM TOK_IDENTIFIER TOK_LBRACE ENUM_VALUE_LIST TOK_RBRACE TOK_SEMICOLON {
        $$ = new nlohmann::ordered_json({
            {"type", "enumeration-type"},
            {"name", *$2},
            {"values", *$4}
        });
        delete $2; delete $4;
    }
    | TOK_TYPEDEF TYPE_SPEC TOK_IDENTIFIER TOK_SEMICOLON {
        $$ = new nlohmann::ordered_json({
            {"type", "typedef-type"},
            {"name", *$3},
            {"old-name", *$2}
        });
        delete $2; delete $3;
    }
    | STATIC_OPT CONST_OPT TYPE_SPEC DECLARATOR_LIST TOK_SEMICOLON {
        $$ = buildVarDecl(g_structDepth == 0, $1, $2, $3, $4);
    }
    ;

STATEMENTS:
    { $$ = new nlohmann::ordered_json(nlohmann::ordered_json::array()); }
    | STATEMENTS STATEMENT {
        $1->push_back(*$2);
        $$ = $1;
        delete $2;
    }
    ;

STATEMENT:
    expr TOK_SEMICOLON {
        $$ = new nlohmann::ordered_json({
            {"type", "expression-statement"},
            {"expression", *$1}
        });
        delete $1;
    }
    | TOK_LBRACE { g_symbolTable.pushScope(); } STATEMENTS TOK_RBRACE {
        g_symbolTable.popScope();
        $$ = new nlohmann::ordered_json({
            {"type", "compound-statement"},
            {"statements", *$3}
        });
        delete $3;
    }
    | TOK_IF TOK_LPAREN expr TOK_RPAREN STATEMENT TOK_ELSE STATEMENT {
        $$ = new nlohmann::ordered_json({
            {"type", "conditional-statement"},
            {"condition", *$3},
            {"if-true", *$5},
            {"if-false", *$7}
        });
        delete $3; delete $5; delete $7;
    }
    | TOK_IF TOK_LPAREN expr TOK_RPAREN STATEMENT {
        $$ = new nlohmann::ordered_json({
            {"type", "conditional-statement"},
            {"condition", *$3},
            {"if-true", *$5},
            {"if-false", nullptr}
        });
        delete $3; delete $5;
    }
    | TOK_SWITCH TOK_LPAREN expr TOK_RPAREN TOK_LBRACE CHOOSE_VARIANTS TOK_RBRACE {
        bool passControl = computePassControl(*$6);
        $$ = new nlohmann::ordered_json({
            {"type", "choose-statement"},
            {"pass-control", passControl},
            {"expression-to-analyze", *$3},
            {"variants", *$6}
        });
        delete $3; delete $6;
    }
    | TOK_WHILE TOK_LPAREN expr TOK_RPAREN STATEMENT {
        $$ = new nlohmann::ordered_json({
            {"type", "loop-statement"},
            {"begin-condition", *$3},
            {"end-condition", nullptr},
            {"loop-index", nullptr},
            {"loop-initialization", nullptr},
            {"loop-pass-to-next-iteration", nullptr},
            {"loop-body", *$5}
        });
        delete $3; delete $5;
    }
    | TOK_DO STATEMENT TOK_WHILE TOK_LPAREN expr TOK_RPAREN TOK_SEMICOLON {
        $$ = new nlohmann::ordered_json({
            {"type", "loop-statement"},
            {"begin-condition", nullptr},
            {"end-condition", *$5},
            {"loop-index", nullptr},
            {"loop-initialization", nullptr},
            {"loop-pass-to-next-iteration", nullptr},
            {"loop-body", *$2}
        });
        delete $2; delete $5;
    }
    | TOK_FOR TOK_LPAREN {
        g_symbolTable.pushScope();
    }
    FOR_INIT expr TOK_SEMICOLON expr TOK_RPAREN STATEMENT {
        g_symbolTable.popScope();
        $$ = new nlohmann::ordered_json({
            {"type", "loop-statement"},
            {"begin-condition", *$5},
            {"end-condition", nullptr},
            {"loop-index", (*$4)["loop-init-name"]},
            {"loop-initialization", (*$4)["loop-init-node"]},
            {"loop-pass-to-next-iteration", *$7},
            {"loop-body", *$9}
        });
        delete $4; delete $5; delete $7; delete $9;
    }
    
    | TOK_RETURN expr TOK_SEMICOLON {
        $$ = new nlohmann::ordered_json({
            {"type", "return-statement"},
            {"expression", *$2},
            {"the-last-statement", false}
        });
        delete $2;
    }
    | TOK_RETURN TOK_SEMICOLON {
        $$ = new nlohmann::ordered_json({
            {"type", "return-statement"},
            {"expression", nullptr},
            {"the-last-statement", false}
        });
    } 
    | TOK_BREAK TOK_SEMICOLON {
        $$ = new nlohmann::ordered_json({{"type", "break-statement"}, {"label", nullptr}});
    }
    | TOK_CONTINUE TOK_SEMICOLON {
        $$ = new nlohmann::ordered_json({{"type", "continue-statement"}, {"label", nullptr}});
    }
    | TOK_GOTO TOK_IDENTIFIER TOK_SEMICOLON {
        $$ = new nlohmann::ordered_json({
            {"type", "goto-statement"},
            {"target-label", *$2}
        });
        delete $2;
    }
    | TOK_IDENTIFIER TOK_COLON STATEMENT {
        $$ = new nlohmann::ordered_json({
            {"type", "label"},
            {"label-name", *$1},
            {"statement", *$3}
        });
        delete $1; delete $3;
    }
    
    | STATIC_OPT CONST_OPT TYPE_SPEC DECLARATOR_LIST TOK_SEMICOLON {
        $$ = buildVarDecl(false, $1, $2, $3, $4);
    }
    
    | TOK_ENUM TOK_IDENTIFIER TOK_LBRACE ENUM_VALUE_LIST TOK_RBRACE TOK_SEMICOLON {
        $$ = new nlohmann::ordered_json({
            {"type", "enumeration-type"},
            {"name", *$2},
            {"values", *$4}
        });
        delete $2; delete $4;
    }
    | TOK_STRUCT TOK_IDENTIFIER BASE_LIST_OPT TOK_LBRACE {
        g_symbolTable.pushScope();
        g_structDepth++;
    }
    F_STATEMENTS TOK_RBRACE TOK_SEMICOLON {
        g_structDepth--;
        g_symbolTable.popScope();
        nlohmann::ordered_json fields = nlohmann::ordered_json::array();
        nlohmann::ordered_json methods = nlohmann::ordered_json::array();
        nlohmann::ordered_json friendsArr = nlohmann::ordered_json::array();
        splitMembers($6, fields, methods, friendsArr);
        $$ = new nlohmann::ordered_json({
            {"type", "struct-type"},
            {"name", *$2},
            {"base-structs", *$3},
            {"fields", fields},
            {"methods", methods},
            {"friends", friendsArr}
        });
        delete $2; delete $3; delete $6;
    }
    | TOK_CLASS TOK_IDENTIFIER BASE_LIST_OPT TOK_LBRACE {
        g_symbolTable.pushScope();
        g_structDepth++;
    }
    F_STATEMENTS TOK_RBRACE TOK_SEMICOLON {  
        g_structDepth--;
        g_symbolTable.popScope();
        nlohmann::ordered_json fields = nlohmann::ordered_json::array();
        nlohmann::ordered_json methods = nlohmann::ordered_json::array();
        nlohmann::ordered_json friendsArr = nlohmann::ordered_json::array();
        splitMembers($6, fields, methods, friendsArr);
        $$ = new nlohmann::ordered_json({
            {"type", "struct-type"},
            {"name", *$2},
            {"base-structs", *$3},
            {"fields", fields},
            {"methods", methods},
            {"friends", friendsArr}
        });
        delete $2; delete $3; delete $6;
    }
    | TOK_UNION TOK_IDENTIFIER TOK_LBRACE {
        g_symbolTable.pushScope();
        g_structDepth++;
    }
    F_STATEMENTS TOK_RBRACE TOK_SEMICOLON {
        g_structDepth--;
        g_symbolTable.popScope();
        nlohmann::ordered_json fields = nlohmann::ordered_json::array();
        nlohmann::ordered_json methods = nlohmann::ordered_json::array();
        nlohmann::ordered_json friendsArr = nlohmann::ordered_json::array();
        splitMembers($5, fields, methods, friendsArr);
        $$ = new nlohmann::ordered_json({
            {"type", "union-type"},
            {"name", *$2},
            {"fields", fields},
            {"methods", methods},
            {"friends", friendsArr}
        });
        delete $2; delete $5;
    }
    | TOK_TYPEDEF TYPE_SPEC TOK_IDENTIFIER TOK_SEMICOLON {
        $$ = new nlohmann::ordered_json({
            {"type", "typedef-type"},
            {"name", *$3},
            {"old-name", *$2}
        });
        delete $2; delete $3;
    }
    ;


CHOOSE_VARIANTS:
     { $$ = new nlohmann::ordered_json(nlohmann::ordered_json::array()); }
    | CHOOSE_VARIANTS CHOOSE_VARIANT {
        $1->push_back(*$2);
        $$ = $1;
        delete $2;
    }
    ;

CHOOSE_VARIANT:
    
    TOK_CASE expr TOK_COLON STATEMENTS {
        markLastReturn($4);
        nlohmann::ordered_json body = {
            {"type", "compound-statement"},
            {"statements", *$4}
        };
        $$ = new nlohmann::ordered_json({
            {"type", "choose-variant"},
            {"condition-label", *$2},
            {"variant-body", body}
        });
        delete $2; delete $4;
    }
    | TOK_DEFAULT TOK_COLON STATEMENTS {
        markLastReturn($3);
        nlohmann::ordered_json body = {
            {"type", "compound-statement"},
            {"statements", *$3}
        };
        $$ = new nlohmann::ordered_json({
            {"type", "choose-variant"},
            {"condition-label", nullptr},
            {"variant-body", body}
        });
        delete $3;
    }
    ;


TYPE_SPEC:
    TOK_KW_INT    { $$ = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","int"}}); }
    | TOK_KW_DOUBLE { $$ = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","double"}}); }
    | TOK_KW_FLOAT  { $$ = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","float"}}); }
    | TOK_KW_CHAR   { $$ = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","char"}}); }
    | TOK_KW_BOOL   { $$ = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","bool"}}); }
    | TOK_KW_VOID   { $$ = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","void"}}); }
    | TOK_KW_LONG   { $$ = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","long"}}); }
    | TOK_KW_SHORT  { $$ = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","short"}}); }
    
    | TOK_UNSIGNED { $$ = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","unsigned int"}}); }
    | TOK_UNSIGNED TOK_KW_INT   { $$ = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","unsigned int"}}); }
    | TOK_UNSIGNED TOK_KW_CHAR  { $$ = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","unsigned char"}}); }
    | TOK_UNSIGNED TOK_KW_SHORT { $$ = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","unsigned short"}}); }
    | TOK_UNSIGNED TOK_KW_LONG  { $$ = new nlohmann::ordered_json({{"type","builtin-type"},{"type-value","unsigned long"}}); }
    
    | TYPE_SPEC TOK_STAR {
        $$ = new nlohmann::ordered_json({
            {"type", "pointer-type"},
            {"pointer-to", *$1}
        });
        delete $1;
    }
    ;


BASE_LIST_OPT:
     { $$ = new nlohmann::ordered_json(nlohmann::ordered_json::array()); }
    | TOK_COLON BASE_LIST { $$ = $2; }
    ;

BASE_LIST:
    TOK_IDENTIFIER {
        $$ = new nlohmann::ordered_json(nlohmann::ordered_json::array());
        $$->push_back({{"type","builtin-type"},{"type-value",*$1}});
        delete $1;
    }
    | BASE_LIST TOK_COMMA TOK_IDENTIFIER {
        $1->push_back({{"type","builtin-type"},{"type-value",*$3}});
        $$ = $1;
        delete $3;
    }
    ;

ENUM_VALUE_LIST:
    ENUM_VALUE {
        $$ = new nlohmann::ordered_json(nlohmann::ordered_json::array());
        $$->push_back(*$1);
        delete $1;
    }
    | ENUM_VALUE_LIST TOK_COMMA ENUM_VALUE {
        $1->push_back(*$3);
        $$ = $1;
        delete $3;
    }
    ;

ENUM_VALUE:
    TOK_IDENTIFIER {
        g_symbolTable.declare(*$1, {{"type","builtin-type"},{"type-value","int"}});
        $$ = new nlohmann::ordered_json(*$1);
        delete $1;
    }
    | TOK_IDENTIFIER TOK_ASSIGN assign_expr {
        g_symbolTable.declare(*$1, {{"type","builtin-type"},{"type-value","int"}});
        $$ = new nlohmann::ordered_json(*$1);
        delete $1;
        delete $3; 
    }
    ;

PARAMS:
     { $$ = new nlohmann::ordered_json(nlohmann::ordered_json::array()); }
    | PARAM_LIST { $$ = $1; }
    ;

PARAM_LIST:
    PARAM {
        $$ = new nlohmann::ordered_json(nlohmann::ordered_json::array());
        $$->push_back(*$1);
        delete $1;
    }
    | PARAM_LIST TOK_COMMA PARAM {
        $1->push_back(*$3);
        $$ = $1;
        delete $3;
    }
    ;

PARAM:
    TYPE_SPEC DECLARATOR {
        stampDeclaratorType(*$2, *$1);
        $$ = $2;
        delete $1;
    }
    ;

DECLARATOR_LIST:
    DECLARATOR {
        $$ = new nlohmann::ordered_json(nlohmann::ordered_json::array());
        $$->push_back(*$1);
        delete $1;
    }
    | DECLARATOR_LIST TOK_COMMA DECLARATOR {
        $1->push_back(*$3);
        $$ = $1;
        delete $3;
    }
    ;

DECLARATOR:
    TOK_IDENTIFIER {    
        $$ = new nlohmann::ordered_json({
            {"type", "declarator"},
            {"variable-name", *$1},
            {"reference", false},
            {"constructor-name", nullptr},
            {"constructor-arguments", nlohmann::ordered_json::array()}
        });
        delete $1;
    }
    | TOK_AMP TOK_IDENTIFIER {
        $$ = new nlohmann::ordered_json({
            {"type", "declarator"},
            {"variable-name", *$2},
            {"reference", true},
            {"constructor-name", nullptr},
            {"constructor-arguments", nlohmann::ordered_json::array()}
        });
        delete $2;
    }
    | TOK_IDENTIFIER TOK_ASSIGN assign_expr {      
        $$ = new nlohmann::ordered_json({
            {"type", "declarator"},
            {"variable-name", *$1},
            {"reference", false},
            {"constructor-name", nullptr},
            {"constructor-arguments", nlohmann::ordered_json::array({*$3})}
        });
        delete $1; delete $3;
    }
    
    | TOK_IDENTIFIER INDEX_SUFFIX_LIST {
        $$ = new nlohmann::ordered_json({
            {"type", "declarator"},
            {"variable-name", *$1},
            {"reference", false},
            {"constructor-name", nullptr},
            {"constructor-arguments", nlohmann::ordered_json::array()},
            {"__index_ranges", *$2}
        });
        delete $1; delete $2;
    }
    ;

STATIC_OPT:
     { $$ = 0; }
    | TOK_STATIC { $$ = 1; }
    ;

CONST_OPT:
     { $$ = 0; }
    | TOK_CONST { $$ = 1; }
    ;

FOR_INIT:
    expr TOK_SEMICOLON {
        $$ = new nlohmann::ordered_json({
            {"loop-init-node", *$1},
            {"loop-init-name", nullptr}
        });
        delete $1;
    }
    | STATIC_OPT CONST_OPT TYPE_SPEC DECLARATOR_LIST TOK_SEMICOLON {
        nlohmann::ordered_json *decl = buildVarDecl(false, $1, $2, $3, $4);
        nlohmann::ordered_json name = nullptr;
        if (!decl->at("declarators").empty()) {
            name = decl->at("declarators")[0].value("variable-name", "");
        }
        $$ = new nlohmann::ordered_json({
            {"loop-init-node", *decl},
            {"loop-init-name", name}
        });
        delete decl;
    }
    ;

expr:
    assign_expr { $$ = $1; }
    | expr TOK_COMMA assign_expr {   
        $$ = new nlohmann::ordered_json({
            {"type", "sequence"},
            {"result-type", inferSimpleType($3)},
            {"left", *$1},
            {"right", *$3}
        });
        delete $1; delete $3;
    }
    ;

assign_expr:
    TOK_INTEGER_LITERAL {
        $$ = new nlohmann::ordered_json({
            {"type", "literal-constant"},
            {"constant-type", "integer"},
            {"constant-value", $1}
        });
    }
    | TOK_REAL_LITERAL {
        $$ = new nlohmann::ordered_json({
            {"type", "literal-constant"},
            {"constant-type", "real"},
            {"constant-value", $1}
        });
    }
    | TOK_STRING_LITERAL {
        $$ = new nlohmann::ordered_json({
            {"type", "literal-constant"},
            {"constant-type", "string"},
            {"constant-value", *$1}
        });
        delete $1;
    }
    | TOK_IDENTIFIER {      
        if (!g_symbolTable.isDeclared(*$1))
            reportSemanticError("use of undeclared identifier '" + *$1 + "'");
        $$ = new nlohmann::ordered_json({
            {"type", "variable-reference"},
            {"variable-name", *$1},
            {"variable-type", g_symbolTable.lookupTypeValue(*$1)},
            {"variable-usage", "rvalue"}
        });
        delete $1;
    }
    | TOK_IDENTIFIER TOK_LPAREN ARG_LIST_OPT TOK_RPAREN {
        std::string resultType = "unknown";
        auto found = g_functionReturnTypes.find(*$1);
        if (found != g_functionReturnTypes.end()) {
            resultType = typeJsonToName(found->second);
        } else {
            reportSemanticError("call to undeclared function '" + *$1 + "'");
        }
        $$ = new nlohmann::ordered_json({
            {"type", "function-call"},
            {"function-name", *$1},
            {"result-type", resultType},
            {"arguments", *$3}
        });
        delete $1; delete $3;
    }
    | TOK_LPAREN expr TOK_RPAREN {       
        $$ = new nlohmann::ordered_json({
            {"type", "parentheses"},
            {"result-type", inferSimpleType($2)},
            {"argument", *$2}
        });
        delete $2;
    }
    | TOK_LPAREN TYPE_SPEC TOK_RPAREN assign_expr %prec UNARY_PREC {
        $$ = new nlohmann::ordered_json({
            {"type", "type-cast"},
            {"result-type", *$2},
            {"argument", *$4}
        });
        delete $2; delete $4;
    }
    | assign_expr INDEX_SUFFIX_LIST %prec TOK_LBRACKET {
        $$ = new nlohmann::ordered_json({
            {"type", "index"},
            {"array-expression", *$1},           
            {"result-type", inferSimpleType($1)},
            {"indices", *$2}
        });
        delete $1; delete $2;
    }
    | assign_expr TOK_PLUS assign_expr {
        if ((*$1)["type"] == "literal-constant" && (*$1)["constant-type"] == "string" &&
            (*$3)["type"] == "literal-constant" && (*$3)["constant-type"] == "string") {
            $$ = new nlohmann::ordered_json({
                {"type", "string-concatenation"},
                {"result-type", "string"},
                {"left", *$1}, {"right", *$3}
            });
        } else {
            $$ = new nlohmann::ordered_json({
                {"type", "sum"},
                {"result-type", promoteArithmeticType(inferOperandType($1), inferOperandType($3), '+')},
                {"left", *$1}, {"right", *$3}
            });
        }
        delete $1; delete $3;
    }
    | assign_expr TOK_MINUS assign_expr {
        $$ = new nlohmann::ordered_json({
            {"type", "difference"},
            {"result-type", promoteArithmeticType(inferOperandType($1), inferOperandType($3), '-')},
            {"left", *$1}, {"right", *$3}
        });
        delete $1; delete $3;
    }
    | assign_expr TOK_STAR assign_expr {
        $$ = new nlohmann::ordered_json({
            {"type", "product"},
            {"result-type", promoteArithmeticType(inferOperandType($1), inferOperandType($3), '*')},
            {"left", *$1}, {"right", *$3}
        });
        delete $1; delete $3;
    }
    | assign_expr TOK_SLASH assign_expr {
        std::string leftType = inferOperandType($1);
        std::string rightType = inferOperandType($3);
        std::string nodeType =
            (isIntegerTypeName(leftType) && isIntegerTypeName(rightType))
                ? "integer-quotient" : "quotient";
        $$ = new nlohmann::ordered_json({
            {"type", nodeType},
            {"result-type", promoteArithmeticType(leftType, rightType, '/')},
            {"left", *$1}, {"right", *$3}
        });
        delete $1; delete $3;
    }
    | assign_expr TOK_PERCENT assign_expr {
        $$ = new nlohmann::ordered_json({
            {"type", "modulo"},
            {"result-type", computeModuloType(inferOperandType($1), inferOperandType($3))},
            {"left", *$1}, {"right", *$3}
        });
        delete $1; delete $3;
    }
    | TOK_INCREMENT assign_expr %prec UNARY_PREC {
        markLvalueIfVariableRef(*$2);
        $$ = new nlohmann::ordered_json({
            {"type", "prefix-increment"},
            {"result-type", inferSimpleType($2)},
            {"argument", *$2}
        });
        delete $2;
    }
    | assign_expr TOK_INCREMENT {
        markLvalueIfVariableRef(*$1);
        $$ = new nlohmann::ordered_json({
            {"type", "postfix-increment"},
            {"result-type", inferSimpleType($1)},
            {"argument", *$1}
        });
        delete $1;
    }
    | TOK_DECREMENT assign_expr %prec UNARY_PREC {
        markLvalueIfVariableRef(*$2);
        $$ = new nlohmann::ordered_json({
            {"type", "prefix-decrement"},
            {"result-type", inferSimpleType($2)},
            {"argument", *$2}
        });
        delete $2;
    }
    | assign_expr TOK_DECREMENT {
        markLvalueIfVariableRef(*$1);
        $$ = new nlohmann::ordered_json({
            {"type", "postfix-decrement"},
            {"result-type", inferSimpleType($1)},
            {"argument", *$1}
        });
        delete $1;
    }    
    | TOK_MINUS assign_expr %prec UNARY_PREC {
        nlohmann::ordered_json zero = {
            {"type", "literal-constant"},
            {"constant-type", "integer"},
            {"constant-value", 0}
        };
        $$ = new nlohmann::ordered_json({
            {"type", "difference"},
            {"result-type", promoteArithmeticType("int", inferSimpleType($2), '-')},
            {"left", zero},
            {"right", *$2}
        });
        delete $2;
    }
    | TOK_PLUS assign_expr %prec UNARY_PREC {
        $$ = $2;
    }   
    | assign_expr TOK_EQ assign_expr {
        $$ = new nlohmann::ordered_json({{"type","equals"},{"result-type","boolean"},{"left",*$1},{"right",*$3}});
        delete $1; delete $3;
    }
    | assign_expr TOK_LT assign_expr {
        $$ = new nlohmann::ordered_json({{"type","less"},{"result-type","boolean"},{"left",*$1},{"right",*$3}});
        delete $1; delete $3;
    }
    | assign_expr TOK_LE assign_expr {
        $$ = new nlohmann::ordered_json({{"type","less-or-equal"},{"result-type","boolean"},{"left",*$1},{"right",*$3}});
        delete $1; delete $3;
    }
    | assign_expr TOK_GT assign_expr {
        $$ = new nlohmann::ordered_json({{"type","greater"},{"result-type","boolean"},{"left",*$1},{"right",*$3}});
        delete $1; delete $3;
    }
    | assign_expr TOK_GE assign_expr {
        $$ = new nlohmann::ordered_json({{"type","greater-or-equal"},{"result-type","boolean"},{"left",*$1},{"right",*$3}});
        delete $1; delete $3;
    }
    | assign_expr TOK_NE assign_expr {
        $$ = new nlohmann::ordered_json({{"type","not-equal"},{"result-type","boolean"},{"left",*$1},{"right",*$3}});
        delete $1; delete $3;
    }  
    | assign_expr TOK_LOGICAL_AND assign_expr {
        $$ = new nlohmann::ordered_json({{"type","logical-and"},{"result-type","boolean"},{"left",*$1},{"right",*$3}});
        delete $1; delete $3;
    }
    | assign_expr TOK_LOGICAL_OR assign_expr {
        $$ = new nlohmann::ordered_json({{"type","logical-or"},{"result-type","boolean"},{"left",*$1},{"right",*$3}});
        delete $1; delete $3;
    }
    | TOK_LOGICAL_NOT assign_expr %prec UNARY_PREC {
        $$ = new nlohmann::ordered_json({{"type","logical-not"},{"result-type","boolean"},{"argument",*$2}});
        delete $2;
    }   
    | assign_expr TOK_AMP assign_expr {
        $$ = new nlohmann::ordered_json({{"type","bitwise-and"},{"result-type","int"},{"left",*$1},{"right",*$3}});
        delete $1; delete $3;
    }
    | assign_expr TOK_PIPE assign_expr {
        $$ = new nlohmann::ordered_json({{"type","bitwise-or"},{"result-type","int"},{"left",*$1},{"right",*$3}});
        delete $1; delete $3;
    }
    | assign_expr TOK_CARET assign_expr {
        $$ = new nlohmann::ordered_json({{"type","bitwise-xor"},{"result-type","int"},{"left",*$1},{"right",*$3}});
        delete $1; delete $3;
    }
    | TOK_TILDE assign_expr %prec UNARY_PREC {
        $$ = new nlohmann::ordered_json({{"type","bitwise-not"},{"result-type","int"},{"argument",*$2}});
        delete $2;
    }
    | assign_expr TOK_SHR assign_expr {       
        std::string leftType = inferSimpleType($1);
        std::string nodeType = isUnsignedTypeName(leftType)
            ? "logic-bitwise-shift-right" : "arithmetic-bitwise-shift-right";
        $$ = new nlohmann::ordered_json({
            {"type", nodeType},{"result-type","int"},
            {"amount",*$3},{"argument",*$1}
        });
        delete $1; delete $3;
    }
    | assign_expr TOK_SHL assign_expr {
        $$ = new nlohmann::ordered_json({
            {"type","bitwise-shift-left"},{"result-type","int"},
            {"amount",*$3},{"argument",*$1}
        });
        delete $1; delete $3;
    }
    | assign_expr TOK_ASSIGN assign_expr {
        markLvalueIfVariableRef(*$1);
        $$ = new nlohmann::ordered_json({
            {"type","assignment"},{"assignment-type",inferSimpleType($1)},
            {"variable",*$1},{"expression",*$3}
        });
        delete $1; delete $3;
    }
    | assign_expr TOK_PLUS_ASSIGN assign_expr {
        markLvalueIfVariableRef(*$1);
        $$ = new nlohmann::ordered_json({
            {"type","assignment-sum"},{"assignment-type",inferSimpleType($1)},
            {"variable",*$1},{"expression",*$3}
        });
        delete $1; delete $3;
    }
    | assign_expr TOK_MINUS_ASSIGN assign_expr {
        markLvalueIfVariableRef(*$1);
        $$ = new nlohmann::ordered_json({
            {"type","assignment-difference"},{"assignment-type",inferSimpleType($1)},
            {"variable",*$1},{"expression",*$3}
        });
        delete $1; delete $3;
    }
    | assign_expr TOK_STAR_ASSIGN assign_expr {
        markLvalueIfVariableRef(*$1);
        $$ = new nlohmann::ordered_json({
            {"type","assignment-product"},{"assignment-type",inferSimpleType($1)},
            {"variable",*$1},{"expression",*$3}
        });
        delete $1; delete $3;
    }
    | assign_expr TOK_SLASH_ASSIGN assign_expr {
        markLvalueIfVariableRef(*$1);
        std::string leftType = inferSimpleType($1);
        std::string rightType = inferSimpleType($3);
        std::string nodeType =
            (isIntegerTypeName(leftType) && isIntegerTypeName(rightType))
                ? "assignment-integer-quotient" : "assignment-quotient";
        $$ = new nlohmann::ordered_json({
            {"type", nodeType}, {"assignment-type",leftType},
            {"variable",*$1},{"expression",*$3}
        });
        delete $1; delete $3;
    }
    | assign_expr TOK_PERCENT_ASSIGN assign_expr {
        markLvalueIfVariableRef(*$1);
        std::string leftType = inferSimpleType($1);
        std::string rightType = inferSimpleType($3);       
        computeModuloType(leftType, rightType);
        $$ = new nlohmann::ordered_json({
            {"type","assignment-modulo"},{"assignment-type",leftType},
            {"variable",*$1},{"expression",*$3}
        });
        delete $1; delete $3;
    }
    | assign_expr TOK_AMP_ASSIGN assign_expr {
        markLvalueIfVariableRef(*$1);
        $$ = new nlohmann::ordered_json({
            {"type","assignment-bitwise-and"},{"result-type","int"},
            {"left",*$1},{"right",*$3}
        });
        delete $1; delete $3;
    }
    | assign_expr TOK_PIPE_ASSIGN assign_expr {
        markLvalueIfVariableRef(*$1);
        $$ = new nlohmann::ordered_json({
            {"type","assignment-bitwise-or"},{"result-type","int"},
            {"left",*$1},{"right",*$3}
        });
        delete $1; delete $3;
    }
    | assign_expr TOK_CARET_ASSIGN assign_expr {
        markLvalueIfVariableRef(*$1);
        $$ = new nlohmann::ordered_json({
            {"type","assignment-bitwise-xor"},{"result-type","int"},
            {"left",*$1},{"right",*$3}
        });
        delete $1; delete $3;
    }
    | assign_expr TOK_SHR_ASSIGN assign_expr {
        markLvalueIfVariableRef(*$1);
        std::string leftType = inferSimpleType($1);
        std::string nodeType = isUnsignedTypeName(leftType)
            ? "assignment-logic-bitwise-shift-right" : "assignment-arithmetic-bitwise-shift-right";
        $$ = new nlohmann::ordered_json({
            {"type", nodeType},{"result-type","int"},
            {"amount",*$3},{"argument",*$1}
        });
        delete $1; delete $3;
    }
    | assign_expr TOK_SHL_ASSIGN assign_expr {
        markLvalueIfVariableRef(*$1);
        $$ = new nlohmann::ordered_json({
            {"type","assignment-bitwise-shift-left"},{"result-type","int"},
            {"amount",*$3},{"argument",*$1}
        });
        delete $1; delete $3;
    }  
    | TOK_STAR assign_expr %prec UNARY_PREC {
        std::string argType = inferSimpleType($2);
        std::string resultType = "unknown";
        bool argIsArray = ($2->value("type","") == "variable-reference") &&
                          g_symbolTable.isArray($2->value("variable-name",""));
        if (!argType.empty() && argType.back() == '*') {
            resultType = argType.substr(0, argType.size() - 1);   
        } else if (argIsArray) {
            resultType = argType;   
        } else if (isIntegerTypeName(argType) || isRealTypeName(argType)) {
            reportSemanticError("cannot dereference non-pointer type '" + argType + "'");
        }
        $$ = new nlohmann::ordered_json({{"type","dereference"},{"result-type",resultType},{"argument",*$2}});
        delete $2;
    }
    | TOK_AMP assign_expr %prec UNARY_PREC {
        markLvalueIfVariableRef(*$2);
        std::string argType = inferSimpleType($2);
        $$ = new nlohmann::ordered_json({{"type","address"},
            {"result-type", argType == "unknown" ? "unknown" : argType + "*"},{"argument",*$2}});
        delete $2;
    }
    | TOK_NEW TYPE_SPEC TOK_LBRACKET expr TOK_RBRACKET %prec UNARY_PREC {
        $$ = new nlohmann::ordered_json({
            {"type","dynamic-memory-allocation"},{"array-flag",true},
            {"address-hint",nullptr},{"element-type",*$2},
            {"amount-of-elements",*$4},{"constructor-name",nullptr},
            {"constructor-arguments",nlohmann::ordered_json::array()}
        });
        delete $2; delete $4;
    }
    | TOK_NEW TYPE_SPEC %prec UNARY_PREC {
        $$ = new nlohmann::ordered_json({
            {"type","dynamic-memory-allocation"},{"array-flag",false},
            {"address-hint",nullptr},{"variable-type",*$2},
            {"constructor-name",nullptr},{"constructor-arguments",nlohmann::ordered_json::array()}
        });
        delete $2;
    }
    | TOK_NEW TYPE_SPEC TOK_LPAREN ARG_LIST_OPT TOK_RPAREN %prec UNARY_PREC {
        $$ = new nlohmann::ordered_json({
            {"type","dynamic-memory-allocation"},{"array-flag",false},
            {"address-hint",nullptr},{"variable-type",*$2},
            {"constructor-name",nullptr},{"constructor-arguments",*$4}
        });
        delete $2; delete $4;
    }
    | TOK_DELETE TOK_LBRACKET TOK_RBRACKET assign_expr %prec UNARY_PREC {
        $$ = new nlohmann::ordered_json({
            {"type","dynamic-memory-deallocation"},{"array-flag",true},
            {"element-type", nameToTypeJson(pointeeName(inferSimpleType($4)))},
            {"amount-of-elements",nullptr}, 
            {"pointer",*$4},{"destructor-name",nullptr},
            {"destructor-arguments",nlohmann::ordered_json::array()}
        });
        delete $4;
    }
    | TOK_DELETE assign_expr %prec UNARY_PREC {       
        $$ = new nlohmann::ordered_json({
            {"type","dynamic-memory-deallocation"},{"array-flag",false},
            {"variable-type", nameToTypeJson(pointeeName(inferSimpleType($2)))},{"pointer",*$2},
            {"destructor-name",nullptr},{"destructor-arguments",nlohmann::ordered_json::array()}
        });
        delete $2;
    }
    | assign_expr TOK_QUESTION expr TOK_COLON assign_expr %prec TOK_QUESTION {
        $$ = new nlohmann::ordered_json({
            {"type","conditional-expression"},
            {"result-type", unifyBranchTypes(inferSimpleType($3), inferSimpleType($5), $3, $5)},
            {"condition",*$1},{"if-true",*$3},{"if-false",*$5}
        });
        delete $1; delete $3; delete $5;
    }
    ;


INDEX_SUFFIX_LIST:
    TOK_LBRACKET expr TOK_RBRACKET {
        $$ = new nlohmann::ordered_json(nlohmann::ordered_json::array());
        $$->push_back(*$2);
        delete $2;
    }
    | INDEX_SUFFIX_LIST TOK_LBRACKET expr TOK_RBRACKET {
        $1->push_back(*$3);
        $$ = $1;
        delete $3;
    }
    ;


ARG_LIST_OPT:
     { $$ = new nlohmann::ordered_json(nlohmann::ordered_json::array()); }
    | ARG_LIST { $$ = $1; }
    ;

ARG_LIST:
    assign_expr {
        $$ = new nlohmann::ordered_json(nlohmann::ordered_json::array());
        $$->push_back(*$1);
        delete $1;
    }
    | ARG_LIST TOK_COMMA assign_expr {
        $1->push_back(*$3);
        $$ = $1;
        delete $3;
    }
    ;

%%

void yyerror(const char *msg) {
    std::fprintf(stderr, "Parse error at line %d: %s\n", yylineno, msg);
}
