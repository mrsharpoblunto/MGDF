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
#include "MGDFModuleInitialiserImpl.hpp"
#include "MGDFParameterConstants.hpp"
#include "MGDFPreferenceConstants.hpp"
#include "MGDFModuleInitialiserImpl.hpp"
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

void ISystemImpl::AddModuleSuspendedCallback(const ModuleStatusChangeFunction::slot_type& callback)
{
	_moduleSuspendedFunction.connect(callback);
}

void ISystemImpl::AddModuleResumedCallback(const ModuleStatusChangeFunction::slot_type& callback)
{
	_moduleResumedFunction.connect(callback);
}

void ISystemImpl::AddModulePoppedCallback(const ModuleStatusChangeFunction::slot_type& callback)
{
	_modulePoppedFunction.connect(callback);
}

System::System(Game *game)
{
	_game = game;
	_saves = NULL;
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

	_globalCallbacks = new GlobalCallbacks(_game);

	//map essential directories to the vfs
	//ensure the vfs automatically enumerates zip files
	_vfs->RegisterArchiveHandler(vfs::CreateZipArchiveHandlerImpl(GetLoggerImpl(),this));
	//ensure the vfs enumerates any custom defined archive formats
	ICustomArchiveHandlers *customHandlers = _globalCallbacks->GetCustomArchiveHandlers();

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

	//enumerate the current games modules & content dirs
	_vfs->MapDirectory(Resources::Instance().ModulesDir(_game->GetUid()).c_str(),Resources::VFS_MODULES.c_str(),_vfs->GetFilterFactory()->CreateFileExtensionInclusionFilter("dll"),true);
	_vfs->MapDirectory(Resources::Instance().ContentDir(_game->GetUid()).c_str(),Resources::VFS_CONTENT.c_str(),NULL,true);

	//set the initial sound volumes
	if (_sound!=NULL)
	{
		_sound->SetSoundVolume((float)atof(_game->GetPreference(PreferenceConstants::SOUND_VOLUME)));
		_sound->SetStreamVolume((float)atof(_game->GetPreference(PreferenceConstants::MUSIC_VOLUME)));
	}

	GetLogger()->Add(THIS_NAME,"Initialised system components successfully",LOG_LOW);
	
	//queue up a bootstrapping event
	QueueBoot();
}

std::string System::GetSystemInformation(SystemStats *stats)
{
	std::stringstream ss;
	ss.setf(std::ios::fixed);
	ss.precision(4);

	ss << "MGDF Version: " << MGDFVersionInfo::MGDF_VERSION << "\r\nMGDF Interface version: " << MGDFVersionInfo::MGDF_INTERFACE_VERSION << "\r\nModule Stack:\r\n";
	BOOST_FOREACH(IModule *module,_moduleStack) {
		ss << "  " << _moduleMetaData[module].Name << "\r\n";
	}

	ss << "\r\nModule Files:\r\n";
	for (stdext::hash_map<std::string,ModuleFile>::iterator iter = _loadedModuleFiles.begin();iter!=_loadedModuleFiles.end();++iter) {
		ss << "  " << iter->first << "\r\n";
	}

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

void System::DisposeModules()
{
	//finish up any events to be processed
	ProcessQueuedEvents();

	//clear all the modules off the stack
	std::list<IModule *>::iterator iter;
	for (iter=_moduleStack.begin();iter!=_moduleStack.end();++iter) {
		if (!(*iter)->Dispose()) {
			FatalError(THIS_NAME,"Error disposing module '"+_moduleMetaData[*iter].Name+"' - "+(*iter)->GetLastError());
		}
	}
	GetLoggerImpl()->Add(THIS_NAME,"Freed all modules successfully");

	//close all open module libraries
	stdext::hash_map<std::string,ModuleFile>::iterator jter;
	for (jter=_loadedModuleFiles.begin();jter!=_loadedModuleFiles.end();++jter) {
		#if defined(FREE_UNUSED_LIBRARIES)
		FreeLibrary(jter->second.Instance);
		#endif
	}
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
	SAFE_DELETE(_globalCallbacks);

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
		else if (dynamic_cast<events::PushLoadEvent *>(e)) {
			PushLoadModule(static_cast<events::PushLoadEvent *>(e));
		}
		else if (dynamic_cast<events::SwapEvent *>(e)) {
			SwapTopModule(static_cast<events::SwapEvent *>(e));
		}
		else if (dynamic_cast<events::PushNewEvent *>(e)) {
			PushNewModule(static_cast<events::PushNewEvent *>(e));
		}
		else if (dynamic_cast<events::PopEvent *>(e)) {
			events::PopEvent *evt = static_cast<events::PopEvent *>(e);
			if (evt->PopCount>=_moduleStack.size()) {
				FatalError(THIS_NAME,"unable to pop "+boost::lexical_cast<std::string>(evt->PopCount)+" modules off the stack, only "+boost::lexical_cast<std::string>(_moduleStack.size())+" modules present");
			}
			PopModules(evt);
			if (_moduleStack.size()>0) {
				ResumeModule(GetTopModule());
			}
			else {
				ShutDown();
			}
		}

		delete e;
	}
}

