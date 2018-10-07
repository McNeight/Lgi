/**
	\file
	\author Matthew Allen <fret@memecode.com>
	\brief Simple single window wrapper around the GHtml2 control
 */
#ifndef _GBROWSER_H_
#define _GBROWSER_H_


class GBrowser : public GWindow
{
	class GBrowserPriv *d;

public:
	class GBrowserEvents
	{
	public:
		virtual ~GBrowserEvents() {}

		virtual bool OnSearch(GBrowser *br, char *txt) { return false; }
	};
	
	GBrowser(GViewI *owner, const char *Title, char *Uri = 0);
	~GBrowser();

	void SetEvents(GBrowserEvents *Events);
	bool SetUri(const char *Uri = 0);
	bool SetHtml(char *Html);

	void OnPosChange();
	int OnNotify(GViewI *c, int f);
	GMessage::Result OnEvent(GMessage *m);
};

#endif