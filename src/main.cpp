#include <cstdio>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
nlohmann::ordered_json g_output;
extern int yyparse();
extern FILE *yyin;
extern int g_semanticErrorCount;
// extern std::string g_baseDir; 
int main(int argc, char **argv) {
    if (argc != 2) { std::cerr << "usage\n"; return 1; }
    yyin = std::fopen(argv[1], "r");
    if (!yyin) { std::cerr << "cannot open file: " << argv[1] << "\n"; return 1; }
    // {
    //     std::string p = argv[1];
    //     size_t slash = p.rfind('/');
    //     g_baseDir = (slash == std::string::npos) ? "." : p.substr(0, slash);
    // }
    int r = yyparse();
    if (r != 0) { std::cerr << "parse failed\n"; return 1; }
    if (g_semanticErrorCount > 0) {
        std::cerr << g_semanticErrorCount << " semantic error(s), no output produced\n";
        return 1;
    }
    std::cout << g_output.dump(2) << std::endl;
    return 0;
}