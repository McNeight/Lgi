#include "Lvc.h"

VcFile::VcFile(AppPriv *priv, VcFolder *owner, GString revision, bool working)
{
	d = priv;
	Owner = owner;
	Revision = revision;
	if (working)
		Chk = new LListItemCheckBox(this, 0, false);
	else
		Chk = NULL;
}

VcFile::~VcFile()
{
}

int VcFile::Checked(int Set)
{
	if (!Chk)
		return -1;

	if (Set >= 0)
		Chk->Value(Set);

	return (int)Chk->Value();
}

void VcFile::SetDiff(GString diff)
{
	Diff = diff;
	if (LListItem::Select())
		d->Diff->Name(Diff);
}

void VcFile::Select(bool b)
{
	LListItem::Select(b);
	if (b)
		d->Diff->Name(Diff);
}

void VcFile::OnMouseClick(GMouse &m)
{
	LListItem::OnMouseClick(m);

	if (m.IsContextMenu())
	{
		GSubMenu s;
		char *File = GetText(COL_FILENAME);
		GFile::Path p = Owner->GetPath();
		p += File;

		if (Chk)
		{
			// Uncommitted changes
			s.AppendItem("Revert Changes", IDM_REVERT);
			s.AppendItem("Browse To", IDM_BROWSE);

			int Cur = GetEol(p);
			GSubMenu *Ln = s.AppendSub("Line Endings");
			GMenuItem *Item = Ln->AppendItem("LF", IDM_EOL_LF);
			if (Item && Cur == IDM_EOL_LF) Item->Checked(true);
			Ln->AppendItem("CRLF", IDM_EOL_CRLF);
			if (Item && Cur == IDM_EOL_CRLF) Item->Checked(true);
			Ln->AppendItem("Auto", IDM_EOL_AUTO);
		}
		else
		{
			// Committed changes
			s.AppendItem("Revert To This Revision", IDM_REVERT_TO_REV);
			s.AppendItem("Blame", IDM_BLAME);
			s.AppendItem("Save As", IDM_SAVE_AS);
		}

		int Cmd = s.Float(GetList(), m);
		switch (Cmd)
		{
			case IDM_REVERT:
			{
				Owner->Revert(p);
				break;
			}
			case IDM_BROWSE:
			{
				if (p.Exists())
					LgiBrowseToFile(p);
				else
					LgiMsg(GetList(), "Can't find file...", AppName);
				break;
			}
			case IDM_REVERT_TO_REV:
			{
				Owner->Revert(p, Revision);
				break;
			}
			case IDM_BLAME:
			{
				Owner->Blame(p);
				break;
			}
			case IDM_SAVE_AS:
			{
				Owner->SaveFileAs(p, Revision);
				break;
			}
			case IDM_EOL_LF:
			case IDM_EOL_CRLF:
			case IDM_EOL_AUTO:
			{
				Owner->SetEol(p, Cmd);
				break;
			}
		}
	}
	else if (m.Left() && m.Down() && d->Tabs)
	{
		d->Tabs->Value(0);
	}
}


bool ConvertEol(const char *Path, bool Cr)
{
	GFile f;
	if (!f.Open(Path, O_READWRITE))
		return false;
	GString s = f.Read();
	s = s.Replace("\r");
	if (Cr)
		s = s.Replace("\n", "\r\n");
	f.SetSize(0);
	return f.Write(s) == s.Length();
}

int GetEol(const char *Path)
{
	GFile f;
	if (!f.Open(Path, O_READ))
		return false;
	GString s = f.Read();
	int Cr = 0, Lf = 0;
	for (char *c = s; *c; c++)
	{
		if (*c == '\r') Cr++;
		if (*c == '\n') Lf++;
	}
	if (Cr > 0)
		return IDM_EOL_CRLF;
	if (Lf > 0)
		return IDM_EOL_LF;
	return 0;
}