////////////////////////////////////////////////////////////////////////////////
// Name:      frame.cpp
// Purpose:   Implementation of wxExFrameWithHistory class
// Author:    Anton van Wezenbeek
// Copyright: (c) 2015 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <thread>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/cmdline.h>
#include <wx/config.h>
#include <wx/generic/dirctrlg.h> // for wxTheFileIconsTable
#include <wx/imaglist.h>
#include <wx/tokenzr.h>
#include <wx/extension/configdlg.h>
#include <wx/extension/frd.h>
#include <wx/extension/listitem.h>
#include <wx/extension/stc.h>
#include <wx/extension/toolbar.h>
#include <wx/extension/util.h>
#include <wx/extension/report/frame.h>
#include <wx/extension/report/defs.h>
#include <wx/extension/report/dir.h>
#include <wx/extension/report/listviewfile.h>
#include <wx/extension/report/util.h>

// The maximal number of files and projects to be supported.
const int NUMBER_RECENT_FILES = 25;
const int NUMBER_RECENT_PROJECTS = 25;
const int ID_RECENT_PROJECT_LOWEST =  wxID_FILE1 + NUMBER_RECENT_FILES + 1;

wxExFrameWithHistory::wxExFrameWithHistory(wxWindow* parent,
  wxWindowID id,
  const wxString& title,
  size_t maxFiles,
  size_t maxProjects,
  int style)
  : wxExManagedFrame(parent, id, title, style)
  , m_FiFDialog(NULL)
  , m_RiFDialog(NULL)
  , m_TextInFiles(_("In files"))
  , m_TextInFolder(_("In folder"))
  , m_TextRecursive(_("Recursive"))
  , m_ProjectModified(false)
  , m_FileHistory(maxFiles, wxID_FILE1)
  , m_FileHistoryList(NULL)
  , m_ProjectHistory(maxProjects, ID_RECENT_PROJECT_LOWEST)
{
  // There is only support for one history in the config.
  // We use file history for this, so update project history ourselves.
  // The order should be inverted, as the last one added is the most recent used.
  for (int i = m_ProjectHistory.GetMaxFiles() - 1 ; i >=0 ; i--)
  {
    SetRecentProject(
      wxConfigBase::Get()->Read(wxString::Format("RecentProject/%d", i)));
  }

  // Take care of default value.
  if (!wxConfigBase::Get()->Exists(m_TextRecursive))
  {
    wxConfigBase::Get()->Write(m_TextRecursive, true); 
  }

  // This set determines what fields are placed on the Find Files dialogs
  // as a list of checkboxes.
  m_Info.insert(wxExFindReplaceData::Get()->GetTextMatchWholeWord());
  m_Info.insert(wxExFindReplaceData::Get()->GetTextMatchCase());
  m_Info.insert(wxExFindReplaceData::Get()->GetTextRegEx());
  
  GetToolBar()->SetToolDropDown(wxID_OPEN, true);
  GetToolBar()->Realize();
  
  Bind(wxEVT_IDLE, &wxExFrameWithHistory::OnIdle, this);
  
  Bind(wxEVT_AUITOOLBAR_TOOL_DROPDOWN, [=](wxAuiToolBarEvent& event) {
    if (event.IsDropDownClicked())
    {
      wxAuiToolBar* tb = static_cast<wxAuiToolBar*>(event.GetEventObject());
  
      tb->SetToolSticky(event.GetId(), true);
  
      // create the popup menu
      // line up our menu with the button
      wxRect rect = tb->GetToolRect(event.GetId());
      wxPoint pt = tb->ClientToScreen(rect.GetBottomLeft());
      pt = ScreenToClient(pt);
      HistoryPopupMenu(m_FileHistory, wxID_FILE1, ID_CLEAR_FILES, pt);
  
      // make sure the button is "un-stuck"
      tb->SetToolSticky(event.GetId(), false);
    }
    else
    {
      event.Skip();
    }}, wxID_OPEN);
    
  Bind(wxEVT_CLOSE_WINDOW, [=](wxCloseEvent& event) {
    m_FileHistory.Save(*wxConfigBase::Get());
  
    if (m_ProjectHistory.GetCount() > 0 && m_ProjectModified)
    {
      wxConfigBase::Get()->DeleteGroup("RecentProject");
      
      for (size_t i = 0; i < m_ProjectHistory.GetCount(); i++)
      {
        wxConfigBase::Get()->Write(
          wxString::Format("RecentProject/%d", i),
          m_ProjectHistory.GetHistoryFile((size_t)i));
      }
    }
  
    event.Skip();});
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    ClearHistory(m_FileHistory);}, ID_CLEAR_FILES);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    ClearHistory(m_ProjectHistory);}, ID_CLEAR_PROJECTS);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    wxExListViewFile* project = GetProject();
    if (project != NULL)
    {
      project->FileSave();
    }}, ID_PROJECT_SAVE);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    if (!event.GetString().empty())
    {
      Grep(event.GetString());
    }
    else
    {
      if (m_FiFDialog == NULL)
      {
        CreateDialogs();
      }

      if  (GetSTC() != NULL && !GetSTC()->GetFindString().empty())
      {
        m_FiFDialog->Reload(); 
      }

      m_FiFDialog->Show(); 
    }}, ID_TOOL_REPORT_FIND);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    if (m_RiFDialog == NULL)
    {
      CreateDialogs();
    }
    if (GetSTC() != NULL && !GetSTC()->GetFindString().empty())
    {
      m_RiFDialog->Reload(); 
    }
    m_RiFDialog->Show();}, ID_TOOL_REPORT_REPLACE);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    DoRecent(m_FileHistory, event.GetId() - wxID_FILE1);},
    wxID_FILE1, 
    wxID_FILE1 + NUMBER_RECENT_FILES);
    
  Bind(wxEVT_MENU, [=](wxCommandEvent& event) {
    DoRecent(m_ProjectHistory, event.GetId() - ID_RECENT_PROJECT_LOWEST, WIN_IS_PROJECT);},
    ID_RECENT_PROJECT_LOWEST, 
    ID_RECENT_PROJECT_LOWEST + NUMBER_RECENT_PROJECTS);
}

