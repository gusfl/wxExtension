/******************************************************************************\
* File:          appl.h
* Purpose:       Declaration of sample classes for wxExRep
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 1998-2009, Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#include <wx/treectrl.h>
#include <wx/generic/dirctrlg.h>
#include <wx/extension/notebook.h>

/// Derive your application from wxExApp.
class wxExRepSampleApp: public wxExApp
{
public:
  wxExRepSampleApp() {}
private:
  virtual bool OnInit();
  DECLARE_NO_COPY_CLASS(wxExRepSampleApp)
};

/// Use wxExFrameWithHistory.
class wxExRepSampleFrame: public wxExFrameWithHistory
{
public:
  /// Constructor.
  wxExRepSampleFrame(const wxString& title);
protected:
  // Interface from wxExFrameWithHistory.
  virtual wxExListViewFile* Activate(int type, const wxExLexer* lexer = NULL);
  virtual wxExSTCWithFrame* GetCurrentSTC() {return m_STC;};
  virtual bool OpenFile(
    const wxExFileName& file,
    int line_number = 0,
    const wxString& match = wxEmptyString,
    long flags = 0);
  void OnCommand(wxCommandEvent& event);
  void OnTree(wxTreeEvent& event);
private:
  wxExNotebook* m_NotebookWithLists; ///< all listviews
  wxExSTCWithFrame* m_STC;           ///< an stc
  wxGenericDirCtrl* m_DirCtrl;
  DECLARE_EVENT_TABLE()
};
