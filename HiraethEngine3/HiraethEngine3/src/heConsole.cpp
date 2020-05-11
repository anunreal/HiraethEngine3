#include "heConsole.h"
#include "heRenderer.h"
#include "heWindow.h"
#include "heCore.h"
#include "heUtils.h"
#include "heWin32Layer.h"
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
	float targetY	= 0.f; // target size of the console as window percentage
	float currentY	= 0.f; // current size of the console as window percentage
	float openSpeed = 5.f; // the speed that the console opens up
	float timeSinceLastInput = 0.f;
	
	std::string inputString;
	uint32_t cursorPosition = 0; // in chars, from the beginning of the input string

	std::vector<std::string> history;
	int32_t historyIndex = -1; // when this is -1, we type a new command. Else this is the index of the history that we are currently seeing
	
	std::vector<std::string> backlog;
	std::vector<HeConsoleCommand> commands;
	HeScaledFont backlogFont;
	int32_t backlogIndex = 0; // the index of the message that should be drawn first (lowest on screen)

	int32_t autoCompleteCycle = 0;  // everytime we press tab, we increase the cycle (index to autoCompleteOptions)
	std::vector<std::string> autoCompleteOptions; // a list of all currently fitting commands. This gets cleared everytime we type something, because we need to update the options
};

HeConsole heConsole;

b8 heConsoleTextInputCallback(HeWindow* window, uint32_t const input) {
	if(heConsole.state != HE_CONSOLE_STATE_CLOSED) {
		if(heScaledFontHasCharacter(&heConsole.backlogFont, input)) {
			heConsole.inputString.insert(heConsole.inputString.begin() + heConsole.cursorPosition, input);
			heConsole.timeSinceLastInput = 0.f;
			heConsole.cursorPosition++;
			heConsole.autoCompleteOptions.clear();
			heConsole.autoCompleteCycle = 0;
		}
		return true;
	} else {
		return false;
	}
};

