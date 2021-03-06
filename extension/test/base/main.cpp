////////////////////////////////////////////////////////////////////////////////
// Name:      main.cpp
// Purpose:   main for wxExtension cpp unit testing
// Author:    Anton van Wezenbeek
// Copyright: (c) 2015
////////////////////////////////////////////////////////////////////////////////

#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestRunner.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/utils.h>

#include "test.h"

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( TestFixture );

int main (int argc, char* argv[])
{
  SetWorkingDirectory();
  SetEnvironment(wxGetHomeDir() + "/.wxex-test-base");
  
  // Get the top level suite from the registry
  CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();

  CppUnit::TextUi::TestRunner runner;
  runner.addTest(suite);

  return !runner.run("", false);
}
