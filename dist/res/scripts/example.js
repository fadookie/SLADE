
function example1() {
    // Browse for archive file to open
    var path = slade.browseFile("Open Archive", slade.archiveManager.getArchiveExtensionsString(), "");
    if (path == "")
    {
        slade.logMessage("No archive selected");
        return;
    }

    // Open it
    var archive = slade.archiveManager.openFile(path);

    // Check it opened ok
    if (archive == null)
        slade.logMessage("Archive not opened: " + slade.globalError);
    else {
        slade.logMessage("Archive opened successfully");

        // List all entries
        archive.allEntries().forEach(function(entry) {
            slade.logMessage(entry.getPath(true) + " (" + entry.getSizeString() + ", " + entry.getTypeString() + ")");
        });

        // Prompt to close
        if (slade.promptYesNo("Close Archive", "Do you want to close the archive now?"))
            slade.archiveManager.closeArchive(archive);
    }
}

function selectExample() {
    var num = slade.promptNumber("SLADEScript Example", "Enter the number of the example to run (1-1)", 1, 1, 1);
    switch (num) {
        case 1: example1(); break;
        default: slade.messageBox("SLADEScript", "Invalid example number"); break;
    }
}

selectExample();
