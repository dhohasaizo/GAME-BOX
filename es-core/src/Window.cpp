#include "Window.h"

#include "components/HelpComponent.h"
#include "components/ImageComponent.h"
#include "components/TextComponent.h"
#include "resources/Font.h"
#include "resources/TextureResource.h"
#include "InputManager.h"
#include "Log.h"
#include "Scripting.h"
#include <algorithm>
#include <iomanip>
#include "guis/GuiInfoPopup.h"
#include "SystemConf.h"
#include "LocaleES.h"
#include "AudioManager.h"
#include <SDL_events.h>
#include "ThemeData.h"
#include <mutex>
#include "components/AsyncNotificationComponent.h"
#include "components/ControllerActivityComponent.h"
#include "guis/GuiMsgBox.h"
#include "components/VolumeInfoComponent.h"
#ifdef _ENABLEEMUELEC
#include "utils/FileSystemUtil.h"
#endif

Window::Window() : mNormalizeNextUpdate(false), mFrameTimeElapsed(0), mFrameCountElapsed(0), mAverageDeltaTime(10),
  mAllowSleep(true), mSleeping(false), mTimeSinceLastInput(0), mScreenSaver(NULL), mRenderScreenSaver(false), mInfoPopup(NULL), mClockElapsed(0) // batocera
{	
	mTransiting = nullptr;
	mTransitionOffset = 0;

	mHelp = new HelpComponent(this);
	mBackgroundOverlay = new ImageComponent(this);
	mBackgroundOverlay->setImage(":/scroll_gradient.png"); // batocera

	mSplash = nullptr;
}

Window::~Window()
{
	for (auto extra : mScreenExtras)
		delete extra;

	delete mBackgroundOverlay;

	// delete all our GUIs
	while(peekGui())
		delete peekGui();

	delete mHelp;
}
/*
#include "animations/LambdaAnimation.h"
#include "animations/AnimationController.h"
#include <SDL_main.h>
#include <SDL_timer.h>
*/
void Window::pushGui(GuiComponent* gui)
{
	mTransiting = nullptr;

	if (mGuiStack.size() > 0)
	{
		auto& top = mGuiStack.back();
		top->topWindow(false);
		/*
		if (top->getValue() != "GuiMsgBox" && mGuiStack.size() >= 2)
		{	
			GuiComponent* showing = gui;
			GuiComponent* hiding = top;

			mTransiting = hiding;
			mTransitionOffset = 0;

			showing->setOpacity(0);
			mGuiStack.push_back(showing);

			int duration = 250;
			int lastTime = SDL_GetTicks();
			int curTime = lastTime;
			int deltaTime = 0.00001;

			AnimationController animController(new LambdaAnimation([this, showing](float t)
			{
				float value = Math::lerp(0.0f, 1.0f, t);
				mTransitionOffset = Renderer::getScreenWidth() * value;
				mTransiting->setOpacity(255 - (255 * value));
				showing->setOpacity(255 * value);
			}, duration));
		
			do
			{
				curTime = SDL_GetTicks();
				deltaTime = curTime - lastTime;
				lastTime = curTime;

				this->update(deltaTime);
				this->render();

				Renderer::swapBuffers();
			} 
			while (!animController.update(deltaTime));

			showing->setOpacity(255);
			mTransiting->setOpacity(255);
			mTransitionOffset = 0;
			mTransiting = nullptr;

			gui->updateHelpPrompts();
			return;
		}*/
	}

	mGuiStack.push_back(gui);
	gui->updateHelpPrompts();
}

void Window::removeGui(GuiComponent* gui)
{
	for(auto i = mGuiStack.cbegin(); i != mGuiStack.cend(); i++)
	{
		if(*i == gui)
		{						
			i = mGuiStack.erase(i);

			if(i == mGuiStack.cend() && mGuiStack.size()) // we just popped the stack and the stack is not empty
			{
				mGuiStack.back()->updateHelpPrompts();
				mGuiStack.back()->topWindow(true);
			}

			return;
		}
	}
}

GuiComponent* Window::peekGui()
{
	if(mGuiStack.size() == 0)
		return NULL;

	return mGuiStack.back();
}

