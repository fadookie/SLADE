#pragma once

#include "UI/SDialog.h"

class TextEditor;
class wxTreeCtrl;
class SScriptDialog : public SDialog
{
public:
	SScriptDialog(wxWindow* parent);
	~SScriptDialog();

private:
	TextEditor*	text_editor;
	wxButton*	btn_run;
	wxTreeCtrl*	tree_scripts;

	// Setup
	void	bindEvents();
	void	populateScriptsTree();

	// Static
	static string	prev_script;
};
