////////////////////////////////////////////////////////////////////////////////
// Name:      vimacros.cpp
// Purpose:   Implementation of class wxExViMacros
// Author:    Anton van Wezenbeek
// Copyright: (c) 2015 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif
#include <wx/stdpaths.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/utils.h>
#include <wx/extension/vimacros.h>
#include <wx/extension/ex.h>
#include <wx/extension/frame.h>
#include <wx/extension/stc.h>
#include <wx/extension/util.h>

#if wxUSE_GUI

bool wxExViMacros::m_IsExpand = false;
bool wxExViMacros::m_IsModified = false;
bool wxExViMacros::m_IsPlayback = false;
bool wxExViMacros::m_IsRecording = false;
wxString wxExViMacros::m_Macro;

std::map <wxString, std::string> wxExViMacros::m_Abbreviations;
std::map <wxString, std::vector< std::string > > wxExViMacros::m_Macros;
std::map <wxString, wxExVariable > wxExViMacros::m_Variables;

void wxExViMacros::AskForInput()
{
  for (auto& it : m_Variables)
  {
    it.second.AskForInput();
  }
}

const std::string wxExViMacros::Decode(const wxString& text)
{
  std::string output;
  
  for (size_t i = 0; i < text.length(); i++)
  {
    if (
      i + 1 < text.length() &&
      text[i]     == '$' &&
      text[i + 1] == '!')
    {
      int skip = 0;
      wxString number;
    
      if (
        i + 3 < text.length() &&
        isdigit(text[i + 2]) && 
        text[i + 3] == '!')
      {
        skip  = 3;
        number = text.Mid(i + 2, 1);
      }
      else if (
        i + 4 < text.length() &&
        isdigit(text[i + 2]) &&
        isdigit(text[i + 3]) &&
        text[i + 4] == '!')
      {
        skip = 4;
        number = text.Mid(i + 2, 2);
      }
      
      if (!number.empty())
      {
        output += wxChar(atoi(number));
        i += skip;
      }
    }
    else
    {
      output += text[i];
    }
  }
  
  return output;
}

const wxString wxExViMacros::Encode(const std::string& text)
{
  wxString output;
  
  for (size_t i = 0; i < text.length(); i++)
  {
    const int c = text[i];
  
    // Encode control and whitespace characters.
    if (iscntrl(c) || isspace(c))
    {
      output += wxString::Format("$!%d!", c);
    }
    else
    {
      output += text[i];
    }
  }

  return output;
}

bool wxExViMacros::Expand(wxExEx* ex, const wxString& variable)
{
  auto it = m_Variables.find(variable);
  
  bool ok;
    
  if (it == m_Variables.end())
  {
    std::pair<std::map<wxString, wxExVariable>::iterator, bool> ret = 
      m_Variables.insert(std::make_pair(variable, wxExVariable(variable)));
      
    wxLogStatus(_("Added variable") + ": "  +  variable);
    
    ok = ret.first->second.Expand(ex);
  
    if (ret.first->second.IsModified())
    {
      m_IsModified = true;
    }
    
    // If ok is false, this is because expansion dialog was cancelled,
    // no need to show log status message.
  }
  else
  {
    ok = it->second.Expand(ex);
  
    if (it->second.IsModified())
    {
      m_IsModified = true;
    }

    // Now only show log status if this is no input variable,
    // as it was cancelled in that case.    
    if (!ok && !it->second.IsInput())
    {
      wxLogStatus(_("Could not expand variable") + ": "  +  variable);
    }
  }

  if (ok)
  {
    wxLogStatus(_("Variable expanded"));
  
    if (!m_IsRecording)
    {
      m_Macro = variable;
    }
  }
  
  return ok;
}  

bool wxExViMacros::Expand(wxExEx* ex, const wxString& variable, wxString& value)
{
  auto it = m_Variables.find(variable);
    
  bool ok;
    
  if (it == m_Variables.end())
  {
    std::pair<std::map<wxString, wxExVariable>::iterator, bool> ret = 
      m_Variables.insert(std::make_pair(variable, wxExVariable(variable)));
      
    wxLogStatus(_("Added variable") + ": "  +  variable);
    
    ok = ret.first->second.Expand(ex, value);
  
    // If we are expanding, one input is enough.    
    if (m_IsExpand)
    {
      ret.first->second.SkipInput();
    }
    
    if (ret.first->second.IsModified())
    {
      m_IsModified = true;
    }
  }
  else
  {
    ok = it->second.Expand(ex, value);

    // If we are expanding, one input is enough.    
    if (m_IsExpand)
    {
      it->second.SkipInput();
    }
  
    if (it->second.IsModified())
    {
      m_IsModified = true;
    }
  }
  
  if (!ok)
  {
    wxLogStatus(_("Could not expand variable") + ": "  +  variable);
  }
  else 
  {
    wxLogStatus(_("Variable expanded"));
  
    if (!m_IsRecording)
    {
      m_Macro = variable;
    }
  }
  
  return ok;
}

