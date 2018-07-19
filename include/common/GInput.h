#ifndef __GINPUT_H
#define __GINPUT_H

/// This callback if supplied to the GInput will create a "..." button, which if clicked
/// will call the callback.
typedef void (*GInputCallback)(class GInput *Dlg, GViewI *EditCtrl, void *Param);

/// This class displays a window with a message and an edit box to enter a string.
/// Once constructed, use the GDialog::DoModal() call to run the input window, it
/// returns TRUE if 'Ok' is clicked.
class LgiClass GInput : public GDialog
{
	GEdit *Edit;
	GInputCallback Callback;
	void *CallbackParam;
	GString Str;

public:
	/// Constructs the dialog.
	GInput
	(
		/// The parent view
		GViewI *parent,
		/// The initial value in the edit box
		const char *InitStr = "",
		/// The message to display in the text box
		const char *Msg = "Enter String",
		/// The title of the window
		const char *Title = "Input",
		/// True if you want the edit box characters hashed out for a password
		bool Password = false,
		/// [Optional] If this parameter is supplied then a "..." button is added to the dialog.
		/// If clicked the callback is called with the dialog and editbox control ptrs.
		GInputCallback callback = 0,
		/// [Optional] Callback user parameter
		void *CallbackParam = 0
	);
	
	int OnNotify(GViewI *Ctrl, int Flags);
	GString GetStr() { return Str; }
};

#endif
