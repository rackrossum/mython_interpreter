#pragma once

#include <iosfwd>
#include <string>
#include <sstream>
#include <string_view>
#include <variant>
#include <stdexcept>
#include <optional>
#include <list>

class TestRunner;

namespace Parse {

    namespace TokenType {
        struct Number {
            int value;
        };

        struct Id {
            std::string value;
        };

        struct Char {
            char value;
        };

        struct String {
            std::string value;
        };

        struct Class{};
        struct Return{};
        struct If {};
        struct Else {};
        struct Def {};
        struct Newline {};
        struct Print {};
        struct Indent {};
        struct Dedent {};
        struct Eof {};
        struct And {};
        struct Or {};
        struct Not {};
        struct Eq {};
        struct NotEq {};
        struct LessOrEq {};
        struct GreaterOrEq {};
        struct None {};
        struct True {};
        struct False {};
    }

    using TokenBase = std::variant<
        TokenType::Number,
        TokenType::Id,
        TokenType::Char,
        TokenType::String,
        TokenType::Class,
        TokenType::Return,
        TokenType::If,
        TokenType::Else,
        TokenType::Def,
        TokenType::Newline,
        TokenType::Print,
        TokenType::Indent,
        TokenType::Dedent,
        TokenType::And,
        TokenType::Or,
        TokenType::Not,
        TokenType::Eq,
        TokenType::NotEq,
        TokenType::LessOrEq,
        TokenType::GreaterOrEq,
        TokenType::None,
        TokenType::True,
        TokenType::False,
        TokenType::Eof
            >;


    //По сравнению с условием задачи мы добавили в тип Token несколько
    //удобных методов, которые делают код короче. Например,
    //
    //token.Is<TokenType::Id>()
    //
    //гораздо короче, чем
    //
    //std::holds_alternative<TokenType::Id>(token).
    struct Token : TokenBase {
        using TokenBase::TokenBase;

        template <typename T>
            bool Is() const {
                return std::holds_alternative<T>(*this);
            }

        template <typename T>
            const T& As() const {
                return std::get<T>(*this);
            }

        template <typename T>
            const T* TryAs() const {
                return std::get_if<T>(this);
            }
    };

    bool operator == (const Token& lhs, const Token& rhs);
    std::ostream& operator << (std::ostream& os, const Token& rhs);

    class LexerError : public std::runtime_error {
        public:
            using std::runtime_error::runtime_error;
    };

    class Lexer {
        private:
            template <typename T>
            void CheckToken() const
            {
                if (!token.Is<T>())
                    throw LexerError(std::string(__FUNCTION__) + ": " + typeid(token).name());
            }

        public:
            explicit Lexer(std::istream& input);

            const Token& CurrentToken() const;
            Token NextToken();

            template <typename T>
                const T& Expect() const {
                    CheckToken<T>();

                    return token.As<T>();
                }

            template <typename T, typename U>
                void Expect(const U& value) const {
                    if (Expect<T>().value != value)
                        throw LexerError("");
                }

            template <typename T>
                const T& ExpectNext() {
                    token = NextToken();

                    return Expect<T>();
                }

            template <typename T, typename U>
                void ExpectNext(const U& value) {
                    token = NextToken();

                    Expect<T, U>(value);
                }

        private:
            using OToken = std::optional<Token>;

            void ParseLine();

            void ParseIndents(std::string_view s);

            OToken TryParseString(std::string_view& s) const;
            OToken TryParseKnownToken(std::string_view& s) const;
            Token ParseUnknownToken(std::string_view s) const;

            std::string GetLine();

        private:
            size_t prevIndent = 0;
            size_t curIndent = 0;

            Token token = TokenType::Eof();
            std::istream& in;

            std::list<Token> lineTokens;
            bool lastIndentCounted = false;
    };

    void RunLexerTests(TestRunner& test_runner);

} /* namespace Parse */
