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

	_stats = new StatisticsManager();
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
	_vfs->MapDirectory(Resources::Instance().ContentDir().c_str(),Resources::VFS_CONTENT.c_str(),NULL,true);

	//set the initial sound volumes
	if (_sound!=NULL)
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

	ss << "MGDF Version: " << MGDFVersionInfo::MGDF_VERSION << "\r\nMGDF Interface version: " << MGDFVersionInfo::MGDF_INTERFACE_VERSION << "\r\n";

	ss << "\r\nPerformance Statistics:\r\n";

	ss << "Renderer FPS: ";
	double avgRenderTime = stats->AvgRenderTime();
	if (avgRenderTime==0)
		ss << "N/A\r\n";
	else 
		ss << 1/avgRenderTime << "\r\n";
	double activeRenderTime = stats->AvgActiveRenderTime();
	ss << "  Render: " << activeRenderTime << "\r\n";
	ss << "  Idle: " << avgRenderTime-activeRenderTime << "\r\n";

	ss << "Expected Sim FPS: ";
	if (stats->ExpectedSimTime()==0)
		ss << "N/A\r\n";
	else 
		ss << 1/stats->ExpectedSimTime() << "\r\n";

	ss << "Actual Sim FPS: ";
	double simTime = stats->SimTime();
	if (simTime==0)
		ss << "N/A\r\n";
	else 
		ss << 1/simTime << "\r\n";

	double simInputTime = stats->AvgSimInputTime();
	double simAudioTime = stats->AvgSimAudioTime();
	double activeSimTime = stats->AvgActiveSimTime();
	ss << "  Input: " << simInputTime << "\r\n";
	ss << "  Audio: " << simAudioTime << "\r\n";
	ss << "  Other: " << activeSimTime << "\r\n";
	ss << "  Idle: " << simTime - (activeSimTime+simInputTime+simAudioTime) << "\r\n";

	return ss.str();
}

void System::DisposeModule()
{
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

int System::Load(const char *saveName, wchar_t *loadBuffer, unsigned int *size,Version &version)
{
	std::string loadName(saveName);
	std::wstring loadDataDir = Resources::Instance().SaveDataDir(loadName);
	std::wstring loadFile = Resources::Instance().GameStateSaveFile(loadName);
	std::wstring loadDir = Resources::Instance().SaveDir(loadName);

	if (loadDataDir.size()+1>*size || loadBuffer==NULL)
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
			std::auto_ptr<xml::IGameStateXMLHandler> handler(_xml->CreateGameStateXMLHandler(_game->GetUid(),_game->GetVersion()));
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

int System::Save(const char *save, wchar_t *saveBuffer, unsigned int *size)
{
	std::string saveName(save);
	std::wstring saveBufferContent(Resources::Instance().SaveDataDir(saveName));

	if (saveBufferContent.size()+1>*size  || saveBuffer==NULL)
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

		std::auto_ptr<xml::IGameStateXMLHandler> handler(_xml->CreateGameStateXMLHandler(_game->GetUid(),_game->GetVersion()));
		handler->Save(Resources::Instance().GameStateSaveFile(saveName));

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
		return 0;
	}
	catch (...)
	{
		FatalError(THIS_NAME,"Unable to load game state data from "+Resources::ToString(saveBufferContent));
		return -1;
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
	if (_module==NULL)
	{
		_module = CreateModule();
		_moduleMetaData.IsDeviceReset =false;
		_moduleMetaData.IsDeviceStateSet=false;
		_moduleMetaData.DeviceCapsChecked = false;

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
		stats->AppendSimInputTime(_timer.ConvertDifferenceToSeconds(inputEnd,inputStart));
		stats->AppendSimAudioTime(_timer.ConvertDifferenceToSeconds(audioEnd,audioStart));
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

    TerminateProcess(GetCurrentProcess(), 1);
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

void System::QueueShutDown()
{
	if (_module!=NULL) {
		_module->ShutDown();
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

		boost::filesystem::wpath savePath(Resources::Instance().SaveBaseDir(),boost::filesystem::native);

		boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
		for ( boost::filesystem::directory_iterator itr(savePath); itr != end_itr; ++itr ) {
			if ( is_directory( *itr )) {
				std::string saveName(itr->path().filename().string());
				char *copy = new char[saveName.size()+1];
				strcpy_s(copy,saveName.size()+1,saveName.c_str());
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

GraphicsManager *System::GetGraphicsImpl()
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