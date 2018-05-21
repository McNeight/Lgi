// \file
/// \author Matthew Allen
#ifndef _LGI_INTERFACES_H_
#define _LGI_INTERFACES_H_

// Includes
#include "GMem.h"
#include "GArray.h"
#include "LgiOsDefs.h"
#include "GColour.h"
#include "LCancel.h"
#include "GStringClass.h"

// Fwd defs
class GXmlTag;
class GMouseHook;
class GFont;
class GRect;
class GdcPt2;
class GRegion;
class GSurface;
class GViewI;
#ifndef BEOS
class GMessage;
#endif
class GMouse;
class GKey;
class GWindow;
class GViewFill;
class GView;
class GVariant;
class GCss;

// Classes
class GDomI
{
public:
	virtual ~GDomI() {}

	virtual bool GetValue(const char *Var, GVariant &Value) { return false; }
	virtual bool SetValue(const char *Var, GVariant &Value) { return false; }
	virtual bool CallMethod(const char *MethodName, GVariant *ReturnValue, GArray<GVariant*> &Args) { return false; }
};

/// Stream interface class
/// 
/// Defines the API
/// for all the streaming data classes. Allows applications to plug
/// different types of date streams into functions that take a GStream.
/// Typically this means being able to swap files with sockets or data
/// buffers etc.
/// 
class GStreamI : virtual public GDomI
{
public:
	/// Open a connection
	/// \returns > zero on success
	virtual int Open
	(
		/// A string connection parameter
		const char *Str = 0,
		/// An integer connection parameter
		int Int = 0
	)
	{ return false; }

	/// Returns true is the stream is still open
	virtual bool IsOpen() { return false; }

	/// Closes the connection
	/// \returns > zero on success
	virtual int Close() { return 0; }

	/// \brief Gets the size of the stream
	/// \return The size or -1 on error (e.g. the information is not available)
	virtual int64 GetSize() { return -1; }

	/// \brief Sets the size of the stream
	/// \return The new size or -1 on error (e.g. the size is not set-able)
	virtual int64 SetSize(int64 Size) { return -1; }

	/// \brief Gets the current position of the stream
	/// \return Current position or -1 on error (e.g. the position is not known)
	virtual int64 GetPos() { return -1; }

	/// \brief Sets the current position of the stream
	/// \return The new current position or -1 on error (e.g. the position can't be set)
	virtual int64 SetPos(int64 Pos) { return -1; }

	/// \brief Read bytes out of the stream
	/// \return > 0 on succes, which indicates the number of bytes read
	virtual ssize_t Read(void *Buffer, ssize_t Size, int Flags = 0) = 0;

	/// \brief Write bytes to the stream
	/// \return > 0 on succes, which indicates the number of bytes written
	virtual ssize_t Write(const void *Buffer, ssize_t Size, int Flags = 0) = 0;

	/// \brief Creates a dynamically allocated copy of the same type of stream.
	/// This new stream is not connected to anything.
	/// \return The new stream or NULL on error.
	virtual GStreamI *Clone() { return 0; }
};

/// Socket logging types..
enum GSocketLogTypes
{
	/// Do no logging
	NET_LOG_NONE = 0,
	/// Log a hex dump of everything
	NET_LOG_HEX_DUMP = 1,
	/// Log just the bytes
	NET_LOG_ALL_BYTES = 2
};

