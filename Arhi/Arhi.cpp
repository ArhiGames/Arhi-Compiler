#include <iostream>
#include <fstream>
#include <sstream>
#include "Types.h"
#include "Tokenizer.h"
#include "Compiler.h"

const std::string gFileName = "code.arhi";

int create_arhi_file()
{
    std::ifstream exist_code_file = std::ifstream(gFileName);
    if (!exist_code_file.good())
    {
        std::ofstream created_file = std::ofstream(gFileName);

        if (created_file.is_open())
        {
            created_file.close();
            std::cout << "Created " << gFileName << " successfully!";
            return 0;
        }
        else
        {
            std::cerr << gFileName << " could not be created!";
            return 1;
        }
    }
    else
    {
        std::cout << gFileName << " already exists! \n";
        return 0;
    }
}

std::string read_file_to_string(const std::string& file_name)
{
    std::ifstream file = std::ifstream(file_name);
    if (!file.is_open())
    {
        std::cerr << "Tokenize error! Could not open " << file_name;
        return "";
    }

    std::stringstream buffer = {};
    buffer << file.rdbuf();

    return buffer.str();
}

void on_tokenizer_complete(const std::vector<std::vector<Token>>& tokens)
{
    std::cout << "Tokenization process complete!" << "\n";
    std::cout << "Compiling started..." << "\n";

    Compiler compiler = Compiler();
    int32 exit_code = compiler.Compile(tokens);

    std::cout << "Compiling proccess completed with code " << exit_code << "\n";
}

int main()
{
    if (create_arhi_file() == 1) return 1;

    const std::string file_content = read_file_to_string(gFileName);
    Tokenizer tokenizer = Tokenizer(file_content, &on_tokenizer_complete);
    tokenizer.Tokenize();

    std::cin.get();
}
