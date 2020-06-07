#include <string>
#include <vector>
#include "..\heConsole.h"
#include "..\heD3.h"
#include "..\heDebugUtils.h"
#include "..\..\main.h"
#include "..\heConverter.h"

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


void command_set_exposure(float f) {
	 heD3Level->camera.exposure = f;
};

void front_command_set_exposure(std::vector<std::string> const& args) {
	if(args.size() != 1) {
		heConsolePrint("Error: set_exposure requires 1 arguments");
		return;
	};
	float i0 = std::stof(args[0]);
	command_set_exposure(i0);
};


void command_set_gamma(float g) {
	 heD3Level->camera.gamma = g;
};

void front_command_set_gamma(std::vector<std::string> const& args) {
	if(args.size() != 1) {
		heConsolePrint("Error: set_gamma requires 1 arguments");
		return;
	};
	float i0 = std::stof(args[0]);
	command_set_gamma(i0);
};


void command_set_name(std::string const& name) {
	 strcpy_s(app.ownName, name.c_str());
};

void front_command_set_name(std::vector<std::string> const& args) {
	if(args.size() != 1) {
		heConsolePrint("Error: set_name requires 1 arguments");
		return;
	};
	std::string i0 = args[0];
	command_set_name(i0);
};


void command_teleport(hm::vec3f const& position) {
	 if(heD3Level->physics.actor) {
		  hePhysicsActorSetPosition(heD3Level->physics.actor, position);
	 }

	 heD3Level->camera.position = position;
};

