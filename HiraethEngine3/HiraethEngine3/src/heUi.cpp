#include "heUi.h"
#include "heWindow.h"
#include "heRenderer.h"
#include "heWin32Layer.h"
#include "heCore.h"

std::vector<HeTextInput*> inputs;

b8 heTextInputCallback(HeWindow* window, uint32_t const input) {
    if(inputs.size() == 0)
        return false;

    HeTextInput* activeInput = inputs[inputs.size() - 1];
    if(!activeInput->active)
        return false;
    
    if(heScaledFontHasCharacter(&activeInput->font, input)) {
        activeInput->string.insert(activeInput->string.begin() + activeInput->cursorPosition, input);
        activeInput->timeSinceLastInput = 0.f;
        activeInput->cursorPosition++;
        activeInput->autoCompleteOptions.clear();
        activeInput->autoCompleteCycle = 0;
    }

    return true;
};

b8 heKeyPressCallback(HeWindow* window, HeKeyCode const input) {
    if(inputs.size() == 0)
        return false;

    HeTextInput* activeInput = inputs[inputs.size() - 1];
    if(!activeInput->active)
        return false;
 
    if(input == HE_KEY_BACKSPACE) {
        // backspace
        if(activeInput->cursorPosition > 0) {
            if(window->keyboardInfo.keyStatus[HE_KEY_LCONTROL]) {
                size_t index = activeInput->string.substr(0, activeInput->cursorPosition).find_last_of(' ');
                if (index == std::string::npos)
                    index = 0;
                activeInput->string.erase(activeInput->string.begin() + index, activeInput->string.begin() + activeInput->cursorPosition);
                activeInput->cursorPosition = (uint32_t) index;
            } else {
                activeInput->string.erase(activeInput->string.begin() + activeInput->cursorPosition - 1);
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
            activeInput->string    = activeInput->history[++activeInput->historyIndex]; 
            activeInput->cursorPosition = (uint32_t) activeInput->string.size();
        }
        activeInput->timeSinceLastInput = 0.f;

    } else if(input == HE_KEY_ARROW_DOWN) {
        // move down in history
        if(--activeInput->historyIndex >= 0) {
            activeInput->string    = activeInput->history[activeInput->historyIndex];
            activeInput->cursorPosition = (uint32_t) activeInput->string.size();
        } else {
            activeInput->string.clear();
            activeInput->cursorPosition = 0;
            activeInput->historyIndex   = -1;
        }
        activeInput->timeSinceLastInput = 0.f;

    } else if(input == HE_KEY_ARROW_LEFT) {
        // move cursor left
        if(activeInput->cursorPosition > 0) {
            size_t newIndex = activeInput->cursorPosition - 1;
            if(window->keyboardInfo.keyStatus[HE_KEY_LCONTROL]) {
                newIndex = (size_t) activeInput->string.substr(0, activeInput->cursorPosition).find_last_of(' '); // skip to next space
                if(newIndex == std::string::npos)
                    newIndex = 0;
            }
            activeInput->cursorPosition = (uint32_t) newIndex;
        }
        activeInput->timeSinceLastInput = 0.f;

    } else if(input == HE_KEY_ARROW_RIGHT) {
        // move cursor right
        if(activeInput->cursorPosition < activeInput->string.size()) {
            size_t newIndex = activeInput->cursorPosition + 1;
            if(window->keyboardInfo.keyStatus[HE_KEY_LCONTROL]) {
                // skip to next space
                newIndex = activeInput->string.substr(activeInput->cursorPosition + 1).find(' ');
                if(newIndex == std::string::npos)
                    newIndex = activeInput->string.size();
                else
                    newIndex += activeInput->cursorPosition + 1; // we searched in a substring, convert to full string size 
            }
            activeInput->cursorPosition = (uint32_t) newIndex;
        }
        activeInput->timeSinceLastInput = 0.f;

    } else if(input == HE_KEY_V && window->keyboardInfo.keyStatus[HE_KEY_LCONTROL]) {
        // paste into console
        std::string clipboard = heWin32ClipboardGet();
        activeInput->string.insert(activeInput->cursorPosition, clipboard);
        activeInput->cursorPosition += (uint32_t) clipboard.size();
        activeInput->timeSinceLastInput = 0.f;

    } else if(input == HE_KEY_DELETE) {
        // remove rhs
        if(activeInput->cursorPosition < activeInput->string.size()) {
            if(window->keyboardInfo.keyStatus[HE_KEY_LCONTROL]) {
                size_t index = activeInput->string.substr(activeInput->cursorPosition + 1).find(' ');
                if (index == std::string::npos)
                    index = activeInput->string.size() - 1;
                else
                    index += activeInput->cursorPosition + 1; // we searched in a substring, convert to full string size 

                activeInput->string.erase(activeInput->string.begin() + activeInput->cursorPosition, activeInput->string.begin() + index + 1);
            } else {
                //activeInput->string.pop_back();
                activeInput->string.erase(activeInput->string.begin() + activeInput->cursorPosition);
                //activeInput->cursorPosition--;
            }
        }
        activeInput->timeSinceLastInput = 0.f;

    } else if(input == HE_KEY_TAB) {
        // try auto complete?
        if(activeInput->autoCompleteOptions.empty()) {
            // evaluate options
            for(std::string const& all : activeInput->autoCompleteList) {
                if(all.compare(0, activeInput->string.size(), activeInput->string) == 0)
                    activeInput->autoCompleteOptions.emplace_back(all);
            }
        }

        if(!activeInput->autoCompleteOptions.empty()) {
            activeInput->string = activeInput->autoCompleteOptions[activeInput->autoCompleteCycle++];
            activeInput->cursorPosition = (uint32_t) activeInput->string.size();
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
    if(input) {
        inputs.emplace_back(input);
        input->active = true;
        if(inputs.size() > 1)
            inputs[inputs.size() - 2]->active = false;
    } else {
        if(inputs.size()) {
            inputs.pop_back();
            if(inputs.size())
                inputs[inputs.size() - 1]->active = true;
        }
    }
};


void heUiRenderTextInput(HeRenderEngine* engine, HeTextInput* input, hm::vec2f const& position) {
    // cursor
    hm::vec2f padding = hm::vec2f(0, 3);
    hm::vec2f size    = hm::vec2f(heScaledFontGetCharacterSize(&input->font, 'M').x, input->font.lineHeight - padding.y * 2);
    hm::vec2f offset  = hm::vec2f(position.x, position.y) + padding;
    if(input->string.size() > 0) {
        uint32_t cursorPos = input->cursorPosition;
        offset.x += heScaledFontGetStringWidthInPixels(&input->font, input->string.substr(0, cursorPos)) + 4 * input->font.scale;
    }

    uint8_t alpha = 255;

    if(!input->active || (input->timeSinceLastInput - std::floor(input->timeSinceLastInput)) > 0.5f)
        alpha = 0;

    if(alpha > 0)
        heUiPushQuad(heRenderEngine, offset, hm::vec2f(offset.x, offset.y + size.y), hm::vec2f(offset.x + size.x, offset.y), offset + size, hm::colour(100, 100, 100, alpha));
    input->timeSinceLastInput += (float) engine->window->frameTime;

    // input string
    if(input->string.size() > 0)
        heUiPushText(heRenderEngine, &input->font, input->string, position, hm::colour(255, 255, 255, 255));
	else if(!input->active && input->description.size() > 0)
        heUiPushText(heRenderEngine, &input->font, input->description, position, hm::colour(190, 190, 190, 255));
};

void heUiRenderTextField(HeRenderEngine* engine, HeUiTextField* field) {
    heUiPushQuad(engine, field->shape.position, field->shape.size, field->backgroundColour);
    heUiRenderTextInput(engine, &field->input, field->shape.position + hm::vec2f(5, 3));
};

void heUiRenderPanel(HeRenderEngine* engine, HeUiPanel* panel, b8 recursive) {
    for(HeUiTextField& textFields : panel->textFields) {
        HeUiInteractionStatus status = heUiGetInteractionShapeStatus(&textFields.shape, engine->window);
        if(status == HE_UI_INTERACTION_STATUS_UNPRESSED) {
            // set as inactive
            //if(textFields.shape.status == HE_UI_INTERACTION_STATUS_CLICKED)
            textFields.input.active = false;
            textFields.shape.status = HE_UI_INTERACTION_STATUS_NONE;
        } else if(status == HE_UI_INTERACTION_STATUS_PRESSED) {
            heUiSetActiveInput(nullptr); // remove previous text field
            heUiSetActiveInput(&textFields.input);
            textFields.shape.status = HE_UI_INTERACTION_STATUS_CLICKED;
        } 

        heUiRenderTextField(engine, &textFields);
    }
};

b8 heUiPushButton(HeRenderEngine* engine, HeScaledFont* font, std::string const& label, hm::vec2f const& position, hm::vec2f const& size) {
    b8 hovered = engine->window->mouseInfo.mousePosition.x >= position.x &&
        engine->window->mouseInfo.mousePosition.x <  position.x + size.x &&
        engine->window->mouseInfo.mousePosition.y >= position.y &&
        engine->window->mouseInfo.mousePosition.y <  position.y + size.y;

    hm::colour bgColour(50);
    if(hovered)
        bgColour = hm::colour(80);

    heUiPushQuad(engine, position, size, bgColour);
    heUiPushText(engine, font, label, position, hm::colour(255));

    return hovered && engine->window->mouseInfo.leftButtonPressed;
};


HeUiInteractionStatus heUiGetInteractionShapeStatus(HeUiInteractionShape* shape, HeWindow const* window) {
    // check if mouse is over widget
    if(window->mouseInfo.mousePosition.x >= shape->position.x &&
       window->mouseInfo.mousePosition.x <  shape->position.x + shape->size.x &&
       window->mouseInfo.mousePosition.y >= shape->position.y &&
       window->mouseInfo.mousePosition.y <  shape->position.y + shape->size.y) {
        // is hovered
        return window->mouseInfo.leftButtonPressed ? HE_UI_INTERACTION_STATUS_PRESSED : HE_UI_INTERACTION_STATUS_HOVERED;
    } else if(window->mouseInfo.leftButtonPressed) {
        return HE_UI_INTERACTION_STATUS_UNPRESSED;
    } else
        return HE_UI_INTERACTION_STATUS_NONE;
};
