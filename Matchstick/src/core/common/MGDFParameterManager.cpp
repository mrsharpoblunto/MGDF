#include "StdAfx.h"

#include "MGDFParameterManager.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF
{
namespace core
{

bool ParameterManager::HasParameter( const char * param ) const
{
	_ASSERTE( param );
	std::string p = param;
	return _parameters.find( p ) != _parameters.end();
}

const char *ParameterManager::GetParameter( const char * param ) const
{
	_ASSERTE( param );
	std::string p = param;
	auto iter = _parameters.find( p );
	if ( iter != _parameters.end() ) {
		return iter->second.c_str();
	}
	return nullptr;
}

MGDFError ParameterManager::AddParameterString( const char * paramString )
{
	_ASSERTE( paramString );
	std::string ps = paramString;
	return ParseParameters( ps, _parameters );
}

MGDFError ParameterManager::ParseParameters( const std::string &paramString, std::map<std::string, std::string> &paramMap )
{
	auto iter = paramString.begin();

	std::string key, value;
	for ( ;; ) {
		key.clear();
		value.clear();

		// Skip past any white space preceding the next token
		while ( iter != paramString.end() && *iter <= ' ' )
			++iter;
		if ( iter == paramString.end() )
			break;

		// Skip past the flag marker
		if ( *iter == '-' )
			++iter;

		//get the flag string
		while ( iter != paramString.end() && ( *iter > ' ' ) && ( *iter != ':' ) ) {
			key += * ( iter++ );
		}

		//check that the key is valid (i.e non null)
		if ( key.length() == 0 ) {
			return MGDF_ERR_INVALID_PARAMETER;
		}

		//parse the value (if present)
		if ( iter != paramString.end() && *iter == ':' ) {
			++iter;
			while ( iter != paramString.end() && ( *iter != '-' ) ) {
				value += * ( iter++ );
			}
			if ( iter != paramString.end() )
				--iter;

			//check that the key is valid (i.e non null)
			if ( key.length() == 0 ) {
				return MGDF_ERR_INVALID_PARAMETER;		
			}

			//erase trailing whitespace
			std::string::size_type pos = value.find_last_not_of( ' ' );
			if ( pos != std::string::npos ) {
				value.erase( pos + 1 );
			}
			//erase leading whitespace
			pos = value.find_first_not_of( ' ' );
			if ( pos != std::string::npos ) {
				value.erase( 0, pos );
			}
		}

		paramMap[key] = value;
	}
	return MGDF_OK;
}

}
}
