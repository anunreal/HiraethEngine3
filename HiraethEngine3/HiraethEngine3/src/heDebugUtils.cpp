#include "hepch.h"
#include "heDebugUtils.h"
#include "heD3.h"
#include "heCore.h"
#include "heUtils.h"
#include "heRenderer.h"
#include "heBinary.h"
#include "heConverter.h"
#include "heWin32Layer.h"
#include "heConsole.h"
#include <sstream> // for he_float_to_string
#include <iomanip> // for std::setprecision 

HeDebugInfo heDebugInfo;
HeProfiler  heProfiler;

inline void heFlagStringBuilder(std::string& output, std::string const& in) {
    if(output.size() == 0)
        output = in;
    else
        output += ", " + in;
};

void heDebugRequestInfo(const HeDebugInfoFlags flags) {
    // print debug information
    if(flags == HE_DEBUG_INFO_LIGHTS) {
        heDebugPrint("=== LIGHTS ===\n");
        for(const auto& all : heD3Level->lights) {
            std::string string;
            he_to_string(&all, string);
            heDebugPrint(string);
        }
        
        heDebugPrint("=== LIGHTS ===");
    }
    
    if(flags == HE_DEBUG_INFO_INSTANCES) {
        heDebugPrint("=== INSTANCES ===\n");
        uint16_t counter = 0;
        for(const auto& all : heD3Level->instances) {
            std::string string;
            he_to_string(&all, string);
            heDebugPrint('[' + std::to_string(counter++) + "] = " + string);
        }
        
        heDebugPrint("=== INSTANCES ===");
    }
    
    if(flags & HE_DEBUG_INFO_CAMERA) {
        heDebugPrint("=== CAMERA ===\n");
        std::string string;
        he_to_string(&heD3Level->camera, string);
        heDebugPrint(string);
        heDebugPrint("=== CAMERA ===");
    }

    /*
    if(flags & HE_DEBUG_INFO_ENGINE) {
        heDebugPrint("=== ENGINE ===\n");
        if(heRenderEngine->renderMode == HE_RENDER_MODE_DEFERRED) {
            std::string string;
            he_to_string(&heRenderEngine->deferred.gBufferFbo, string);
            heDebugPrint("gBuffer = " + string);
        }
        heDebugPrint("=== ENGINE ===");
    }
    */

    if(flags & HE_DEBUG_INFO_MEMORY) {
        heDebugPrint("=== MEMORY ===");
        uint64_t total = heMemoryTracker[HE_MEMORY_TYPE_TEXTURE] +
            heMemoryTracker[HE_MEMORY_TYPE_VAO] + 
            heMemoryTracker[HE_MEMORY_TYPE_FBO] +
            heMemoryTracker[HE_MEMORY_TYPE_SHADER] + 
            heMemoryTracker[HE_MEMORY_TYPE_CONTEXT];
        
        heDebugPrint("textures = " + he_bytes_to_string(heMemoryTracker[HE_MEMORY_TYPE_TEXTURE]));
        heDebugPrint("vaos     = " + he_bytes_to_string(heMemoryTracker[HE_MEMORY_TYPE_VAO]));
        heDebugPrint("fbos     = " + he_bytes_to_string(heMemoryTracker[HE_MEMORY_TYPE_FBO]));
        heDebugPrint("context  = " + he_bytes_to_string(heMemoryTracker[HE_MEMORY_TYPE_CONTEXT]));
        heDebugPrint("shaders  = " + he_bytes_to_string(heMemoryTracker[HE_MEMORY_TYPE_SHADER]));
        heDebugPrint("total    = " + he_bytes_to_string(total));
        heDebugPrint("total    = " + he_bytes_to_string(heMemoryGetUsage()));
        heDebugPrint("=== MEMORY ===");
    }
};

b8 heDebugIsInfoRequested(const HeDebugInfoFlags flag) {
    b8 set = heDebugInfo.flags & flag;
    heDebugInfo.flags = heDebugInfo.flags & ~flag;
    return set;
};

void heDebugSetOutput(std::ostream* stream) {
    heDebugInfo.stream = stream;
};

void heDebugPrint(const std::string& message) {
    std::string output;
    output.append(message);
    output.push_back('\n');
    
    if (heDebugInfo.stream == nullptr) {
        heConsolePrint(output);
        std::cout << output;
        std::cout.flush();
    } else {
        heConsolePrint(output);
        *heDebugInfo.stream << output;
        heDebugInfo.stream->flush();
    }
};