IModuleInitialiser *System::CreateModuleInitialiser() const
{
	return new ModuleInitialiser();
}

void System::QueuePopModules(unsigned int val)
{
	_eventQueue.push_back(events::EventFactory::CreatePopEvent(val));
}

void System::QueueSwapTopModule(const char *s,IModuleInitialiser *init)
{
	_eventQueue.push_back(events::EventFactory::CreateSwapEvent(std::string(s),init));
}

void System::QueueSaveGameState(const char *name)
{
	_eventQueue.push_back(events::EventFactory::CreateSaveEvent(std::string(name)));
}

void System::QueueLoadGameState(const char *name)
{
	_eventQueue.push_back(events::EventFactory::CreateLoadEvent(std::string(name),false));
}

void System::QueueBoot() 
{
	_eventQueue.push_back(events::EventFactory::CreateLoadEvent("boot",true));
}

void System::QueuePushNewModule(const char *moduleName,IModuleInitialiser *moduleInitialiser)
{
	_eventQueue.push_back(events::EventFactory::CreatePushNewEvent(std::string(moduleName),moduleInitialiser));
}

void System::QueuePushLoadModule(std::string moduleName, MGDF::IModuleInitialiser *moduleInitialiser,std::string loadDataDir) {
	_eventQueue.push_back(events::EventFactory::CreatePushLoadEvent(moduleName,moduleInitialiser,loadDataDir));
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
	//clear all modules off the stack before loading
	if (_moduleStack.size()>0) {
		events::PopEvent pe(_moduleStack.size());	
		PopModules(&pe);
	}

	std::string loadFile;
	std::string loadDir;
	std::string loadDataDir;

	if (e->IsBootLoad) {
		loadFile = Resources::Instance().GameStateBootFile(_game->GetUid());
		loadDir = Resources::Instance().BootDir(_game->GetUid());
		loadDataDir = Resources::Instance().BootDataDir(_game->GetUid());
	}
	else {
		loadFile = Resources::Instance().GameStateSaveFile(_game->GetUid(),e->Loadname);
		loadDir = Resources::Instance().SaveDir(_game->GetUid(),e->Loadname);
		loadDataDir = Resources::Instance().SaveDataDir(_game->GetUid(),e->Loadname);
	}

	//then reload all modules
    try
    {
		//clear out the working directory
		ClearWorkingDirectory();

		std::auto_ptr<xml::IGameStateXMLHandler> handler(_xml->CreateGameStateXMLHandler(_game->GetUid(),_game->GetVersion()));
		//if the game state requires a migration up to the current version then perform the required changes to the game state schema
		//but don't bother migrating for boot files.
		try 
		{
			if (handler->Load(loadFile) && !e->IsBootLoad) {
				MigrateGameState(handler.get(),loadFile,loadDir);
			}
		}
		catch (...)
		{
			FatalError(THIS_NAME,"Unable to load game state data from "+loadDir);
		}

		//push the module load events onto the event queue for immediate processing
		for (xml::IGameStateXMLHandler::iterator iter=handler->Begin();iter!=handler->End();++iter) {
			ModuleInitialiser *moduleInitialiser = new ModuleInitialiser();
			moduleInitialiser->AddParams(iter->Parameters);
			if (e->IsBootLoad) moduleInitialiser->AddParam("boot","true");//always add boot parameter so the module can tell if this is a boot load.
			QueuePushLoadModule(iter->Name,moduleInitialiser,loadDataDir);
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
	
	IGameStateMigrator *migrator = _globalCallbacks->GetGameStateMigrator();
	if (migrator==NULL) {
		std::string warning = "WARNING: Mismatched save file cannot be migrated as no GameStateMigrator for game uid '";
		GetLoggerImpl()->Add(THIS_NAME,warning+_game->GetUid()+"' is defined");
	}
	else 
	{
		//migrate all module save files
		bool result = true;
		for (xml::IGameStateXMLHandler::iterator iter=handler->Begin();iter!=handler->End();++iter) {
			result = result && migrator->Migrate(iter->Name.c_str(),saveDataDir.c_str(),handler->GetVersion(),_game->GetVersion());

			if (!result) {
				FatalError(THIS_NAME,"unable to migrate '"+ iter->Name+"' save data in '"+saveDataDir+"' from version '" + VersionHelper::Format(handler->GetVersion())+"' to version '" + VersionHelper::Format(_game->GetVersion())+"'");
			}
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
push a module onto the stack and load its state from a file 
*/
void System::PushLoadModule(const events::PushLoadEvent *e) {
	IModule *module = PushModule(e->ModuleName);
	//load the module
	if (module==NULL || !module->LoadModule(e->LoadDataDir.c_str(),Resources::Instance().WorkingDir().c_str(),e->ModuleInitialiser))
	{
		FatalError(THIS_NAME,"Error loading module '"+e->ModuleName+"' from '"+e->LoadDataDir+"' - "+ GetTopModule()->GetLastError());
	}
}

/**
push a module onto the stack and initialise it
*/
void System::PushNewModule(const events::PushNewEvent *e) {
	IModule *module = PushModule(e->ModuleName);
	//init the module
	if (module==NULL || !module->NewModule(Resources::Instance().WorkingDir().c_str(),e->ModuleInitialiser))
	{
		FatalError(THIS_NAME,"Error initialising module '"+e->ModuleName+"' - "+GetTopModule()->GetLastError());
	}
}

/**
push a module based on its name in the vfs onto the stack 
*/
IModule *System::PushModule(std::string moduleName)
{
	IModule *module=NULL;
	ModuleMetaData metaData;
	ModuleFile moduleFile;
	GetModulePtr getModule;
	IsCompatibleInterfaceVersionPtr versionFunc;

	std::string::size_type separator = moduleName.find_first_of(".");
	if (separator!=std::string::npos) {
		std::string moduleClass = moduleName.substr(moduleName.find_first_of(":")+1);
		std::string moduleFileName = moduleName.substr(0,moduleName.find_first_of(":"));

		if (_vfs->GetFile((Resources::VFS_MODULES+moduleFileName).c_str())==NULL) {
			FatalError(THIS_NAME,"module '"+moduleFileName+"' was not found");
		}

		moduleFile = GetModuleFile(moduleFileName);

		versionFunc = (IsCompatibleInterfaceVersionPtr)GetProcAddress(moduleFile.Instance, "IsCompatibleInterfaceVersion");
		if (versionFunc==NULL) {
			FatalError(THIS_NAME,"unable to find DLL function IsCompatibleInterfaceVersion() in '"+moduleFileName+"'");
		}

		bool compatible = versionFunc(MGDFVersionInfo::MGDF_INTERFACE_VERSION);
		if (!compatible) {
			FatalError(THIS_NAME,"MGDF Interface version "+boost::lexical_cast<std::string>(MGDFVersionInfo::MGDF_INTERFACE_VERSION)+" is not compatible with '"+moduleName+"'");
		}

		//Get function pointer
		getModule = (GetModulePtr)GetProcAddress(moduleFile.Instance, "GetModule");
		if (getModule == NULL) {
			FatalError(THIS_NAME,"Unable to find DLL function GetModule() in '"+moduleFileName+"'");
		}

		//create the module
		module = getModule(moduleClass.c_str(),this);

		if (module==NULL) {
			FatalError(THIS_NAME,"Unable to find module class named '"+moduleClass+"' in module file '"+moduleFileName+"'");
		}

		//suspend the current top module
		if (_moduleStack.size()>0) {
			SuspendModule(GetTopModule());
		}
		
		//create the module metadata
		metaData.Name = moduleName;
		metaData.ModuleFile = moduleFileName;
		metaData.IsDeviceReset =false;
		metaData.IsDeviceStateSet=false;
		metaData.DeviceCapsChecked = false;
		_moduleMetaData[module] = metaData;

		_moduleStack.push_front(module);//add new module on top as current active module

		GetLoggerImpl()->Add(THIS_NAME,"pushed module '"+moduleName+"' successfully");
	}
	else {
		FatalError(THIS_NAME,"invalid module name '"+moduleName+"'");
	}
	return module;
}

System::ModuleFile System::GetModuleFile(std::string moduleFileName)
{
	ModuleFile moduleFile;
	//if module library has not already been loaded
	if (_loadedModuleFiles.find(moduleFileName)==_loadedModuleFiles.end()) {
		moduleFile.InstCount=1;

		//load the dll
		CurrentDirectoryHelper::Instance().Push(Resources::Instance().ModulesDir(_game->GetUid()));
		moduleFile.Instance = LoadLibrary(_vfs->GetFile((Resources::VFS_MODULES+moduleFileName).c_str())->GetPhysicalPath());
		CurrentDirectoryHelper::Instance().Pop();//restore the old current directory

		if (moduleFile.Instance == NULL) {
			FatalError(THIS_NAME,"unable to load Module file '"+moduleFileName+"'");
		}
		_loadedModuleFiles[moduleFileName]=moduleFile;
	} else { //else get cached copy of the library
		moduleFile = _loadedModuleFiles[moduleFileName];

		//a library reports having only one instance
		//when FREE_UNUSED_LIBRARIES is disabled
		#if defined(FREE_UNUSED_LIBRARIES)
		++(_loadedModuleClasses[moduleFileName].InstCount);
		#endif
	}
	return moduleFile;
}

/**
 suspend the current top module
*/
void System::SuspendModule(IModule *module)
{
	//pretend that we've lost the device so the suspended module cleans up its D3DPOOL_DEFAULT resources when they won't be needed
	if (!module->DeviceLost())
	{
		FatalError(THIS_NAME,"handling lost device in module '"+_moduleMetaData[module].Name+"' - "+module->GetLastError());
	}
	_moduleMetaData[module].IsDeviceStateSet = false;
	_moduleMetaData[module].IsDeviceReset = false;

	if (!module->Suspend())
	{
		FatalError(THIS_NAME,"Error suspending module '"+_moduleMetaData[module].Name+"' - "+module->GetLastError());
	}

	GetLoggerImpl()->Add(THIS_NAME,"suspended execution of module '"+_moduleMetaData[module].Name+"'");
	_moduleSuspendedFunction(module);
}

/**
 resume a suspended module
*/
void System::ResumeModule(IModule *module)
{
	if (!module->Resume()) 
	{
		FatalError(THIS_NAME,"Error resuming module '"+_moduleMetaData[module].Name+"' - "+module->GetLastError());
	}
	GetLoggerImpl()->Add(THIS_NAME,"resumed execution of module '"+_moduleMetaData[module].Name+"'");

	_moduleResumedFunction(module);
}

void System::PopModules(const events::PopEvent *e)
{
	bool isTopModule = true;
	for (int i = e->PopCount;i>0;--i) {
		IModule *module = GetTopModule();
		ModuleMetaData metaData = _moduleMetaData[module];

		GetLoggerImpl()->Add(THIS_NAME,"popping module '"+metaData.Name+"'");

		//library instance counts are not decremented when FREE_UNUSED_LIBRARIES is diabled
		#if defined(FREE_UNUSED_LIBRARIES) 
		--_loadedModuleFiles[metaData.ModuleFile].InstCount;//reduce the module instance count by one
		#endif

		_modulePoppedFunction(module);//tell any interested parties that this module is being popped from the stack

		//TODO call device lost for this module to let it clean up graphics resources.
		_moduleMetaData.erase(_moduleMetaData.find(module));//remove the metadata from the meta data hashmap
		if (!module->Dispose()) //uninitialise the module
		{
			FatalError(THIS_NAME,"Error disposing module '"+_moduleMetaData[module].Name+"' - "+module->GetLastError());
		}

		//if no more active instances of this module class, unload it.
		//however if FREE_UNUSED_LIBRARIES is disabled, this cannot occur
		#if defined(FREE_UNUSED_LIBRARIES)
		if (_loadedModuleFiles[metaData.ModuleFile].InstCount==0) {
			FreeLibrary(_loadedModuleClasses[metaData.ModuleFile].Instance);
			_loadedModuleFiles.erase(metaData.ModuleFile);
		}
		#endif

		_moduleStack.pop_front();//remove the top module	
		isTopModule = false;
	}

	GetLoggerImpl()->Add(THIS_NAME,"popped all modules successfully!");
}

/**
 swaps the top module with the specified one
*/
void System::SwapTopModule(const events::SwapEvent *e) 
{
	if (_moduleStack.size()>0) {
		events::PopEvent pe(1);	
		PopModules(&pe);
	}
	PushNewModule((events::PushNewEvent *)e);
}

/**
returns the module on the top of the stack
*/
IModule *System::GetTopModule() {
	if (_moduleStack.size()>0) {
		return *(_moduleStack.begin());
	}
	return NULL;
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
	xml::IGameStateXMLHandler::iterator iter;

	std::string saveDataDirName = Resources::Instance().SaveDataDir(_game->GetUid(),e->Savename);

	for (std::list<IModule *>::iterator moduleIter = _moduleStack.begin();moduleIter!=_moduleStack.end();++moduleIter) {
		handler->Add(_moduleMetaData[(*moduleIter)].Name);
		if (!(*moduleIter)->SaveModule(saveDataDirName.c_str())) {
			FatalError(THIS_NAME,"Error saving module state '"+_moduleMetaData[(*moduleIter)].Name+"': "+(*moduleIter)->GetLastError());
		}
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

	if (_moduleStack.size()>0) {
		IModule *module = GetTopModule();

		if (!module->UpdateScene(simulationTime)) {
			FatalError(THIS_NAME,"Error updating scene in module '"+_moduleMetaData[module].Name+"' - "+module->GetLastError());		
		}
	}
}

void System::DrawScene(double alpha)
{
	if (_moduleStack.size()>0) 
	{
		IModule *module = GetTopModule();

		//check that the d3d device satisfies the device capabilities required for this module.
		if (!_moduleMetaData[module].DeviceCapsChecked) 
		{
			if (!module->CheckDeviceCaps())
			{
				FatalError(THIS_NAME,"Device capabilities not met for module '"+_moduleMetaData[module].Name+"' - "+module->GetLastError());
			}
			_moduleMetaData[module].DeviceCapsChecked = true;
		}

		//set up the renderer state
		if (!_moduleMetaData[module].IsDeviceStateSet) 
		{
			if (!module->SetDeviceState()) {
				FatalError(THIS_NAME,"Unable to set device state for for module '"+_moduleMetaData[module].Name+"' - "+module->GetLastError());
			}
			_moduleMetaData[module].IsDeviceStateSet = true;
		}

		//reset the D3DPOOL_DEFAULT resources if needed
		if (!_moduleMetaData[module].IsDeviceReset) {
			if (!module->DeviceReset())
			{
				FatalError(THIS_NAME,"Unable to reset device for for module '"+_moduleMetaData[module].Name+"' - "+module->GetLastError());
			}
			_moduleMetaData[module].IsDeviceReset = true;
		}

		if (!module->DrawScene(alpha))
		{
			FatalError(THIS_NAME,"Error drawing scene in module '"+_moduleMetaData[module].Name+"' - "+module->GetLastError());		
		}
	}
}

void System::DeviceLost()
{
	IModule *module = GetTopModule();
	if (module!=NULL) {
		if (!module->DeviceLost()) {
			FatalError(THIS_NAME,"Error handling lost device in module '"+_moduleMetaData[module].Name+"' - "+module->GetLastError());
		}
		_moduleMetaData[module].IsDeviceReset = false;
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
	GetLoggerImpl()->Add(THIS_NAME,"notified of fatal error, telling modules to panic");
	GetLoggerImpl()->Flush();

	_fatalErrorFunction(sender,message);//signal any callbacks to the fatal error event

	for (std::list<IModule *>::iterator iter = _moduleStack.begin();iter!=_moduleStack.end();++iter) {
		(*iter)->Panic();
	}

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

IParameterManager *System::GetParameters() const
{
	return GetParameterManagerImpl();
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