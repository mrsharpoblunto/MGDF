#include "StdAfx.h"

#include <iomanip>
#include <sstream>
#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
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

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
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
{
	_game = game;
	_saves = nullptr;
	_module = nullptr;
	_version = VersionHelper::Create(MGDFVersionInfo::MGDF_VERSION());
	_lastError.Description=nullptr;
	_lastError.Sender=nullptr;

	_storage = Components::Instance().Get<storage::IStorageFactoryComponent>();
	_input = Components::Instance().Get<input::IInputManagerComponent>();
	_sound = Components::Instance().Get<audio::ISoundManagerComponent>();
	_vfs = Components::Instance().Get<vfs::IVirtualFileSystemComponent>();
	_graphics = nullptr;

	_stats = new StatisticsManager();
	_d3dDevice = nullptr;

	_moduleFactory = new ModuleFactory(_game);

	//map essential directories to the vfs
	//ensure the vfs automatically enumerates zip files
	_vfs->RegisterArchiveHandler(vfs::CreateZipArchiveHandlerImpl(GetLoggerImpl(),this));

	//ensure the vfs enumerates any custom defined archive formats
	ICustomArchiveHandlers *customHandlers = _moduleFactory->GetCustomArchiveHandlers();
	if (customHandlers!=nullptr) {
		unsigned int length = 0;
		customHandlers->GetHandlers(nullptr,&length,GetLoggerImpl(),this);
		IArchiveHandler **handlers = new IArchiveHandler *[length];
		customHandlers->GetHandlers(handlers,&length,GetLoggerImpl(),this);
		for (unsigned int i=0;i<length;++i) 
		{
			_vfs->RegisterArchiveHandler(handlers[i]);
		}
		delete[] handlers;
		delete customHandlers;
	}

	//enumerate the current games content directory
	_vfs->MapDirectory(Resources::Instance().ContentDir().c_str(),Resources::VFS_CONTENT().c_str(),nullptr,true);

	//set the initial sound volumes
	if (_sound!=nullptr)
	{
		_sound->SetSoundVolume((float)atof(_game->GetPreference(PreferenceConstants::SOUND_VOLUME)));
		_sound->SetStreamVolume((float)atof(_game->GetPreference(PreferenceConstants::MUSIC_VOLUME)));
	}

	GetLogger()->Add(THIS_NAME,"Initialised system components successfully",LOG_LOW);
}

std::string System::GetSystemInformation(SystemStats *stats)
{
	std::stringstream ss;
	ss.setf(std::ios::fixed);
	ss.precision(4);

	std::vector<std::pair<const char *,double> > cpuCounters;
	std::vector<std::pair<const char *,double> > gpuCounters;

	//lock as this method is called from the render thread and we
	//dont want the sim thread modifying the cpu counters while we
	//enumerate them
	{
		boost::mutex::scoped_lock lock(_timer.Mutex());

		_timer.GetCounterAverages(cpuCounters,gpuCounters);

		ss << "MGDF Version: " << MGDFVersionInfo::MGDF_VERSION() << "\r\nMGDF Interface version: " << MGDFVersionInfo::MGDF_INTERFACE_VERSION << "\r\n";

		ss << "\r\nPerformance Statistics:\r\n";

		ss << "Render Thread\r\n";
		ss << " FPS : ";
		double avgRenderTime = stats->AvgRenderTime();
		if (avgRenderTime==0)
			ss << "N/A\r\n";
		else 
			ss << 1/avgRenderTime << "\r\n";
		double activeRenderTime = stats->AvgActiveRenderTime();
		ss << " Render CPU : " << activeRenderTime << "\r\n";
		ss << " Idle CPU : " << avgRenderTime-activeRenderTime << "\r\n";

		ss << "\r\nSim Thread\r\n";
		ss << " Expected FPS : ";
		if (stats->ExpectedSimTime()==0)
			ss << "N/A\r\n";
		else 
			ss << 1/stats->ExpectedSimTime() << "\r\n";

		ss << " Actual FPS : ";
		double simTime = stats->SimTime();
		if (simTime==0)
			ss << "N/A\r\n";
		else 
			ss << 1/simTime << "\r\n";

		double simInputTime = stats->AvgSimInputTime();
		double simAudioTime = stats->AvgSimAudioTime();
		double activeSimTime = stats->AvgActiveSimTime();
		ss << " Input CPU : " << simInputTime << "\r\n";
		ss << " Audio CPU : " << simAudioTime << "\r\n";
		ss << " Other CPU : " << activeSimTime << "\r\n";
		ss << " Idle CPU : " << simTime - (activeSimTime+simInputTime+simAudioTime) << "\r\n";

		if (gpuCounters.size()>0)
		{
			ss << "\r\nGPU\r\n";
			for (auto iter = gpuCounters.begin();iter!=gpuCounters.end();++iter)
			{
				ss << " " << iter->first << " : " << iter->second << "\r\n";
			}
		}

		if (cpuCounters.size()>0)
		{
			ss << "\r\nCPU\r\n";
			for (auto iter = cpuCounters.begin();iter!=cpuCounters.end();++iter)
			{
				ss << " " << iter->first << " : " << iter->second << "\r\n";
			}
		}
	}
	return ss.str();
}

