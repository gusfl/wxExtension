/******************************************************************************\
* File:          test.cpp
* Purpose:       Implementation for wxExtension cpp unit testing
* Author:        Anton van Wezenbeek
* RCS-ID:        $Id$
* Created:       za 17 jan 2009 11:51:20 CET
*
* Copyright (c) 2009 Anton van Wezenbeek
* All rights are reserved. Reproduction in whole or part is prohibited
* without the written consent of the copyright owner.
\******************************************************************************/

#include <TestCaller.h>
#include <wx/extension/extension.h>
#include "test.h"

#define TEST_FILE "./test.h"
#define TEST_BIN "./test.bin"

class wxExFileTest: public wxExFile
{
public :
  wxExFileTest(const wxString& filename) : wxExFile(filename) {;};
private:
  virtual bool GetContentsChanged() const {return false;};
  virtual void ResetContentsChanged() {;};
  virtual void DoFileLoad(bool) {;};
  virtual void DoFileSave(bool) {;};
};

void wxExTestFixture::testFile()
{
  wxExFileTest file(TEST_FILE);
  
  CPPUNIT_ASSERT(file.GetStat().IsOk());
  CPPUNIT_ASSERT(file.GetStat().GetFullPath() == file.GetFileName().GetFullPath());
  // The fullpath should be normalized, test it.
  CPPUNIT_ASSERT(file.GetFileName().GetFullPath() != TEST_FILE);
  CPPUNIT_ASSERT(!file.GetStat().IsReadOnly());
  file.CheckFileSync();
  CPPUNIT_ASSERT(!file.GetStat().IsReadOnly());
  CPPUNIT_ASSERT(file.FileLoad(TEST_BIN));
  CPPUNIT_ASSERT(!file.IsOpened());
  CPPUNIT_ASSERT(file.Open(wxExFileName(TEST_BIN).GetFullPath()));
  wxCharBuffer buffer = file.Read();
  CPPUNIT_ASSERT(buffer.length() == 40);
}
  
void wxExTestFixture::testFileName()
{
  wxExFileName fileName(TEST_FILE);
  
  CPPUNIT_ASSERT(!fileName.GetLexer().GetScintillaLexer().empty());
  CPPUNIT_ASSERT(fileName.GetStat().IsOk());
  fileName.Assign("xxx");
  CPPUNIT_ASSERT(fileName.GetStat().IsOk());
}

void wxExTestFixture::testFileStatistics()
{
  wxExFileStatistics fileStatistics;
  
  CPPUNIT_ASSERT(fileStatistics.Get().empty());
  CPPUNIT_ASSERT(fileStatistics.Get("xx") == 0);
}

void wxExTestFixture::testLexer()
{
  wxExLexers lexers(wxFileName("../extension/data/lexers.xml"));
  wxExLexer lexer;
  lexer = lexers.FindByText("// this is a cpp comment text");
  CPPUNIT_ASSERT(lexer.GetScintillaLexer().empty());
  
  // now read lexers
  lexers.Read();
  lexer = lexers.FindByText("// this is a cpp comment text");
  
  CPPUNIT_ASSERT(!lexer.GetExtensions().empty());
  CPPUNIT_ASSERT(!lexer.GetCommentBegin().empty());
  CPPUNIT_ASSERT(!lexer.GetCommentBegin2().empty());
  CPPUNIT_ASSERT(lexer.GetCommentEnd().empty());
  CPPUNIT_ASSERT(!lexer.GetCommentEnd2().empty());
  CPPUNIT_ASSERT(!lexer.GetKeywords().empty());
  CPPUNIT_ASSERT(!lexer.GetKeywordsString().empty());
  CPPUNIT_ASSERT(lexer.IsKeyword("class"));
  CPPUNIT_ASSERT(lexer.IsKeyword("const"));
  CPPUNIT_ASSERT(lexer.KeywordStartsWith("cla"));
  CPPUNIT_ASSERT(!lexer.KeywordStartsWith("xxx"));
  CPPUNIT_ASSERT(!lexer.MakeComment("test", true).empty());
  CPPUNIT_ASSERT(!lexer.MakeComment("test", "test").empty());
  CPPUNIT_ASSERT(lexer.SetKeywords("hello:1"));
  CPPUNIT_ASSERT(lexer.SetKeywords("test11 test21:1 test31:1 test12:2 test22:2"));
  CPPUNIT_ASSERT(!lexer.IsKeyword("class")); // now overwritten
  CPPUNIT_ASSERT(lexer.IsKeyword("test11"));
  CPPUNIT_ASSERT(lexer.IsKeyword("test21"));
  CPPUNIT_ASSERT(lexer.IsKeyword("test12"));
  CPPUNIT_ASSERT(lexer.IsKeyword("test22"));
  CPPUNIT_ASSERT(lexer.KeywordStartsWith("te"));
  CPPUNIT_ASSERT(!lexer.KeywordStartsWith("xx"));
  CPPUNIT_ASSERT(!lexer.GetKeywords().empty());
}

