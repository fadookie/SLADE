#pragma once

#include "Game.h"
#include "ActionSpecial.h"
#include "ThingType.h"
#include "UDMFProperty.h"
#include "Utility/PropertyList/PropertyList.h"

class ParseTreeNode;
class ArchiveEntry;
class Archive;
class MapLine;
class MapThing;
class MapObject;

namespace Game
{
	// Feature Support
	enum class Feature
	{
		Boom,
		AnyMapName,
		MixTexFlats,
		TxTextures,
		LongNames,
	};
	enum class UDMFFeature
	{
		Slopes,				// Slope support
		FlatLighting,		// Flat lighting independent from sector lighting
		FlatPanning,		// Flat panning
		FlatRotation,		// Flat rotation
		FlatScaling,		// Flat scaling
		LineTransparency,	// Line transparency
		SectorColor,		// Sector colour
		SectorFog,			// Sector fog
		SideLighting,		// Sidedef lighting independent from sector lighting
		SideMidtexWrapping,	// Per-sidedef midtex wrapping
		SideScaling,		// Line scaling
		TextureScaling,		// Per-texture line scaling
		TextureOffsets,		// Per-texture offsets compared to per-sidedef
		ThingScaling,		// Per-thing scaling
		ThingRotation,		// Per-thing pitch and yaw rotation
	};

	// Tag types
	enum TagTypes
	{
		AS_TT_NO = 0,
		AS_TT_SECTOR,
		AS_TT_LINE,
		AS_TT_THING,
		AS_TT_SECTOR_BACK,
		AS_TT_SECTOR_OR_BACK,
		AS_TT_SECTOR_AND_BACK,

		// Special handling for that one
		AS_TT_LINEID,
		AS_TT_LINEID_HI5,

		// Some more specific types
		AS_TT_1THING_2SECTOR,					// most ZDoom teleporters work like this
		AS_TT_1THING_3SECTOR,					// Teleport_NoFog & Thing_Destroy
		AS_TT_1THING_2THING,					// TeleportOther, NoiseAlert, Thing_Move, Thing_SetGoal
		AS_TT_1THING_4THING,					// Thing_ProjectileIntercept, Thing_ProjectileAimed
		AS_TT_1THING_2THING_3THING,				// TeleportGroup
		AS_TT_1SECTOR_2THING_3THING_5THING,		// TeleportInSector
		AS_TT_1LINEID_2LINE,					// Teleport_Line
		AS_TT_LINE_NEGATIVE,					// Scroll_Texture_Both
		AS_TT_4THING,							// ThrustThing
		AS_TT_5THING,							// Radius_Quake
		AS_TT_1LINE_2SECTOR,					// Sector_Attach3dMidtex
		AS_TT_1SECTOR_2SECTOR,					// Sector_SetLink
		AS_TT_1SECTOR_2SECTOR_3SECTOR_4SECTOR,	// Plane_Copy
		AS_TT_SECTOR_2IS3_LINE,					// Static_Init
		AS_TT_1SECTOR_2THING,					// PointPush_SetForce
	};

	struct tt_t
	{
		ThingType*	type;
		int			number;
		int			index;
		tt_t(ThingType* type = NULL) { this->type = type; index = 0; }

		bool operator< (const tt_t& right) const { return (index < right.index); }
		bool operator> (const tt_t& right) const { return (index > right.index); }
	};

	struct as_t
	{
		ActionSpecial*	special;
		int				number;
		int				index;
		as_t(ActionSpecial* special = NULL) { this->special = special; index = 0; }

		bool operator< (const as_t& right) const { return (index < right.index); }
		bool operator> (const as_t& right) const { return (index > right.index); }
	};

	struct udmfp_t
	{
		UDMFProperty*	property;
		int				index;
		udmfp_t(UDMFProperty* property = NULL) { this->property = property; index = 0; }

		bool operator< (const udmfp_t& right) const { return (index < right.index); }
		bool operator> (const udmfp_t& right) const { return (index > right.index); }
	};

