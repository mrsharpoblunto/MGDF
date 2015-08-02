#pragma once

#include <string>
#include <map>
#include <mutex>

#include <MGDF/MGDFVirtualFileSystem.hpp>

struct WCharCmp {
	bool operator()( const wchar_t *a, const wchar_t *b ) const {
		return std::wcscmp( a, b ) < 0;
	}
};

/**
 abstract class which contains the common functionality to default file instances aswell as the zip and other archive file implementations
 of the standard ifile interface
*/
class FakeFile : public MGDF::IFile, public MGDF::IFileReader
{
public:
	FakeFile( const std::wstring &name, const std::wstring &physicalFile, IFile *parent );
	FakeFile( const std::wstring &name, FakeFile *parent, void *data, size_t dataLength );  //NULL data indicates a folder
	void AddChild( FakeFile *file );
	virtual ~FakeFile( void );

	MGDF::IFile *GetParent() const override final;
	MGDF::IFile *GetChild( const wchar_t *name ) const override final;
	bool GetAllChildren( const MGDF::IFileFilter *filter, IFile **childBuffer, size_t *bufferLength ) const override final;
	size_t GetChildCount() const override final;
	const wchar_t* GetLogicalPath() const override final;

	MGDF::MGDFError Open( IFileReader **reader ) override final;

	bool IsOpen() const override final;
	void Close() override final;
	UINT32 Read( void* buffer, UINT32 length ) override final;
	void SetPosition( INT64 pos ) override final;
	INT64 GetPosition() const override final;
	bool EndOfFile() const override final;
	INT64 GetSize() const override final;

	bool IsFolder() const override final;
	bool IsArchive() const override final;
	const wchar_t *GetArchiveName() const override final;
	const wchar_t *GetPhysicalPath() const override final;
	const wchar_t *GetName() const override final;
	time_t GetLastWriteTime() const override final;
protected:
	mutable std::mutex _mutex;
	mutable std::wstring _logicalPath;

	std::map<const wchar_t *, FakeFile *,WCharCmp> *_children;
	MGDF::IFile *_parent;
	std::wstring _name;
	std::wstring _physicalPath;

	size_t _dataLength;
	void *_data;
	INT32 _position;
	bool _isOpen;
};