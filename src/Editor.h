/*
 * Copyright 2014-2017 Kacper Kasper <kacperkasper@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef EDITOR_H
#define EDITOR_H


#include <ScintillaView.h>
#include <SciLexer.h>


class Preferences;


enum {
	EDITOR_SAVEPOINT_LEFT		= 'svpl',
	EDITOR_SAVEPOINT_REACHED	= 'svpr'
};


class Editor : public BScintillaView {
public:
	enum Margin {
		NUMBER 		= 0,
		FOLD
	};

						Editor();

	void				NotificationReceived(SCNotification* notification);

	void				SetPreferences(Preferences* preferences);

private:
	void				_MaintainIndentation(char ch);
	void				_UpdateLineNumberWidth();
	void				_BraceHighlight();
	bool				_BraceMatch(int pos);
	void				_MarginClick(int margin, int pos);

	void				_SetLineIndentation(int line, int indent);
	Sci_CharacterRange	_GetSelection();
	void				_SetSelection(int anchor, int currentPos);

	Preferences*		fPreferences;
};


#endif // EDITOR_H