bool wxExViMacros::ExpandTemplate(
  wxExEx* ex, const wxExVariable& v, wxString& expanded)
{
  if (!m_IsExpand)
  {
    m_IsExpand = true;
    AskForInput();
  }

  // Read the file (file name is in m_Value), expand
  // all macro variables in it, and set expanded.
  const wxFileName filename(
#ifdef wxExUSE_PORTABLE
      wxPathOnly(wxStandardPaths::Get().GetExecutablePath())
#else
      wxStandardPaths::Get().GetUserDataDir()
#endif
      + wxFileName::GetPathSeparator() + v.GetValue());

  wxFileInputStream input(filename.GetFullPath());
  
  if (!input.IsOk())
  {
    wxLogError("Could not open template file: " + filename.GetFullPath());
    return false;
  }

  // Keep current macro, in case you cancel expanding,
  // this one is restored.
  wxString macro = m_Macro;
  
  wxTextInputStream text(input);
  
  while (input.IsOk() && !input.Eof()) 
  {
    const wxChar c = text.GetChar();
    
    if (c != '@')
    {
      expanded += c;
    }
    else
    {
      wxString variable;
      bool completed = false;
      
      while (input.IsOk() && !input.Eof() && !completed) 
      {
        const wxChar c = text.GetChar();
    
        if (c != '@')
        {
          variable += c;
        }
        else
        {
          completed = true;
        }
      }
      
      if (!completed)
      {
        m_Macro = macro;
        return false;
      }
      
      // Prevent recursion.
      if (variable == v.GetName())
      {
        m_Macro = macro;
        return false;
      }
      
      wxString value;
      
      if (!Expand(ex, variable, value))
      {
        m_Macro = macro;
        return false;
      }
      
      expanded += value;
    }
  }
  
  m_IsExpand = false;

  // Set back to normal value.  
  AskForInput();
    
  if (!m_IsRecording)
  {
    m_Macro = v.GetName();
  }
    
  return true;
}

const std::vector< wxString > wxExViMacros::Get() const
{
  std::vector< wxString > v;
    
  for (const auto& it : m_Macros)
  {
    if (it.first.size() > 1)
    {
      v.push_back(it.first);
    }
  }
   
  for (const auto& it : m_Variables)
  {
    v.push_back(it.first);
  }
  
  std::sort(v.begin(), v.end());
  
  return v;
}

const std::vector< std::string > wxExViMacros::Get(const wxString& macro) const
{
  const auto it = m_Macros.find(macro);
    
  if (it != m_Macros.end())
  {
    return it->second;
  }
  else
  {
    const auto it = m_Variables.find(macro);
    std::vector<std::string> v;
    
    if (it != m_Variables.end())
    {
      v.push_back(it->second.GetValue().ToStdString());
    }
  
    return v;
  }
}

int wxExViMacros::GetCount() const
{
  return m_Macros.size() + m_Variables.size();
}

const wxFileName wxExViMacros::GetFileName()
{
  return wxFileName(
#ifdef wxExUSE_PORTABLE
    wxPathOnly(wxStandardPaths::Get().GetExecutablePath()),
#else
    wxStandardPaths::Get().GetUserDataDir(),
#endif
    "macros.xml");
}

const std::string wxExViMacros::GetRegister(const char name) const
{
  if (name == '*')
  {
    return wxExClipboardGet().ToStdString();
  }
  else
  {
    const auto it = m_Macros.find(name);
    
    std::string output;
    
    if (it != m_Macros.end())
    {
      for (const auto& it2 : it->second)
      {
        output += it2;
      }
    }

    return output;
  }
}

const std::vector< std::string > wxExViMacros::GetRegisters() const
{
  std::vector< std::string > r;
  
  for (const auto& it : m_Macros)
  {
    if (it.first.size() == 1)
    {
      wxString output;
    
      for (const auto& it2 : it.second)
      {
        output += it2;
      }
    
      r.push_back(wxString(wxString(it.first) + ": " + wxExSkipWhiteSpace(output)).ToStdString());
    }
  }
   
  const wxString clipboard(wxExSkipWhiteSpace(wxExClipboardGet()));
              
  if (!clipboard.empty())
  {
    r.push_back(wxString("*: " + clipboard).ToStdString());
  }
                
  return r;
}

bool wxExViMacros::IsRecorded(const wxString& macro) const
{
  return !Get(macro).empty();
}