bool Window::init()
{
	if(!Renderer::init())
	{
		LOG(LogError) << "Renderer failed to initialize!";
		return false;
	}

	InputManager::getInstance()->init();

	ResourceManager::getInstance()->reloadAll();

	//keep a reference to the default fonts, so they don't keep getting destroyed/recreated
	if(mDefaultFonts.empty())
	{
		mDefaultFonts.push_back(Font::get(FONT_SIZE_SMALL));
		mDefaultFonts.push_back(Font::get(FONT_SIZE_MEDIUM));
		mDefaultFonts.push_back(Font::get(FONT_SIZE_LARGE));
	}

	mBackgroundOverlay->setImage(":/scroll_gradient.png");
	mBackgroundOverlay->setResize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	if (mClock == nullptr)
	{
		mClock = std::make_shared<TextComponent>(this);
		mClock->setFont(Font::get(FONT_SIZE_SMALL));
		mClock->setHorizontalAlignment(ALIGN_RIGHT);
		mClock->setVerticalAlignment(ALIGN_TOP);
		mClock->setPosition(Renderer::getScreenWidth()*0.94, Renderer::getScreenHeight()*0.9965 - Font::get(FONT_SIZE_SMALL)->getHeight());
		mClock->setSize(Renderer::getScreenWidth()*0.05, 0);
		mClock->setColor(0x777777FF);
	}

	if (mControllerActivity == nullptr)
		mControllerActivity = std::make_shared<ControllerActivityComponent>(this);

	if (mVolumeInfo == nullptr)
		mVolumeInfo = std::make_shared<VolumeInfoComponent>(this);

	// update our help because font sizes probably changed
	if (peekGui())
#ifdef _ENABLEEMUELEC	
		// emuelec
      if(Utils::FileSystem::exists("/emuelec/bin/fbfix")) {
      system("/emuelec/bin/fbfix");      
  } else { 
	  if(Utils::FileSystem::exists("/storage/.kodi/addons/script.emuelec.Amlogic-ng.launcher/bin/fbfix")) {
	   system("/storage/.kodi/addons/script.emuelec.Amlogic-ng.launcher/bin/fbfix");
	  }
  }
#endif
	peekGui()->updateHelpPrompts();
	return true;
}

void Window::reactivateGui()
{
	for (auto extra : mScreenExtras)
		extra->onShow();

	for (auto i = mGuiStack.cbegin(); i != mGuiStack.cend(); i++)
		(*i)->onShow();

	if (peekGui())
		peekGui()->updateHelpPrompts();
}

void Window::deinit()
{
	for (auto extra : mScreenExtras)
		extra->onHide();

	// Hide all GUI elements on uninitialisation - this disable
	for(auto i = mGuiStack.cbegin(); i != mGuiStack.cend(); i++)
		(*i)->onHide();

	InputManager::getInstance()->deinit();
	TextureResource::clearQueue();
	ResourceManager::getInstance()->unloadAll();
	Renderer::deinit();
}

void Window::textInput(const char* text)
{
	if(peekGui())
		peekGui()->textInput(text);
}

