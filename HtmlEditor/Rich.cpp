#include "Lgi.h"
#include "GTextView3.h"
#include "GTree.h"
#include "GTabView.h"
#include "GBox.h"
#include "resource.h"
#include "LgiSpellCheck.h"
#include "GDisplayString.h"
#include "GButton.h"
#include "IHttp.h"
#include "GOptionsFile.h"

#if 1
#include "GRichTextEdit.h"
typedef GRichTextEdit EditCtrl;
#else
#include "GHtmlEdit.h"
typedef GHtmlEdit EditCtrl;
#endif

enum Ctrls
{
	IDC_EDITOR = 100,
	IDC_HTML,
	IDC_TABS,
	IDC_TREE,
	IDC_TO_HTML,
	IDC_TO_NODES,
	IDC_VIEW_IN_BROWSER,
	IDC_SAVE_FILE,
	IDC_SAVE,
	IDC_EXIT,
	IDC_INSTALL
};

enum Messages
{
	M_INSTALL = M_USER + 200,
};

const char *AppName = "HtmlEdit";
#define LOAD_DOC 1
#define SrcFileName	"Reply4.html"

#if 0

char Src[] =
	"This is a test. A longer string for the purpose of inter-line cursor testing.<br>\n"
	"A longer string for the <b>purpose</b> of inter-line cursor testing.<br>\n"
	"--<br>"
	"<a href=\"http://web/~matthew\">Matthew Allen</a>";

#else

char Src[] =	
	"<html>\n"
	"<body style=\"font-size: 9pt;\">Hi Suzy,<br/>\n"
	"<br/>\n"
	"The aircon is a little warm in our area of the woods. There seems to be nothing coming out of the 2nd AC unit.<br/>\n"
	"<br/>\n"
	"Regards<br/>\n"
	"--<br/>\n"
	"<a href=\"http://web/~matthew\">Matthew Allen</a>\n"
	"</body>\n"
	"</html>";

#endif

class GCapabilityInstallTarget : public GCapabilityTarget
{
public:
	virtual void StartInstall(CapsHash *Caps) = 0;
};

class CapsBar : public GView
{
	GCapabilityInstallTarget *App;
	GCapabilityTarget::CapsHash *Caps;
	GButton *Ok, *Install;

public:
	CapsBar(GCapabilityInstallTarget *Parent, GCapabilityTarget::CapsHash *caps)
	{
		App = Parent;
		Caps = caps;
		Ok = new GButton(IDOK, 0, 0, -1, -1, "Ok");
		Install = new GButton(IDC_INSTALL, 0, 0, -1, -1, "Install");
	}

	bool Pour(GRegion &r)
	{
		GRect *rc = FindLargestEdge(r, GV_EDGE_TOP);
		if (!rc)
			return false;

		GRect p = *rc;
		p.y2 = p.y1 + SysFont->GetHeight() + 11;
		SetPos(p);
		return true;	
	}

	int OnNotify(GViewI *Ctrl, int Flags)
	{
		switch (Ctrl->GetId())
		{
			case IDOK:
			{
				App->OnCloseInstaller();
				break;
			}
			case IDC_INSTALL:
			{
				App->StartInstall(Caps);
				break;
			}
		}
		return 0;
	}

	void OnPaint(GSurface *pDC)
	{
		pDC->Colour(GColour::Red);
		pDC->Rectangle();

		const char *k;
		SysFont->Colour(GColour::White, GColour::Red);
		SysFont->Transparent(true);

		GString s = "Missing components: ";
		for (bool b = Caps->First(&k); b; b = Caps->Next(&k))
		{
			s += k;
			s += " ";
		}

		GDisplayString d(SysFont, s);
		d.Draw(pDC, 6, 6);

		if (Ok && Install)
		{
			int x = GetClient().x2 - 6;
			if (!Ok->IsAttached())
				Ok->Attach(this);
			GRect r;
			r.Set(0, 0, Ok->X()-1, Ok->Y()-1);
			r.Offset(x - r.X(), (Y() - r.Y()) >> 1);
			Ok->SetPos(r);			

			if (!Install->IsAttached())
				Install->Attach(this);
			r.Set(0, 0, Install->X()-1, Install->Y()-1);
			r.Offset(Ok->GetPos().x1 - 6 - r.X(), (Y() - r.Y()) >> 1);
			Install->SetPos(r);
		}
			
	}
};

