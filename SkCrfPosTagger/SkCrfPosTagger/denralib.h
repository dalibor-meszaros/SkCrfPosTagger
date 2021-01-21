// author:	Dalibor Mészáros
// version:	0.02 customized for Bachelor's thesis
// updated:	15/04/2015
//

#ifndef DENRALIB_H
#define DENRALIB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <locale.h>
#include <time.h>
#include <algorithm>
#include <codecvt>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  OS SPECIFIC

#ifdef _WIN32

#define POPEN _popen
#define WPOPEN _wpopen
#define PCLOSE _pclose
#define FSEEK64 _fseeki64
#define FTELL64 _ftelli64

#else

#define POPEN popen
#define WPOPEN wpopen
#define PCLOSE pclose
#define FSEEK64 fseeko64
#define FTELL64 ftello64

#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  DEFINE MACROS

// data types

#define BOOL unsigned char
#define LONG long long int
#define ULONG unsigned long long int

// constants

#define TRUE 1
#define FALSE 0
#define KB 1024
#define MB 1048576
#define GB 1073741824
#define BOM 65279
#define FOPEN_MODE_WRITE_UTF8 "wtS, ccs=UTF-8"
#define FOPEN_MODE_WRITE_UTF8_W L"wtS, ccs=UTF-8"
#define FOPEN_MODE_READ_UTF8 "rtS, ccs=UTF-8"
#define FOPEN_MODE_READ_UTF8_W L"rtS, ccs=UTF-8"

// converts & formulas

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define CONVERT_KB(x) ((double)(x) / KB)
#define CONVERT_MB(x) ((double)(x) / MB)
#define CONVERT_GB(x) ((double)(x) / GB)
#define CONVERT_SEC(x) ((double)(x) / CLOCKS_PER_SEC)
#define CONVERT_PCT(frac, total) (((double)(frac) / (total)) * 100)
#define CLOCK_START(x) (x) = clock()
#define CLOCK_ELAPSED(x) (clock() - (x))
#define TIME_START(x) (x) = time(NULL)
#define TIME_ELAPSED(x) (time(NULL) - (x))
#define SET_LOCALE(str) setlocale(LC_ALL, str)

// exit error values

#define EXIT_SUCCESS			0
#define EXIT_FAILURE			1
#define EXIT_ERROR_FOPEN		2
#define EXIT_ERROR_POPEN		3
#define EXIT_ERROR_MALLOC		4
#define EXIT_ERROR_INPUT		5
#define EXIT_ERROR_EMPTY		6
#define EXIT_ERROR_READ			7
#define EXIT_ERROR_HANDLE		8

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  GLOBAL VARIABLES
std::wstring _denra_lib_gv_diacritics = L"áéíĺóŕúýčďľěňřšťžôäöüőűÁÉÍĹÓŔÚÝČĎĽĚŇŘŠŤŽÔÄӦÜŐŰ";
std::wstring _denra_lib_gv_diacritics_removed = L"aeiloruycdlenrstzoaououAEILORUYCDLENRSTZOAOUOU";

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  INLINE MAX/MIN

template <typename anyType>
inline anyType Max(anyType arg_x, anyType arg_y) {
	return arg_x > arg_y ? arg_x : arg_y;
}

template <typename anyType>
inline anyType Min(anyType arg_x, anyType arg_y) {
	return arg_x < arg_y ? arg_x : arg_y;
}

