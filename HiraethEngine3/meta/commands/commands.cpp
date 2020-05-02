void command_hello(std::string const& in) {
	heConsolePrint("Hello!: " + in);
};

void front_command_hello(std::vector<std::string> const& args) {
	std::string i0 = args[0];
	command_hello(i0);
};


void command_add(int a, int b) {
	heConsolePrint("Add: " + std::to_string((a + b)));
};

void front_command_add(std::vector<std::string> const& args) {
	int i0 = std::stoi(args[0]);
	int i1 = std::stoi(args[1]);
	command_add(i0, i1);
};