wxExFrameWithHistory::~wxExFrameWithHistory()
{
}

void wxExFrameWithHistory::ClearHistory(wxFileHistory& history)
{
  if (history.GetCount() > 0)
  {
    for (int i = history.GetCount() - 1; i >= 0; i--)
    {
      history.RemoveFileFromHistory(i);
    }
  }
}

void wxExFrameWithHistory::CreateDialogs()
{
  std::set<wxString> t(m_Info);
  t.insert(m_TextRecursive);
  
  const std::vector<wxExConfigItem> f {
    wxExConfigItem(wxExFindReplaceData::Get()->GetTextFindWhat(), 
      CONFIG_COMBOBOX, 
      wxEmptyString, 
      true),
    wxExConfigItem(m_TextInFiles, 
      CONFIG_COMBOBOX, 
      wxEmptyString, 
      true),
    wxExConfigItem(m_TextInFolder, 
      CONFIG_COMBOBOXDIR, 
      wxEmptyString, 
      true,
      1005),
    wxExConfigItem(t)};
  
  m_FiFDialog = new wxExConfigDialog(this,
    f,
    _("Find In Files"),
    0,
    1,
    wxAPPLY | wxCANCEL,
    ID_FIND_IN_FILES);
    
  m_RiFDialog = new wxExConfigDialog(this,
    std::vector<wxExConfigItem> {f.at(0),
      wxExConfigItem(wxExFindReplaceData::Get()->GetTextReplaceWith(), 
        CONFIG_COMBOBOX),
      f.at(1),
      f.at(2),
      wxExConfigItem(
        // Match whole word does not work with replace.
        std::set<wxString>{
        wxExFindReplaceData::Get()->GetTextMatchCase(),
        wxExFindReplaceData::Get()->GetTextRegEx(),
        m_TextRecursive})},
    _("Replace In Files"),
    0,
    1,
    wxAPPLY | wxCANCEL,
    ID_REPLACE_IN_FILES);
}

void wxExFrameWithHistory::DoRecent(
  wxFileHistory& history, 
  size_t index, 
  long flags)
{
  if (history.GetCount() > 0 && (int)index < history.GetMaxFiles())
  {
    const wxString file(history.GetHistoryFile(index));

    if (!wxFileExists(file))
    {
      history.RemoveFileFromHistory(index);
      wxLogStatus(_("Removed not existing file: %s from history"), 
        file.c_str());
    }
    else
    {
      OpenFile(file, 0, wxEmptyString, 0, flags);
    }
    
    if (flags == WIN_IS_PROJECT)
    {
      m_ProjectModified = true;
    }
  }
}