std::string he_to_string(HeLightSourceType const& type) {
    std::string s;
    
    switch(type) {
        case HE_LIGHT_SOURCE_TYPE_DIRECTIONAL:
        s = "directional";
        break;
        
        case HE_LIGHT_SOURCE_TYPE_POINT:
        s = "point";
        break;
        
        case HE_LIGHT_SOURCE_TYPE_SPOT:
        s = "spot";
        break;
    }
    
    return s;
};

std::string he_to_string(HePhysicsShape const& type) {
    std::string s;
    
    switch(type) {
        case HE_PHYSICS_SHAPE_CONVEX_MESH:
        s = "convex";
        break;
        
        case HE_PHYSICS_SHAPE_CONCAVE_MESH:
        s = "concave";
        break;
        
        case HE_PHYSICS_SHAPE_BOX:
        s = "box";
        break;
        
        case HE_PHYSICS_SHAPE_SPHERE:
        s = "sphere";
        break;
        
        case HE_PHYSICS_SHAPE_CAPSULE:
        s = "capsule";
        break;
    };
    
    return s;
};

std::string he_to_string(HeColourFormat const& type) {
    std::string s;
    
    switch(type) {
    case HE_COLOUR_FORMAT_RGB8:
        s = "rgb8";
        break;
        
    case HE_COLOUR_FORMAT_RGBA8:
        s = "rgba8";
        break;
        
    case HE_COLOUR_FORMAT_RGB16:
        s = "rgb16";
        break;
        
    case HE_COLOUR_FORMAT_RGBA16:
        s = "rgba16";
        break;

    case HE_COLOUR_FORMAT_COMPRESSED_RGB8:
        s = "compressed_rgb8";
        break;

    case HE_COLOUR_FORMAT_COMPRESSED_RGBA8:
        s = "compressed_rgba8";
        break;

    default:
        s = "unknown_colour_format";
        break;
    };
    
    return s;
};

std::string he_to_string(HeMemoryType const& type) {
    std::string s;

    switch(type) {
    case HE_MEMORY_TYPE_TEXTURE:
        s = "textures";
        break;

    case HE_MEMORY_TYPE_VAO:
        s = "vaos";
        break;

    case HE_MEMORY_TYPE_FBO:
        s = "fbos";
        break;
    }

    return s;
    
};

std::string he_gl_error_to_string(uint32_t const& error) {
    std::string s;

    switch(error) {
    case 0x0500:
        s = "Invalid Enum";
        break;

    case 0x0501:
        s = "Invalid Value";
        break;

    case 0x0502:
        s = "Invalid Operation";
        break;

    case 0x0503:
        s = "Stack Overflow";
        break;

    case 0x0504:
        s = "Stack Underflow";
        break;

    case 0x0505:
        s = "Out of Memory";
        break;

    case 0x0506:
        s = "Invalid Framebuffer Operation";
        break;

    case 0x0507:
        s = "Lost Context";
        break;
        
    default:
        s = std::to_string(error);
        break;
    }
    
    return s;
};


std::string he_to_string(HeShaderData const* ptr) {
    std::string data;
    
    switch(ptr->type) {
        case HE_DATA_TYPE_INT:
        case HE_DATA_TYPE_UINT:
        case HE_DATA_TYPE_BOOL:
        data = std::to_string(ptr->_int);
        break;
        
        case HE_DATA_TYPE_FLOAT:
        data = std::to_string(ptr->_float);
        break;
        
        case HE_DATA_TYPE_VEC2:
        data = hm::to_string(ptr->_vec2);
        break;
        
        case HE_DATA_TYPE_VEC3:
        data = hm::to_string(ptr->_vec3);
        break;
        
        case HE_DATA_TYPE_COLOUR:
        data = hm::to_string(ptr->_colour);
        break;
    }
    
    return data;
};


void he_to_string(HeTexture const* ptr, std::string& output, std::string const& prefix) {
    output += prefix + "HeTexture {\n";
#ifdef HE_ENABLE_NAMES
    output += prefix + "\tname       = " + ptr->name + '\n';
#endif  
    output += prefix + "\treferences = " + std::to_string(ptr->referenceCount) + '\n';
    output += prefix + "\tid         = " + std::to_string(ptr->textureId)      + '\n';
    output += prefix + "\tformat     = " + he_to_string(ptr->format)           + '\n';
    output += prefix + "\tsize       = " + hm::to_string(ptr->size)            + '\n';
    output += prefix + "\tchannels   = " + std::to_string(ptr->channels)       + '\n';
    output += prefix + "\tcubeMap    = " + std::to_string(ptr->cubeMap)        + '\n';
    output += prefix + "\tmipmaps    = " + std::to_string(ptr->mipmapCount)    + '\n';
    output += prefix + "\tmemory     = " + he_bytes_to_string(ptr->memory)     + '\n';
    output += prefix + "};\n";
};

