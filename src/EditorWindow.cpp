/*
 * Koder is a code editor for Haiku based on Scintilla.
 *
 * Copyright (C) 2014-2015 Kacper Kasper <kacperkasper@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "EditorWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Entry.h>
#include <File.h>
#include <FilePanel.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MimeType.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>

#include "Editor.h"
#include "GoToLineWindow.h"
#include "Languages.h"
#include "Preferences.h"
#include "Styler.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "EditorWindow"

Preferences* EditorWindow::fPreferences = NULL;
Styler* EditorWindow::fStyler = NULL;

EditorWindow::EditorWindow()
	:
	BWindow(BRect(200, 200, 605, 605), gAppName, B_DOCUMENT_WINDOW, 0)
{
	fGoToLineWindow = NULL;
	fOpenedFilePath = NULL;
	fOpenedFileMimeType.SetTo("text/plain");
	
	BMessenger* windowMessenger = new BMessenger(this);
	fOpenPanel = new BFilePanel();
	fSavePanel = new BFilePanel(B_SAVE_PANEL, windowMessenger, NULL, 0, false);
	
	fMainMenu = new BMenuBar("MainMenu");
	BLayoutBuilder::Menu<>(fMainMenu)
		.AddMenu(B_TRANSLATE("File"))
			.AddItem(B_TRANSLATE("New"), MAINMENU_FILE_NEW, 'N')
			.AddSeparator()
			.AddItem(B_TRANSLATE("Open"), MAINMENU_FILE_OPEN, 'O')
			.AddItem(B_TRANSLATE("Save"), MAINMENU_FILE_SAVE, 'S')
			.AddItem(B_TRANSLATE("Save as" B_UTF8_ELLIPSIS), MAINMENU_FILE_SAVEAS)
			.AddSeparator()
			.AddItem(B_TRANSLATE("Close"), B_QUIT_REQUESTED, 'W')
			.AddItem(B_TRANSLATE("Quit"), MAINMENU_FILE_QUIT, 'Q')
		.End()
		.AddMenu(B_TRANSLATE("Edit"))
			.AddItem(B_TRANSLATE("Undo"), B_UNDO, 'Z')
			.AddItem(B_TRANSLATE("Redo"), B_REDO, 'Y')
			.AddSeparator()
			.AddItem(B_TRANSLATE("Cut"), B_CUT, 'X')
			.AddItem(B_TRANSLATE("Copy"), B_COPY, 'C')
			.AddItem(B_TRANSLATE("Paste"), B_PASTE, 'V')
			.AddSeparator()
			.AddItem(B_TRANSLATE("Select all"), B_SELECT_ALL, 'A')
			.AddSeparator()
			.AddItem(B_TRANSLATE("File preferences" B_UTF8_ELLIPSIS), MAINMENU_EDIT_FILE_PREFERENCES)
			.AddItem(B_TRANSLATE("Application preferences" B_UTF8_ELLIPSIS), MAINMENU_EDIT_APP_PREFERENCES)
		.End()
		.AddMenu(B_TRANSLATE("Search"))
			.AddItem(B_TRANSLATE("Go to line" B_UTF8_ELLIPSIS), MAINMENU_SEARCH_GOTOLINE, ',')
		.End()
		.AddMenu(B_TRANSLATE("Language"))
			.AddItem("Dummy", MAINMENU_LANGUAGE)
		.End()
		.AddMenu(B_TRANSLATE("Help"))
			.AddItem(B_TRANSLATE("About" B_UTF8_ELLIPSIS), B_ABOUT_REQUESTED)
		.End();

	BMenu *languageMenu = fMainMenu->FindItem(MAINMENU_LANGUAGE)->Menu();
	languageMenu->RemoveItem((int32) 0);
	_PopulateLanguageMenu(languageMenu);

	fEditor = new Editor();

	BGroupLayout *layout = new BGroupLayout(B_VERTICAL, 0);
	SetLayout(layout);
	layout->AddView(fMainMenu);
	layout->AddView(fEditor);
	
	if(fPreferences != NULL) {
		fEditor->SendMessage(SCI_SETTABWIDTH, fPreferences->fTabWidth, 0);
		fEditor->SendMessage(SCI_SETUSETABS, !fPreferences->fTabsToSpaces, 0);
		fEditor->SendMessage(SCI_SETCARETLINEVISIBLE, fPreferences->fLineHighlighting, 0);
		if(fPreferences->fLineNumbers) {
			fEditor->SendMessage(SCI_SETMARGINTYPEN, 0, (long int) SC_MARGIN_NUMBER);
			fEditor->SendMessage(SCI_SETMARGINWIDTHN, 0, 50);
		}
		fEditor->SendMessage(SCI_SETINDENTATIONGUIDES, fPreferences->fIndentationGuides, 0);
	}

	fEditor->SendMessage(SCI_STYLESETFONT, STYLE_DEFAULT, (sptr_t) "DejaVu Sans Mono");
	
	fEditor->SendMessage(SCI_USEPOPUP, 0, 0);
	
	fStyler->ApplyGlobal(fEditor);

	RefreshTitle();
}

void
EditorWindow::New()
{
	be_app->PostMessage(WINDOW_NEW);
}

void
EditorWindow::OpenFile(entry_ref* ref)
{
	BEntry entry(ref);
	
	char mimeType[256];
	int32 caretPos = 0;
	BNode node(&entry);
	node.ReadAttr("be:caret_position", B_INT32_TYPE, 0, &caretPos, 4);
	node.ReadAttr("BEOS:TYPE", B_MIME_TYPE, 0, mimeType, 256);
	
	BFile file(&entry, B_READ_ONLY);
	off_t size;
	file.GetSize(&size);
	char* buffer = new char[size + 1];
	file.Read(buffer, size);
	buffer[size] = 0;
	fEditor->SetText(buffer);
	fEditor->SendMessage(SCI_SETSAVEPOINT, 0, 0);
	delete []buffer;
	
	fEditor->SendMessage(SCI_GOTOPOS, caretPos, 0);
	fOpenedFileMimeType.SetTo(mimeType);
	
	be_roster->AddToRecentDocuments(ref, gAppMime);
	
	if(fOpenedFilePath == NULL)
		fOpenedFilePath = new BPath(&entry);
	RefreshTitle();
}

void
EditorWindow::RefreshTitle()
{
	BString title;
	if(fEditor->SendMessage(SCI_GETMODIFY, 0, 0))
		title << "*";
	if(fOpenedFilePath != NULL)
		title << fOpenedFilePath->Path();
	else {
		title << B_TRANSLATE("Untitled");
	}
	SetTitle(title);
}

void
EditorWindow::SaveFile(BPath* path)
{
	// TODO error checking
	BEntry backup(path->Path());
	if(fOpenedFilePath != NULL) {
		backup.Rename(BString(path->Path()).Append("~"));
	}
	BFile file(path->Path(), B_WRITE_ONLY | B_CREATE_FILE);
	int length = fEditor->TextLength() + 1;
	char* buffer = new char[length];
	fEditor->GetText(0, length, buffer);
	file.Write(buffer, length - 1);
	fEditor->SendMessage(SCI_SETSAVEPOINT, 0, 0);
	delete []buffer;
	
	const char* mimeType = fOpenedFileMimeType.Type();
	BNode node(path->Path());
	BNodeInfo nodeInfo(&node);
	nodeInfo.SetType(mimeType);
	
	if(fOpenedFilePath != NULL)
		backup.Remove();
	
	if(fOpenedFilePath == NULL)
		fOpenedFilePath = new BPath(*path);
	RefreshTitle();
}

bool
EditorWindow::QuitRequested()
{
	bool close = true;
	if(fEditor->SendMessage(SCI_GETMODIFY, 0, 0)) {
		int32 result;
		BAlert* modifiedAlert = new BAlert(B_TRANSLATE("Unsaved changes"),
			B_TRANSLATE("The file contains unsaved changes. What do you "
				"want to do?"),
			"Cancel", "Discard", "Save", B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
			B_STOP_ALERT);
		modifiedAlert->SetShortcut(0, B_ESCAPE);
		result = modifiedAlert->Go();
		switch(result) {
		case 0:
			close = false;
		break;
		case 1:
			close = true;
		break;
		case 2:
			if(fOpenedFilePath == NULL)
				fSavePanel->Show();
			else
				SaveFile(fOpenedFilePath);
			close = true;
		break;
		}
	}
	if(close == true) {
		if(fOpenedFilePath != NULL) {
			int32 caretPos = fEditor->SendMessage(SCI_GETCURRENTPOS, 0, 0);
			BNode node(fOpenedFilePath->Path());
			node.WriteAttr("be:caret_position", B_INT32_TYPE, 0, &caretPos, 4);
		}

		if(fGoToLineWindow != NULL) {
			fGoToLineWindow->LockLooper();
			fGoToLineWindow->Quit();
		}
		
		delete fOpenPanel;
		delete fSavePanel;
		
		BMessage closing(WINDOW_CLOSE);
		closing.AddPointer("window", this);
		be_app->PostMessage(&closing);
	}
	return close;
}

void
EditorWindow::MessageReceived(BMessage* message)
{
	if(message->what >= MAINMENU_LANGUAGE && message->what < MAINMENU_LANGUAGE + LANGUAGE_COUNT) {
		uint32 lang = message->what - MAINMENU_LANGUAGE;
		Languages languages;
		LanguageDefinition& langDef = languages.GetLanguage(static_cast<LanguageType>(lang));
		fEditor->SendMessage(SCI_SETLEXER, static_cast<uptr_t>(langDef.fLexerID), 0);
		fStyler->ApplyLanguage(fEditor, langDef.fLexerName.String());
		languages.ApplyLanguage(fEditor, "data/langs.xml", langDef.fLexerName.String());
		return;
	}
	switch(message->what) {
		case MAINMENU_FILE_NEW:
			New();
		break;
		case MAINMENU_FILE_OPEN: {
			fOpenPanel->Show();
		} break;
		case MAINMENU_FILE_SAVE: {
			if(fEditor->SendMessage(SCI_GETMODIFY, 0, 0)) {
				if(fOpenedFilePath == NULL)
					fSavePanel->Show();
				else
					SaveFile(fOpenedFilePath);
			}
		} break;
		case MAINMENU_FILE_SAVEAS: {
			fSavePanel->Show();
		} break;
		case MAINMENU_FILE_QUIT: {
			be_app->PostMessage(B_QUIT_REQUESTED);
		} break;
		case MAINMENU_EDIT_APP_PREFERENCES: {
			be_app->PostMessage(message);
		} break;
		case MAINMENU_SEARCH_GOTOLINE: {
			if(fGoToLineWindow == NULL) {
				fGoToLineWindow = new GoToLineWindow(this);
			}
			fGoToLineWindow->ShowCentered(Frame());
		} break;
		case B_SAVE_REQUESTED: {
			entry_ref ref;
			if(message->FindRef("directory", &ref) == B_OK) {
				BString name;
				message->FindString("name", &name);
				BPath path(&ref);
				path.Append(name);
				
				SaveFile(&path);
			}
		} break;
		case B_CUT: {
			fEditor->SendMessage(SCI_CUT, 0, 0);
		} break;
		case B_COPY: {
			fEditor->SendMessage(SCI_COPY, 0, 0);
		} break;
		case B_PASTE: {
			fEditor->SendMessage(SCI_PASTE, 0, 0);
		} break;
		case B_SELECT_ALL: {
			fEditor->SendMessage(SCI_SELECTALL, 0, 0);
		} break;
		case B_UNDO: {
			fEditor->SendMessage(SCI_UNDO, 0, 0);
			// TODO: disable menuitem
		} break;
		case B_REDO: {
			fEditor->SendMessage(SCI_REDO, 0, 0);
		} break;
		case EDITOR_SAVEPOINT_LEFT:
		case EDITOR_SAVEPOINT_REACHED:
			RefreshTitle();
		break;
		case B_ABOUT_REQUESTED:
			be_app->PostMessage(message);
		break;
		case GTLW_GO: {
			int32 line;
			if(message->FindInt32("line", &line) == B_OK) {
				fEditor->SendMessage(SCI_GOTOLINE, line - 1, 0);
			}
		} break;
		default:
			BWindow::MessageReceived(message);
		break;
	}
}


void
EditorWindow::_PopulateLanguageMenu(BMenu* languageMenu)
{
	Languages languages;
	languages.SortAlphabetically();
	for(int i = 0; i < LANGUAGE_COUNT; ++i) {
		LanguageDefinition& langDef = languages.GetLanguages().at(i);
		BMenuItem *menuItem = new BMenuItem(langDef.fShortName, new BMessage(MAINMENU_LANGUAGE + langDef.fType));
		languageMenu->AddItem(menuItem);
	}
}