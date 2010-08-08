////////////////////////////////////////////////////////////////////////////////
// Name:      vi.cpp
// Purpose:   Implementation of class wxExVi
// Author:    Anton van Wezenbeek
// Created:   2009-11-21
// RCS-ID:    $Id$
// Copyright: (c) 2009 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/tokenzr.h> 
#include <wx/extension/vi.h>
#include <wx/extension/configdlg.h>
#include <wx/extension/file.h>
#include <wx/extension/frame.h>
#include <wx/extension/frd.h>
#include <wx/extension/lexers.h>
#include <wx/extension/stc.h>
#include <wx/extension/util.h>

#if wxUSE_GUI

wxExConfigDialog* wxExVi::m_CommandDialog = NULL;
wxExConfigDialog* wxExVi::m_FindDialog = NULL;
wxString wxExVi::m_LastCommand;
wxString wxExVi::m_LastFindCharCommand;

wxExVi::wxExVi(wxExSTC* stc)
  : m_STC(stc)
  , m_IsActive(false)
  , m_MarkerSymbol(0)
  , m_IndicatorYank(0)
  , m_InsertMode(false)
  , m_InsertRepeatCount(1)
  , m_SearchFlags(wxSTC_FIND_REGEXP | wxFR_MATCHCASE)
  , m_SearchForward(true)
{
}

void wxExVi::Delete(int lines) const
{
  if (m_STC->GetReadOnly())
  {
    return;
  }

  const auto line = m_STC->LineFromPosition(m_STC->GetCurrentPos());
  const auto start = m_STC->PositionFromLine(line);
  const auto end = m_STC->PositionFromLine(line + lines);

  m_STC->SetSelectionStart(start);

  if (end != -1)
  {
    m_STC->SetSelectionEnd(end);
  }
  else
  {
    m_STC->DocumentEndExtend();
  }

#if wxUSE_STATUSBAR
  const auto end_line = m_STC->LineFromPosition(m_STC->GetCurrentPos());
#endif

  m_STC->Cut();

#if wxUSE_STATUSBAR
  if (lines >= 2)
  {
    wxExFrame::StatusText(
      wxString::Format(_("%d fewer lines"), end_line - line));
  }
#endif
}

bool wxExVi::Delete(
  const wxString& begin_address, 
  const wxString& end_address)
{
  if (m_STC->GetReadOnly())
  {
    return false;
  }

  if (!SetSelection(begin_address, end_address))
  {
    return false;
  }

  const auto lines = wxExGetNumberOfLines(m_STC->GetSelectedText());
  
  m_STC->Cut();

  if (begin_address.StartsWith("'"))
  {
    DeleteMarker(begin_address.GetChar(1));
  }

  if (end_address.StartsWith("'"))
  {
    DeleteMarker(end_address.GetChar(1));
  }

#if wxUSE_STATUSBAR
  if (lines >= 2)
  {
    wxExFrame::StatusText(wxString::Format(_("%d fewer lines"), lines));
  }
#endif

  return true;
}

void wxExVi::DeleteMarker(const wxUniChar& marker)
{
  const auto it = m_Markers.find(marker);

  if (it != m_Markers.end())
  {
    m_STC->MarkerDelete(it->second, m_MarkerSymbol.GetNo());
    m_Markers.erase(it);
  }
}