void he_to_string(HeFboAttachment const* ptr, std::string& output, std::string const& prefix) {
    output += prefix + "HeFboAttachment {\n";
    if(ptr->texture)
        output += prefix + "\ttexture = " + std::to_string(ptr->id)      + '\n';
    else {
        output += prefix + "\tbuffer  = " + std::to_string(ptr->id)      + '\n';
        output += prefix + "\tsamples = " + std::to_string(ptr->samples) + '\n';
    }
    output += prefix + "\tformat  = " + he_to_string(ptr->format)        + '\n';
    output += prefix + "\tsize    = " + hm::to_string(ptr->size)         + '\n';
    output += prefix + "\tmemory  = " + he_bytes_to_string(ptr->memory)  + '\n';
    output += prefix + "};\n";
};

void he_to_string(HeFbo const* ptr, std::string& output, std::string const& prefix) {
    output += prefix + "HeFbo {\n";
#ifdef HE_ENABLE_NAMES
    output += prefix + "\tname    = " + ptr->name + '\n';
#endif
    output += prefix + "\tid      = " + std::to_string(ptr->fboId) + '\n';
    output += prefix + "\tsize    = " + hm::to_string(ptr->size) + '\n';
    std::string depth;
    he_to_string(&ptr->depthAttachment, depth, prefix + '\t');
    output += prefix + "\tdepth   = " + depth + '\n';
    //output += prefix + "\tmemory  = " + std::to_string(ptr->memory) + '\n';

    output += prefix + "\tattachments:\n";
    for(auto const& all : ptr->colourAttachments)
        he_to_string(&all, output, prefix + "\t\t");
    
    output += prefix + "};\n";
};

void he_to_string(HeD3LightSource const* ptr, std::string& output, std::string const& prefix) {
    std::string type = he_to_string(ptr->type);;
    std::string vectorName = (ptr->type == HE_LIGHT_SOURCE_TYPE_DIRECTIONAL) ? "direction   " : "position    ";
    
    std::string dataString;
    if(ptr->type == HE_LIGHT_SOURCE_TYPE_POINT)
        dataString += "\tlight_values = " + std::to_string(ptr->data[0]) + "/" + std::to_string(ptr->data[1]) + "/" + std::to_string(ptr->data[2]);
    else if(ptr->type == HE_LIGHT_SOURCE_TYPE_SPOT) {
        dataString += "\tdirection    = " + hm::to_string(hm::vec3f(ptr->data[0], ptr->data[1], ptr->data[2])) + "\n";
        dataString += "\tlight circle = " + std::to_string(ptr->data[3]) + " / " + std::to_string(ptr->data[4]) + "\n";
        dataString += "\tlight_values = " + std::to_string(ptr->data[5]) + "/" + std::to_string(ptr->data[6]) + "/" + std::to_string(ptr->data[7]);
    }
    
    std::string active = ((ptr->active) ? "true" : "false");
    output += prefix + "HeD3LightSource {\n";
    output += prefix + "\tactive       = " + active + "\n";
    output += prefix + "\ttype         = " + type + "\n";
    output += prefix + "\tcolour       = " + hm::to_string(ptr->colour) + "\n";
    output += prefix + "\t" + vectorName + " = " + hm::to_string(ptr->vector) + "\n";
    output += prefix + dataString + "\n";
    output += prefix + "};\n";
};

void he_to_string(HeD3Instance const* ptr, std::string& output, std::string const& prefix) {
    output += prefix + "HeD3Instance {\n";
    
#ifdef HE_ENABLE_NAMES
    output += prefix + "\tname     = " + ptr->name + "\n";
    output += prefix + "\tmesh     = " + ptr->mesh->name + "\n";
#endif
    
    if(ptr->material != nullptr)
        he_to_string(ptr->material, output, prefix + '\t');
    output += prefix + "\tposition = " + hm::to_string(ptr->transformation.position) + "\n";
    output += prefix + "\trotation = " + hm::to_string(ptr->transformation.rotation) + "\n";
    output += prefix + "\tscale    = " + hm::to_string(ptr->transformation.scale) + "\n";
    
    if(ptr->physics)
        he_to_string(ptr->physics, output, prefix + '\t');
    
    output += prefix + "};\n";
};

