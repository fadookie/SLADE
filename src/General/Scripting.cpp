
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    Scripting.cpp
 * Description: SLADE scripting system, using Duktape to embed a
 *              JavaScript engine
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
#include "Scripting.h"
#include "Archive/ArchiveManager.h"
#include "Archive/Formats/All.h"
#include "External/duktape/dukglue/dukglue.h"
#include "General/Console/Console.h"
#include "MainEditor/MainWindow.h"
#include "UI/TextEditor/TextEditor.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
namespace Scripting
{
	duk_context*	context = nullptr;
	ArchiveManager	archive_manager;
	string			error;

	// Testing
	string	prev_script_test;
}


/*******************************************************************
 * SCRIPTINTERFACE STRUCT
 *
 * This is so that we can set up a namespace for all the general
 * global functions in scripts
 *******************************************************************/
struct ScriptInterface
{
	// Log a message to the console
	void logMessage(const char* message)
	{
		LOG_MESSAGE(1, message);
	}

	// Get the global error message
	string globalError()
	{
		return Global::error;
	}

	// Show a message box
	void messageBox(string title, string message)
	{
		wxMessageBox(message, title);
	}

	// Prompt for a string
	string promptString(string title, string message, string default_value)
	{
		return wxGetTextFromUser(message.c_str(), title.c_str(), default_value.c_str());
	}

	// Prompt for a number
	int promptNumber(string title, string message, int default_value, int min, int max)
	{
		return (int)wxGetNumberFromUser(message.c_str(), "", title.c_str(), default_value, min, max);
	}

	// Prompt for a yes/no answer
	bool promptYesNo(string title, string message)
	{
		return (wxMessageBox(message, title, wxYES_NO | wxICON_QUESTION) == wxYES);
	}

	// Returns the global archive manager
	ArchiveManager* archiveManager()
	{
		return ArchiveManager::getInstance();
	}

	// Returns the current archive open in the UI
	Archive* currentArchive()
	{
		return theMainWindow->getCurrentArchive();
	}

	// Returns the current entry open in the UI
	ArchiveEntry* currentEntry()
	{
		return theMainWindow->getCurrentEntry();
	}

	// Returns the currently selected entries in the current archive
	vector<ArchiveEntry*> currentEntrySelection()
	{
		return theMainWindow->getCurrentEntrySelection();
	}
};

/*******************************************************************
 * SCRIPTING NAMESPACE FUNCTIONS
 *******************************************************************/
namespace Scripting
{
	void registerInterface()
	{
		dukglue_register_constructor<ScriptInterface>(context, "SLADEScriptInterface");

		dukglue_register_method(context, &ScriptInterface::logMessage,				"logMessage");
		dukglue_register_method(context, &ScriptInterface::messageBox,				"messageBox");
		dukglue_register_method(context, &ScriptInterface::promptString,			"promptString");
		dukglue_register_method(context, &ScriptInterface::promptNumber,			"promptNumber");
		dukglue_register_method(context, &ScriptInterface::promptYesNo,				"promptYesNo");
		dukglue_register_method(context, &ScriptInterface::currentArchive,			"getCurrentArchive");
		dukglue_register_method(context, &ScriptInterface::currentEntry,			"getCurrentEntry");
		dukglue_register_method(context, &ScriptInterface::currentEntrySelection,	"getCurrentEntrySelection");

		dukglue_register_property(context, &ScriptInterface::archiveManager, nullptr,	"archiveManager");
		dukglue_register_property(context, &ScriptInterface::globalError, nullptr,		"globalError");
	}

	void registerArchiveManager()
	{
		dukglue_register_method(context, &ArchiveManager::s_OpenFile,	"openFile");
		dukglue_register_method(context, &ArchiveManager::numArchives,	"numArchives");
		dukglue_register_method(context, &ArchiveManager::closeAll,		"closeAll");
		dukglue_register_method<ArchiveManager, Archive*, int>(
			context,
			&ArchiveManager::getArchive,
			"getArchive"
		);
		dukglue_register_method<ArchiveManager, bool, Archive*>(
			context,
			&ArchiveManager::closeArchive,
			"closeArchive"
		);
	}

