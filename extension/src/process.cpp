////////////////////////////////////////////////////////////////////////////////
// Name:      process.cpp
// Purpose:   Implementation of class wxExProcess
// Author:    Anton van Wezenbeek
// Copyright: (c) 2012 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/config.h>
#include <wx/tokenzr.h>
#include <wx/txtstrm.h> // for wxTextInputStream
#include <wx/extension/process.h>
#include <wx/extension/configdlg.h>
#include <wx/extension/defs.h>
#include <wx/extension/shell.h>
#include <wx/extension/util.h> // for wxExConfigFirstOf

BEGIN_EVENT_TABLE(wxExProcess, wxProcess)
  EVT_MENU(ID_SHELL_COMMAND, wxExProcess::OnCommand)
  EVT_MENU(ID_SHELL_COMMAND_STOP, wxExProcess::OnCommand)
  EVT_TIMER(-1, wxExProcess::OnTimer)
END_EVENT_TABLE()

wxExSTCEntryDialog* wxExProcess::m_Dialog = NULL;
wxString wxExProcess::m_WorkingDirKey = _("Process folder");

wxExProcess::wxExProcess()
  : wxProcess(wxPROCESS_REDIRECT)
  , m_Timer(new wxTimer(this))
  , m_Error(false)
{
}

wxExProcess::~wxExProcess()
{
  delete m_Timer;
}

wxExProcess::wxExProcess(const wxExProcess& process)
{
  *this = process;
}

wxExProcess& wxExProcess::operator=(const wxExProcess& p)
{
  m_Error = p.m_Error;
  m_Output = p.m_Output;
  m_Timer = new wxTimer(this);
  
  return *this;
}

bool wxExProcess::CheckInput()
{
  if (IsInputAvailable())
  {
    wxTextInputStream tis(*GetInputStream());
    
    while (IsInputAvailable())
    {
      m_Output << tis.GetChar();
    }
  }
  else if (IsErrorAvailable())
  {
    wxTextInputStream tis(*GetErrorStream());
    
    while (IsErrorAvailable())
    {
      m_Output << tis.GetChar();
    }
  }

  if (m_Output.empty())
  {
    return false;
  }
  
  wxStringTokenizer tkz(m_Output, wxTextFile::GetEOL());
  
  while (tkz.HasMoreTokens())
  {
    HandleLine(tkz.GetNextToken());
  }
  
  m_Output.clear();
  
  return true;
}

int wxExProcess::ConfigDialog(
  wxWindow* parent,
  const wxString& title,
  bool modal)
{
  std::vector<wxExConfigItem> v;

  v.push_back(wxExConfigItem(
    _("Process"), 
    CONFIG_COMBOBOX, 
    wxEmptyString,
    true));

  v.push_back(wxExConfigItem(
    m_WorkingDirKey, 
    CONFIG_COMBOBOXDIR, 
    wxEmptyString,
    true,
    1000));

  if (modal)
  {
    return wxExConfigDialog(parent,
      v,
      title).ShowModal();
  }
  else
  {
    wxExConfigDialog* dlg = new wxExConfigDialog(
      parent,
      v,
      title);
      
    return dlg->Show();
  }
}

long wxExProcess::Execute(
  const wxString& command_to_execute,
  int flags,
  const wxString& wd)
{
  wxString command(command_to_execute);
  
  if (command.empty())
  {
    if (wxExConfigFirstOf(_("Process")).empty())
    {
      if (ConfigDialog(wxTheApp->GetTopWindow()) == wxID_CANCEL)
      {
        return -2;
      }
    }
    
    command = wxExConfigFirstOf(_("Process"));
  }
  else
  {
    wxConfigBase::Get()->Write(_("Process"), command);
  }

  if (m_Dialog == NULL)
  {
    m_Dialog = new wxExSTCEntryDialog(
      wxTheApp->GetTopWindow(),
      command,
      wxEmptyString,
      wxEmptyString,
      wxOK,
      true);
      
    m_Dialog->GetSTCShell()->SetEventHandler(this);
  }
      
  const struct wxExecuteEnv env = {
    (wd.empty() ? wxExConfigFirstOf(m_WorkingDirKey): wd), 
    wxEnvVariableHashMap()};
    
  if (!(flags & wxEXEC_SYNC))
  { 
    if (!ReportCreate())
    {
      return -1; 
    }

    // For asynchronous execution, however, the return value is the process id and zero 
    // value indicates that the command could not be executed
    const long pid = wxExecute(command, flags, this, &env);

    if (pid > 0)
    {
      wxLogVerbose("Execute: " + command);
      
      m_Dialog->GetSTCShell()->EnableShell(true);
    
      CheckInput();
      
      m_Timer->Start(100); // each 100 milliseconds
    }
    
    return pid;
  }
  else
  {
    wxArrayString output;
    wxArrayString errors;
    long retValue;
    
    m_Dialog->GetSTCShell()->EnableShell(false);
    
    // Call wxExecute to execute the command and
    // collect the output and the errors.
    if ((retValue = wxExecute(
      command,
      output,
      errors,
      flags,
      &env)) != -1)
    {
      wxLogVerbose("Execute: " + command);
    }

    // We have an error if the command could not be executed.  
    m_Error = (retValue == -1);
    m_Output = wxJoin(errors, '\n') + wxJoin(output, '\n');
  
    return retValue;
  }
}

