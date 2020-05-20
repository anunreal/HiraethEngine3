#include "heUi.h"
#include "heWindow.h"
#include "heRenderer.h"
#include "heWin32Layer.h"

HeTextInput* activeInput = nullptr;

b8 heTextInputCallback(HeWindow* window, uint32_t const input) {
    if(!activeInput || !activeInput->active)
        return false;

    if(heScaledFontHasCharacter(&activeInput->font, input)) {
        activeInput->inputString.insert(activeInput->inputString.begin() + activeInput->cursorPosition, input);
        activeInput->timeSinceLastInput = 0.f;
        activeInput->cursorPosition++;
        activeInput->autoCompleteOptions.clear();
        activeInput->autoCompleteCycle = 0;
    }

    return true;
};

b8 heKeyPressCallback(HeWindow* window, HeKeyCode const input) {
    if(!activeInput || !activeInput->active)
        return false;

    if(input == HE_KEY_BACKSPACE) {
        // backspace
        if(activeInput->cursorPosition > 0) {
            if(window->keyboardInfo.keyStatus[HE_KEY_LCONTROL]) {
                size_t index = activeInput->inputString.substr(0, activeInput->cursorPosition).find_last_of(' ');
                if (index == std::string::npos)
                    index = 0;
                activeInput->inputString.erase(activeInput->inputString.begin() + index, activeInput->inputString.begin() + activeInput->cursorPosition);
                activeInput->cursorPosition = (uint32_t) index;
            } else {
                activeInput->inputString.erase(activeInput->inputString.begin() + activeInput->cursorPosition - 1);
                activeInput->cursorPosition--;
            }
            activeInput->autoCompleteCycle = 0;
            activeInput->autoCompleteOptions.clear();
        }
        activeInput->timeSinceLastInput = 0.f;

    } else if(input == HE_KEY_ENTER) {
        activeInput->entered = true;
    } else if(input == HE_KEY_ARROW_UP) {
        // move up in history
        if((int32_t) activeInput->history.size() > activeInput->historyIndex + 1) {
            activeInput->inputString    = activeInput->history[++activeInput->historyIndex]; 
            activeInput->cursorPosition = (uint32_t) activeInput->inputString.size();
        }
        activeInput->timeSinceLastInput = 0.f;

    } else if(input == HE_KEY_ARROW_DOWN) {
        // move down in history
        if(--activeInput->historyIndex >= 0) {
            activeInput->inputString    = activeInput->history[activeInput->historyIndex];
            activeInput->cursorPosition = (uint32_t) activeInput->inputString.size();
        } else {
            activeInput->inputString.clear();
            activeInput->cursorPosition = 0;
            activeInput->historyIndex   = -1;
        }
        activeInput->timeSinceLastInput = 0.f;

    } else if(input == HE_KEY_ARROW_LEFT) {
        // move cursor left
        if(activeInput->cursorPosition > 0) {
            size_t newIndex = activeInput->cursorPosition - 1;
            if(window->keyboardInfo.keyStatus[HE_KEY_LCONTROL]) {
                newIndex = (size_t) activeInput->inputString.substr(0, activeInput->cursorPosition).find_last_of(' '); // skip to next space
                if(newIndex == std::string::npos)
                    newIndex = 0;
            }
            activeInput->cursorPosition = (uint32_t) newIndex;
        }
        activeInput->timeSinceLastInput = 0.f;

    } else if(input == HE_KEY_ARROW_RIGHT) {
        // move cursor right
        if(activeInput->cursorPosition < activeInput->inputString.size()) {
            size_t newIndex = activeInput->cursorPosition + 1;
            if(window->keyboardInfo.keyStatus[HE_KEY_LCONTROL]) {
                // skip to next space
                newIndex = activeInput->inputString.substr(activeInput->cursorPosition + 1).find(' ');
                if(newIndex == std::string::npos)
                    newIndex = activeInput->inputString.size();
                else
                    newIndex += activeInput->cursorPosition + 1; // we searched in a substring, convert to full string size 
            }
            activeInput->cursorPosition = (uint32_t) newIndex;
        }
        activeInput->timeSinceLastInput = 0.f;

    } else if(input == HE_KEY_V && window->keyboardInfo.keyStatus[HE_KEY_LCONTROL]) {
        // paste into console
        std::string clipboard = heWin32ClipboardGet();
        activeInput->inputString.insert(activeInput->cursorPosition, clipboard);
        activeInput->cursorPosition += (uint32_t) clipboard.size();
        activeInput->timeSinceLastInput = 0.f;

    } else if(input == HE_KEY_DELETE) {
        // remove rhs
        if(activeInput->cursorPosition < activeInput->inputString.size()) {
            if(window->keyboardInfo.keyStatus[HE_KEY_LCONTROL]) {
                size_t index = activeInput->inputString.substr(activeInput->cursorPosition + 1).find(' ');
                if (index == std::string::npos)
                    index = activeInput->inputString.size() - 1;
                else
                    index += activeInput->cursorPosition + 1; // we searched in a substring, convert to full string size 

                activeInput->inputString.erase(activeInput->inputString.begin() + activeInput->cursorPosition, activeInput->inputString.begin() + index + 1);
            } else {
                //activeInput->inputString.pop_back();
                activeInput->inputString.erase(activeInput->inputString.begin() + activeInput->cursorPosition);
                //activeInput->cursorPosition--;
            }
        }
        activeInput->timeSinceLastInput = 0.f;

    } else if(input == HE_KEY_TAB) {
        // try auto complete?
        if(activeInput->autoCompleteOptions.empty()) {
            // evaluate options
            for(std::string const& all : activeInput->autoCompleteList) {
                if(all.compare(0, activeInput->inputString.size(), activeInput->inputString) == 0)
                    activeInput->autoCompleteOptions.emplace_back(all);
            }
        }

        if(!activeInput->autoCompleteOptions.empty()) {
            activeInput->inputString = activeInput->autoCompleteOptions[activeInput->autoCompleteCycle++];
            activeInput->cursorPosition = (uint32_t) activeInput->inputString.size();
            if(activeInput->autoCompleteCycle == activeInput->autoCompleteOptions.size())
                activeInput->autoCompleteCycle = 0;
        }

    }

    return true;    
};

void heUiCreate() {
    heRenderEngine->window->keyboardInfo.textInputCallbacks.emplace_back(&heTextInputCallback);
	heRenderEngine->window->keyboardInfo.keyPressCallbacks.emplace_back(&heKeyPressCallback);	
};

void heUiSetActiveInput(HeTextInput* input) {
    activeInput = input;
};
