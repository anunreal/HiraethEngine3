#ifndef HE_DEBUG_UTILS_H
#define HE_DEBUG_UTILS_H

#include "heTypes.h"
#include <string>
#include "hm/colour.hpp"
#include "heAssets.h"

struct HeRenderEngine; // used for profiler

// used to print out debug information on the fly
struct HeDebugInfo {
    // a pointer to a stream to which to print the information (can be a file stream). If this is nullptr, std::cout
    // will be used as default
    std::ostream* stream = nullptr;
    // flags set. When a flag is set, the information will be printed the next time its accessed (i.e. for the light
    // the next time a level is rendered), then that flag will be removed.
    uint32_t flags = 0;
};

struct HeProfiler {
	struct HeProfilerEntry {
		double      duration;
		hm::colour  colour;
		std::string name;

		HeProfilerEntry() : name(""), duration(-1.), colour(0) {}; 
		HeProfilerEntry(std::string const& name, double const duration, hm::colour const& colour) :
			name(name), duration(duration), colour(colour) {};
	};
	
	__int64 currentMark = 0; // marks a time stamp
	uint32_t entryOffset = 0;
	b8 displayed = false;
	HeProfilerEntry entries[20];
	HeScaledFont font;
};

extern HeProfiler heProfiler;

// returns true if the given flag was requested before (since the last check). This will remove the flag
extern HE_API inline b8 heDebugIsInfoRequested(const HeDebugInfoFlags flag);
// requests given debug information. This will set the given flag.
extern HE_API inline void heDebugRequestInfo(const HeDebugInfoFlags flags);
// sets the output stream for debug information (heDebugPrint)
extern HE_API inline void heDebugSetOutput(std::ostream* stream);
// prints given debug information to the stream set with heDebugSetOutput
extern HE_API void heDebugPrint(const std::string& message);


// -- enums -- (returning string because these are simple)

extern HE_API std::string he_to_string(HeLightSourceType const& type);
extern HE_API std::string he_to_string(HePhysicsShape    const& type);
extern HE_API std::string he_to_string(HeColourFormat    const& type);
extern HE_API std::string he_to_string(HeMemoryType      const& type);


// -- helper structs

struct HeShaderData;
extern HE_API std::string he_to_string(HeShaderData const* ptr);


// -- structs -- (a big string, avoid copying with reference)

struct HeTexture;
extern HE_API void he_to_string(HeTexture const* ptr, std::string& output, std::string const& prefix = "");

struct HeFboAttachment;
extern HE_API void he_to_string(HeFboAttachment const* ptr, std::string& output, std::string const& prefix = "");

struct HeFbo;
extern HE_API void he_to_string(HeFbo const* ptr, std::string& output, std::string const& prefix = "");

struct HeD3LightSource;
extern HE_API void he_to_string(HeD3LightSource const* ptr, std::string& output, std::string const& prefix = "");

struct HeD3Instance;
extern HE_API void he_to_string(HeD3Instance const* ptr, std::string& output, std::string const& prefix = "");

struct HeD3Camera;
extern HE_API void he_to_string(HeD3Camera const* ptr, std::string& output, std::string const& prefix = "");

struct HePhysicsShapeInfo;
extern HE_API void he_to_string(HePhysicsShapeInfo const* ptr, std::string& output, std::string const& prefix = "");

struct HePhysicsActorInfo;
extern HE_API void he_to_string(HePhysicsActorInfo const* ptr, std::string& output, std::string const& prefix = "");

struct HePhysicsComponent;
extern HE_API void he_to_string(HePhysicsComponent const* ptr, std::string& output, std::string const& prefix = "");

struct HePhysicsActor;
extern HE_API void he_to_string(HePhysicsActor const* ptr, std::string& output, std::string const& prefix = "");

struct HeMaterial;
extern HE_API void he_to_string(HeMaterial const* ptr, std::string& output, std::string const& prefix = "");


// -- other

// returns given byte count formatted into bytes, kb or mb
extern HE_API std::string he_bytes_to_string(uint64_t const bytes);
// formats a float into a string with given precision
extern HE_API std::string he_float_to_string(float const _float, uint8_t const precision);


// -- profiler

// adds a new entry to the profiler for this frame. 
extern HE_API void heProfilerAddEntry(std::string const& name, double duration, hm::colour const& colour);
extern HE_API void heProfilerAddEntry(std::string const& name, double duration);
extern HE_API void heProfilerFrameStart();
extern HE_API void heProfilerFrameMark(std::string const& name, hm::colour const& colour);
extern HE_API void heProfilerRender(HeRenderEngine* engine);
extern HE_API void heProfilerToggleDisplay();


// -- commands

// tries to run a command. These commands are mainly for dynamic debugging (print debug info, set variables...).
// If this is no valid command, false is returned
extern HE_API b8 heCommandRun(std::string const& command);
// runs a thread that checks the console for input and then runs commands. This must be run in a seperate thread as
// it will block io. The function will run as long as running is true. If running is a nullptr, the function will
// run until the application closes
extern HE_API void heCommandThread(b8* running);
// print all errors in the queue (see heErrorSaveAll())
extern HE_API void heErrorsPrint();

#endif //HE_DEBUG_UTILS_H
