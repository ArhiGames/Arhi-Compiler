#pragma once

#include <iostream>
#include <vector>
#include <ostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stack>
#include "Types.h"
#include "Tokenizer.h"

enum class ECompileErrorType : uint8;
enum class EAssignmentType : uint8;
struct Token;
struct Variable;
struct Function;

class Compiler
{
public:
	Compiler() = default;
	~Compiler() = default;

public:
	int32 Compile(const std::vector<std::vector<Token>>& tokens);

private:
	void CompileToken(const std::vector<Token>& tokens, std::ofstream& output_file, bool& bUseExitCode);
	void CreateStandardAssembly(std::ofstream& output_file);
	void CreateStandardExitAssemblyCode(std::ofstream& output_file);
	bool IsCorrectVariableName(const std::string& variable_name, const std::string& result) const;
	bool IsCorrectFunctionName(const std::string& function_name, const std::string& result) const;
	bool CheckTypeSize(const Variable& variablea, const Variable& variableb) const;

	std::string GetMathematicResultIntoRegister(std::vector<Token> tokens, const int32 register_size, std::ofstream& output_file);
	std::string PerformMathematicTask(const std::string& first_value, const std::string& second_value, const int32 register_size, const char operation, const bool b_first_operation, std::ofstream& output_file);
	int32 Precedence(char op);

	int32 GetVariableSize(const std::string& variable_type) const;

	Variable GetLocalVariableReference(const std::string variable_name) const;
	Function GetFunction(const std::string& function_name) const;

	std::string GetCorrectVariableMathematicsRegisterGrade1(int32 variable_size) const;
	std::string GetCorrectVariableMathematicsRegisterGrade2(int32 variable_size) const;
	std::string GetCorrectVariableMathematicsRegisterGrade3(int32 variable_size) const;
	std::string GetCorrectVariableMathematicsRegisterGrade4(int32 variable_size) const;
	std::string GetParameterRegister(const uint32 parameter_num, const int32 parameter_size) const;

	void Compare(const std::vector<Token>& left, const std::vector<Token>& right, std::ofstream& output_file);
	bool IsBoolean(const std::string& variable_type) const;
	bool IsBoolean(const Variable& variable_type) const;
	bool IsComplexIfStatement(const std::vector<Token>& tokens) const;
	EAssignmentType GetAssignmentType(const std::string& variable_type) const;

	std::string TokenTypeToString(ETokenType type) const;
	std::string GetAssemblyTypesizeSpecifier(const int32 size) const;

	std::string GetConditionCodeEnding(const Token& condition) const;
	void MoveByCondition(const std::vector<Token>& ifworth, const std::vector<Token>& elseworth, const Token& condition, const std::string& expected_location, const int32 result_size, std::ofstream& output_file);

	void Move(std::ofstream& output_file, const std::string& destination, const std::string& source, const int32 destination_size, const int32 source_size);

	void HandleNegateMacro(const std::vector<Token>& tokens, std::ofstream& output_file);
	void HandleClampMacro(const std::vector<Token>& tokens, std::ofstream& output_file);
	void HandleRepeatMacro(const std::vector<Token>& tokens, std::ofstream& output_file);
	void HandleSwapMacro(const std::vector<Token>& tokens, std::ofstream& output_file);

	void HandleMacros(const std::vector<Token>& tokens, std::ofstream& output_file, bool& bUseExitCode);
	void HandleScope(const std::vector<Token>& tokens, std::ofstream& output_file);
	bool HandleVariableChanges(const std::vector<Token>& tokens, std::ofstream& output_file);
	void HandleVariableDecleration(const std::vector<Token>& tokens, std::ofstream& output_file);
	void HandleVariableParameters(const std::vector<Variable>& parameters, std::ofstream& output_file);
	void HandleFunctionDecleration(const std::vector<Token>& tokens, std::ofstream& output_file);
	int32 HandleFunctionCall(const std::vector<Token>& tokens, std::ofstream& output_file);
	void HandleReturnKeyword(const std::vector<Token>& tokens, std::ofstream& output_file);

	bool HandleComplexAssignment(const std::vector<Token>& tokens, std::ofstream& output_file, const std::string& expected_result_location, const int32 result_size, EAssignmentType assignment_type);
	bool HandleComplexBooleanAssignment(const std::vector<Token>& tokens, std::ofstream& output_file, const std::string& expected_result_location, const int32 result_size);

	bool CheckforSymicolon(const Token& token_to_check);

private:
	int32 m_DataSectionIndex = 1;
	int32 m_BssSectionIndex = 2;
	std::vector<int32> m_CurrentStacksizes = {};
	std::vector<std::vector<Variable>> m_LocalVariables = {};
	std::vector<Function> m_Functions = {};
	Function* m_pCurrentFunction = 0;
	int32 m_RemainingFunctionScopes = 0;
	int32 m_CurrentLine = 0;
	int32 m_SectionNumber = 0;
};

enum class ECompileErrorType : uint8
{
	None,
	SyntaxError,
	MissingSymbol
};

enum class EAssignmentType : uint8
{
	Integer = 0,
	FloatingPoint = 1,
	Boolean = 2,
	NotSpecified = 3
};

struct Variable
{
	std::string variable_name = {};
	std::string variable_assembly_safe = {};
	std::string type = {};
	uint32 type_size = 4;
	bool bUnsigned = false;
	bool bChangable = false;
	bool is_boolean = false;
	bool bIsArray = false;

	Variable() = default;
	explicit Variable(const std::string& variable_name, const std::string& variable_assembly_safe, const std::string& type,
		uint32 type_size, bool bUnsigned, bool bChangable, bool is_boolean, bool bIsArray)
		: variable_name(variable_name), variable_assembly_safe(variable_assembly_safe), type(type),
		type_size(type_size), bUnsigned(bUnsigned), bChangable(bChangable), is_boolean(is_boolean), bIsArray(bIsArray)
	{
	}
	~Variable() = default;
};

struct Function 
{
	std::string function_name = {};
	int32 return_size = {};
	std::vector<Variable> function_parameters = {};
	std::string return_type = {};

	explicit Function() = default;
	explicit Function(const std::string& function_name, const int32 return_size,
		const std::vector<Variable>& function_parameters, const std::string& return_type)
		: function_name(function_name), return_size(return_size), function_parameters(function_parameters),
		return_type(return_type)
	{
	}
	~Function() = default;
};

