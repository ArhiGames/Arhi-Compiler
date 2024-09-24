#include "Tokenizer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

std::ostream& operator<<(std::ostream& os, ETokenType tokenType) {
    switch (tokenType) {
    case ETokenType::Variable:
        os << "Variable";
        break;
    case ETokenType::Name:
        os << "Name";
        break;
    case ETokenType::Numeric:
        os << "Numeric";
        break;
    case ETokenType::Keyword:
        os << "Keyword";
        break;
    case ETokenType::Macro:
        os << "Macro";
        break;
    case ETokenType::Operator:
        os << "Operator";
        break;
    case ETokenType::BooleanOperator:
        os << "Boolean operator";
        break;
    case ETokenType::IndexOperator:
        os << "index operator";
        break;
    case ETokenType::Comment:
        os << "Comment";
        break;
    case ETokenType::Semicolon:
        os << "Semicolon";
        break;
    case ETokenType::Assignment:
        os << "Assignment";
        break;
    case ETokenType::Referral:
        os << "Referral";
        break;
    case ETokenType::Parenthesis:
        os << "Parenthesis";
        break;
    case ETokenType::Scope:
        os << "Scope";
        break;
    case ETokenType::Unkown:
        os << "Unkown";
        break;
    default:
        os << "Unknown Token Type";
        break;
    }
    return os;
}

const std::vector<std::string> variables = { "bool", "boolean", "byte", "int8", "uint8", "int16", "uint16", "int32", "uint32", "int64", "uint64", "void" };
const std::vector<std::string> operators = { "++", "--", "->", "+", "-", "*", "/", "," };
const std::vector<std::string> boolean_operators = { "?", "<=", "<", ">=", ">", "==", "!=" };
const std::vector<std::string> keywords = { "global", "local", "if", "define", "return", "true", "false" };
const std::vector<std::string> arhi_macros = { "exit!", "negate!", "clamp!", "repeat!", "swap!" };

void Tokenizer::Tokenize()
{
    std::stringstream input = std::stringstream(m_SourceCode);
	std::string line = { };
    uint32 line_number = 1;

    while (std::getline(input, line))
    {
        TokenizeSingleLine(line, line_number);
        line_number++;
    }

	m_OnCompletionEvent(m_Tokens);
}