class InstallThread : public GEventTargetThread
{
	int AppHnd;

public:
	InstallThread(int appHnd) : GEventTargetThread("InstallThread")
	{
		AppHnd = appHnd;
	}

	GMessage::Result OnEvent(GMessage *m)
	{
		switch (m->Msg())
		{
			case M_INSTALL:
			{
				GAutoPtr<GString> Component((GString*)m->A());
				const char *Base = "http://memecode.com/components/lookup.php?app=Scribe&wordsize=%i&component=%s&os=win64&version=2.2.0";
				GString s;
				s.Printf(Base, sizeof(NativeInt)*8, Component->Get());
				#if _MSC_VER == _MSC_VER_VS2013
				s += "&tags=vc12";
				#elif _MSC_VER == _MSC_VER_VS2008
				s += "&tags=vc9";
				#endif

				GMemStream o(1024);
				GString err;
				int Installed = 0;
				if (!LgiGetUri(&o, &err, s))
				{
					LgiTrace("%s:%i - Get URI failed.\n", _FL);
					break;
				}

				GXmlTree t;
				GXmlTag r;
				o.SetPos(0);
				if (!t.Read(&r, &o))
				{
					LgiTrace("%s:%i - Bad XML.\n", _FL);
					break;
				}

				if (!r.IsTag("components"))
				{
					LgiTrace("%s:%i - No components tag.\n", _FL);
					break;
				}

				for (GXmlTag *c = r.Children.First(); c; c = r.Children.Next())
				{
					if (c->IsTag("file"))
					{
						// int Bytes = c->GetAsInt("size");
						const char *Link = c->GetContent();
						GMemStream File(1024);
						if (LgiGetUri(&File, &err, Link))
						{
							char p[MAX_PATH];
							LgiGetExeFile(p, sizeof(p));
							LgiTrimDir(p);
							LgiMakePath(p, sizeof(p), p, LgiGetLeaf(Link));

							GFile f;
							if (f.Open(p, O_WRITE))
							{
								f.SetSize(0);
								if (f.Write(File.GetBasePtr(), File.GetSize()) == File.GetSize())
								{
									GAutoPtr<GString> comp(new GString(*Component));
									PostObject(AppHnd, M_INSTALL, comp);
									Installed++;
								}
								else
									LgiTrace("%s:%i - Couldn't write to '%p'.\n", _FL, p);
							}
							else LgiTrace("%s:%i - Can't open '%s' for writing.\n", _FL, p);
						}
						else LgiTrace("%s:%i - Link download failed.\n", _FL);
					}
				}

				if (Installed == 0)
				{
					LgiTrace("%s:%i - No installed components from URI:\n%s\n", _FL, s.Get());
				}
				break;
			}
		}

		return 0;
	}
};

class App : public GWindow, public GCapabilityInstallTarget
{
	GBox *Split;
	GTextView3 *Txt;
	GTabView *Tabs;
	GTree *Tree;
	uint64 LastChange;

	EditCtrl *Edit;

