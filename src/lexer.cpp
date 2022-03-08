#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <set>

using namespace std;

namespace 
{
    using namespace Parse;
    std::unordered_map<std::string_view, Parse::Token> keywords
    {
        {"class",TokenType::Class()},
        {"return", TokenType::Return()},
        {"if", TokenType::If()},
        {"else", TokenType::Else()},
        {"def", TokenType::Def()},
        {"print", TokenType::Print()},
        {"and", TokenType::And()},
        {"or", TokenType::Or()},
        {"not", TokenType::Not()},
        {"None", TokenType::None()},
        {"True", TokenType::True()},
        {"False", TokenType::False()}
    };

    std::unordered_map<std::string_view, Parse::Token> operators
    {
        {"\n", TokenType::Newline()},
        {"==", TokenType::Eq()},
        {"!=", TokenType::NotEq()},
        {"<=", TokenType::LessOrEq()},
        {">=", TokenType::GreaterOrEq()},
        {"+", TokenType::Char{'+'}},
        {"-", TokenType::Char{'-'}},
        {"*", TokenType::Char{'*'}},
        {"/", TokenType::Char{'/'}},
        {".", TokenType::Char{'.'}},
        {">", TokenType::Char{'>'}},
        {"<", TokenType::Char{'<'}},
        {",", TokenType::Char{','}},
        {"(", TokenType::Char{'('}},
        {")", TokenType::Char{')'}},
        {"=", TokenType::Char{'='}},
        {":", TokenType::Char{':'}},
        {"?", TokenType::Char{'?'}}
    };

    std::string knownSymbols = "\n=+-*/.><,(): !?";
    std::set<char> singleLetterTokens2 = {'\n', '=', '+', '-', '*', '/',
        '.', '>', '<', ',', '(', ')', ':', ' '};
}

namespace Parse {

    bool operator == (const Token& lhs, const Token& rhs) {
        using namespace TokenType;

        if (lhs.index() != rhs.index()) {
            return false;
        }
        if (lhs.Is<Char>()) {
            return lhs.As<Char>().value == rhs.As<Char>().value;
        } else if (lhs.Is<Number>()) {
            return lhs.As<Number>().value == rhs.As<Number>().value;
        } else if (lhs.Is<String>()) {
            return lhs.As<String>().value == rhs.As<String>().value;
        } else if (lhs.Is<Id>()) {
            return lhs.As<Id>().value == rhs.As<Id>().value;
        } else {
            return true;
        }
    }

    std::ostream& operator << (std::ostream& os, const Token& rhs) {
        using namespace TokenType;

#define VALUED_OUTPUT(type) \
        if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

        VALUED_OUTPUT(Number);
        VALUED_OUTPUT(Id);
        VALUED_OUTPUT(String);
        VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
        if (rhs.Is<type>()) return os << #type;

        UNVALUED_OUTPUT(Class);
        UNVALUED_OUTPUT(Return);
        UNVALUED_OUTPUT(If);
        UNVALUED_OUTPUT(Else);
        UNVALUED_OUTPUT(Def);
        UNVALUED_OUTPUT(Newline);
        UNVALUED_OUTPUT(Print);
        UNVALUED_OUTPUT(Indent);
        UNVALUED_OUTPUT(Dedent);
        UNVALUED_OUTPUT(And);
        UNVALUED_OUTPUT(Or);
        UNVALUED_OUTPUT(Not);
        UNVALUED_OUTPUT(Eq);
        UNVALUED_OUTPUT(NotEq);
        UNVALUED_OUTPUT(LessOrEq);
        UNVALUED_OUTPUT(GreaterOrEq);
        UNVALUED_OUTPUT(None);
        UNVALUED_OUTPUT(True);
        UNVALUED_OUTPUT(False);
        UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

        return os << "Unknown token :(";
    }


    Lexer::Lexer(std::istream& input)
        : in(input)
    {
        token = NextToken();
    }

