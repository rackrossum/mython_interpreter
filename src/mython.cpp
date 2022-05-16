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

void RunMythonProgram(istream& input, ostream& output) {
  Ast::Print::SetOutputStream(output);

  Parse::Lexer lexer(input);
  auto program = ParseProgram(lexer);

  Runtime::Closure closure;
  program->Execute(closure);
}

int main() {
#ifdef TEST
  TestAll();
#endif


  RunMythonProgram(cin, cout);

  return 0;
}

void TestSimplePrints() {
  istringstream input(R"(
print 57
print 10, 24, -8
print 'hello'
print "world"
print True, False
print
print None
)");

  ostringstream output;
  RunMythonProgram(input, output);

  ASSERT_EQUAL(output.str(), "57\n10 24 -8\nhello\nworld\nTrue False\n\nNone\n");
}

void TestAssignments() {
  istringstream input(R"(
x = 57
print x
x = 'C++ black belt'
print x
y = False
x = y
print x
x = None
print x, y
)");

  ostringstream output;
  RunMythonProgram(input, output);

  ASSERT_EQUAL(output.str(), "57\nC++ black belt\nFalse\nNone False\n");
}

void TestArithmetics() {
  istringstream input(
    "print 1+2+3+4+5, 1*2*3*4*5, 1-2-3-4-5, 36/4/3, 2*5+10/2"
  );

  ostringstream output;
  RunMythonProgram(input, output);

  ASSERT_EQUAL(output.str(), "15 120 -13 3 15\n");
}

void TestVariablesArePointers() {
  istringstream input(R"(
class Counter:
  def __init__():
    self.value = 0

  def add():
    self.value = self.value + 1

class Dummy:
  def do_add(counter):
    counter.add()

x = Counter()
y = x

x.add()
y.add()

print x.value

d = Dummy()
d.do_add(x)

print y.value
)");

  ostringstream output;
  RunMythonProgram(input, output);

  ASSERT_EQUAL(output.str(), "2\n3\n");
}

void TestCase3()
{
    istringstream input(R"(
x = 1
xx = 2
_xxx = 3
x4 = 4
x_5 = 5

str_ = 'string'
str_2 = str_
str_3 = str_2

print str(str_ + str_2 + str_3)
)");
ostringstream output;
RunMythonProgram(input, output);

ASSERT_EQUAL(output.str(), "stringstringstring\n");
}

void TestCase6()
{
    istringstream input(R"(
x = 4
y = 5

if x > y:
  print "x > y"
else:
  print "x <= y"

if x > 0:
  if y < 0:
    print "y < 0"
  else:
    print "y >= 0"
else:
  print 'x <= 0'

x = 3
y = -3

if x > 0:
  if y < 0:
    print "y < 0"
else:
  print 'x <= 0'

x = -4
y = -4

if x > 0:
  if y < 0:
    print "y < 0"
else:
  print 'x <= 0'

x = ""

if x:
  print '"" is True'
else:
  print '"" is False'

x = 'non-empty string'

if x:
  print 'non-empty string is True'
else:
  print 'non-empty string is False'

x = 0

if x:
  print '0 is True'
else:
  print '0 is False'

x = 100

if x:
  print '100 is True'
else:
  print '100 is False'

x = None

if x:
  print 'None is True'
else:
  print 'None is False'
)");

ostringstream output;
RunMythonProgram(input, output);
}

void TestAll() {
  TestRunner tr;
  Runtime::RunObjectHolderTests(tr);
  Runtime::RunObjectsTests(tr);
  Ast::RunUnitTests(tr);
  Parse::RunLexerTests(tr);
  TestParseProgram(tr);

  RUN_TEST(tr, TestSimplePrints);
  RUN_TEST(tr, TestAssignments);
  RUN_TEST(tr, TestArithmetics);
  RUN_TEST(tr, TestVariablesArePointers);
  RUN_TEST(tr, TestCase3);
  RUN_TEST(tr, TestCase6);
}
