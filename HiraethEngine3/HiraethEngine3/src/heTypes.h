#ifndef HE_TYPES_H
#define HE_TYPES_H

#pragma warning( disable : 26812 ) // use enum classes over enum typedefs

#ifdef HE_EXPORTS
#define HE_API __declspec(dllexport)
#else
#define HE_API __declspec(dllimport)
#endif

// possible macro definitions:
// HE_USE_WIN32
// HE_USE_STBI
// HE_USE_BINARY
// HE_ENABLE_ERROR_CHECKING
// HE_ENABLE_HOTSWAWP_SHADER
// HE_ENABLE_LOGGING_ALL
// HE_ENABLE_LOG_MSG
// HE_ENABLE_DEBUG_MSG
// HE_ENABLE_WARNING_MSG
// HE_ENABLE_ERROR_MSG
// HE_ENABLE_NAMES

typedef bool b8;

typedef enum HeKeyCode {
	HE_KEY_0		   = 0x030,
	HE_KEY_1		   = 0x031,
	HE_KEY_2		   = 0x032,
	HE_KEY_3		   = 0x033,
	HE_KEY_4		   = 0x034,
	HE_KEY_5		   = 0x035,
	HE_KEY_6		   = 0x036,
	HE_KEY_7		   = 0x037,
	HE_KEY_8		   = 0x038,
	HE_KEY_9		   = 0x039,
	HE_KEY_A		   = 0x041,
	HE_KEY_B		   = 0x042,
	HE_KEY_C		   = 0x043,
	HE_KEY_D		   = 0x044,
	HE_KEY_E		   = 0x045,
	HE_KEY_F		   = 0x046,
	HE_KEY_G		   = 0x047,
	HE_KEY_H		   = 0x048,
	HE_KEY_I		   = 0x049,
	HE_KEY_J		   = 0x04A,
	HE_KEY_K		   = 0x04B,
	HE_KEY_L		   = 0x04C,
	HE_KEY_M		   = 0x04D,
	HE_KEY_N		   = 0x04E,
	HE_KEY_O		   = 0x04F,
	HE_KEY_P		   = 0x050,
	HE_KEY_Q		   = 0x051,
	HE_KEY_R		   = 0x052,
	HE_KEY_S		   = 0x053,
	HE_KEY_T		   = 0x054,
	HE_KEY_U		   = 0x055,
	HE_KEY_V		   = 0x056,
	HE_KEY_W		   = 0x057,
	HE_KEY_X		   = 0x058,
	HE_KEY_Y		   = 0x059,
	HE_KEY_Z		   = 0x05A,
	HE_KEY_CAPS		   = 0x14,
	HE_KEY_SPACE	   = 0x20,
	HE_KEY_LSHIFT	   = 0xA0,
	HE_KEY_RSHIFT	   = 0xA1,
	HE_KEY_ESCAPE	   = 0x1B,
	HE_KEY_ENTER	   = 0x0D,
	HE_KEY_BACKSPACE   = 0x08,
	HE_KEY_LCONTROL	   = 0xA2,
	HE_KEY_RCONTROL	   = 0xA3,
	HE_KEY_ARROW_LEFT  = 0x25,
	HE_KEY_ARROW_UP    = 0x26,
	HE_KEY_ARROW_RIGHT = 0x27,
	HE_KEY_ARROW_DOWN  = 0x28,
	HE_KEY_END         = 0x23,
	HE_KEY_DELETE      = 0x2E,
	HE_KEY_TAB         = 0x09,
	HE_KEY_F1		   = 0x70,
	HE_KEY_F2		   = 0x71,
	HE_KEY_F3		   = 0x72,
	HE_KEY_F4		   = 0x73,
	HE_KEY_F5		   = 0x74,
	HE_KEY_F6		   = 0x75,
	HE_KEY_F7		   = 0x76,
	HE_KEY_F8		   = 0x77,
	HE_KEY_F9		   = 0x78,
	HE_KEY_F10		   = 0x79,
	HE_KEY_F11		   = 0xFA,
	HE_KEY_F12		   = 0x7B
} HeKeyCode;

