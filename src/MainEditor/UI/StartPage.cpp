
#include "Main.h"
#include "StartPage.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "Utility/Tokenizer.h"
#include "Graphics/Icons.h"
#include "General/SAction.h"


SStartPage::SStartPage(wxWindow* parent) : wxPanel(parent, -1)
{
	wxPanel::SetName("startpage");

	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);
}

void SStartPage::init()
{
	// wxWebView
#ifdef USE_WEBVIEW_STARTPAGE
	html_startpage_ = wxWebView::New(
		this,
		-1,
		wxEmptyString,
		wxDefaultPosition,
		wxDefaultSize,
		wxWebViewBackendDefault,
		wxBORDER_NONE
	);
	html_startpage_->SetZoomType(
		App::platform() == App::MacOS ? wxWEBVIEW_ZOOM_TYPE_TEXT : wxWEBVIEW_ZOOM_TYPE_LAYOUT
	);

	// wxHtmlWindow
#else
	html_startpage_ = new wxHtmlWindow(
		this,
		-1,
		wxDefaultPosition,
		wxDefaultSize,
		wxHW_SCROLLBAR_NEVER,
		"startpage"
	);
#endif

	// Add to sizer
	GetSizer()->Add(html_startpage_, 1, wxEXPAND);

	// Bind events
#ifdef USE_WEBVIEW_STARTPAGE
	html_startpage_->Bind(wxEVT_WEBVIEW_NAVIGATING, &SStartPage::onHTMLLinkClicked, this);

	html_startpage_->Bind(wxEVT_WEBVIEW_ERROR, [&](wxWebViewEvent& e)
	{
		Log::error(e.GetString());
	});

	html_startpage_->Bind(wxEVT_WEBVIEW_LOADED, [&](wxWebViewEvent& e)
	{
		html_startpage_->Reload();
	});

#else
	html_startpage_->Bind(wxEVT_COMMAND_HTML_LINK_CLICKED, &SStartPage::onHTMLLinkClicked, this);
#endif

	// Get data used to build the page
	auto res_archive = theArchiveManager->programResourceArchive();
	if (res_archive)
	{
		entry_base_html_ = res_archive->entryAtPath(
			App::useWebView() ? "html/startpage.htm" : "html/startpage_basic.htm"
		);
		entry_css_ = res_archive->entryAtPath("html/style.css");
		entry_export_.push_back(res_archive->entryAtPath("logo.png"));
		entry_export_.push_back(res_archive->entryAtPath("icons/entry_list/Rounded/archive.png"));
		entry_export_.push_back(res_archive->entryAtPath("icons/entry_list/Rounded/wad.png"));
		entry_export_.push_back(res_archive->entryAtPath("icons/entry_list/Rounded/zip.png"));
		entry_export_.push_back(res_archive->entryAtPath("icons/entry_list/Rounded/folder.png"));

		// Load tips
		auto entry_tips = res_archive->entryAtPath("tips.txt");
		if (entry_tips)
		{
			Tokenizer tz;
			tz.openMem((const char*)entry_tips->getData(), entry_tips->getSize(), entry_tips->getName());
			while (!tz.atEnd())
				tips_.push_back(tz.getToken());
		}
	}
}

#ifdef USE_WEBVIEW_STARTPAGE

