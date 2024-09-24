#include "Compiler.h"
#include <algorithm>

const std::string ASSEMBLY_FILE_NAME = "arhi.asm";

namespace arhi
{
	template <typename T>
	constexpr T clamp(const T& value, const T& low, const T& high) 
	{
		return (value < low) ? low : (value > high) ? high : value;
	}
}

int32 Compiler::Compile(const std::vector<std::vector<Token>>& tokens)
{
	std::ofstream assembly_file = std::ofstream(ASSEMBLY_FILE_NAME);

	if (assembly_file.is_open())
	{
		CreateStandardAssembly(assembly_file);

		bool bHasExitCode = false;
		m_CurrentLine = 1;
		for (const std::vector<Token>& token_line : tokens)
		{
			CompileToken(token_line, assembly_file, bHasExitCode);
			m_CurrentLine++;

		}

		if (!bHasExitCode)
		{
			std::cerr << "[Error] Your programm has to use the exit! macro at the end of the programm!\n";
		}

		assembly_file.close();
	}

	return 0;
}

void Compiler::CompileToken(const std::vector<Token>& tokens, std::ofstream& output_file, bool& bUseExitCode)
{
	const size_t length = tokens.size();

	if (tokens[0].type == ETokenType::Macro)
	{
		CheckforSymicolon(tokens[length - 1]);
		HandleMacros(tokens, output_file, bUseExitCode);
	}
	else if (tokens[0].type == ETokenType::Scope)
	{
		HandleScope(tokens, output_file);
	}
	else if (tokens[0].type == ETokenType::Name)
	{
		CheckforSymicolon(tokens[length - 1]);
		if (tokens[1].value == "(")
		{
			HandleFunctionCall(tokens, output_file);
		}
		else
		{
			HandleVariableChanges(tokens, output_file);
		}
	}
	else if (tokens[0].type == ETokenType::Keyword) 
	{
		if (tokens[0].value == "define")
		{
			HandleFunctionDecleration(tokens, output_file);
		}
		else if (tokens[0].value == "return")
		{
			CheckforSymicolon(tokens[length - 1]);
			HandleReturnKeyword(tokens, output_file);
		}
		else if (tokens.size() > 3)
		{
			if (tokens[3].type == ETokenType::Variable)
			{
				CheckforSymicolon(tokens[length - 1]);
				HandleVariableDecleration(tokens, output_file);
			}
		}
	}
}

void Compiler::CreateStandardAssembly(std::ofstream& output_file)
{
	output_file << "section .data\n";
	output_file << "section .bss\n";
	output_file << "section .text\n";
	output_file << " global _start\n";
}

void Compiler::CreateStandardExitAssemblyCode(std::ofstream& output_file)
{
	output_file << " mov rax, 60\n";
	output_file << " mov rdi, 0\n";
	output_file << " syscall";
}

bool Compiler::IsCorrectVariableName(const std::string& variable_name, const std::string& result) const
{
	if (result.empty())
	{
		std::cerr << "[Error] There is no variable avaiable called '" << variable_name << "'!\n";
		return false;
	}

	return true;
}

bool Compiler::IsCorrectFunctionName(const std::string& function_name, const std::string& result) const
{
	if (result.empty())
	{
		std::cerr << "[Error] There is no method/function avaiable called '" << function_name << "'!\n";
		return false;
	}

	return true;
}

bool Compiler::CheckTypeSize(const Variable& variablea, const Variable& variableb) const
{
	if (variablea.type_size != variableb.type_size)
	{
		std::cerr << "[Error] The variable '" << variablea.variable_name << "' has to have the same type size as the variable '" << variableb.variable_name << "'! Line " << m_CurrentLine << "\n";
		return false;
	}

	return true;
}

std::string Compiler::GetMathematicResultIntoRegister(std::vector<Token> tokens, const int32 register_size, std::ofstream& output_file)
{
	int32 necessary_parenthesis_deletions = 0;
	for (int32 j = 0; j < tokens.size(); j++)
	{
		if (tokens[j].type == ETokenType::Parenthesis)
		{
			if (j == 0) continue;
			if (tokens[j].value == "(")
			{
				if (tokens[j - 1].value == "+")
				{
					necessary_parenthesis_deletions++;
					tokens.erase(tokens.begin() + j);
				}
			}
			else if (tokens[j].value == ")")
			{
				necessary_parenthesis_deletions--;
				if (necessary_parenthesis_deletions == 0)
				{
					tokens.erase(tokens.begin() + j);
				}
			}
		}
	}

	int32 i = 0;
	const size_t length = tokens.size();

	std::vector<std::string> values = {};
	std::vector<char> operators = {};

	bool b_first_operation = true;
	while (i < length)
	{
		if (tokens[i].type == ETokenType::Parenthesis)
		{
			if (tokens[i].value == "(")
			{
				operators.push_back('(');
			}
			else if (tokens[i].value == ")")
			{
				while (!operators.empty() && operators[operators.size() - 1] != '(')
				{
					const std::string second_value = values[values.size() - 1];
					values.pop_back();

					const std::string first_value = values[values.size() - 1];
					values.pop_back();

					char using_operator = operators[operators.size() - 1];
					operators.pop_back();

					values.push_back(PerformMathematicTask(first_value, second_value, register_size, using_operator, b_first_operation, output_file));
					b_first_operation = false;
				}
				if (!operators.empty())
				{
					operators.pop_back(); 
				}
			}
		}
		else if (tokens[i].type == ETokenType::Numeric || tokens[i].type == ETokenType::Name)
		{
			if (tokens[i].type == ETokenType::Numeric)
			{
				values.push_back(tokens[i].value);
			}
			else
			{
				if (length > i + 1)
				{
					if (tokens[i + 1].value == "(")
					{
						std::cerr << "[Error / Warning] You cannot use functions in mathematic operations! Use a temporal variable! Line " << m_CurrentLine << "\n";
						return "";
					}
				}

				const Variable variable_name = GetLocalVariableReference(tokens[i].value);
				if (!IsCorrectVariableName(tokens[i].value, variable_name.variable_name)) return "";
				values.push_back(variable_name.variable_assembly_safe + "]");
			}
		}
		else if (tokens[i].type == ETokenType::Keyword)
		{
			std::cerr << "[Error] You cannot use keywords in mathematic operations! Line " << m_CurrentLine << "\n";
			return "";
		}
		else if (tokens[i].type == ETokenType::Operator)
		{
			while (!operators.empty() && Precedence(operators[operators.size() - 1]) > Precedence(tokens[i].value[0]))
			{
				const std::string second_value = values[values.size() - 1];
				values.pop_back();

				const std::string first_value = values[values.size() - 1];
				values.pop_back();

				char using_operator = operators[operators.size() - 1];
				operators.pop_back();

				values.push_back(PerformMathematicTask(first_value, second_value, register_size, using_operator, b_first_operation, output_file));
				b_first_operation = false;
			}
			operators.push_back(tokens[i].value[0]);
		}

		i++;
	}

	while (!operators.empty())
	{
		const std::string second_value = values[values.size() - 1];
		values.pop_back();

		const std::string first_value = values[values.size() - 1];
		values.pop_back();

		char using_operator = operators[operators.size() - 1];
		operators.pop_back();

		values.push_back(PerformMathematicTask(first_value, second_value, register_size, using_operator, b_first_operation, output_file));
		b_first_operation = false;
	}

	return GetCorrectVariableMathematicsRegisterGrade1(register_size);
}