void Window::input(InputConfig* config, Input input)
{
	if (mScreenSaver) {
		if (mScreenSaver->isScreenSaverActive() && Settings::getInstance()->getBool("ScreenSaverControls") &&
			((Settings::getInstance()->getString("ScreenSaverBehavior") == "slideshow") || 			
			(Settings::getInstance()->getString("ScreenSaverBehavior") == "random video")))
		{
			if (mScreenSaver->getCurrentGame() != nullptr && (config->isMappedLike("right", input) || config->isMappedTo("start", input) || config->isMappedTo("select", input)))
			{
				if (config->isMappedLike("right", input) || config->isMappedTo("select", input))
				{
					if (input.value != 0) // handle screensaver control
						mScreenSaver->nextVideo();
					
					return;
				}
				else if (config->isMappedTo("start", input) && input.value != 0)
				{
					// launch game!
					cancelScreenSaver();
					mScreenSaver->launchGame();
					// to force handling the wake up process
					mSleeping = true;
				}
			}
		}
	}

	if (mSleeping)
	{
		// wake up
		mTimeSinceLastInput = 0;
		cancelScreenSaver();
		mSleeping = false;
		onWake();
		return;
	}

	mTimeSinceLastInput = 0;
	if (cancelScreenSaver())
		return;

	if (config->getDeviceId() == DEVICE_KEYBOARD && input.value && input.id == SDLK_g && SDL_GetModState() & KMOD_LCTRL) // && Settings::getInstance()->getBool("Debug"))
	{
		// toggle debug grid with Ctrl-G
		Settings::getInstance()->setBool("DebugGrid", !Settings::getInstance()->getBool("DebugGrid"));
	}
	else if (config->getDeviceId() == DEVICE_KEYBOARD && input.value && input.id == SDLK_t && SDL_GetModState() & KMOD_LCTRL) // && Settings::getInstance()->getBool("Debug"))
	{
		// toggle TextComponent debug view with Ctrl-T
		Settings::getInstance()->setBool("DebugText", !Settings::getInstance()->getBool("DebugText"));
	}
	else if (config->getDeviceId() == DEVICE_KEYBOARD && input.value && input.id == SDLK_i && SDL_GetModState() & KMOD_LCTRL) // && Settings::getInstance()->getBool("Debug"))
	{
		// toggle TextComponent debug view with Ctrl-I
		Settings::getInstance()->setBool("DebugImage", !Settings::getInstance()->getBool("DebugImage"));
	}
	else
	{
		if (mControllerActivity != nullptr)
			mControllerActivity->input(config, input);

		if (peekGui())
			peekGui()->input(config, input); // this is where the majority of inputs will be consumed: the GuiComponent Stack
	}
}

// batocera Notification messages
static std::mutex mNotificationMessagesLock;

void Window::displayNotificationMessage(std::string message, int duration)
{
	std::unique_lock<std::mutex> lock(mNotificationMessagesLock);

	if (duration <= 0)
	{
		duration = Settings::getInstance()->getInt("audio.display_titles_time");
		if (duration <= 2 || duration > 120)
			duration = 10;

		duration *= 1000;
	}

	NotificationMessage msg;
	msg.first = message;
	msg.second = duration;
	mNotificationMessages.push_back(msg);
}

void Window::stopInfoPopup() 
{
	if (mInfoPopup == nullptr)
		return;
	
	delete mInfoPopup; 
	mInfoPopup = nullptr;
}

void Window::processNotificationMessages()
{
	std::unique_lock<std::mutex> lock(mNotificationMessagesLock);

	if (mNotificationMessages.empty())
		return;
	
	NotificationMessage msg = mNotificationMessages.back();
	mNotificationMessages.pop_back();

	LOG(LogDebug) << "Notification message :" << msg.first.c_str();

	if (mInfoPopup) 
		delete mInfoPopup; 
	
	mInfoPopup = new GuiInfoPopup(this, msg.first, msg.second);
}

void Window::processSongTitleNotifications()
{
	if (!Settings::getInstance()->getBool("audio.display_titles"))
		return;

	std::string songName = AudioManager::getInstance()->getSongName();
	if (!songName.empty())
	{
		displayNotificationMessage(_U("\uF028  ") + songName); // _("Now playing: ") + 
		AudioManager::getInstance()->setSongName("");
	}	
}

