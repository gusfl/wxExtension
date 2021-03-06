////////////////////////////////////////////////////////////////////////////////
// Name:      app.h
// Purpose:   Declaration of sample classes for wxExtension report
// Author:    Anton van Wezenbeek
// Copyright: (c) 2015 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/extension/app.h>
#include <wx/extension/notebook.h>
#include <wx/extension/stc.h>
#include <wx/extension/report/frame.h>

/// Derive your application from wxExApp.
class wxExRepSampleApp: public wxExApp
{
public:
  wxExRepSampleApp() {}
private:
  /// Override the OnInit.
  virtual bool OnInit();
  DECLARE_NO_COPY_CLASS(wxExRepSampleApp)
};

/// Use wxExFrameWithHistory.
class wxExRepSampleFrame: public wxExFrameWithHistory
{
public:
  /// Constructor.
  wxExRepSampleFrame();
protected:
  // Interface from wxExFrameWithHistory.
  virtual wxExListViewFileName* Activate(
    wxExListViewFileName::wxExListType type, 
    const wxExLexer* lexer = NULL);
  virtual bool AllowClose(wxWindowID id, wxWindow* page);
  virtual wxExListView* GetListView();
  virtual wxExSTC* GetSTC();
  virtual bool OpenFile(
    const wxExFileName& file,
    int line_number = 0,
    const wxString& match = wxEmptyString,
    long flags = 0);
private:
  wxExNotebook* m_NotebookWithLists; ///< all listviews
  wxExProcess* m_Process;
  wxExSTC* m_STC;
};