void he_to_string(HeD3Camera const* ptr, std::string& output, std::string const& prefix) {
    output += prefix + "HeD3Camera {\n";
    output += prefix + "\tposition = " + hm::to_string(ptr->position) + "\n";
    output += prefix + "\trotation = " + hm::to_string(ptr->rotation) + "\n";
    output += prefix + "};\n";
};

void he_to_string(HePhysicsShapeInfo const* ptr, std::string& output, std::string const& prefix) {
    output += prefix + "HePhysicsShapeInfo {\n";
    output += prefix + "\tshape       = " + he_to_string(ptr->type) + "\n";
    
    if(ptr->type == HE_PHYSICS_SHAPE_BOX)
        output += prefix + "\thalf-axis   = " + hm::to_string(ptr->box) + '\n';
    else if(ptr->type == HE_PHYSICS_SHAPE_SPHERE)
        output += prefix + "\tradius      = " + std::to_string(ptr->sphere) + '\n';
    else if(ptr->type == HE_PHYSICS_SHAPE_CAPSULE)
        output += prefix + "\tradii       = " + hm::to_string(ptr->capsule) + '\n';
    
    output += prefix + "\tmass        = " + std::to_string(ptr->mass) + '\n';
    output += prefix + "\tfriction    = " + std::to_string(ptr->friction) + '\n';
    output += prefix + "\trestitution = " + std::to_string(ptr->restitution) + '\n';
    output += prefix + "};\n";
};

void he_to_string(HePhysicsActorInfo const* ptr, std::string& output, std::string const& prefix) {
    output += prefix + "HePhysicsActorInfo {\n";
    output += prefix + "\tstepHeight = " + std::to_string(ptr->stepHeight) + '\n';
    output += prefix + "\tjumpHeight = " + std::to_string(ptr->jumpHeight) + '\n';
    output += prefix + "\teyeOffset  = " + std::to_string(ptr->eyeOffset) + '\n';
    output += prefix + "};\n";
};

void he_to_string(HePhysicsComponent const* ptr, std::string& output, std::string const& prefix) {
    output += prefix + "HePhysicsComponent {\n";
    he_to_string(&ptr->shapeInfo, output, prefix + '\t');
    output += prefix + "};\n";
};

void he_to_string(HePhysicsActorSimple const* ptr, std::string& output, std::string const& prefix) {
    output += prefix + "HePhysicsActor {\n";
    he_to_string(&ptr->shapeInfo, output, prefix + '\t');
    he_to_string(&ptr->actorInfo, output, prefix + '\t');
    output += prefix + "};\n";
};

void he_to_string(HeMaterial const* ptr, std::string& output, std::string const& prefix) {
    output += prefix + "HeMaterial {\n";
    output += prefix + "\ttype   = " + std::to_string(ptr->type) + '\n';
#ifdef HE_ENABLE_NAMES
    output += prefix + "\tshader = " + ptr->shader->name + '\n';
    output += prefix + "\ttextures:\n";
    for(auto const& all : ptr->textures)
        output += prefix + "\t\t" + all.first + " = " + all.second->name + '\n';
#endif
    
    output += prefix + "\tuniforms:\n";
    for(auto const& all : ptr->uniforms)
        output += prefix + "\t\t" + all.first + " = " + he_to_string(&all.second) + '\n';
    
    output += prefix + "};\n";
};


// -- other

std::string he_bytes_to_string(uint64_t const bytes) {
    std::string s;

    if(bytes < 1000)
        s = std::to_string(bytes) + "b";
    else if(bytes < 1000000)
        s = he_float_to_string(bytes / 1000.f, 1) + "kb";
    else if(bytes < 1000000000)
        s = he_float_to_string(bytes / 1000000.f, 1) + "mb";
    
    return s;
};

std::string he_float_to_string(float const _float, uint8_t const precision) {
    std::stringstream stream;
    stream << std::fixed << std::setprecision(precision) << _float;
    return stream.str();
};


// -- profiler

void heProfilerCreate(HeFont const* font) {
    heFontCreateScaled(font, &heProfiler.font, 10);
};

void heProfilerAddEntry(std::string const& name, double const duration, hm::colour const& colour) {
    heProfiler.entries[heProfiler.entryOffset++] = HeProfiler::HeProfilerEntry(name, duration, colour);
};