template <typename anyType>
inline anyType Swap(anyType &arg_x, anyType &arg_y) {
	anyType backup = arg_x;
	arg_x = arg_y;
	arg_y = backup;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  STRING ALTER

std::string& StringToLowerAlter(std::string& arg_string, size_t arg_start = 0, size_t arg_end = -1) {
	size_t i;
	if (arg_end > arg_string.length())
		arg_end = arg_string.length();
	for (i = arg_start; i < arg_end; ++i) {
		arg_string.at(i) = tolower(arg_string.at(i));
	}
	return arg_string;
}

std::wstring& StringToLowerAlter(std::wstring& arg_string, size_t arg_start = 0, size_t arg_end = -1) {
	size_t i;
	if (arg_end > arg_string.length())
		arg_end = arg_string.length();
	for (i = arg_start; i < arg_end; ++i) {
		arg_string.at(i) = towlower(arg_string.at(i));
	}
	return arg_string;
}

std::string& StringReplaceAllAlter(std::string& arg_string, const std::string& arg_from, const std::string& arg_to, size_t arg_start = 0, size_t arg_end = -1) {
	if (arg_end > arg_string.length())
		arg_end = arg_string.length();
	while ((arg_start = arg_string.find(arg_from, arg_start)) != std::string::npos && arg_start <= arg_end) {
		arg_string.replace(arg_start, arg_from.length(), arg_to);
		arg_start += arg_to.length();
		arg_end += arg_to.length() - arg_from.length();
	}
	return arg_string;
}

std::wstring& StringReplaceAllAlter(std::wstring& arg_string, const std::wstring& arg_from, const std::wstring& arg_to, size_t arg_start = 0, size_t arg_end = -1) {
	if (arg_end > arg_string.length())
		arg_end = arg_string.length();
	while ((arg_start = arg_string.find(arg_from, arg_start)) != std::wstring::npos && arg_start <= arg_end) {
		arg_string.replace(arg_start, arg_from.length(), arg_to);
		arg_start += arg_to.length();
		arg_end += arg_to.length() - arg_from.length();
	}
	return arg_string;
}

std::wstring& StringRemoveDiacriticsAlter(std::wstring& arg_string, size_t arg_start = 0, size_t arg_end = -1) {
	size_t i;
	if (arg_end > arg_string.length())
		arg_end = arg_string.length();
	for (i = 0; i < _denra_lib_gv_diacritics.length(); ++i)
		StringReplaceAllAlter(arg_string, _denra_lib_gv_diacritics.substr(i, 1), _denra_lib_gv_diacritics_removed.substr(i, 1), arg_start, arg_end);
	return arg_string;
}

std::string& StringRemoveBomAlter(std::string& arg_string) {
	if (arg_string.at(0) == BOM)
		arg_string.erase(0, 1);
	return arg_string;
}

std::wstring& StringRemoveBomAlter(std::wstring& arg_string) {
	if (arg_string.at(0) == BOM)
		arg_string.erase(0, 1);
	return arg_string;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  STRING COPY+MODIFY/CREATE

std::string StringToLower(std::string& arg_string, size_t arg_start = 0, size_t arg_end = -1) {
	size_t i;
	std::string retrn = arg_string;
	if (arg_end > retrn.length())
		arg_end = retrn.length();
	for (i = arg_start; i < arg_end; ++i) {
		(retrn).at(i) = tolower((retrn).at(i));
	}
	return retrn;
}

std::wstring StringToLower(std::wstring& arg_string, size_t arg_start = 0, size_t arg_end = -1) {
	size_t i;
	std::wstring retrn = arg_string;
	if (arg_end > retrn.length())
		arg_end = retrn.length();
	for (i = arg_start; i < arg_end; ++i) {
		(retrn).at(i) = towlower((retrn).at(i));
	}
	return retrn;
}

std::string StringReplaceAll(std::string& arg_string, const std::string& arg_from, const std::string& arg_to, size_t arg_start = 0, size_t arg_end = -1) {
	std::string retrn = arg_string;
	if (arg_end > retrn.length())
		arg_end = retrn.length();
	while ((arg_start = retrn.find(arg_from, arg_start)) != std::string::npos && arg_start <= arg_end) {
		retrn.replace(arg_start, arg_from.length(), arg_to);
		arg_start += arg_to.length();
		arg_end += arg_to.length() - arg_from.length();
	}
	return retrn;
}

std::wstring StringReplaceAll(std::wstring& arg_string, const std::wstring& arg_from, const std::wstring& arg_to, size_t arg_start = 0, size_t arg_end = -1) {
	std::wstring retrn = arg_string;
	if (arg_end > retrn.length())
		arg_end = retrn.length();
	while ((arg_start = retrn.find(arg_from, arg_start)) != std::wstring::npos && arg_start <= arg_end) {
		retrn.replace(arg_start, arg_from.length(), arg_to);
		arg_start += arg_to.length();
		arg_end += arg_to.length() - arg_from.length();
	}
	return retrn;
}

std::wstring StringRemoveDiacritics(std::wstring& arg_string, size_t arg_start = 0, size_t arg_end = -1) {
	size_t i;
	std::wstring retrn = arg_string;
	if (arg_end > retrn.length())
		arg_end = retrn.length();
	for (i = 0; i < _denra_lib_gv_diacritics.length(); ++i)
		StringReplaceAllAlter(retrn, _denra_lib_gv_diacritics.substr(i, 1), _denra_lib_gv_diacritics_removed.substr(i, 1), arg_start, arg_end);
	return retrn;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  DIRECTORY

#ifndef DISABLE_SYSTEM
inline void DirectoryCreateSys(const char* arg_dirname) {
#ifdef _WIN32
	std::string command_a = std::string() + "if not exist \"" + arg_dirname + "\" mkdir \"" + arg_dirname + "\"";
#else
	std::string command_a = std::string() + "mkdir -p \"" + arg_dirname + "\"";
#endif
	system(command_a.c_str());
}

inline void DirectoryCreateSys(const wchar_t* arg_dirname) {
#ifdef _WIN32
	std::wstring command_w = std::wstring() + L"if not exist \"" + arg_dirname + L"\" mkdir \"" + arg_dirname + L"\"";
#else
	std::wstring command_w = std::wstring() + L"mkdir -p \"" + arg_dirname + L"\"";
#endif
	std::string command_a(command_w.begin(), command_w.end());
	system(command_a.c_str());
}

inline void DirectoryDeleteSys(const char* arg_dirname) {
#ifdef _WIN32
	std::string command_a = std::string() + "rmdir /s/q " + arg_dirname + "\"";
#else
	std::string command_a = std::string() + "rm -rf \"" + arg_dirname + "\"";
#endif
	system(command_a.c_str());
}

inline void DirectoryDeleteSys(const wchar_t* arg_dirname) {
#ifdef _WIN32
	std::wstring command_w = std::wstring() + L"rmdir /s/q \"" + arg_dirname + L"\"";
#else
	std::wstring command_w = std::wstring() + L"rm -rf \"" + arg_dirname + L"\"";
#endif
	std::string command_a(command_w.begin(), command_w.end());
	system(command_a.c_str());
}
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  FILE

size_t FileLoad(const char* arg_filename, const char* arg_mode, std::string& arg_data, FILE*& arg_file) {
	clock_t clock_start = clock();
	size_t loaded_size;
	if ((arg_file = fopen(arg_filename, arg_mode)) == NULL) {
		fprintf(stderr, "Error: Unable to acquire handle for file %s\n", arg_filename);
		DirectoryDeleteSys("~temp");
		exit(EXIT_ERROR_FOPEN);
	}
	FSEEK64(arg_file, 0, SEEK_END);
	(arg_data).resize(FTELL64(arg_file));
	rewind(arg_file);
	loaded_size = fread(&(arg_data)[0], sizeof(char), arg_data.size(), arg_file);
	if (loaded_size == 0) {
		fprintf(stderr, "Error: Unable to load %.2f MB\n", CONVERT_MB(arg_data.size()));
		DirectoryDeleteSys("~temp");
		exit(EXIT_ERROR_READ);
	}
	arg_data.shrink_to_fit();
	return loaded_size;
}

size_t FileLoad(const char* arg_filename, const char* arg_mode, std::string& arg_data) {
	clock_t clock_start = clock();
	FILE *file;
	size_t loaded_size;
	if ((file = fopen(arg_filename, arg_mode)) == NULL) {
		fprintf(stderr, "Error: Unable to acquire handle for file %s\n", arg_filename);
		DirectoryDeleteSys("~temp");
		exit(EXIT_ERROR_FOPEN);
	}
	FSEEK64(file, 0, SEEK_END);
	(arg_data).resize(FTELL64(file));
	rewind(file);
	loaded_size = fread(&(arg_data)[0], sizeof(char), arg_data.size(), file);
	if (loaded_size == 0) {
		fprintf(stderr, "Error: Unable to load %.2f MB\n", CONVERT_MB(arg_data.size()));
		DirectoryDeleteSys("~temp");
		exit(EXIT_ERROR_READ);
	}
	fclose(file);
	arg_data.shrink_to_fit();
	return loaded_size;
}

size_t FileLoad(const wchar_t* arg_filename, const wchar_t* arg_mode, std::wstring& arg_data, FILE*& arg_file) {
	clock_t clock_start = clock();
	size_t loaded_size;
	if ((arg_file = _wfopen(arg_filename, arg_mode)) == NULL) {
		fwprintf(stderr, L"Error: Unable to acquire handle for file %s\n", arg_filename);
		DirectoryDeleteSys("~temp");
		exit(EXIT_ERROR_FOPEN);
	}
	FSEEK64(arg_file, 0, SEEK_END);
	(arg_data).resize(FTELL64(arg_file));
	rewind(arg_file);
	loaded_size = fread(&(arg_data)[0], sizeof(wchar_t), arg_data.size(), arg_file);
	if (loaded_size == 0) {
		fprintf(stderr, "Error: Unable to load %.2f MB\n", CONVERT_MB(arg_data.size()));
		DirectoryDeleteSys("~temp");
		exit(EXIT_ERROR_READ);
	}
	arg_data.shrink_to_fit();
	return loaded_size;
}

size_t FileLoad(const wchar_t* arg_filename, const wchar_t* arg_mode, std::wstring& arg_data) {
	clock_t clock_start = clock();
	FILE *file;
	size_t loaded_size;
	if ((file = _wfopen(arg_filename, arg_mode)) == NULL) {
		fwprintf(stderr, L"Error: Unable to acquire handle for file %s\n", arg_filename);
		DirectoryDeleteSys("~temp");
		exit(EXIT_ERROR_FOPEN);
	}
	FSEEK64(file, 0, SEEK_END);
	(arg_data).resize(FTELL64(file));
	rewind(file);
	loaded_size = fread(&(arg_data)[0], sizeof(wchar_t), arg_data.size(), file);
	if (loaded_size == 0) {
		fprintf(stderr, "Error: Unable to load %.2f MB\n", CONVERT_MB(arg_data.size()));
		DirectoryDeleteSys("~temp");
		exit(EXIT_ERROR_READ);
	}
	fclose(file);
	arg_data.shrink_to_fit();
	return loaded_size;
}

BOOL FileTest(const char* arg_filename) {
	FILE *file;
	if ((file = fopen(arg_filename, "r")) == NULL) {
		return FALSE;
	}
	fclose(file);
	return TRUE;
}

BOOL FileTest(const wchar_t* arg_filename) {
	FILE *file;
	if ((file = _wfopen(arg_filename, L"r")) == NULL) {
		return FALSE;
	}
	fclose(file);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  EXECUTE

std::string Execute(const char * arg_cmd) {
	char buffer[1024];
	std::string ret = "";
	FILE * pipe = POPEN(arg_cmd, "r");
	if (!pipe) {
		fprintf(stderr, "Error: Unable to execute %s\n", arg_cmd);
		DirectoryDeleteSys("~temp");
		exit(EXIT_ERROR_POPEN);
	}
	while (!feof(pipe)) {
		if (fgets(buffer, 1024, pipe) != NULL)
			ret += buffer;
	}
	PCLOSE(pipe);
	return ret;
}

std::wstring Execute(const wchar_t * arg_cmd) {
	wchar_t buffer[1024];
	std::wstring ret = L"";
	FILE * pipe = WPOPEN(arg_cmd, L"r");
	if (!pipe) {
		fwprintf(stderr, L"Error: Unable to execute %s\n", arg_cmd);
		DirectoryDeleteSys("~temp");
		exit(EXIT_ERROR_POPEN);
	}
	while (!feof(pipe)) {
		if (fgetws(buffer, 1024, pipe) != NULL)
			ret += buffer;
	}
	PCLOSE(pipe);
	return ret;
}

#endif