	void registerArchive()
	{
		dukglue_register_method(context, &Archive::getFilename,		"getFilename");
		dukglue_register_method(context, &Archive::s_AllEntries,	"allEntries");
		dukglue_register_method(context, &Archive::s_GetDir,		"getDir");

		dukglue_register_property(context, &Archive::isModified,	nullptr, "modified");
		dukglue_register_property(context, &Archive::isOnDisk,		nullptr, "onDisk");
		dukglue_register_property(context, &Archive::isReadOnly,	nullptr, "readOnly");

		// Register all subclasses
		// (perhaps it'd be a good idea to make Archive not abstract and handle
		//  the format-specific stuff somewhere else, rather than in subclasses)
		dukglue_set_base_class<Archive, WadArchive>(context);
		dukglue_set_base_class<Archive, ZipArchive>(context);
		dukglue_set_base_class<Archive, LibArchive>(context);
		dukglue_set_base_class<Archive, DatArchive>(context);
		dukglue_set_base_class<Archive, ResArchive>(context);
		dukglue_set_base_class<Archive, PakArchive>(context);
		dukglue_set_base_class<Archive, BSPArchive>(context);
		dukglue_set_base_class<Archive, GrpArchive>(context);
		dukglue_set_base_class<Archive, RffArchive>(context);
		dukglue_set_base_class<Archive, GobArchive>(context);
		dukglue_set_base_class<Archive, LfdArchive>(context);
		dukglue_set_base_class<Archive, HogArchive>(context);
		dukglue_set_base_class<Archive, ADatArchive>(context);
		dukglue_set_base_class<Archive, Wad2Archive>(context);
		dukglue_set_base_class<Archive, WadJArchive>(context);
		dukglue_set_base_class<Archive, WolfArchive>(context);
		dukglue_set_base_class<Archive, GZipArchive>(context);
		dukglue_set_base_class<Archive, BZip2Archive>(context);
		dukglue_set_base_class<Archive, TarArchive>(context);
		dukglue_set_base_class<Archive, DiskArchive>(context);
		dukglue_set_base_class<Archive, PodArchive>(context);
		dukglue_set_base_class<Archive, ChasmBinArchive>(context);
	}

	void registerArchiveEntry()
	{
		dukglue_register_method(context, &ArchiveEntry::getName,			"getName");
		dukglue_register_method(context, &ArchiveEntry::getUpperName,		"getUpperName");
		dukglue_register_method(context, &ArchiveEntry::getUpperNameNoExt,	"getUpperNameNoExt");
		dukglue_register_method(context, &ArchiveEntry::getPath,			"getPath");
		dukglue_register_method(context, &ArchiveEntry::getSizeString,		"getSizeString");
		dukglue_register_method(context, &ArchiveEntry::getTypeString,		"getTypeString");
	}

	void registerArchiveTreeNode()
	{
		dukglue_register_method(context, &ArchiveTreeNode::getArchive,		"getArchive");
		dukglue_register_method(context, &ArchiveTreeNode::getName,			"getName");
		dukglue_register_method(context, &ArchiveTreeNode::numEntries,		"numEntries");
		dukglue_register_method(context, &ArchiveTreeNode::s_GetEntries,	"getEntries");
		dukglue_register_method(context, &ArchiveTreeNode::s_EntryIndex,	"entryIndex");
		dukglue_register_method<ArchiveTreeNode, ArchiveEntry*, unsigned>(
			context,
			&ArchiveTreeNode::getEntry,
			"getEntry"
		);
	}
}

/* Scripting::init
 * Sets up the scripting system
 *******************************************************************/