std::string Compiler::PerformMathematicTask(const std::string& first_value, const std::string& second_value, const int32 register_size, const char operation, const bool b_first_operation, std::ofstream& output_file)
{
	const std::string register_first_grade = GetCorrectVariableMathematicsRegisterGrade1(register_size);
	const std::string register_second_grade = GetCorrectVariableMathematicsRegisterGrade2(register_size);
	const std::string register_third_grade = GetCorrectVariableMathematicsRegisterGrade3(register_size);

	if (b_first_operation)
	{
		output_file << " mov " << register_first_grade << ", " << first_value << "\n";
		output_file << " mov " << register_second_grade  << ", " << second_value << "\n";
	}
	else
	{
		if (first_value == register_first_grade || first_value == register_second_grade || first_value == register_third_grade)
		{
			output_file << " mov " << register_second_grade << ", " << second_value << "\n";
		}
		else
		{
			output_file << " mov " << register_second_grade << ", " << first_value << "\n";
		}
	}

	if (operation == '+')
	{
		output_file << " add " << register_first_grade << ", " << register_second_grade << "\n";
	}
	else if (operation == '-')
	{
		output_file << " sub " << register_first_grade << ", " << register_second_grade << "\n";
	}
	else if (operation == '*')
	{
		if (register_size == 1)
		{
			output_file << " mul " << register_second_grade << "\n";
		}
		else
		{
			output_file << " imul " << register_first_grade << ", " << register_second_grade << "\n";
		}
	}

	return register_first_grade;
}

int32 Compiler::Precedence(char op)
{
	if (op == '+' || op == '-') return 1;
	else if (op == '*' || op == '/') return 2;
	return 0;
}

int32 Compiler::GetVariableSize(const std::string& variable_type) const
{
	if (variable_type == "int64" || variable_type == "uint64") return 8;
	else if (variable_type == "int32" || variable_type == "uint32") return 4;
	else if (variable_type == "int16" || variable_type == "uint16") return 2;
	else if (variable_type == "int8" || variable_type == "uint8" || variable_type == "byte" 
		|| variable_type == "bool" || variable_type == "boolean") return 1;
	else if (variable_type == "void") return 0;
}

Variable Compiler::GetLocalVariableReference(const std::string variable_name) const
{
	for (const std::vector<Variable>& variable_reference_list : m_LocalVariables)
	{
		for (const Variable& variable_reference : variable_reference_list)
		{
			if (variable_reference.variable_name == variable_name)
			{
				return variable_reference;
			}
		}
	}

	return Variable();
}

Function Compiler::GetFunction(const std::string& function_name) const
{
	for (const Function& function : m_Functions)
	{
		if (function.function_name.empty()) continue;
		if (function.function_name == function_name)
		{
			return function;
		}
	}

	return Function();
}

std::string Compiler::GetCorrectVariableMathematicsRegisterGrade1(int32 variable_size) const
{
	switch (variable_size)
	{
		case 8: return "rax";
		case 4: return "eax";
		case 2: return "ax";
		case 1: return "al";
		default: return "";
	}
}

std::string Compiler::GetCorrectVariableMathematicsRegisterGrade2(int32 variable_size) const
{
	switch (variable_size)
	{
		case 8: return "rbx";
		case 4: return "ebx";
		case 2: return "bx";
		case 1: return "bl";
		default: return "";
	}
}

std::string Compiler::GetCorrectVariableMathematicsRegisterGrade3(int32 variable_size) const
{
	switch (variable_size)
	{
		case 8: return "rcx";
		case 4: return "ecx";
		case 2: return "cx";
		case 1: return "cl";
		default: return "";
	}
}

std::string Compiler::GetCorrectVariableMathematicsRegisterGrade4(int32 variable_size) const
{
	switch (variable_size)
	{
		case 8: return "rdx";
		case 4: return "edx";
		case 2: return "dx";
		case 1: return "dl";
		default: return "";
	}
}

std::string Compiler::GetParameterRegister(const uint32 parameter_num, const int32 parameter_size) const
{
	static const char* registers_64[] = { "rdi", "rsi", "rdx", "rcx", "r8", "r9" };
	static const char* registers_32[] = { "edi", "esi", "edx", "ecx", "r8d", "r9d" };
	static const char* registers_16[] = { "di", "si", "dx", "cx", "r8w", "r9w" };
	static const char* registers_8[] = { "dil", "sil", "dl", "cl", "r8b", "r9b" };

	if (parameter_num >= 6)
	{
		std::cout << "[Error] Unsupported parameter number -> functions only support 6 parameters... Other parameters will be ignored!\n";
		return "";
	}

	switch (parameter_size)
	{
	case 8:
		return registers_64[parameter_num];
	case 4:
		return registers_32[parameter_num];
	case 2:
		return registers_16[parameter_num];
	case 1:
		return registers_8[parameter_num];
	default:
		std::cout << "[Error] Unsupported parameter size: valid sizes are 1, 2, 4, or 8 bytes...\n";
	}
}

void Compiler::Compare(const std::vector<Token>& left, const std::vector<Token>& right, std::ofstream& output_file)
{
	HandleComplexAssignment(left, output_file, "rcx", 8, EAssignmentType::NotSpecified);
	HandleComplexAssignment(right, output_file, "rdx", 8, EAssignmentType::NotSpecified);
	output_file << " cmp rcx, rdx\n";
}

bool Compiler::IsBoolean(const std::string& variable_type) const
{
	return variable_type == "bool" || variable_type == "boolean";
}