/// Virtual base class for a socket. See the documentation for GSocket for a more
/// through treatment of this object's API.
class GSocketI :
	virtual public GStreamI,
	public LCancel
{
public:
	enum SocketMsgType
	{
		SocketMsgNone,
		SocketMsgInfo,
		SocketMsgSend,
		SocketMsgReceive,
		SocketMsgWarning,
		SocketMsgError,
	};

	virtual ~GSocketI() {}

	/// Returns the actual socket (as defined by the OS)
	virtual OsSocket Handle(OsSocket Set = INVALID_SOCKET) { return INVALID_SOCKET; }

	// Host/Port meta data
		/// Returns the IP at this end of the socket
		virtual bool GetLocalIp
		(
			/// Ptr to a buffer of at least 16 bytes
			char *IpAddr
		) { return false; }
		/// Return the port at this end of the connection
		virtual int GetLocalPort() { return 0; }
		/// Gets the remote IP
		virtual bool GetRemoteIp(char *IpAddr) { return false; }
		/// Return the port at this end of the connection
		virtual int GetRemotePort() { return 0; }

	// Timeout
		/// Gets the current timeout for operations in ms
		virtual int GetTimeout() { return -1; }
		/// Sets the current timeout for operations in ms
		virtual void SetTimeout(int ms) {}
		/// Sets the continue token
		virtual void SetContinue(bool *Token) {}


	// State
		/// True if there is data available to read.
		virtual bool IsReadable(int TimeoutMs = 0) { return false; }
		/// True if the socket can be written to.
		virtual bool IsWritable(int TimeoutMs = 0) { return false; }
		/// True if the socket can be accept.
		virtual bool CanAccept(int TimeoutMs = 0) { return false; }
		/// Returns whether the socket is set to blocking or not
		virtual bool IsBlocking() { return true; }
		/// Set whether the socket should block or not
		virtual void IsBlocking(bool block) {}
		/// Get the send delay setting
		virtual bool IsDelayed() { return true; }
		/// Set the send delay setting
		virtual void IsDelayed(bool Delay) {}

// UDP
	
	/// Get UPD mode
	virtual bool GetUdp() { return false; }
	/// Set UPD mode
	virtual void SetUdp(bool b) {}
	/// Read UPD packet
	virtual int ReadUdp(void *Buffer, int Size, int Flags, uint32 *Ip = 0, uint16 *Port = 0) { return 0; }
	/// Write UPD packet
	virtual int WriteUdp(void *Buffer, int Size, int Flags, uint32 Ip, uint16 Port) { return 0; }

// Server

	/// Listens on a given port for an incoming connection.
	virtual bool Listen(int Port = 0) { return false; }
	/// Accepts an incoming connection and connects the socket you pass in to the remote host.
	virtual bool Accept(GSocketI *c) { return false; }


// Event call backs

	/// Called when the connection is dropped
	virtual void OnDisconnect() {}
	/// Called when data is read
	virtual void OnRead(char *Data, ssize_t Len) {}
	/// Called when data is written
	virtual void OnWrite(const char *Data, ssize_t Len) {}
	/// Called when an error occurs
	virtual void OnError(int ErrorCode, const char *ErrorDescription) {}
	/// Called when some events happens
	virtual void OnInformation(const char *Str) {}

	/// Process an error
	virtual int Error(void *Param) { return 0; }
	virtual const char *GetErrorString() { return NULL; }
};

class GAppI
{
public:
	/// The idle function should return false to wait for more
	/// messages, otherwise it will be called continuously when no
	/// messages are available.
	typedef bool (*OnIdleProc)(void *Param);

	/// Destroys the object
	virtual ~GAppI() {}

	virtual bool IsOk() = 0;
	
	/// Returns this processes ID
	virtual OsProcessId GetProcessId() = 0;
	
	/// Returns the thread currently running the active message loop
	virtual OsThread GetGuiThread() = 0;

	/// Resets the arguments
	virtual void SetAppArgs(OsAppArguments &AppArgs) = 0;
	
	/// Returns the arguemnts
	virtual OsAppArguments *GetAppArgs() = 0;

	/// Returns the n'th argument as a heap string. Free with DeleteArray(...).
	virtual const char *GetArgumentAt(int n) = 0;
	
	/// Enters the message loop.
	virtual bool Run
	(
		/// If true this function will return when the application exits (with LgiCloseApp()).
		/// Otherwise if false only pending events will be processed and then the function returns.
		bool Loop = true,
		/// [Optional] Idle callback
		OnIdleProc IdleCallback = 0,
		/// [Optional] User param for IdleCallback
		void *IdleParam = 0
	) = 0;
	
	/// Event called to process the command line
	virtual void OnCommandLine() = 0;
	
	/// Event called to process files dropped on the application
	virtual void OnReceiveFiles(GArray<char*> &Files) = 0;

	/// Event called to process URLs given to the application
	virtual void OnUrl(const char *Url) = 0;
	
	/// Exits the event loop with the code specified
	virtual void Exit
	(
		/// The application exit code.
		int Code = 0
	) = 0;
	
	/// \brief Parses the command line for a switch
	/// \return true if the option exists.
	virtual bool GetOption
	(
		/// The option to look for.
		const char *Option,
		/// String to receive the value (if any) of the option
		GString &Value
	) = 0;

	/// \brief Parses the command line for a switch
	/// \return true if the option exists.
	virtual bool GetOption
	(
		/// The option to look for.
		const char *Option,
		/// The buffer to receive the value of the command line parameter or NULL if you don't care.
		char *Dst = 0,
		/// The buffer size in bytes
		int DstSize = 0
	) = 0;
	
	/// Gets the application conf stored in lgi.conf
	virtual GXmlTag *GetConfig(const char *Tag) = 0;

