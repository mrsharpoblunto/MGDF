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

//to enable the forced unloading of libraries once they are no longer in use by the application
//leaving this commented out leaves the unloading of the dlls to the OS
//NOTE: with FREE_UNUSED_LIBRARIES enabled memory leaks in external modules are not reported correctly
//#define FREE_UNUSED_LIBRARIES

using namespace XERCES_CPP_NAMESPACE;

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
	_saves = NULL;
	_module = NULL;
	_version = VersionHelper::Create(MGDFVersionInfo::MGDF_VERSION);
	_lastError.Description=NULL;
	_lastError.Sender=NULL;

	_xml = Components::Instance().Get<xml::IXMLFactoryComponent>();
	_input = Components::Instance().Get<input::IInputManagerComponent>();
	_sound = Components::Instance().Get<audio::ISoundManagerComponent>();
	_vfs = Components::Instance().Get<vfs::IVirtualFileSystemComponent>();
	_graphics = NULL;

	_stats = new StatisticsManager(_game->GetUid());
	_d3dDevice = NULL;

	_moduleFactory = new ModuleFactory(_game);

	//map essential directories to the vfs
	//ensure the vfs automatically enumerates zip files
	_vfs->RegisterArchiveHandler(vfs::CreateZipArchiveHandlerImpl(GetLoggerImpl(),this));

	//ensure the vfs enumerates any custom defined archive formats
	ICustomArchiveHandlers *customHandlers = _moduleFactory->GetCustomArchiveHandlers();
	if (customHandlers!=NULL) {
		unsigned int length = 0;
		customHandlers->GetHandlers(NULL,&length,GetLoggerImpl(),this);
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
	_vfs->MapDirectory(Resources::Instance().ContentDir(_game->GetUid()).c_str(),Resources::VFS_CONTENT.c_str(),NULL,true);

	//set the initial sound volumes
	if (_sound!=NULL)
	{
		_sound->SetSoundVolume((float)atof(_game->GetPreference(PreferenceConstants::SOUND_VOLUME)));
		_sound->SetStreamVolume((float)atof(_game->GetPreference(PreferenceConstants::MUSIC_VOLUME)));
	}

	GetLogger()->Add(THIS_NAME,"Initialised system components successfully",LOG_LOW);
	
	//queue up a bootstrapping event
	_eventQueue.push_back(new events::NewEvent());
}

std::string System::GetSystemInformation(SystemStats *stats)
{
	std::stringstream ss;
	ss.setf(std::ios::fixed);
	ss.precision(4);

	ss << "MGDF Version: " << MGDFVersionInfo::MGDF_VERSION << "\r\nMGDF Interface version: " << MGDFVersionInfo::MGDF_INTERFACE_VERSION << "\r\n";

	ss << "\r\nPerformance Statistics:\r\n";

	ss << "Renderer FPS: ";
	if (stats->RenderTime==0)
		ss << "N/A\r\n";
	else 
		ss << 1/stats->RenderTime << "\r\n";
	ss << "  Render: " << stats->ActiveRenderTime << "\r\n";
	ss << "  Idle: " << stats->RenderTime-stats->ActiveRenderTime << "\r\n";

	ss << "Expected Sim FPS: ";
	if (stats->ExpectedSimTime==0)
		ss << "N/A\r\n";
	else 
		ss << 1/stats->ExpectedSimTime << "\r\n";

	ss << "Actual Sim FPS: ";
	if (stats->SimTime==0)
		ss << "N/A\r\n";
	else 
		ss << 1/stats->SimTime << "\r\n";

	ss << "  Input: " << stats->SimInputTime << "\r\n";
	ss << "  Audio: " << stats->SimAudioTime << "\r\n";
	ss << "  Other: " << stats->ActiveSimTime << "\r\n";
	ss << "  Idle: " << stats->SimTime - (stats->ActiveSimTime+stats->SimInputTime+stats->SimAudioTime) << "\r\n";

	return ss.str();
}

void System::DisposeModule()
{
	//finish up any events to be processed
	ProcessQueuedEvents();

	if (_module!=NULL && !_module->Dispose()) {
		_module = NULL;
		FatalError(THIS_NAME,"Error disposing module");
	}
	GetLoggerImpl()->Add(THIS_NAME,"Freed module successfully");
}

