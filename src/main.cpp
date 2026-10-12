#include <cstdio>
#include <iostream>
#include <nlohmann/json.hpp>
nlohmann::ordered_json g_output;
extern int yyparse();
extern FILE *yyin;
extern int g_semanticErrorCount;
int main(int argc, char **argv) {
    if (argc != 2) { std::cerr << "usage\n"; return 1; }
    yyin = std::fopen(argv[1], "r");
    if (!yyin) { std::cerr << "cannot open file: " << argv[1] << "\n"; return 1; }
    int r = yyparse();
    if (r != 0) { std::cerr << "parse failed\n"; return 1; }
    if (g_semanticErrorCount > 0) {
        std::cerr << g_semanticErrorCount << " semantic error(s), no output produced\n";
        return 1;
    }
    std::cout << g_output.dump(2) << std::endl;
    return 0;
}