bool Compiler::IsBoolean(const Variable& variable_type) const
{
	return variable_type.type == "bool" || variable_type.type == "boolean";
}

bool Compiler::IsComplexIfStatement(const std::vector<Token>& tokens) const
{
	for (const Token& token : tokens)
	{
		if (token.value == "?") return true;
	}

	return false;
}

EAssignmentType Compiler::GetAssignmentType(const std::string& variable_type) const
{
	if (variable_type == "float") return EAssignmentType::FloatingPoint;
	else if (variable_type == "bool" || variable_type == "boolean") return EAssignmentType::Boolean;
	else return EAssignmentType::Integer;
}

std::string Compiler::TokenTypeToString(ETokenType type) const
{
	switch (type)
	{
	case ETokenType::Variable:
		return "variable";
	case ETokenType::Name:
		return "a name of a variable or function";
	case ETokenType::Numeric:
		return "a numeric literal (number)";
	case ETokenType::Keyword:
		return "keyword";
	case ETokenType::Macro:
		return "macro";
	case ETokenType::Operator:
		return "operator";
	case ETokenType::BooleanOperator:
		return "boolean operator";
	case ETokenType::Comment:
		return "comment";
	case ETokenType::Scope:
		return "scope";
	case ETokenType::Semicolon:
		return "symicolon";
	case ETokenType::Assignment:
		return "assignment operator";
	case ETokenType::Referral:
		return "referral";
	case ETokenType::Parenthesis:
		return "parenthesis";
	case ETokenType::IndexOperator:
		return "square brackets";
	default:
		return "no valid token";
	}
}

std::string Compiler::GetAssemblyTypesizeSpecifier(const int32 size) const
{
	if (size == 8) return "qword";
	if (size == 4) return "dword";
	if (size == 2) return "word";
	if (size == 1) return "byte";
	else return "";
}

std::string Compiler::GetConditionCodeEnding(const Token& condition) const
{
	if (condition.value == "==" || condition.value == "?")
	{
		return "e";
	}
	else if (condition.value == ">")
	{
		return "g";
	}
	else if (condition.value == ">=")
	{
		return "ge";
	}
	else if (condition.value == "<")
	{
		return "l";
	}
	else if (condition.value == "<=")
	{
		return "le";
	}
	else if (condition.value == "!=")
	{
		return "ne";
	}

	return std::string();
}

void Compiler::MoveByCondition(const std::vector<Token>& ifworth, const std::vector<Token>& elseworth, const Token& condition, const std::string& expected_location, const int32 result_size, std::ofstream& output_file)
{
	const std::string register_second_grade = GetCorrectVariableMathematicsRegisterGrade2(result_size);

	output_file << " pushf\n";
	const std::string keyword = " cmov" + GetConditionCodeEnding(condition);
	HandleComplexAssignment(ifworth, output_file, register_second_grade,
		result_size, EAssignmentType::NotSpecified);
	HandleComplexAssignment(elseworth, output_file, expected_location,
		result_size, EAssignmentType::NotSpecified);
	output_file << " popf\n";
	output_file << keyword << " " << expected_location << ", " << register_second_grade << "\n";
}

void Compiler::Move(std::ofstream& output_file, const std::string& destination, const std::string& source, const int32 destination_size, const int32 source_size)
{
	if (destination_size > source_size && source_size <= 2)
	{
		output_file << " movzx " << destination << ", " << GetAssemblyTypesizeSpecifier(source_size) << " " << source << "\n";
		return;
	}
	else if (destination_size < source_size && source_size <= 2)
	{
		output_file << " movsx " << destination << ", " << GetAssemblyTypesizeSpecifier(source_size) << " " << source << "\n";
		return;
	}

	output_file << " mov " << destination << ", " << source << "\n";
}

void Compiler::HandleNegateMacro(const std::vector<Token>& tokens, std::ofstream& output_file)
{
	std::vector<Token> first_param_tokens = {};
	int32 i = 2;
	Variable variable = {};

	while (tokens.size() > i && tokens[i].value != ",")
	{
		first_param_tokens.push_back(tokens[i]);
		i++;
	}
	if (tokens[tokens.size() - 3].type == ETokenType::Name)
	{
		variable = GetLocalVariableReference(tokens[tokens.size() - 3].value);
		if (!IsCorrectVariableName(tokens[tokens.size() - 3].value, variable.variable_assembly_safe + "]")) return;
		if (variable.bUnsigned)
		{
			std::cerr << "[Error] You cannot negate unsigned variables!\n";
			return;
		}
	}

	const std::string assembly_typesize_specifier = GetAssemblyTypesizeSpecifier(variable.type_size);

	std::string correct_register_grade_one = GetCorrectVariableMathematicsRegisterGrade1(variable.type_size);
	HandleComplexAssignment(first_param_tokens, output_file,
		correct_register_grade_one, variable.type_size, EAssignmentType::NotSpecified);

	if (variable.type_size == 1)
	{
		output_file << " movsx ax, al\n";
		correct_register_grade_one = "ax";
	}
	output_file << " imul " << correct_register_grade_one << ", -1\n";
	if (variable.type_size == 1) correct_register_grade_one = "al";
	output_file << " mov " << assembly_typesize_specifier << " " << variable.variable_assembly_safe + "]" << ", " << correct_register_grade_one << "\n";
}