System::~System(void)
{
	if (_saves!=NULL)
	{
		for (std::vector<const char *>::const_iterator iter=_saves->Items()->begin();iter!=_saves->Items()->end();++iter)
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
	if (_lastError.Description!=NULL)
	{
		delete[] _lastError.Description;
	}
	if (_lastError.Sender!=NULL)
	{
		delete[] _lastError.Sender;
	}

	GetLogger()->Add(THIS_NAME,"Uninitialised system successfully",LOG_LOW);
}

void System::SetD3DDevice(IDirect3DDevice9 *d3dDevice)
{
	_d3dDevice = d3dDevice;
}

void System::CreateGraphicsImpl(IDirect3D9 *d3d9)
{
	_graphics = new GraphicsManager(d3d9);
	if (_graphics->GetAdaptorModes()->Size()==0) {
		FatalError(THIS_NAME,"No compatible adaptor modes found");
	}
	_graphics->LoadPreferences(_game);
}

const Version *System::GetMGDFVersion() const
{
	return &_version;
}

bool System::HasQueuedEvents()
{
	return _eventQueue.size()>0;
}

/**
process any events that have been queued by the modules
*/
void System::ProcessQueuedEvents()
{
	while (_eventQueue.size()>0) {//go until all queued events have been executed
		events::ISystemEvent *e = *(_eventQueue.begin());
		_eventQueue.pop_front();

		if (dynamic_cast<events::SaveEvent *>(e)) {
			SaveGameState(static_cast<events::SaveEvent *>(e));
		}
		else if (dynamic_cast<events::LoadEvent *>(e)) {
			LoadGameState(static_cast<events::LoadEvent *>(e));
		}
		else if (dynamic_cast<events::NewEvent *>(e)) {
			NewGameState(static_cast<events::NewEvent *>(e));
		}

		delete e;
	}
}

void System::QueueSaveGameState(const char *name)
{
	_eventQueue.push_back(events::EventFactory::CreateSaveEvent(std::string(name)));
}

void System::QueueLoadGameState(const char *name)
{
	_eventQueue.push_back(events::EventFactory::CreateLoadEvent(std::string(name)));
}

void System::ClearWorkingDirectory()
{
	boost::filesystem::path workingDir(Resources::Instance().WorkingDir(),boost::filesystem::native); 
	if (exists(workingDir)) {
		remove_all(workingDir);
	}
	else {
		create_directory(workingDir);
	}
}

//folder to load from
void System::LoadGameState(const events::LoadEvent *e)
{
	if (_module!=NULL && !_module->Dispose()) {
		_module = NULL;
		FatalError(THIS_NAME,"Error disposing module");
	}

	std::string loadFile = Resources::Instance().GameStateSaveFile(_game->GetUid(),e->Loadname);
	std::string loadDir = Resources::Instance().SaveDir(_game->GetUid(),e->Loadname);
	std::string loadDataDir = Resources::Instance().SaveDataDir(_game->GetUid(),e->Loadname);

    try
    {
		//clear out the working directory
		ClearWorkingDirectory();

		std::auto_ptr<xml::IGameStateXMLHandler> handler(_xml->CreateGameStateXMLHandler(_game->GetUid(),_game->GetVersion()));
		//if the game state requires a migration up to the current version then perform the required changes to the game state schema
		try 
		{
			if (handler->Load(loadFile)) {
				MigrateGameState(handler.get(),loadFile,loadDir);
			}
		}
		catch (MGDFException ex)
		{
			FatalError(THIS_NAME,"Unable to load game state data from "+loadDir+" - "+ex.what());
		}
		catch (...)
		{
			FatalError(THIS_NAME,"Unable to load game state data from "+loadDir);
		}

		_module = CreateModule();
		_moduleMetaData.IsDeviceReset =false;
		_moduleMetaData.IsDeviceStateSet=false;
		_moduleMetaData.DeviceCapsChecked = false;

		if (!_module->LoadModule(loadDataDir.c_str(),Resources::Instance().WorkingDir().c_str()))
		{
			FatalError(THIS_NAME,"Error loading module from '"+loadDataDir+"' - "+ _module->GetLastError());
		}
	}
	catch (MGDFException e) {
		FatalError(THIS_NAME,e.what());	
	}

	GetLoggerImpl()->Add(THIS_NAME,"loaded state from '"+loadDataDir+"' successfully");
}

/**
migrate a game state forward to the current version
*/
void System::MigrateGameState(xml::IGameStateXMLHandler *handler,std::string gameStateFile,std::string saveDataDir) {
	
	IGameStateMigrator *migrator = _moduleFactory->GetGameStateMigrator();
	if (migrator==NULL) {
		std::string warning = "WARNING: Mismatched save file cannot be migrated as no GameStateMigrator for game uid '";
		GetLoggerImpl()->Add(THIS_NAME,warning+_game->GetUid()+"' is defined");
	}
	else 
	{
		//migrate all module save files
		if (!migrator->Migrate(saveDataDir.c_str(),handler->GetVersion(),_game->GetVersion())) {
			FatalError(THIS_NAME,"unable to migrate save data in '"+saveDataDir+"' from version '" + VersionHelper::Format(handler->GetVersion())+"' to version '" + VersionHelper::Format(_game->GetVersion())+"'");
		}

		//save the updated module config
		handler->SetVersion(_game->GetVersion());
		handler->Save(gameStateFile);

		//clean up
		delete migrator;

		GetLoggerImpl()->Add(THIS_NAME,"migrated to version '"+VersionHelper::Format(_game->GetVersion())+"' successfully");
	}
}

/**
create and initialize a new module
*/
void System::NewGameState(const events::NewEvent *e) {
	_module = CreateModule();
	_moduleMetaData.IsDeviceReset =false;
	_moduleMetaData.IsDeviceStateSet=false;
	_moduleMetaData.DeviceCapsChecked = false;

	//init the module
	if (!_module->NewModule(Resources::Instance().WorkingDir().c_str()))
	{
		FatalError(THIS_NAME,"Error initialising module - "+std::string(_module->GetLastError()));
	}
}

/**
push a module based on its name in the vfs onto the stack 
*/
IModule *System::CreateModule()
{
	IModule *module=NULL;

	if (!_moduleFactory->IsCompatibleInterfaceVersion(MGDFVersionInfo::MGDF_INTERFACE_VERSION)) {
		FatalError(THIS_NAME,"MGDF Interface version "+boost::lexical_cast<std::string>(MGDFVersionInfo::MGDF_INTERFACE_VERSION)+" is not compatible");
	}

	//create the module
	module = _moduleFactory->GetModule(this);

	if (module==NULL) {
		FatalError(THIS_NAME,"Unable to create module class");
	}

	return module;
}

/**
returns the module on the top of the stack
*/
IModule *System::GetModule() {
	return _module;
}

/**
 this saves a list of all the modules in the stack and instructs all the modules to save thier
 internal states, also saves the persistency tree data.
*/
void System::SaveGameState(const events::SaveEvent *e)
{
	//create the subdir for the names save files
	boost::filesystem::path saveDir(Resources::Instance().SaveDir(_game->GetUid(),e->Savename),boost::filesystem::native); 
	if (!exists(saveDir))
		create_directory(saveDir);
	else {
		remove_all(saveDir);//clear the dir
		create_directory(saveDir);//recreate it
	}
	//create the save data sub-folder
	boost::filesystem::path saveDataDir(Resources::Instance().SaveDataDir(_game->GetUid(),e->Savename),boost::filesystem::native); 
	create_directory(saveDataDir);

	std::auto_ptr<xml::IGameStateXMLHandler> handler(_xml->CreateGameStateXMLHandler(_game->GetUid(),_game->GetVersion()));

	std::string saveDataDirName = Resources::Instance().SaveDataDir(_game->GetUid(),e->Savename);

	if (!_module->SaveModule(saveDataDirName.c_str())) {
		FatalError(THIS_NAME,"Error saving module state - "+std::string(_module->GetLastError()));
	}
	handler->Save(Resources::Instance().GameStateSaveFile(_game->GetUid(),e->Savename));

	GetSaves();
	bool exists = false;
	for (unsigned int i=0;i<_saves->Size();++i)
	{
		if (strcmp(e->Savename.c_str(),_saves->Get(i))==0)
		{
			exists = true;
			break;
		}
	}
	if (!exists) {
		char *copy = new char[e->Savename.size()+1];
		strcpy_s(copy,e->Savename.size()+1,e->Savename.c_str());
		_saves->Add(copy);
	}

	GetLoggerImpl()->Add(THIS_NAME,"saved configuration to '"+Resources::Instance().SaveDir(_game->GetUid(),e->Savename)+"' successfully!");
}

void System::UpdateScene(double simulationTime,SystemStats *stats,boost::mutex &statsMutex)
{
	LARGE_INTEGER inputStart = _timer.GetCurrentTimeTicks();
	_input->ReadInputDevices();
	LARGE_INTEGER inputEnd = _timer.GetCurrentTimeTicks();

	LARGE_INTEGER audioStart = _timer.GetCurrentTimeTicks();
	if (_sound!=NULL) _sound->Update();
	LARGE_INTEGER audioEnd = _timer.GetCurrentTimeTicks();

	{
		boost::mutex::scoped_lock lock(statsMutex);
		stats->SimInputTime = _timer.ConvertDifferenceToSeconds(inputEnd,inputStart);
		stats->SimAudioTime = _timer.ConvertDifferenceToSeconds(audioEnd,audioStart);
	}

	if (_module!=NULL) {
		if (!_module->UpdateScene(simulationTime)) {
			FatalError(THIS_NAME,"Error updating scene in module - "+std::string(_module->GetLastError()));		
		}
	}
}

void System::DrawScene(double alpha)
{
	if (_module!=NULL) 
	{
		//check that the d3d device satisfies the device capabilities required for this module.
		if (!_moduleMetaData.DeviceCapsChecked) 
		{
			if (!_module->CheckDeviceCaps())
			{
				FatalError(THIS_NAME,"Device capabilities not met for module - "+std::string(_module->GetLastError()));
			}
			_moduleMetaData.DeviceCapsChecked = true;
		}

		//set up the renderer state
		if (!_moduleMetaData.IsDeviceStateSet) 
		{
			if (!_module->SetDeviceState()) {
				FatalError(THIS_NAME,"Unable to set device state for for module - "+std::string(_module->GetLastError()));
			}
			_moduleMetaData.IsDeviceStateSet = true;
		}

		//reset the D3DPOOL_DEFAULT resources if needed
		if (!_moduleMetaData.IsDeviceReset) {
			if (!_module->DeviceReset())
			{
				FatalError(THIS_NAME,"Unable to reset device for for module - "+std::string(_module->GetLastError()));
			}
			_moduleMetaData.IsDeviceReset = true;
		}

		if (!_module->DrawScene(alpha))
		{
			FatalError(THIS_NAME,"Error drawing scene in module - "+std::string(_module->GetLastError()));		
		}
	}
}

void System::DeviceLost()
{
	if (_module!=NULL) {
		if (!_module->DeviceLost()) {
			FatalError(THIS_NAME,"Error handling lost device in module - "+std::string(_module->GetLastError()));
		}
		_moduleMetaData.IsDeviceReset = false;
	}
}

void System::DeviceReset()
{
}

void System::FatalError(const char *sender,const char *message)
{
	boost::mutex::scoped_lock lock(_mutex);

	std::string fullMessage = "FATAL ERROR: ";
	GetLoggerImpl()->Add(sender,(fullMessage+message).c_str(),LOG_ERROR);
	GetLoggerImpl()->Add(THIS_NAME,"notified of fatal error, telling module to panic");
	GetLoggerImpl()->Flush();

	if (_module!=NULL)
	{
		_module->Panic();
	}

	_fatalErrorFunction(sender,message);//signal any callbacks to the fatal error event


	exit(1);
}

void System::SetLastError(const char *sender, int code,const char *description)
{
	if (_lastError.Description!=NULL)
	{
		delete[] _lastError.Description;
	}
	if (_lastError.Sender!=NULL)
	{
		delete[] _lastError.Sender;
	}

	if (description!=NULL)
	{
		size_t descLen = strlen(description);
		_lastError.Description = new char[descLen+1];
		strncpy_s(_lastError.Description,sizeof(_lastError.Description),description,descLen);
	}
	if (sender!=NULL)
	{
		size_t senderLen = strlen(sender);
		_lastError.Sender = new char[senderLen+1];
		strncpy_s(_lastError.Sender,sizeof(_lastError.Description),sender,senderLen);
	}
}


void System::ShutDown()
{
	_shutDownFunction();//message the shutdown callback
}

const IStringList *System::GetSaves() const
{
	if (_saves == NULL) 
	{
		const_cast<StringList *>(_saves) = new StringList();

		boost::filesystem::path savePath(Resources::Instance().UserDir(_game->GetUid()),boost::filesystem::native);

		boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
		for ( boost::filesystem::directory_iterator itr(savePath); itr != end_itr; ++itr ) {
			if ( is_directory( *itr )) {
				char *copy = new char[itr->leaf().size()+1];
				strcpy_s(copy,itr->leaf().size()+1,itr->leaf().c_str());
				_saves->Add(copy);//add the save folder to the list
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
			boost::filesystem::path savePath(Resources::Instance().UserDir(_game->GetUid())+saveName,boost::filesystem::native);
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

GraphicsManager *System::GetGraphicsImpl()
{
	return _graphics;
}

ILogger *System::GetLogger() const
{
	return GetLoggerImpl();
}

IInputManager *System::GetInput() const
{
	return _input;
}

ISoundManager *System::GetSound() const
{
	return _sound;
}

IDirect3DDevice9 *System::GetD3DDevice() const
{
	return _d3dDevice;
}

const Error *System::GetLastError() const
{
	return &_lastError;
}

}
}