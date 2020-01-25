#pragma once
#ifndef ES_CORE_WINDOW_H
#define ES_CORE_WInDOW_H

#include "HelpPrompt.h"
#include "InputConfig.h"
#include "Settings.h"
#include "math/Vector2f.h"
#include <memory>
#include <functional>

class FileData;
class Font;
class GuiComponent;
class HelpComponent;
class ImageComponent;
class InputConfig;
class TextCache;
class Transform4x4f;
struct HelpStyle;
class TextureResource;
class GuiInfoPopup;
class AsyncNotificationComponent;
class ThemeData;
class TextComponent;
class ControllerActivityComponent;
class VolumeInfoComponent;

class Window
{
public:
	class ScreenSaver {
	public:
		virtual void startScreenSaver() = 0;
		virtual void stopScreenSaver() = 0;
		virtual void nextVideo() = 0;
		virtual void renderScreenSaver() = 0;
		virtual bool allowSleep() = 0;
		virtual void update(int deltaTime) = 0;
		virtual bool isScreenSaverActive() = 0;
		virtual FileData* getCurrentGame() = 0;
		virtual void launchGame() = 0;
		virtual void resetCounts() = 0;
	};

	Window();
	~Window();

	void pushGui(GuiComponent* gui);
	void displayNotificationMessage(std::string message, int duration = -1); // batocera
	void removeGui(GuiComponent* gui);
	GuiComponent* peekGui();
	inline int getGuiStackSize() { return (int)mGuiStack.size(); }

	void textInput(const char* text);
	void input(InputConfig* config, Input input);
	void update(int deltaTime);
	void render();

	bool init();
	void deinit();

	void normalizeNextUpdate();

	inline bool isSleeping() const { return mSleeping; }
	bool getAllowSleep();
	void setAllowSleep(bool sleep);

	void renderLoadingScreen(std::string text, float percent = -1, unsigned char opacity = 255);
	void endRenderLoadingScreen();

	void renderHelpPromptsEarly(); // used to render HelpPrompts before a fade
	void setHelpPrompts(const std::vector<HelpPrompt>& prompts, const HelpStyle& style);

	void setScreenSaver(ScreenSaver* screenSaver) { mScreenSaver = screenSaver; }

	void stopInfoPopup();

	void startScreenSaver();
	bool cancelScreenSaver();
	void renderScreenSaver();

	void registerNotificationComponent(AsyncNotificationComponent* pc);
	void unRegisterNotificationComponent(AsyncNotificationComponent* pc);

	void postToUiThread(const std::function<void(Window*)>& func);
	void reactivateGui();

	void onThemeChanged(const std::shared_ptr<ThemeData>& theme);

private:
	void processPostedFunctions();

	void renderRegisteredNotificationComponents(const Transform4x4f& trans);
	std::vector<AsyncNotificationComponent*> mAsyncNotificationComponent;


	std::vector<std::function<void(Window*)>> mFunctions;


	void processNotificationMessages();
	void processSongTitleNotifications();

	void onSleep();
	void onWake();

	// Returns true if at least one component on the stack is processing
	bool isProcessing();

	HelpComponent*	mHelp;
	ImageComponent* mBackgroundOverlay;
	ScreenSaver*	mScreenSaver;
	GuiInfoPopup*	mInfoPopup;
	bool			mRenderScreenSaver;
	
	std::vector<GuiComponent*> mScreenExtras;
	std::vector<GuiComponent*> mGuiStack;

	typedef std::pair<std::string, int> NotificationMessage;
	std::vector<NotificationMessage> mNotificationMessages; // batocera

	std::vector< std::shared_ptr<Font> > mDefaultFonts;
	std::shared_ptr<TextureResource> mSplash;

	int mFrameTimeElapsed;
	int mFrameCountElapsed;
	int mAverageDeltaTime;

	std::unique_ptr<TextCache> mFrameDataText;

	int mClockElapsed;
	std::shared_ptr<TextComponent>	mClock;
	std::shared_ptr<ControllerActivityComponent>	mControllerActivity;
	std::shared_ptr<VolumeInfoComponent> mVolumeInfo;

	bool mNormalizeNextUpdate;

	bool mAllowSleep;
	bool mSleeping;
	unsigned int mTimeSinceLastInput;

	bool mRenderedHelpPrompts;

	GuiComponent* mTransiting;
	int mTransitionOffset;
};

#endif // ES_CORE_WINDOW_H