bool wxExVi::DoCommand(const wxString& command, bool dot)
{
  if (command.empty())
  {
    return false;
  }
  
  if (command.StartsWith(":"))
  {
    if (command.length() > 1)
    {
      // This is a previous entered command.
      return DoCommandRange(command);
    }
    else
    {
      // A command will follow.
      DoCommandLine();
      return true;
    }
  }
          
  auto repeat = atoi(command.c_str());

  if (repeat == 0)
  {
    repeat++;
  }
  
  bool handled = true;

  // Handle multichar commands.
  if (command.EndsWith("cw") && !m_STC->GetReadOnly())
  {
    for (auto i = 0; i < repeat; i++) m_STC->WordRightExtend();

    if (dot && !m_InsertText.empty())
    {
      m_STC->ReplaceSelection(m_InsertText);
    }
    else
    {
      InsertMode();
    }
  }
  else if (command == "cc" && !m_STC->GetReadOnly())
  {
    m_STC->Home();
    m_STC->DelLineRight();

    if (dot && !m_InsertText.empty())
    {
      m_STC->ReplaceSelection(m_InsertText);
    }
    else
    {
      InsertMode();
    }
  }
  else if (command.EndsWith("dd") && !m_STC->GetReadOnly())
  {
    Delete(repeat);
  }
  else if (command == "d0" && !m_STC->GetReadOnly())
  {
    m_STC->HomeExtend();
    m_STC->Cut();
  }
  else if (command == "d$" && !m_STC->GetReadOnly())
  {
    m_STC->LineEndExtend();
    m_STC->Cut();
  }
  else if (command.EndsWith("dw") && !m_STC->GetReadOnly())
  {
    m_STC->BeginUndoAction();
    const auto start = m_STC->GetCurrentPos();
    for (auto i = 0; i < repeat; i++) 
      m_STC->WordRight();
    m_STC->SetSelection(start, m_STC->GetCurrentPos());
    m_STC->Cut();
    m_STC->EndUndoAction();
  }
  else if (command.Matches("*f?"))
  {
    for (auto i = 0; i < repeat; i++) 
      m_STC->FindNext(command.Last(), m_SearchFlags);
    m_LastFindCharCommand = command;
  }
  else if (command.Matches("*F?"))
  {
    for (auto i = 0; i < repeat; i++) 
      m_STC->FindNext(command.Last(), m_SearchFlags, false);
    m_LastFindCharCommand = command;
  }
  else if (command.Matches("*J"))
  {
    m_STC->BeginUndoAction();
    m_STC->SetTargetStart(m_STC->PositionFromLine(m_STC->GetCurrentLine()));
    m_STC->SetTargetEnd(m_STC->PositionFromLine(m_STC->GetCurrentLine() + repeat));
    m_STC->LinesJoin();
    m_STC->EndUndoAction();
 }
  else if (command.Matches("m?"))
  {
    DeleteMarker(command.Last());
    m_Markers[command.Last()] = m_STC->GetCurrentLine();
    m_STC->MarkerAdd(m_STC->GetCurrentLine(), m_MarkerSymbol.GetNo());
  }
  else if (command.Matches("*r?") && !m_STC->GetReadOnly())
  {
    m_STC->wxStyledTextCtrl::Replace(
      m_STC->GetCurrentPos(), 
      m_STC->GetCurrentPos() + repeat, 
      wxString(command.Last(), repeat));
  }
  else if (command.EndsWith("yw"))
  {
    const auto start = m_STC->GetCurrentPos();
    for (auto i = 0; i < repeat; i++) 
      m_STC->WordRight();
    m_STC->CopyRange(start, m_STC->GetCurrentPos());
    SetIndicator(m_IndicatorYank, start, m_STC->GetCurrentPos());
    m_STC->GotoPos(start);
  }
  else if (command.EndsWith("yy"))
  {
    Yank(repeat);
  }
  else if (command == "zc" || command == "zo")
  {
    const auto level = m_STC->GetFoldLevel(m_STC->GetCurrentLine());
    const auto line_to_fold = (level & wxSTC_FOLDLEVELHEADERFLAG) ?
      m_STC->GetCurrentLine(): m_STC->GetFoldParent(m_STC->GetCurrentLine());

    if (m_STC->GetFoldExpanded(line_to_fold) && command == "zc")
      m_STC->ToggleFold(line_to_fold);
    else if (!m_STC->GetFoldExpanded(line_to_fold) && command == "zo")
      m_STC->ToggleFold(line_to_fold);
  }
  else if (command == "zE")
  {
    m_STC->SetProperty("fold", "0");
  }
  else if (command == "zf")
  {
    m_STC->SetProperty("fold", "1");
  }
  else if (command == "ZZ")
  {
    wxPostEvent(wxTheApp->GetTopWindow(), 
      wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, wxID_SAVE));
    wxPostEvent(wxTheApp->GetTopWindow(), 
      wxCloseEvent(wxEVT_CLOSE_WINDOW));
  }
  else if (command.EndsWith(">>") && !m_STC->GetReadOnly())
  {
    m_STC->Indent(repeat);
  }
  else if (command.EndsWith("<<") && !m_STC->GetReadOnly())
  {
    m_STC->Indent(repeat, false);
  }
  else if (command.Matches("'?"))
  {
    const auto it = m_Markers.find(command.Last());

    if (it != m_Markers.end())
    {
      m_STC->GotoLine(it->second);
    }
    else
    {
      wxBell();
    }
  }
  else
  {
    switch ((int)command.Last())
    {
      case 'a': 
      case 'i': 
      case 'o': 
      case 'A': 
      case 'C': 
      case 'I': 
      case 'O': 
        InsertMode(command.Last(), repeat, false, dot); 
        break;
      case 'R': 
        InsertMode(command.Last(), repeat, true, dot); 
        break;

      case '0': 
        if (command.length() == 1)
        {
          m_STC->Home(); 
        }
        else
        {
          handled = false;
        }
        break;
      case 'b': for (auto i = 0; i < repeat; i++) m_STC->WordLeft(); break;
      case 'e': for (auto i = 0; i < repeat; i++) m_STC->WordRightEnd(); break;
      case 'g': m_STC->DocumentStart(); break;
      case 'h': 
        for (auto i = 0; i < repeat; i++) m_STC->CharLeft(); 
        break;
      case 'j': 
        for (auto i = 0; i < repeat; i++) m_STC->LineDown(); 
        break;
      case 'k': 
        for (auto i = 0; i < repeat; i++) m_STC->LineUp(); 
        break;
      case 'l': 
      case ' ': 
        for (auto i = 0; i < repeat; i++) m_STC->CharRight(); 
        break;
      case 'n': 
        for (auto i = 0; i < repeat; i++) 
          m_STC->FindNext(
            wxExFindReplaceData::Get()->GetFindString(), 
            m_SearchFlags, 
            m_SearchForward);
        break;

      case 'p': Put(true); break;
      case 'P': Put(false); break;

      case 'w': for (auto i = 0; i < repeat; i++) m_STC->WordRight(); break;
      case 'u': m_STC->Undo(); break;
      case 'x': 
        for (auto i = 0; i < repeat; i++) 
        {
          m_STC->CharRight();
          m_STC->DeleteBack(); 
        }
        break;

      case 'D': 
        if (!m_STC->GetReadOnly())
        {
          m_STC->LineEndExtend();
          m_STC->Cut();
          }
        break;
      case 'G': 
        if (repeat > 1)
        {
          m_STC->GotoLine(repeat - 1);
        }
        else
        {
          m_STC->DocumentEnd();
        }
        break;
      case 'H': m_STC->GotoLine(m_STC->GetFirstVisibleLine());
        break;
      case 'M': m_STC->GotoLine(
        m_STC->GetFirstVisibleLine() + m_STC->LinesOnScreen() / 2);
        break;
      case 'L': m_STC->GotoLine(
        m_STC->GetFirstVisibleLine() + m_STC->LinesOnScreen()); 
        break;
      case 'N': 
        for (auto i = 0; i < repeat; i++) 
          m_STC->FindNext(
            wxExFindReplaceData::Get()->GetFindString(), 
            m_SearchFlags, 
            !m_SearchForward);
        break;
      case 'X': for (auto i = 0; i < repeat; i++) m_STC->DeleteBack(); break;

      case '/': 
      case '?': 
        DoCommandFind(command.Last());
        break;

      case '.': DoCommand(m_LastCommand, true); break;
      case ';': DoCommand(m_LastFindCharCommand, false); break;
      case '~': ToggleCase(); break;
      case '$': m_STC->LineEnd(); break;
      case '{': m_STC->ParaUp(); break;
      case '}': m_STC->ParaDown(); break;
      case '%': GotoBrace(); break;

      case '*': FindWord(); break;
      case '#': FindWord(false); break;
      
      case 2:  // ^b
        for (auto i = 0; i < repeat; i++) m_STC->PageUp(); 
        break;
      case 7:  // ^g (^f is not possible, already find accel key)
        for (auto i = 0; i < repeat; i++) m_STC->PageDown(); 
        break;
      case 16: // ^p (^y is not possible, already redo accel key)
        for (auto i = 0; i < repeat; i++) m_STC->LineScrollUp(); 
        break;
      case 17: // ^q (^n is not possible, already new doc accel key)
        for (auto i = 0; i < repeat; i++) m_STC->LineScrollDown(); 
        break;

      default:
        handled = false;
    }
  }

  return handled;
}

