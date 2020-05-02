#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <fstream>

static bool ISSUED_ERROR = false;


enum DataType {
	DATA_TYPE_INVALID,
	DATA_TYPE_STRING,
	DATA_TYPE_INT,
};

struct Command {
	std::string name;
	std::string procName;
	std::vector<DataType> arguments;

	std::vector<std::string> functionCode;
};

void eat_spaces(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
                                    return !isspace(ch);
                                    }));
};

std::vector<std::string> split_string(const std::string& string, const char delimn) {
    std::vector<std::string> result;
    std::string::const_iterator start = string.begin();
    std::string::const_iterator end = string.end();
    std::string::const_iterator next = std::find(start, end, delimn);
    
    while (next != end) {
        result.emplace_back(std::string(start, next));
        start = next + 1;
        
        while(start != end && start[0] == delimn)
            ++start;
        
        next = std::find(start, end, delimn);
        
    }
    
    if(start != end)
        result.emplace_back(std::string(start, next));
    return result;
};

void parse_command_file(std::string const& file, std::vector<Command>& commands) {
	std::ifstream stream(file);
	if(!stream) {
		std::cout << "ERROR: Could not find file [" + file + "]" << std::endl;
		return;
	}

	std::string line;
	
	while(std::getline(stream, line)) {
	    eat_spaces(line);
		if(line.empty())
			continue;

		// parse function
		uint64_t openBraces = 1;
		Command* cmd = &commands.emplace_back();
		cmd->functionCode.emplace_back(line);

		while(std::getline(stream, line) && openBraces > 0) {
            cmd->functionCode.emplace_back(line);
			openBraces += std::count(line.begin(), line.end(), '{');
			openBraces -= std::count(line.begin(), line.end(), '}');
		}	
	}
	
	stream.close();
};

DataType parse_data_type(std::string& type) {
	size_t constIndex = type.find("const");
	if(constIndex != std::string::npos)
		type.erase(constIndex);
	
	size_t refIndex = type.find('&');
	if(refIndex != std::string::npos)
		type.erase(refIndex);

	// remove trailing whitespaces
    type = type.substr(0, type.find(' ')); 
	
	if(type == "std::string")
		return DATA_TYPE_STRING;
	else if(type == "int")
		return DATA_TYPE_INT;

	return DATA_TYPE_INVALID;
};

void parse_command_info(Command* command) {
	std::string const& header = command->functionCode[0];
    size_t index = header.find(' ') + 1;
	size_t index2 = header.find('(');
	command->name = header.substr(index, index2 - index);
	command->procName = command->name;
	if(command->name.compare(0, 8, "command_") == 0)
		command->name = command->name.substr(8);

	// parse argument types
	index = index2 + 1;
	index2 = header.find(')');
	std::string argumentsString = header.substr(index, index2 - index);
	if(argumentsString.size() > 0) {
		std::vector<std::string> arguments = split_string(argumentsString, ',');

        for(std::string& all : arguments) {
			eat_spaces(all);
			index = all.find_last_of(' ');
			std::string typeString = all.substr(0, index);
			DataType type = parse_data_type(typeString);
			if(type == DATA_TYPE_INVALID) {
				ISSUED_ERROR = true;
				return;
			}

			command->arguments.emplace_back(type);
		}
	}
};

void write_command_hook(Command* command, std::ofstream& stream) {
	for(std::string const& all : command->functionCode)
	    stream << all << std::endl;

	stream << std::endl;
	stream << "void front_command_" + command->name + "(std::vector<std::string> const& args) {\n";
	stream << "\tif(args.size() != " + std::to_string(command->arguments.size()) + ") {\n";
	stream << "\t\theConsolePrint(\"Error: " + command->name + " requires " + std::to_string(command->arguments.size()) + " arguments\");\n";
    stream << "\t\treturn;\n";
    stream << "\t};\n";

	std::string callString = "\t"  + command->procName + "("; 
	
	uint8_t argumentCount = (uint8_t) command->arguments.size();
	for(uint8_t i = 0; i < argumentCount; ++i) {
		if(command->arguments[i] == DATA_TYPE_STRING)
			stream << "\tstd::string i" + std::to_string(i) + " = args[" + std::to_string(i) + "];" << std::endl;
		else if(command->arguments[i] == DATA_TYPE_INT)
			stream << "\tint i" + std::to_string(i) + " = std::stoi(args[" + std::to_string(i) + "]);" << std::endl;

		callString += 'i' + std::to_string(i) + ", ";
	}

	if(argumentCount > 0)
		callString = callString.substr(0, callString.size() - 2); // cut last , that was put there by the last argument
	
	callString += ");\n";
	stream << callString;
	stream << "};\n\n\n";
};

int main(int argc, char* argv[]) {
    // parse command files
	if(argc < 2) {
		std::cout << "Please specify a command meta file as argument" << std::endl;
		system("pause");
		return 1;
	}

	std::string file(argv[1]);
	std::string outFile(file.substr(0, file.find_last_of('.')) + ".cpp");
	
	std::cout << "Parsing file: " << file << std::endl;
	std::vector<Command> commands;
	parse_command_file(file, commands);

	if(ISSUED_ERROR)
		return -1;
	
	std::ofstream out(outFile);
	out << "#include <string>\n";
	out << "#include <vector>\n";
	out << "#include \"..\\heConsole.h\"\n";
	out << std::endl;

	std::string registerCode = "void heRegisterCommands() {\n";
	
	for(auto& all : commands) {
		parse_command_info(&all);
		if(ISSUED_ERROR)
			return -1;
		write_command_hook(&all, out);
		registerCode += "\theConsoleRegisterCommand(\"" + all.name + "\", &front_command_" + all.name + ");\n";
	};

	registerCode += "};\n";

	out << registerCode << std::endl;
	out.flush();
	out.close();
	std::cout << "Successfully registered " << commands.size() << " commands" << std::endl;
	
	return 0;
};