void Window::update(int deltaTime)
{
	processPostedFunctions();
	processSongTitleNotifications();
	processNotificationMessages();

	if (mNormalizeNextUpdate)
	{
		mNormalizeNextUpdate = false;
		if (deltaTime > mAverageDeltaTime)
			deltaTime = mAverageDeltaTime;
	}

	if (mVolumeInfo)
		mVolumeInfo->update(deltaTime);

	mFrameTimeElapsed += deltaTime;
	mFrameCountElapsed++;
	if (mFrameTimeElapsed > 500)
	{
		mAverageDeltaTime = mFrameTimeElapsed / mFrameCountElapsed;

		if (Settings::getInstance()->getBool("DrawFramerate"))
		{
			std::stringstream ss;

			// fps
			ss << std::fixed << std::setprecision(1) << (1000.0f * (float)mFrameCountElapsed / (float)mFrameTimeElapsed) << "fps, ";
			ss << std::fixed << std::setprecision(2) << ((float)mFrameTimeElapsed / (float)mFrameCountElapsed) << "ms";

			// vram
			float textureVramUsageMb = TextureResource::getTotalMemUsage() / 1000.0f / 1000.0f;
			float textureTotalUsageMb = TextureResource::getTotalTextureSize() / 1000.0f / 1000.0f;
			float fontVramUsageMb = Font::getTotalMemUsage() / 1000.0f / 1000.0f;

			ss << "\nFont VRAM: " << fontVramUsageMb << " Tex VRAM: " << textureVramUsageMb <<
				" Tex Max: " << textureTotalUsageMb;
			mFrameDataText = std::unique_ptr<TextCache>(mDefaultFonts.at(1)->buildTextCache(ss.str(), 50.f, 50.f, 0xFF00FFFF));
		}

		mFrameTimeElapsed = 0;
		mFrameCountElapsed = 0;
	}

	/* draw the clock */ // batocera
	if (Settings::getInstance()->getBool("DrawClock") && mClock) 
	{
		mClockElapsed -= deltaTime;
		if (mClockElapsed <= 0)
		{
			time_t     clockNow = time(0);
			struct tm  clockTstruct = *localtime(&clockNow);

			if (clockTstruct.tm_year > 100) 
			{ 
				// Display the clock only if year is more than 1900+100 ; rpi have no internal clock and out of the networks, the date time information has no value */
				// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime for more information about date/time format
				
				char       clockBuf[32];
				strftime(clockBuf, sizeof(clockBuf), "%H:%M", &clockTstruct);
				mClock->setText(clockBuf);
			}

			mClockElapsed = 1000; // next update in 1000ms
		}
	}

	mTimeSinceLastInput += deltaTime;

	if (mTransiting != nullptr)
		mTransiting->update(deltaTime);

	if (peekGui())
		peekGui()->update(deltaTime);

	// Update the screensaver
	if (mScreenSaver)
		mScreenSaver->update(deltaTime);

	// update pads // batocera
	if (mControllerActivity)
		mControllerActivity->update(deltaTime);
}

void Window::render()
{
	Transform4x4f transform = Transform4x4f::Identity();

	mRenderedHelpPrompts = false;

	// draw only bottom and top of GuiStack (if they are different)
	if(mGuiStack.size())
	{
		auto& bottom = mGuiStack.front();
		auto& top = mGuiStack.back();

		bottom->render(transform);
		if(bottom != top)
		{
			if (mTransiting == nullptr && top->isKindOf<GuiMsgBox>() && mGuiStack.size() > 2)
			{
				auto& middle = mGuiStack.at(mGuiStack.size()-2);
				if (middle != bottom)
					middle->render(transform);
			}

			mBackgroundOverlay->render(transform);

			Transform4x4f topTransform = transform;

			if (mTransiting != nullptr)
			{
				Vector3f target(mTransitionOffset, 0, 0);

				Transform4x4f cam = Transform4x4f::Identity();
				cam.translation() = -target;

				mTransiting->render(cam);

				target = Vector3f(Renderer::getScreenWidth() - mTransitionOffset, 0, 0);
				topTransform.translation() = target;
			}

			top->render(topTransform);
		}
	}
	
	if (mGuiStack.size() < 2 || !Renderer::isSmallScreen())
		if(!mRenderedHelpPrompts)
			mHelp->render(transform);

	if(Settings::getInstance()->getBool("DrawFramerate") && mFrameDataText)
	{
		Renderer::setMatrix(Transform4x4f::Identity());
		mDefaultFonts.at(1)->renderTextCache(mFrameDataText.get());
	}

        // clock // batocera
	if (Settings::getInstance()->getBool("DrawClock") && mClock && (mGuiStack.size() < 2 || !Renderer::isSmallScreen()))
		mClock->render(transform);
	
	if (Settings::getInstance()->getBool("ShowControllerActivity") && mControllerActivity != nullptr && (mGuiStack.size() < 2 || !Renderer::isSmallScreen()))
		mControllerActivity->render(transform);

	Renderer::setMatrix(Transform4x4f::Identity());

	unsigned int screensaverTime = (unsigned int)Settings::getInstance()->getInt("ScreenSaverTime");
	if(mTimeSinceLastInput >= screensaverTime && screensaverTime != 0)
		startScreenSaver();

	if(!mRenderScreenSaver && mInfoPopup)
		mInfoPopup->render(transform);

	renderRegisteredNotificationComponents(transform);
	
	// Always call the screensaver render function regardless of whether the screensaver is active
	// or not because it may perform a fade on transition
	renderScreenSaver();

	for (auto extra : mScreenExtras)
		extra->render(transform);

	if (mVolumeInfo)
		mVolumeInfo->render(transform);

	if(mTimeSinceLastInput >= screensaverTime && screensaverTime != 0)
	{
		if (!isProcessing() && mAllowSleep && (!mScreenSaver || mScreenSaver->allowSleep()))
		{
			// go to sleep
			if (mSleeping == false) {
				mSleeping = true;
				onSleep();
			}
		}
	}
}

