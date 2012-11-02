#include "StdAfx.h"

#include <fstream>
#include "Test3.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF { namespace Test {


Test3::~Test3(void)
{
}

Test3::Test3()
{
	_testState = 0;
}

TestModule *Test3::NextTestModule()
{
	return NULL;
}

void Test3::Update(ISystem *system,TextManagerState *state)
{
	if (system->GetInput()->IsKeyPress(VK_ESCAPE))
	{
		system->ShutDown();
	}

	if (_testState==0)
	{
		state->AddLine(WHITE,"");
		state->AddLine(WHITE,"Load/Save Tests");
		state->AddLine(WHITE,"");

		system->RemoveSave("testsave");
		state->AddLine(WHITE,"Save game state");

		unsigned int size=0;
		system->BeginSave("testsave",NULL,&size);
		wchar_t *saveDir = new wchar_t[size];
		if (system->BeginSave("testsave",saveDir,&size)==0)
		{
			std::wstring saveFile(saveDir);
			saveFile+=L"currentState.txt";
			std::ofstream out(saveFile.c_str(),std::ios::out);
			out << 2;
			out.close();
			state->SetStatus(GREEN,"[Test Passed]");
			state->AddLine(WHITE,"Search saved game state");
			_testState = 1;
		}
		else
		{
			state->SetStatus(RED,"[Test Failed]");
			_testState = 999;
		}
		delete[] saveDir;
	}
	else if (_testState==1)
	{
		//we didn't complete saving yet so it shouldn't appear in the list
		if (system->GetSaves()->Size()!=0)
		{
			state->SetStatus(RED,"[Test Failed]");
			_testState = 999;
		}
		else
		{
			if (system->CompleteSave("testsave") && system->GetSaves()->Size()==1 && strcmp(system->GetSaves()->Get(0),"testsave")==0)
			{
				state->SetStatus(GREEN,"[Test Passed]");
				state->AddLine(WHITE,"Load game state");

				unsigned size=0;
				MGDF::Version version;
				system->Load("testsave",NULL,&size,version);
				wchar_t *saveDir = new wchar_t[size];
				if (system->Load("testsave",saveDir,&size,version)==0)
				{
					//make sure the version we loaded is the same that we saved.
					if (version.Major!=0 || version.Minor != 1) 
					{
						state->SetStatus(RED,"[Test Failed - Invalid version]");
						_testState = 999;
					}
					else
					{
						std::wstring saveFile(saveDir);
						saveFile+=L"currentState.txt";
						std::ifstream in(saveFile.c_str(),std::ios::in);
						if (in.fail()) 
						{
							_testState = 3;
						}
						else
						{
							in >> _testState;
							in.close();
							if (_testState!=2) _testState = 3;
						}
					}
				}
				else 
				{
					state->SetStatus(RED,"[Test Failed - Unable to load]");
					_testState = 999;
				}
				delete[] saveDir;
			}
			else 
			{
				state->SetStatus(RED,"[Test Failed]");
				_testState = 999;
			}
		}
	}
	else if (_testState==2)
	{
		state->SetStatus(GREEN,"[Test Passed]");
		_testState = 4;
	}
	else if (_testState==3)
	{
		state->SetStatus(RED,"[Test Failed]");
		_testState = 999;
	}
	else if (_testState==4)
	{
		state->AddLine(WHITE,"Testing custom VFS archive handler registration");
		
		bool success = false;
		IFile *file = system->GetVFS()->GetFile(L"/content/test.fakearchive/testfile.txt");
		if (file!=NULL)
		{
			file->OpenFile();
			char *data = new char[file->GetSize()];
			file->Read(data,file->GetSize());
			success = strncmp(data,"hello world",file->GetSize())==0;
			delete[] data;
		}

		if (!success)
		{
			state->SetStatus(RED,"[Test Failed]");
			_testState = 999;
		}
		else
		{
			state->SetStatus(GREEN,"[Test Passed]");
			_testState++;
		}
	}
	else if (_testState==5)
	{
		state->AddLine(WHITE,"Press [ALT]+[ENTER] to toggle fullscreen/windowed mode. Then press [Y/N] if this works correctly");
		_testState++;
	}
	else if (_testState==6 && system->GetInput()->IsKeyPress('Y'))
	{
		++_testState;
		state->SetStatus(GREEN,"[Test Passed]");
		state->AddLine(WHITE,"Press [ALT]+[F12] to toggle the system information overlay. Then press [Y/N] if this works correctly");
	}
	else if (_testState==6 && system->GetInput()->IsKeyPress('N'))
	{
		_testState =999;
		state->SetStatus(RED,"[Test Failed]");
	}
	else if (_testState==7 && system->GetInput()->IsKeyPress('Y'))
	{
		_testState =999;
		state->SetStatus(GREEN,"[Test Passed]");
	}
	else if (_testState==7 && system->GetInput()->IsKeyPress('N'))
	{
		_testState =999;
		state->SetStatus(RED,"[Test Failed]");
	}
	else if (_testState==999)
	{
		_testState++;
		state->AddLine(WHITE,"All tests complete. Press the [ESC] key to exit");
	}
}

}}