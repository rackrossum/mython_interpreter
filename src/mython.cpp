#include "object.h"
#include "object_holder.h"
#include "statement.h"
#include "lexer.h"
#include "parse.h"

#include <test_runner.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

void TestAll();

void RunMythonProgram(istream& input, ostream& output);

int main() {
	int errcode = 0;
	try {
#ifdef TEST
		TestAll();
		std::cout << "\n\n\n";
#endif
		std::cout << "This is Mython intepreter. Indent is 2 spaces.\n";
		std::cout << "Type in EOF command after input(CTRL+d for Linux, CTRL+z for Windows)\n";
		RunMythonProgram(cin, cout);
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;	
		errcode = -1;
	}
	
#ifdef _WIN32  
	std::cout << "Press any key to continue..." << std::endl;
	std::getchar();
#endif
	return errcode;
}

void TestCases(TestRunner&);

void TestAll() {
  TestRunner tr;
  Runtime::RunObjectHolderTests(tr);
  Runtime::RunObjectsTests(tr);
  Ast::RunUnitTests(tr);
  Parse::RunLexerTests(tr);
  TestParseProgram(tr);
  TestCases(tr);
}