void Window::normalizeNextUpdate()
{
	mNormalizeNextUpdate = true;
}

bool Window::getAllowSleep()
{
	return mAllowSleep;
}

void Window::setAllowSleep(bool sleep)
{
	mAllowSleep = sleep;
}

void Window::endRenderLoadingScreen()
{
	mSplash = nullptr;	
}

void Window::renderLoadingScreen(std::string text, float percent, unsigned char opacity)
{
	if (mSplash == NULL)
#ifdef _ENABLEEMUELEC
		mSplash = TextureResource::get(":/splash_emuelec.svg", false, true, true, false, false);
#else
		mSplash = TextureResource::get(":/splash_batocera.svg", false, true, true, false, false);
#endif

	Transform4x4f trans = Transform4x4f::Identity();
	Renderer::setMatrix(trans);
#ifdef _ENABLEEMUELEC
	Renderer::drawRect(0, 0, Renderer::getScreenWidth(), Renderer::getScreenHeight(), 0xFFFFFF00 | opacity, 0x84848500 | opacity, true); //emuelec gradient
#else
	Renderer::drawRect(0, 0, Renderer::getScreenWidth(), Renderer::getScreenHeight(), 0xFFFFFF00 | opacity, 0xEFEFEF00 | opacity, true); // batocera
#endif

	if (percent >= 0)
	{
		float baseHeight = 0.04f;

		float w = Renderer::getScreenWidth() / 2.0f;
		float h = Renderer::getScreenHeight() * baseHeight;

		float x = Renderer::getScreenWidth() / 2.0f - w / 2.0f;
		float y = Renderer::getScreenHeight() - (Renderer::getScreenHeight() * 3 * baseHeight);

		Renderer::drawRect(x, y, w, h, 0xA0A0A000 | opacity);
#ifdef _ENABLEEMUELEC
		Renderer::drawRect(x, y, (w*percent), h, 0xA8A2D000 | opacity, 0x51468700 | opacity, true); // loading bar
#else
		Renderer::drawRect(x, y, (w*percent), h, 0xDF101000 | opacity, 0xAF000000 | opacity, true);
#endif
	}

	ImageComponent splash(this, true);
	splash.setResize(Renderer::getScreenWidth() * 0.51f, 0.0f);

	if (mSplash != NULL)
		splash.setImage(mSplash);
	else
#ifdef _ENABLEEMUELEC	
		splash.setImage(":/splash_emuelec.svg"); // emuelec
#else
		splash.setImage(":/splash_batocera.svg"); // batocera
#endif

	splash.setPosition((Renderer::getScreenWidth() - splash.getSize().x()) / 2, (Renderer::getScreenHeight() - splash.getSize().y()) / 2 * 0.6f);
	splash.render(trans);

	auto& font = mDefaultFonts.at(1);
#ifdef _ENABLEEMUELEC		
	TextCache* cache = font->buildTextCache(text, 0, 0, 0x51468700 | opacity); // text color
#else
	TextCache* cache = font->buildTextCache(text, 0, 0, 0x65656500 | opacity);
#endif

	float x = Math::round((Renderer::getScreenWidth() - cache->metrics.size.x()) / 2.0f);
	float y = Math::round(Renderer::getScreenHeight() * 0.78f);// * 0.835f
	trans = trans.translate(Vector3f(x, y, 0.0f));
	Renderer::setMatrix(trans);
	font->renderTextCache(cache);
	delete cache;

	Renderer::swapBuffers();

#if defined(_WIN32)
	// Avoid Window Freezing on Windows
	SDL_Event event;
	while (SDL_PollEvent(&event));
#endif
}

