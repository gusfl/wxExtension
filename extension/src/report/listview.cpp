////////////////////////////////////////////////////////////////////////////////
// Name:      listview.cpp
// Purpose:   Implementation of class wxExListViewWithFrame
// Author:    Anton van Wezenbeek
// Copyright: (c) 2015 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/config.h>
#include <wx/extension/configdlg.h>
#include <wx/extension/interruptable.h>
#include <wx/extension/lexers.h>
#include <wx/extension/listitem.h>
#include <wx/extension/util.h>
#include <wx/extension/vcs.h>
#include <wx/extension/report/listview.h>
#include <wx/extension/report/defs.h>
#include <wx/extension/report/frame.h>
#include <wx/extension/report/textfile.h>
#include <wx/extension/report/util.h>

wxExListViewWithFrame::wxExListViewWithFrame(wxWindow* parent,
  wxExFrameWithHistory* frame,
  wxExListType type,
  wxWindowID id,
  long menu_flags,
  const wxExLexer* lexer,
  const wxPoint& pos,
  const wxSize& size,
  long style,
  const wxValidator& validator,
  const wxString &name)
  : wxExListViewFileName(
      parent, 
      type, 
      id, 
      lexer, 
      pos, 
      size, 
      style, 
      validator, 
      name)
  , m_Frame(frame)
  , m_MenuFlags(menu_flags)
{
  if (GetType() == LIST_HISTORY)
  {
    m_Frame->UseFileHistoryList(this);
  }

  wxAcceleratorEntry entries[5];

  entries[0].Set(wxACCEL_NORMAL, WXK_DELETE, wxID_DELETE);
  entries[1].Set(wxACCEL_CTRL, WXK_INSERT, wxID_COPY);
  entries[2].Set(wxACCEL_SHIFT, WXK_INSERT, wxID_PASTE);
  entries[3].Set(wxACCEL_SHIFT, WXK_DELETE, wxID_CUT);
  entries[4].Set(wxACCEL_CTRL, 'M', ID_LIST_COMPARE);

  wxAcceleratorTable accel(WXSIZEOF(entries), entries);
  SetAcceleratorTable(accel);
  
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    for (int i = GetFirstSelected(); i != -1; i = GetNextSelected(i))
      ItemActivated(i);}, ID_LIST_OPEN_ITEM);
  
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    bool first = true;
    wxString file1,file2;
    wxExListViewFileName* list = NULL;
    for (int i = GetFirstSelected(); i != -1; i = GetNextSelected(i))
    {
      wxExListItem li(this, i);
      const wxFileName* filename = &li.GetFileName();
      if (wxFileName::DirExists(filename->GetFullPath())) continue; // IsDir no ok
      switch (event.GetId())
      {
        case ID_LIST_COMPARE:
        {
          if (GetSelectedItemCount() == 1)
          {
            list = m_Frame->Activate(LIST_FILE);
            if (list == NULL) return;
            const int main_selected = list->GetFirstSelected();
            wxExCompareFile(wxExListItem(list, main_selected).GetFileName(), *filename);
          }
          else
          {
            if (first)
            {
              first = false;
              file1 = filename->GetFullPath();
            }
            else
            {
              first = true;
              file2 = filename->GetFullPath();
            }
            if (first) wxExCompareFile(wxFileName(file1), wxFileName(file2));
          }
        }
        break;
      }
    }}, ID_LIST_COMPARE);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    wxExMake(wxExListItem(this, GetFirstSelected()).GetFileName());}, ID_LIST_RUN_MAKE);

  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    RunItems(event.GetId());}, ID_TOOL_LOWEST, ID_TOOL_HIGHEST);
  
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    std::vector< wxString > files;
    for (int i = GetFirstSelected(); i != -1; i = GetNextSelected(i))
    {
      files.push_back(wxExListItem(this, i).GetFileName().GetFullPath());
    }
    wxExVCSExecute(m_Frame, event.GetId() - ID_EDIT_VCS_LOWEST - 1, files);
    }, ID_EDIT_VCS_LOWEST, ID_EDIT_VCS_HIGHEST);
}