void wxExProcess::HideDialog()
{
  if (m_Dialog != NULL)
  {
    m_Dialog->Hide();
  }
}

void wxExProcess::HandleLine(const wxString& line)
{
  wxString lineno;
  wxString path;

  // Check on error in php script output.
  std::vector <wxString> v;

  if (wxExMatch(".*in (.*) on line (.*)", m_Output, v) > 1)
  {
    path = v[0];
    lineno = v[1];
  }
  else
  {
    // Check on error in gcc output (and some others).
    wxStringTokenizer tkz(line, ':');
    path = tkz.GetNextToken();

    if (tkz.HasMoreTokens())
    {
      lineno = tkz.GetNextToken();
    }
  }

  if (atoi(lineno.c_str()) == 0)
  {
    lineno.clear();
  }
    
  if (!wxFileExists(path))
  {
    lineno.clear();
    path.clear();
  }
  
  if (!line.empty())
  {
    ReportAdd(line, path, lineno);
  }
}
  
bool wxExProcess::IsRunning() const
{
  if (GetPid() <= 0)
  {
    return false;
  }

  return Exists(GetPid());
}

bool wxExProcess::IsSelected() const
{
  return !wxExConfigFirstOf(_("Process")).empty();
}

wxKillError wxExProcess::Kill(wxSignal sig)
{
  if (!IsRunning())
  {
    return wxKILL_NO_PROCESS;
  }

  m_Timer->Stop();
  
  wxLogStatus(_("Stopped"));

  DeletePendingEvents();
  
  m_Dialog->Hide();

  return wxProcess::Kill(GetPid(), sig);
}

void  wxExProcess::OnCommand(wxCommandEvent& event)
{
  switch (event.GetId())
  {
  case ID_SHELL_COMMAND:
    {
    // send command to process
    wxTextOutputStream os(*GetOutputStream());
    os.WriteString(event.GetString() + "\n");
    
    m_Dialog->GetSTCShell()->AddText(m_Dialog->GetSTCShell()->GetEOL());
    m_Dialog->GetSTCShell()->Prompt();
    }
    break;

  case ID_SHELL_COMMAND_STOP:
    Kill();
    break;
    
  default: wxFAIL; break;
  }
}
  
void wxExProcess::OnTerminate(int pid, int status)
{
  m_Timer->Stop();

  wxLogStatus(_("Ready"));
  wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, ID_TERMINATED_PROCESS);
  wxPostEvent(wxTheApp->GetTopWindow(), event);
  
  // Collect remaining input.
  CheckInput();
}

void wxExProcess::OnTimer(wxTimerEvent& event)
{
  CheckInput();
}

bool wxExProcess::ReportAdd(
  const wxString& line, 
  const wxString& path,
  const wxString& lineno)
{
  m_Dialog->GetSTCShell()->AddText(line);
  m_Dialog->GetSTCShell()->Prompt();
  return true;
}

bool wxExProcess::ReportCreate()
{
  m_Dialog->SetTitle(wxExConfigFirstOf(_("Process")));
  m_Dialog->GetSTCShell()->ClearAll();
  m_Dialog->GetSTCShell()->Prompt();
  m_Dialog->Show();
  return true;
}

#if wxUSE_GUI
void wxExProcess::ShowOutput(const wxString& caption) const
{
  if (!m_Error)
  {
    if (m_Dialog != NULL)
    {
      m_Dialog->GetSTC()->SetText(m_Output);
      m_Dialog->SetTitle(caption.empty() ? wxExConfigFirstOf(_("Process")): caption);
      m_Dialog->Show();
    }
    else if (!m_Output.empty())
    {
      wxLogMessage(m_Output);
    }
  }
  else
  {
    // Executing command failed, so no output,
    // show failing command.
    if (!wxExConfigFirstOf(_("Process")).empty())
    {
      wxLogError(wxExConfigFirstOf(_("Process")));
    }
  }
}
#endif