void Window::renderHelpPromptsEarly()
{
	mHelp->render(Transform4x4f::Identity());
	mRenderedHelpPrompts = true;
}

void Window::setHelpPrompts(const std::vector<HelpPrompt>& prompts, const HelpStyle& style)
{
	// Keep a temporary reference to the previous grid.
	// It avoids unloading/reloading images if they are the same, and avoids flickerings
	auto oldGrid = mHelp->getGrid();

	mHelp->clearPrompts();
	mHelp->setStyle(style);

	mClockElapsed = -1;

	std::vector<HelpPrompt> addPrompts;

	std::map<std::string, bool> inputSeenMap;
	std::map<std::string, int> mappedToSeenMap;
	for(auto it = prompts.cbegin(); it != prompts.cend(); it++)
	{
		// only add it if the same icon hasn't already been added
		if(inputSeenMap.emplace(it->first, true).second)
		{
			// this symbol hasn't been seen yet, what about the action name?
			auto mappedTo = mappedToSeenMap.find(it->second);
			if(mappedTo != mappedToSeenMap.cend())
			{
				// yes, it has!

				// can we combine? (dpad only)
				if((it->first == "up/down" && addPrompts.at(mappedTo->second).first != "left/right") ||
					(it->first == "left/right" && addPrompts.at(mappedTo->second).first != "up/down"))
				{
					// yes!
					addPrompts.at(mappedTo->second).first = "up/down/left/right";
					// don't need to add this to addPrompts since we just merged
				}else{
					// no, we can't combine!
					addPrompts.push_back(*it);
				}
			}else{
				// no, it hasn't!
				mappedToSeenMap.emplace(it->second, (int)addPrompts.size());
				addPrompts.push_back(*it);
			}
		}
	}

	// sort prompts so it goes [dpad_all] [dpad_u/d] [dpad_l/r] [a/b/x/y/l/r] [start/select]
	std::sort(addPrompts.begin(), addPrompts.end(), [](const HelpPrompt& a, const HelpPrompt& b) -> bool {

		static const char* map[] = {
			"up/down/left/right",
			"up/down",
			"left/right",
			"a", "b", "x", "y", "l", "r",
			"start", "select",
			NULL
		};

		int i = 0;
		int aVal = 0;
		int bVal = 0;
		while(map[i] != NULL)
		{
			if(a.first == map[i])
				aVal = i;
			if(b.first == map[i])
				bVal = i;
			i++;
		}

		return aVal > bVal;
	});

	mHelp->setPrompts(addPrompts);
}


void Window::onSleep()
{
	Scripting::fireEvent("sleep");
}

void Window::onWake()
{
	Scripting::fireEvent("wake");
}

bool Window::isProcessing()
{
	return count_if(mGuiStack.cbegin(), mGuiStack.cend(), [](GuiComponent* c) { return c->isProcessing(); }) > 0;
}

void Window::startScreenSaver()
{
	if (mScreenSaver && !mRenderScreenSaver)
	{
		for (auto extra : mScreenExtras)
			extra->onScreenSaverActivate();

		// Tell the GUI components the screensaver is starting
		for(auto i = mGuiStack.cbegin(); i != mGuiStack.cend(); i++)
			(*i)->onScreenSaverActivate();

		mScreenSaver->startScreenSaver();
		mRenderScreenSaver = true;
	}
}

bool Window::cancelScreenSaver()
{
	if (mScreenSaver && mRenderScreenSaver)
	{		
		mScreenSaver->stopScreenSaver();
		mRenderScreenSaver = false;
		mScreenSaver->resetCounts();

		// Tell the GUI components the screensaver has stopped
		for(auto i = mGuiStack.cbegin(); i != mGuiStack.cend(); i++)
			(*i)->onScreenSaverDeactivate();

		for (auto extra : mScreenExtras)
			extra->onScreenSaverDeactivate();

		return true;
	}

	return false;
}

