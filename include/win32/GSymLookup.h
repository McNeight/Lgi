/// \file
/// \author Matthew Allen
#ifndef _GSYMLOOKUP_H_
#define _GSYMLOOKUP_H_

#ifdef WIN32
#include "ImageHlp.h"
#include <direct.h>
#include <stdio.h>
#undef _T
#include <tchar.h>

#if _MSC_VER < 1300
typedef DWORD DWORD_PTR;
#define __w64
#endif

#ifdef _UNICODE
#define TCHAR_FORMAT	"%S"
#else
#define TCHAR_FORMAT	"%s"
#endif

typedef BOOL (__stdcall *SYMINITIALIZEPROC)( HANDLE, LPSTR, BOOL );
typedef BOOL (__stdcall *SYMCLEANUPPROC)( HANDLE );

typedef BOOL (__stdcall * STACKWALKPROC)
			( DWORD, HANDLE, HANDLE, LPSTACKFRAME, LPVOID,
			 PREAD_PROCESS_MEMORY_ROUTINE,PFUNCTION_TABLE_ACCESS_ROUTINE,
			 PGET_MODULE_BASE_ROUTINE, PTRANSLATE_ADDRESS_ROUTINE );

typedef LPVOID (__stdcall *SYMFUNCTIONTABLEACCESSPROC)( HANDLE, DWORD );

typedef DWORD (__stdcall *SYMGETMODULEBASEPROC)( HANDLE, DWORD );

typedef BOOL (__stdcall *SYMGETSYMFROMADDRPROC)
							 ( HANDLE, DWORD, PDWORD, PIMAGEHLP_SYMBOL );

#ifdef _WIN64
typedef BOOL (WINAPI *pSymGetLineFromAddr64)(HANDLE hProcess, DWORD64 dwAddr, PDWORD pdwDisplacement, PIMAGEHLP_LINE64 Line);
#else
typedef BOOL (__stdcall *proc_SymGetLineFromAddr)(	HANDLE hProcess,
													DWORD dwAddr,
													PDWORD pdwDisplacement,
													PIMAGEHLP_LINE Line);
#endif

typedef DWORD (__stdcall *proc_SymGetOptions)(VOID);
typedef DWORD (__stdcall *proc_SymSetOptions)(DWORD SymOptions);

typedef BOOL (WINAPI *pSymFromAddr)(HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFO Symbol);

/// Lookup the file/line information for an instruction pointer value
class GSymLookup
{
	HMODULE DbgHelp;
	HANDLE hProcess;
	
	SYMINITIALIZEPROC           SymInitialize;
	SYMCLEANUPPROC              SymCleanup;
	STACKWALKPROC               StackWalk;
	SYMFUNCTIONTABLEACCESSPROC  SymFunctionTableAccess;
	SYMGETMODULEBASEPROC        SymGetModuleBase;
	SYMGETSYMFROMADDRPROC       SymGetSymFromAddr;
	#ifdef _WIN64
	pSymGetLineFromAddr64		SymGetLineFromAddr64;
	#else
	proc_SymGetLineFromAddr     SymGetLineFromAddr;
	#endif	
	proc_SymGetOptions          SymGetOptions;
	proc_SymSetOptions          SymSetOptions;

	bool    InitOk;

public:
	typedef NativeInt Addr;

	GSymLookup()
	{
		hProcess = GetCurrentProcess();
		InitOk = 0;
		DbgHelp = LoadLibrary(_T("dbghelp.dll"));
		if (DbgHelp)
		{
			SymInitialize = (SYMINITIALIZEPROC) GetProcAddress(DbgHelp, "SymInitialize");
			SymCleanup = (SYMCLEANUPPROC) GetProcAddress(DbgHelp, "SymCleanup");
			StackWalk = (STACKWALKPROC) GetProcAddress(DbgHelp, "StackWalk");
			SymFunctionTableAccess = (SYMFUNCTIONTABLEACCESSPROC) GetProcAddress(DbgHelp, "SymFunctionTableAccess");
			SymGetModuleBase = (SYMGETMODULEBASEPROC) GetProcAddress(DbgHelp, "SymGetModuleBase");
			SymGetSymFromAddr = (SYMGETSYMFROMADDRPROC) GetProcAddress(DbgHelp, "SymGetSymFromAddr");
			#ifdef _WIN64
			SymGetLineFromAddr64 = (pSymGetLineFromAddr64) GetProcAddress(DbgHelp, "SymGetLineFromAddr64");
			#else
			SymGetLineFromAddr = (proc_SymGetLineFromAddr) GetProcAddress(DbgHelp, "SymGetLineFromAddr");
			#endif
			SymGetOptions = (proc_SymGetOptions) GetProcAddress(DbgHelp, "SymGetOptions");
			SymSetOptions = (proc_SymSetOptions) GetProcAddress(DbgHelp, "SymSetOptions");

			DWORD dwOpts = SymGetOptions();
			DWORD Set = SymSetOptions(dwOpts | SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST);

			char s[256];
			_getcwd(s, sizeof(s));

			#ifdef _MSC_VER
			char path[MAX_PATH] = "";
			size_t sz;
			getenv_s(&sz, path, sizeof(path), "path");
			#else
			char *path = getenv("path");
			#endif
			int pathlen = (int)strlen(path);
			
			int len = pathlen + (int)strlen(s) + 256;
			char *all = (char*)malloc(len);
			if (all)
			{
				#if _MSC_VER >= 1300
				strcpy_s(all, len, path);
				#else
				strcpy(all, path);
				#endif

				if (all[strlen(all)-1] != ';')
				{
					#if _MSC_VER >= 1300
					strcat_s(all, len, ";");
					#else
					strcat(all, ";");
					#endif
				}

				#ifdef _DEBUG
				char *Mode = "Debug";
				#else
				char *Mode = "Release";
				#endif

				#if _MSC_VER >= 1300
				sprintf_s(all+strlen(all), len-strlen(all), "%s\\%s", s, Mode);
				#else
				sprintf(all+strlen(all), "%s\\%s", s, Mode);
				#endif

				InitOk = SymInitialize(hProcess, all, true) != 0;
				free(all);
			}
		}
	}
	
