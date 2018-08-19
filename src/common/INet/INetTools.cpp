#include <string.h>
#include <ctype.h>

#include "GMem.h"
#include "LgiOsDefs.h"
#include "INetTools.h"
#include "GString.h"

#define IsAlpha(c)					(((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))

// Gets a field value
char *InetGetField(const char *s)
{
	const char *e = s;
	static const char *WhiteSpace = " \r\t\n";

	// Look for the end of the string
	while (*e)
	{
		if (*e == '\n')
		{
			if (!strchr(" \t", e[1]))
			{
				break;
			}
		}

		e++;
	}

	// Trim whitespace off each end of the string
	while (s < e && strchr(WhiteSpace, *s))
		s++;
	while (e > s && strchr(" \r\t\n", e[-1]))
		e--;

	// Calc the length
	size_t Size = e - s;
	char *Str = new char[Size+1];
	if (Str)
	{
		const char *In = s;
		char *Out = Str;

		while (In < e)
		{
			if (*In == '\r')
			{
			}
			else if (*In == 9)
			{
				*Out++ = ' ';
			}
			else
			{
				*Out++ = *In;
			}

			In++;
		}

		*Out++ = 0;
	}

	return Str;
}

const char *SeekNextLine(const char *s, const char *End)
{
	if (s)
	{
		for (; *s && *s != '\n' && (!End || s < End); s++);
		if (*s == '\n' && (!End || s < End)) s++;
	}

	return s;
}

// Search through headers for a field
char *InetGetHeaderField(	// Returns an allocated string or NULL on failure
	const char *Headers,	// Pointer to all the headers
	const char *Field,		// The field your after
	ssize_t Len)			// Maximum len to run, or -1 for NULL terminated
{
	if (Headers && Field)
	{
		// for all lines
		const char *End = Len < 0 ? 0 : Headers + Len;
		size_t FldLen = strlen(Field);
		
		for (const char *s = Headers;
			*s && (!End || s < End);
			s = SeekNextLine(s, End))
		{
			if (*s != 9 &&
				_strnicmp(s, Field, FldLen) == 0)
			{
				// found a match
				s += FldLen;
				if (*s == ':')
				{
					s++;
					while (*s)
					{
					    if (strchr(" \t\r", *s))
					    {
						    s++;
						}
						else if (*s == '\n')
						{
						    if (strchr(" \r\n\t", s[1]))
						        s += 2;
						    else
						        break;
						}
						else break;						    
					}
					
					return InetGetField(s);
				}
			}
		}
	}

	return 0;
}

void InetTokeniseHeaders(List<GInetHeader> &Out, const char *In)
{
	// loop through and extract all the headers
	const char *e = 0;
	for (const char *s=In; s && *s; s = e)
	{
		// skip whitespace
		while (strchr(" \t", *s)) s++;

		// get end-of-line
		e = strstr(s, "\r\n");
		if (!e)
			e = s + strlen(s);
		size_t Len = e - s;

		// process line
		char *eName = strnchr(s, ':', Len);
		if (eName)
		{
			GInetHeader *h = new GInetHeader(NewStr(s, eName - s));
			if (h)
			{
				// move on to text after the whitespace
				s = eName + 1;
				while (strchr(" \t", *s)) s++;

				// extract the data itself
				h->Str = InetGetField(s);
				
				Out.Insert(h);
			}
		}
		
		if (*e == '\r') e++;
		if (*e == '\n') e++;
	}
}

char *InetRemoveField(char *Headers, const char *Field)
{
	char *Status = Headers;

	if (Headers && Field)
	{
		// loop through and look at all the headers
		char *e = 0;
		for (char *s=Headers; s && *s; s = *e == '\n' ? e + 1 : e)
		{
			// skip whitespace
			while (strchr(" \t", *s)) s++;

			// get end-of-line
			e = strchr(s, '\n');
			if (!e) e = s + strlen(s);
			size_t Len = e - s;

			// process line
			char *eName = strnchr(s, ':', Len);
			if (eName)
			{
				if (_strnicmp(s, Field, eName - s) == 0)
				{
					GMemQueue Out;

					// found header... push headers before this one
					Out.Write((uchar*)Headers, (int) (s - Headers));

					// get end
					char *End = eName;
					for (char *eol = strchr(End, '\n'); eol; eol = strchr(eol+1, '\n'))
					{
						End = eol + 1;
						if (eol[1] != '\t' && eol[1] != ' ') break;
					}

					// push text after this header
					Out.Write((uchar*)End, (int)strlen(End));

					// replace text..
					DeleteArray(Headers);
					int64 Len = Out.GetSize();
					Status = new char[Len+1];
					if (Status)
					{
						Out.Read((uchar*)Status, (int)Len);
						Status[Len] = 0;
					}

					// leave loop
					break;
				}
			}
		}
	}

	return Status;
}

char *InetGetAllHeaders(const char *s)
{
	char *All = 0;
	if (s)
	{
		const char *Start = s;
		while (s && *s)
		{
			int i=0;
			for (; *s && *s != '\r' && *s != '\n'; s++, i++);
			if (*s == '\r') s++;
			if (*s == '\n') s++;
			if (!i) break;			
		}

		All = NewStr(Start, s - Start);
	}

	return All;
}

char *InetExtractBoundary(const char *Field)
{
	if (Field)
	{
		char *Start = stristr(Field, "boundary=");
		if (Start)
		{
			// skip over the id
			Start += 9;
			if (*Start == '\"')
			{
				// attempt to read the start and end of the boundry key
				Start++;
				char *End = strchr(Start, '\"');
				if (End)
				{
					*End = 0;
					return NewStr(Start);
				}
			}
			else
			{
				// not delimited.... hmmm... grrrr!
				char *End = Start;
				while (	*End &&
						!strchr(" \t\r\n", *End) &&
						*End != ';')
				{
					End++;
				}

				*End = 0;
				return NewStr(Start);
			}
		}
	}

	return 0;
}

char *InetGetSubField(const char *s, const char *Field)
{
	char *Status = 0;

	if (s && Field)
	{
		s = strchr(s, ';');
		if (s)
		{
			s++;

			size_t FieldLen = strlen(Field);
			char White[] = " \t\r\n";
			while (*s)
			{
				// Skip leading whitespace
				while (*s && (strchr(White, *s) || *s == ';')) s++;

				// Parse field name
				if (IsAlpha((uint8)*s))
				{
					const char *f = s;
					while (*s && (IsAlpha(*s) || *s == '-')) s++;
					bool HasField = ((s-f) == FieldLen) && (_strnicmp(Field, f, FieldLen) == 0);
					while (*s && strchr(White, *s)) s++;
					if (*s == '=')
					{
						s++;
						while (*s && strchr(White, *s)) s++;
						if (*s && strchr("\'\"", *s))
						{
							// Quote Delimited Field
							char d = *s++;
							char *e = strchr((char*)s, d);
							if (e)
							{
								if (HasField)
								{
									Status = NewStr(s, e-s);
									break;
								}

								s = e + 1;
							}
							else break;
						}
						else
						{
							// Space Delimited Field
							const char *e = s;
							while (*e && !strchr(White, *e) && *e != ';') e++;

							if (HasField)
							{
								Status = NewStr(s, e-s);
								break;
							}

							s = e;
						}
					}
					else break;
				}
				else break;
			}
		}
	}

	return Status;
}

