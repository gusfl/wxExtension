/******************************************************************************\
* File:          svn.h
* Purpose:       Declaration of exSVN class
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 1998-2009, Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#ifndef _EXSVN_H
#define _EXSVN_H

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

/// SVN types supported.
enum exSvnType
{ 
  SVN_CAT,    ///< svn cat
  SVN_COMMIT, ///< svn commit 
  SVN_DIFF,   ///< svn diff 
  SVN_INFO,   ///< svn info
  SVN_LOG,    ///< svn log 
  SVN_STAT,   ///< svn stat 
};

#if wxUSE_GUI

class exSTCEntryDialog;

/// This class collects all svn handling.
class exSVN
{
public:
  /// Constructor, specify the type of what to get and a fullpath.
  exSVN(exSvnType m_Type, const wxString& fullpath = wxEmptyString);

  /// Gets the contents (GetInfo should already be called).
  const wxString& GetContents() const {return m_Contents;};

  /// Gets info from svn.
  /// If no fullpath was specified, a dialog with base folder is shown, otherwise
  /// the specified fullpath is used for getting svn contents from.
  /// Returns -1 if dialog was cancelled, 0 if okay, or the number of errors 
  /// that were reported by svn otherwise.
  /// If you use show_dialog = false, then the dialog is not shown,
  /// and defaults from the config are used.
  int GetInfo(bool show_dialog = true);

  /// Gets info and if not cancelled shows contents in a dialog.
  /// Returns return code from Get.
  int GetInfoAndShowContents();

  /// Shows contents in a dialog only, GetInfo should already be done.
  void ShowContents();
private:
  const exSvnType m_Type;
  wxString m_Caption;
  wxString m_Command;
  wxString m_Contents;
  const wxString m_FullPath;
  int m_ReturnCode;
  static exSTCEntryDialog* m_STCEntryDialog;
};
#endif
#endif