	/// Sets a single tag in the config. (Not written to disk)
	virtual void SetConfig(GXmlTag *Tag) = 0;
	
	/// Gets the control with the keyboard focus
	virtual GViewI *GetFocus() = 0;
	
	/// Gets the MIME type of a file
	virtual GAutoString GetFileMimeType
	(
		/// The file to identify
		const char *File
	) = 0;
		
	/// Get a system metric
	virtual int32 GetMetric
	(
		/// One of #LGI_MET_DECOR_X, #LGI_MET_DECOR_Y
		LgiSystemMetric Metric
	) = 0;

	/// Get the mouse hook instance
	virtual GMouseHook *GetMouseHook() = 0;

	/// Returns the number of cpu cores or -1 if unknown.
	virtual int GetCpuCount() { return -1; }

	/// Gets the font cache
	virtual class GFontCache *GetFontCache() = 0;
};

class GEventSinkI
{
public:
	virtual ~GEventSinkI() {}
	virtual bool PostEvent(int Cmd, GMessage::Param a = 0, GMessage::Param b = 0) = 0;
};

class GEventTargetI
{
public:
	virtual ~GEventTargetI() {}
	virtual GMessage::Result OnEvent(GMessage *Msg) = 0;
};

class GEventsI : public GEventTargetI
{
public:
	virtual ~GEventsI() {}

	// Events
	virtual void OnMouseClick(GMouse &m) = 0;
	virtual void OnMouseEnter(GMouse &m) = 0;
	virtual void OnMouseExit(GMouse &m) = 0;
	virtual void OnMouseMove(GMouse &m) = 0;
	virtual bool OnMouseWheel(double Lines) = 0;
	virtual bool OnKey(GKey &k) = 0;
	virtual void OnAttach() = 0;
	virtual void OnCreate() = 0;
	virtual void OnDestroy() = 0;
	virtual void OnFocus(bool f) = 0;
	virtual void OnPulse() = 0;
	virtual void OnPosChange() = 0;
	virtual bool OnRequestClose(bool OsShuttingDown) = 0;
	virtual int OnHitTest(int x, int y) = 0;
	virtual void OnChildrenChanged(GViewI *Wnd, bool Attaching) = 0;
	virtual void OnPaint(GSurface *pDC) = 0;
	virtual int OnNotify(GViewI *Ctrl, int Flags) = 0;
	virtual int OnCommand(int Cmd, int Event, OsView Wnd) = 0;
};

class GViewIterator
{
public:
	virtual ~GViewIterator() {}
	virtual GViewI *First() = 0;
	virtual GViewI *Last() = 0;
	virtual GViewI *Next() = 0;
	virtual GViewI *Prev() = 0;
	virtual int Length() = 0;
	virtual int IndexOf(GViewI *view) = 0;
	virtual GViewI *operator [](int Idx) = 0;
};

class GViewLayoutInfo
{
public:
	struct Range
	{
		// 0 if unknown, -1 for "all available"
		int32 Min, Max;

		Range()
		{
			Min = Max = 0;
		}
	};

	Range Width; 
	Range Height;
};