void Compiler::HandleClampMacro(const std::vector<Token>& tokens, std::ofstream& output_file)
{
	Variable variable = {};
	std::vector<Token> max_value = {};
	std::vector<Token> min_value = {};

	int32 i = 2;
	int32 parameter = 0;
	int32 paranthesis = 1;
	while (tokens.size() > i)
	{
		if (tokens[i].value == ",")
		{
			parameter++;
			i++;
			continue;
		}
		else if (tokens[i].value == "(") paranthesis++;
		else if (tokens[i].value == ")")
		{
			paranthesis--;
			if (paranthesis == 0) break;
		}

		if (parameter == 0)
		{
			if (variable.variable_name == "")
			{
				variable = GetLocalVariableReference(tokens[i].value);
				if (!IsCorrectVariableName(tokens[i].value, variable.variable_assembly_safe + "]")) return;
			}
			else
			{
				std::cerr << "[Error] The first parameter of 'clamp!' must be a reference!\n";
				return;
			}
		}
		else if (parameter == 1)
		{
			min_value.push_back(tokens[i]);
		}
		else if (parameter == 2)
		{
			max_value.push_back(tokens[i]);
		}

		i++;
	}

	const std::string correct_register_first = GetCorrectVariableMathematicsRegisterGrade1(variable.type_size);
	const std::string correct_register_second = GetCorrectVariableMathematicsRegisterGrade2(variable.type_size);
	const std::string correct_compare_register = GetCorrectVariableMathematicsRegisterGrade3(variable.type_size);
	HandleComplexAssignment(min_value, output_file,
		correct_compare_register, variable.type_size, EAssignmentType::NotSpecified);

	output_file << " mov " << correct_register_first << ", " << variable.variable_assembly_safe << "]\n";
	output_file << " cmp eax, " << correct_compare_register << "\n";
	output_file << " mov " << correct_register_second << ", " << correct_compare_register << "\n";
	output_file << " cmovl " << correct_register_first << ", " << correct_register_second << "\n";

	HandleComplexAssignment(max_value, output_file,
		correct_compare_register, variable.type_size, EAssignmentType::NotSpecified);

	output_file << " cmp eax, " << correct_compare_register << "\n";
	output_file << " mov " << correct_register_second << ", " << correct_compare_register << "\n";
	output_file << " cmovg " << correct_register_first << ", " << correct_register_second << "\n";
	output_file << " mov " << GetAssemblyTypesizeSpecifier(variable.type_size) << " " << variable.variable_assembly_safe << "], " << correct_register_first << "\n";
}

void Compiler::HandleRepeatMacro(const std::vector<Token>& tokens, std::ofstream& output_file)
{
	std::vector<Token> first_parameter = {};
	std::vector<std::vector<Token>> second_parameter = {};

	int32 i = 2;
	int32 parameter = 0;
	int32 paranthesis = 1;
	bool add_new_list_second_parameter = true;
	while (i < tokens.size())
	{
		if (tokens[i].value == ",") 
		{
			parameter++;
			i++;
			continue;
		}
		else if (tokens[i].value == "(") paranthesis++;
		else if (tokens[i].value == ")")
		{
			paranthesis--;
			if (paranthesis == 0)
			{
				if (parameter == 1)
				{
					if (tokens[i - 1].value != "}")
					{
						std::cerr << "[Error] The expression has to end with a '}'! Line " << m_CurrentLine << "\n";
						return;
					}
				}

				break;
			}
		}

		if (parameter == 0)
		{
			first_parameter.push_back(tokens[i]);
		}
		else if (parameter == 1)
		{
			if (second_parameter.size() == 0)
			{
				if (tokens[i].value != "{")
				{
					std::cerr << "[Error] The expression has to start with a '{'! Line " << m_CurrentLine << "\n";
					return;
				}
				else
				{
					i++;
				}
			}

			if (tokens[i].value == "}")
			{
				i++;
				continue;
			}

			if (add_new_list_second_parameter)
			{
				second_parameter.push_back(std::vector<Token>({ tokens[i] }));
				add_new_list_second_parameter = false;
			}
			else 
			{
				second_parameter[second_parameter.size() - 1].push_back(tokens[i]);
			}

			if (tokens[i].value == ";")
			{
				add_new_list_second_parameter = true;
				i++;
				continue;
			}
		}
		
		i++;
	}

	HandleComplexAssignment(first_parameter, output_file, "r8", 8, EAssignmentType::Integer);
	output_file << "REPEAT" << m_SectionNumber << ":\n";

	bool nothing = false;
	for (const std::vector<Token>& second_parameter_token : second_parameter)
	{
		CompileToken(second_parameter_token, output_file, nothing);
	}

	output_file << " dec r8\n";
	output_file << " jnz REPEAT" << m_SectionNumber << "\n";
	m_SectionNumber++;
}

void Compiler::HandleSwapMacro(const std::vector<Token>& tokens, std::ofstream& output_file)
{
	Variable first_parameter = {};
	Variable second_parameter = {};

	int32 parameter = 0;
	for (int32 i = 2; i < tokens.size() - 2; i++)
	{
		if (tokens[i].value == ",") 
		{
			parameter++;
			continue;
		}

		if (tokens[i].type == ETokenType::Name)
		{
			const Variable variable = GetLocalVariableReference(tokens[i].value);
			if (!IsCorrectVariableName(tokens[i].value, variable.variable_name)) return;
			
			if (parameter == 0) first_parameter = variable;
			else if (parameter == 1) second_parameter = variable;
		}
	}

	if (!CheckTypeSize(first_parameter, second_parameter)) return;

	const std::string correct_register_grade_one = GetCorrectVariableMathematicsRegisterGrade1(first_parameter.type_size);
	const std::string correct_register_grade_two = GetCorrectVariableMathematicsRegisterGrade2(second_parameter.type_size);
	output_file << " mov " << correct_register_grade_one << ", " << first_parameter.variable_assembly_safe << "]\n";
	output_file << " mov " << correct_register_grade_two << ", " << second_parameter.variable_assembly_safe << "]\n";
	output_file << " xchg " << correct_register_grade_one << ", " << correct_register_grade_two << "\n";
	output_file << " mov " << GetAssemblyTypesizeSpecifier(first_parameter.type_size) << " " << first_parameter.variable_assembly_safe << "]" << ", " << correct_register_grade_one << "\n";
	output_file << " mov " << GetAssemblyTypesizeSpecifier(second_parameter.type_size) << " " << second_parameter.variable_assembly_safe << "]" << ", " << correct_register_grade_two << "\n";
}

void Compiler::HandleMacros(const std::vector<Token>& tokens, std::ofstream& output_file, bool& bUseExitCode)
{
	if (tokens[0].value == "exit!")
	{
		const std::string correct_register = GetCorrectVariableMathematicsRegisterGrade3(8);
		HandleComplexAssignment(std::vector<Token>(tokens.begin() + 2, tokens.end() - 2), output_file,
			correct_register, 8, EAssignmentType::Integer);

		output_file << " mov rax, 60\n";
		output_file << " mov rdi, rcx\n";
		output_file << " syscall\n";

		bUseExitCode = true;
	}
	else if (tokens[0].value == "negate!")
	{
		HandleNegateMacro(tokens, output_file);
	}
	else if (tokens[0].value == "clamp!")
	{
		HandleClampMacro(tokens, output_file);
	}
	else if (tokens[0].value == "repeat!")
	{
		HandleRepeatMacro(tokens, output_file);
	}
	else if (tokens[0].value == "swap!")
	{
		HandleSwapMacro(tokens, output_file);
	}
}