void heProfilerAddEntry(std::string const& name, double const duration) {
    hm::colour c;
    c.r = std::rand() % 255;
    c.g = std::rand() % 255;
    c.b = std::rand() % 255;
    c.a = 255;
    heProfilerAddEntry(name, duration, c);
};

void heProfilerRender(HeRenderEngine* engine) { 
    if(heProfiler.displayed) {
        // render profiler  
        //float const MS_COUNT   = 1000.f / engine->window->windowInfo.fpsCap; 
        float const MS_COUNT   = (float) engine->window->frameTime * 1000.f; // the amount of ms displayed
        float const MAX_WIDTH  = 0.99f;                            // in clip space
        float const PER_MS     = MAX_WIDTH / MS_COUNT;             // clip space per millisecond
        float const HEIGHT     = 15.f;                             // bar height in pixels
        float const BARYOFFSET = (float) engine->window->windowInfo.size.y - 20.f; 
    
        float xoffset = (1.0f - MAX_WIDTH) / 2.f * engine->window->windowInfo.size.x; // in pixels
        float yoffset = BARYOFFSET - HEIGHT - 10;
        
        hm::vec2f colourQuadSize = hm::vec2f(20, 10);
        
        for(uint32_t i = 0; i < heProfiler.entryOffset; ++i) {
            HeProfiler::HeProfilerEntry const& entry = heProfiler.entries[i];
            float widthInPixels = PER_MS * (float) entry.duration * engine->window->windowInfo.size.x;
            if(xoffset + widthInPixels > engine->window->windowInfo.size.x * MAX_WIDTH)
                widthInPixels = engine->window->windowInfo.size.x * MAX_WIDTH - xoffset;
            
            // draw in graph
            heUiRenderQuad(engine, hm::vec2f(xoffset, BARYOFFSET), hm::vec2f(xoffset, BARYOFFSET + HEIGHT), hm::vec2f(xoffset + widthInPixels, BARYOFFSET), hm::vec2f(xoffset + widthInPixels, BARYOFFSET + HEIGHT), entry.colour);

            // draw description
            heUiRenderQuad(engine, hm::vec2f(10, yoffset), hm::vec2f(10, yoffset + colourQuadSize.y), hm::vec2f(10 + colourQuadSize.x, yoffset), hm::vec2f(10, yoffset) + colourQuadSize, entry.colour);

            heUiRenderText(engine, &heProfiler.font, entry.name + ":", hm::vec2f(10 + colourQuadSize.x + 5, yoffset), hm::colour(255)); // immediate mode
            heUiRenderText(engine, &heProfiler.font, std::to_string(entry.duration) + "ms", hm::vec2f(10 + colourQuadSize.x + 155, yoffset), hm::colour(255), HE_TEXT_ALIGN_RIGHT); // immediate mode
            
            xoffset += widthInPixels;   
            yoffset -= 15;
        }

        // draw frame time
        heUiRenderText(engine, &heProfiler.font, "Frame: " + std::to_string((int) (1. / engine->window->frameTime)) + " (" + std::to_string(engine->window->frameTime) + ")", hm::vec2f(10, yoffset), hm::colour(255));

        // draw sleep time
        float remaining = (MAX_WIDTH * engine->window->windowInfo.size.x) - xoffset;
        if(remaining > 0.f)
            heUiRenderQuad(engine, hm::vec2f(xoffset, BARYOFFSET), hm::vec2f(xoffset, BARYOFFSET + HEIGHT), hm::vec2f(xoffset + remaining, BARYOFFSET), hm::vec2f(xoffset + remaining, BARYOFFSET + HEIGHT), hm::colour(0));
    }   
};

void heProfilerFrameStart() {
    heProfiler.currentMark = heWin32TimeGet();
    heProfiler.entryOffset = 0;
};

void heProfilerFrameMark(std::string const& name, hm::colour const& colour) {
    __int64 now = heWin32TimeGet();
    double duration = heWin32TimeCalculateMs(now - heProfiler.currentMark);
    heProfiler.currentMark = now;
    heProfilerAddEntry(name, duration, colour);
};

void heProfilerFrameMark(std::string const& name) {
    __int64 now = heWin32TimeGet();
    double duration = heWin32TimeCalculateMs(now - heProfiler.currentMark);
    heProfiler.currentMark = now;
    heProfilerAddEntry(name, duration);
};

void heProfilerToggleDisplay() {
    heProfiler.displayed = !heProfiler.displayed;
};

