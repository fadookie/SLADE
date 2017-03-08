#pragma once

namespace Scripting
{
	bool	init();
	bool	close();

	string	getError();
	bool	runScript(const string& script);
}
