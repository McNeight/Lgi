/// \file
/// \author Matthew Allen
#ifndef _GHTML_H
#define _GHTML_H

#include "GDocView.h"
#include "GHtmlCommon.h"
#include "GHtmlParser.h"
#include "GCapabilities.h"

namespace Html1
{

class GTag;
class GFontCache;

/// A lightwight scripting safe HTML control. It has limited CSS support, renders
/// most tables, even when nested. You can provide support for loading external
/// images by implementing the GDocumentEnv::GetImageUri method of the 
/// GDocumentEnv interface and passing it into GHtml2::SetEnv.
/// All attempts to open URL's are passed into GDocumentEnv::OnNavigate method of the
/// environment if set. Likewise any content inside active scripting tags, e.g. &lt;? ?&gt;
/// will be striped out of the document and passed to GDocumentEnv::OnDynamicContent, which
/// should return the relevant HTML that the script resolves to. A reasonable default
/// implementation of the GDocumentEnv interface is the GDefaultDocumentEnv object.
///
/// You can set the content of the control through the GHtml2::Name method.
///
/// Retreive any selected content through GHtml2::GetSelection.
class GHtml :
	public GDocView,
	public ResObject,
	public GHtmlParser,
	public GCapabilityClient
{
	friend class GTag;
	friend class GFlowRegion;

	class GHtmlPrivate *d;

protected:	
	// Data
	GFontCache			*FontCache;
	GTag				*Tag;				// Tree root
	GTag				*Cursor;			// Cursor location..
	GTag				*Selection;			// Edge of selection or NULL
	char				IsHtml;
	int					ViewWidth;
	GToolTip			Tip;
	GCss::Store			CssStore;
	GHashTbl<const char*, bool> CssHref;
	
	// Display
	GAutoPtr<GSurface>	MemDC;

	// This lock is separate from the window lock to avoid deadlocks.
	struct GJobSem : public LMutex
	{
    	// Data that has to be accessed under Lock
	    GArray<GDocumentEnv::LoadJob*> Jobs;
	    GJobSem() : LMutex("GJobSem") {}
	} JobSem;

	// Methods
	void _New();
	void _Delete();
	GFont *DefFont();
	void CloseTag(GTag *t);
	void ParseDocument(const char *Doc);
	void OnAddStyle(const char *MimeType, const char *Styles);
	int ScrollY();
	void SetCursorVis(bool b);
	bool GetCursorVis();
	GRect *GetCursorPos();
	bool IsCursorFirst();
	bool CompareTagPos(GTag *a, ssize_t AIdx, GTag *b, ssize_t BIdx);
	int GetTagDepth(GTag *Tag);
	GTag *PrevTag(GTag *t);
	GTag *NextTag(GTag *t);
	GTag *GetLastChild(GTag *t);

public:
	GHtml(int Id, int x, int y, int cx, int cy, GDocumentEnv *system = 0);
	~GHtml();

	// Html
	const char *GetClass() { return "GHtml"; }
	bool GetFormattedContent(const char *MimeType, GString &Out, GArray<GDocView::ContentMedia> *Media = 0);

	/// Get the tag at an x,y location
	GTag *GetTagByPos(	int x, int y,
						ssize_t *Index,
						GdcPt2 *LocalCoords = NULL,
						bool DebugLog = false);
	/// Layout content and return size.
	GdcPt2 Layout(bool ForceLayout = false);

	// Options
	bool GetLinkDoubleClick();
	void SetLinkDoubleClick(bool b);
	void SetLoadImages(bool i);
	bool GetEmoji();
	void SetEmoji(bool i);

	// GDocView
	bool SetVariant(const char *Name, GVariant &Value, char *Array = 0);
	
	/// Copy the selection to the clipboard
	bool Copy();
	/// Returns true if there is a selection
	bool HasSelection();
	/// Unselect all the text in the control
	void UnSelectAll();
	/// Select all the text in the control (not impl)
	void SelectAll();
	/// Return the selection in a dynamically allocated string
	char *GetSelection();
	
	// Prop

	// Window

	/// Sets the HTML content of the control
	bool Name(const char *s);
	/// Returns the HTML content
	char *Name();
	/// Sets the HTML content of the control
	bool NameW(const char16 *s);
	/// Returns the HTML content
	char16 *NameW();

	// Impl
	void OnPaint(GSurface *pDC);
	void OnMouseClick(GMouse &m);
	void OnMouseMove(GMouse &m);
	LgiCursor GetCursor(int x, int y);
	bool OnMouseWheel(double Lines);
	bool OnKey(GKey &k);
	int OnNotify(GViewI *c, int f);
	void OnPosChange();
	void OnPulse();
	GMessage::Result OnEvent(GMessage *Msg);
	const char *GetMimeType() { return "text/html"; }
	void OnContent(GDocumentEnv::LoadJob *Res);
	bool GotoAnchor(char *Name);
	GHtmlElement *CreateElement(GHtmlElement *Parent);
	bool EvaluateCondition(const char *Cond);
	bool GetVariant(const char *Name, GVariant &Value, char *Array = NULL);

	// Javascript handlers
	GDom *getElementById(char *Id);

	// Events
	bool OnFind(class GFindReplaceCommon *Params);
	virtual bool OnSubmitForm(GTag *Form);
	virtual void OnCursorChanged() {}
	virtual void OnLoad();
	virtual bool OnContextMenuCreate(struct GTagHit &Hit, GSubMenu &RClick) { return true; }
	virtual void OnContextMenuCommand(struct GTagHit &Hit, int Cmd) {}
};

}
#endif