void wxExFrameWithHistory::FileHistoryPopupMenu()
{
  HistoryPopupMenu(m_FileHistory, wxID_FILE1, ID_CLEAR_FILES);
}

void wxExFrameWithHistory::FindInFiles(wxWindowID dialogid)
{
  const bool replace = (dialogid == ID_REPLACE_IN_FILES);
  const wxExTool tool =
    (replace ?
       ID_TOOL_REPORT_REPLACE:
       ID_TOOL_REPORT_FIND);

  if (!wxExTextFileWithListView::SetupTool(tool, this))
  {
    return;
  }

  wxLogStatus(GetFindReplaceInfoText(replace));
    
  int flags = wxDIR_FILES | wxDIR_HIDDEN;
  
  if (wxConfigBase::Get()->ReadBool(m_TextRecursive, true)) 
  {
    flags |= wxDIR_DIRS;
  }

  Unbind(wxEVT_IDLE, &wxExFrameWithHistory::OnIdle, this);

#ifndef __WXGTK__    
  std::thread t([=] {
#endif
    wxExDirTool dir(
      tool,
      wxExConfigFirstOf(m_TextInFolder),
      wxExConfigFirstOf(m_TextInFiles),
      flags);

    dir.FindFiles();

    wxLogStatus(tool.Info(&dir.GetStatistics().GetElements()));
    
    Bind(wxEVT_IDLE, &wxExFrameWithHistory::OnIdle, this);
#ifndef __WXGTK__    
    });
  t.detach();
#endif
}

bool wxExFrameWithHistory::FindInFiles(
  const std::vector< wxString > & files,
  int id,
  bool show_dialog,
  wxExListView* report)
{
  const wxExFileName filename(files[0]);
  const wxExTool tool(id);
  
  if (show_dialog && FindInFilesDialog(
    tool.GetId(),
    filename.DirExists() && !filename.FileExists()) == wxID_CANCEL)
  {
    return false;
  }
  
  if (!wxExTextFileWithListView::SetupTool(tool, this, report))
  {
    return false;
  }
  
  wxExStatistics<int> stats;
  
  for (const auto& it : files)
  {
    const wxExFileName fn(it);
    
    if (fn.FileExists())
    {
      wxExTextFileWithListView file(fn, tool);
      file.RunTool();
      stats += file.GetStatistics().GetElements();
    }
    else
    {
      wxExDirTool dir(
        tool, 
        fn.GetFullPath(), 
        wxExConfigFirstOf(m_TextInFiles));
        
      dir.FindFiles();
      stats += dir.GetStatistics().GetElements();
    }
  }
  
  wxLogStatus(tool.Info(&stats));
  
  return true;
}

int wxExFrameWithHistory::FindInFilesDialog(
  int id,
  bool add_in_files)
{
  if (GetSTC() != NULL)
  {
    GetSTC()->GetFindString();
  }

  if (wxExConfigDialog(this,
    std::vector<wxExConfigItem> {
      wxExConfigItem(
        wxExFindReplaceData::Get()->GetTextFindWhat(), 
        CONFIG_COMBOBOX, 
        wxEmptyString, 
        true),
      (add_in_files ? wxExConfigItem(
        m_TextInFiles, 
        CONFIG_COMBOBOX, 
        wxEmptyString, 
        true) : wxExConfigItem()),
      (id == ID_TOOL_REPORT_REPLACE ? wxExConfigItem(
        wxExFindReplaceData::Get()->GetTextReplaceWith(), 
        CONFIG_COMBOBOX): wxExConfigItem()),
      wxExConfigItem(m_Info)},
    GetFindInCaption(id)).ShowModal() == wxID_CANCEL)
  {
    return wxID_CANCEL;
  }

  wxLogStatus(GetFindReplaceInfoText(id == ID_TOOL_REPORT_REPLACE));
        
  return wxID_OK;
}

const wxString wxExFrameWithHistory::GetFindInCaption(int id) const
{
  return (id == ID_TOOL_REPORT_REPLACE ?
    _("Replace In Selection"):
    _("Find In Selection"));
}

const wxString wxExFrameWithHistory::GetFindReplaceInfoText(bool replace) const
{
  wxString log;
  
  // Printing a % in wxLogStatus gives assert
  if (
    !wxExFindReplaceData::Get()->GetFindString().Contains("%") &&
    !wxExFindReplaceData::Get()->GetReplaceString().Contains("%"))
  {
    log = _("Searching for") + ": " + wxExFindReplaceData::Get()->GetFindString();

    if (replace)
    {
      log += " " + _("replacing with") + ": " + wxExFindReplaceData::Get()->GetReplaceString();
    }
  }

  return log;
}

