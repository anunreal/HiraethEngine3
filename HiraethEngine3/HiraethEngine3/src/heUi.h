#ifndef HE_UI_H
#define HE_UI_H

#include "heTypes.h"
#include "heAssets.h"
#include "hm/hm.hpp"
#include <string>
#include <vector>

struct HeTextInput {
	b8           active         = false; // whether the user should be able to type into this input right now
    b8           entered        = false; // gets set to true if this text input is active and the enter key was pressed
    uint32_t     cursorPosition = 0; // in chars, as offset
	std::string  inputString;
	HeScaledFont font;
	float        timeSinceLastInput = 0.f;

    
    // history feature

    std::vector<std::string> history;
	int32_t historyIndex = -1; // when this is -1, we type a new text. Else this is the index of the history that we are currently seeing

    
    // auto complete feature

    std::vector<std::string> autoCompleteOptions; // all currently fitting options
    std::vector<std::string> autoCompleteList; // list of all "expected" strings that we may want to autocomplete to if they fit
    uint32_t autoCompleteCycle = 0; // index to autoCompleteOptions
};

struct HeUiTextField {
    hm::colour  backgroundColour;
    hm::vec2f   size;     // in pixels
    hm::vec2f   position; // in screen space
    HeTextInput input;
};

extern HE_API void heUiCreate();
extern HE_API void heUiSetActiveInput(HeTextInput* input);

#endif // HE_UI_H
