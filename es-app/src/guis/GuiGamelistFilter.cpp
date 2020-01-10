#include "guis/GuiGamelistFilter.h"

#include "components/OptionListComponent.h"
#include "views/UIModeController.h"
#include "SystemData.h"
#include "guis/GuiTextEditPopup.h"
#include "guis/GuiTextEditPopupKeyboard.h"

GuiGamelistFilter::GuiGamelistFilter(Window* window, SystemData* system) : GuiComponent(window), mMenu(window, _("FILTER GAMELIST BY")), mSystem(system)
{
	initializeMenu();
}

void GuiGamelistFilter::initializeMenu()
{
	addChild(&mMenu);

	// get filters from system

	mFilterIndex = mSystem->getIndex(true);

	ComponentListRow row;

	mMenu.addEntry(_("RESET ALL FILTERS"), false, std::bind(&GuiGamelistFilter::resetAllFilters, this));

	// addTextFilterToMenu();
	addFiltersToMenu();

	mMenu.addButton(_("BACK"), "back", std::bind(&GuiGamelistFilter::applyFilters, this));

	if (Renderer::isSmallScreen())
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, (Renderer::getScreenHeight() - mMenu.getSize().y()) / 2);
	else
		mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.15f);
}

void GuiGamelistFilter::resetAllFilters()
{
	mFilterIndex->resetFilters();
	for (std::map<FilterIndexType, std::shared_ptr< OptionListComponent<std::string> >>::const_iterator it = mFilterOptions.cbegin(); it != mFilterOptions.cend(); ++it ) {
		std::shared_ptr< OptionListComponent<std::string> > optionList = it->second;
		optionList->selectNone();
	}
}

GuiGamelistFilter::~GuiGamelistFilter()
{
	mFilterOptions.clear();

	if (!mFilterIndex->isFiltered())
		mSystem->deleteIndex();
}

void GuiGamelistFilter::addTextFilterToMenu()
{
	auto theme = ThemeData::getMenuTheme();
	std::shared_ptr<Font> font = theme->Text.font;
	unsigned int color = theme->Text.color;

	ComponentListRow row;

	auto lbl = std::make_shared<TextComponent>(mWindow, _("FIND GAMES"), font, color);
	row.addElement(lbl, true); // label

	mTextFilter = std::make_shared<TextComponent>(mWindow, mFilterIndex->getTextFilter(), font, color, ALIGN_RIGHT);
	row.addElement(mTextFilter, true);

	auto spacer = std::make_shared<GuiComponent>(mWindow);
	spacer->setSize(Renderer::getScreenWidth() * 0.005f, 0);
	row.addElement(spacer, false);

	auto bracket = std::make_shared<ImageComponent>(mWindow);
	bracket->setImage(theme->Icons.arrow);
	bracket->setResize(Vector2f(0, lbl->getFont()->getLetterHeight()));
	row.addElement(bracket, false);

	auto updateVal = [this](const std::string& newVal) { mTextFilter->setValue(Utils::String::toUpper(newVal)); };

	row.makeAcceptInputHandler([this, updateVal] 
	{
		if (Settings::getInstance()->getBool("UseOSK"))
			mWindow->pushGui(new GuiTextEditPopupKeyboard(mWindow, _("FIND GAMES"), mTextFilter->getValue(), updateVal, false));		
		else
			mWindow->pushGui(new GuiTextEditPopup(mWindow, _("FIND GAMES"), mTextFilter->getValue(), updateVal, false));		
	});

	mMenu.addRow(row);
}

void GuiGamelistFilter::addFiltersToMenu()
{
	std::vector<FilterDataDecl> decls = mFilterIndex->getFilterDataDecls();
	
	int skip = 0;
	if (!UIModeController::getInstance()->isUIModeFull())
		skip = 1;
	if (UIModeController::getInstance()->isUIModeKid())
		skip = 2;

	for (std::vector<FilterDataDecl>::const_iterator it = decls.cbegin(); it != decls.cend()-skip; ++it ) {

		FilterIndexType type = (*it).type; // type of filter
		std::map<std::string, int>* allKeys = (*it).allIndexKeys; // all possible filters for this type
		std::string menuLabel = (*it).menuLabel; // text to show in menu
		std::shared_ptr< OptionListComponent<std::string> > optionList;
		
		// add filters (with first one selected)
		ComponentListRow row;

		// add genres
		optionList = std::make_shared< OptionListComponent<std::string> >(mWindow, menuLabel, true);
		for(auto it: *allKeys)
			optionList->add(_(it.first.c_str()), it.first, mFilterIndex->isKeyBeingFilteredBy(it.first, type));

		if (allKeys->size() > 0)
			mMenu.addWithLabel(menuLabel, optionList);

		mFilterOptions[type] = optionList;
	}
}

void GuiGamelistFilter::applyFilters()
{
	if (mTextFilter)
		mFilterIndex->setTextFilter(mTextFilter->getValue());

	std::vector<FilterDataDecl> decls = mFilterIndex->getFilterDataDecls();
	for (std::map<FilterIndexType, std::shared_ptr< OptionListComponent<std::string> >>::const_iterator it = mFilterOptions.cbegin(); it != mFilterOptions.cend(); ++it ) 
	{
		std::shared_ptr< OptionListComponent<std::string> > optionList = it->second;
		std::vector<std::string> filters = optionList->getSelectedObjects();
		mFilterIndex->setFilter(it->first, &filters);
	}

	delete this;

}

bool GuiGamelistFilter::input(InputConfig* config, Input input)
{
	bool consumed = GuiComponent::input(config, input);
	if(consumed)
		return true;

	if(config->isMappedTo(BUTTON_BACK, input) && input.value != 0)
		applyFilters();

	return false;
}

std::vector<HelpPrompt> GuiGamelistFilter::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	return prompts;
}
