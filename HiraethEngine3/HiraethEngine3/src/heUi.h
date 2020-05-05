#ifndef HE_UI_H
#define HE_UI_H

#include "heTypes.h"
#include <string>

struct HeTextInput {
	std::string inputString;
	uint32_t    cursorPosition = 0;
	b8          active         = false;
};

#endif // HE_UI_H