    void Lexer::ParseIndents(std::string_view line)
    {
        curIndent = line.find_first_not_of(" ");
        if (curIndent == std::string_view::npos)
            curIndent = 0;

        if (curIndent % 2 != 0)
            throw LexerError(__FUNCTION__);
        curIndent /= 2;

        for (int i = 0; i < std::abs(int(curIndent) - int(prevIndent)); ++i)
        {
            if (curIndent > prevIndent)
                lineTokens.push_back(TokenType::Indent{});
            else 
                lineTokens.push_back(TokenType::Dedent{});
        }

        prevIndent = curIndent;
    }

    const Token& Lexer::CurrentToken() const 
    {
        return token;
    }


    Token Lexer::NextToken()
    {
        if (!lineTokens.empty())
        {
            token = lineTokens.front();
            lineTokens.pop_front();
            return token;
        }

        if (in.peek() == EOF)
        {
            if (!lastIndentCounted)
            {
                std::string_view _;
                ParseIndents(_);
                lastIndentCounted = true;
                return NextToken();
            }

            return TokenType::Eof();
        }

        ParseLine();
        return NextToken();
    }

    void Lexer::ParseLine() 
    {
        std::string line;

        while ((in.peek() != EOF) && (line.find_first_not_of(" \n") == std::string::npos))
        {
            line = GetLine();
        }


        std::string_view svLine = line;

        ParseIndents(svLine);

        if (in.peek() == EOF)
            lastIndentCounted = true;

        while (!svLine.empty())
        {
            while (svLine.front() == ' ')
                svLine.remove_prefix(1);

            OToken str = TryParseString(svLine);
            if (str)
            {
                lineTokens.push_back(str.value());
                continue;
            }

            auto isSymbol = knownSymbols.find(svLine.front()) != std::string::npos;
            auto pos = !isSymbol ?
                svLine.find_first_of(knownSymbols, 1) :
                svLine.find_first_not_of(knownSymbols, 1);

            auto substr = svLine.substr(0, pos);
            svLine.remove_prefix(substr.size());

            if (isSymbol)
            {
                while (!substr.empty())
                {
                    while (substr.front() == ' ')
                        substr.remove_prefix(1);

                    OToken t = TryParseKnownToken(substr);
                    if (t)
                    {
                        lineTokens.push_back(t.value());
                    }
                }
            }
            else
                lineTokens.push_back(ParseUnknownToken(substr));
        }
    }

    Lexer::OToken Lexer::TryParseKnownToken(std::string_view& s) const
    {
        auto lookUp = [&](size_t size)
        {
            Lexer::OToken t;
            auto it = operators.find(s.substr(0, size));
            if (it != operators.end())
            {
                t = it->second;
                s.remove_prefix(it->first.size());
            }

            return t;
        };

        Lexer::OToken res = lookUp(2);
        if (res)
            return res;

        res = lookUp(1);
        if (res)
            return res;

        return nullopt;
    }

    Token Lexer::ParseUnknownToken(std::string_view s) const
    {
        int value;
        auto [ptr, ec] { std::from_chars(s.data(), s.data() + s.size(), value) };
        if (ec == std::errc())
            return TokenType::Number{value};

        auto it = keywords.find(s);
        if (it != keywords.end())
            return it->second;

        return TokenType::Id{std::string(s)};
    }

    Lexer::OToken Lexer::TryParseString(std::string_view& s) const
    {
        if (s.size() < 2)
            return nullopt;

        auto start = s.front();
        
        if (start == '\"' || start == '\'')
        {
            auto end = s.find(start, 1);
            if (end == s.npos)
                throw LexerError(__FUNCTION__);
            auto string = s.substr(1, end - 1);
            s.remove_prefix(end + 1);
            return TokenType::String{std::string(string)};
        }

        return nullopt;
    }

    std::string Lexer::GetLine() 
    {
        if (in.peek() == EOF)
            return "";
 
        std::string res;
        getline(in, res);
        if (res != "")
            res.push_back('\n');

        return res;
    }

} /* namespace Parse */
