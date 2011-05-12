#pragma once

namespace MGDF {

	struct Error
	{
		unsigned int Code;
		char *Sender;
		char *Description;
	};

		//error codes
	#define MGDF_ERR_ERROR_ALLOCATING_BUFFER 1001
	#define MGDF_ERR_NO_FREE_SOURCES 1002
	#define MGDF_ERR_VORBIS_LIB_LOAD_FAILED 1003
	#define MGDF_ERR_INVALID_FORMAT 1004
	#define MGDF_ERR_INVALID_ARCHIVE 1005
	#define MGDF_ERR_INVALID_ARCHIVE_FILE 1006
	#define MGDF_ERR_INVALID_FILE 1007
}