void wxExVi::DoCommandFind(const wxUniChar& c)
{
  const wxString title = "vi " + wxString(c);

  if (m_FindDialog == NULL)
  {
    // Do not use stc as parent, as that might be destroyed.
    m_FindDialog = wxExConfigComboBoxDialog(
      wxTheApp->GetTopWindow(), 
      title, 
      wxExFindReplaceData::Get()->GetTextFindWhat());
  }
  else
  {
    m_FindDialog->Reload();
    m_FindDialog->SetTitle(title);
  }

  m_FindDialog->SelectAll();

  if (m_FindDialog->ShowModal() == wxID_CANCEL)
  {
    return;
  }

  m_SearchForward = (c == '/');
  
  m_STC->FindNext(
    wxExConfigFirstOf(wxExFindReplaceData::Get()->GetTextFindWhat()), 
    m_SearchFlags, 
    m_SearchForward);
}

void wxExVi::DoCommandLine()
{
  const wxString item = "commandline"; // do not translate

  if (m_CommandDialog == NULL)
  {
    m_CommandDialog = wxExConfigComboBoxDialog(
      wxTheApp->GetTopWindow(), 
      "vi :", 
      item);
  }
  
  m_CommandDialog->SelectAll();

  if (m_CommandDialog->ShowModal() == wxID_CANCEL)
  {
    return;
  }

  const wxString val = wxExConfigFirstOf(item);

  if (val.empty())
  {
    return;
  }

  const wxString command = ":" + val;

  if (command == ":$")
  {
    m_STC->DocumentEnd();
  }
  else if (command == ":close")
  {
    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, wxID_CLOSE);
    wxPostEvent(wxTheApp->GetTopWindow(), event);
  }
  else if (command == ":d")
  {
    Delete(1);
  }
  else if (command.StartsWith(":e"))
  {
    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, wxID_OPEN);
    if (command.Contains(" "))
    {
      event.SetString(command.AfterFirst(' '));
    }
    wxPostEvent(wxTheApp->GetTopWindow(), event);
  }
  else if (command == ":n")
  {
    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, ID_EDIT_NEXT);
    wxPostEvent(wxTheApp->GetTopWindow(), event);
  }
  else if (command == ":prev")
  {
    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, ID_EDIT_PREVIOUS);
    wxPostEvent(wxTheApp->GetTopWindow(), event);
  }
  else if (command == ":q")
  {
    wxCloseEvent event(wxEVT_CLOSE_WINDOW);
    wxPostEvent(wxTheApp->GetTopWindow(), event);
  }
  else if (command == ":q!")
  {
    wxCloseEvent event(wxEVT_CLOSE_WINDOW);
    event.SetCanVeto(false); 
    wxPostEvent(wxTheApp->GetTopWindow(), event);
  }
  else if (command.StartsWith(":r"))
  {
    wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, ID_EDIT_READ);

    if (command.Contains(" "))
    {
      event.SetString(command.AfterFirst(' '));
    }

    wxPostEvent(wxTheApp->GetTopWindow(), event);
  }
  else if (command.StartsWith(":w"))
  {
    if (command.Contains(" "))
    {
      wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, wxID_SAVEAS);
      event.SetString(command.AfterFirst(' '));
      wxPostEvent(wxTheApp->GetTopWindow(), event);
    }
    else
    {
      wxCommandEvent event(wxEVT_COMMAND_MENU_SELECTED, wxID_SAVE);
      wxPostEvent(wxTheApp->GetTopWindow(), event);
    }
  }
  else if (command == ":x")
  {
    wxPostEvent(wxTheApp->GetTopWindow(), 
      wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, wxID_SAVE));
      
    wxPostEvent(wxTheApp->GetTopWindow(), 
      wxCloseEvent(wxEVT_CLOSE_WINDOW));
  }
  else if (command == ":y")
  {
    Yank(1);
  }
  else if (command.Last() == '=')
  {
    m_STC->CallTipShow(
      m_STC->GetCurrentPos(), 
      wxString::Format("%s%d",
        command.AfterFirst(':').c_str(), 
        ToLineNumber(command.AfterFirst(':').BeforeLast('='))));
  }
  else if (command.AfterFirst(':').IsNumber())
  {
    m_STC->GotoLine(atoi(command.AfterFirst(':').c_str()) - 1);
  }
  else
  {
    if (DoCommandRange(command))
    {
      m_LastCommand = command;
      m_InsertText.clear();
    }
    else
    {
      wxBell();
    }
  }
}

