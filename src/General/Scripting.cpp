
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
#include "Utility/SFileDialog.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
namespace Scripting
{
	duk_context*	context = nullptr;
	ArchiveManager	archive_manager;
	string			error;
	wxWindow*		current_window = nullptr;

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
		wxMessageBox(message, title, 5L, Scripting::current_window);
	}

	// Prompt for a string
	string promptString(string title, string message, string default_value)
	{
		return wxGetTextFromUser(
			message.c_str(),
			title.c_str(),
			default_value.c_str(),
			Scripting::current_window
		);
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

	// Browse for a single file
	string browseFile(string title, string extensions, string filename)
	{
		SFileDialog::fd_info_t inf;
		SFileDialog::openFile(inf, title, extensions, Scripting::current_window, filename);
		return inf.filenames.empty() ? "" : inf.filenames[0];
	}

	// Browse for multiple files
	vector<string> browseFiles(string title, string extensions)
	{
		SFileDialog::fd_info_t inf;
		vector<string> filenames;
		if (SFileDialog::openFiles(inf, title, extensions, Scripting::current_window))
			filenames.assign(inf.filenames.begin(), inf.filenames.end());
		return filenames;
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

	// Switch to the tab for [archive], opening it if necessary
	bool showArchive(Archive* archive)
	{
		if (!archive)
			return false;

		theMainWindow->getArchiveManagerPanel()->openTab(archive);
		return true;
	}

	// Show [entry]
	bool showEntry(ArchiveEntry* entry)
	{
		return theMainWindow->getArchiveManagerPanel()->showEntry(entry);
	}



	// Raw Duktape functions
	duk_ret_t getMCBuffer(duk_context* ctx)
	{
		auto mc = (MemChunk*)duk_require_pointer(ctx, 0);
		if (mc)
			Scripting::pushBuffer(*mc);
		return 1;
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
		dukglue_register_method(context, &ScriptInterface::browseFile,				"browseFile");
		dukglue_register_method(context, &ScriptInterface::browseFiles,				"browseFiles");
		dukglue_register_method(context, &ScriptInterface::currentArchive,			"getCurrentArchive");
		dukglue_register_method(context, &ScriptInterface::currentEntry,			"getCurrentEntry");
		dukglue_register_method(context, &ScriptInterface::currentEntrySelection,	"getCurrentEntrySelection");
		dukglue_register_method(context, &ScriptInterface::showArchive,				"showArchive");
		dukglue_register_method(context, &ScriptInterface::showEntry,				"showEntry");

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
		dukglue_register_method(
			context,
			&ArchiveManager::getArchiveExtensionsString,
			"getArchiveExtensionsString"
		);
	}

	void registerArchive()
	{
		dukglue_register_method(context, &Archive::getFilename,					"getFilename");
		dukglue_register_method(context, &Archive::s_AllEntries,				"allEntries");
		dukglue_register_method(context, &Archive::s_GetDir,					"getDir");
		dukglue_register_method(context, &Archive::s_CreateEntry,				"createEntry");
		dukglue_register_method(context, &Archive::s_CreateEntryInNamespace,	"createEntryInNamespace");
		dukglue_register_method(context, &Archive::removeEntry,					"removeEntry");
		dukglue_register_method(context, &Archive::renameEntry,					"renameEntry");

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
		dukglue_register_method(context, &ArchiveEntry::getType,			"getType");
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

	void registerEntryType()
	{
		dukglue_register_property(context, &EntryType::getId, nullptr, "id");
		dukglue_register_property(context, &EntryType::getName, nullptr, "name");
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
	registerEntryType();
	registerArchiveEntry();
	registerArchiveTreeNode();
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

		if (!runScript(script, true))
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
bool Scripting::runScript(const string& script, bool use_global_context)
{
	// Create context for script if not using the global one
	duk_context* script_context = context;
	if (!use_global_context)
	{
		duk_push_thread(context);
		duk_context* script_context = duk_get_context(context, -1);
	}

	duk_push_lstring(script_context, script.c_str(), script.length());
	bool ok = true;
	if (duk_peval(script_context) != 0)
	{
		error = duk_safe_to_string(script_context, -1);
		ok = false;
	}
	duk_pop(script_context);

	return ok;
}

/* Scripting::invalidate
 * Invalidates [object] in the scripting context
 *******************************************************************/
void Scripting::invalidate(void* object)
{
	dukglue_invalidate_object(context, object);
}

/* Scripting::setCurrentWindow
 * Sets the 'current' scripting window, used as the parent for
 * script-launched dialogs, etc.
 *******************************************************************/
void Scripting::setCurrentWindow(wxWindow* window)
{
	current_window = window;
}

bool Scripting::pushBuffer(MemChunk& mc)
{
	return pushBuffer(mc.getData(), mc.getSize());
}

bool Scripting::pushBuffer(const uint8_t* data, unsigned size)
{
	auto buf = duk_push_fixed_buffer(context, size);
	memcpy(buf, data, size);
	duk_push_buffer_object(context, -1, 0, 1, DUK_BUFOBJ_UINT8ARRAY);
	duk_remove(context, -2);

	return true;
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