	struct gc_mapinfo_t
	{
		string	mapname;
		string	sky1;
		string	sky2;
	};

	struct sectype_t
	{
		int		type;
		string	name;
		sectype_t() { type = -1; name = "Unknown"; }
		sectype_t(int type, string name) { this->type = type; this->name = name; }
	};

	WX_DECLARE_HASH_MAP(int, as_t, wxIntegerHash, wxIntegerEqual, ASpecialMap);
	WX_DECLARE_HASH_MAP(int, tt_t, wxIntegerHash, wxIntegerEqual, ThingTypeMap);
	WX_DECLARE_STRING_HASH_MAP(udmfp_t, UDMFPropMap);

	class Configuration
	{
	public:
		Configuration();
		~Configuration();

		void	setDefaults();
		string	currentGame() { return current_game; }
		string	currentPort() { return current_port; }
		bool	supportsSectorFlags() { return boom_sector_flag_start > 0; }
		string	udmfNamespace();
		string	skyFlat() { return sky_flat; }
		string	scriptLanguage() { return script_language; }
		int		lightLevelInterval();

		string			readConfigName(MemChunk& mc);
		unsigned		nMapNames() { return maps.size(); }
		string			mapName(unsigned index);
		gc_mapinfo_t	mapInfo(string mapname);

		// Feature Support
		bool	featureSupported(Feature feature) { return supported_features_[feature]; }
		bool	featureSupported(UDMFFeature feature) { return udmf_features_[feature]; }

		// Config #include handling
		void	buildConfig(string filename, string& out);
		void	buildConfig(ArchiveEntry* entry, string& out, bool use_res = true);

		// Configuration reading
		void	readActionSpecials(ParseTreeNode* node, ActionSpecial* group_defaults = NULL, Arg::SpecialMap* shared_args = NULL);
		void	readThingTypes(ParseTreeNode* node, ThingType* group_defaults = NULL);
		void	readUDMFProperties(ParseTreeNode* node, UDMFPropMap& plist);
		void	readGameSection(ParseTreeNode* node_game, bool port_section = false);
		bool	readConfiguration(string& cfg, string source = "", uint8_t format = MAP_UNKNOWN, bool ignore_game = false, bool clear = true);
		bool	openConfig(string game, string port = "", uint8_t format = MAP_UNKNOWN);
		//bool	openEmbeddedConfig(ArchiveEntry* entry);
		//bool	removeEmbeddedConfig(string name);

		// Action specials
		ActionSpecial*	actionSpecial(unsigned id);
		string			actionSpecialName(int special);
		vector<as_t>	allActionSpecials();

		// Thing types
		ThingType*		thingType(unsigned type);
		vector<tt_t>	allThingTypes();

		// Thing flags
		int		nThingFlags() { return flags_thing.size(); }
		string	thingFlag(unsigned flag_index);
		bool	thingFlagSet(unsigned flag_index, MapThing* thing);
		bool	thingFlagSet(string udmf_name, MapThing* thing, int map_format);
		bool	thingBasicFlagSet(string flag, MapThing* line, int map_format);
		string	thingFlagsString(int flags);
		void	setThingFlag(unsigned flag_index, MapThing* thing, bool set = true);
		void	setThingFlag(string udmf_name, MapThing* thing, int map_format, bool set = true);
		void	setThingBasicFlag(string flag, MapThing* line, int map_format, bool set = true);

		// DECORATE
		bool	parseDecorateDefs(Archive* archive);
		void	clearDecorateDefs();

		// Line flags
		int		nLineFlags() { return flags_line.size(); }
		string	lineFlag(unsigned flag_index);
		bool	lineFlagSet(unsigned flag_index, MapLine* line);
		bool	lineFlagSet(string udmf_name, MapLine* line, int map_format);
		bool	lineBasicFlagSet(string flag, MapLine* line, int map_format);
		string	lineFlagsString(MapLine* line);
		void	setLineFlag(unsigned flag_index, MapLine* line, bool set = true);
		void	setLineFlag(string udmf_name, MapLine* line, int map_format, bool set = true);
		void	setLineBasicFlag(string flag, MapLine* line, int map_format, bool set = true);