bool wxExVi::DoCommandRange(const wxString& command)
{
  // :[address] m destination
  // :[address] s [/pattern/replacement/] [options] [count]
  wxStringTokenizer tkz(command.AfterFirst(':'), "dmsyw");
  
  if (!tkz.HasMoreTokens())
  {
    return false;
  }

  const wxString address = tkz.GetNextToken();
  const wxChar cmd = tkz.GetLastDelimiter();

  wxString begin_address;
  wxString end_address;
    
  if (address == ".")
  {
    begin_address = address;
    end_address = address;
  }
  else if (address == "%")
  {
    begin_address = "1";
    end_address = "$";
  }
  else
  {
    begin_address = address.BeforeFirst(',');
    end_address = address.AfterFirst(',');
  }

  switch (cmd)
  {
  case 0: 
    return false; break;
    
  case 'd':
    return Delete(begin_address, end_address);
    break;
  case 'm':
    return Move(begin_address, end_address, tkz.GetString());
    break;
  case 's':
    {
    wxStringTokenizer tkz(tkz.GetString(), "/");

    if (!tkz.HasMoreTokens())
    {
      return false;
    }

    tkz.GetNextToken(); // skip empty token
    const wxString pattern = tkz.GetNextToken();
    const wxString replacement = tkz.GetNextToken();
  
    return Substitute(begin_address, end_address, pattern, replacement);
    }
    break;
  case 'y':
    return Yank(begin_address, end_address);
    break;
  case 'w':
    return Write(begin_address, end_address, tkz.GetString());
    break;
  default:
    wxFAIL;
    return false;
  }
}

