#include "heConsole.h"
#include "heRenderer.h"
#include "heWindow.h"
#include "heCore.h"
#include "heUtils.h"
#include "meta/commands.cpp"

#define TYPE_BAR_HEIGHT    25
#define BACK_LOG_FONT_SIZE 15

struct HeConsoleCommand {
	std::string name;
	void(*proc)(std::vector<std::string> const& args);

	HeConsoleCommand(std::string const& name, void(*proc)(std::vector<std::string> const& args)) :
		name(name), proc(proc) {};
};

struct HeConsole {
	HeConsoleState state = HE_CONSOLE_STATE_CLOSED;
	float targetY   = 0.f; // target size of the console as window percentage
	float currentY  = 0.f; // current size of the console as window percentage
	float openSpeed = 5.f; // the speed that the console opens up
	float timeSinceLastInput = 0.f;
	
	std::string inputString;
	
	std::vector<std::string> backlog;
	std::vector<HeConsoleCommand> commands;
	HeFont* backlogFont;
};

HeConsole heConsole;


void heConsoleCommandRun(std::string const& input) {
	size_t nameIndex = input.find(' ');
	std::string name = input.substr(0, nameIndex);
	std::vector<std::string> args;

	// rtrim?

	if(name.size() < input.size())
		args = heStringSplit(input.substr(nameIndex + 1), ' ');
	
	for(auto const& all : heConsole.commands) {
		if(all.name == name) {
			all.proc(args);
			break;
		}
	}
};



bool heConsoleTextInputCallback(HeWindow* window, uint32_t const input) {
	if(heConsole.state != HE_CONSOLE_STATE_CLOSED) {
		if(input == 8) { // backspace
			if(heConsole.inputString.size() > 0)
                heConsole.inputString.pop_back();
		} else if(input == 127) { // control backspace
			if(heConsole.inputString.size() > 0) {
				size_t index = heConsole.inputString.find_last_of(' ');
				if (index == std::string::npos)
					index = 0;
				heConsole.inputString = heConsole.inputString.substr(0, index);
	    	}    		
		} else if(input == 13) { // enter
			if(heConsole.inputString.size() > 0) {
			    heConsolePrint(heConsole.inputString);
				//heCommandRun(heConsole.inputString);
				heConsoleCommandRun(heConsole.inputString);
			    heConsole.inputString.clear();
			}
		} else if(heFontHasCharacter(heConsole.backlogFont, input))
		    heConsole.inputString.push_back(((char) input));

		heConsole.timeSinceLastInput = 0.f;
		return true;
	} else {
		return false;
	}
};

void heConsoleCreate(HeFont* backlogFont) {
	heConsole.backlogFont = backlogFont;
	heRenderEngine->window->keyboardInfo.textInputCallbacks.emplace_back(&heConsoleTextInputCallback);
	//heConsoleRegisterCommand("hello", {}, front_command_hello);
	//heConsoleRegisterCommand("add", {}, front_command_add);
    heRegisterCommands();
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
	heConsole.state = state;

	if(state == HE_CONSOLE_STATE_CLOSED)
		heConsole.targetY = 0.f;
	else if(state == HE_CONSOLE_STATE_OPEN)
		heConsole.targetY = 0.3f;
	else if(state == HE_CONSOLE_STATE_OPEN_FULL)
		heConsole.targetY = 0.7f;

	heConsoleUpdateSize((float) heRenderEngine->window->frameTime);
};

void heConsoleToggleOpen(HeConsoleState const state) {
	if(heConsole.state != state)
		heConsoleOpen(state);
	else
		heConsoleOpen(HE_CONSOLE_STATE_CLOSED);
};

void heConsolePrint(std::string const& message) {
	heConsole.backlog.emplace_back(message);
};

void heConsoleRender(float const delta) {
	heConsoleUpdateSize((float) heRenderEngine->window->frameTime);

	if(heConsole.currentY == 0.f)
		return;
	
	HeWindow* window = heRenderEngine->window;

	uint32_t sizeY = (uint32_t) (heConsole.currentY * window->windowInfo.size.y);

	{ // render background
		heUiRenderQuad(heRenderEngine, hm::vec2i(0, 0), hm::vec2i(0, sizeY), hm::vec2i(window->windowInfo.size.x, 0), hm::vec2i(window->windowInfo.size.x, sizeY), hm::colour(40, 40, 40, 200));
		heUiRenderQuad(heRenderEngine, hm::vec2i(0, sizeY), hm::vec2i(0, sizeY + TYPE_BAR_HEIGHT), hm::vec2i(window->windowInfo.size.x, sizeY), hm::vec2i(window->windowInfo.size.x, sizeY + TYPE_BAR_HEIGHT), hm::colour(10, 10, 10, 255));
	}

	float textScale = BACK_LOG_FONT_SIZE / (float) heConsole.backlogFont->size;

	{ // render backlog
		float textY = sizeY - (heConsole.backlogFont->lineHeight * textScale);

		int32_t index = (int32_t) (heConsole.backlog.size() - 1);
		while(textY > 0 && index >= 0) {
			heUiPushText(heRenderEngine, heConsole.backlogFont, heConsole.backlog[index], hm::vec2i(10, (int32_t) textY), BACK_LOG_FONT_SIZE, hm::colour(255, 255, 255, 200));
			index--;
			textY -= (heConsole.backlogFont->lineHeight * textScale);
		}
	}

	{ // render input
        heUiPushText(heRenderEngine, heConsole.backlogFont, heConsole.inputString, hm::vec2i(10, (int32_t) sizeY), BACK_LOG_FONT_SIZE, hm::colour(255, 255, 255, 255));	
        
        // cursor
		hm::vec2i size = heFontGetCharacterSize(heConsole.backlogFont, 'M', BACK_LOG_FONT_SIZE);
		hm::vec2i padding = hm::vec2i(10, heConsole.backlogFont->lineHeight / 2);
		padding = hm::vec2i(3, 3);
		hm::vec2i offset  = hm::vec2i(10 + heFontGetStringWidthInPixels(heConsole.backlogFont, heConsole.inputString, BACK_LOG_FONT_SIZE), (int32_t) sizeY) + padding;
		uint8_t alpha = 255;

		if((heConsole.timeSinceLastInput - std::floor(heConsole.timeSinceLastInput)) > 0.5f)
		   alpha = 0;
		
		heUiRenderQuad(heRenderEngine, offset, hm::vec2i(offset.x, offset.y + size.y), hm::vec2i(offset.x + size.x, offset.y), offset + size, hm::colour(255, 255, 255, alpha));
        heConsole.timeSinceLastInput += delta;
	}
};

void heConsoleRegisterCommand(std::string const& name, void(*proc)(std::vector<std::string> const& args)) {
	heConsole.commands.emplace_back(HeConsoleCommand(name, proc));
};


HeConsoleState heConsoleGetState() {
	return heConsole.state;
};
