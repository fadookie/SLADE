#pragma once

class wxWindow;
namespace Scripting
{
	bool	init();
	bool	close();

	string	getError();
	bool	runScript(const string& script);
	void	invalidate(void* object);

	// Testing
	void	openScriptTestDialog(wxWindow* parent);
}
