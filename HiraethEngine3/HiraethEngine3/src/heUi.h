#ifndef HE_UI_H
#define HE_UI_H

#include "heTypes.h"
#include "heAssets.h"
#include "hm/hm.hpp"

struct HeRenderEngine; // avoid include
struct HeWindow;

struct HeTextInput {
    b8           active         = false; // whether the user should be able to type into this input right now
    b8           entered        = false; // gets set to true if this text input is active and the enter key was pressed
    uint32_t     cursorPosition = 0; // in chars, as offset
    std::string  string;
    std::string  description; // if this is set and the input string is empty, this string will be displayed in a grayish tone. This can describe what should go in that text input
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

struct HeUiInteractionShape {
    hm::vec2f size;
    hm::vec2f position;
    b8 isScreenSpace = true; // if this is true, size and position are in screen space (pixels). If this is false, the coordinates are percentages of the window (0 to 1)
    HeUiInteractionStatus status = HE_UI_INTERACTION_STATUS_NONE;
};

struct HeUiTextField {
    hm::colour backgroundColour;
    HeUiInteractionShape shape;
    HeTextInput input;
};

struct HeUiPanel {
    std::vector<HeUiTextField> textFields;
};

// sets up the ui system. This will hook up the input callbacks
extern HE_API void heUiCreate(HeRenderEngine* engine);
// sets the active text input. Only one text input can be typed in at a time. If no text input should be active,
// pass a nullptr as input
extern HE_API void heUiSetActiveInput(HeTextInput* input);

extern HE_API void heUiRenderTextInput(HeRenderEngine* engine, HeTextInput* input, hm::vec2f const& position);
// renders a text field as quad and its input string with cursor
extern HE_API void heUiRenderTextField(HeRenderEngine* engine, HeUiTextField* field);
// updates and renders all gui components of that panel. If recursive is true, this will also update and render all
// child panels and their child panels and so on
extern HE_API void heUiRenderPanel(HeRenderEngine* engine, HeUiPanel* panel, b8 recursive = true);

// renders a button at given position with given size.
extern HE_API b8 heUiPushButton(HeRenderEngine* engine, HeScaledFont* font, std::string const& label, hm::vec2f const& position, hm::vec2f const& size);

// figures out the current status of that shape by checking the mouse input of the window
extern HE_API inline HeUiInteractionStatus heUiGetInteractionShapeStatus(HeUiInteractionShape* shape, HeWindow const* window);

#endif // HE_UI_H