typedef enum HeDataType {
	HE_DATA_TYPE_NONE	 = 0,
	HE_DATA_TYPE_INT	 = 0x1404,
	HE_DATA_TYPE_UINT	 = 0x1405,
	HE_DATA_TYPE_VEC2	 = 0x8B50,
	HE_DATA_TYPE_VEC3	 = 0x8B51,
	HE_DATA_TYPE_VEC4	 = 0x8B52,
	HE_DATA_TYPE_BOOL	 = 0x8B56,
	HE_DATA_TYPE_FLOAT	 = 0x1406,
	HE_DATA_TYPE_DOUBLE	 = 0x1406,
	HE_DATA_TYPE_COLOUR	 = 0x8B52,
	HE_DATA_TYPE_MAT4	 = 0x8B5C,
} HeDataType;

typedef enum HeColourFormat {
	HE_COLOUR_FORMAT_NONE	= 0,
	HE_COLOUR_FORMAT_RGB8	= 0x8051,
	HE_COLOUR_FORMAT_RGBA8	= 0x8058,
	HE_COLOUR_FORMAT_RGB16	= 0x881B,
	HE_COLOUR_FORMAT_RGBA16 = 0x881A,
	HE_COLOUR_FORMAT_RGB32	= 0x8815,
	HE_COLOUR_FORMAT_RGBA32 = 0x8814,
	HE_COLOUR_FORMAT_COMPRESSED_RGBA8 = 0x0001
} HeColourFormat;

typedef enum HeAccessType {
	HE_ACCESS_NONE	= 0,
	HE_ACCESS_READ_ONLY	 = 0x88B8,
	HE_ACCESS_WRITE_ONLY = 0x88B9,
	HE_ACCESS_READ_WRITE = 0x88BA
} HeAccessType;

typedef enum HeLightSourceType {
	HE_LIGHT_SOURCE_TYPE_NONE,
	HE_LIGHT_SOURCE_TYPE_POINT,
	HE_LIGHT_SOURCE_TYPE_DIRECTIONAL,
	HE_LIGHT_SOURCE_TYPE_SPOT
} HeSourceLightType;

typedef enum HeDebugInfoFlags {
	HE_DEBUG_INFO_LIGHTS	= 0b00001,
	HE_DEBUG_INFO_INSTANCES = 0b00010,
	HE_DEBUG_INFO_CAMERA	= 0b00100,
	HE_DEBUG_INFO_ENGINE	= 0b01000,
	HE_DEBUG_INFO_MEMORY	= 0b10000
} HeDebugInfoFlags;

typedef enum HePhysicsShape {
	HE_PHYSICS_SHAPE_NONE,
	HE_PHYSICS_SHAPE_CONVEX_MESH,
	HE_PHYSICS_SHAPE_CONCAVE_MESH,
	HE_PHYSICS_SHAPE_BOX,
	HE_PHYSICS_SHAPE_SPHERE,
	HE_PHYSICS_SHAPE_CAPSULE,
} HePhysicsShapeType;

typedef enum HeVboUsage {
	// modified once and used few times
	HE_VBO_USAGE_STREAM = 0x88E0,
	// modified once and used often
	HE_VBO_USAGE_STATIC = 0x88E4,
	// modified and used often
	HE_VBO_USAGE_DYNAMIC = 0x88E8
} HeVboUsage;

typedef enum HeVaoType {
	HE_VAO_TYPE_NONE,
	HE_VAO_TYPE_POINTS	  = 0x0000,
	HE_VAO_TYPE_LINES	  = 0x0001,
	HE_VAO_TYPE_TRIANGLES = 0x0004
} HeVaoType;

typedef enum HeFrameBufferBits {
	HE_FRAME_BUFFER_BIT_COLOUR	= 0x4000,
	HE_FRAME_BUFFER_BIT_DEPTH	= 0x0100,
	HE_FRAME_BUFFER_BIT_STENCIL = 0x0400
} HeFrameBufferBits;