void front_command_teleport(std::vector<std::string> const& args) {
	if(args.size() != 1) {
		heConsolePrint("Error: teleport requires 1 arguments");
		return;
	};
	hm::vec3f i0 = hm::parseVec3f(args[0]);
	command_teleport(i0);
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


void command_toggle_profiler() {
	 heProfilerToggleDisplay();
};

void front_command_toggle_profiler(std::vector<std::string> const& args) {
	if(args.size() != 0) {
		heConsolePrint("Error: toggle_profiler requires 0 arguments");
		return;
	};
	command_toggle_profiler();
};


void command_export_texture(std::string const& in, std::string const& out) {
     heTextureCompress(in, out);
     HE_LOG("Successfully converted texture");
};

void front_command_export_texture(std::vector<std::string> const& args) {
	if(args.size() != 2) {
		heConsolePrint("Error: export_texture requires 2 arguments");
		return;
	};
	std::string i0 = args[0];
	std::string i1 = args[1];
	command_export_texture(i0, i1);
};


void command_export_textures() {
    std::string folder = "res/textures/instances";
    std::vector<HeFileDescriptor> files; 
    heWin32FolderGetFiles(folder, files, true);

    for(auto& all : files) {
        std::string out = "bin" + all.fullPath;
        heTextureCompress(all.fullPath, out);
    }
    
    HE_LOG("Successfully converted textures");
};

void front_command_export_textures(std::vector<std::string> const& args) {
	if(args.size() != 0) {
		heConsolePrint("Error: export_textures requires 0 arguments");
		return;
	};
	command_export_textures();
};


void command_export_assets() {
    std::string folder = "res/instances";
    std::vector<HeFileDescriptor> files; 
    heWin32FolderGetFiles(folder, files, true);

    for(auto& all : files) {
        std::string out = "bin" + all.fullPath;
        heBinaryConvertD3InstanceFile(all.fullPath, out);
    }
    
    HE_LOG("Successfully converted textures");
};

void front_command_export_assets(std::vector<std::string> const& args) {
	if(args.size() != 0) {
		heConsolePrint("Error: export_assets requires 0 arguments");
		return;
	};
	command_export_assets();
};


void command_export_skybox() {
    HeD3Skybox* skybox = &heD3Level->skybox; 
    heTextureExport(skybox->specular, "binres/textures/skybox/" + skybox->specular->name + ".h3asset");
    heTextureExport(skybox->irradiance, "binres/textures/skybox/" + skybox->irradiance->name + ".h3asset");
    HE_LOG("Successfully exported skybox");
};

void front_command_export_skybox(std::vector<std::string> const& args) {
	if(args.size() != 0) {
		heConsolePrint("Error: export_skybox requires 0 arguments");
		return;
	};
	command_export_skybox();
};


void command_print_memory() {
    HE_LOG("=== MEMORY ===");
    uint64_t total = heMemoryTracker[HE_MEMORY_TYPE_TEXTURE] +
        heMemoryTracker[HE_MEMORY_TYPE_VAO] + 
        heMemoryTracker[HE_MEMORY_TYPE_FBO] +
        heMemoryTracker[HE_MEMORY_TYPE_SHADER] + 
        heMemoryTracker[HE_MEMORY_TYPE_CONTEXT];

    HE_LOG("textures = " + he_bytes_to_string(heMemoryTracker[HE_MEMORY_TYPE_TEXTURE]));
    HE_LOG("vaos     = " + he_bytes_to_string(heMemoryTracker[HE_MEMORY_TYPE_VAO]));
    HE_LOG("fbos     = " + he_bytes_to_string(heMemoryTracker[HE_MEMORY_TYPE_FBO]));
    HE_LOG("context  = " + he_bytes_to_string(heMemoryTracker[HE_MEMORY_TYPE_CONTEXT]));
    HE_LOG("shaders  = " + he_bytes_to_string(heMemoryTracker[HE_MEMORY_TYPE_SHADER]));
    HE_LOG("total    = " + he_bytes_to_string(total));
    HE_LOG("total    = " + he_bytes_to_string(heMemoryGetUsage()));
    HE_LOG("=== MEMORY ===");
};

void front_command_print_memory(std::vector<std::string> const& args) {
	if(args.size() != 0) {
		heConsolePrint("Error: print_memory requires 0 arguments");
		return;
	};
	command_print_memory();
};


void command_print_textures() {
    HE_LOG("=== TEXTURES ===");
    std::string string;
    for(auto const& all : heAssetPool.texturePool)
        he_to_string(&all.second, string);

    HE_LOG(string);   
    HE_LOG("=== TEXTURES ===");
};

void front_command_print_textures(std::vector<std::string> const& args) {
	if(args.size() != 0) {
		heConsolePrint("Error: print_textures requires 0 arguments");
		return;
	};
	command_print_textures();
};


void command_print_instances() {
    HE_LOG("=== INSTANCES ===");
    uint16_t counter = 0;
    for(const auto& all : heD3Level->instances) {
        std::string string;
        he_to_string(&all, string);
        HE_LOG('[' + std::to_string(counter++) + "] = " + string);
    }

    HE_LOG("=== INSTANCES ===");
};

void front_command_print_instances(std::vector<std::string> const& args) {
	if(args.size() != 0) {
		heConsolePrint("Error: print_instances requires 0 arguments");
		return;
	};
	command_print_instances();
};


void command_print_instance(std::string const& name) {
    uint16_t counter = 0;
    for(const auto& all : heD3Level->instances) {
        if(all.name == name) {
            std::string string;
            he_to_string(&all, string);
            HE_LOG('[' + std::to_string(counter++) + "] = " + string);
        } else
            counter++;
    }
};

void front_command_print_instance(std::vector<std::string> const& args) {
	if(args.size() != 1) {
		heConsolePrint("Error: print_instance requires 1 arguments");
		return;
	};
	std::string i0 = args[0];
	command_print_instance(i0);
};


void command_help() {
	heConsolePrint("=== HELP ===");
	heConsolePrint("> set_position int: index, vec3: position");
	heConsolePrint("> set_jumpheight float: height");
	heConsolePrint("> set_exposure float: f");
	heConsolePrint("> set_gamma float: g");
	heConsolePrint("> set_name string: name");
	heConsolePrint("> teleport vec3: position");
	heConsolePrint("> toggle_physics_debug ");
	heConsolePrint("> toggle_profiler ");
	heConsolePrint("> export_texture string: in, string: out");
	heConsolePrint("> export_textures ");
	heConsolePrint("> export_assets ");
	heConsolePrint("> export_skybox ");
	heConsolePrint("> print_memory ");
	heConsolePrint("> print_textures ");
	heConsolePrint("> print_instances ");
	heConsolePrint("> print_instance string: name");
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
	heConsoleRegisterCommand("set_exposure", &front_command_set_exposure);
	heConsoleRegisterCommand("set_gamma", &front_command_set_gamma);
	heConsoleRegisterCommand("set_name", &front_command_set_name);
	heConsoleRegisterCommand("teleport", &front_command_teleport);
	heConsoleRegisterCommand("toggle_physics_debug", &front_command_toggle_physics_debug);
	heConsoleRegisterCommand("toggle_profiler", &front_command_toggle_profiler);
	heConsoleRegisterCommand("export_texture", &front_command_export_texture);
	heConsoleRegisterCommand("export_textures", &front_command_export_textures);
	heConsoleRegisterCommand("export_assets", &front_command_export_assets);
	heConsoleRegisterCommand("export_skybox", &front_command_export_skybox);
	heConsoleRegisterCommand("print_memory", &front_command_print_memory);
	heConsoleRegisterCommand("print_textures", &front_command_print_textures);
	heConsoleRegisterCommand("print_instances", &front_command_print_instances);
	heConsoleRegisterCommand("print_instance", &front_command_print_instance);
	heConsoleRegisterCommand("help", &front_command_help);
};