void System::DisposeModule()
{
	if (_module!=nullptr && !_module->Dispose()) {
		_module = nullptr;
		FatalError(THIS_NAME,"Error disposing module");
	}
	GetLoggerImpl()->Add(THIS_NAME,"Freed module successfully");
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
	SAFE_DELETE(_game);
	SAFE_DELETE(_stats);
	SAFE_DELETE(_graphics);
	SAFE_DELETE(_moduleFactory);

	//clear out error information
	if (_lastError.Description!=nullptr)
	{
		delete[] _lastError.Description;
	}
	if (_lastError.Sender!=nullptr)
	{
		delete[] _lastError.Sender;
	}

	GetLogger()->Add(THIS_NAME,"Uninitialised system successfully",LOG_LOW);
}

void System::SetD3DDevice(ID3D11Device *d3dDevice)
{
	_d3dDevice = d3dDevice;
	_timer.InitGPUTimer(_d3dDevice,GPU_TIMER_BUFFER,TIMER_SAMPLES);
}

void System::CreateGraphicsImpl(ID3D11Device *device,IDXGIAdapter1 *adapter)
{
	_graphics = new GraphicsManager(device,adapter);
	if (_graphics->GetAdaptorModes()->Size()==0) {
		FatalError(THIS_NAME,"No compatible adaptor modes found");
	}
	_graphics->LoadPreferences(_game);
}

const Version *System::GetMGDFVersion() const
{
	return &_version;
}

int System::Load(const char *saveName, wchar_t *loadBuffer, unsigned int *size,Version &version)
{
	std::string loadName(saveName);
	std::wstring loadDataDir = Resources::Instance().SaveDataDir(loadName);
	std::wstring loadFile = Resources::Instance().GameStateSaveFile(loadName);
	std::wstring loadDir = Resources::Instance().SaveDir(loadName);

	if (loadDataDir.size()+1>*size || loadBuffer==nullptr)
	{
		*size = loadDataDir.size()+1;
		return *size;
	}
	else
	{
		*size = loadDataDir.size()+1;
		memcpy(loadBuffer,loadDataDir.c_str(),sizeof(wchar_t) * (*size));
		try
		{
			std::auto_ptr<storage::IGameStateStorageHandler> handler(_storage->CreateGameStateStorageHandler(_game->GetUid(),_game->GetVersion()));
			try 
			{
				handler->Load(loadFile);
				version = *handler->GetVersion();
				return 0;
			}
			catch (MGDFException ex)
			{
				FatalError(THIS_NAME,"Unable to load game state data from "+Resources::ToString(loadDir)+" - "+ex.what());
			}
			catch (...)
			{
				FatalError(THIS_NAME,"Unable to load game state data from "+Resources::ToString(loadDir));
			}
		}
		catch (MGDFException e) {
			FatalError(THIS_NAME,e.what());	
		}
		return -1;
	}
}

