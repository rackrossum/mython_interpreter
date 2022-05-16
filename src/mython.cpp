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
#ifdef TEST
  TestAll();
#endif


  RunMythonProgram(cin, cout);

  return 0;
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