void wxExVi::FindWord(bool find_next) const
{
  const auto start = m_STC->WordStartPosition(m_STC->GetCurrentPos(), true);
  const auto end = m_STC->WordEndPosition(m_STC->GetCurrentPos(), true);
  
  wxExFindReplaceData::Get()->SetFindString(
    "\\<" + m_STC->GetTextRange(start, end) + "\\>");
  
  m_STC->FindNext(
    wxExFindReplaceData::Get()->GetFindString(), m_SearchFlags, find_next);
}

void wxExVi::GotoBrace() const
{
  auto brace_match = m_STC->BraceMatch(m_STC->GetCurrentPos());
          
  if (brace_match != wxSTC_INVALID_POSITION)
  {
    m_STC->GotoPos(brace_match);
  }
  else
  {
    brace_match = m_STC->BraceMatch(m_STC->GetCurrentPos() - 1);
            
    if (brace_match != wxSTC_INVALID_POSITION)
    {
      m_STC->GotoPos(brace_match);
    }
  }
}

void wxExVi::InsertMode(
  const wxUniChar c, 
  int repeat, 
  bool overtype,
  bool dot)
{
  if (!m_STC->GetReadOnly())
  {
    if (!dot)
    {
      m_InsertMode = true;
      m_InsertText.clear();
      m_InsertRepeatCount = repeat;
      m_STC->BeginUndoAction();
    }

    switch ((int)c)
    {
      case 'a': m_STC->CharRight(); 
        break;

      case 'i': 
        break;

      case 'o': 
        m_STC->LineEnd(); 
        m_STC->NewLine(); 
        break;
      case 'A': m_STC->LineEnd(); 
        break;

      case 'C': 
      case 'R': 
        m_STC->SetSelectionStart(m_STC->GetCurrentPos());
        m_STC->SetSelectionEnd(m_STC->GetLineEndPosition(m_STC->GetCurrentLine()));
        break;

      case 'I': 
        m_STC->Home(); 
        break;

      case 'O': 
        m_STC->Home(); 
        m_STC->NewLine(); 
        m_STC->LineUp(); 
        break;

      default: wxFAIL;
    }

    if (dot)
    {
      if (c == 'R' || c == 'C')
      {
        m_STC->ReplaceSelection(m_InsertText);
      }
      else
      {
        m_STC->AddText(m_InsertText);
      }
    }
    else
    {
      m_STC->SetOvertype(overtype);
    }
  }
}