	GAutoPtr<GSpellCheck> Speller;
	CapsBar *Bar;
	GCapabilityTarget::CapsHash Caps;
	GAutoPtr<GEventTargetThread> Installer;
	GOptionsFile Options;

public:
	App() : Options(GOptionsFile::PortableMode, AppName)
	{
		LastChange = 0;
		Edit = 0;
		Txt = 0;
		Bar = NULL;
		Tabs = NULL;
		Tree = NULL;
		Name("Rich Text Testbed");

		if (!Options.SerializeFile(false) ||
			!SerializeState(&Options, "WndState", true))
		{
			GRect r(0, 0, 1200, 800);
			SetPos(r);
			MoveToCenter();
		}

		SetQuitOnClose(true);
		#ifdef WIN32
		SetIcon((const char*)IDI_APP);
		#endif
		
		#if defined(WINDOWS)
		Speller = CreateWindowsSpellCheck();
		#elif defined(MAC)
		Speller = CreateAppleSpellCheck();
		#elif !defined(LINUX)
		Speller = CreateAspellObject();
		#endif
		
		if (Attach(0))
		{
			Menu = new GMenu;
			if (Menu)
			{
				Menu->Attach(this);

				GSubMenu *s = Menu->AppendSub("&File");
				s->AppendItem("To &HTML", IDC_TO_HTML, true, -1, "F5");
				s->AppendItem("To &Nodes", IDC_TO_NODES, true, -1, "F6");
				s->AppendItem("View In &Browser", IDC_VIEW_IN_BROWSER, true, -1, "F7");
				s->AppendItem("&Save", IDC_SAVE, true, -1, "Ctrl+S");
				s->AppendItem("Save &As", IDC_SAVE_FILE, true, -1, "Ctrl+Shift+S");
				s->AppendSeparator();
				s->AppendItem("E&xit", IDC_EXIT, true, -1, "Ctrl+W");
			}

			AddView(Split = new GBox);
			if (Split)
			{
				Split->AddView(Edit = new EditCtrl(IDC_EDITOR));
				if (Edit)
				{
					if (Speller)
					{
						GVariant v;
						Edit->SetValue("SpellCheckLanguage", v = "English");
						Edit->SetValue("SpellCheckDictionary", v = "AU");
						Edit->SetSpellCheck(Speller);
					}
					Edit->Sunken(true);
					Edit->SetId(IDC_EDITOR);
					Edit->Register(this);
					// Edit->Name("<span style='color:#800;'>The rich editor control is not functional in this build.</span><b>This is some bold</b>");

					#if LOAD_DOC
					#ifndef SrcFileName
					Edit->Name(Src);
					#else
					char p[MAX_PATH];
					LgiGetSystemPath(LSP_APP_INSTALL, p, sizeof(p));
					LgiMakePath(p, sizeof(p), p, "Test");
					LgiMakePath(p, sizeof(p), p, SrcFileName);
					GAutoString html(ReadTextFile(p));
					if (html)
						Edit->Name(html);
					#endif
					#endif
				}

				Split->AddView(Tabs = new GTabView(IDC_TABS));
				if (Tabs)
				{
					// Tabs->Debug();
					
					GTabPage *p = Tabs->Append("Html Output");
					if (p)
					{
						p->AddView(Txt = new GTextView3(IDC_HTML, 0, 0, 100, 100));
						Txt->SetPourLargest(true);
					}
					
					p = Tabs->Append("Node View");
					if (p)
					{
						p->AddView(Tree = new GTree(IDC_TREE, 0, 0, 100, 100));
						Tree->SetPourLargest(true);
					}
				}

				Split->Value(GetClient().X()/2);
			}

			AttachChildren();
			PourAll();
			Visible(true);

			if (Edit)
				Edit->Focus(true);
		}
	}

	~App()
	{
		SerializeState(&Options, "WndState", false);
		Options.SerializeFile(true);
		Installer.Reset();
	}

	bool NeedsCapability(const char *Name, const char *Param = NULL)
	{
		if (Caps.Find(Name))
			return true;

		Caps.Add(Name, true);

		if (!Bar)
		{
			Bar = new CapsBar(this, &Caps);
			if (Bar)
			{
				AddView(Bar, 0);
				AttachChildren();
				
				PourAll();
				Invalidate();
			}
		}
		return true;
	}
	
	void StartInstall(CapsHash *Caps)
	{
		if (!Installer)
			Installer.Reset(new InstallThread(AddDispatch()));
		if (Installer)
		{
			const char *c;
			for (bool b = Caps->First(&c); b; b = Caps->Next(&c))
			{
				GAutoPtr<GString> s(new GString(c));
				Installer->PostObject(Installer->GetHandle(), M_INSTALL, s);
			}
		}
	}

