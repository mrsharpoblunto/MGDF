#pragma once

#include <string>
#include <map>
#include <mutex>

#include <MGDF/MGDFVirtualFileSystem.hpp>

struct WCharCmp {
	bool operator()( const wchar_t *a, const wchar_t *b ) {
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

	MGDF::IFile *GetParent() const override;
	MGDF::IFile *GetChild( const wchar_t *name ) const override;
	bool GetAllChildren( const MGDF::IFileFilter *filter, IFile **childBuffer, size_t *bufferLength ) const override;
	size_t GetChildCount() const override;
	const wchar_t* GetLogicalPath() const override;

	MGDF::MGDFError OpenFile( IFileReader **reader ) override;

	bool IsOpen() const override;
	void Close() override;
	UINT32 Read( void* buffer, UINT32 length ) override;
	void SetPosition( INT64 pos ) override;
	INT64 GetPosition() const override;
	bool EndOfFile() const override;
	INT64 GetSize() const override;

	bool IsFolder() const override;
	bool IsArchive() const override;
	const wchar_t *GetArchiveName() const override;
	const wchar_t *GetPhysicalPath() const override;
	const wchar_t *GetName() const override;
	time_t GetLastWriteTime() const override;
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