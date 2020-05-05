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


void command_toggle_physics_debug() {
	if(heD3Level->physics.enableDebugDraw)
		hePhysicsLevelDisableDebugDraw(&heD3Level->physics);
	else
		hePhysicsLevelEnableDebugDraw(&heD3Level->physics, heRenderEngine);
};

void front_command_toggle_physics_debug(std::vector<std::string> const& args) {
	if(args.size() != 0) {
		heConsolePrint("Error: toggle_physics_debug requires 0 arguments");
		return;
	};
	command_toggle_physics_debug();
};


void command_help() {
	heConsolePrint("=== HELP ===");
	heConsolePrint("> set_position int: index, vec3: position");
	heConsolePrint("> set_jumpheight float: height");
	heConsolePrint("> toggle_physics_debug ");
	heConsolePrint("=== HELP ===");

};

void front_command_help(std::vector<std::string> const& args) {
	if(args.size() != 0) {
		heConsolePrint("Error: help requires 0 arguments");
		return;
	};
	command_help();
};


void heRegisterCommands() {
	heConsoleRegisterCommand("set_position", &front_command_set_position);
	heConsoleRegisterCommand("set_jumpheight", &front_command_set_jumpheight);
	heConsoleRegisterCommand("toggle_physics_debug", &front_command_toggle_physics_debug);
	heConsoleRegisterCommand("help", &front_command_help);
};