int System::BeginSave(const char *save, wchar_t *saveBuffer, unsigned int *size)
{
	std::string saveName(save);

	//only alphanumerics and space allowed.
	for (auto iter = saveName.begin();iter!=saveName.end();++iter)
	{
		if (!isalnum(*iter) && *iter!=' ') 
		{
			SetLastError(THIS_NAME,MGDF_ERR_INVALID_SAVE_NAME,(saveName+" is an invalid save name. Only alphanumeric characters and the space character are permitted").c_str());
			return -1;
		}
	}

	saveName.insert(0,PENDING_SAVE_PREFIX); 

	std::wstring saveBufferContent(Resources::Instance().SaveDataDir(saveName));

	if (saveBufferContent.size()+1>*size  || saveBuffer==nullptr)
	{
		*size = saveBufferContent.size()+1;
		return *size;
	}
	else
	{
		*size = saveBufferContent.size()+1;
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
		FatalError(THIS_NAME,"Unable to load game state data from "+Resources::ToString(saveBufferContent));
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
			SetLastError(THIS_NAME,MGDF_ERR_NO_PENDING_SAVE,(saveName+" is not a pending save. Ensure that BeginSave is called with a matching save name before calling CompleteSave").c_str());
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
		for (unsigned int i=0;i<_saves->Size();++i)
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
		FatalError(THIS_NAME,"Unable to complete loading game state data");
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
			FatalError(THIS_NAME,"Error initialising module - "+std::string(_module->GetLastError()));
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
		FatalError(THIS_NAME,"MGDF Interface version "+boost::lexical_cast<std::string>(MGDFVersionInfo::MGDF_INTERFACE_VERSION)+" is not compatible");
	}

	//create the module
	module = _moduleFactory->GetModule(this);

	if (module==nullptr) {
		FatalError(THIS_NAME,"Unable to create module class");
	}

	return module;
}

/**
returns the module on the top of the stack
*/
IModule *System::GetModule() const {
	return _module;
}

void System::UpdateScene(double simulationTime,SystemStats *stats,boost::mutex &statsMutex)
{
	LARGE_INTEGER inputStart = _timer.GetCurrentTimeTicks();
	_input->ProcessInput();
	LARGE_INTEGER inputEnd = _timer.GetCurrentTimeTicks();

	LARGE_INTEGER audioStart = _timer.GetCurrentTimeTicks();
	if (_sound!=nullptr) _sound->Update();
	LARGE_INTEGER audioEnd = _timer.GetCurrentTimeTicks();

	{
		boost::mutex::scoped_lock lock(statsMutex);
		stats->AppendSimInputTime(_timer.ConvertDifferenceToSeconds(inputEnd,inputStart));
		stats->AppendSimAudioTime(_timer.ConvertDifferenceToSeconds(audioEnd,audioStart));
	}

	if (_module!=nullptr) {
		if (!_module->UpdateScene(simulationTime)) {
			FatalError(THIS_NAME,"Error updating scene in module - "+std::string(_module->GetLastError()));		
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
			FatalError(THIS_NAME,"Error drawing scene in module - "+std::string(_module->GetLastError()));		
		}
	}
	_timer.End();
}

void System::BackBufferChanged()
{
	if (_module!=nullptr) {
		if (!_module->BackBufferChanged()) {
			FatalError(THIS_NAME,"Error handling back buffer change in module - "+std::string(_module->GetLastError()));
		}
	}
}

void System::FatalError(const char *sender,const char *message)
{
	boost::mutex::scoped_lock lock(_mutex);

	std::string fullMessage = "FATAL ERROR: ";
	GetLoggerImpl()->Add(sender,(fullMessage+message).c_str(),LOG_ERROR);
	GetLoggerImpl()->Add(THIS_NAME,"notified of fatal error, telling module to panic");
	GetLoggerImpl()->Flush();

	if (_module!=nullptr)
	{
		_module->Panic();
	}

	_fatalErrorFunction(sender,message);//signal any callbacks to the fatal error event

    TerminateProcess(GetCurrentProcess(), 1);
}

void System::SetLastError(const char *sender, int code,const char *description)
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

	for (unsigned int i=0;i<_saves->Size();++i)
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
	return GetLoggerImpl();
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