bool wxExVi::Move(
  const wxString& begin_address, 
  const wxString& end_address, 
  const wxString& destination)
{
  if (m_STC->GetReadOnly())
  {
    return false;
  }

  const auto dest_line = ToLineNumber(destination);

  if (dest_line == 0)
  {
    return false;
  }

  if (!SetSelection(begin_address, end_address))
  {
    return false;
  }

  if (begin_address.StartsWith("'"))
  {
    DeleteMarker(begin_address.GetChar(1));
  }

  if (end_address.StartsWith("'"))
  {
    DeleteMarker(end_address.GetChar(1));
  }

  m_STC->BeginUndoAction();

  m_STC->Cut();
  m_STC->GotoLine(dest_line - 1);
  m_STC->Paste();

  m_STC->EndUndoAction();
  
#if wxUSE_STATUSBAR
  const auto lines = wxExGetNumberOfLines(m_STC->GetSelectedText());
  if (lines >= 2)
  {
    wxExFrame::StatusText(wxString::Format(_("%d lines moved"), lines));
  }
#endif

  return true;
}

bool wxExVi::OnChar(const wxKeyEvent& event)
{
  if (!m_IsActive)
  {
    return true;
  }
  else if (m_InsertMode)
  {
    m_InsertText += event.GetUnicodeKey();

    return true;
  }
  else
  {
    if (!(event.GetModifiers() & wxMOD_ALT))
    {
      m_Command += event.GetUnicodeKey();

      if (DoCommand(m_Command, false))
      {
        if ((m_Command.length() > 1 && !m_Command.Matches("m?")) || 
            m_Command == "a" || 
            m_Command == "i" || 
            m_Command == "o" || 
            m_Command == "p" ||
            m_Command == "x" ||
            m_Command == "A" || 
            m_Command == "C" || 
            m_Command == "D" || 
            m_Command == "I" || 
            m_Command == "O" || 
            m_Command == "R" || 
            m_Command == "X" || 
            m_Command == "~"
            )
        {
          m_LastCommand = m_Command;
        }

        m_Command.clear();
      }
      return false;
    }
    else
    {
      return true;
    }
  }
}

bool wxExVi::OnKeyDown(const wxKeyEvent& event)
{
  if (!m_IsActive)
  {
    return false;
  }

  bool handled = true;

  switch (event.GetKeyCode())
  {
    case WXK_BACK:
      if (!m_InsertMode)
      {
        m_STC->CharLeft();
        handled = true;
      }
      else
      {
        handled = false;
      }
      break;
      
    case WXK_ESCAPE:
      if (m_InsertMode)
      {
        // Add extra inserts if necessary.        
        for (auto i = 1; i < m_InsertRepeatCount; i++)
        {
          m_STC->AddText(m_InsertText);
        }
        
        m_STC->EndUndoAction();
        m_InsertMode = false;
      }
      else
      {
        wxBell();
      }

      m_Command.clear();
      break;
      
    case WXK_RETURN:
      if (!m_InsertMode)
      {
        auto repeat = atoi(m_Command.c_str());

        if (repeat == 0)
        {
          repeat++;
        }
  
        for (auto i = 0; i < repeat; i++) m_STC->LineDown();

        m_Command.clear();
      }
      else
      {
        m_InsertText += event.GetUnicodeKey();
        handled = false;
      }
      break;
      
   default: handled = false;
  }

  return !handled;
}

