#include "StdAfx.h"

#include <MGDF/MGDF.hpp>

#include "d3d9.h"
#include "MGDFSystemBuilder.hpp"
#include "MGDFGameBuilder.hpp"
#include "MGDFSystemImpl.hpp"
#include "../common/MGDFVersionInfo.hpp"
#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFParameterManagerImpl.hpp"
#include "../common/MGDFResources.hpp"
#include "../common/MGDFExceptions.hpp"
#include "MGDFParameterConstants.hpp"
#include "../xml/MGDFXMLFactoryComponentImpl.hpp"
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

bool SystemBuilder::RegisterComponents(HINSTANCE instance,HWND window)
{
	//init global common components
	InitParameterManager();
	InitResources();
	InitLogger();


	xml::IXMLFactoryComponent *xmlImpl = xml::CreateXMLFactoryComponentImpl(instance,window);
	if (xmlImpl!=NULL) {
		Components::Instance().RegisterComponent<xml::IXMLFactoryComponent>(xmlImpl);
	}
	else {
		GetLoggerImpl()->Add(TYPE_NAME(SystemBuilder),"FATAL ERROR: Unable to register XMLFactory",LOG_ERROR);
		return false;
	}

	input::IInputManagerComponent *input = input::CreateInputManagerComponentImpl(instance,window);
	if (input!=NULL) {
		Components::Instance().RegisterComponent<input::IInputManagerComponent>(input);
	}
	else {
		GetLoggerImpl()->Add(TYPE_NAME(SystemBuilder),"FATAL ERROR: Unable to register InputManager",LOG_ERROR);
		return false;
	}

	vfs::IVirtualFileSystemComponent *vfs = vfs::CreateVirtualFileSystemComponentImpl(instance,window,GetLoggerImpl());
	if (vfs!=NULL) {
		Components::Instance().RegisterComponent<vfs::IVirtualFileSystemComponent>(vfs);
	}
	else {
		GetLoggerImpl()->Add(TYPE_NAME(SystemBuilder),"FATAL ERROR: Unable to register VirtualFileSystem",LOG_ERROR);
		return false;
	}


	audio::ISoundManagerComponent *audioImpl = audio::CreateSoundManagerComponentImpl(instance,vfs,window);
	if (audioImpl!=NULL) {
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
	Components::Instance().UnregisterComponent<xml::IXMLFactoryComponent>();
	Components::Instance().UnregisterComponent<input::IInputManagerComponent>();
	Components::Instance().UnregisterComponent<audio::ISoundManagerComponent>();
	Components::Instance().UnregisterComponent<vfs::IVirtualFileSystemComponent>();
}

System *SystemBuilder::CreateSystem(HINSTANCE instance,HWND window)
{
	if (!RegisterComponents(instance,window))
	{
		return NULL;
	}

	if (GetParameterManagerImpl()->HasParameter(ParameterConstants::BOOT_GAME)) {
		Game *game=NULL;

		try 
		{
			game = GameBuilder::LoadGame();
		}
		catch (MGDFException ex)
		{
			std::string error = "FATAL ERROR: Unable to load game boot configuration - ";
			error+=ex.what();
			GetLoggerImpl()->Add(TYPE_NAME(SystemBuilder),error,LOG_ERROR);
			return NULL;
		} 
		catch (...)
		{
			GetLoggerImpl()->Add(TYPE_NAME(SystemBuilder),"FATAL ERROR: Unable to load game boot configuration",LOG_ERROR);
			return NULL;
		}

		if (MGDFVersionInfo::MGDF_INTERFACE_VERSION!=game->GetInterfaceVersion()) {
			GetLoggerImpl()->Add(TYPE_NAME(SystemBuilder),"FATAL ERROR: Unsupported MGDF Interface version",LOG_ERROR);
			delete game;
			return NULL;
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
			return NULL;
		}
	}
	else {
		return NULL;
	}
}

void SystemBuilder::DisposeSystem(System *system)
{
	if (system!=NULL)
	{
		system->DisposeModule();
	}
	UnregisterComponents();
	SAFE_DELETE_ARRAY(system);
}

void SystemBuilder::InitParameterManager()
{
	std::string cmdLine= GetCommandLine();
	std::string::iterator cmdLineIter = cmdLine.begin();

    // Skip past program name (first token in command line).
    if (*cmdLineIter == '"')  // Check for and handle quoted program name
    {
        cmdLineIter++;

        // Skip over until another double-quote or a null 
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
	std::string paramString(cmdLineIter,cmdLine.end());

	//add the parameters to the parameter manager
	GetParameterManagerImpl()->AddParameterString(paramString.c_str());
}

void SystemBuilder::InitLogger()
{
	if (GetParameterManagerImpl()->HasParameter(ParameterConstants::BOOT_GAME))
	{
		//todo move log file
	}

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

void SystemBuilder::InitResources()
{
	if (GetParameterManagerImpl()->HasParameter(ParameterConstants::GAMES_DIR_OVERRIDE)) {
		std::string gamesDirOverride(GetParameterManagerImpl()->GetParameter(ParameterConstants::GAMES_DIR_OVERRIDE));
		if (gamesDirOverride[gamesDirOverride.length()-1]!='\\' && gamesDirOverride[gamesDirOverride.length()-1]!='/')
		{
			gamesDirOverride.append("\\");
		}
		Resources::Instance().SetGameBaseDir(Resources::ToWString(gamesDirOverride));
	}

	bool userDirOverride = GetParameterManagerImpl()->HasParameter(ParameterConstants::USER_DIR_OVERRIDE);
	std::string gameUid = GetParameterManagerImpl()->HasParameter(ParameterConstants::BOOT_GAME) ? GetParameterManagerImpl()->GetParameter(ParameterConstants::BOOT_GAME) : "";

	Resources::Instance().SetUserBaseDir(userDirOverride,gameUid);
	Resources::Instance().CreateRequiredDirectories();
}


}}