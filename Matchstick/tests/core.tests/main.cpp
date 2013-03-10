#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <map>
#include <TestReporterStdout.h>
#include <vector>
#include <boost/algorithm/string.hpp>

//this snippet ensures that the location of memory leaks is reported correctly
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)

using namespace UnitTest;

template <typename T>
struct Node {
	Node<T> *Parent;
	T *Data;
	bool Execute;
	std::map<std::string, Node> Children;

	Node() : Data( nullptr ), Execute( true ) {
	}
};

struct TestFlags {
	bool Teamcity;
};

class TeamcityTestReporter : public TestReporter
{
public:
	TeamcityTestReporter() : _currentSuite( nullptr ), _suiteOpen( false ) {
	}

private:
	virtual void ReportTestStart( TestDetails const& test ) {
		if ( test.suiteName ) {
			if ( _currentSuite ) {
				if ( strcmp( test.suiteName, _currentSuite ) != 0 ) {
					printf( "##teamcity[testSuiteFinished name='%s']\r\n", _currentSuite );
					printf( "##teamcity[testSuiteStarted name='%s']\r\n", test.suiteName );
					_currentSuite = test.suiteName;
					_suiteOpen = true;
				}
			} else {
				printf( "##teamcity[testSuiteStarted name='%s']\r\n", test.suiteName );
				_currentSuite = test.suiteName;
				_suiteOpen = true;
			}
		} else if ( _currentSuite ) {
			printf( "##teamcity[testSuiteFinished name='%s']\r\n", _currentSuite );
			_currentSuite = nullptr;
			_suiteOpen = false;
		}

		printf( "##teamcity[testStarted name='%s']\r\n", test.testName );
	}
	virtual void ReportFailure( TestDetails const& test, char const* failure ) {
		printf( "##teamcity[testFailed name='%s' message='test failed' details='%s at line %d in %s' ]\r\n", test.testName, failure, test.lineNumber, test.filename );
	}
	virtual void ReportTestFinish( TestDetails const& test, float secondsElapsed ) {
		printf( "##teamcity[testFinished name='%s']\r\n", test.testName );
	}
	virtual void ReportSummary( int totalTestCount, int failedTestCount, int failureCount, float secondsElapsed ) {
		if ( _currentSuite && _suiteOpen ) {
			printf( "##teamcity[testSuiteFinished name='%s']\r\n", _currentSuite );
		}

		if ( totalTestCount == 0 ) {
			printf( "[Junkship.Tests.exe] No tests were run.\r\n" );
		} else if ( failureCount > 0 ) {
			printf( "[Junkship.Tests.exe] FAILED. Tests run: %d; Failures: %d.\r\n", totalTestCount, failedTestCount - failureCount );
		} else {
			printf( "[Junkship.Tests.exe] SUCCEEDED. Tests run: %d; Failures: 0.\r\n", totalTestCount );
		}
	}

	const char *_currentSuite;
	bool _suiteOpen;
};

void BuildTestTree( Node<Test> &tree );
void SetExecute( Node<Test> &tree, bool value );
void GetTestList( Node<Test> &tree, TestList &list );
void ParseArguments( Node<Test> &tree, TestFlags &flags, int argc, char **argv );

int main( int argc, char **argv )
{
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );

	TestFlags flags;
	flags.Teamcity = false;

	Node<Test> tree;
	BuildTestTree( tree );

	_ASSERTE( argc >= 1 );
	ParseArguments( tree, flags, argc, argv );

	TestList tests;
	GetTestList( tree, tests );

	TestReporter *reporter = flags.Teamcity ? ( TestReporter * ) new TeamcityTestReporter() : ( TestReporter * ) new TestReporterStdout();
	TestRunner runner( *reporter );

	int result = runner.RunTestsIf( tests, NULL, True(), 0 );
	delete reporter;
	return result;
}

