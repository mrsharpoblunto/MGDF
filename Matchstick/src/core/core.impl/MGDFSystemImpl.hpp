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
#include "MGDFModuleFactory.hpp"
#include "MGDFSystemStats.hpp"
#include "MGDFTimer.hpp"

namespace MGDF { namespace core {

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

	virtual IModule *GetModule()=0;

	void AddShutDownCallback(const ShutDownFunction::slot_type& callback);
	void AddFatalErrorCallback(const FatalErrorFunction::slot_type& callback);
protected:	
	//event callbacks
	boost::signal<void (void)> _shutDownFunction;
	boost::signal<void (std::string,std::string)> _fatalErrorFunction;
};

typedef ListImpl<IStringList,const char *> StringList;

/**
 reference implementation of the ISystem interface
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
	void DisposeModule();

	virtual IModule *GetModule();

	//queueable events in the System interface
	virtual void QueueSaveGameState(const char *);
	virtual void QueueLoadGameState(const char *);

	//error handling functions
	virtual void FatalError(const char *,const char *);
	virtual void SetLastError(const char *sender, int code,const char *description);

	virtual ILogger *GetLogger() const;
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
	//useful metadata that is attatched to the loaded module
	typedef struct {
		bool IsDeviceReset;
		bool IsDeviceStateSet;
		bool DeviceCapsChecked;
	} ModuleMetaData;

	//implement the events queueable in the interface
	IModule *CreateModule();
	void NewGameState(const events::NewEvent *e);
	void SaveGameState(const events::SaveEvent *e);
	void LoadGameState(const events::LoadEvent *e);

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
	Version _version;
	Error _lastError;
	boost::mutex _mutex;
	Timer _timer;

	std::list<events::ISystemEvent *> _eventQueue; //the event queue
	IModule * _module; //the currently executing module
	ModuleMetaData _moduleMetaData; //records metadata for the current module
	ModuleFactory *_moduleFactory;
};

}
}