void wxExTestFixture::testLexers()
{
  wxExLexers lexers(wxFileName("../extension/data/lexers.xml"));
  
  CPPUNIT_ASSERT(!lexers.BuildWildCards(wxFileName(TEST_FILE)).empty());
  CPPUNIT_ASSERT(lexers.Count() > 0);
  CPPUNIT_ASSERT(lexers.FindByFileName(wxFileName(TEST_FILE)).GetScintillaLexer() == "cpp");
  CPPUNIT_ASSERT(lexers.FindByName("cpp").GetScintillaLexer() == "cpp");
  CPPUNIT_ASSERT(lexers.FindByText("// this is a cpp comment text").GetScintillaLexer() == "cpp");
}

void wxExTestFixture::testRCS()
{
  wxExRCS rcs;
  CPPUNIT_ASSERT(rcs.GetDescription().empty());
  CPPUNIT_ASSERT(rcs.GetUser().empty());
}

void wxExTestFixture::testStat()
{
  wxExStat stat(TEST_FILE);
  CPPUNIT_ASSERT(stat.IsOk());
  CPPUNIT_ASSERT(!stat.IsReadOnly());
  CPPUNIT_ASSERT(stat.Sync("./test-base.link"));
}

void wxExTestFixture::testStatistics()
{
  wxExStatistics<long> statistics;
  statistics.Inc("test");
  CPPUNIT_ASSERT(statistics.Get("test") == 1);
  statistics.Inc("test");
  CPPUNIT_ASSERT(statistics.Get("test") == 2);
  statistics.Set("test", 13);
  CPPUNIT_ASSERT(statistics.Get("test") == 13);
  statistics.Dec("test");
  CPPUNIT_ASSERT(statistics.Get("test") == 12);
  statistics.Inc("test2");
  CPPUNIT_ASSERT(statistics.Get("test2") == 1);
  wxExStatistics<long> copy(statistics);
  CPPUNIT_ASSERT(copy.Get("test2") == 1);
  statistics.Clear();
  CPPUNIT_ASSERT(statistics.GetItems().empty());
}

void wxExTestFixture::testTextFile()
{
  wxExTextFile textFile(wxExFileName(TEST_FILE), ID_TOOL_REPORT_COUNT);
  CPPUNIT_ASSERT(textFile.RunTool());
  CPPUNIT_ASSERT(!textFile.GetStatistics().GetElements().GetItems().empty());
  CPPUNIT_ASSERT(!textFile.IsOpened()); // file should be closed after running tool

  CPPUNIT_ASSERT(textFile.RunTool()); // do the same test
  CPPUNIT_ASSERT(!textFile.GetStatistics().GetElements().GetItems().empty());
  CPPUNIT_ASSERT(!textFile.IsOpened()); // file should be closed after running tool

  wxExTextFile textFile2(wxExFileName(TEST_FILE), ID_TOOL_REPORT_KEYWORD);
  CPPUNIT_ASSERT(textFile2.RunTool());
//  CPPUNIT_ASSERT(!m_TextFile->GetStatistics().GetKeywords().GetItems().empty());
}

