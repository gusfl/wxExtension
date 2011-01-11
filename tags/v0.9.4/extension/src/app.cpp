/******************************************************************************\
* File:          app.cpp
* Purpose:       Implementation of wxExApp class
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
*
* Copyright (c) 2009 Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/config.h>
#include <wx/dir.h>
#ifdef wxExUSE_PORTABLE
#include <wx/fileconf.h> 
#endif
#include <wx/stdpaths.h>
#include <wx/extension/app.h>
#include <wx/extension/frd.h>
#include <wx/extension/lexers.h>
#include <wx/extension/log.h>
#include <wx/extension/printing.h>
#include <wx/extension/tool.h>
#include <wx/extension/vcs.h>

int wxExApp::OnExit()
{
  delete wxExFindReplaceData::Set(NULL);
  delete wxExLexers::Set(NULL);
  delete wxExLog::Set(NULL);
  delete wxExPrinting::Set(NULL);
  delete wxExTool::Set(NULL);

  return wxApp::OnExit(); // this destroys the config
}

bool wxExApp::OnInit()
{
  if (!wxApp::OnInit())
  {
    return false;
  }

  // First construct the config, as most classes use it.
  wxConfigBase* config;
#ifdef wxExUSE_PORTABLE
  config = new wxFileConfig(
    wxEmptyString,
    wxEmptyString,
    wxFileName(
      wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetPath(),
      GetAppName().Lower() + wxString(".conf")).GetFullPath(),
    wxEmptyString,
    wxCONFIG_USE_LOCAL_FILE);
#else
  // As wxStandardPaths::GetUserDataDir is used, subdir is necessary for config.
  // (ignored on non-Unix system)
  config = new wxConfig(
    wxEmptyString,
    wxEmptyString,
    wxEmptyString,
    wxEmptyString,
    wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_SUBDIR);
#endif
  wxConfigBase::Set(config);
  
  int lang = wxLANGUAGE_DEFAULT;
  
  if (wxConfigBase::Get()->Exists("LANG"))
  {
    lang = wxConfigBase::Get()->ReadLong("LANG", wxLANGUAGE_DEFAULT);
  }
  
  // Init the localization, from now on things will be translated.
  // Do not load wxstd, we load all files ourselved,
  // and do not want messages about loading non existing wxstd files.
  m_Locale.Init(lang, wxLOCALE_DONT_LOAD_DEFAULT);
  
  // If there are catalogs in the catalog_dir, then add them to the m_Locale.
  // README: We use the canonical name, also for windows, not sure whether that is
  // the best.
  m_CatalogDir = wxStandardPaths::Get().GetLocalizedResourcesDir(
    m_Locale.GetCanonicalName(),
    // This seems to be necessary for wxGTK. For wxMSW it isn't used.
    wxStandardPaths::ResourceCat_Messages);

  if (wxFileName::DirExists(m_CatalogDir))
  {
    wxArrayString files;
    wxDir::GetAllFiles(m_CatalogDir, &files);

    for (
      auto it = files.begin();
      it != files.end();
      it++)
    {
      const wxFileName fn(*it);

      if (!m_Locale.AddCatalog(fn.GetName()))
      {
        wxLogError("Catalog could not be added: " + fn.GetName());
      }
    }
  }

  wxExVCS(wxFileName(
#ifdef wxExUSE_PORTABLE
    wxPathOnly(wxStandardPaths::Get().GetExecutablePath())
#else
    wxStandardPaths::Get().GetUserDataDir()
#endif
    + wxFileName::GetPathSeparator() + "vcs.xml")
    ).Read();

  return true;
}