	void OnInstall(CapsHash *Caps, bool Status)
	{
		DeleteObj(Bar);
		if (Edit && Caps && Status)
		{
			const char *k;
			for (bool b = Caps->First(&k); b; b = Caps->Next(&k))
				Edit->PostEvent(M_COMPONENT_INSTALLED, new GString(k));
		}
		PourAll();
	}

	void OnCloseInstaller()
	{
		DeleteObj(Bar);
		PourAll();
	}

	int OnCommand(int Cmd, int Event, OsView Wnd)
	{
		switch (Cmd)
		{
			case IDC_EXIT:
				LgiCloseApp();
				break;
			case IDC_TO_HTML:
				Tabs->Value(0);
				Txt->Name(Edit->Name());
				break;
			case IDC_TO_NODES:
				Tabs->Value(1);
				#ifdef _DEBUG
				Tree->Empty();
				Edit->DumpNodes(Tree);
				#endif
				break;
			case IDC_VIEW_IN_BROWSER:
			{
				GFile::Path p(LSP_TEMP);
				p += "export.html";
				if (Edit->Save(p))
				{
					LgiExecute(p);
				}
				break;
			}
			case IDC_SAVE_FILE:
			{
				GFileSelect s;
				s.Parent(this);
				s.Type("HTML", "*.html");
				if (s.Save())
					Edit->Save(s.Name());
				break;
			}
			case IDC_SAVE:
			{
				if (Edit)
				{
					GString Html;
					GArray<GDocView::ContentMedia> Media;
					if (Edit->GetFormattedContent("text/html", Html, &Media))
					{
						GFile::Path p(LSP_APP_INSTALL);
						p += "Output";
						if (!p.IsFolder())
							FileDev->CreateFolder(p);
						p += "output.html";
						GFile f;
						if (f.Open(p, O_WRITE))
						{
							f.SetSize(0);
							f.Write(Html);
							f.Close();
							
							GString FileName = p.GetFull();
							
							for (unsigned i=0; i<Media.Length(); i++)
							{
								GDocView::ContentMedia &Cm = Media[i];
								
								p--;
								p += Cm.FileName;
								if (f.Open(p, O_WRITE))
								{
									f.SetSize(0);
									if (Cm.Data.IsBinary())
										f.Write(Cm.Data.CastVoidPtr(), Cm.Data.Value.Binary.Length);
									else if (Cm.Stream)
									{
										GCopyStreamer Cp;
										Cp.Copy(Cm.Stream, &f);
									}
									else
										LgiAssert(0);
									f.Close();
								}
							}
							
							LgiExecute(FileName);
						}
					}
				}
				break;
			}
		}

		return GWindow::OnCommand(Cmd, Event, Wnd);
	}

	int OnNotify(GViewI *c, int f)
	{
		if (c->GetId() == IDC_EDITOR &&
			#if 1
			(f == GNotifyDocChanged || f == GNotifyCursorChanged) &&
			#else
			(f == GNotifyDocChanged) &&
			#endif
			Edit)
		{
			LastChange = LgiCurrentTime();
			Tree->Empty();
		}

		return 0;
	}
	
	void OnReceiveFiles(GArray<char*> &Files)
	{
		if (Edit && Files.Length() > 0)
		{
			GAutoString t(ReadTextFile(Files[0]));
			if (t)
				Edit->Name(t);
		}
	}

	GMessage::Result OnEvent(GMessage *m)
	{
		if (m->Msg() == M_INSTALL)
		{
			GAutoPtr<GString> c((GString*)m->A());
			if (c)
			{
				GCapabilityTarget::CapsHash h;
				h.Add(*c, true);
				OnInstall(&h, true);
			}
		}

		return GWindow::OnEvent(m);
	}
};

int LgiMain(OsAppArguments &AppArgs)
{
	GApp a(AppArgs, "RichEditTest");
	a.AppWnd = new App;
	a.Run();
	return 0;
}