void wxExTestFixture::testTiming()
{
  wxExFileTest file(TEST_FILE);

  CPPUNIT_ASSERT(file.IsOpened());

  wxStopWatch sw;

  const int max = 10000;

  for (int i = 0; i < max; i++)
  {
    CPPUNIT_ASSERT(file.Read().length() > 0);
  }

  const long exfile_read = sw.Time();

  sw.Start();

  wxFile wxfile(TEST_FILE);

  for (int j = 0; j < max; j++)
  {
    char* charbuffer = new char[wxfile.Length()];
    wxfile.Read(charbuffer, wxfile.Length());
    wxString* buffer = new wxString(charbuffer, wxfile.Length());
    CPPUNIT_ASSERT(buffer->length() > 0);
    delete charbuffer;
    delete buffer;
  }

  const long file_read = sw.Time();

  printf(
    "wxExFile::Read:%ld wxFile::Read:%ld\n",
    exfile_read,
    file_read);
}

void wxExTestFixture::testTimingAttrib()
{
  const int max = 1000;

  wxStopWatch sw;

  const wxExFileName exfile(TEST_FILE);

  int checked = 0;

  for (int i = 0; i < max; i++)
  {
    checked += exfile.GetStat().IsReadOnly();
  }

  const long exfile_time = sw.Time();

  sw.Start();

  const wxFileName file(TEST_FILE);

  for (int j = 0; j < max; j++)
  {
    checked += file.IsFileWritable();
  }

  const long file_time = sw.Time();

  printf(
    "wxExFileName::IsReadOnly:%ld wxFileName::IsFileWritable:%ld\n",
    exfile_time,
    file_time);
}

void wxExTestFixture::testTool()
{
  CPPUNIT_ASSERT(wxExTool(ID_TOOL_REPORT_COUNT).IsCount());
  CPPUNIT_ASSERT(wxExTool(ID_TOOL_REPORT_FIND).IsFindType());
  CPPUNIT_ASSERT(wxExTool(ID_TOOL_REPORT_REPLACE).IsFindType());
  CPPUNIT_ASSERT(wxExTool(ID_TOOL_REPORT_COUNT).IsStatisticsType());
  CPPUNIT_ASSERT(wxExTool(ID_TOOL_REPORT_COUNT).IsReportType());
}

wxExTestSuite::wxExTestSuite()
  : CppUnit::TestSuite("wxExtension test suite")
{
  addTest(new CppUnit::TestCaller<wxExTestFixture>(
    "testFile",
    &wxExTestFixture::testFile));

  addTest(new CppUnit::TestCaller<wxExTestFixture>(
    "testFileName",
    &wxExTestFixture::testFileName));

  addTest(new CppUnit::TestCaller<wxExTestFixture>(
    "testFileStatistics",
    &wxExTestFixture::testFileStatistics));

  addTest(new CppUnit::TestCaller<wxExTestFixture>(
    "testLexer",
    &wxExTestFixture::testLexer));

  addTest(new CppUnit::TestCaller<wxExTestFixture>(
    "testLexers",
    &wxExTestFixture::testLexers));

  addTest(new CppUnit::TestCaller<wxExTestFixture>(
    "testRCS",
    &wxExTestFixture::testRCS));

  addTest(new CppUnit::TestCaller<wxExTestFixture>(
    "testStat",
    &wxExTestFixture::testStat));

  addTest(new CppUnit::TestCaller<wxExTestFixture>(
    "testStatistics",
    &wxExTestFixture::testStatistics));

  addTest(new CppUnit::TestCaller<wxExTestFixture>(
    "testTiming",
    &wxExTestFixture::testTiming));

  addTest(new CppUnit::TestCaller<wxExTestFixture>(
    "testTimingAttrib",
    &wxExTestFixture::testTimingAttrib));
    
  addTest(new CppUnit::TestCaller<wxExTestFixture>(
    "testTool",
    &wxExTestFixture::testTool));
}