void wxExVi::Put(bool after) const
{
  const auto lines = wxExGetNumberOfLines(wxExClipboardGet());
  
  if (lines > 1)
  {
    if (after) m_STC->LineDown();
    m_STC->Home();
  }

  m_STC->Paste();

  if (lines > 1 && after)
  {
    m_STC->LineUp();
  }
  
  m_STC->IndicatorClearRange(0, m_STC->GetLength() - 1);
}        

void wxExVi::SetIndicator(
  const wxExIndicator& indicator, 
  int start, 
  int end) const
{
  if (!wxExLexers::Get()->IndicatorIsLoaded(indicator))
  {
    return;
  }

  m_STC->SetIndicatorCurrent(indicator.GetNo());

  // When yanking, old one can be cleared.
  // For put it is useful to keep them.
  if (indicator == m_IndicatorYank)
  {
    m_STC->IndicatorClearRange(0, m_STC->GetLength() - 1);
  }

  m_STC->IndicatorFillRange(start, end - start);
}

bool wxExVi::SetSelection(
  const wxString& begin_address, 
  const wxString& end_address) const
{
  const auto begin_line = ToLineNumber(begin_address);
  const auto end_line = ToLineNumber(end_address);

  if (begin_line == 0 || 
      end_line == 0 ||
      end_line < begin_line)
  {
    return false;
  }

  m_STC->SetSelectionStart(m_STC->PositionFromLine(begin_line - 1));
  m_STC->SetSelectionEnd(m_STC->PositionFromLine(end_line));

  return true;
}

bool wxExVi::Substitute(
  const wxString& begin_address, 
  const wxString& end_address, 
  const wxString& pattern,
  const wxString& replacement) const
{
  if (m_STC->GetReadOnly())
  {
    return false;
  }

  m_STC->SetSearchFlags(wxSTC_FIND_REGEXP);

  const auto begin_line = ToLineNumber(begin_address);
  const auto end_line = ToLineNumber(end_address);

  if (begin_line == 0 || 
      end_line == 0 || 
      end_line < begin_line)
  {
    return false;
  }

  int nr_replacements = 0;

  m_STC->BeginUndoAction();
  m_STC->SetTargetStart(m_STC->PositionFromLine(begin_line - 1));
  const auto target_end = m_STC->PositionFromLine(end_line);
  m_STC->SetTargetEnd(target_end);

  const bool is_re = m_STC->IsTargetRE(replacement);

  while (m_STC->SearchInTarget(pattern) > 0)
  {
    const auto target_start = m_STC->GetTargetStart();

    if (target_start >= target_end)
    {
      break;
    }

    m_STC->MarkTargetChange();
  
    const auto length = (is_re ? 
      m_STC->ReplaceTargetRE(replacement): 
      m_STC->ReplaceTarget(replacement));

    m_STC->SetTargetStart(target_start + length);
    m_STC->SetTargetEnd(target_end);

    nr_replacements++;
  }

  m_STC->EndUndoAction();

#if wxUSE_STATUSBAR
  wxExFrame::StatusText(wxString::Format(_("Replaced: %d occurrences of: %s"),
    nr_replacements, pattern.c_str()));
#endif

  return true;
}