		// Line action (SPAC) triggers
		string			spacTriggerString(MapLine* line, int map_format);
		int				spacTriggerIndexHexen(MapLine* line);
		wxArrayString	allSpacTriggers();
		void			setLineSpacTrigger(unsigned trigger_index, MapLine* line);
		static int		parseTagged(ParseTreeNode* tagged);

		// UDMF properties
		UDMFProperty*	getUDMFProperty(string name, int type);
		vector<udmfp_t>	allUDMFProperties(int type);
		void			cleanObjectUDMFProps(MapObject* object);

		// Sector types
		string				sectorTypeName(int type);
		vector<sectype_t>	allSectorTypes() { return sector_types; }
		int					sectorTypeByName(string name);
		int					baseSectorType(int type);
		int					sectorBoomDamage(int type);
		bool				sectorBoomSecret(int type);
		bool				sectorBoomFriction(int type);
		bool				sectorBoomPushPull(int type);
		int					boomSectorType(int base, int damage, bool secret, bool friction, bool pushpull);

		// Defaults
		string	getDefaultString(int type, string property);
		int		getDefaultInt(int type, string property);
		double	getDefaultFloat(int type, string property);
		bool	getDefaultBool(int type, string property);
		void	applyDefaults(MapObject* object, bool udmf = false);

		// Misc
		void	setLightLevelInterval(int interval);
		int		upLightLevel(int light_level);
		int		downLightLevel(int light_level);

		// Testing
		void	dumpActionSpecials();
		void	dumpThingTypes();
		void	dumpValidMapNames();
		void	dumpUDMFProperties();

	private:
		string				current_game;		// Current game name
		string				current_port;		// Current port name (empty if none)
		bool				map_formats[4];		// Supported map formats
		string				udmf_namespace;		// Namespace to use for UDMF
		int 				boom_sector_flag_start;  // Beginning of Boom sector flags
		ASpecialMap			action_specials;	// Action specials
		ActionSpecial		as_unknown;			// Default action special
		ActionSpecial		as_generalized_s;	// Dummy for Boom generalized switched specials
		ActionSpecial		as_generalized_m;	// Dummy for Boom generalized manual specials
		ThingTypeMap		thing_types;		// Thing types
		vector<ThingType*>	tt_group_defaults;	// Thing type group defaults
		ThingType			ttype_unknown;		// Default thing type
		string				sky_flat;			// Sky flat for 3d mode
		string				script_language;	// Scripting language (should be extended to allow multiple)
		vector<int>			light_levels;		// Light levels for up/down light in editor

		// Flags
		struct flag_t
		{
			int		flag;
			string	name;
			string	udmf;
			flag_t() { flag = 0; name = ""; udmf = ""; }
			flag_t(int flag, string name, string udmf = "") { this->flag = flag; this->name = name; this->udmf = udmf; }
		};
		vector<flag_t>	flags_thing;
		vector<flag_t>	flags_line;
		vector<flag_t>	triggers_line;

		// Sector types
		vector<sectype_t>	sector_types;

		// Map info
		vector<gc_mapinfo_t>	maps;

		// UDMF properties
		UDMFPropMap	udmf_vertex_props;
		UDMFPropMap	udmf_linedef_props;
		UDMFPropMap	udmf_sidedef_props;
		UDMFPropMap	udmf_sector_props;
		UDMFPropMap	udmf_thing_props;

		// Defaults
		PropertyList	defaults_line;
		PropertyList	defaults_line_udmf;
		PropertyList	defaults_side;
		PropertyList	defaults_side_udmf;
		PropertyList	defaults_sector;
		PropertyList	defaults_sector_udmf;
		PropertyList	defaults_thing;
		PropertyList	defaults_thing_udmf;

		// Feature Support
		std::map<Feature, bool>		supported_features_;
		std::map<UDMFFeature, bool>	udmf_features_;
	};
}
