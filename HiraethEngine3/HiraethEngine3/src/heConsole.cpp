#include "hepch.h"
#include "heConsole.h"
#include "heRenderer.h"
#include "heWindow.h"
#include "heCore.h"
#include "heUtils.h"
#include "heWin32Layer.h"
#include "heUi.h"
#include "meta/commands.cpp"

uint32_t TYPE_BAR_HEIGHT    = 27;
uint32_t BACK_LOG_FONT_SIZE = 15;

struct HeConsoleCommand {
    std::string name;
    void(*proc)(std::vector<std::string> const& args);

    HeConsoleCommand(std::string const& name, void(*proc)(std::vector<std::string> const& args)) :
        name(name), proc(proc) {};
};

struct HeConsole {
    HeConsoleState state = HE_CONSOLE_STATE_CLOSED;
    float targetY        = 0.f; // target size of the console as window percentage
    float currentY       = 0.f; // current size of the console as window percentage
    float openSpeed      = 5.f; // the speed that the console opens up
    
    std::vector<std::string> backlog;
    std::vector<HeConsoleCommand> commands;
    int32_t backlogIndex = 0; // the index of the message that should be drawn first (lowest on screen)

    HeTextInput input;
};

HeConsole heConsole;

b8 heConsoleScrollCallback(HeWindow* window, int8_t const direction, hm::vec2i const& position) {
    int32_t verticalSize = (int32_t) (heConsole.currentY * window->windowInfo.size.y);
    if(heConsole.state != HE_CONSOLE_STATE_CLOSED && position.y < verticalSize) {
        if(direction < 0 && heConsole.backlogIndex > 0) {
            heConsole.backlogIndex += direction;
            if(heConsole.backlogIndex < 0)
                heConsole.backlogIndex = 0;
        } else if(direction > 0 && heConsole.backlogIndex < (int32_t) heConsole.backlog.size() - 1) {
            heConsole.backlogIndex += direction;
            if(heConsole.backlogIndex > (int32_t) heConsole.backlog.size())
                heConsole.backlogIndex = (int32_t) heConsole.backlog.size();
        }
        return true;
    } else
        return false;
};


void heConsoleCreate(HeFont* backlogFont) {
    heFontCreateScaled(backlogFont, &heConsole.input.font, BACK_LOG_FONT_SIZE);
    heRenderEngine->window->mouseInfo.scrollCallbacks.emplace_back(&heConsoleScrollCallback);
    heRegisterCommands();

    heConsole.input.description = "command";
    
    // set auto complete options
    heConsole.input.autoCompleteList.reserve(heConsole.commands.size());
    for(HeConsoleCommand const& all : heConsole.commands) {
        heConsole.input.autoCompleteList.emplace_back(all.name);
    }
};

void heConsoleUpdateSize(float delta) {
    if(heConsole.targetY > heConsole.currentY) {
        heConsole.currentY += heConsole.openSpeed * delta;
        if(heConsole.currentY > heConsole.targetY)
            heConsole.currentY = heConsole.targetY;
    } else if(heConsole.targetY < heConsole.currentY) {
        heConsole.currentY -= heConsole.openSpeed * delta;
        if(heConsole.currentY < heConsole.targetY)
            heConsole.currentY = heConsole.targetY;
    }
};

void heConsoleOpen(HeConsoleState const state) {
    if(state == HE_CONSOLE_STATE_CLOSED) {
        heConsole.targetY = 0.f;
        heUiSetActiveInput(nullptr);
        heConsole.input.active = false;
    } else if(state == HE_CONSOLE_STATE_OPEN) {
        heConsole.targetY = 0.3f;
        if(heConsole.state == HE_CONSOLE_STATE_CLOSED)
            heUiSetActiveInput(&heConsole.input);
    } else if(state == HE_CONSOLE_STATE_OPEN_FULL) {
        heConsole.targetY = 0.7f;
        if(heConsole.state == HE_CONSOLE_STATE_CLOSED)
            heUiSetActiveInput(&heConsole.input);
    }

    heConsole.state = state;
    heConsoleUpdateSize((float) heRenderEngine->window->frameTime);
};

void heConsoleToggleOpen(HeConsoleState const state) {
    if(heConsole.state != state)
        heConsoleOpen(state);
    else
        heConsoleOpen(HE_CONSOLE_STATE_CLOSED);
};