bool wxExViMacros::IsRecordedMacro(const wxString& macro) const
{
  return m_Macros.find(macro) != m_Macros.end();
}

bool wxExViMacros::Load(wxXmlDocument& doc)
{
  // This test is to prevent showing an error if the macro file does not exist,
  // as this is not required.
  if (!GetFileName().FileExists())
  {
    return false;
  } 
  
  if (!doc.Load(GetFileName().GetFullPath()))
  {
    return false;
  }
  
  return true;
}

bool wxExViMacros::LoadDocument()
{
  wxXmlDocument doc;
  
  if (!Load(doc))
  {
    return false;
  }
  
  // If modified is true, then you did not save previous
  // recordings.
  // We assume that this is your choice, so we reset the member.
  m_IsModified = false;
  
  m_Abbreviations.clear();
  m_Macros.clear();
  m_Variables.clear();
  
  wxXmlNode* child = doc.GetRoot()->GetChildren();
  
  while (child)
  {
    if (child->GetName() == "abbreviation")
    {
      ParseNodeAbbreviation(child);
    }
    else if (child->GetName() == "macro")
    {
      ParseNodeMacro(child);
    }
    else if (child->GetName() == "variable")
    {
      ParseNodeVariable(child);
    }
      
    child = child->GetNext();
  }
  
  return true;
}

void wxExViMacros::ParseNodeAbbreviation(wxXmlNode* node)
{
  const wxString abb(node->GetAttribute("name"));
  const wxString text(node->GetNodeContent().Strip(wxString::both));
  
  const auto it = m_Abbreviations.find(abb);

  if (it != m_Abbreviations.end())
  {
    wxLogError("Duplicate abbreviation: %s on line: %d", 
     abb,
     node->GetLineNumber());
  }
  else
  {
    m_Abbreviations.insert(std::make_pair(abb, text.ToStdString()));
  }
}

void wxExViMacros::ParseNodeMacro(wxXmlNode* node)
{
  std::vector<std::string> v;
  
  wxXmlNode* command = node->GetChildren();

  while (command)
  {
    v.push_back(Decode(command->GetNodeContent()));
    command = command->GetNext();
  }
  
  const auto it = m_Macros.find(node->GetAttribute("name"));

  if (it != m_Macros.end())
  {
    wxLogError("Duplicate macro: %s on line: %d", 
      node->GetAttribute("name"),
      node->GetLineNumber());
  }
  else
  {
    m_Macros.insert(std::make_pair(node->GetAttribute("name"), v));
  }
}

void wxExViMacros::ParseNodeVariable(wxXmlNode* node)
{
  const wxExVariable variable(node);
  const auto it = m_Variables.find(variable.GetName());

  if (it != m_Variables.end())
  {
    wxLogError("Duplicate variable: %s on line: %d", 
     variable.GetName(),
     node->GetLineNumber());
  }
  else
  {
    m_Variables.insert(std::make_pair(variable.GetName(), variable));
  }
}

bool wxExViMacros::Playback(wxExEx* ex, const wxString& macro, int repeat)
{
  if (!IsRecordedMacro(macro))
  {
    wxLogStatus(_("Unknown macro") + ": "  +  macro);
    return false;
  }
  
  if (m_IsPlayback && macro == m_Macro)
  {
    wxLogStatus(_("Already playing back"));
    return false;
  }

  if (repeat <= 0)
  {
    return false;
  }
  
  ex->GetSTC()->BeginUndoAction();
  
  bool stop = false;
  
  if (!m_IsPlayback && !m_IsRecording)
  {
    m_Macro = macro;
    wxExFrame::StatusText(m_Macro, "PaneMacro");
  }
  
  m_IsPlayback = true;
  
  wxBusyCursor wait;
    
  AskForInput();
  
  for (int i = 0; i < repeat && !stop; i++)
  {
    for (auto& it : m_Macros[macro])
    { 
      stop = !ex->Command(it);
      
      if (stop)
      {
        wxLogStatus(_("Macro aborted at '") + it + "'");
        break;
      }
    }
  }

  ex->GetSTC()->EndUndoAction();

  if (!stop)
  {
    wxLogStatus(_("Macro played back"));
    m_Macro = macro; // might be overridden by expanded variable
    wxExFrame::StatusText(m_Macro, "PaneMacro");
  }
  
  m_IsPlayback = false;
  
  return !stop;
}

void wxExViMacros::Record(const std::string& text, bool new_command)
{
  if (!m_IsRecording || m_IsPlayback || text.empty())
  {
    return;
  }
  
  if (new_command) 
  {
    m_Macros[m_Macro].push_back(text);
  }
  else
  {
    if (m_Macros[m_Macro].empty())
    {
      std::string s;
      m_Macros[m_Macro].push_back(s);
    }
    
    m_Macros[m_Macro].back() += text;
  }
}