void Compiler::HandleScope(const std::vector<Token>& tokens, std::ofstream& output_file)
{
	if (tokens[0].value == "{")
	{
		output_file << " push rbp\n";
		output_file << " mov rbp, rsp\n";

		m_CurrentStacksizes.push_back(0);
		m_LocalVariables.push_back(std::vector<Variable>());

		if (m_pCurrentFunction)
		{
			if (m_RemainingFunctionScopes == 0)
			{
				HandleVariableParameters(m_pCurrentFunction->function_parameters, output_file);
			}
		}
		m_RemainingFunctionScopes++;
	}
	else if (tokens[0].value == "}")
	{
		output_file << " mov rsp, rbp\n";
		output_file << " pop rbp\n";

		m_CurrentStacksizes.pop_back();
		m_LocalVariables.pop_back();

		m_RemainingFunctionScopes--;
		if (m_RemainingFunctionScopes == 0)
		{
			if (m_pCurrentFunction)
			{
				if (m_pCurrentFunction->function_name != "main")
				{
					output_file << " ret\n";
				}
			}

			m_pCurrentFunction = nullptr;
		}
	}
}

bool Compiler::HandleVariableChanges(const std::vector<Token>& tokens, std::ofstream& output_file)
{
	if (tokens[1].type == ETokenType::Operator)
	{
		const Variable variable_reference = GetLocalVariableReference(tokens[0].value);
		if (IsCorrectVariableName(tokens[0].value, variable_reference.variable_name))
		{
			const std::string correct_register = GetCorrectVariableMathematicsRegisterGrade1(variable_reference.type_size);
			output_file << " mov " << correct_register + ", " << variable_reference.variable_assembly_safe << "]\n";
			if (tokens[1].value == "++") output_file << " inc " << correct_register << "\n";
			else if (tokens[1].value == "--") output_file << " dec " << correct_register << "\n";
			output_file << " mov " << variable_reference.variable_assembly_safe << "], " << correct_register << "\n";
			
			return true;
		}
	}
	else if (tokens[1].type == ETokenType::Assignment)
	{
		const Variable write_to_reference = GetLocalVariableReference(tokens[0].value);

		return HandleComplexAssignment(std::vector<Token>(tokens.begin() + 2, tokens.end() - 1), output_file,
			write_to_reference.variable_assembly_safe + "]", write_to_reference.type_size, GetAssignmentType(write_to_reference.type));
	}
	else if (tokens[1].type == ETokenType::Referral)
	{
		bool bIsValid = true;

		const Variable write_to_reference = GetLocalVariableReference(tokens[0].value);
		if (tokens[2].type != ETokenType::Numeric)
		{
			std::cerr << "[Error] Expected a numeric literal (number), but got " << TokenTypeToString(tokens[2].type) << " -> '" << tokens[2].value << "'! Line " << m_CurrentLine << "\n";
			bIsValid = false;
		}

		const std::string correct_register = GetCorrectVariableMathematicsRegisterGrade1(write_to_reference.type_size);
		output_file << " mov " << correct_register << ", " << tokens[2].value << "\n";
		output_file << " mov " << write_to_reference.variable_assembly_safe << "], " + correct_register + "\n";

		return bIsValid;
	}

	std::cerr << "[Error] Expected an (assignment) operator, but got " << TokenTypeToString(tokens[1].type) << " -> '" << tokens[1].value << "'! Line " << m_CurrentLine << "\n";
	return false;
}

void Compiler::HandleVariableDecleration(const std::vector<Token>& tokens, std::ofstream& output_file)
{
	bool bIsArray = false;
	if (tokens[0].type != ETokenType::Keyword)
	{
		std::cerr << "[Error] Expected a keyword like local or global, but got '" + tokens[0].value << "'! Line: " << m_CurrentLine << "\n";
	}
	if (tokens[1].type != ETokenType::Name)
	{
		std::cerr << "[Error] Expected a variable name, but got " << TokenTypeToString(tokens[1].type) << " -> '" << tokens[1].value << "'! Line " << m_CurrentLine << "\n";
	}
	if (tokens[2].type != ETokenType::Referral)
	{
		std::cerr << "[Error] Expected a referral like ':', but got " << TokenTypeToString(tokens[2].type) << " -> '" << tokens[2].value << "'! Line " << m_CurrentLine << "\n";
	}
	if (tokens[3].type != ETokenType::Variable)
	{
		std::cerr << "[Error] Expected a variable type, but got " << TokenTypeToString(tokens[3].type) << " -> '" << tokens[3].value << "'! Line " << m_CurrentLine << "\n";
	}
	if (tokens[4].type == ETokenType::IndexOperator)
	{
		if (tokens[5].type != ETokenType::IndexOperator)
		{
			std::cerr << "[Error] Expected square brackets, but got " << TokenTypeToString(tokens[5].type) << " -> '" << tokens[5].value << "'! Line " << m_CurrentLine << "\n";
		}
		else
		{
			bIsArray = true;
		}
	}
	else
	{
		if (tokens[4].type != ETokenType::Assignment) 
		{
			std::cerr << "[Error] Expected an assignment operator, but got " << TokenTypeToString(tokens[4].type) << " -> '" << tokens[4].value << "'! Line " << m_CurrentLine << "\n";
		}
	}

	if (tokens[0].value == "local")
	{
		bool bUnsigned = false;
		if (tokens[3].value[0] == 'u')
		{
			if (tokens[5].value[0] == '-')
			{
				std::cerr << "[Error] Unsigned variables cannot be constructed negative!\n";
				return;
			}
		}

		const uint32 size = GetVariableSize(tokens[3].value);

		m_CurrentStacksizes[m_CurrentStacksizes.size() - 1] += size;

		std::string stack_position = "[rbp-" + std::to_string(m_CurrentStacksizes[m_CurrentStacksizes.size() - 1]);
		m_LocalVariables[m_LocalVariables.size() - 1].push_back(Variable(tokens[1].value, stack_position, tokens[3].value, size, bUnsigned, false, IsBoolean(tokens[3].value), bIsArray));

		if (bIsArray)
		{
			const int32 tokens_length = tokens.size() - 2;
			int32 tokens_index = 8;

			std::vector<std::vector<Token>> parameter_tokens = {};
			while (tokens_length > tokens_index)
			{

			}
		}
		else
		{
			std::string value = {};
			if (size == 8) value = "qword " + stack_position + "]";
			if (size == 4) value = "dword " + stack_position + "]";
			if (size == 2) value = "word " + stack_position + "]";
			if (size == 1) value = "byte " + stack_position + "]";

			output_file << " sub rsp, " << size << "\n";
			const std::vector<Token> assignment_tokens = std::vector<Token>(tokens.begin() + 5, tokens.end() - 1);
			HandleComplexAssignment(assignment_tokens, output_file,
				value, size, GetAssignmentType(tokens[3].value));
		}
	}
}

