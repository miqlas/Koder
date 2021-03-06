/*
 * Copyright 2014-2017 Kacper Kasper <kacperkasper@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "AppPreferencesWindow.h"

#include <stdlib.h>

#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <RadioButton.h>
#include <StringView.h>

#include <Scintilla.h>

#include "Preferences.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AppPreferencesWindow"


AppPreferencesWindow::AppPreferencesWindow(Preferences* preferences)
	:
	BWindow(BRect(0, 0, 400, 300), B_TRANSLATE("Application preferences"), B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS, 0)
{
	fCurrentPreferences = preferences;

	fStartPreferences = new Preferences();
	*fStartPreferences = *fCurrentPreferences;

	fTempPreferences = new Preferences();
	*fTempPreferences = *fCurrentPreferences;

	_InitInterface();
	CenterOnScreen();

	_SyncPreferences(fCurrentPreferences);
}


AppPreferencesWindow::~AppPreferencesWindow()
{
	delete fStartPreferences;
	delete fTempPreferences;
}


void
AppPreferencesWindow::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case Actions::COMPACT_LANG_MENU: {
			fTempPreferences->fCompactLangMenu =
				(fCompactLangMenuCB->Value() == B_CONTROL_ON ? true : false);
			_PreferencesModified();
		} break;
		case Actions::FULL_PATH_IN_TITLE: {
			fTempPreferences->fFullPathInTitle =
				(fFullPathInTitleCB->Value() == B_CONTROL_ON ? true : false);
			_PreferencesModified();
		} break;
		case Actions::TABS_TO_SPACES: {
			fTempPreferences->fTabsToSpaces =
				(fTabsToSpacesCB->Value() == B_CONTROL_ON ? true : false);
			_PreferencesModified();
		} break;
		case Actions::TAB_WIDTH: {
			fTempPreferences->fTabWidth = atoi(fTabWidthTC->Text());
			_PreferencesModified();
		} break;
		case Actions::LINE_HIGHLIGHTING: {
			fTempPreferences->fLineHighlighting =
				(fLineHighlightingCB->Value() == B_CONTROL_ON ? true : false);
			_PreferencesModified();
		} break;
		case Actions::LINE_NUMBERS: {
			fTempPreferences->fLineNumbers =
				(fLineNumbersCB->Value() == B_CONTROL_ON ? true : false);
			_PreferencesModified();
		} break;
		case Actions::LINELIMIT_COLUMN: {
			fTempPreferences->fLineLimitColumn =
				atoi(fLineLimitColumnTC->Text());
			_PreferencesModified();
		} break;
		case Actions::LINELIMIT_SHOW: {
			bool show = (fLineLimitShowCB->Value() == B_CONTROL_ON ? true : false);
			fTempPreferences->fLineLimitShow = show;
			_SetLineLimitBoxEnabled(show);
			_PreferencesModified();
		} break;
		case Actions::LINELIMIT_BACKGROUND: {
			fTempPreferences->fLineLimitMode = EDGE_BACKGROUND;
			_PreferencesModified();
		} break;
		case Actions::LINELIMIT_LINE: {
			fTempPreferences->fLineLimitMode = EDGE_LINE;
			_PreferencesModified();
		} break;
		case Actions::INDENTGUIDES_SHOW: {
			bool show = (fIndentGuidesShowCB->Value() == B_CONTROL_ON ? true : false);
			fTempPreferences->fIndentGuidesShow = show;
			_SetIndentGuidesBoxEnabled(show);
			_PreferencesModified();
		} break;
		case Actions::INDENTGUIDES_REAL: {
			fTempPreferences->fIndentGuidesMode = SC_IV_REAL;
			_PreferencesModified();
		} break;
		case Actions::INDENTGUIDES_FORWARD: {
			fTempPreferences->fIndentGuidesMode = SC_IV_LOOKFORWARD;
			_PreferencesModified();
		} break;
		case Actions::INDENTGUIDES_BOTH: {
			fTempPreferences->fIndentGuidesMode = SC_IV_LOOKBOTH;
			_PreferencesModified();
		} break;
		case Actions::BRACES_HIGHLIGHTING: {
			fTempPreferences->fBracesHighlighting =
				(fBracesHighlightingCB->Value() == B_CONTROL_ON ? true : false);
			_PreferencesModified();
		} break;
		case Actions::APPLY: {
			*fCurrentPreferences = *fTempPreferences;
			fApplyButton->SetEnabled(false);
			BMessage changed(APP_PREFERENCES_CHANGED);
			be_app->PostMessage(&changed);
		} break;
		case Actions::REVERT: {
			*fTempPreferences = *fStartPreferences;
			fRevertButton->SetEnabled(false);
			fApplyButton->SetEnabled(true);
			_SyncPreferences(fTempPreferences);
		} break;
		default: {
			BWindow::MessageReceived(message);
		} break;
	}
}


void
AppPreferencesWindow::Quit()
{
	be_app->PostMessage(APP_PREFERENCES_QUITTING);

	BWindow::Quit();
}


void
AppPreferencesWindow::_InitInterface()
{
	fEditorBox = new BBox("editorPrefs");
	fEditorBox->SetLabel(B_TRANSLATE("Editor"));
	fCompactLangMenuCB = new BCheckBox("compactLangMenu", B_TRANSLATE("Compact language menu"), new BMessage((uint32) Actions::COMPACT_LANG_MENU));
	fFullPathInTitleCB = new BCheckBox("fullPathInTitle", B_TRANSLATE("Show full path in title"), new BMessage((uint32) Actions::FULL_PATH_IN_TITLE));
	fTabsToSpacesCB = new BCheckBox("tabsToSpaces", B_TRANSLATE("Convert tabs to spaces"), new BMessage((uint32) Actions::TABS_TO_SPACES));
	fTabWidthTC = new BTextControl("tabWidth", B_TRANSLATE("Tab width: "), "4", new BMessage((uint32) Actions::TAB_WIDTH));
	fTabWidthText = new BStringView("tabWidthText", B_TRANSLATE(" characters"));
	fLineHighlightingCB = new BCheckBox("lineHighlighting", B_TRANSLATE("Highlight current line"), new BMessage((uint32) Actions::LINE_HIGHLIGHTING));
	fLineNumbersCB = new BCheckBox("lineNumbers", B_TRANSLATE("Display line numbers"), new BMessage((uint32) Actions::LINE_NUMBERS));

	fLineLimitHeaderView = new BView("lineLimitHeader", 0);
	fLineLimitShowCB = new BCheckBox("lineLimitShow", B_TRANSLATE("Mark lines exceeding "), new BMessage((uint32) Actions::LINELIMIT_SHOW));
	fLineLimitColumnTC = new BTextControl("lineLimitColumn", "", "80", new BMessage((uint32) Actions::LINELIMIT_COLUMN));
	fLineLimitText = new BStringView("lineLimitText", B_TRANSLATE(" characters"));

	BLayoutBuilder::Group<>(fLineLimitHeaderView, B_HORIZONTAL, 0)
		.Add(fLineLimitShowCB)
		.Add(fLineLimitColumnTC)
		.Add(fLineLimitText);

	fLineLimitBox = new BBox("lineLimitPrefs");
	fLineLimitBackgroundRadio = new BRadioButton("lineLimitRadio", B_TRANSLATE("Background"), new BMessage((uint32) Actions::LINELIMIT_BACKGROUND));
	fLineLimitLineRadio = new BRadioButton("lineLimitRadio", B_TRANSLATE("Line"), new BMessage((uint32) Actions::LINELIMIT_LINE));

	BLayoutBuilder::Group<>(fLineLimitBox, B_VERTICAL, 5)
		.Add(fLineLimitBackgroundRadio)
		.Add(fLineLimitLineRadio)
		.SetInsets(10, 25, 15, 10);
	fLineLimitBox->SetLabel(fLineLimitHeaderView);

	fIndentGuidesBox = new BBox("indentGuidesPrefs");
	fIndentGuidesShowCB = new BCheckBox("indentGuidesShow", B_TRANSLATE("Show indentation guides"), new BMessage((uint32) Actions::INDENTGUIDES_SHOW));
	fIndentGuidesRealRadio = new BRadioButton("indentGuidesReal", B_TRANSLATE("Real"), new BMessage((uint32) Actions::INDENTGUIDES_REAL));
	fIndentGuidesLookForwardRadio = new BRadioButton("indentGuidesForward", B_TRANSLATE("Up to the next non-empty line"), new BMessage((uint32) Actions::INDENTGUIDES_FORWARD));
	fIndentGuidesLookBothRadio = new BRadioButton("indentGuidesBoth", B_TRANSLATE("Up to the next/previous non-empty line"), new BMessage((uint32) Actions::INDENTGUIDES_BOTH));

	BLayoutBuilder::Group<>(fIndentGuidesBox, B_VERTICAL, 5)
		.Add(fIndentGuidesRealRadio)
		.Add(fIndentGuidesLookForwardRadio)
		.Add(fIndentGuidesLookBothRadio)
		.SetInsets(10, 25, 15, 10);
	fIndentGuidesBox->SetLabel(fIndentGuidesShowCB);

	fBracesHighlightingCB = new BCheckBox("bracesHighlighting", B_TRANSLATE("Highlight braces"), new BMessage((uint32) Actions::BRACES_HIGHLIGHTING));

	fApplyButton = new BButton(B_TRANSLATE("Apply"), new BMessage((uint32) Actions::APPLY));
	fRevertButton = new BButton(B_TRANSLATE("Revert"), new BMessage((uint32) Actions::REVERT));

	fApplyButton->SetEnabled(false);
	fRevertButton->SetEnabled(false);

	BLayoutBuilder::Group<>(fEditorBox, B_VERTICAL, 5)
		.Add(fCompactLangMenuCB)
		.Add(fFullPathInTitleCB)
		.Add(fTabsToSpacesCB)
		.AddGroup(B_HORIZONTAL, 0)
			.Add(fTabWidthTC)
			.Add(fTabWidthText)
		.End()
		.Add(fLineHighlightingCB)
		.Add(fLineNumbersCB)
		.Add(fLineLimitBox)
		.Add(fIndentGuidesBox)
		.Add(fBracesHighlightingCB)
		.AddGlue()
		.SetInsets(10, 15, 15, 10);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 5)
		.Add(fEditorBox)
		.AddGroup(B_HORIZONTAL, 5)
			.Add(fRevertButton)
			.AddGlue()
			.Add(fApplyButton)
		.End()
		.SetInsets(5, 5, 5, 5);
}


void
AppPreferencesWindow::_SyncPreferences(Preferences* preferences)
{
	if(preferences->fCompactLangMenu == true) {
		fCompactLangMenuCB->SetValue(B_CONTROL_ON);
	} else {
		fCompactLangMenuCB->SetValue(B_CONTROL_OFF);
	}

	if(preferences->fFullPathInTitle == true) {
		fFullPathInTitleCB->SetValue(B_CONTROL_ON);
	} else {
		fFullPathInTitleCB->SetValue(B_CONTROL_OFF);
	}

	if(preferences->fTabsToSpaces == true) {
		fTabsToSpacesCB->SetValue(B_CONTROL_ON);
	} else {
		fTabsToSpacesCB->SetValue(B_CONTROL_OFF);
	}

	BString tabWidthString;
	tabWidthString << preferences->fTabWidth;
	fTabWidthTC->SetText(tabWidthString.String());

	if(preferences->fLineNumbers == true) {
		fLineNumbersCB->SetValue(B_CONTROL_ON);
	} else {
		fLineNumbersCB->SetValue(B_CONTROL_OFF);
	}

	if(preferences->fLineHighlighting == true) {
		fLineHighlightingCB->SetValue(B_CONTROL_ON);
	} else {
		fLineHighlightingCB->SetValue(B_CONTROL_OFF);
	}

	BString columnString;
	columnString << preferences->fLineLimitColumn;
	fLineLimitColumnTC->SetText(columnString.String());
	if(preferences->fLineLimitShow == true) {
		fLineLimitShowCB->SetValue(B_CONTROL_ON);
		_SetLineLimitBoxEnabled(true);
	} else {
		fLineLimitShowCB->SetValue(B_CONTROL_OFF);
		_SetLineLimitBoxEnabled(false);
	}

	if(preferences->fIndentGuidesShow == true) {
		fIndentGuidesShowCB->SetValue(B_CONTROL_ON);
		_SetIndentGuidesBoxEnabled(true);
	} else {
		fIndentGuidesShowCB->SetValue(B_CONTROL_OFF);
		_SetIndentGuidesBoxEnabled(false);
	}

	if(preferences->fBracesHighlighting == true) {
		fBracesHighlightingCB->SetValue(B_CONTROL_ON);
	} else {
		fBracesHighlightingCB->SetValue(B_CONTROL_OFF);
	}
}


void
AppPreferencesWindow::_PreferencesModified()
{
	fApplyButton->SetEnabled(true);
	fRevertButton->SetEnabled(true);
}


void
AppPreferencesWindow::_SetLineLimitBoxEnabled(bool enabled)
{
	fLineLimitColumnTC->SetEnabled(enabled);
	fLineLimitBackgroundRadio->SetEnabled(enabled);
	fLineLimitLineRadio->SetEnabled(enabled);

	switch(fTempPreferences->fLineLimitMode) {
		case 1:
			fLineLimitLineRadio->SetValue(B_CONTROL_ON);
		break;
		case 2:
			fLineLimitBackgroundRadio->SetValue(B_CONTROL_ON);
		break;
	}
}


void
AppPreferencesWindow::_SetIndentGuidesBoxEnabled(bool enabled)
{
	fIndentGuidesRealRadio->SetEnabled(enabled);
	fIndentGuidesLookForwardRadio->SetEnabled(enabled);
	fIndentGuidesLookBothRadio->SetEnabled(enabled);

	switch(fTempPreferences->fIndentGuidesMode) {
		case 1:
			fIndentGuidesRealRadio->SetValue(B_CONTROL_ON);
		break;
		case 2:
			fIndentGuidesLookForwardRadio->SetValue(B_CONTROL_ON);
		break;
		case 3:
			fIndentGuidesLookBothRadio->SetValue(B_CONTROL_ON);
		break;
	}
}