bool wxExFrameWithHistory::Grep(const wxString& arg)
{
  wxCmdLineParser cl(arg);
  cl.AddParam("match", wxCMD_LINE_VAL_STRING);
  cl.AddParam("folder", wxCMD_LINE_VAL_STRING);
  cl.AddParam("extension", wxCMD_LINE_VAL_STRING);
  cl.AddSwitch("h", wxEmptyString, "help", wxCMD_LINE_OPTION_HELP);
  cl.AddSwitch("r", wxEmptyString, "recursive");

  if (cl.Parse() != 0)
  {
    return false;
  }
  
  const wxExTool tool(ID_TOOL_REPORT_FIND);

  if (!wxExTextFileWithListView::SetupTool(tool, this))
  {
    wxLogStatus("setup failed");
    return false;
  }
  
  auto* stc = GetSTC();
  if (stc != NULL)
    wxSetWorkingDirectory(stc->GetFileName().GetPath());
  wxExFindReplaceData::Get()->SetFindString(cl.GetParam(0));
  wxExFindReplaceData::Get()->SetUseRegEx(true);
  wxLogStatus(GetFindReplaceInfoText());
  
  const wxString arg1(cl.GetParam(1));
  const wxString arg2(cl.GetParam(2));
  const int arg3(wxDIR_FILES | (cl.FoundSwitch("r") ? wxDIR_DIRS: 0));
  
  Unbind(wxEVT_IDLE, &wxExFrameWithHistory::OnIdle, this);
    
#ifndef __WXGTK__    
  std::thread t([=]{
#endif
    wxExDirTool dir(tool, arg1, arg2, arg3);
    dir.FindFiles();
    wxLogStatus(tool.Info(&dir.GetStatistics().GetElements()));
    
    Bind(wxEVT_IDLE, &wxExFrameWithHistory::OnIdle, this);
#ifndef __WXGTK__    
    });
  t.detach();
#endif
    
  return true;
}

void wxExFrameWithHistory::HistoryPopupMenu(
  const wxFileHistory& history, int first_id, int clear_id, const wxPoint& pos)
{
  wxMenu* menu = new wxMenu();

  for (size_t i = 0; i < history.GetCount(); i++)
  {
    const wxFileName file(history.GetHistoryFile(i));
    
    if (file.FileExists())
    {
      wxMenuItem* item = new wxMenuItem(
        menu, 
        first_id + i, 
        file.GetFullName());

      item->SetBitmap(wxTheFileIconsTable->GetSmallImageList()->GetBitmap(
        wxExGetIconID(file)));
    
      menu->Append(item);
    }
  }
  
  if (menu->GetMenuItemCount() > 0)
  {
    menu->AppendSeparator();
    menu->Append(clear_id, wxGetStockLabel(wxID_CLEAR));
      
    PopupMenu(menu, pos);
  }
    
  delete menu;
}
  
void wxExFrameWithHistory::OnCommandConfigDialog(
  wxWindowID dialogid,
  int commandid)
{
  switch (commandid)
  {
    case wxID_CANCEL:
      if (wxExInterruptable::Cancel())
      {
        wxLogStatus(_("Cancelled"));
      }
      break;

    case wxID_OK:
    case wxID_APPLY:
      switch (dialogid)
      {
        case wxID_ADD:
          {
          int flags = 0;
        
          if (wxConfigBase::Get()->ReadBool(GetProject()->GetTextAddFiles(), true)) 
          {
            flags |= wxDIR_FILES;
          }
        
          if (wxConfigBase::Get()->ReadBool(GetProject()->GetTextAddRecursive(), true)) 
          {
            flags |= wxDIR_DIRS;
          }

          GetProject()->AddItems(
            wxExConfigFirstOf(GetProject()->GetTextInFolder()),
            wxExConfigFirstOf(GetProject()->GetTextAddWhat()),
            flags);
          }
          break;

        case ID_FIND_IN_FILES:
        case ID_REPLACE_IN_FILES:
          FindInFiles(dialogid);
          break;

        default: wxFAIL;
      }
      break;

    default: wxFAIL;
  }
}