void Compiler::HandleVariableParameters(const std::vector<Variable>& parameters, std::ofstream& output_file)
{
	uint32 parameter_num = 0;

	uint32 type_size = 0;
	for (const Variable& variable : parameters) type_size = type_size + variable.type_size;
	if (type_size > 0) output_file << " sub rsp, " << type_size << "\n";

	for (const Variable& variable : parameters)
	{
		const std::string correct_register = GetParameterRegister(parameter_num, variable.type_size);
		if (!correct_register.empty())
		{
			if (variable.type_size == 8) output_file << " mov qword " << variable.variable_assembly_safe << "], " << correct_register << "\n";
			else if (variable.type_size == 4) output_file << " mov dword " << variable.variable_assembly_safe << "], " << correct_register << "\n";
			else if (variable.type_size == 2) output_file << " mov word " << variable.variable_assembly_safe << "], " << correct_register << "\n";
			else if (variable.type_size == 1) output_file << " mov byte " << variable.variable_assembly_safe << "], " << correct_register << "\n";

			m_LocalVariables.push_back({ variable });
			m_CurrentStacksizes[m_CurrentStacksizes.size() - 1] += variable.type_size;

			parameter_num++;
		}
	}
}

void Compiler::HandleFunctionDecleration(const std::vector<Token>& tokens, std::ofstream& output_file)
{
	if (tokens[1].value == "main")
	{
		if (tokens[0].type != ETokenType::Keyword)
		{
			std::cerr << "[Error] Expected a keyword (define), but got '" + tokens[0].value << "'! Line: " << m_CurrentLine << "\n";
		}
		if (tokens[2].type != ETokenType::Parenthesis)
		{
			std::cerr << "[Error] Expected an open parenthesi '(', but got " << TokenTypeToString(tokens[2].type) << " -> '" << tokens[2].value << "'! Line " << m_CurrentLine << "\n";
		}
		if (tokens[3].type != ETokenType::Parenthesis)
		{
			std::cerr << "[Error] Expected a closed parenthesi ')', but got " << TokenTypeToString(tokens[tokens.size() - 3].type) << " -> '" << tokens[tokens.size() - 3].value << "'! Line " << m_CurrentLine << "\n";
		}

		output_file << "_start:\n";

		const Function function = Function("main", 8, {}, {});
		m_Functions.push_back(function);
		m_pCurrentFunction = &(m_Functions[m_Functions.size() - 1]);
	}
	else
	{
		if (tokens[0].type != ETokenType::Keyword)
		{
			std::cerr << "[Error] Expected a keyword (define), but got '" + tokens[0].value << "'! Line: " << m_CurrentLine << "\n";
		}
		if (tokens[1].type != ETokenType::Name)
		{
			std::cerr << "[Error] Expected a function name, but got " << TokenTypeToString(tokens[1].type) << " -> '" << tokens[1].value << "'! Line " << m_CurrentLine << "\n";
		}
		if (tokens[2].type != ETokenType::Parenthesis)
		{
			std::cerr << "[Error] Expected an open parenthesi '(', but got " << TokenTypeToString(tokens[2].type) << " -> '" << tokens[2].value << "'! Line " << m_CurrentLine << "\n";
		}
		if (tokens[tokens.size() - 3].type != ETokenType::Parenthesis)
		{
			std::cerr << "[Error] Expected a closed parenthesi ')', but got " << TokenTypeToString(tokens[tokens.size() - 3].type) << " -> '" << tokens[tokens.size() - 3].value << "'! Line " << m_CurrentLine << "\n";
		}
		if (tokens[tokens.size() - 2].value != "->")
		{
			std::cerr << "[Error] Expected the arrow operator '->', but got " << TokenTypeToString(tokens[tokens.size() - 2].type) << " -> '" << tokens[tokens.size() - 2].value << "'! Line " << m_CurrentLine << "\n";
		}
		if (tokens[tokens.size() - 1].type != ETokenType::Variable)
		{
			std::cerr << "[Error] Expected a variable type for the return value, but got " << TokenTypeToString(tokens[tokens.size() - 1].type) << " -> '" << tokens[tokens.size() - 1].value << "'! Line " << m_CurrentLine << "\n";
		}

		std::vector<Variable> parameters = {};
		if (tokens.size() != 6)
		{
			int32 current_stack_size = 0;
			for (int32 i = 3; i < tokens.size() - 3; i++)
			{
				if (tokens.at(i).value != ",")
				{
					bool bError = false;
					if (tokens.at(i).type != ETokenType::Name)
					{
						std::cerr << "[Error] Expected a variable name, but got " << TokenTypeToString(tokens.at(i).type) << " -> '" << tokens.at(i).value << "'! Line " << m_CurrentLine << "\n";
						bError = true;
					}
					if (tokens.at(i + 1).type != ETokenType::Referral)
					{
						std::cerr << "[Error] Expected a referral, but got " << TokenTypeToString(tokens.at(i + 1).type) << " -> '" << tokens.at(i + 1).value << "'! Line " << m_CurrentLine << "\n";
						bError = true;
					}
					if (tokens.at(i + 2).type != ETokenType::Variable)
					{
						std::cerr << "[Error] Expected a variable type, but got " << TokenTypeToString(tokens.at(i + 2).type) << " -> '" << tokens.at(i + 2).value << "'! Line " << m_CurrentLine << "\n";
						bError = true;
					}
					if (tokens.at(i + 3).value != ",")
					{
						if (tokens.at(i + 3).type == ETokenType::Name)
						{
							std::cerr << "[Error] Expected a comma ',' in the parameter list, but got " << TokenTypeToString(tokens.at(i + 3).type) << " -> '" << tokens.at(i + 3).value << "'! Line " << m_CurrentLine << "\n";
							bError = true;
						}
					}
					if (bError) continue;
				
					const std::string variable_type = tokens.at(i + 2).value;
					const std::string variable_name = tokens.at(i).value;

					bool bUnsigned = variable_type[0] == 'u';

					const int32 variable_size = GetVariableSize(variable_type);
					current_stack_size = current_stack_size + variable_size;
					const std::string assembly_stack_safe = "[rbp-" + std::to_string(current_stack_size);

					const Variable variable = Variable(variable_name, assembly_stack_safe, variable_type, variable_size, bUnsigned, true, IsBoolean(variable_type), false);
					parameters.push_back(variable);
					i = i + 3;
				}
			}
		}

		output_file << tokens[1].value << ":\n";

		const Function function = Function(tokens[1].value, GetVariableSize(tokens[tokens.size() - 1].value), parameters, tokens[tokens.size() - 1].value);
		m_Functions.push_back(function);
		m_pCurrentFunction = &(m_Functions[m_Functions.size() - 1]);
	}
}

