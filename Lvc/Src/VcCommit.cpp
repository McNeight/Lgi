#include "Lvc.h"

VcCommit::VcCommit(AppPriv *priv)
{
	d = priv;
	Current = false;
}

char *VcCommit::GetRev()
{
	return Rev;
}

void VcCommit::SetCurrent(bool b)
{
	Current = b;
}

char *VcCommit::GetText(int Col)
{
	switch (Col)
	{
		case 0:
			return Current ? "***" : NULL;
		case 1:
			return Rev;
		case 2:
			return Author;
		case 3:
			Cache = Ts.Get();
			return Cache;
		case 4:
			Cache = Msg.Split("\n", 1)[0];
			return Cache;
	}

	return NULL;
}

bool VcCommit::GitParse(GString s)
{
	GString::Array lines = s.Split("\n");
	if (lines.Length() <= 3)
		return false;

	for (unsigned ln = 0; ln < lines.Length(); ln++)
	{
		GString &l = lines[ln];
		if (ln == 0)
			Rev = l.Strip();
		else if (l.Find("Author:") >= 0)
			Author = l.Split(":", 1)[1].Strip();
		else if (l.Find("Date:") >= 0)
			Ts.Parse(l.Split(":", 1)[1].Strip());
		else if (l.Strip().Length() > 0)
		{
			if (Msg)
				Msg += "\n";
			Msg += l.Strip();
		}
	}

	return Author && Msg && Rev;
}

bool VcCommit::SvnParse(GString s)
{
	GString::Array lines = s.Split("\n");
	if (lines.Length() < 1)
		return false;

	for (unsigned ln = 0; ln < lines.Length(); ln++)
	{
		GString &l = lines[ln];
		if (ln == 0)
		{
			GString::Array a = l.Split("|");
			if (a.Length() > 3)
			{
				Rev = a[0].Strip(" \tr");
				Author = a[1].Strip();
				Ts.Parse(a[2]);
			}
		}
		else
		{
			if (Msg)
				Msg += "\n";
			Msg += l.Strip();
		}
	}

	return Author && Rev && Ts.IsValid();
}

VcFolder *VcCommit::GetFolder()
{
	GTreeItem *i = d->Tree->Selection();
	return dynamic_cast<VcFolder*>(i);
}

void VcCommit::OnMouseClick(GMouse &m)
{
	if (m.IsContextMenu())
	{
		GSubMenu s;
		s.AppendItem("Update", IDM_UPDATE, !Current);
		int Cmd = s.Float(GetList(), m);
		switch (Cmd)
		{
			case IDM_UPDATE:
			{
				VcFolder *f = GetFolder();
				if (!f)
				{
					LgiAssert(!"No folder?");
					break;
				}

				f->OnUpdate(Rev);
				break;
			}
		}
	}
}
