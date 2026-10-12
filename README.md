dependencies: <nlohmann/json.hpp> 3.12
flex 2.6.4
bison 3.8.2

Примичания: nlohmann на fedora называется json-devel

initialize flex and bison (внутри папки src):

bison -d -o parser.tab.cpp parser.y
flex -o lexer.yy.cpp lexer.l

fedora:
g++ -std=c++17 -o fullparser main.cpp parser.tab.cpp lexer.yy.cpp 

mac: 

g++ -std=c++17 -I/opt/homebrew/include -o fullparser main.cpp parser.tab.cpp lexer.yy.cpp

compilation: src/main.cpp

run: In Parser_project:  src/fullparser <path_to_test_file>
example: src/fullparser tests/inputs/arith_int.cpp

OR run all tests in tests/input : ./tests/run_tests.sh