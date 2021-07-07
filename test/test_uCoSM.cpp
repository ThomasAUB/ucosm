#include <iostream>
#include <string>
#include <list>
#include <cppunit/TestCase.h>
#include <cppunit/TestFixture.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/XmlOutputter.h>
#include <netinet/in.h>

#include "test_priority.h"
CPPUNIT_TEST_SUITE_REGISTRATION( PriorityTest );

#include "test_delay.h"
CPPUNIT_TEST_SUITE_REGISTRATION( DelayTest );

#include "test_mem_pool_32.h"
CPPUNIT_TEST_SUITE_REGISTRATION( MemPool32Test );

#include "test_creator.h"
CPPUNIT_TEST_SUITE_REGISTRATION( CreatorTest );

#include "test_task_function.h"
CPPUNIT_TEST_SUITE_REGISTRATION( TaskHandlerTest );

uint8_t UcosmSysData::sCnt = 0;

int main(){
    // informs test-listener about testresults
    CPPUNIT_NS::TestResult testresult;

    // register listener for collecting the test-results
    CPPUNIT_NS::TestResultCollector collectedresults;
    testresult.addListener (&collectedresults);

    // register listener for per-test progress output
    CPPUNIT_NS::BriefTestProgressListener progress;
    testresult.addListener (&progress);

    // insert test-suite at test-runner by registry
    CPPUNIT_NS::TestRunner testrunner;
    testrunner.addTest (CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest ());
    testrunner.run(testresult);

    // output results in compiler-format
    CPPUNIT_NS::CompilerOutputter compileroutputter(&collectedresults, std::cerr);
    compileroutputter.write ();

    // Output XML for Jenkins CPPunit plugin
    // std::ofstream xmlFileOut("MicroParcelMessageTest.xml");
    // CPPUNIT_NS::XmlOutputter xmlOut(&collectedresults, xmlFileOut);
    // xmlOut.write();

    // return 0 if tests were successful
    return collectedresults.wasSuccessful() ? 0 : 1;
}