void wxExVi::ToggleCase() const
{
  wxString text(m_STC->GetTextRange(
    m_STC->GetCurrentPos(), 
    m_STC->GetCurrentPos() + 1));

  wxIslower(text[0]) ? text.UpperCase(): text.LowerCase();

  m_STC->wxStyledTextCtrl::Replace(
    m_STC->GetCurrentPos(), 
    m_STC->GetCurrentPos() + 1, 
    text);

  m_STC->CharRight();
}

// Returns 0 and bells on error in address, otherwise the vi line number,
// so subtract 1 for stc line number.
int wxExVi::ToLineNumber(const wxString& address) const
{
  wxString filtered_address(address);

  // Check if we are referring to a defined marker.
  int marker = 0;

  if (address.StartsWith("'"))
  {
    auto it = 
      m_Markers.find(address.GetChar(1));

    if (it != m_Markers.end())
    {
      marker = it->second;
    }
    else
    {
      wxBell();
      return 0;
    }

    filtered_address = filtered_address.substr(2);
  }

  int dot = 0;

  if (filtered_address.Contains("."))
  {
    dot = m_STC->GetCurrentLine();
    filtered_address.Replace(".", "");
  }

  int dollar = 0;

  if (filtered_address.Contains("$"))
  {
    dollar = m_STC->GetLineCount();
    filtered_address.Replace("$", "");
  }

  if (!filtered_address.IsNumber()) 
  {
    wxBell();
    return 0;
  }

  // Calculate the line.
  int i = 0;
  
  if (!filtered_address.empty())
  {
    if ((i = atoi(filtered_address.c_str())) == 0)
    {
      wxBell();
      return 0;
    }
  }
  
  const auto line_no = marker + dot + dollar + i + 1;
  
  // Limit the range of what is returned.
  if (line_no <= 0)
  {
    return 1;
  }
  else if (line_no > m_STC->GetLineCount())
  {
    return m_STC->GetLineCount();
  }  
  else
  {
    return line_no;
  }
}

bool wxExVi::Write(
  const wxString& begin_address, 
  const wxString& end_address,
  const wxString& filename) const
{
  const auto begin_line = ToLineNumber(begin_address);
  const auto end_line = ToLineNumber(end_address);

  if (begin_line == 0 || end_line == 0)
  {
    return false;
  }

  const auto start = m_STC->PositionFromLine(begin_line - 1);
  const auto end = m_STC->PositionFromLine(end_line);

  wxFile file(filename, wxFile::write);

  return 
    file.IsOpened() && 
    file.Write(m_STC->GetTextRange(start, end));
}

void wxExVi::Yank(int lines) const
{
  const auto line = m_STC->LineFromPosition(m_STC->GetCurrentPos());
  const auto start = m_STC->PositionFromLine(line);
  const auto end = m_STC->PositionFromLine(line + lines);

  if (end != -1)
  {
    m_STC->CopyRange(start, end);
    SetIndicator(m_IndicatorYank, start, end);
  }
  else
  {
    m_STC->CopyRange(start, m_STC->GetLastPosition());
    SetIndicator(m_IndicatorYank, start, m_STC->GetLastPosition());
  }

#if wxUSE_STATUSBAR
  if (lines >= 2)
  {
    wxExFrame::StatusText(wxString::Format(_("%d lines yanked"), 
      wxExGetNumberOfLines(wxExClipboardGet()) - 1));
  }
#endif
}

bool wxExVi::Yank(
  const wxString& begin_address, 
  const wxString& end_address) const
{
  const auto begin_line = ToLineNumber(begin_address);
  const auto end_line = ToLineNumber(end_address);

  if (begin_line == 0 || end_line == 0)
  {
    return false;
  }

  const auto start = m_STC->PositionFromLine(begin_line - 1);
  const auto end = m_STC->PositionFromLine(end_line);

  m_STC->CopyRange(start, end);
  SetIndicator(m_IndicatorYank, start, end);

#if wxUSE_STATUSBAR
  const auto lines = wxExGetNumberOfLines(wxExClipboardGet()) - 1;
  
  if (lines >= 2)
  {
    wxExFrame::StatusText(wxString::Format(_("%d lines yanked"), lines));
  }
#endif

  return true;
}

#endif // wxUSE_GUI
