// Changes all monsters in a map to Archviles
// ----------------------------------------------------------------------------

// Get map things
var things = slade.map.things;

// Loop through all things
for (var a = 0; a < things.length; a++) {
    // Change to type 64 (Archvile) if the thing is in the 'Monsters' group
	// (or any sub-group)
	if (things[a].typeInfo.group.toLowerCase().indexOf("monsters") == 0)
		things[a].setIntProperty("type", 64);
}
