// Replace all map textures with AASHITTY and flats with SLIME09
// Requires a map opened in the map editor
// ----------------------------------------------------------------------------

// Get map sidedefs
var sides = slade.map.sidedefs;

// Loop through all sidedefs
for (var i = 0; i < sides.length; i++) {
    // Replace the middle texture if it is not blank (-)
    if (sides[i].textureMiddle != "-")
		sides[i].setStringProperty("texturemiddle", "AASHITTY");

    // Replace the upper texture if it is not blank (-)
    if (sides[i].textureTop != "-")
		sides[i].setStringProperty("texturetop", "AASHITTY");

    // Replace the lower texture if it is not blank (-)
    if (sides[i].textureBottom != "-")
		sides[i].setStringProperty("texturebottom", "AASHITTY");
}

// Get map sectors
var sectors = slade.map.sectors;

// Loop through all sectors
for (var i = 0; i < sectors.length; i++) {
    // Set ceiling texture
    sectors[i].setStringProperty("textureceiling", "SLIME09");

    // Set floor texture
    sectors[i].setStringProperty("texturefloor", "SLIME09");
}
