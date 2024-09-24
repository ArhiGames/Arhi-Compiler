#pragma once

#include "Types.h"
#include <iostream>
#include <functional>
#include <vector>

struct Token;
enum class ETokenType : uint8;

class Tokenizer
{
public:
	Tokenizer() = delete;
	explicit Tokenizer(const std::string& source_code, std::function<void(const std::vector<std::vector<Token>>&)> callback)
		: m_SourceCode(source_code), m_OnCompletionEvent(callback)
	{
	}
	~Tokenizer() = default;

	void Tokenize();

private:
	void TokenizeSingleLine(const std::string& source_line, const uint32 line_number);
	bool IsOperator(const std::string& string_to_check) const;
	bool IsBooleanOperator(const std::string& string_to_check) const;

private:
	std::string m_SourceCode = {};
	std::vector<std::vector<Token>> m_Tokens = {};

	bool m_bIsInComment = false;

private:
	std::function<void(const std::vector<std::vector<Token>>&)> m_OnCompletionEvent;
};

enum class ETokenType : uint8
{
	Variable = 0,
	Name = 1,
	Numeric = 2,
	Keyword = 3,
	Macro = 4,
	Operator = 5,
	BooleanOperator = 6,
	Comment = 7,
	Semicolon = 8,
	Assignment = 9,
	Referral = 10,
	Parenthesis = 11,
	Scope = 12,
	IndexOperator = 13,
	Unkown = 14
};

struct Token
{
	ETokenType type = ETokenType::Unkown;
	std::string value = "";
	uint32 line = 0;

	bool empty() const 
	{
		return value == "" && line == 0 && type == ETokenType::Unkown;
	}

	Token() = default;
	Token(ETokenType token, const std::string& value, uint32 line)
		: type(token), value(value), line(line)
	{
	}
	~Token() = default;
};
