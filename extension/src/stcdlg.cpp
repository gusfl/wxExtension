////////////////////////////////////////////////////////////////////////////////
// Name:      stcdlg.cpp
// Purpose:   Implementation of class wxExSTCEntryDialog
// Author:    Anton van Wezenbeek
// Copyright: (c) 2013 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/persist/toplevel.h>
#include <wx/extension/stcdlg.h>
#include <wx/extension/process.h>
#include <wx/extension/shell.h>

#if wxUSE_GUI

wxExSTCEntryDialog::wxExSTCEntryDialog(wxWindow* parent,
  const wxString& caption,
  const wxString& text,
  const wxString& prompt,
  long button_style,
  bool use_shell,
  wxWindowID id,
  const wxPoint& pos,
  const wxSize& size, 
  long style,
  const wxString& name)
  : wxExDialog(parent, caption, button_style, id, pos, size, style, name)
  , m_Process(NULL)
{
#if wxUSE_STATTEXT
  if (!prompt.empty())
  {
    // See wxWidgets: src\generic\textdlgg.cpp, use similar bottom border flags.
    AddUserSizer(CreateTextSizer(prompt), 
      wxSizerFlags().DoubleBorder(wxBOTTOM));
  }
#endif

  wxPersistentRegisterAndRestore(this);

  m_STC = (use_shell ?
    new wxExSTCShell(this, text, wxEmptyString, true, -1, wxEmptyString,
      wxExSTC::STC_MENU_DEFAULT | wxExSTC::STC_MENU_OPEN_LINK):
    new wxExSTC(this, text));
  
  m_STC->SetEdgeMode(wxSTC_EDGE_NONE);
  m_STC->ResetMargins();
  m_STC->SetViewEOL(false);
  m_STC->SetViewWhiteSpace(wxSTC_WS_INVISIBLE);

  AddUserSizer(m_STC);

  LayoutSizers();
  
  Bind(wxEVT_CLOSE_WINDOW, [=](wxCloseEvent& event){
    if (m_Process != NULL)
    {
      if (m_Process->IsRunning())
      {
        m_Process->Kill();
      }
    }});

  Bind(wxEVT_BUTTON, [=](wxCommandEvent& event) {
      if (m_Process != NULL)
      {
        if (m_Process->IsRunning())
        {
          m_Process->Kill();
        }
      }
      event.Skip();}, wxID_OK);
  
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
      wxPostEvent(wxTheApp->GetTopWindow(), event);}, wxID_FIND);
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
      wxPostEvent(wxTheApp->GetTopWindow(), event);}, wxID_REPLACE);
}

const wxExLexer* wxExSTCEntryDialog::GetLexer() const
{
  return &m_STC->GetLexer();
}

wxExSTC* wxExSTCEntryDialog::GetSTC()
{
  return m_STC;
}

wxExSTCShell* wxExSTCEntryDialog::GetSTCShell()
{
  return (wxExSTCShell *)m_STC;
}

const wxString wxExSTCEntryDialog::GetText() const 
{
  return m_STC->GetText();
}

const wxCharBuffer wxExSTCEntryDialog::GetTextRaw() const 
{
  return m_STC->GetTextRaw();
}

bool wxExSTCEntryDialog::SetLexer(const wxString& lexer) 
{
  return m_STC->SetLexer(lexer);
}
#endif // wxUSE_GUI