void Tokenizer::TokenizeSingleLine(const std::string& source_line, const uint32 line_number)
{
    size_t i = 0;
    const size_t length = source_line.length();
    std::vector<Token> line_tokens = { };

    while (i < length)
    {
        const char current_symbol = source_line[i];

        if (current_symbol == '/')
        {
            if (i + 1 < length) 
            {
                if (source_line[i + 1] == '/') break;
                else if (source_line[i + 1] == '*') m_bIsInComment = true;
            }
        }
        else if (current_symbol == '*')
        {
            if (i + 1 < length)
            {
                if (source_line[i + 1] == '/') m_bIsInComment = false;
            }
        }
        if (m_bIsInComment)
        {
            i++;
            continue;
        }

        if (!std::isspace(current_symbol))
        {
            if (std::isalpha(current_symbol) || current_symbol == '_')
            {
                bool bShouldBreak = false;

                std::string type = {};
                while (i < length && !std::isspace(source_line[i]) && source_line[i] != ':' 
                    && source_line[i] != '(' && source_line[i] != ')' && !IsOperator(std::string(1, source_line[i]))
                    && source_line[i] != ';' && source_line[i] != '[' && source_line[i] != ']')
                {
                    type.push_back(source_line[i]);
                    i++;
                }

                for (const std::string& variable : variables)
                {
                    if (variable == type)
                    {
                        line_tokens.push_back({ ETokenType::Variable, type, line_number });
                        bShouldBreak = true;
                        break;
                    }
                }
                if (bShouldBreak) continue;
                for (const std::string& macro : arhi_macros)
                {
                    if (macro == type)
                    {
                        line_tokens.push_back({ ETokenType::Macro, type, line_number });
                        bShouldBreak = true;
                        break;
                    }
                }
                if (bShouldBreak) continue;
                for (const std::string& keyword : keywords)
                {
                    if (type == keyword)
                    {
                        line_tokens.push_back({ ETokenType::Keyword, type, line_number });
                        bShouldBreak = true;
                        break;
                    }
                }
                if (bShouldBreak) continue;

                line_tokens.push_back({ ETokenType::Name, type, line_number });  
                if (source_line[i] == '(' || source_line[i] == ')' || IsOperator(std::string(1, source_line[i]))
                    || source_line[i] == ';' || source_line[i] == ':' || source_line[i] != '[' || source_line[i] != ']') i--;
            }
            else if (std::isdigit(current_symbol) || (current_symbol == '-' && std::isdigit(source_line[i + 1])))
            {
                std::string number = {};
                while (i < length && (std::isdigit(source_line[i]) || source_line[i] == '-'))
                {
                    number.push_back(source_line[i]);
                    i++;
                }
                line_tokens.push_back({ ETokenType::Numeric, number, line_number });
                continue;
            }
            else if (IsBooleanOperator(std::string(1, current_symbol)) || source_line[i] == '=' || source_line[i] == '!')
            {
                std::string arhioperator = {};
                while (IsBooleanOperator(std::string(1, source_line[i])) || source_line[i] == '=' || source_line[i] == '!')
                {
                    arhioperator.push_back(source_line[i]);
                    i++;
                }

                if (IsBooleanOperator(arhioperator))
                {
                    line_tokens.push_back({ ETokenType::BooleanOperator, arhioperator, line_number });
                    continue;
                }
                if (source_line[i] == ';' || std::isdigit(source_line[i]) || std::isalpha(source_line[i])) i--;
            }
            else if (IsOperator(std::string(1, current_symbol)) || source_line[i] == '>')
            {
                std::string arhioperator = {};
                while (IsOperator(std::string(1, source_line[i])) || source_line[i] == '>')
                {
                    arhioperator.push_back(source_line[i]);
                    i++;
                }

                if (IsOperator(arhioperator))
                {
                    line_tokens.push_back({ ETokenType::Operator, arhioperator, line_number });
                    continue;
                }
                if (source_line[i] == ';' || std::isdigit(source_line[i]) || std::isalpha(source_line[i])) i--;
            }
            if (current_symbol == '(' || current_symbol == ')')
            {
                const std::string value = std::string(1, current_symbol);
                line_tokens.push_back({ ETokenType::Parenthesis, value, line_number });
            }
            else if (current_symbol == '[' || current_symbol == ']')
            {
                const std::string value = std::string(1, current_symbol);
                line_tokens.push_back({ ETokenType::IndexOperator, value, line_number });
            }
            else if (current_symbol == '{' || current_symbol == '}')
            {
                const std::string value = std::string(1, current_symbol);
                line_tokens.push_back({ ETokenType::Scope, value, line_number });
            }
            else if (current_symbol == '=')
            {
                line_tokens.push_back({ ETokenType::Assignment, "=", line_number });
            }
            else if (current_symbol == ':')
            {
                line_tokens.push_back({ ETokenType::Referral, ":", line_number });
            }
            else if (current_symbol == ';')
            {
                line_tokens.push_back({ ETokenType::Semicolon, ";", line_number });
            }
        }

        i++;
    }

    for (const Token& token : line_tokens)
    {
        std::cout << "Typ: " << token.type << ", Wert: " << token.value << "\n";
    }
    std::cout << "\n";

    if (line_tokens.size() > 0)
    {
        m_Tokens.push_back(line_tokens);
    }
}

bool Tokenizer::IsOperator(const std::string& string_to_check) const
{
    const uint32 length = string_to_check.length();
    for (const std::string arhioperator : operators)
    {
        if (arhioperator == string_to_check)
        {
            return true;
        }
    }

    return false;
}

bool Tokenizer::IsBooleanOperator(const std::string& string_to_check) const
{
    const uint32 length = string_to_check.length();
    for (const std::string arhioperator : boolean_operators)
    {
        if (arhioperator == string_to_check)
        {
            return true;
        }
    }

    return false;
}
