#pragma once

#include <list>
#include <hash_map> //stdext::hash_map
#include <boost/signals.hpp>
#include <boost/thread/thread.hpp>

#include <MGDF/MGDF.hpp>
#include <MGDF/MGDFSystem.hpp>
#include <MGDF/MGDFModule.hpp>

#include "../common/MGDFListImpl.hpp"
#include "../common/MGDFSystemComponent.hpp"
#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFParameterManagerImpl.hpp"
#include "../xml/MGDFXMLFactoryComponentImpl.hpp"
#include "../input/MGDFInputManagerComponentImpl.hpp"
#include "../audio/MGDFSoundManagerComponentImpl.hpp"
#include "../vfs/MGDFVirtualFileSystemComponentImpl.hpp"

#include "MGDFGameImpl.hpp"
#include "MGDFStatisticsManagerImpl.hpp"
#include "MGDFGraphicsManagerImpl.hpp"
#include "MGDFSystemEvents.hpp"
#include "MGDFGlobalCallbacks.hpp"
#include "MGDFSystemStats.hpp"
#include "MGDFTimer.hpp"

namespace MGDF { namespace core {

typedef IModule * (*GetModulePtr)(const char *,ISystem *);
typedef bool (*IsCompatibleInterfaceVersionPtr)(int);

/**
this class represents a slightly more functional version of the system interface with a number of internal
implementation additions, this class should be inherited to create mock system objects
*/
class ISystemImpl: public ISystem, public IErrorHandler
{
public:
	virtual ~ISystemImpl(void){};

	//callbacks
	typedef boost::signal<void (void)> ShutDownFunction;//shutDown callback function signature
	typedef boost::signal<void (std::string,std::string)> FatalErrorFunction;//fatal error callback function signature
	typedef boost::signal<void (IModule *)> ModuleStatusChangeFunction;//module change function signature

	virtual IModule *GetTopModule()=0;

	void AddShutDownCallback(const ShutDownFunction::slot_type& callback);
	void AddFatalErrorCallback(const FatalErrorFunction::slot_type& callback);
	void AddModuleSuspendedCallback(const ModuleStatusChangeFunction::slot_type& callback);
	void AddModuleResumedCallback(const ModuleStatusChangeFunction::slot_type& callback);
	void AddModulePoppedCallback(const ModuleStatusChangeFunction::slot_type& callback);
protected:	
	//event callbacks
	boost::signal<void (void)> _shutDownFunction;
	boost::signal<void (std::string,std::string)> _fatalErrorFunction;
	boost::signal<void (IModule *)> _moduleSuspendedFunction;
	boost::signal<void (IModule *)> _moduleResumedFunction;
	boost::signal<void (IModule *)> _modulePoppedFunction;
};

typedef ListImpl<IStringList,const char *> StringList;

/**
 reference implementation of the ISystem interface

 parameters recognised and used by the System

 -validateXML:true  will validate all xml bootconfig and moduleconfig files upon parsing
                    using an xsd schema
\author gcconner
*/
class System:public ISystemImpl
{
public:
	System(Game *game);
	virtual ~System(void);

	void UpdateScene(double simulationTime,SystemStats *stats,boost::mutex &statsMutex);
	void DrawScene(double alpha);
	bool HasQueuedEvents();
	void ProcessQueuedEvents();
	void DeviceLost();
	void DeviceReset();
	void SetD3DDevice(IDirect3DDevice9 *d3dDevice);
	void CreateGraphicsImpl(IDirect3D9 *d3d9);
	GraphicsManager *GetGraphicsImpl();
	std::string GetSystemInformation(SystemStats *stats);
	void DisposeModules();

	virtual IModule *GetTopModule();

	//queueable events in the System interface
	virtual void QueuePushNewModule(const char *,IModuleInitialiser *init=NULL);
	virtual void QueuePopModules(unsigned int );
	virtual void QueueSwapTopModule(const char *,IModuleInitialiser *init=NULL);
	virtual void QueueSaveGameState(const char *);
	virtual void QueueLoadGameState(const char *);

	//functions that can be called from within a module without queueing
	virtual IModuleInitialiser *CreateModuleInitialiser() const;

	//error handling functions
	virtual void FatalError(const char *,const char *);
	virtual void SetLastError(const char *sender, int code,const char *description);

	virtual ILogger *GetLogger() const;
	virtual IParameterManager *GetParameters() const;
	virtual IVirtualFileSystem *GetVFS() const;
	virtual ISoundManager *GetSound() const;
	virtual IGraphicsManager *GetGraphics() const;
	virtual IStatisticsManager *GetStatistics() const;
	virtual IGame *GetGame() const;
	virtual IInputManager *GetInput() const;
	virtual IDirect3DDevice9 *GetD3DDevice() const;
	virtual void ShutDown();
	virtual const IStringList *GetSaves() const;
	virtual void RemoveSave(const char *saveName);
	virtual const Version * GetMGDFVersion() const;
	virtual const Error *GetLastError() const;

	void FatalError(std::string sender,std::string message)
	{
		FatalError(sender.c_str(),message.c_str());
	}
private:

	//represents a loaded module file, ths instances are counted to prevent 
	//unecessary instantsiations and also ensures that when a module file has no
	//module instances loaded, the module file is unloaded
	typedef struct {
		HINSTANCE Instance;
		int InstCount;
	} ModuleFile;

	//useful metadata that is attatched to each loaded module
	typedef struct {
		std::string ModuleFile;
		std::string Name;
		bool IsDeviceReset;
		bool IsDeviceStateSet;
		bool DeviceCapsChecked;
	} ModuleMetaData;

	//implement the events queueable in the interface
	IModule *PushModule(std::string);
	void PushLoadModule(const events::PushLoadEvent *e);
	void PushNewModule(const events::PushNewEvent *e);
	void PopModules(const events::PopEvent *e);
	void SwapTopModule(const events::SwapEvent *e);
	void SaveGameState(const events::SaveEvent *e);
	void LoadGameState(const events::LoadEvent *e);

	void QueueBoot();
	void QueuePushLoadModule(std::string moduleName, MGDF::IModuleInitialiser *moduleInitialiser,std::string loadDataDir);

	void SuspendModule(IModule *);
	void ResumeModule(IModule *);

	ModuleFile GetModuleFile(std::string moduleName);
	void ClearWorkingDirectory();
	void MigrateGameState(xml::IGameStateXMLHandler *handler,std::string gameStateFile,std::string saveDataDir);

	xml::IXMLFactoryComponent *_xml;
	input::IInputManagerComponent *_input;
	audio::ISoundManagerComponent *_sound;
	vfs::IVirtualFileSystemComponent *_vfs;
	Game *_game;
	StringList *_saves;
	GraphicsManager *_graphics;
	StatisticsManager *_stats;
	IDirect3DDevice9 *_d3dDevice;
	GlobalCallbacks *_globalCallbacks;
	Version _version;
	Error _lastError;
	boost::mutex _mutex;
	Timer _timer;

	std::list<events::ISystemEvent *> _eventQueue; //the event queue
	std::list<IModule *> _moduleStack; //all loaded modules (can have multiple instances of a single moduleclass loaded)
	stdext::hash_map<IModule *,ModuleMetaData> _moduleMetaData; //records metadata for each module
	stdext::hash_map<std::string,ModuleFile> _loadedModuleFiles; //all loaded module classes
};

}
}