b8 heConsoleKeyPressCallback(HeWindow* window, HeKeyCode const input) {
	if(heConsole.state != HE_CONSOLE_STATE_CLOSED) {
		if(input == HE_KEY_BACKSPACE) {
			// backspace
			if(heConsole.cursorPosition > 0) {
				if(window->keyboardInfo.keyStatus[HE_KEY_LCONTROL]) {
					size_t index = heConsole.inputString.substr(0, heConsole.cursorPosition).find_last_of(' ');
					if (index == std::string::npos)
						index = 0;
					heConsole.inputString.erase(heConsole.inputString.begin() + index, heConsole.inputString.begin() + heConsole.cursorPosition);
					heConsole.cursorPosition = (uint32_t) index;
				} else {
					//heConsole.inputString.pop_back();
					heConsole.inputString.erase(heConsole.inputString.begin() + heConsole.cursorPosition - 1);
					heConsole.cursorPosition--;
				}
				heConsole.autoCompleteCycle = 0;
				heConsole.autoCompleteOptions.clear();
			}
			heConsole.timeSinceLastInput = 0.f;

		} else if(input == HE_KEY_ENTER) {
			// run command
			if(heConsole.inputString.size() > 0) {
				heConsolePrint(heConsole.inputString);
				heConsoleCommandRun(heConsole.inputString);
				heConsole.history.insert(heConsole.history.begin(), heConsole.inputString);
				heConsole.inputString.clear();
				heConsole.cursorPosition    = 0;
				heConsole.historyIndex      = -1;
				heConsole.backlogIndex      = 0;
				heConsole.autoCompleteCycle = 0;
				heConsole.autoCompleteOptions.clear();
			}

		} else if(input == HE_KEY_ARROW_UP) {
			// move up in history
			if((int32_t) heConsole.history.size() > heConsole.historyIndex + 1) {
				heConsole.inputString    = heConsole.history[++heConsole.historyIndex]; 
				heConsole.cursorPosition = (uint32_t) heConsole.inputString.size();
			}
			heConsole.timeSinceLastInput = 0.f;

		} else if(input == HE_KEY_ARROW_DOWN) {
			// move down in history
			if(--heConsole.historyIndex >= 0) {
				heConsole.inputString    = heConsole.history[heConsole.historyIndex];
				heConsole.cursorPosition = (uint32_t) heConsole.inputString.size();
			} else {
				heConsole.inputString.clear();
				heConsole.cursorPosition = 0;
				heConsole.historyIndex   = -1;
			}
			heConsole.timeSinceLastInput = 0.f;

		} else if(input == HE_KEY_ARROW_LEFT) {
			// move cursor left
			if(heConsole.cursorPosition > 0) {
				size_t newIndex = heConsole.cursorPosition - 1;
				if(window->keyboardInfo.keyStatus[HE_KEY_LCONTROL]) {
					newIndex = (size_t) heConsole.inputString.substr(0, heConsole.cursorPosition).find_last_of(' '); // skip to next space
					if(newIndex == std::string::npos)
						newIndex = 0;
				}
				heConsole.cursorPosition = (uint32_t) newIndex;
			}
			heConsole.timeSinceLastInput = 0.f;

		} else if(input == HE_KEY_ARROW_RIGHT) {
			// move cursor right
			if(heConsole.cursorPosition < heConsole.inputString.size()) {
				size_t newIndex = heConsole.cursorPosition + 1;
				if(window->keyboardInfo.keyStatus[HE_KEY_LCONTROL]) {
					// skip to next space
					newIndex = heConsole.inputString.substr(heConsole.cursorPosition + 1).find(' ');
					if(newIndex == std::string::npos)
						newIndex = heConsole.inputString.size();
					else
						newIndex += heConsole.cursorPosition + 1; // we searched in a substring, convert to full string size 
				}
				heConsole.cursorPosition = (uint32_t) newIndex;
			}
			heConsole.timeSinceLastInput = 0.f;

		} else if(input == HE_KEY_V && window->keyboardInfo.keyStatus[HE_KEY_LCONTROL]) {
			// paste into console
			std::string clipboard = heWin32ClipboardGet();
			heConsole.inputString.insert(heConsole.cursorPosition, clipboard);
			heConsole.cursorPosition += (uint32_t) clipboard.size();
			heConsole.timeSinceLastInput = 0.f;

		} else if(input == HE_KEY_DELETE) {
			// remove rhs
			if(heConsole.cursorPosition < heConsole.inputString.size()) {
				if(window->keyboardInfo.keyStatus[HE_KEY_LCONTROL]) {
					size_t index = heConsole.inputString.substr(heConsole.cursorPosition + 1).find(' ');
					if (index == std::string::npos)
						index = heConsole.inputString.size() - 1;
					else
						index += heConsole.cursorPosition + 1; // we searched in a substring, convert to full string size 

					heConsole.inputString.erase(heConsole.inputString.begin() + heConsole.cursorPosition, heConsole.inputString.begin() + index + 1);
				} else {
					//heConsole.inputString.pop_back();
					heConsole.inputString.erase(heConsole.inputString.begin() + heConsole.cursorPosition);
					//heConsole.cursorPosition--;
				}
			}
			heConsole.timeSinceLastInput = 0.f;

		} else if(input == HE_KEY_TAB) {
			// try auto complete?
			if(heConsole.autoCompleteOptions.empty()) {
				// evaluate options
				for(HeConsoleCommand const& all : heConsole.commands) {
					if(all.name.compare(0, heConsole.inputString.size(), heConsole.inputString) == 0)
						heConsole.autoCompleteOptions.emplace_back(all.name);
				}
			}

			if(!heConsole.autoCompleteOptions.empty()) {
				heConsole.inputString = heConsole.autoCompleteOptions[heConsole.autoCompleteCycle++];
				heConsole.cursorPosition = (uint32_t) heConsole.inputString.size();
				if(heConsole.autoCompleteCycle == heConsole.autoCompleteOptions.size())
					heConsole.autoCompleteCycle = 0;
			}

		}

		return true;
	} else
		return false;
};

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
	heFontCreateScaled(backlogFont, &heConsole.backlogFont, BACK_LOG_FONT_SIZE);
	heRenderEngine->window->keyboardInfo.textInputCallbacks.emplace_back(&heConsoleTextInputCallback);
	heRenderEngine->window->keyboardInfo.keyPressCallbacks.emplace_back(&heConsoleKeyPressCallback);
	heRenderEngine->window->mouseInfo.scrollCallbacks.emplace_back(&heConsoleScrollCallback);
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
	if(message.find('\n') != std::string::npos) {
		std::vector<std::string> messages = heStringSplit(message, '\n');
		for(std::string all : messages)
			heConsole.backlog.emplace_back(all);
	} else
		heConsole.backlog.emplace_back(message);
};