void wxExListViewWithFrame::BuildPopupMenu(wxExMenu& menu)
{
  bool exists = true;
  bool is_folder = false;
  bool read_only = false;
  bool is_make = false;

  if (GetSelectedItemCount() >= 1)
  {
    const wxExListItem item(this, GetFirstSelected());

    exists = item.GetFileName().GetStat().IsOk();
    is_folder = wxFileName::DirExists(item.GetFileName().GetFullPath());
    read_only = item.GetFileName().GetStat().IsReadOnly();
    is_make = item.GetFileName().GetLexer().GetScintillaLexer() == "makefile";
  }

  if (GetSelectedItemCount() >= 1 && exists)
  {
    menu.Append(ID_LIST_OPEN_ITEM, _("&Open"), wxART_FILE_OPEN);
    menu.AppendSeparator();
  }

  wxExListViewFileName::BuildPopupMenu(menu);

  if (GetSelectedItemCount() > 1 && exists &&
     !wxConfigBase::Get()->Read(_("Comparator")).empty())
  {
    menu.AppendSeparator();
    menu.Append(ID_LIST_COMPARE, _("C&ompare") + "\tCtrl+M");
  }

  if (GetSelectedItemCount() == 1)
  {
    if (is_make)
    {
      menu.AppendSeparator();
      menu.Append(ID_LIST_RUN_MAKE, _("&Make"));
    }

    if ( GetType() != LIST_FILE &&
        !wxExVCS().Use() &&
         exists && !is_folder)
    {
      wxExListView* list = m_Frame->Activate(LIST_FILE);

      if (list != NULL && list->GetSelectedItemCount() == 1)
      {
        wxExListItem thislist(this, GetFirstSelected());
        const wxString current_file = thislist.GetFileName().GetFullPath();

        wxExListItem otherlist(list, list->GetFirstSelected());
        const wxString with_file = otherlist.GetFileName().GetFullPath();

        if (current_file != with_file &&
            !wxConfigBase::Get()->Read(_("Comparator")).empty())
        {
          menu.AppendSeparator();
          menu.Append(ID_LIST_COMPARE,
            _("&Compare With") + " " + wxExGetEndOfText(with_file));
        }
      }
    }
  }

  if (GetSelectedItemCount() >= 1)
  {
    if (exists && !is_folder)
    {
      if (wxExVCS::DirExists(
        wxExListItem(this, GetFirstSelected()).GetFileName()))
      {
        menu.AppendSeparator();
        menu.AppendVCS(wxExListItem(this, GetFirstSelected()).GetFileName());
      }
    }

    // Finding in the LIST_FIND and REPLACE would 
    /// result in recursive calls, do not add them.
    if ( exists &&
         GetType() != LIST_FIND && GetType() != LIST_REPLACE &&
        (m_MenuFlags & LIST_MENU_REPORT_FIND))
    {
      menu.AppendSeparator();
      menu.Append(ID_TOOL_REPORT_FIND, 
        wxExEllipsed(m_Frame->GetFindInCaption(ID_TOOL_REPORT_FIND)));

      if (!read_only)
      {
        menu.Append(ID_TOOL_REPORT_REPLACE, 
          wxExEllipsed(m_Frame->GetFindInCaption(ID_TOOL_REPORT_REPLACE)));
      }
    }
  }

  if (GetSelectedItemCount() > 0 && 
      exists && 
     (m_MenuFlags & LIST_MENU_TOOL) &&
      wxExLexers::Get()->GetCount() > 0)
  {
    menu.AppendSeparator();
    menu.AppendTools();
  }
}

bool wxExListViewWithFrame::Destroy()	
{
  if (wxExInterruptable::Running())
  {
    wxExInterruptable::Cancel();
  }
    
  return wxExListViewFileName::Destroy();
}

wxExListViewWithFrame::wxExListType wxExListViewWithFrame::GetTypeTool(
  const wxExTool& tool)
{
  switch (tool.GetId())
  {
    case ID_TOOL_REPORT_FIND: return LIST_FIND; break;
    case ID_TOOL_REPORT_KEYWORD: return LIST_KEYWORD; break;
    case ID_TOOL_REPORT_REPLACE: return LIST_REPLACE; break;
    default: wxFAIL; return LIST_FILE;
  }
}

void wxExListViewWithFrame::ItemActivated(long item_number)
{
  const wxExListItem item(this, item_number);

  if (item.GetFileName().FileExists())
  {
    const wxString line_number_str = GetItemText(item_number, _("Line No"));
    const int line_number = atoi(line_number_str.c_str());
    const wxString match =
      (GetType() == LIST_REPLACE ?
         GetItemText(item_number, _("Replaced")):
         GetItemText(item_number, _("Match")));

    m_Frame->OpenFile(
      item.GetFileName().GetFullPath(),
      line_number, 
      match);
  }
  else
  { 
    wxExListViewFileName::ItemActivated(item_number);
  }
}

void wxExListViewWithFrame::RunItems(const wxExTool& tool)
{
  if (tool.GetId() == ID_TOOL_REPORT_KEYWORD && GetType() == LIST_KEYWORD)
  {
    return;
  }

  if (tool.IsFindType())
  {
    if (m_Frame->FindInFilesDialog(tool.GetId()) == wxID_CANCEL)
    {
      return;
    }
  }

  if (!wxExTextFileWithListView::SetupTool(tool, m_Frame))
  {
    return;
  }

  wxExStatistics<int> stats;

  for (int i = GetFirstSelected(); i != -1; i = GetNextSelected(i))
  {
    stats += wxExRun(wxExListItem(this, i), tool).GetElements();
  }

  wxLogStatus(tool.Info(&stats));
}
