#pragma once

class wxWindow;
namespace Scripting
{
	bool	init();
	bool	close();

	string	getError();
	bool	runScript(const string& script, bool use_global_context = false);
	void	invalidate(void* object);
	void	setCurrentWindow(wxWindow* window);

	// Buffers
	bool	pushBuffer(MemChunk& mc);
	bool	pushBuffer(const uint8_t* data, unsigned size);

	// Testing
	void	openScriptTestDialog(wxWindow* parent);
}