// functions for depth and stencil tests
typedef enum HeFragmentTestFunction {
	// always passes
	HE_FRAGMENT_TEST_ALWAYS = 0x0207,
	// never passes
	HE_FRAGMENT_TEST_NEVER	= 0x0200,
	// passes if the given value is at least a threshold
	HE_FRAGMENT_TEST_GEQUAL = 0x0206,
	// passes if the given value is less or equal to a threshold
	HE_FRAGMENT_TEST_LEQUAL = 0x0203,
	// passes if the given value is less than the threshold
	HE_FRAGMENT_TEST_LESS	= 0x0201,
} HeFragmentTestFunction;

typedef enum HeShaderType {
	HE_SHADER_TYPE_FRAGMENT = 0x8B30,
	HE_SHADER_TYPE_VERTEX	= 0x8B31,
	HE_SHADER_TYPE_GEOMETRY = 0x8DD9
} HeShaderType;

typedef enum HeMemoryType {
	HE_MEMORY_TYPE_NONE = 0,
	HE_MEMORY_TYPE_TEXTURE,
	HE_MEMORY_TYPE_VAO,
	HE_MEMORY_TYPE_FBO,
	HE_MEMORY_TYPE_SHADER,
	HE_MEMORY_TYPE_CONTEXT
} HeMemoryType;

typedef enum HeTextureParameter {
	HE_TEXTURE_NONE				  = 0,
	HE_TEXTURE_FILTER_LINEAR	  = 0b0000001,
	HE_TEXTURE_FILTER_BILINEAR	  = 0b0000010,
	HE_TEXTURE_FILTER_TRILINEAR	  = 0b0000100,
	HE_TEXTURE_FILTER_ANISOTROPIC = 0b0001000,
	HE_TEXTURE_CLAMP_EDGE		  = 0b0010000,
	HE_TEXTURE_CLAMP_BORDER		  = 0b0100000,
	HE_TEXTURE_CLAMP_REPEAT		  = 0b1000000
} HeTextureParameter;

typedef enum HeConsoleState {
	HE_CONSOLE_STATE_CLOSED,
	HE_CONSOLE_STATE_OPEN,
	HE_CONSOLE_STATE_OPEN_FULL,
} HeConsoleState;

typedef enum HeTextureRenderMode {
	HE_TEXTURE_RENDER_2D       = 0b0001,
	HE_TEXTURE_RENDER_CUBE_MAP = 0b0010,
	HE_TEXTURE_RENDER_HDR      = 0b0100,
} HeTextureRenderMode;

typedef enum HeRenderMode {
	HE_RENDER_MODE_FORWARD,
	HE_RENDER_MODE_DEFERRED
} HeRenderMode;

typedef enum HeTextAlignMode {
	HE_TEXT_ALIGN_LEFT,
	HE_TEXT_ALIGN_RIGHT,
	HE_TEXT_ALIGN_CENTER,
} HeTextAlignMode;

typedef enum HeUiInteractionStatus {
    HE_UI_INTERACTION_STATUS_NONE,
    HE_UI_INTERACTION_STATUS_HOVERED, // mouse is over this widget
    HE_UI_INTERACTION_STATUS_PRESSED, // mouse just pressed on this widget in the last frame(therefore is currently hovered)
    HE_UI_INTERACTION_STATUS_CLICKED, // mouse just pressed on this widget (doesnt have to be hovered), (should only be one widget at a time in total)
    HE_UI_INTERACTION_STATUS_UNPRESSED, // mouse just pressed somewhere else on the screen, but this widget was clicked before
} HeUiInteractionStatus;

// a small macro to enable bitwise operations on enums
#define HE_ENABLE_BIT(T) inline T operator| (T a, T b) { return (T)((int) a | (int) b); }; \
	inline T operator& (T a, T b) { return (T)((int) a & (int) b); };	\
	inline T operator|=(T a, T b) { a = a & b; return a; };

HE_ENABLE_BIT(HeDebugInfoFlags);
HE_ENABLE_BIT(HeFrameBufferBits);
HE_ENABLE_BIT(HeTextureParameter);
HE_ENABLE_BIT(HeTextureRenderMode);

#endif
