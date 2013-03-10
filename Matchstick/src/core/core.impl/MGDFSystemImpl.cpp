#include "StdAfx.h"

#include <iomanip>
#include <sstream>
#include <boost/filesystem/operations.hpp>
#include <boost/foreach.hpp>

#include "../common/MGDFResources.hpp"
#include "../common/MGDFVersionHelper.hpp"
#include "../common/MGDFVersionInfo.hpp"
#include "../common/MGDFExceptions.hpp"
#include "MGDFSystemImpl.hpp"
#include "MGDFParameterConstants.hpp"
#include "MGDFPreferenceConstants.hpp"
#include "MGDFComponents.hpp"
#include "MGDFCurrentDirectoryHelper.hpp"

#include "../vfs/archive/zip/ZipArchiveHandlerImpl.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

#define PENDING_SAVE_PREFIX "__"

namespace MGDF { namespace core {


void ISystemImpl::AddShutDownCallback(const ShutDownFunction::slot_type& callback)
{
	_shutDownFunction.connect(callback);
}

void ISystemImpl::AddFatalErrorCallback(const FatalErrorFunction::slot_type& callback)
{
	_fatalErrorFunction.connect(callback);
}

System::System(Game *game)
	: _game(game)
	, _saves(nullptr)
	, _module(nullptr)
	, _version(VersionHelper::Create(MGDFVersionInfo::MGDF_VERSION()))
	, _storage(Components::Instance().Get<storage::IStorageFactoryComponent>())
	, _input(Components::Instance().Get<input::IInputManagerComponent>())
	, _sound(Components::Instance().Get<audio::ISoundManagerComponent>())
	, _vfs(Components::Instance().Get<vfs::IVirtualFileSystemComponent>())
	, _graphics(nullptr)
	, _stats(new StatisticsManager())
	, _d3dDevice(nullptr)
{
	_lastError.Description=nullptr;
	_lastError.Sender=nullptr;

	_ASSERTE(game);
	_moduleFactory = new ModuleFactory(_game);

	//map essential directories to the vfs
	//ensure the vfs automatically enumerates zip files
	_vfs->RegisterArchiveHandler(vfs::zip::CreateZipArchiveHandlerImpl(this));

	//ensure the vfs enumerates any custom defined archive formats
	UINT32 length = 0;
	_moduleFactory->GetCustomArchiveHandlers(nullptr,&length,&Logger::Instance(),this);
	if (length > 0) {
		IArchiveHandler **handlers = new IArchiveHandler *[length];
		_moduleFactory->GetCustomArchiveHandlers(handlers,&length,&Logger::Instance(),this);
		for (UINT32 i=0;i<length;++i)  {
			_vfs->RegisterArchiveHandler(handlers[i]);
		}
		delete[] handlers;
	}

	//enumerate the current games content directory
	_vfs->Mount(Resources::Instance().ContentDir().c_str());

	//set the initial sound volumes
	if (_sound!=nullptr)
	{
		_sound->SetSoundVolume((float)atof(_game->GetPreference(PreferenceConstants::SOUND_VOLUME)));
		_sound->SetStreamVolume((float)atof(_game->GetPreference(PreferenceConstants::MUSIC_VOLUME)));
	}

	LOG("Initialised system components successfully",LOG_LOW);
}

std::string System::GetSystemInformation(SystemStats &stats)
{
	Timings timings;
	stats.GetTimings(timings);

	std::stringstream ss;
	ss.setf(std::ios::fixed);
	ss.precision(4);

	ss << "MGDF Version: " << MGDFVersionInfo::MGDF_VERSION() << "\r\nMGDF Interface version: " << MGDFVersionInfo::MGDF_INTERFACE_VERSION << "\r\n";
	ss << "\r\nPerformance Statistics:\r\n";

	ss << "Render Thread\r\n";
	ss << " FPS : ";
	if (timings.AvgRenderTime==0)
		ss << "N/A\r\n";
	else 
		ss << 1/timings.AvgRenderTime << "\r\n";
	ss << " Render CPU : " << timings.AvgActiveRenderTime << "\r\n";
	ss << " Idle CPU : " << timings.AvgRenderTime-timings.AvgActiveRenderTime << "\r\n";

	ss << "\r\nSim Thread\r\n";
	ss << " Expected FPS : ";
	if (timings.ExpectedSimTime==0)
		ss << "N/A\r\n";
	else 
		ss << 1/timings.ExpectedSimTime << "\r\n";

	ss << " Actual FPS : ";
	if (timings.AvgSimTime==0)
		ss << "N/A\r\n";
	else 
		ss << 1/timings.AvgSimTime << "\r\n";

	ss << " Input CPU : " << timings.AvgSimInputTime << "\r\n";
	ss << " Audio CPU : " << timings.AvgSimAudioTime << "\r\n";
	ss << " Other CPU : " << timings.AvgActiveSimTime << "\r\n";
	ss << " Idle CPU : " << (timings.AvgSimTime - timings.AvgActiveSimTime - timings.AvgSimInputTime - timings.AvgSimAudioTime) << "\r\n";

	_timer.GetCounterInformation(ss);

	return ss.str();
}

void System::DisposeModule()
{
	if (_module!=nullptr && !_module->Dispose()) {
		_module = nullptr;
		FATALERROR(this,"Error disposing module");
	}
	LOG("Freed module successfully",LOG_LOW);
}

System::~System(void)
{
	if (_saves!=nullptr)
	{
		for (auto iter=_saves->Items()->begin();iter!=_saves->Items()->end();++iter)
		{
			delete[] *iter;
		}
		delete _saves;
	}
	delete _game;
	delete _stats;
	delete _graphics;
	delete _moduleFactory;

	//clear out error information
	if (_lastError.Description!=nullptr)
	{
		delete[] _lastError.Description;
	}
	if (_lastError.Sender!=nullptr)
	{
		delete[] _lastError.Sender;
	}

	LOG("Uninitialised system successfully",LOG_LOW);
}

void System::SetD3DDevice(ID3D11Device *d3dDevice)
{
	_d3dDevice = d3dDevice;
	_timer.InitGPUTimer(_d3dDevice,GPU_TIMER_BUFFER,TIMER_SAMPLES);
}

UINT32 System::GetCompatibleD3DFeatureLevels(D3D_FEATURE_LEVEL *levels,UINT32 *featureLevelsSize)
{
	return _moduleFactory->GetCompatibleFeatureLevels(levels,featureLevelsSize);
}

void System::CreateGraphicsImpl(ID3D11Device *device,IDXGIAdapter1 *adapter)
{
	_graphics = new GraphicsManager(device,adapter);
	if (_graphics->GetAdaptorModes()->Size()==0) {
		FATALERROR(this,"No compatible adaptor modes found");
	}
	_graphics->LoadPreferences(_game);
}

const Version *System::GetMGDFVersion() const
{
	return &_version;
}

INT32 System::Load(const char *saveName, wchar_t *loadBuffer, UINT32 *size,Version &version)
{
	std::string loadName(saveName);
	std::wstring loadDataDir = Resources::Instance().SaveDataDir(loadName);
	std::wstring loadFile = Resources::Instance().GameStateSaveFile(loadName);
	std::wstring loadDir = Resources::Instance().SaveDir(loadName);

	if (loadDataDir.size()+1>UINT32_MAX)
	{
		FATALERROR(this,"Unable to store load data path in 32bit buffer");
		return -1;
	}

	if (loadDataDir.size()+1>*size || loadBuffer==nullptr)
	{
		*size = static_cast<UINT32>(loadDataDir.size())+1;
		return *size;
	}
	else
	{
		*size = static_cast<UINT32>(loadDataDir.size())+1;
		memcpy(loadBuffer,loadDataDir.c_str(),sizeof(wchar_t) * (*size));

		std::auto_ptr<storage::IGameStateStorageHandler> handler(_storage->CreateGameStateStorageHandler(_game->GetUid(),_game->GetVersion()));
		try 
		{
			handler->Load(loadFile);
			version = *handler->GetVersion();
			return 0;
		}
		catch (MGDFException ex)
		{
			FATALERROR(this,"Unable to load game state data from "+Resources::ToString(loadDir)+" - "+ex.what());
		}
		catch (...)
		{
			FATALERROR(this,"Unable to load game state data from "+Resources::ToString(loadDir));
		}
		return -1;
	}
}

INT32 System::BeginSave(const char *save, wchar_t *saveBuffer, UINT32 *size)
{
	std::string saveName(save);

	//only alphanumerics and space allowed.
	for (auto iter = saveName.begin();iter!=saveName.end();++iter)
	{
		if (!isalnum(*iter) && *iter!=' ') 
		{
			SETLASTERROR(this,MGDF_ERR_INVALID_SAVE_NAME,saveName << " is an invalid save name. Only alphanumeric characters and the space character are permitted");
			return -1;
		}
	}

	saveName.insert(0,PENDING_SAVE_PREFIX); 

	std::wstring saveBufferContent(Resources::Instance().SaveDataDir(saveName));

	if (saveBufferContent.size()+1>UINT32_MAX)
	{
		FATALERROR(this,"Unable to store save data path in 32bit buffer");
		return -1;
	}

	if (saveBufferContent.size()+1>*size  || saveBuffer==nullptr)
	{
		*size = static_cast<UINT32>(saveBufferContent.size())+1;
		return *size;
	}
	else
	{
		*size = static_cast<UINT32>(saveBufferContent.size())+1;
		memcpy(saveBuffer,saveBufferContent.c_str(),sizeof(wchar_t) * (*size));
	}

	try 
	{
		//create the subdir for the names save files
		boost::filesystem::wpath saveDir(Resources::Instance().SaveDir(saveName),boost::filesystem::native); 
		if (!exists(saveDir))
			create_directory(saveDir);
		else {
			remove_all(saveDir);//clear the dir
			create_directory(saveDir);//recreate it
		}
		//create the save data sub-folder
		boost::filesystem::wpath saveDataDir(saveBufferContent,boost::filesystem::native); 
		create_directory(saveDataDir);

		std::auto_ptr<storage::IGameStateStorageHandler> handler(_storage->CreateGameStateStorageHandler(_game->GetUid(),_game->GetVersion()));
		handler->Save(Resources::Instance().GameStateSaveFile(saveName));

		return 0;
	}
	catch (...)
	{
		FATALERROR(this,"Unable to load game state data from " << Resources::ToString(saveBufferContent));
		return -1;
	}
}

bool System::CompleteSave(const char *save)
{
	try 
	{
		std::string saveName(save);
		std::string pendingSave(save);
		pendingSave.insert(0,PENDING_SAVE_PREFIX);

		boost::filesystem::wpath pendingSaveDir(Resources::Instance().SaveDir(pendingSave),boost::filesystem::native); 
		boost::filesystem::wpath saveDir(Resources::Instance().SaveDir(saveName),boost::filesystem::native); 

		if (!exists(pendingSaveDir)) 
		{
			SETLASTERROR(this,MGDF_ERR_NO_PENDING_SAVE,saveName << " is not a pending save. Ensure that BeginSave is called with a matching save name before calling CompleteSave");
			return false;
		}

		if (exists(saveDir))
		{
			remove_all(saveDir);
		}
		//swap the pending with the completed save
		boost::system::error_code ec;
		boost::filesystem::rename(pendingSaveDir,saveDir);

		//update the list of save games
		GetSaves();
		bool exists = false;
		for (UINT32 i=0;i<_saves->Size();++i)
		{
			if (strcmp(saveName.c_str(),_saves->Get(i))==0)
			{
				exists = true;
				break;
			}
		}
		if (!exists) {
			char *copy = new char[saveName.size()+1];
			strcpy_s(copy,saveName.size()+1,saveName.c_str());
			_saves->Add(copy);
		}

		return true;
	}
	catch (...)
	{
		FATALERROR(this,"Unable to complete loading game state data");
		return false;
	}
}

void System::ClearWorkingDirectory()
{
	boost::filesystem::wpath workingDir(Resources::Instance().WorkingDir(),boost::filesystem::native); 
	if (exists(workingDir)) {
		remove_all(workingDir);
	}
	else {
		create_directory(workingDir);
	}
}

/**
create and initialize a new module
*/
void System::Initialize() {
	if (_module==nullptr)
	{
		_module = CreateModule();

		//init the module
		if (!_module->New(Resources::Instance().WorkingDir().c_str()))
		{
			FATALERROR(this,"Error initialising module - " << _module->GetLastError());
		}
	}
}

/**
push a module based on its name in the vfs onto the stack 
*/
IModule *System::CreateModule()
{
	IModule *module=nullptr;

	if (!_moduleFactory->IsCompatibleInterfaceVersion(MGDFVersionInfo::MGDF_INTERFACE_VERSION)) {
		FATALERROR(this,"MGDF Interface version " << MGDFVersionInfo::MGDF_INTERFACE_VERSION << " is not compatible");
	}

	//create the module
	module = _moduleFactory->GetModule(this);

	if (module==nullptr) {
		FATALERROR(this,"Unable to create module class");
	}

	return module;
}

/**
returns the module on the top of the stack
*/
IModule *System::GetModule() const {
	return _module;
}

void System::UpdateScene(double simulationTime,SystemStats &stats)
{
	LARGE_INTEGER inputStart = _timer.GetCurrentTimeTicks();
	_input->ProcessSim();
	LARGE_INTEGER inputEnd = _timer.GetCurrentTimeTicks();

	LARGE_INTEGER audioStart = _timer.GetCurrentTimeTicks();
	if (_sound!=nullptr) _sound->Update();
	LARGE_INTEGER audioEnd = _timer.GetCurrentTimeTicks();

	stats.AppendSimInputAndAudioTimes(
		_timer.ConvertDifferenceToSeconds(inputEnd,inputStart),
		_timer.ConvertDifferenceToSeconds(audioEnd,audioStart));

	if (_module!=nullptr) {
		if (!_module->UpdateScene(simulationTime)) {
			FATALERROR(this,"Error updating scene in module - " << _module->GetLastError());		
		}
	}
}

void System::DrawScene(double alpha)
{
	_timer.Begin();
	if (_module!=nullptr) 
	{
		if (!_module->DrawScene(alpha))
		{
			FATALERROR(this,"Error drawing scene in module - " << _module->GetLastError());		
		}
	}
	_timer.End();
}

void System::BackBufferChanged()
{
	if (_module!=nullptr) {
		if (!_module->BackBufferChanged()) {
			FATALERROR(this,"Error handling back buffer change in module - " << _module->GetLastError());
		}
	}
}

void System::FatalError(const char *sender,const char *message)
{
	boost::mutex::scoped_lock lock(_mutex);

	std::ostringstream ss;
	ss << "FATAL ERROR: " << message;
	Logger::Instance().Add(sender,ss.str().c_str(),LOG_ERROR);
	LOG("Notified of fatal error, telling module to panic",LOG_ERROR);
	Logger::Instance().Flush();

	if (_module!=nullptr)
	{
		_module->Panic();
	}

	_fatalErrorFunction(sender,message);//signal any callbacks to the fatal error event

    TerminateProcess(GetCurrentProcess(), 1);
}

void System::SetLastError(const char *sender, UINT32 code,const char *description)
{
	if (_lastError.Description!=nullptr)
	{
		delete[] _lastError.Description;
	}
	if (_lastError.Sender!=nullptr)
	{
		delete[] _lastError.Sender;
	}

	if (description!=nullptr)
	{
		size_t descLen = strlen(description);
		_lastError.Description = new char[descLen+1];
		strncpy_s(_lastError.Description,descLen+1,description,descLen);
	}
	if (sender!=nullptr)
	{
		size_t senderLen = strlen(sender);
		_lastError.Sender = new char[senderLen+1];
		strncpy_s(_lastError.Sender,senderLen+1,sender,senderLen);
	}
}

void System::QueueShutDown()
{
	if (_module!=nullptr) {
		_module->ShutDown();
	}
}

void System::ShutDown()
{
	_shutDownFunction();//message the shutdown callback
}

const IStringList *System::GetSaves() const
{
	if (_saves == nullptr) 
	{
		const_cast<StringList *>(_saves) = new StringList();

		boost::filesystem::wpath savePath(Resources::Instance().SaveBaseDir(),boost::filesystem::native);

		boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
		for ( boost::filesystem::directory_iterator itr(savePath); itr != end_itr; ++itr ) {
			if ( is_directory( *itr )) {
				std::string saveName(itr->path().filename().string());
				if (saveName.find(PENDING_SAVE_PREFIX)!=0)
				{
					char *copy = new char[saveName.size()+1];
					strcpy_s(copy,saveName.size()+1,saveName.c_str());
					_saves->Add(copy);//add the save folder to the list
				}
			}
		}
	}
	return _saves;
}

void System::RemoveSave(const char *saveName)
{
	GetSaves();

	for (UINT32 i=0;i<_saves->Size();++i)
	{
		if (strcmp(saveName,_saves->Get(i))==0)
		{
			delete[] _saves->Get(i);
			_saves->Remove(i);
			boost::filesystem::wpath savePath(Resources::Instance().UserBaseDir()+Resources::ToWString(saveName),boost::filesystem::native);
			boost::filesystem::remove_all(savePath);
			return;
		}
	}
}

IGame *System::GetGame() const
{
	return _game;
}

IStatisticsManager *System::GetStatistics() const
{
	return _stats;
}

IVirtualFileSystem *System::GetVFS() const
{
	return _vfs;
}

IGraphicsManager *System::GetGraphics() const
{
	return _graphics;
}

GraphicsManager *System::GetGraphicsImpl() const
{
	return _graphics;
}

ILogger *System::GetLogger() const
{
	return &Logger::Instance();
}

ITimer * System::GetTimer() const
{
	return (ITimer *)&_timer;
}

IInputManager *System::GetInput() const
{
	return _input;
}

input::IInputManagerComponent *System::GetInputManagerImpl() const
{
	return _input;
}

ISoundManager *System::GetSound() const
{
	return _sound;
}

ID3D11Device *System::GetD3DDevice() const
{
	return _d3dDevice;
}

const Error *System::GetLastError() const
{
	return &_lastError;
}

}
}