bool wxExViMacros::SaveDocument(bool only_if_modified)
{
  if (!m_IsModified && only_if_modified)
  {
    return false;
  }
  
  wxXmlDocument doc;
  
  if (!Load(doc))
  {
    return false;
  }
  
  wxXmlNode* root = doc.GetRoot();
  wxXmlNode* child;
  
  while (child = root->GetChildren())
  {
    root->RemoveChild(child);
    delete child;
  }
 
  for (
    auto it = 
      m_Macros.rbegin();
    it != m_Macros.rend();
    ++it)
  {
    if (!it->second.empty())
    {
      wxXmlNode* element = new wxXmlNode(root, wxXML_ELEMENT_NODE, "macro");
      element->AddAttribute("name", it->first);
      
      for (
        auto it2 = it->second.rbegin();
        it2 != it->second.rend();
        ++it2)
      { 
        wxXmlNode* cmd = new wxXmlNode(element, wxXML_ELEMENT_NODE, "command");
        new wxXmlNode(cmd, wxXML_TEXT_NODE, "", Encode(*it2));
      }
    }
  }
  
  for (
    auto it2 = 
      m_Variables.rbegin();
    it2 != m_Variables.rend();
    ++it2)
  {
    wxXmlNode* element = new wxXmlNode(root, wxXML_ELEMENT_NODE, "variable");
    it2->second.Save(element);
  }
  
  for (
    auto it3 = 
      m_Abbreviations.rbegin();
    it3 != m_Abbreviations.rend();
    ++it3)
  {
    wxXmlNode* element = new wxXmlNode(root, wxXML_ELEMENT_NODE, "abbreviation");
    element->AddAttribute("name", it3->first);
    new wxXmlNode(element, wxXML_TEXT_NODE, "", it3->second);
  }
  
  const bool ok = doc.Save(GetFileName().GetFullPath());
  
  if (ok)
  {
    m_IsModified = false;
  }

  return ok;
}

void wxExViMacros::SetAbbreviation(const wxString& ab, const std::string& value)
{
  m_Abbreviations[ab] = value;
  m_IsModified = true;
}

void wxExViMacros::SetRegister(const char name, const std::string& value)
{
  if (!wxIsascii(name))
  {
    return;
  }
  
  if (name == '*')
  {
    wxExClipboardAdd(value);
    return;
  }

  std::vector<std::string> v;
  
  // The black hole register, everything written to it is discarded.
  if (name != '_')
  {
    if (wxIsupper(name))
    {
      v.push_back(GetRegister(tolower(name)) + value);
    }
    else
    {
      v.push_back(value);
    }
  }
  
  m_Macros[(char)tolower(name)] = v;

  m_IsModified = true;
}

void wxExViMacros::StartRecording(const wxString& macro)
{
  if (m_IsRecording || macro.empty())
  {
    return;
  }
  
  m_IsRecording = true;
  m_IsModified = true;
  
  if (macro.size() == 1)
  {
    // We only use lower case macro's, to be able to
    // append to them using qA.
    m_Macro = macro.Lower();
  
    // Clear macro if it is lower case
    // (otherwise append to the macro).
    if (wxIslower(macro[0]))
    {
      m_Macros[m_Macro].clear();
    }
  }
  else
  {
    m_Macro = macro;
    m_Macros[m_Macro].clear();
  }
  
  wxExFrame::StatusText(m_Macro, "PaneMacro");
  
  wxLogStatus(_("Macro recording"));
}

bool wxExViMacros::StartsWith(const wxString& text) const
{
  if (text.empty())
  {
    return false;
  }

  if (wxIsdigit(text[0]))
  {
    return false;
  }
  
  for (const auto& it : m_Macros)
  {
    if (it.first.StartsWith(text))
    {
      return true;
    }
  }
   
  for (const auto& it : m_Variables)
  {
    if (it.first.StartsWith(text))
    {
      return true;
    }
  }
  
  return false;
}
  
void wxExViMacros::StopRecording()
{
  if (!m_IsRecording)
  {
    return;
  }
  
  m_IsRecording = false;
  
  if (!Get(m_Macro).empty())
  {
    wxLogStatus(wxString::Format(_("Macro '%s' is recorded"), m_Macro.c_str()));
  }
  else
  {
    m_Macros.erase(m_Macro);
    m_Macro.clear();
    wxLogStatus(wxEmptyString);
    wxExFrame::StatusText(wxEmptyString, "PaneMacro");
  }
}

#endif // wxUSE_GUI