void ParseArguments( Node<Test> &tree, TestFlags &flags, int argc, char **argv )
{
	for ( int i = 1; i < argc; ++i ) {
		std::string directive( argv[i] );

		//parse out any special flags
		if ( directive == "--teamcity" ) {
			flags.Teamcity = true;
			continue;
		}

		if ( directive[0] != '-' && directive[0] != '+' ) {
			printf( "Invalid test directive '%s'\n", directive );
			exit( 1 );
		}
		bool execute = directive[0] == '+';

		std::vector<std::string> components;
		boost::split( components, directive.substr( 1 ), boost::is_any_of( "/" ) );

		//all tests
		if ( components.size() == 1 && components[0] == "all" ) {
			SetExecute( tree, execute );
			tree.Execute = true;
			continue;
		}

		auto suite = tree.Children.find( components[0] );
		if ( suite == tree.Children.end() ) {
			printf( "No test or suite with name '%s'\n", components[0] );
			exit( 1 );
		}

		//an entire test suite
		if ( components.size() == 1 ) {
			SetExecute( suite->second, execute );
			continue;
		}

		auto test = suite->second.Children.find( components[1] );
		if ( test == suite->second.Children.end() ) {
			printf( "No test with name '%s'\n", components[1] );
			exit( 1 );
		}

		//test in a suite
		if ( components.size() == 2 ) {
			suite->second.Execute = true;
			SetExecute( test->second, execute );
			continue;
		}

		auto row = test->second.Children.find( components[2] );
		if ( row == test->second.Children.end() ) {
			printf( "No row test with name '%s'\n", components[2] );
			exit( 1 );
		}

		//a single rowtest
		if ( components.size() == 3 ) {
			suite->second.Execute = true;
			test->second.Execute = true;
			SetExecute( row->second, execute );
			continue;
		}

		printf( "Invalid test directive %s\n", directive );
		exit( 1 );
	}
}

void SetExecute( Node<Test> &tree, bool value )
{
	if ( tree.Children.size() > 0 ) {
		for ( auto iter = tree.Children.begin(); iter != tree.Children.end(); ++iter ) {
			SetExecute( iter->second, value );
		}
	}
	tree.Execute = value;
}

void GetTestList( Node<Test> &tree, TestList &list )
{
	if ( !tree.Execute ) return;

	if ( tree.Children.size() > 0 ) {
		for ( auto iter = tree.Children.begin(); iter != tree.Children.end(); ++iter ) {
			GetTestList( iter->second, list );
		}
	} else if ( tree.Data ) {
		tree.Data->next = nullptr;
		list.Add( tree.Data );
	}
}

void BuildTestTree( Node<Test> &tree )
{
	auto testList = Test::GetTestList();
	Test *head = testList.GetHead();
	while ( head ) {
		//this belongs in a suite
		if ( head->m_details.suiteName && strlen( head->m_details.suiteName ) > 0 ) {
			std::string suiteName = std::string( head->m_details.suiteName );

			auto iter = tree.Children.find( suiteName );
			Node<Test> *suiteNode;

			//ensure the tree has a child for this suite
			if ( iter == tree.Children.end() ) {
				suiteNode = & ( tree.Children[suiteName] = Node<Test>() );
			} else {
				suiteNode = & ( iter->second );
			}

			//this is a rowtest suite so we need to go one level deeper.
			if ( strncmp( "RT_", head->m_details.testName, 3 ) == 0 ) {
				std::string rowTestPair = std::string( head->m_details.testName ).substr( 3 );
				std::string::size_type index = rowTestPair.find_first_of( '_' );
				std::string rowTestSuite = rowTestPair.substr( 0, index );
				std::string rowTestName = rowTestPair.substr( index + 1 );

				iter = suiteNode->Children.find( rowTestSuite );
				Node<Test> *rowSuiteNode;

				//ensure the tree has a child for this suite
				if ( iter == suiteNode->Children.end() ) {
					rowSuiteNode = & ( suiteNode->Children[rowTestSuite] = Node<Test>() );
				} else {
					rowSuiteNode = & ( iter->second );
				}

				auto rowTestNode = & ( rowSuiteNode->Children[rowTestName] = Node<Test>() );
				rowTestNode->Data = head;
			} else {
				auto testNode = & ( suiteNode->Children[std::string( head->m_details.testName )] = Node<Test>() );
				testNode->Data = head;
			}
		} else {
			auto testNode = & ( tree.Children[std::string( head->m_details.testName )] = Node<Test>() );
			testNode->Data = head;
		}
		head = head->next;
	}
}
