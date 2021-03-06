////////////////////////////////////////////////////////////////////////////////
// Name:      process.h
// Purpose:   Declaration of class wxExProcess
// Author:    Anton van Wezenbeek
// Copyright: (c) 2015 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/process.h>

class wxTimer;
class wxTimerEvent;
class wxExSTC;
class wxExSTCEntryDialog;
class wxExSTCShell;

/// Offers a wxProcess, capturing execution output depending
/// on sync or async call to Execute.
class WXDLLIMPEXP_BASE wxExProcess : public wxProcess
{
public:
  /// Default constructor.
  wxExProcess();
  
  /// Destructor.
 ~wxExProcess();

  /// Copy constructor.
  wxExProcess(const wxExProcess& process);

  /// Assignment operator.
  wxExProcess& operator=(const wxExProcess& p);

  /// Handles shell commands.
  bool Command(int id, const wxString& command);
  
  /// Shows a config dialog, allowing you to set the command and folder.
  /// Returns dialog return code.
  static int ConfigDialog(
    wxWindow* parent,
    const wxString& title = _("Select Process"),
    bool modal = true);

  /// Executes the process.
  /// - In case asynchronously (wxEXEC_ASYNC) this call immediately returns.
  ///   The dialog will be shown, with STC component shell enabled,
  ///   and will be filled with output from the process.
  /// - In case synchronously (wxEXEC_SYNC) this call returns after execute ends, 
  ///   and the output is available using GetOutput.
  ///   The dialog is not shown (call ShowOutput).
  /// Return value is false if process could not execute (and GetError is true), 
  /// or if config dialog was invoked and cancelled (and GetError is false).
  /// Normally the reason for not being able to run the process is logged
  /// by the wxWidgets library using a wxLogError. You can prevent that
  /// using a wxLogNull before calling Execute.
  bool Execute(
    /// command to be executed, if empty
    /// last given command is used
    const wxString& command = wxEmptyString,
    /// flags for wxExecute
    int flags = wxEXEC_ASYNC,
    /// working dir, if empty last working dir is used
    const wxString& wd = wxEmptyString);
  
  /// Returns true if the command could not be executed.
  bool GetError() const {return m_Error;};

  /// Gets the output from Execute (only filled for wxEXEC_SYNC).
  const wxString& GetOutput() const {return m_Output;};
  
  /// Returns the shell, for input to the process.
  /// If you did not yet invoke Execute,
  /// NULL is returned.
  wxExSTCShell* GetShell();

  /// Returns the STC component from the dialog
  /// (might be NULL if dialog is NULL).
  wxExSTC* GetSTC() const;
  
  /// Returns true when the command executed resulted in stderr errors.
  bool HasStdError() const {return m_HasStdError;};
  
  /// Returns true if this process is running.
  bool IsRunning() const;

  /// Returns true if a process command is selected.
  bool IsSelected() const {return !m_Command.empty();};

  /// Kills the process (sends specified signal if process still running).
  wxKillError Kill(wxSignal sig = wxSIGKILL);
  
#if wxUSE_GUI
  /// Shows output from Execute (wxEXEC_SYNC) on the STC component.
  /// You can override this method to e.g. prepare a lexer on GetSTC
  /// before calling this base method.
  virtual void ShowOutput(const wxString& caption = wxEmptyString) const;
#endif
protected:
  /// Overriden from wxProcess.
  virtual void OnTerminate(int pid, int status);
private:
  bool CheckInput(const wxString& command = wxEmptyString);
  bool HandleCommand(const wxString& command);

  bool m_Busy;
  bool m_Error;
  bool m_HasStdError;
  bool m_Sync;

  wxString m_Command;  
  wxString m_Output;
  
  static wxString m_WorkingDirKey;

#if wxUSE_GUI
  static wxExSTCEntryDialog* m_Dialog;
#endif  

  wxTimer* m_Timer;
};