class GViewI :
	public GEventsI,
	public GEventSinkI,
	public virtual GDomI
{
	friend class GView;

public:
	// Handles
	virtual OsView Handle() = 0;
	virtual OsWindow WindowHandle() = 0;
	virtual GView *GetGView() { return NULL; }

	// Heirarchy
	virtual bool Attach(GViewI *p) = 0;
	virtual bool AttachChildren() = 0;
	virtual bool Detach() = 0;
	virtual bool IsAttached() = 0;
	virtual GWindow *GetWindow() = 0;
	virtual GViewI *GetParent() = 0;
	virtual void SetParent(GViewI *p) = 0;
	virtual void Quit(bool DontDelete = false) = 0;
	virtual bool AddView(GViewI *v, int Where = -1) = 0;
	virtual bool DelView(GViewI *v) = 0;
	virtual bool HasView(GViewI *v) = 0;
	virtual GViewIterator *IterateViews() = 0;

	// Threading
	virtual bool Lock(const char *file, int line, int TimeOut = -1) = 0;
	virtual void Unlock() = 0;
	virtual bool InThread() = 0;

	// Properties
	virtual bool Enabled() = 0;
	virtual void Enabled(bool e) = 0;
	virtual bool Visible() = 0;
	virtual void Visible(bool v) = 0;
	virtual bool Focus() = 0;
	virtual void Focus(bool f) = 0;
	virtual class GDragDropSource *DropSource(GDragDropSource *Set = NULL) = 0;
	virtual class GDragDropTarget *DropTarget(GDragDropTarget *Set = NULL) = 0;
	virtual bool DropTarget(bool t) = 0;
	virtual bool Sunken() = 0;
	virtual void Sunken(bool i) = 0;
	virtual bool Flat() = 0;
	virtual void Flat(bool i) = 0;
	virtual bool Raised() = 0;
	virtual void Raised(bool i) = 0;
	virtual bool GetTabStop() = 0;
	virtual void SetTabStop(bool b) = 0;

	// Style
    virtual GCss *GetCss(bool Create = false) = 0;
    virtual void SetCss(GCss *css) = 0;
    virtual bool SetCssStyle(const char *CssStyle) = 0;
    virtual bool SetColour(GColour &c, bool Fore) = 0;
	virtual GFont *GetFont() = 0;
	virtual void SetFont(GFont *Fnt, bool OwnIt = false) = 0;

	// Name and value
	virtual bool Name(const char *n) = 0;
	virtual bool NameW(const char16 *n) = 0;
	virtual char *Name() = 0;
	virtual char16 *NameW() = 0;
	virtual int64 Value() = 0;
	virtual void Value(int64 i) = 0;
	virtual const char *GetClass() { return "GViewI"; } // mainly for debugging
	virtual GString::Array *CssClasses() { return NULL; }

	// Size and position	
	virtual GRect &GetPos() = 0;
	virtual GRect &GetClient(bool InClientSpace = true) = 0;
	virtual bool SetPos(GRect &p, bool Repaint = false) = 0;
	virtual int X() = 0;
	virtual int Y() = 0;
	virtual GdcPt2 GetMinimumSize() = 0;
	virtual void SetMinimumSize(GdcPt2 Size) = 0;	

	// Id
	virtual int GetId() = 0;
	virtual void SetId(int i) = 0;

	// Events and notification
	virtual void SendNotify(int Data = 0) = 0;
	virtual GViewI *GetNotify() = 0;
	virtual void SetNotify(GViewI *n) = 0;

	// Mouse
	virtual LgiCursor GetCursor(int x, int y) = 0;
	virtual bool Capture(bool c) = 0;
	virtual bool IsCapturing() = 0;
	virtual bool GetMouse(GMouse &m, bool ScreenCoords = false) = 0;

	// Helper
	virtual GViewI *FindControl(OsView hnd) = 0;
	virtual GViewI *FindControl(int Id) = 0;
	virtual int64 GetCtrlValue(int Id) = 0;
	virtual void SetCtrlValue(int Id, int64 i) = 0;
	virtual char *GetCtrlName(int Id) = 0;
	virtual void SetCtrlName(int Id, const char *s) = 0;
	virtual bool GetCtrlEnabled(int Id) = 0;
	virtual void SetCtrlEnabled(int Id, bool Enabled) = 0;
	virtual bool GetCtrlVisible(int Id) = 0;
	virtual void SetCtrlVisible(int Id, bool Visible) = 0;
	virtual bool Pour(GRegion &r) = 0;

	template<class T>
	bool GetViewById(int Id, T *&Ptr)
	{
		GViewI *Ctrl = FindControl(Id);
		Ptr = dynamic_cast<T*>(Ctrl);
		return Ptr != NULL;
	}

	// Points
	virtual void PointToScreen(GdcPt2 &p) = 0;
	virtual void PointToView(GdcPt2 &p) = 0;
	virtual bool WindowVirtualOffset(GdcPt2 *Offset) = 0;	
	virtual GViewI *WindowFromPoint(int x, int y, bool Debug = false) = 0;
	virtual GdcPt2 &GetWindowBorderSize() = 0;
	virtual bool IsOver(GMouse &m) = 0;

	// Misc
	virtual bool Invalidate(GRect *r = 0, bool Repaint = false, bool NonClient = false) = 0;
	virtual bool Invalidate(GRegion *r, bool Repaint = false, bool NonClient = false) = 0;
	virtual void SetPulse(int Ms = -1) = 0;
	virtual bool OnLayout(GViewLayoutInfo &Inf) = 0;

protected:
	virtual bool OnViewMouse(GView *v, GMouse &m) = 0;
	virtual bool OnViewKey(GView *v, GKey &k) = 0;
};

class GMemoryPoolI
{
public:
	virtual ~GMemoryPoolI() {}

	virtual void *Alloc(size_t Size) = 0;
	virtual void Free(void *Ptr) = 0;
	virtual void Empty() = 0;
};

#endif
