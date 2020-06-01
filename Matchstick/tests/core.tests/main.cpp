#include "stdafx.h"

#include <TestReporterStdout.h>
#include <stdio.h>
#include <stdlib.h>

#include <map>
#include <string>
#include <vector>

// this snippet ensures that the location of memory leaks is reported correctly
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)

using namespace UnitTest;

template <typename T>
struct Node {
  Node<T> *Parent;
  T *Data;
  bool Execute;
  std::map<std::string, Node> Children;

  Node() : Data(nullptr), Execute(true), Parent(nullptr) {}
};

void BuildTestTree(Node<Test> &tree);
void SetExecute(Node<Test> &tree, bool value);
void GetTestList(Node<Test> &tree, TestList &list);
void ParseArguments(Node<Test> &tree, int argc, char **argv);

int main(int argc, char **argv) {
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

  Node<Test> tree;
  BuildTestTree(tree);

  _ASSERTE(argc >= 1);
  ParseArguments(tree, argc, argv);

  TestList tests;
  GetTestList(tree, tests);

  TestReporterStdout reporter;
  TestRunner runner(reporter);

  int result = runner.RunTestsIf(tests, NULL, True(), 0);
  return result;
}

void ParseArguments(Node<Test> &tree, int argc, char **argv) {
  for (int i = 1; i < argc; ++i) {
    std::string directive(argv[i]);

    if (directive[0] != '-' && directive[0] != '+') {
      printf("Invalid test directive '%s'\n", directive.c_str());
      exit(1);
    }
    bool execute = directive[0] == '+';

    std::vector<std::string> components;

    char *copy = new char[directive.size()];
    strcpy_s(copy, directive.size(), directive.c_str() + 1);
    char *context = 0;
    char *ptr = strtok_s(copy, "/", &context);
    while (ptr) {
      components.push_back(std::string(ptr));
      ptr = strtok_s(0, "/", &context);
    }
    delete[] copy;

    // all tests
    if (components.size() == 1 && components[0] == "all") {
      SetExecute(tree, execute);
      tree.Execute = true;
      continue;
    }

    auto suite = tree.Children.find(components[0]);
    if (suite == tree.Children.end()) {
      printf("No test or suite with name '%s'\n", components[0].c_str());
      exit(1);
    }

    // an entire test suite
    if (components.size() == 1) {
      SetExecute(suite->second, execute);
      continue;
    }

    auto test = suite->second.Children.find(components[1]);
    if (test == suite->second.Children.end()) {
      printf("No test with name '%s'\n", components[1].c_str());
      exit(1);
    }

    // test in a suite
    if (components.size() == 2) {
      suite->second.Execute = true;
      SetExecute(test->second, execute);
      continue;
    }

    auto row = test->second.Children.find(components[2]);
    if (row == test->second.Children.end()) {
      printf("No row test with name '%s'\n", components[2].c_str());
      exit(1);
    }

    // a single rowtest
    if (components.size() == 3) {
      suite->second.Execute = true;
      test->second.Execute = true;
      SetExecute(row->second, execute);
      continue;
    }

    printf("Invalid test directive %s\n", directive.c_str());
    exit(1);
  }
}

void SetExecute(Node<Test> &tree, bool value) {
  if (tree.Children.size() > 0) {
    for (auto iter = tree.Children.begin(); iter != tree.Children.end();
         ++iter) {
      SetExecute(iter->second, value);
    }
  }
  tree.Execute = value;
}

void GetTestList(Node<Test> &tree, TestList &list) {
  if (!tree.Execute) return;

  if (tree.Children.size() > 0) {
    for (auto iter = tree.Children.begin(); iter != tree.Children.end();
         ++iter) {
      GetTestList(iter->second, list);
    }
  } else if (tree.Data) {
    tree.Data->next = nullptr;
    list.Add(tree.Data);
  }
}

void BuildTestTree(Node<Test> &tree) {
  auto testList = Test::GetTestList();
  Test *head = testList.GetHead();
  while (head) {
    // this belongs in a suite
    if (head->m_details.suiteName && strlen(head->m_details.suiteName) > 0) {
      std::string suiteName = std::string(head->m_details.suiteName);

      auto iter = tree.Children.find(suiteName);
      Node<Test> *suiteNode;

      // ensure the tree has a child for this suite
      if (iter == tree.Children.end()) {
        suiteNode = &(tree.Children[suiteName] = Node<Test>());
      } else {
        suiteNode = &(iter->second);
      }

      // this is a rowtest suite so we need to go one level deeper.
      if (strncmp("RT_", head->m_details.testName, 3) == 0) {
        std::string rowTestPair =
            std::string(head->m_details.testName).substr(3);
        std::string::size_type index = rowTestPair.find_first_of('_');
        std::string rowTestSuite = rowTestPair.substr(0, index);
        std::string rowTestName = rowTestPair.substr(index + 1);

        iter = suiteNode->Children.find(rowTestSuite);
        Node<Test> *rowSuiteNode;

        // ensure the tree has a child for this suite
        if (iter == suiteNode->Children.end()) {
          rowSuiteNode = &(suiteNode->Children[rowTestSuite] = Node<Test>());
        } else {
          rowSuiteNode = &(iter->second);
        }

        auto rowTestNode =
            &(rowSuiteNode->Children[rowTestName] = Node<Test>());
        rowTestNode->Data = head;
      } else {
        auto testNode =
            &(suiteNode->Children[std::string(head->m_details.testName)] =
                  Node<Test>());
        testNode->Data = head;
      }
    } else {
      auto testNode = &(tree.Children[std::string(head->m_details.testName)] =
                            Node<Test>());
      testNode->Data = head;
    }
    head = head->next;
  }
}