void wxExFrameWithHistory::OnIdle(wxIdleEvent& event)
{
  event.Skip();

  const wxString title(GetTitle());
  
  if (title.empty())
  {
    return;
  }
  
  auto* stc = GetSTC();
  auto* project = GetProject();

  const wxUniChar indicator('*');

  if ((project != NULL && project->GetContentsChanged()) ||
       // using GetContentsChanged gives assert in vcs dialog
      (stc != NULL && stc->GetModify() && stc->AllowChangeIndicator()))
  {
    // Project or editor changed, add indicator if not yet done.
    if (title.Last() != indicator)
    {
      SetTitle(title + " " + indicator);
    }
  }
  else
  {
    // Project or editor not changed, remove indicator if not yet done.
    if (title.Last() == indicator && title.size() > 2)
    {
      SetTitle(title.substr(0, title.length() - 2));
    }
  }
}

void wxExFrameWithHistory::OnNotebook(wxWindowID id, wxWindow* page)
{
  wxExManagedFrame::OnNotebook(id, page);
  
  wxExSTC* stc = wxDynamicCast(page, wxExSTC);

  if (stc != NULL)
  {
    if (stc->GetFileName().FileExists())
    {
      SetRecentFile(stc->GetFileName().GetFullPath());
    }
  }
  else
  {
      // TODO: crash for FiF, new page is not a wxExListViewFile
//    wxExListViewFile* lv = wxDynamicCast(page, wxExListViewFile);
    
//    if (lv != NULL)
//    {
//      SetRecentProject(lv->GetFileName().GetFullPath());
//    }
  }
}

bool wxExFrameWithHistory::OpenFile(
  const wxExFileName& filename,
  int line_number,
  const wxString& match,
  int col_number,
  long flags)
{
  if (wxExManagedFrame::OpenFile(filename, line_number, match, col_number, flags))
  {
    SetRecentFile(filename.GetFullPath());
    return true;
  }

  return false;
}

void wxExFrameWithHistory::ProjectHistoryPopupMenu()
{
  HistoryPopupMenu(m_ProjectHistory, ID_RECENT_PROJECT_LOWEST, ID_CLEAR_PROJECTS);
}

bool wxExFrameWithHistory::SetRecentFile(const wxString& file)
{
  if (file.empty() || m_FileHistory.GetMaxFiles() <= 0)
  {
    return false;
  }
  
  m_FileHistory.AddFileToHistory(file);

  if (m_FileHistoryList != NULL)
  {
    wxExListItem item(m_FileHistoryList, file);
    item.Insert((long)0);

    if (m_FileHistoryList->GetItemCount() > 1)
    {
      for (int i = m_FileHistoryList->GetItemCount() - 1; i >= 1 ; i--)
      {
        wxExListItem item(m_FileHistoryList, i);

        if (item.GetFileName().GetFullPath() == file)
        {
          item.Delete();
        }
      }
    }
  }
  
  return true;
}

bool wxExFrameWithHistory::SetRecentProject(const wxString& project) 
{
  if (project.empty() || m_ProjectHistory.GetMaxFiles() <= 0)
  {
    return false;
  }
  
  m_ProjectHistory.AddFileToHistory(project);
  m_ProjectModified = true;
  
  return true;
}
    
void wxExFrameWithHistory::UseFileHistory(wxWindowID id, wxMenu* menu)
{
  UseHistory(id, menu, m_FileHistory);

  // We can load file history now.
  m_FileHistory.Load(*wxConfigBase::Get());
}

void wxExFrameWithHistory::UseFileHistoryList(wxExListView* list)
{
  m_FileHistoryList = list;
  m_FileHistoryList->Hide();

  // Add all (existing) items from FileHistory.
  for (size_t i = 0; i < m_FileHistory.GetCount(); i++)
  {
    wxExListItem item(
      m_FileHistoryList, 
      m_FileHistory.GetHistoryFile(i));

    if (item.GetFileName().GetStat().IsOk())
    {
      item.Insert();
    }
  }
}

void wxExFrameWithHistory::UseHistory(
  wxWindowID id, 
  wxMenu* menu, 
  wxFileHistory& history)
{
  wxMenu* submenu = new wxMenu;
  menu->Append(id, _("Open &Recent"), submenu);
  history.UseMenu(submenu);
}

void wxExFrameWithHistory::UseProjectHistory(wxWindowID id, wxMenu* menu)
{
  if (m_ProjectHistory.GetMaxFiles() > 0)
  {
    UseHistory(id, menu, m_ProjectHistory);

    // And add the files to the menu.
    m_ProjectHistory.AddFilesToMenu();
  }
}