void Window::renderScreenSaver()
{
	if (mScreenSaver)
		mScreenSaver->renderScreenSaver();
}

void Window::registerNotificationComponent(AsyncNotificationComponent* pc)
{
	std::unique_lock<std::mutex> lock(mNotificationMessagesLock);

	if (std::find(mAsyncNotificationComponent.cbegin(), mAsyncNotificationComponent.cend(), pc) != mAsyncNotificationComponent.cend())
		return;

	mAsyncNotificationComponent.push_back(pc);
}

void Window::unRegisterNotificationComponent(AsyncNotificationComponent* pc)
{
	std::unique_lock<std::mutex> lock(mNotificationMessagesLock);

	auto it = std::find(mAsyncNotificationComponent.cbegin(), mAsyncNotificationComponent.cend(), pc);
	if (it != mAsyncNotificationComponent.cend())
		mAsyncNotificationComponent.erase(it);
}

void Window::renderRegisteredNotificationComponents(const Transform4x4f& trans)
{
	std::unique_lock<std::mutex> lock(mNotificationMessagesLock);

#define PADDING_H  (Renderer::getScreenWidth()*0.01)

	float posY = Renderer::getScreenHeight() * 0.02f;

	for (auto child : mAsyncNotificationComponent)
	{		
		float posX = Renderer::getScreenWidth()*0.99f - child->getSize().x();

		child->setPosition(posX, posY, 0);
		child->render(trans);

		posY += child->getSize().y() + PADDING_H;
	}
}

void Window::postToUiThread(const std::function<void(Window*)>& func)
{
	std::unique_lock<std::mutex> lock(mNotificationMessagesLock);

	mFunctions.push_back(func);	
}

void Window::processPostedFunctions()
{
	std::unique_lock<std::mutex> lock(mNotificationMessagesLock);

	for (auto func : mFunctions)
		func(this);	

	mFunctions.clear();
}

void Window::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
	for (auto extra : mScreenExtras)
		delete extra;

	mScreenExtras.clear();
	mScreenExtras = ThemeData::makeExtras(theme, "screen", this);

	std::stable_sort(mScreenExtras.begin(), mScreenExtras.end(), [](GuiComponent* a, GuiComponent* b) { return b->getZIndex() > a->getZIndex(); });

	if (mBackgroundOverlay)
		mBackgroundOverlay->setImage(ThemeData::getMenuTheme()->Background.fadePath);

	if (mClock)
	{
		mClock->setFont(Font::get(FONT_SIZE_SMALL));
		mClock->setColor(0x777777FF);		
		mClock->setHorizontalAlignment(ALIGN_RIGHT);
		mClock->setVerticalAlignment(ALIGN_TOP);
		
		// if clock element does not exist in screen view -> <view name="screen"><text name="clock"> 
		// skin it from system.helpsystem -> <view name="system"><helpsystem name="help"> )
		if (!theme->getElement("screen", "clock", "text"))
		{
			auto elem = theme->getElement("system", "help", "helpsystem");
			if (elem && elem->has("textColor"))
				mClock->setColor(elem->get<unsigned int>("textColor"));

			if (elem && (elem->has("fontPath") || elem->has("fontSize")))
				mClock->setFont(Font::getFromTheme(elem, ThemeFlags::ALL, Font::get(FONT_SIZE_MEDIUM)));
		}
		
		mClock->setPosition(Renderer::getScreenWidth()*0.94, Renderer::getScreenHeight()*0.9965 - mClock->getFont()->getHeight());
		mClock->setSize(Renderer::getScreenWidth()*0.05, 0);

		mClock->applyTheme(theme, "screen", "clock", ThemeFlags::ALL ^ (ThemeFlags::TEXT));
	}

	if (mControllerActivity)
	{
		mControllerActivity->applyTheme(theme, "screen", "controllerActivity", ThemeFlags::ALL ^ (ThemeFlags::TEXT));
	}


	mVolumeInfo = std::make_shared<VolumeInfoComponent>(this);
}