void SStartPage::load(bool new_tip)
{
	//html_startpage_->SetPage("<html><head></head><body>Loading...</body></html>", wxEmptyString);

	// Can't do anything without html entry
	if (!entry_base_html_)
	{
		LOG_MESSAGE(1, "No start page resource found");
		html_startpage_->SetPage(
			"<html><head><title>SLADE</title></head><body><center><h1>"
			"Something is wrong with slade.pk3 :(</h1><center></body></html>",
			wxEmptyString
		);
		return;
	}

	// Get html as string
	string html = wxString::FromAscii((const char*)(entry_base_html_->getData()), entry_base_html_->getSize());

	// Read css
	string css;
	if (entry_css_)
		css = wxString::FromAscii((const char*)(entry_css_->getData()), entry_css_->getSize());

	// Generate tip of the day string
	string tip;
	if (tips_.size() < 2) // Needs at least two choices or it's kinda pointless.
		tip = "Did you know? Something is wrong with the tips.txt file in your slade.pk3.";
	else
	{
		int tipindex = last_tip_index_;
		if (new_tip || last_tip_index_ <= 0)
		{
			// Don't show same tip twice in a row
			do { tipindex = 1 + (rand() % tips_.size()); } while (tipindex == last_tip_index_);
		}

		last_tip_index_ = tipindex;
		tip = tips_[tipindex];
	}

	// Generate recent files string
	string recent;
	if (theArchiveManager->numRecentFiles() > 0)
	{
		for (unsigned a = 0; a < 12; a++)
		{
			if (a >= theArchiveManager->numRecentFiles())
				break;	// No more recent files

			// Determine icon
			string fn = theArchiveManager->recentFile(a);
			string icon = "archive";
			if (fn.EndsWith(".wad"))
				icon = "wad";
			else if (fn.EndsWith(".zip") || fn.EndsWith(".pk3") || fn.EndsWith(".pke"))
				icon = "zip";
			else if (wxDirExists(fn))
				icon = "folder";

			// Add recent file row
			recent += S_FMT(
				"<div class=\"recent\">"
				"<img src=\"%s.png\" class=\"recent\" />"
				"<a class=\"recent\" href=\"recent://%d\">%s</a>"
				"</div>",
				icon,
				a,
				fn
			);
		}
	}
	else
		recent = "No recently opened files";

	// Insert css, tip and recent files into html
	html.Replace("/*#css#*/", css);
	html.Replace("#recent#", recent);
	html.Replace("#totd#", tip);

	// Write html and images to temp folder
	for (unsigned a = 0; a < entry_export_.size(); a++)
		entry_export_[a]->exportFile(App::path(entry_export_[a]->getName(), App::Dir::Temp));
	string html_file = App::path("startpage.htm", App::Dir::Temp);
	wxFile outfile(html_file, wxFile::write);
	outfile.Write(html);
	outfile.Close();

	if (App::platform() == App::Linux)
		html_file = "file://" + html_file;

	// Load page
	html_startpage_->ClearHistory();
	html_startpage_->LoadURL(html_file);

	if (App::platform() == App::Windows)
		html_startpage_->Reload();
}

#else

void SStartPage::load(bool new_tip)
{
	// Get relevant resource entries
	Archive* res_archive = theArchiveManager->programResourceArchive();
	if (!res_archive)
		return;
	ArchiveEntry* entry_html = res_archive->entryAtPath("html/startpage_basic.htm");
	ArchiveEntry* entry_logo = res_archive->entryAtPath("logo.png");
	ArchiveEntry* entry_tips = res_archive->entryAtPath("tips.txt");

	// Can't do anything without html entry
	if (!entry_html)
	{
		html_startpage_->SetPage("<html><head><title>SLADE</title></head><body><center><h1>Something is wrong with slade.pk3 :(</h1><center></body></html>");
		return;
	}

	// Get html as string
	string html = wxString::FromAscii((const char*)(entry_html->getData()), entry_html->getSize());

	// Generate tip of the day string
	string tip = "It seems tips.txt is missing from your slade.pk3";
	if (entry_tips)
	{
		Tokenizer tz;
		tz.openMem((const char*)entry_tips->getData(), entry_tips->getSize(), entry_tips->getName());
		srand(wxGetLocalTime());
		int numtips = tz.getInteger();
		if (numtips < 2) // Needs at least two choices or it's kinda pointless.
			tip = "Did you know? Something is wrong with the tips.txt file in your slade.pk3.";
		else
		{
			int tipindex = 0;
			// Don't show same tip twice in a row
			do { tipindex = 1 + (rand() % numtips); } while (tipindex == last_tip_index_);
			last_tip_index_ = tipindex;
			for (int a = 0; a < tipindex; a++)
				tip = tz.getToken();
		}
	}

	// Generate recent files string
	string recent;
	for (unsigned a = 0; a < 12; a++)
	{
		if (a >= theArchiveManager->numRecentFiles())
			break;	// No more recent files

					// Add line break if needed
		if (a > 0) recent += "<br/>\n";

		// Add recent file link
		recent += S_FMT("<a href=\"recent://%d\">%s</a>", a, theArchiveManager->recentFile(a));
	}

	// Insert tip and recent files into html
	html.Replace("#recent#", recent);
	html.Replace("#totd#", tip);

	// Write html and images to temp folder
	if (entry_logo) entry_logo->exportFile(App::path("logo.png", App::Dir::Temp));
	string html_file = App::path("startpage_basic.htm", App::Dir::Temp);
	wxFile outfile(html_file, wxFile::write);
	outfile.Write(html);
	outfile.Close();

	// Load page
	html_startpage_->LoadPage(html_file);

	// Clean up
	wxRemoveFile(html_file);
	wxRemoveFile(App::path("logo.png", App::Dir::Temp));
}

