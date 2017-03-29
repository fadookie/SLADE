
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    SScriptDialog.cpp
 * Description: SLADEScript manager dialog
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
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "SScriptDialog.h"
#include "Archive/ArchiveManager.h"
#include "General/Scripting.h"
#include "UI/TextEditor/TextEditor.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
string SScriptDialog::prev_script = "";


/*******************************************************************
 * SCRIPTTREEITEMDATA CLASS
 *******************************************************************
 Just used to store ArchiveEntry pointers with wxTreeCtrl items
 */
class ScriptTreeItemData : public wxTreeItemData
{
public:
	ScriptTreeItemData(ArchiveEntry* entry) : entry{ entry } {}
	ArchiveEntry* entry;
};


/*******************************************************************
 * SSCRIPTDIALOG CLASS FUNCTIONS
 *******************************************************************/

/* SScriptDialog::SScriptDialog
 * SScriptDialog class constructor
 *******************************************************************/
SScriptDialog::SScriptDialog(wxWindow* parent) :
	SDialog(parent, "Script Manager", "script_manager", 800, 600)
{
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Scripts Tree
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 1, wxEXPAND | wxALL, 10);
	tree_scripts = new wxTreeCtrl(
		this,
		-1,
		wxDefaultPosition,
		{ 200, -1 },
		wxTR_DEFAULT_STYLE|wxTR_NO_LINES|wxTR_HIDE_ROOT|wxTR_FULL_ROW_HIGHLIGHT
	);
	tree_scripts->EnableSystemTheme();
	populateScriptsTree();
	hbox->Add(tree_scripts, 0, wxEXPAND | wxRIGHT, 10);

	// Text Editor
	text_editor = new TextEditor(this, -1);
	text_editor->SetText(prev_script);
	text_editor->setLanguage(TextLanguage::getLanguageByName("sladescript"));
	hbox->Add(text_editor, 1, wxEXPAND);

	// Buttons
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
	btn_run = new wxButton(this, -1, "Run Script");
	hbox->AddStretchSpacer();
	hbox->Add(btn_run, 0, wxEXPAND);

	// Bind events
	bindEvents();

	SetMinSize({ 500, 400 });
	Layout();
	CenterOnParent();
}

/* SScriptDialog::~SScriptDialog
 * SScriptDialog class destructor
 *******************************************************************/
SScriptDialog::~SScriptDialog()
{
}

/* SScriptDialog::bindEvents
 * Bind events for wx controls
 *******************************************************************/
void SScriptDialog::bindEvents()
{
	// 'Run' button click
	btn_run->Bind(wxEVT_BUTTON, [=](wxCommandEvent)
	{
		prev_script = text_editor->GetText();
		if (!Scripting::runScript(prev_script))
			wxMessageBox(Scripting::getError(), "Script Error", wxOK | wxICON_ERROR, this);
	});

	// Tree item activate
	tree_scripts->Bind(wxEVT_TREE_ITEM_ACTIVATED, [=](wxTreeEvent e)
	{
		auto data = (ScriptTreeItemData*)tree_scripts->GetItemData(e.GetItem());
		if (data && data->entry)
			text_editor->loadEntry(data->entry);
	});
}

/* SScriptDialog::populateScriptsTree
 * Loads scripts from slade.pk3 into the scripts tree control
 *******************************************************************/
void SScriptDialog::populateScriptsTree()
{
	// Clear tree
	tree_scripts->DeleteAllItems();

	// Get 'scripts' dir of slade.pk3
	auto scripts_dir = theArchiveManager->programResourceArchive()->getDir("scripts");
	if (!scripts_dir)
		return;

	// Recursive function to populate the tree
	std::function<void(wxTreeCtrl*, wxTreeItemId, ArchiveTreeNode*)> addToTree =
		[&](wxTreeCtrl* tree, wxTreeItemId node, ArchiveTreeNode* dir)
	{
		// Add subdirs
		for (unsigned a = 0; a < dir->nChildren(); a++)
		{
			auto subdir = (ArchiveTreeNode*)dir->getChild(a);
			auto subnode = tree->AppendItem(node, subdir->getName());
			addToTree(tree, subnode, subdir);
		}

		// Add script files
		for (unsigned a = 0; a < dir->numEntries(); a++)
		{
			auto entry = dir->getEntry(a);
			tree->AppendItem(node, entry->getName(true), -1, -1, new ScriptTreeItemData(entry));
		}
	};

	// Populate from root
	auto root = tree_scripts->AddRoot("Scripts");
	addToTree(tree_scripts, root, scripts_dir);
}
