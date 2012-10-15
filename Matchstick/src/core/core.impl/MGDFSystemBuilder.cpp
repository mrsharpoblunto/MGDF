#include "StdAfx.h"

#include <MGDF/MGDF.hpp>

#include "MGDFSystemBuilder.hpp"
#include "MGDFGameBuilder.hpp"
#include "MGDFSystemImpl.hpp"
#include "../common/MGDFVersionInfo.hpp"
#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFParameterManagerImpl.hpp"
#include "../common/MGDFResources.hpp"
#include "../common/MGDFExceptions.hpp"
#include "MGDFParameterConstants.hpp"
#include "../storage/MGDFStorageFactoryComponentImpl.hpp"
#include "../input/MGDFInputManagerComponentImpl.hpp"
#include "../audio/MGDFSoundManagerComponentImpl.hpp"
#include "../vfs/MGDFVirtualFileSystemComponentImpl.hpp"
#include "MGDFComponents.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core {

bool SystemBuilder::RegisterBaseComponents(HINSTANCE instance,HWND window)
{
	//init global common components
	InitParameterManager();
	InitResources();
	InitLogger();

	storage::IStorageFactoryComponent *storageImpl = storage::CreateStorageFactoryComponentImpl(instance,window);
	if (storageImpl!=nullptr) {
		Components::Instance().RegisterComponent<storage::IStorageFactoryComponent>(storageImpl);
	}
	else {
		GetLoggerImpl()->Add(TYPE_NAME(SystemBuilder),"FATAL ERROR: Unable to register StorageFactory",LOG_ERROR);
		return false;
	}

	return true;
}

bool SystemBuilder::RegisterAdditionalComponents(HINSTANCE instance,HWND window,std::string gameUid)
{
	input::IInputManagerComponent *input = input::CreateInputManagerComponentImpl(instance,window);
	if (input!=nullptr) {
		Components::Instance().RegisterComponent<input::IInputManagerComponent>(input);
	}
	else {
		GetLoggerImpl()->Add(TYPE_NAME(SystemBuilder),"FATAL ERROR: Unable to register InputManager",LOG_ERROR);
		return false;
	}

	vfs::IVirtualFileSystemComponent *vfs = vfs::CreateVirtualFileSystemComponentImpl(instance,window,GetLoggerImpl());
	if (vfs!=nullptr) {
		Components::Instance().RegisterComponent<vfs::IVirtualFileSystemComponent>(vfs);
	}
	else {
		GetLoggerImpl()->Add(TYPE_NAME(SystemBuilder),"FATAL ERROR: Unable to register VirtualFileSystem",LOG_ERROR);
		return false;
	}

	audio::ISoundManagerComponent *audioImpl = audio::CreateSoundManagerComponentImpl(instance,vfs,window);
	if (audioImpl!=nullptr) {
		Components::Instance().RegisterComponent<audio::ISoundManagerComponent>(audioImpl);
	}
	else 
	{
		//its a problem, but we can still probably run if the soundmanager failed to initialize
		GetLoggerImpl()->Add(TYPE_NAME(SystemBuilder),"ERROR: Unable to register SoundManager",LOG_ERROR);
	}

	return true;
}

void SystemBuilder::UnregisterComponents()
{
	Components::Instance().UnregisterComponent<storage::IStorageFactoryComponent>();
	Components::Instance().UnregisterComponent<input::IInputManagerComponent>();
	Components::Instance().UnregisterComponent<audio::ISoundManagerComponent>();
	Components::Instance().UnregisterComponent<vfs::IVirtualFileSystemComponent>();
}

System *SystemBuilder::CreateSystem(HINSTANCE instance,HWND window)
{
	//do the bare minimum setup required to be able to 
	//load up the game configuration file
	if (!RegisterBaseComponents(instance,window))
	{
		return nullptr;
	}

	Game *game=nullptr;
	try 
	{
		//try and load the game configuration file
		storage::IStorageFactoryComponent *storageFactory = Components::Instance().Get<storage::IStorageFactoryComponent>();
		std::auto_ptr<storage::IGameStorageHandler> handler(storageFactory->CreateGameStorageHandler());
		handler->Load(Resources::Instance().GameFile());

		//now that we know the UID for the game, we'll set up the user resources paths again
		//and shift the logfile over to the new user directory
		InitResources(handler->GetGameUid());
		GetLoggerImpl()->MoveOutputFile();

		game = GameBuilder::LoadGame(handler.get());
	}
	catch (MGDFException ex)
	{
		std::string error = "FATAL ERROR: Unable to load game boot configuration - ";
		error+=ex.what();
		GetLoggerImpl()->Add(TYPE_NAME(SystemBuilder),error,LOG_ERROR);
		return nullptr;
	} 
	catch (...)
	{
		GetLoggerImpl()->Add(TYPE_NAME(SystemBuilder),"FATAL ERROR: Unable to load game boot configuration",LOG_ERROR);
		return nullptr;
	}

	//now that the game file loaded, initialize everything else
	//and set the log directory correctly.
	if (!RegisterAdditionalComponents(instance,window,game->GetUid()))
	{
		return nullptr;
	}

	if (MGDFVersionInfo::MGDF_INTERFACE_VERSION!=game->GetInterfaceVersion()) {
		GetLoggerImpl()->Add(TYPE_NAME(SystemBuilder),"FATAL ERROR: Unsupported MGDF Interface version",LOG_ERROR);
		delete game;
		return nullptr;
	}

	try 
	{
		System *system = new System(game);
		Components::Instance().RegisterComponentErrorHandler(system); //register the system error handlerswith all components

		return system;
	}
	catch (...)
	{
		GetLoggerImpl()->Add(TYPE_NAME(SystemBuilder),"FATAL ERROR: Unable to create system",LOG_ERROR);
		return nullptr;
	}
}