void heConsoleRender(float const delta) {
	heConsoleUpdateSize((float) heRenderEngine->window->frameTime);

	if(heConsole.currentY == 0.f)
		return;
	
	HeWindow* window = heRenderEngine->window;
	uint32_t sizeY = (uint32_t) (heConsole.currentY * window->windowInfo.size.y);

	{ // render background
		heUiPushQuad(heRenderEngine, hm::vec2i(0, 0), hm::vec2i(0, sizeY), hm::vec2i(window->windowInfo.size.x, 0), hm::vec2i(window->windowInfo.size.x, sizeY), hm::colour(40, 40, 40, 200));
		heUiPushQuad(heRenderEngine, hm::vec2i(0, sizeY), hm::vec2i(0, sizeY + TYPE_BAR_HEIGHT), hm::vec2i(window->windowInfo.size.x, sizeY), hm::vec2i(window->windowInfo.size.x, sizeY + TYPE_BAR_HEIGHT), hm::colour(10, 10, 10, 255));
	}

	{ // render backlog
		float textY = sizeY - heConsole.backlogFont.lineHeight;

		// messages
		int32_t index = (int32_t) (heConsole.backlog.size() - heConsole.backlogIndex - 1);
		uint32_t displayedCount = 0;
		while(textY > 0 && index >= 0) {
			heUiPushText(heRenderEngine, &heConsole.backlogFont, heConsole.backlog[index], hm::vec2i(10, (int32_t) textY), hm::colour(255, 255, 255, 200));
			index--;
			textY -= heConsole.backlogFont.lineHeight;
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
			hm::vec2f offset(heRenderEngine->window->windowInfo.size.x - 20.f, yOffset);
			
			heUiPushQuad(heRenderEngine, hm::vec2f(offset.x, 10.f), hm::vec2f(offset.x, 10.f + totalHeight), hm::vec2f(offset.x + width, 10.f), hm::vec2f(offset.x + width, 10.f + totalHeight), hm::colour(50));
			heUiPushQuad(heRenderEngine, offset, hm::vec2f(offset.x, offset.y + height), hm::vec2f(offset.x + width, offset.y), hm::vec2f(offset.x + width, offset.y + height), hm::colour(70));
		}
	}

	{ // render input
		// cursor
		hm::vec2f padding = hm::vec2f(0, 3);
		hm::vec2f size    = hm::vec2f(heScaledFontGetCharacterSize(&heConsole.backlogFont, 'M').x, heConsole.backlogFont.lineHeight - padding.y * 2);
		hm::vec2f offset  = hm::vec2f(10, (float) sizeY) + padding;
		if(heConsole.inputString.size() > 0) {
			uint32_t cursorPos = heConsole.cursorPosition;
			offset.x += heScaledFontGetStringWidthInPixels(&heConsole.backlogFont, heConsole.inputString.substr(0, cursorPos)) + 4 * heConsole.backlogFont.scale;
		}

		uint8_t alpha = 255;

		if((heConsole.timeSinceLastInput - std::floor(heConsole.timeSinceLastInput)) > 0.5f)
			alpha = 0;
		
		heUiPushQuad(heRenderEngine, offset, hm::vec2f(offset.x, offset.y + size.y), hm::vec2f(offset.x + size.x, offset.y), offset + size, hm::colour(100, 100, 100, alpha));
		heConsole.timeSinceLastInput += delta;

		// input string
		if(heConsole.inputString.size() > 0)
			heUiPushText(heRenderEngine, &heConsole.backlogFont, heConsole.inputString, hm::vec2i(10, (int32_t) sizeY), hm::colour(255, 255, 255, 255));	
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
