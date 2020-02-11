#pragma once

#ifdef HE_EXPORTS
#define HE_API __declspec(dllexport)
#else
#define HE_API __declspec(dllimport)
#endif

typedef enum HeKeyCode {
    HE_KEY_0		 = 0x030,
    HE_KEY_1		 = 0x031,
    HE_KEY_2		 = 0x032,
    HE_KEY_3		 = 0x033,
    HE_KEY_4		 = 0x034,
    HE_KEY_5		 = 0x035,
    HE_KEY_6		 = 0x036,
    HE_KEY_7		 = 0x037,
    HE_KEY_8		 = 0x038,
    HE_KEY_9		 = 0x039,
    HE_KEY_A		 = 0x041,
    HE_KEY_B		 = 0x042,
    HE_KEY_C		 = 0x043,
    HE_KEY_D		 = 0x044,
    HE_KEY_E		 = 0x045,
    HE_KEY_F		 = 0x046,
    HE_KEY_G		 = 0x047,
    HE_KEY_H		 = 0x048,
    HE_KEY_I		 = 0x049,
    HE_KEY_J		 = 0x04A,
    HE_KEY_K		 = 0x04B,
    HE_KEY_L		 = 0x04C,
    HE_KEY_M		 = 0x04D,
    HE_KEY_N		 = 0x04E,
    HE_KEY_O		 = 0x04F,
    HE_KEY_P		 = 0x050,
    HE_KEY_Q		 = 0x051,
    HE_KEY_R		 = 0x052,
    HE_KEY_S		 = 0x053,
    HE_KEY_T		 = 0x054,
    HE_KEY_U		 = 0x055,
    HE_KEY_V		 = 0x056,
    HE_KEY_W		 = 0x057,
    HE_KEY_X		 = 0x058,
    HE_KEY_Y		 = 0x059,
    HE_KEY_Z		 = 0x05A,
    HE_KEY_CAPS		 = 0x14,
    HE_KEY_SPACE     = 0x20,
    HE_KEY_LSHIFT    = 0xA0,
    HE_KEY_RSHIFT    = 0xA1,
    HE_KEY_ESCAPE    = 0x1B,
    HE_KEY_ENTER     = 0x0D,
    HE_KEY_BACKSPACE = 0x08,
    HE_KEY_F1		 = 0x70,
    HE_KEY_F2		 = 0x71,
    HE_KEY_F3		 = 0x72,
    HE_KEY_F4		 = 0x73,
    HE_KEY_F5		 = 0x74,
    HE_KEY_F6		 = 0x75,
    HE_KEY_F7		 = 0x76,
    HE_KEY_F8		 = 0x77,
    HE_KEY_F9		 = 0x78,
    HE_KEY_F10		 = 0x79,
    HE_KEY_F11		 = 0xFA,
    HE_KEY_F12		 = 0x7B
} HeKeyCode;

typedef enum HeUniformDataType {
    HE_UNIFORM_DATA_TYPE_NONE,
    HE_UNIFORM_DATA_TYPE_INT     = 0x1404,
    HE_UNIFORM_DATA_TYPE_VEC2    = 0x8B50,
    HE_UNIFORM_DATA_TYPE_VEC3    = 0x8B51,
    HE_UNIFORM_DATA_TYPE_VEC4    = 0x8B52,
    HE_UNIFORM_DATA_TYPE_BOOL    = 0x8B56,
    HE_UNIFORM_DATA_TYPE_FLOAT   = 0x1406,
    HE_UNIFORM_DATA_TYPE_DOUBLE  = 0x1406,
    HE_UNIFORM_DATA_TYPE_COLOUR  = 0x8B52,
    HE_UNIFORM_DATA_TYPE_MAT4    = 0x8B5C,
} HeUniformDataType;

typedef enum HeFboFlags {
    HE_FBO_FLAG_NONE                = 0,
    HE_FBO_FLAG_HDR                 = 0b0001,
    HE_FBO_FLAG_DEPTH_RENDER_BUFFER = 0b0010,
    HE_FBO_FLAG_DEPTH_TEXTURE       = 0b0100
} HeFboFlags;

typedef enum HeColourFormat {
    HE_COLOUR_FORMAT_NONE   = 0,
    HE_COLOUR_FORMAT_RGBA8  = 0x8058,
    HE_COLOUR_FORMAT_RGBA16 = 0x881A,
    HE_COLOUR_FORMAT_RGBA32 = 0x8814
} HeColourFormat;

typedef enum HeAccessType {
    HE_ACCESS_READ_ONLY  = 0x88B8,
    HE_ACCESS_WRITE_ONLY,
    HE_ACCESS_READ_WRITE,
} HeAccessType;

typedef enum HeLightType {
    HE_LIGHT_TYPE_NONE,
    HE_LIGHT_TYPE_POINT,
    HE_LIGHT_TYPE_DIRECTIONAL,
    HE_LIGHT_TYPE_SPOT
} HeLightType;