int32 Compiler::HandleFunctionCall(const std::vector<Token>& tokens, std::ofstream& output_file)
{
	const Function function = GetFunction(tokens[0].value);
	if (IsCorrectFunctionName(tokens[0].value, function.function_name))
	{
		int32 closed_parenthesi_index = tokens.size() - 2;
		if (tokens[tokens.size() - 1].value != ";") 
		{
			closed_parenthesi_index = tokens.size() - 1;
		}

		if (tokens[1].type != ETokenType::Parenthesis)
		{
			std::cerr << "[Error] Expected an open parenthesi '(', but got " << TokenTypeToString(tokens[1].type) << " -> '" << tokens[1].value << "'! Line " << m_CurrentLine << "\n";
		}
		if (tokens[closed_parenthesi_index].type != ETokenType::Parenthesis)
		{
			std::cerr << "[Error] Expected a closed parenthesi ')', but got " << TokenTypeToString(tokens[closed_parenthesi_index].type) << " -> '" << tokens[closed_parenthesi_index].value << "'! Line " << m_CurrentLine << "\n";
		}

		if (function.function_parameters.size() > 0)
		{
			const int32 tokens_length = tokens.size() - 1;
			int32 parameter_num = 0;
			int32 tokens_index = 2;
			for (const Variable& variable : function.function_parameters)
			{
				std::vector<Token> parameter_tokens = {};
				while (tokens_length > tokens_index && tokens[tokens_index].value != "," && tokens[tokens_index].value != ")")
				{
					parameter_tokens.push_back(tokens[tokens_index]);
					tokens_index++;
				}
				if (tokens_length > tokens_index)
				{
					if (tokens[tokens_index].value != "," || tokens[tokens_index].value != ")") tokens_index++;
				}

				if (parameter_tokens.size() > 0)
				{
					const std::string write_to_reference = GetParameterRegister(parameter_num, variable.type_size);
					HandleComplexAssignment(parameter_tokens, output_file,
						write_to_reference, variable.type_size, GetAssignmentType(variable.type));
				}

				parameter_num++;
			}
		}

		output_file << " call " << function.function_name << "\n";
		return function.return_size;
	}

	return 0;
}

void Compiler::HandleReturnKeyword(const std::vector<Token>& tokens, std::ofstream& output_file)
{
	if (m_pCurrentFunction)
	{
		if (m_pCurrentFunction->function_name == "main")
		{
			if (tokens.size() == 2)
			{
				output_file << " mov rax, 60\n";
				output_file << " mov rdi, 0\n";
				output_file << " syscall\n";
			}
			else 
			{
				const std::string correct_register = GetCorrectVariableMathematicsRegisterGrade3(m_pCurrentFunction->return_size);
				HandleComplexAssignment(std::vector<Token>(tokens.begin() + 1, tokens.end() - 1), output_file,
					correct_register, m_pCurrentFunction->return_size, EAssignmentType::Integer);

				output_file << " mov rax, 60\n";
				output_file << " mov rdi, " << correct_register << "\n";
				output_file << " syscall\n";
			}
		}
		else
		{
			if (tokens.size() == 2)
			{
				output_file << " mov rsp, rbp\n";
				output_file << " pop rbp\n";
				output_file << " ret\n";
			}
			else
			{
				if (m_pCurrentFunction->return_size == 0)
				{
					std::cerr << "[Error] You cannot return, if your return type is 'void'! Line " << m_CurrentLine << "\n";
				}
				else
				{
					const std::string correct_register = GetCorrectVariableMathematicsRegisterGrade1(m_pCurrentFunction->return_size);
					HandleComplexAssignment(std::vector<Token>(tokens.begin() + 1, tokens.end() - 1), output_file,
						correct_register, m_pCurrentFunction->return_size, GetAssignmentType(m_pCurrentFunction->return_type));

					output_file << " mov rsp, rbp\n";
					output_file << " pop rbp\n";
					output_file << " ret\n";
				}
			}
		}
	}
	else 
	{
		std::cerr << "[Error] You cannot return outside of functions! Line " << m_CurrentLine << "\n";
	}
}

