/******************************************************************************\
* File:          util.h
* Purpose:       Include file for wxExtension report utilities
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 1998-2009, Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#ifndef _EX_REPORT_UTIL_H
#define _EX_REPORT_UTIL_H

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/aui/auibook.h>
#include <wx/dir.h> // for wxDIR_DEFAULT
#include <wx/filename.h>

class wxExFrameWithHistory;
class wxExListViewFile;

/*! \file */

/// Compares the files, using wxExecute on comparator set in the config.
bool wxExCompareFile(const wxFileName& file1, const wxFileName& file2);

/// Shows a find in files dialog and finds or replaces text in files if chosen.
size_t wxExFindInFiles(bool replace = false);

/// Finds other filenames from the one specified in the same dir structure.
/// Results are put on the list if not null, or in the filename if not null.
bool wxExFindOtherFileName(
  const wxFileName& filename,
  wxExListViewFile* listview,
  wxFileName* lastfile); // in case more files found, only most recent here

/// Do something (id) for all pages on the notebook.
bool wxExForEach(wxAuiNotebook* notebook, int id, const wxFont& font = wxFont());

/// Run make on specified makefile.
bool wxExMake(wxExFrameWithHistory* frame, const wxFileName& makefile);

/// Opens files and updates history, both for files and projects.
void wxExOpenFiles(wxExFrameWithHistory* frame,
  const wxArrayString& files,
  long file_flags = 0,
  int dir_flags = wxDIR_DEFAULT);

/// Shows a dialog and opens selected files and updates history, 
/// both for files and projects.
void wxExOpenFilesDialog(wxExFrameWithHistory* frame,
  long style = wxFD_OPEN | wxFD_MULTIPLE | wxFD_CHANGE_DIR,
  const wxString& wildcards = wxFileSelectorDefaultWildcardStr,
  bool ask_for_continue = false);
#endif
