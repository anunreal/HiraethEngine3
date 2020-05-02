#include <string>
#include <vector>
#include "..\heConsole.h"

void command_hello(std::string const& in) {
	heConsolePrint("Hello!: " + in);
};

void front_command_hello(std::vector<std::string> const& args) {
	if(args.size() != 1) {
		heConsolePrint("Error: hello requires 1 arguments");
		return;
	};
	std::string i0 = args[0];
	command_hello(i0);
};


void command_add(int a, int b) {
	heConsolePrint("Add: " + std::to_string((a + b)));
};

void front_command_add(std::vector<std::string> const& args) {
	if(args.size() != 2) {
		heConsolePrint("Error: add requires 2 arguments");
		return;
	};
	int i0 = std::stoi(args[0]);
	int i1 = std::stoi(args[1]);
	command_add(i0, i1);
};


void command_klari(int count) {
	for(int i = 0; i < count; ++i)
		heConsolePrint("<3");
};

void front_command_klari(std::vector<std::string> const& args) {
	if(args.size() != 1) {
		heConsolePrint("Error: klari requires 1 arguments");
		return;
	};
	int i0 = std::stoi(args[0]);
	command_klari(i0);
};


void heRegisterCommands() {
	heConsoleRegisterCommand("hello", &front_command_hello);
	heConsoleRegisterCommand("add", &front_command_add);
	heConsoleRegisterCommand("klari", &front_command_klari);
};

