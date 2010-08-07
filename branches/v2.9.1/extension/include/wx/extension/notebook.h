/******************************************************************************\
* File:          notebook.h
* Purpose:       Declaration of class wxExNotebook
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 1998-2009, Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#ifndef _EXNOTEBOOK_H
#define _EXNOTEBOOK_H

#include <map>
#include <wx/aui/auibook.h>

#if wxUSE_GUI
#if wxUSE_AUI

class wxExManagedFrame;

/// Offers a notebook with page mapping and interfaces with wxExManagedFrame.
class wxExNotebook : public wxAuiNotebook
{
public:
  /// Constructor.
  wxExNotebook(wxWindow* parent,
    wxExManagedFrame* frame, // NULL is allowed
    wxWindowID id = wxID_ANY,
    const wxPoint& pos = wxDefaultPosition,
    const wxSize& size = wxDefaultSize,
    long style = wxAUI_NB_DEFAULT_STYLE);

  /// Adds the page with given key and fills the map.
  /// If the key already exists, NULL is returned,
  /// and no new page is added.
  wxWindow* AddPage(
    wxWindow* page,
    const wxString& key,
    const wxString& text = wxEmptyString, // in that case uses key as text
    bool select = false,
    const wxBitmap& bitmap = wxNullBitmap);

  /// Deletes the page with the given key.
  bool DeletePage(const wxString& key);

  /// Do something for each page in the notebook.
  /// The pages should all be castable to wxExSTC pages.
  /// The id should be inbetween ID_ALL_LOWEST and ID_ALL_HIGHEST.
  /// Cannot be const as it can call DeletePage.
  bool ForEach(int id);

  /// Returns the key specified by the given page.
  /// If the page does not exist an empty string is returned.
  const wxString GetKeyByPage(wxWindow* page) const;

  /// Gets all keys as a string, separated by a newline (for testing).
  const wxString GetKeys() const;

  /// Returns the page specified by the given key.
  /// If the key does not exist NULL is returned.
  wxWindow* GetPageByKey(const wxString& key) const;

  /// Sets the pagetext for the given new key,
  /// on the page for the given key.
  /// If the key does not exist false is returned.
  bool SetPageText(
    const wxString& key,
    const wxString& new_key,
    const wxString& text);
protected:
  void OnNotebook(wxAuiNotebookEvent& event);
private:
  void ErasePage(const wxString& key); // remove from the map

  wxExManagedFrame* m_Frame;
  // In bookctrl.h: m_pages
  std::map<wxString, wxWindow*> m_MapPages;

  DECLARE_EVENT_TABLE()
};
#endif // wxUSE_AUI
#endif // wxUSE_GUI
#endif