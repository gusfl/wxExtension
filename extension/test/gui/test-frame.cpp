////////////////////////////////////////////////////////////////////////////////
// Name:      test.cpp
// Purpose:   Implementation for wxExtension cpp unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2015 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <vector>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/extension/managedframe.h>
#include <wx/extension/stc.h>
#include "test.h"

void fixture::testFrame()
{
  m_Frame->SetFocus(); // otherwise focus is on stc component caused by testEx

  std::vector<wxExStatusBarPane> panes;

  panes.push_back(wxExStatusBarPane());
  
  for (int i = 0; i < 25; i++)
  {
    panes.push_back(wxExStatusBarPane(wxString::Format("Pane%d", i)));
  }
  
  panes.push_back(wxExStatusBarPane("PaneInfo"));
  panes.push_back(wxExStatusBarPane("PaneLexer"));
  panes.push_back(wxExStatusBarPane("PaneFileType"));
  panes.push_back(wxExStatusBarPane("LastPane"));

  m_StatusBar = m_Frame->SetupStatusBar(panes);
  
  CPPUNIT_ASSERT( m_StatusBar->GetFieldsCount () == panes.size());
  
  CPPUNIT_ASSERT(!m_Frame->OpenFile(GetTestFile()));
  CPPUNIT_ASSERT( m_Frame->OpenFile(GetTestFile().GetFullPath(), "contents"));
  
  CPPUNIT_ASSERT( m_Frame->GetGrid() == NULL);
  CPPUNIT_ASSERT( m_Frame->GetListView() == NULL);
  CPPUNIT_ASSERT( m_Frame->GetSTC() == NULL);
  
  m_Frame->SetFindFocus(NULL);
  m_Frame->SetFindFocus(m_Frame);
  m_Frame->SetFindFocus(m_Frame->GetSTC());
  
  wxMenuBar* bar = new wxMenuBar();
  m_Frame->SetMenuBar(bar);
  
  m_Frame->StatusBarClicked("test");
  m_Frame->StatusBarClicked("Pane1");
  m_Frame->StatusBarClicked("Pane2");
  
  m_Frame->StatusBarClickedRight("test");
  m_Frame->StatusBarClickedRight("Pane1");
  m_Frame->StatusBarClickedRight("Pane2");
  
  CPPUNIT_ASSERT(!m_Frame->StatusText("hello", "test"));
  CPPUNIT_ASSERT( m_Frame->StatusText("hello1", "Pane1"));
  CPPUNIT_ASSERT( m_Frame->StatusText("hello2", "Pane2"));
  CPPUNIT_ASSERT( m_Frame->GetStatusText("Pane1") = "hello1");
  CPPUNIT_ASSERT( m_Frame->GetStatusText("Pane2") = "hello2");
  
  CPPUNIT_ASSERT(!m_Frame->UpdateStatusBar(m_Frame->GetSTC(), "test"));
  CPPUNIT_ASSERT(!m_Frame->UpdateStatusBar(m_Frame->GetSTC(), "Pane1"));
  CPPUNIT_ASSERT(!m_Frame->UpdateStatusBar(m_Frame->GetSTC(), "Pane2"));
  CPPUNIT_ASSERT(!m_Frame->UpdateStatusBar(m_Frame->GetSTC(), "PaneInfo"));
  
  wxExSTC* stc = new wxExSTC(m_Frame, "hello stc");
  stc->SetFocus();
  
  CPPUNIT_ASSERT( m_Frame->GetSTC() == stc);
  CPPUNIT_ASSERT( m_Frame->UpdateStatusBar(stc, "PaneInfo"));
  CPPUNIT_ASSERT( m_Frame->UpdateStatusBar(stc, "PaneLexer"));
  CPPUNIT_ASSERT( m_Frame->UpdateStatusBar(stc, "PaneFileType"));
}