	~GSymLookup()
	{
		if (DbgHelp)
		{
			SymCleanup(hProcess);
			FreeLibrary(DbgHelp);
		}
	}
	
	bool GetStatus()
	{
		return InitOk;
	}
	
	/// Looks up the file and line related to an instruction pointer address
	/// \returns non zero on success
	bool Lookup
	(
		/// The return string buffer
		char *buf,
		/// The buffer's length
		int buffer_len,
		/// The address
		Addr *Ip,
		/// Cound of IP's
		int IpLen
	)
	{
		if (!buf || buffer_len < 1 || !Ip || IpLen < 1)
			return false;

		#if _MSC_VER >= 1300
		#define Sprintf sprintf_s
		#else
		#define Sprintf snprintf
		#endif

		bool Status = true;
		char *buf_end = buf + buffer_len;
		for (int i=0; i<IpLen; i++)
		{
			MEMORY_BASIC_INFORMATION mbi;
			VirtualQuery((void*)Ip[i], &mbi, sizeof(mbi));
			HINSTANCE Pid = (HINSTANCE)mbi.AllocationBase;

			TCHAR module[256] = _T("");
			if (GetModuleFileName(Pid, module, sizeof(module)))
			{
				BYTE symbolBuffer[ sizeof(IMAGEHLP_SYMBOL) + 512 ];
				PIMAGEHLP_SYMBOL pSymbol = (PIMAGEHLP_SYMBOL)symbolBuffer;
				DWORD symDisplacement = 0;
				IMAGEHLP_LINE Line;
				
				// Is this right???
				DWORD_PTR Offset = (DWORD_PTR)Ip[i] - (DWORD_PTR)Pid;

				pSymbol->SizeOfStruct = sizeof(symbolBuffer);
				pSymbol->MaxNameLength = 512;
				memset(&Line, 0, sizeof(Line));
				Line.SizeOfStruct = sizeof(Line);

				TCHAR *mod = _tcsrchr(module, '\\');
				if (mod) mod++;
				else mod = module;

				bool found_addr = false;
				if (SymGetSymFromAddr(hProcess, (DWORD_PTR)Ip[i], &symDisplacement, pSymbol))
				{
					if (SymGetLineFromAddr(hProcess, (DWORD_PTR)Ip[i], &symDisplacement, &Line))
					{
						int Ch = Sprintf(buf, buf_end-buf, "    %08.8x: "TCHAR_FORMAT", %s:%i", Ip[i], mod, Line.FileName, Line.LineNumber);
						if (Ch > 0)
							buf += Ch;
						else
							Status = false;
						found_addr = true;
					}
					else if (pSymbol->Name[0] != '$')
					{
						int Ch = Sprintf(buf, buf_end-buf, "    %08.8x: "TCHAR_FORMAT ", %s+0x%x", Ip[i], mod, pSymbol->Name, symDisplacement);
						if (Ch > 0)
							buf += Ch;
						else
							Status = false;
						found_addr = true;
					}
				}

				if (!found_addr)
				{
					int Ch = Sprintf(buf, buf_end-buf, "    %08.8x: "TCHAR_FORMAT, Ip[i], mod);
					if (Ch > 0)
						buf += Ch;
					else
						Status = false;
				}
			}
			else
			{
				int Ch = Sprintf(buf, buf_end-buf, "    %08.8x: unknown module", Ip[i]);
				if (Ch > 0)
					buf += Ch;
				else
					Status = false;
			}

			int Ch = Sprintf(buf, buf_end-buf, "\r\n");
			if (Ch > 0)
				buf += Ch;
		}

		return Status;
	}
	
	int BackTrace(long Eip, long Ebp, Addr *addr, int len)
	{
		if (!addr || len < 1)
			return 0;

		int i = 0;

		// Save the stack trace
		Addr RegEbp = Ebp;
		if (!Ebp)
		{
			memset(addr, 0, sizeof(Addr) * len);
			#if defined(_WIN64)
			LgiAssert(!"Not impl");
			#elif defined(_MSC_VER)
			// Microsoft C++
			_asm {
				mov eax, ebp
				mov RegEbp, eax
			}
			#else
			// GCC
			asm("movl %%ebp, %%eax;"
				"movl %%eax, %0;"
				:"=r"(RegEbp)	/* output */
				:				/* no input */
				:"%eax"			/* clobbered register */
				);  
			#endif
		}

		if (Eip)
			addr[i++] = Eip;

		for (; i<len; i++)
		{
			if ((RegEbp & 3) != 0 ||
				IsBadReadPtr( (void*)RegEbp, 8 ))
				break;

			// if (i >= 0)
			addr[i] = (Addr) *((uint8**)RegEbp + 1);
			RegEbp = *(Addr*)RegEbp;
		}

		return i;
	}
};
#endif

#endif