/******************************************************************************\
* File:          vcs.h
* Purpose:       Declaration of wxExVCS class
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 1998-2009, Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#ifndef _EXVCS_H
#define _EXVCS_H

#include <map>
#include <wx/filename.h>
#include <wx/xml/xml.h>
#include <wx/extension/command.h>
#include <wx/extension/vcscommand.h>
#include <wx/extension/vcsentry.h>

class wxMenu;

/// This class collects all vcs handling.
/// The VCS entries are read in from vcs.xml, this is done
/// automatically during the first Get call.
class WXDLLIMPEXP_BASE wxExVCS : public wxExCommand
{
public:
  // The vcs id's here can be set using the config dialog, and are not
  // present in the vcs.xml. These enums should be public,
  // as new entries should start after the last one here.
  enum
  {
    VCS_NONE = 0, // no version control
    VCS_AUTO, // uses the VCS appropriate for current file
  };

  /// Constructor for vcs from specified filename.
  /// This must be an existing xml file containing all vcs.
  /// It does not Read this file, however if you use the global Get,
  /// it both constructs and reads the vcs.
  wxExVCS(const wxFileName& filename);

  /// Constructor, specify the menu command id and a file.
  wxExVCS(int menu_id, const wxString& file = wxEmptyString);
  
  /// Constructor, specify the menu command id and several files.
  wxExVCS(int menu_id, const wxArrayString& files);
  
#if wxUSE_GUI
  /// Builds a menu, default assumes it is a popup menu.
  /// Returns number of items in menu.
  /// Sets the files member if you filename is ok.
  int BuildMenu(
    int base_id, 
    wxMenu* menu, 
    const wxFileName& filename = wxFileName(),
    bool is_popup = true);
#endif

#if wxUSE_GUI
  /// Shows a dialog with options, returns dialog return code.
  int ConfigDialog(
    wxWindow* parent,
    const wxString& title = _("Set VCS")) const;
#endif    

  /// Returns true if specified filename (a path) is a vcs directory.
  bool DirExists(const wxFileName& filename) const;

  /// Executes the vcs command, and collects the output.
  long Execute();

#if wxUSE_GUI
  /// Shows a dialog and executes the vcs command if not cancelled.
  /// If no fullpath was specified, a dialog with base folder is shown, 
  /// otherwise the specified fullpath is used for getting vcs contents from.
  /// Returns wxID_CANCEL if dialog was cancelled, an execute error occurred, 
  /// or there is no output collected. Returns wxID_OK if okay (use GetError
  /// to check whether the output contains errors or normal info).
  wxStandardID ExecuteDialog(wxWindow* parent);
#endif    

  /// Returns the vcs object.
  static wxExVCS* Get(bool createOnDemand = true);

  /// Gets the flags and command (without the 'vcs') used to get the output.
  const wxString& GetCommandWithFlags() const {return m_CommandWithFlags;};
  
  /// Gets the xml filename.
  const wxFileName& GetFileName() {return m_FileName;};

  /// Returns true if this command can behave like
  /// opening a file.  
  bool IsOpenCommand() const {
    return m_Command.IsOpen();};

  /// Reads all vcs (first clears them) from file.
  /// Returns true if the file could be read and loaded as valid xml file.
  bool Read();

#if wxUSE_GUI
  /// Combines all in one method. Calls the ExecuteDialog,
  /// and calls ShowOutput if return code was wxID_OK.
  /// Returns return code from ExecuteDialog.
  wxStandardID Request(wxWindow* parent);
#endif  

  /// Sets the object as the current one, returns the pointer 
  /// to the previous current object 
  /// (both the parameter and returned value may be NULL). 
  static wxExVCS* Set(wxExVCS* vcs);

#if wxUSE_GUI
  virtual void ShowOutput(const wxString& caption = wxEmptyString) const;
#endif

  /// Does current vcs allow keyword expansion.
  bool SupportKeywordExpansion() const;

  /// Returns true if VCS usage is set in the config.
  bool Use() const;
private:
  bool CheckPath(const wxString& vcs, const wxFileName& fn) const;
  bool CheckPathAll(const wxString& vcs, const wxFileName& fn) const;
  const wxExVCSEntry FindVCSEntry(const wxFileName& filename) const;
  void Initialize(int command_id);
  int ShowDialog(wxWindow* parent);
  bool UseFlags() const {
    return !m_Command.IsHelp();};
  bool UseSubcommand() const {
    return m_Command.IsHelp();};
  
  wxExVCSCommand m_Command;

  wxString m_Caption;
  wxString m_CommandWithFlags;
  wxString m_FlagsKey;

  static std::map<wxString, wxExVCSEntry> m_Entries;
  static wxArrayString m_Files;
  static wxFileName m_FileName;
  static wxExVCS* m_Self;
};
#endif