bool Compiler::HandleComplexAssignment(const std::vector<Token>& tokens, std::ofstream& output_file, const std::string& expected_result_location, const int32 result_size, const EAssignmentType assignment_type)
{
	if (assignment_type == EAssignmentType::Integer || assignment_type == EAssignmentType::NotSpecified)
	{
		if (tokens.size() == 1)
		{
			std::string read_from = {};
			if (tokens[0].type == ETokenType::Numeric)
			{
				read_from = tokens[0].value;

				if (expected_result_location != read_from)
				{
					const int32 max_size = arhi::clamp(4, 0, (int32)expected_result_location.size());
					const bool is_pointer = expected_result_location.substr(0, max_size) == "qwor" ||
						expected_result_location.substr(0, max_size) == "dwor" ||
						expected_result_location.substr(0, max_size) == "word" ||
						expected_result_location.substr(0, max_size) == "byte";

					if (!is_pointer)
					{
						const std::string value = GetAssemblyTypesizeSpecifier(result_size);
						output_file << " mov " << value << " " << expected_result_location << ", " << read_from << "\n";
					}
					else 
					{
						output_file << " mov " << expected_result_location << ", " << read_from << "\n";
					}
				}

				return true;
			}
			else if (tokens[0].type == ETokenType::Name)
			{
				const std::string correct_register = GetCorrectVariableMathematicsRegisterGrade1(result_size);
				const std::string correct_assembly_specifier = GetAssemblyTypesizeSpecifier(result_size);

				const Variable variable = GetLocalVariableReference(tokens[0].value);
				if (!IsCorrectVariableName(tokens[0].value, variable.variable_name)) return false;
				read_from = variable.variable_assembly_safe + "]";

				if (correct_register != read_from)
				{
					Move(output_file, correct_register, read_from, result_size, variable.type_size);
				}
				if (expected_result_location != correct_register)
				{
					output_file << " mov " << correct_assembly_specifier << " " << expected_result_location << ", " << correct_register << "\n";
				}

				return true;
			}
		}
		else
		{
			if (tokens.size() >= 4 && IsComplexIfStatement(tokens))
			{
				int32 i = 0;
				Token condition = {};
				std::vector<Token> left = {};
				std::vector<Token> right = {};
				std::vector<Token> ifworth = {};
				std::vector<Token> elseworth = {};
				for (const Token& token : tokens)
				{
					if (token.type == ETokenType::BooleanOperator || token.type == ETokenType::Referral)
					{
						if (token.type == ETokenType::BooleanOperator) 
						{
							if (token.value == "?")
							{
								if (!left.empty() && right.empty())
								{
									i = 2;
									continue;
								}
							}
						}

						if (token.value != "?" && token.type == ETokenType::BooleanOperator) condition = token;
						i++;
						continue;
					}
					
					if (i < 2)
					{
						if (i == 0) left.push_back(token);
						else right.push_back(token);
					}
					else
					{
						if (i == 2) ifworth.push_back(token);
						else elseworth.push_back(token);
					}
				}
				if (condition.empty() && right.empty())
				{
					if (left.size() == 1)
					{
						const Variable variable = GetLocalVariableReference(left[0].value);
						if (!IsCorrectVariableName(left[0].value, variable.variable_name)) return false;
						if (IsBoolean(variable))
						{
							right.push_back(Token(ETokenType::Keyword, "true", left[0].line));
							condition = Token(ETokenType::BooleanOperator, "==", left[0].line);
						}
					}
				}

				const int32 size = arhi::clamp(result_size, 4, 8);
				const std::string correct_register = GetCorrectVariableMathematicsRegisterGrade1(size);
				Compare(left, right, output_file);
				MoveByCondition(ifworth, elseworth, condition, correct_register, size, output_file);

				const std::string final_register = GetCorrectVariableMathematicsRegisterGrade1(result_size);
				output_file << " mov " << expected_result_location << ", " << final_register << "\n";

				return true;
			}
			else
			{
				const std::string correct_register = GetCorrectVariableMathematicsRegisterGrade1(result_size);
				if (GetMathematicResultIntoRegister(tokens, result_size, output_file) != "")
				{
					if (expected_result_location != correct_register)
					{
						output_file << " mov " << expected_result_location << ", " << correct_register << "\n";
			
						return true;
					}
				}
				else
				{
					const int32 function_result_size = HandleFunctionCall(tokens, output_file);
					if (function_result_size != 0)
					{
						const std::string function_result_register = GetCorrectVariableMathematicsRegisterGrade1(function_result_size);
						if (function_result_register != expected_result_location)
						{
							Move(output_file, expected_result_location, function_result_register, result_size, function_result_size);
						}

						return true;
					}
				}
			}
		}
	}
	if (assignment_type == EAssignmentType::Boolean || assignment_type == EAssignmentType::NotSpecified)
	{
		HandleComplexBooleanAssignment(tokens, output_file, expected_result_location, result_size);
	}

	return false;
}

bool Compiler::HandleComplexBooleanAssignment(const std::vector<Token>& tokens, std::ofstream& output_file, const std::string& expected_result_location, const int32 result_size)
{
	if (tokens.size() == 1)
	{
		const std::string correct_register = GetCorrectVariableMathematicsRegisterGrade1(result_size);

		if (tokens[0].type == ETokenType::Keyword)
		{
			if (tokens[0].value == "true")
			{
				output_file << " mov " << expected_result_location << ", 1\n";
			}
			else if (tokens[0].value == "false")
			{
				output_file << " mov " << expected_result_location << ", 0\n";
			}
			else
			{
				std::cerr << "[Error] You can only use the keywords 'true' and 'false' to assign booleans! Line " << m_CurrentLine << "\n";
				return false;
			}
		}
		else if (tokens[0].type == ETokenType::Name)
		{
			const Variable variable = GetLocalVariableReference(tokens[0].value);
			if (!IsCorrectVariableName(tokens[0].value, variable.variable_name)) return false;
			if (variable.type == "bool" || variable.type == "boolean")
			{
				output_file << " mov " << correct_register << ", " << variable.variable_assembly_safe << "]\n";
				output_file << " mov " << expected_result_location << ", " << correct_register << "\n";
			}
			else
			{
				std::cerr << "[Error] You cannot assign non boolean type variables to boolean type variables! Line " << m_CurrentLine << "\n";
				return false;
			}
		}

		std::cerr << "[Error] You can only use the keywords 'true' and 'false' to assign booleans! Line " << m_CurrentLine << "\n";
		return false;
	}
	else
	{
		std::vector<Token> left = {};
		std::vector<Token> right = {};
		Token condition = {};

		int32 side = 0;
		for (int32 i = 0; i < tokens.size(); i++)
		{
			if (tokens[i].type == ETokenType::BooleanOperator)
			{
				condition = tokens[i];
				side++;
				continue;
			}

			if (side == 0) left.push_back(tokens[i]);
			else if (side == 1) right.push_back(tokens[i]);
		}
		if (condition.empty() && right.empty())
		{
			if (left.size() == 1)
			{
				const Variable variable = GetLocalVariableReference(left[0].value);
				if (!IsCorrectVariableName(left[0].value, variable.variable_name)) return false;
				if (IsBoolean(variable))				
				{
					right.push_back(Token(ETokenType::Keyword, "true", left[0].line));
					condition = Token(ETokenType::BooleanOperator, "==", left[0].line);
				}
			}
		}

		if (condition.empty())
		{
			std::cerr << "[Error] You cannot define booleans like you did! You have to use a condition! Line " << m_CurrentLine << "\n";
			return false;
		}

		Compare(left, right, output_file);
		output_file << " set" << GetConditionCodeEnding(condition) << " al\n";
		Move(output_file, expected_result_location, "al", result_size, 1);
	}

	return true;
}

bool Compiler::CheckforSymicolon(const Token& token_to_check)
{
	if (token_to_check.type != ETokenType::Semicolon)
	{
		std::cerr << "[Error] Expected an symicolon, but got " << TokenTypeToString(token_to_check.type) << " -> '" << token_to_check.value << "'! Line " << m_CurrentLine << "\n";
		return false;
	}

	return true;
}
