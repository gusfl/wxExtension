////////////////////////////////////////////////////////////////////////////////
// Name:      test.h
// Purpose:   Declaration of classes for wxExtension cpp unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2015 Anton van Wezenbeek
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>
#include <cppunit/extensions/HelperMacros.h>
#include "../test.h"

class wxExManagedFrame;
class wxExStatusBar;
class wxExSTCShell;

/// CppUnit gui test fixture.
/// These classes require either an wxExApp object, or wx to be initialized.
class fixture : public wxExTestFixture
{
  CPPUNIT_TEST_SUITE( fixture );
  
  CPPUNIT_TEST( testAddress );
  CPPUNIT_TEST( testAddressRange );
  CPPUNIT_TEST( testConfigDialog );
  CPPUNIT_TEST( testConfigItem );
  CPPUNIT_TEST( testDialog );
  CPPUNIT_TEST( testEx );
  CPPUNIT_TEST( testFileDialog );
  CPPUNIT_TEST( testFileStatistics );
  CPPUNIT_TEST( testFrame );
  CPPUNIT_TEST( testFrd );
  CPPUNIT_TEST( testGrid );
  CPPUNIT_TEST( testHexMode );
  CPPUNIT_TEST( testIndicator );
  CPPUNIT_TEST( testLexer );
  CPPUNIT_TEST( testLexers );
  CPPUNIT_TEST( testLink );
  CPPUNIT_TEST( testListItem );
  CPPUNIT_TEST( testListView );
  CPPUNIT_TEST( testManagedFrame );
  CPPUNIT_TEST( testMarker );
  CPPUNIT_TEST( testMenu );
  CPPUNIT_TEST( testNotebook );
  CPPUNIT_TEST( testOTL );
  CPPUNIT_TEST( testPrinting );
  CPPUNIT_TEST( testProcess );
  CPPUNIT_TEST( testProperty );
  CPPUNIT_TEST( testShell );
  CPPUNIT_TEST( testStatistics );
  CPPUNIT_TEST( testStatusBar );
  CPPUNIT_TEST( testSTC );
  CPPUNIT_TEST( testSTCEntryDialog );
  CPPUNIT_TEST( testSTCFile );
  CPPUNIT_TEST( testStyle );
  CPPUNIT_TEST( testTextFile );
  CPPUNIT_TEST( testToVectorString );
  CPPUNIT_TEST( testUtil );
  CPPUNIT_TEST( testVariable );
  CPPUNIT_TEST( testVCS );
  CPPUNIT_TEST( testVCSCommand );
  CPPUNIT_TEST( testVCSEntry );
  CPPUNIT_TEST( testVersion );
  CPPUNIT_TEST( testVi );
  CPPUNIT_TEST( testViFSM );
  CPPUNIT_TEST( testViMacros );

  CPPUNIT_TEST_SUITE_END();

public:
  /// Default constructor.
  fixture(); 
  
  void testAddress();
  void testAddressRange();
  void testConfigDialog();
  void testConfigItem();
  void testDialog();
  void testEx();
  void testFileDialog();
  void testFileStatistics();
  void testFrame();
  void testFrd();
  void testGrid();
  void testHexMode();
  void testIndicator();
  void testLexer();
  void testLexers();
  void testLink();
  void testListItem();
  void testListView();
  void testManagedFrame();
  void testMarker();
  void testMenu();
  void testNotebook();
  void testOTL();
  void testPrinting();
  void testProcess();
  void testProperty();
  void testShell();
  void testStatistics();
  void testStatusBar();
  void testSTC();
  void testSTCEntryDialog();
  void testSTCFile();
  void testStyle();
  void testTextFile();
  void testToVectorString();
  void testUtil();
  void testVariable();
  void testVCS();
  void testVCSCommand();
  void testVCSEntry();
  void testVersion();
  void testVi();
  void testViFSM();
  void testViMacros();
private:
  void Process(const std::string& str, wxExSTCShell* shell);

  wxExManagedFrame* m_Frame;
  static wxExStatusBar* m_StatusBar;
  
  const std::vector<std::pair<std::string, std::string>> m_Abbreviations;
  const std::vector<std::string> m_BuiltinVariables;
};
