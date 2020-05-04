#include <string>
#include <vector>
#include "..\heConsole.h"
#include "..\heD3.h"

void command_set_position(int index, hm::vec3f const& position) {
	HeD3Instance* instance = heD3LevelGetInstance(heD3Level, index);
	heD3InstanceSetPosition(instance, position);
};

void front_command_set_position(std::vector<std::string> const& args) {
	if(args.size() != 2) {
		heConsolePrint("Error: set_position requires 2 arguments");
		return;
	};
	int i0 = std::stoi(args[0]);
	hm::vec3f i1 = hm::parseVec3f(args[1]);
	command_set_position(i0, i1);
};


void command_set_jumpheight(float height) {
	 HePhysicsActor* actor = heD3Level->physics.actor;
	 actor->actorInfo.jumpHeight = height;
};

void front_command_set_jumpheight(std::vector<std::string> const& args) {
	if(args.size() != 1) {
		heConsolePrint("Error: set_jumpheight requires 1 arguments");
		return;
	};
	float i0 = std::stof(args[0]);
	command_set_jumpheight(i0);
};


void heRegisterCommands() {
	heConsoleRegisterCommand("set_position", &front_command_set_position);
	heConsoleRegisterCommand("set_jumpheight", &front_command_set_jumpheight);
};