#endif

void SStartPage::refresh()
{
#ifdef USE_WEBVIEW_STARTPAGE
	html_startpage_->Reload();
#endif
}


#ifdef USE_WEBVIEW_STARTPAGE

/* SStartPage::onHTMLLinkClicked
 * Called when a link is clicked on the HTML Window, so that
 * external (http) links are opened in the default browser
 *******************************************************************/
void SStartPage::onHTMLLinkClicked(wxEvent& e)
{
	wxWebViewEvent& ev = (wxWebViewEvent&)e;
	string href = ev.GetURL();

#ifdef __WXGTK__
	if (!href.EndsWith("startpage.htm"))
		href.Replace("file://", "");
#endif

	//LOG_MESSAGE(2, "URL %s", href);

	if (href.EndsWith("/"))
		href.RemoveLast(1);

	if (href.StartsWith("http://"))
	{
		wxLaunchDefaultBrowser(ev.GetURL());
		ev.Veto();
	}
	else if (href.StartsWith("recent://"))
	{
		// Recent file
		string rs = href.Mid(9);
		unsigned long index = 0;
		rs.ToULong(&index);
		SActionHandler::setWxIdOffset(index);
		SActionHandler::doAction("aman_recent");
		load();
		html_startpage_->Reload();
	}
	else if (href.StartsWith("action://"))
	{
		// Action
		if (href.EndsWith("open"))
			SActionHandler::doAction("aman_open");
		else if (href.EndsWith("newwad"))
			SActionHandler::doAction("aman_newwad");
		else if (href.EndsWith("newzip"))
			SActionHandler::doAction("aman_newzip");
		else if (href.EndsWith("newmap"))
		{
			SActionHandler::doAction("aman_newmap");
			return;
		}
		else if (href.EndsWith("reloadstartpage"))
			load();
		html_startpage_->Reload();
	}
	else if (wxFileExists(href))
	{
		// Navigating to file, open it
		string page = App::path("startpage.htm", App::Dir::Temp);
		if (wxFileName(href).GetLongPath() != wxFileName(page).GetLongPath())
		{
			theArchiveManager->openArchive(href);
			ev.Veto();
		}
	}
	else if (wxDirExists(href))
	{
		// Navigating to folder, open it
		theArchiveManager->openDirArchive(href);
		ev.Veto();
	}
}

#else

/* SStartPage::onHTMLLinkClicked
 * Called when a link is clicked on the HTML Window, so that
 * external (http) links are opened in the default browser
 *******************************************************************/
void SStartPage::onHTMLLinkClicked(wxEvent& e)
{
	wxHtmlLinkEvent& ev = (wxHtmlLinkEvent&)e;
	string href = ev.GetLinkInfo().GetHref();

	if (href.StartsWith("http://"))
		wxLaunchDefaultBrowser(ev.GetLinkInfo().GetHref());
	else if (href.StartsWith("recent://"))
	{
		// Recent file
		string rs = href.Mid(9);
		unsigned long index = 0;
		rs.ToULong(&index);
		SActionHandler::doAction("aman_recent", index);
		load();
	}
	else if (href.StartsWith("action://"))
	{
		// Action
		if (href.EndsWith("open"))
			SActionHandler::doAction("aman_open");
		else if (href.EndsWith("newwad"))
			SActionHandler::doAction("aman_newwad");
		else if (href.EndsWith("newzip"))
			SActionHandler::doAction("aman_newzip");
		else if (href.EndsWith("newmap"))
			SActionHandler::doAction("aman_newmap");
		else if (href.EndsWith("reloadstartpage"))
			load();
	}
	else
		html_startpage_->OnLinkClicked(ev.GetLinkInfo());
}

#endif