void heConsolePrint(std::string const& message) {
    if(message.find('\n') != std::string::npos) {
        std::vector<std::string> messages = heStringSplit(message, '\n');
        for(std::string all : messages)
            heConsole.backlog.emplace_back(all);
    } else
        heConsole.backlog.emplace_back(message);
};

void heConsoleRender(HeRenderEngine* engine) {
    heConsoleUpdateSize((float) engine->window->frameTime);

    if(heConsole.currentY == 0.f)
        return;


    // check for command sent
    if(heConsole.input.entered && heConsole.input.string.size() > 0) {
        heConsole.input.entered = false;
        heConsolePrint("> " + heConsole.input.string);
        heConsoleCommandRun(heConsole.input.string);
        heConsole.input.history.insert(heConsole.input.history.begin(), heConsole.input.string);
        heConsole.input.string.clear();
        heConsole.input.autoCompleteOptions.clear();
        heConsole.input.cursorPosition    =  0;
        heConsole.input.historyIndex      = -1;
        heConsole.input.autoCompleteCycle =  0;
        heConsole.backlogIndex            =  0;
    }

    
    HeWindow* window = engine->window;
    uint32_t sizeY = (uint32_t) (heConsole.currentY * window->windowInfo.size.y);

    { // render background
        heUiPushQuad(engine, hm::vec2i(0, 0), hm::vec2i(0, sizeY), hm::vec2i(window->windowInfo.size.x, 0), hm::vec2i(window->windowInfo.size.x, sizeY), hm::colour(40, 40, 40, 200));
        heUiPushQuad(engine, hm::vec2i(0, sizeY), hm::vec2i(0, sizeY + TYPE_BAR_HEIGHT), hm::vec2i(window->windowInfo.size.x, sizeY), hm::vec2i(window->windowInfo.size.x, sizeY + TYPE_BAR_HEIGHT), hm::colour(10, 10, 10, 255));
    }

    { // render backlog
        float textY = sizeY - heConsole.input.font.lineHeight;

        // messages
        int32_t index = (int32_t) (heConsole.backlog.size() - heConsole.backlogIndex - 1);
        uint32_t displayedCount = 0;
        while(textY > 0 && index >= 0) {
            heUiPushText(engine, &heConsole.input.font, heConsole.backlog[index], hm::vec2i(10, (int32_t) textY), hm::colour(255, 255, 255, 200));
            index--;
            textY -= heConsole.input.font.lineHeight;
            displayedCount++;
        }

        // scroll bar
        if(displayedCount < heConsole.backlog.size()) {
            // could not display all messages
            uint32_t totalHeight = sizeY - 20;
            float scrollOffset   = 1.f - heConsole.backlogIndex / (float) heConsole.backlog.size();
            float percent        = displayedCount / (float) heConsole.backlog.size();
            float height         = totalHeight * percent; // 20 is padding
            float yOffset        = totalHeight * (scrollOffset) + 10 - height;

            float width = 10;
            hm::vec2f offset(engine->window->windowInfo.size.x - 20.f, yOffset);
            
            heUiPushQuad(engine, hm::vec2f(offset.x, 10.f), hm::vec2f(offset.x, 10.f + totalHeight), hm::vec2f(offset.x + width, 10.f), hm::vec2f(offset.x + width, 10.f + totalHeight), hm::colour(50));
            heUiPushQuad(engine, offset, hm::vec2f(offset.x, offset.y + height), hm::vec2f(offset.x + width, offset.y), hm::vec2f(offset.x + width, offset.y + height), hm::colour(70));
        }
    }

    { // render input
        heUiRenderTextInput(engine, &heConsole.input, hm::vec2f(10.f, (float) sizeY));
    }
};

void heConsoleRegisterCommand(std::string const& name, void(*proc)(std::vector<std::string> const& args)) {
    heConsole.commands.emplace_back(HeConsoleCommand(name, proc));
};

void heConsoleCommandRun(std::string const& input) {
    size_t nameIndex = input.find(' ');
    std::string name = input.substr(0, nameIndex);
    std::vector<std::string> args;

    if(name.size() < input.size())
        args = heStringSplit(input.substr(nameIndex + 1), ' ');

    b8 found = false;
    for(auto const& all : heConsole.commands) {
        if(all.name == name) {
            all.proc(args);
            found = true;
            break;
        }
    }

    if(!found)
        heConsolePrint("Error: No such command [" + name + "]");
};


HeConsoleState heConsoleGetState() {
    return heConsole.state;
};