void SystemBuilder::DisposeSystem(System *system)
{
	if (system!=nullptr)
	{
		system->DisposeModule();
	}
	UnregisterComponents();
	SAFE_DELETE_ARRAY(system);
}

void SystemBuilder::InitParameterManager()
{
	std::string paramString;
	
	//use the supplied params.txt in the application path (if provided)
	//providing a params.txt can be useful for debugging purposes.
	boost::filesystem3::path paramsTxt(Resources::Instance().RootDir()+L"params.txt",boost::filesystem::native);
	if (boost::filesystem3::exists(paramsTxt))
	{
		std::ifstream input(paramsTxt.native().c_str(),std::ios::in);
		std::stringstream buffer;
		buffer << input.rdbuf();
		paramString = buffer.str();
	}
	//otherwise parse the command line
	else
	{
		std::string cmdLine= GetCommandLine();
		auto cmdLineIter = cmdLine.begin();

		// Skip past program name (first token in command line).
		if (*cmdLineIter == '"')  // Check for and handle quoted program name
		{
			cmdLineIter++;

			// Skip over until another double-quote or a nullptr 
			while (cmdLineIter!=cmdLine.end() && *cmdLineIter != '"')
				++cmdLineIter;

			// Skip over double-quote
			if (*cmdLineIter == '"')            
				++cmdLineIter;    
		}
		else   
		{
			// First token wasn't a quote
			while (*cmdLineIter > ' ')
				++cmdLineIter;
		}
		paramString  = std::string(cmdLineIter,cmdLine.end());
	}

	//add the parameters to the parameter manager
	GetParameterManagerImpl()->AddParameterString(paramString.c_str());
}

void SystemBuilder::InitLogger()
{
	if (GetParameterManagerImpl()->HasParameter(ParameterConstants::LOG_LEVEL)) {
		const char *level = GetParameterManagerImpl()->GetParameter(ParameterConstants::LOG_LEVEL);

		if (level == ParameterConstants::VALUE_LOG_LEVEL_ERROR) {
			GetLoggerImpl()->SetLoggingLevel(LOG_ERROR);
		}
		if (level == ParameterConstants::VALUE_LOG_LEVEL_LOW) {
			GetLoggerImpl()->SetLoggingLevel(LOG_LOW);
		}
		if (level == ParameterConstants::VALUE_LOG_LEVEL_MEDIUM) {
			GetLoggerImpl()->SetLoggingLevel(LOG_MEDIUM);
		}
		if (level == ParameterConstants::VALUE_LOG_LEVEL_HIGH) {
			GetLoggerImpl()->SetLoggingLevel(LOG_HIGH);
		}
	}
}

void SystemBuilder::InitResources(std::string gameUid)
{
	if (GetParameterManagerImpl()->HasParameter(ParameterConstants::GAME_DIR_OVERRIDE)) {
		std::string gamesDirOverride(GetParameterManagerImpl()->GetParameter(ParameterConstants::GAME_DIR_OVERRIDE));
		if (gamesDirOverride[gamesDirOverride.length()-1]!='\\' && gamesDirOverride[gamesDirOverride.length()-1]!='/')
		{
			gamesDirOverride.append("\\");
		}
		Resources::Instance().SetGameBaseDir(Resources::ToWString(gamesDirOverride));
	}

	bool userDirOverride = GetParameterManagerImpl()->HasParameter(ParameterConstants::USER_DIR_OVERRIDE);

	Resources::Instance().SetUserBaseDir(userDirOverride,gameUid);
	if (!gameUid.empty())
	{
		Resources::Instance().CreateRequiredDirectories();
	}
}


}}