bool Scripting::init()
{
	// Init duktape context
	context = duk_create_heap_default();
	if (!context)
		return false;

	// Register general functions from script interface class
	registerInterface();

	// Register classes
	registerArchiveEntry();
	registerArchive();
	registerArchiveManager();

	// Initialise scripting environment
	auto script_init_entry = theArchiveManager->programResourceArchive()->entryAtPath("scripts/init.js");
	if (script_init_entry)
	{
		auto script = wxString::From8BitData(
			(const char*)script_init_entry->getData(),
			script_init_entry->getSize()
		);

		if (!runScript(script))
		{
			LOG_MESSAGE(1, "Error initialising scripting environment: %s", error);
			return false;
		}
	}

	return true;
}

/* Scripting::close
 * Closes and cleans up the scripting system
 *******************************************************************/
bool Scripting::close()
{
	if (context)
		duk_destroy_heap(context);

	return true;
}

/* Scripting::getError
 * Returns the last error generated from javascript
 *******************************************************************/
string Scripting::getError()
{
	return error;
}

/* Scripting::runScript
 * Runs [script] and returns true on success or false if an error
 * occurred
 *******************************************************************/
bool Scripting::runScript(const string& script)
{
	duk_push_lstring(context, script.c_str(), script.length());
	bool ok = true;
	if (duk_peval(context) != 0)
	{
		error = duk_safe_to_string(context, -1);
		ok = false;
	}
	duk_pop(context);
	return ok;
}

/* Scripting::invalidate
 * Invalidates [object] in the scripting context
 *******************************************************************/
void Scripting::invalidate(void* object)
{
	dukglue_invalidate_object(context, object);
}

void Scripting::openScriptTestDialog(wxWindow* parent)
{
	wxDialog dlg(
		parent,
		-1,
		"SLADEScript Test",
		wxDefaultPosition,
		wxSize(800, 600),
		wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER
	);

	auto sizer = new wxBoxSizer(wxVERTICAL);
	dlg.SetSizer(sizer);

	auto text_editor = new TextEditor(&dlg, -1);
	text_editor->SetText(prev_script_test);
	text_editor->setLanguage(TextLanguage::getLanguageByName("sladescript"));
	sizer->Add(text_editor, 1, wxEXPAND | wxALL, 10);
	sizer->Add(dlg.CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 10);

	dlg.CenterOnParent();
	if (dlg.ShowModal() == wxID_OK)
	{
		prev_script_test = text_editor->GetText();
		if (!runScript(prev_script_test))
			wxMessageBox(error, "Script Error", wxOK | wxICON_ERROR, parent);
	}
}


/*******************************************************************
 * CONSOLE COMMANDS
 *******************************************************************/

CONSOLE_COMMAND(exec_script, 1, false)
{
	if (!Scripting::runScript(args[0]))
		theConsole->logMessage(Scripting::getError());
}

CONSOLE_COMMAND(exec_script_file, 1, false)
{
	wxFile file;
	string script;
	if (file.Open(args[0]) && file.ReadAll(&script))
	{
		if (!Scripting::runScript(script))
			theConsole->logMessage(Scripting::getError());
	}
}

CONSOLE_COMMAND(script_test, 0, false)
{
	Scripting::openScriptTestDialog(theMainWindow);
}

CONSOLE_COMMAND(exec_script_res, 1, false)
{
	auto path = S_FMT("scripts/%s", args[0]);
	auto script_init_entry = theArchiveManager->programResourceArchive()->entryAtPath(path);
	if (script_init_entry)
	{
		auto script = wxString::From8BitData(
			(const char*)script_init_entry->getData(),
			script_init_entry->getSize()
		);

		if (!Scripting::runScript(script))
			theConsole->logMessage(S_FMT("Error initialising scripting environment: %s", Scripting::error));
	}
	else
		theConsole->logMessage(S_FMT("Resource script \"scripts/%s\" not found", args[0]));
}

CONSOLE_COMMAND(script_reset, 0, true)
{
	Scripting::close();
	Scripting::init();
}
