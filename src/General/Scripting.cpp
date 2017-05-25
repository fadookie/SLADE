
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
#include "UI/TextEditor/TextEditor.h"
#include "Utility/SFileDialog.h"
#include "MapEditor/SLADEMap/SLADEMap.h"
#include "Dialogs/SScriptDialog.h"
#include "MainEditor/MainEditor.h"
#include "MapEditor/Edit/Edit3D.h"
#include "MapEditor/MapEditContext.h"
#include "Game/ThingType.h"


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
		Log::message(Log::MessageType::Script, message);
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
		return MainEditor::currentArchive();
	}

	// Returns the current entry open in the UI
	ArchiveEntry* currentEntry()
	{
		return MainEditor::currentEntry();
	}

	// Returns the currently selected entries in the current archive
	vector<ArchiveEntry*> currentEntrySelection()
	{
		return MainEditor::currentEntrySelection();
	}

	// Switch to the tab for [archive], opening it if necessary
	bool showArchive(Archive* archive)
	{
		if (!archive)
			return false;

		MainEditor::openArchiveTab(archive);
		return true;
	}

	// Show [entry]
	bool showEntry(ArchiveEntry* entry)
	{
		return MainEditor::showEntry(entry);
	}
	
	// Returns the map editor
	MapEditContext* mapEditor()
	{
		return &(MapEditor::editContext());
	}

	SLADEMap* currentMap()
	{
		return &(MapEditor::editContext().map());
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
		dukglue_register_property(context, &ScriptInterface::mapEditor, nullptr,		"mapEditor");
		dukglue_register_property(context, &ScriptInterface::currentMap, nullptr,		"map");
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
		dukglue_register_property(context, &EntryType::getId, nullptr,		"id");
		dukglue_register_property(context, &EntryType::getName, nullptr,	"name");
	}

	void registerSLADEMap()
	{
		dukglue_register_property(context, &SLADEMap::mapName, nullptr,			"name");
		dukglue_register_property(context, &SLADEMap::udmfNamespace, nullptr,	"udmfNamespace");
		dukglue_register_property(context, &SLADEMap::s_GetVertices, nullptr,	"vertices");
		dukglue_register_property(context, &SLADEMap::s_GetLines, nullptr,		"linedefs");
		dukglue_register_property(context, &SLADEMap::s_GetSides, nullptr,		"sidedefs");
		dukglue_register_property(context, &SLADEMap::s_GetSectors, nullptr,	"sectors");
		dukglue_register_property(context, &SLADEMap::s_GetThings, nullptr,		"things");

		dukglue_register_method(context, &SLADEMap::nVertices,	"numVertices");
		dukglue_register_method(context, &SLADEMap::nLines,		"numLines");
		dukglue_register_method(context, &SLADEMap::nSides,		"numSides");
		dukglue_register_method(context, &SLADEMap::nSectors,	"numSectors");
		dukglue_register_method(context, &SLADEMap::nThings,	"numThings");
	}

	void registerItemSelection()
	{
		dukglue_register_method(context, &ItemSelection::selectedVertices,	"selectedVertices");
		dukglue_register_method(context, &ItemSelection::selectedLines,		"selectedLines");
		dukglue_register_method(context, &ItemSelection::selectedSectors,	"selectedSectors");
		dukglue_register_method(context, &ItemSelection::selectedThings,	"selectedThings");
	}

	void registerMapEditor()
	{
		registerItemSelection();

		dukglue_register_property(context, &MapEditContext::s_EditMode, nullptr,		"editMode");
		dukglue_register_property(context, &MapEditContext::s_SectorEditMode, nullptr,	"sectorEditMode");
		dukglue_register_property(context, &MapEditContext::gridSize, nullptr,			"gridSize");
		dukglue_register_property(context, &MapEditContext::selection, nullptr,			"selection");
	}

	void registerMapVertex()
	{
		dukglue_set_base_class<MapObject, MapVertex>(context);

		dukglue_register_property(context, &MapVertex::xPos, nullptr, "x");
		dukglue_register_property(context, &MapVertex::yPos, nullptr, "y");
	}

	void registerMapLine()
	{
		dukglue_set_base_class<MapObject, MapLine>(context);

		dukglue_register_property(context, &MapLine::x1, nullptr,			"x1");
		dukglue_register_property(context, &MapLine::y1, nullptr,			"y1");
		dukglue_register_property(context, &MapLine::x2, nullptr,			"x2");
		dukglue_register_property(context, &MapLine::y2, nullptr,			"y2");
		dukglue_register_property(context, &MapLine::v1, nullptr,			"vertex1");
		dukglue_register_property(context, &MapLine::v2, nullptr,			"vertex2");
		dukglue_register_property(context, &MapLine::s1, nullptr,			"side1");
		dukglue_register_property(context, &MapLine::s2, nullptr,			"side2");
		dukglue_register_property(context, &MapLine::getSpecial, nullptr,	"special");

		dukglue_register_method(context, &MapLine::getLength,	"length");
		dukglue_register_method(context, &MapLine::frontSector,	"frontSector");
		dukglue_register_method(context, &MapLine::backSector,	"backSector");
	}

	void registerMapSide()
	{
		dukglue_set_base_class<MapObject, MapSide>(context);

		dukglue_register_property(context, &MapSide::getSector, nullptr,		"sector");
		dukglue_register_property(context, &MapSide::getParentLine, nullptr,	"line");
		dukglue_register_property(context, &MapSide::getTexLower, nullptr,		"textureBottom");
		dukglue_register_property(context, &MapSide::getTexMiddle, nullptr,		"textureMiddle");
		dukglue_register_property(context, &MapSide::getTexUpper, nullptr,		"textureTop");
		dukglue_register_property(context, &MapSide::getOffsetX, nullptr,		"offsetX");
		dukglue_register_property(context, &MapSide::getOffsetY, nullptr,		"offsetY");
	}

	void registerMapSector()
	{
		dukglue_set_base_class<MapObject, MapSector>(context);

		dukglue_register_property(context, &MapSector::getFloorTex, nullptr,		"textureFloor");
		dukglue_register_property(context, &MapSector::getCeilingTex, nullptr,		"textureCeiling");
		dukglue_register_property(context, &MapSector::getFloorHeight, nullptr,		"heightFloor");
		dukglue_register_property(context, &MapSector::getCeilingHeight, nullptr,	"heightCeiling");
		dukglue_register_property(context, &MapSector::getLightLevel, nullptr,		"lightLevel");
		dukglue_register_property(context, &MapSector::getSpecial, nullptr,			"special");
		dukglue_register_property(context, &MapSector::getTag, nullptr,				"id");
	}

	void registerMapThing()
	{
		dukglue_set_base_class<MapObject, MapThing>(context);

		dukglue_register_property(context, &MapThing::xPos, nullptr,		"x");
		dukglue_register_property(context, &MapThing::yPos, nullptr,		"y");
		dukglue_register_property(context, &MapThing::getType, nullptr,		"type");
		dukglue_register_property(context, &MapThing::getAngle, nullptr,	"angle");
		
		//dukglue_register_method(context, &MapThing::s_TypeInfo,	"typeInfo");
	}

	void registerMapObject()
	{
		dukglue_register_property(context, &MapObject::getIndex, nullptr, "index");

		dukglue_register_method(context, &MapObject::getTypeName,			"getTypeName");
		dukglue_register_method(context, &MapObject::hasProp,				"hasProperty");
		dukglue_register_method(context, &MapObject::boolProperty,			"boolProperty");
		dukglue_register_method(context, &MapObject::intProperty,			"intProperty");
		dukglue_register_method(context, &MapObject::floatProperty,			"floatProperty");
		dukglue_register_method(context, &MapObject::stringProperty,		"stringProperty");
		dukglue_register_method(context, &MapObject::s_SetBoolProperty,		"setBoolProperty");
		dukglue_register_method(context, &MapObject::s_SetIntProperty,		"setIntProperty");
		dukglue_register_method(context, &MapObject::s_SetFloatProperty,	"setFloatProperty");
		dukglue_register_method(context, &MapObject::s_SetStringProperty,	"setStringProperty");

		registerMapVertex();
		registerMapLine();
		registerMapSide();
		registerMapSector();
		registerMapThing();
	}

	void registerThingType()
	{
		using namespace Game;

		/*dukglue_register_property(context, &ThingType::name, nullptr,		"name");
		dukglue_register_property(context, &ThingType::group, nullptr,		"group");
		dukglue_register_property(context, &ThingType::radius, nullptr,		"radius");
		dukglue_register_property(context, &ThingType::height, nullptr,		"height");
		dukglue_register_property(context, &ThingType::scaleX, nullptr,		"scaleX");
		dukglue_register_property(context, &ThingType::scaleY, nullptr,		"scaleY");
		dukglue_register_property(context, &ThingType::angled, nullptr,		"angled");
		dukglue_register_property(context, &ThingType::hanging, nullptr,		"hanging");
		dukglue_register_property(context, &ThingType::fullbright, nullptr,	"fullbright");
		dukglue_register_property(context, &ThingType::decoration, nullptr,	"decoration");
		dukglue_register_property(context, &ThingType::solid, nullptr,		"solid");
		dukglue_register_property(context, &ThingType::needsTag, nullptr,		"tagged");
		dukglue_register_property(context, &ThingType::sprite, nullptr,		"sprite");
		dukglue_register_property(context, &ThingType::icon, nullptr,		"icon");
		dukglue_register_property(context, &ThingType::translation, nullptr,	"translation");
		dukglue_register_property(context, &ThingType::palette, nullptr,		"palette");*/
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
	registerSLADEMap();
	registerMapEditor();
	registerMapObject();
	registerThingType();

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
		script_context = duk_get_context(context, -1);
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
	SScriptDialog dlg(parent);
	dlg.ShowModal();
}


/*******************************************************************
 * CONSOLE COMMANDS
 *******************************************************************/

CONSOLE_COMMAND(exec_script, 1, false)
{
	if (!Scripting::runScript(args[0]))
		Log::message(Log::MessageType::Script, Scripting::getError());
}

CONSOLE_COMMAND(exec_script_file, 1, false)
{
	wxFile file;
	string script;
	if (file.Open(args[0]) && file.ReadAll(&script))
	{
		if (!Scripting::runScript(script))
			Log::message(Log::MessageType::Script, Scripting::getError());
	}
}

CONSOLE_COMMAND(script_test, 0, false)
{
	Scripting::openScriptTestDialog(Scripting::current_window);
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
			Log::message(
				Log::MessageType::Script,
				S_FMT("Error initialising scripting environment: %s", Scripting::error)
			);
	}
	else
		Log::message(Log::MessageType::Script, S_FMT("Resource script \"scripts/%s\" not found", args[0]));
}

CONSOLE_COMMAND(script_reset, 0, true)
{
	Scripting::close();
	Scripting::init();
}
