/*
  Copyright 1999-2005 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.

  You may not use this file except in compliance with the License.
  obtain a copy of the License at

    http://www.imagemagick.org/script/license.php

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  Windows NT utility methods for ImageMagick.
*/
#ifndef _MAGICK_NTBASE_H
#define _MAGICK_NTBASE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include "magick/delegate.h"

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
#include <winuser.h>
#include <wingdi.h>
#include <io.h>
#include <process.h>
#include <errno.h>
#if defined(_DEBUG)
#include <crtdbg.h>
#endif

#define PROT_READ  0x01
#define PROT_WRITE  0x02
#define MAP_SHARED  0x01
#define MAP_PRIVATE  0x02
#define MAP_ANONYMOUS  0x20
#define F_OK 0
#define R_OK 4
#define W_OK 2
#define RW_OK 6
#define _SC_PAGESIZE 1
#define _SC_PHYS_PAGES 2

#if !defined(chsize)
# if defined(__BORLANDC__)
#   define chsize(file,length)  chsize(file,length)
# else
#   define chsize(file,length)  _chsize(file,length)
# endif
#endif

#if !defined(access)
#  define access(path,mode)  _access(path,mode)
#endif
#if !defined(closedir)
#  define closedir(directory)  NTCloseDirectory(directory)
#endif
#if !defined(access)
#  define ftruncate(file,length)  NTFileTruncate(file,length)
#endif
#if !defined(lt_dlclose)
#  define lt_dlclose(handle)  NTCloseLibrary(handle)
#endif
#if !defined(lt_dlerror)
#  define lt_dlerror()  NTGetLibraryError()
#endif
#if !defined(lt_dlexit)
#  define lt_dlexit()  NTExitLibrary()
#endif
#if !defined(lt_dlinit)
#  define lt_dlinit()  NTInitializeLibrary()
#endif
#if !defined(lt_dlopen)
#  define lt_dlopen(filename)  NTOpenLibrary(filename)
#endif
#if !defined(lt_dlsetsearchpath)
#  define lt_dlsetsearchpath(path)  NTSetSearchPath(path)
#endif
#if !defined(lt_dlsym)
#  define lt_dlsym(handle,name)  NTGetLibrarySymbol(handle,name)
#endif
#if !defined(mmap)
#  define mmap(address,length,protection,access,file,offset) \
  NTMapMemory(address,length,protection,access,file,offset)
#endif
#if !defined(msync)
#  define msync(address,length,flags)  NTSyncMemory(address,length,flags)
#endif
#if !defined(munmap)
#  define munmap(address,length)  NTUnmapMemory(address,length)
#endif
#if !defined(opendir)
#  define opendir(directory)  NTOpenDirectory(directory)
#endif
#if !defined(pclose)
#  define pclose  _pclose
#endif
#if !defined(popen)
#  define popen  _popen
#endif
#if !defined(readdir)
#  define readdir(directory)  NTReadDirectory(directory)
#endif
#if !defined(seekdir)
#  define seekdir(directory,offset)  NTSeekDirectory(directory,offset)
#endif
#if !defined(strcasecmp)
#  define strcasecmp  strcmpi
#endif
#if !defined(strncasecmp)
#  define strncasecmp  strnicmp
#endif
#if !defined(telldir)
#  define telldir(directory)  NTTellDirectory(directory)
#endif
#if !defined(tempnam)
#  define tempnam  _tempnam
#endif
#if !defined(vsnprintf)
#  define vsnprintf  _vsnprintf
#endif

#if defined(_MT) && defined(__WINDOWS__)
#  define SAFE_GLOBAL  __declspec(thread)
#else
#  define SAFE_GLOBAL
#endif

#if defined(__BORLANDC__)
#undef _O_RANDOM
#define _O_RANDOM 0
#undef _O_SEQUENTIAL
#define _O_SEQUENTIAL 0
#undef _O_SHORT_LIVED
#define _O_SHORT_LIVED 0
#undef _O_TEMPORARY
#define _O_TEMPORARY 0
#endif

#if !defined(XS_VERSION)
struct dirent
{
  char
    d_name[2048];

  int
    d_namlen;
};

typedef struct _DIR
{
  HANDLE
    hSearch;

  WIN32_FIND_DATA
    Win32FindData;

  BOOL
    firsttime;

  struct dirent
    file_info;
} DIR;

typedef struct _NTMEMORYSTATUSEX
{
  DWORD
    dwLength,
    dwMemoryLoad;

  DWORDLONG
    ullTotalPhys,
    ullAvailPhys,
    ullTotalPageFile,
    ullAvailPageFile,
    ullTotalVirtual,
    ullAvailVirtual,
    ullAvailExtendedVirtual;
} NTMEMORYSTATUSEX;

typedef UINT
  (CALLBACK *LPFNDLLFUNC1)(DWORD,UINT);

typedef UINT
  (CALLBACK *LPFNDLLFUNC2)(NTMEMORYSTATUSEX *);

#endif

#if !defined(ssize_t) && !defined(__MINGW32__)
typedef long ssize_t;
#endif

extern MagickExport char
  *NTGetLastError(void);

extern MagickExport const GhostscriptVectors
  *NTGhostscriptDLLVectors(void);

#if !defined(HasLTDL)
extern MagickExport const char
  *NTGetLibraryError(void);
#endif

#if !defined(XS_VERSION)
extern MagickExport char
  *NTRegistryKeyLookup(const char *);

extern MagickExport DIR
  *NTOpenDirectory(const char *);

extern MagickExport double
  NTElapsedTime(void),
  NTUserTime(void);

extern MagickExport int
  Exit(int),
  IsWindows95(),
  NTCloseDirectory(DIR *),
  NTCloseLibrary(void *),
  NTExitLibrary(void),
  NTGhostscriptDLL(char *,int),
  NTGhostscriptEXE(char *,int),
  NTGhostscriptFonts(char *,int),
  NTGhostscriptLoadDLL(void),
  NTGhostscriptUnLoadDLL(void),
  NTInitializeLibrary(void),
  NTSetSearchPath(const char *),
  NTSyncMemory(void *,size_t,int),
  NTUnmapMemory(void *,size_t),
  NTSystemCommand(const char *);

extern MagickExport long
  NTSystemConfiguration(int),
  NTTellDirectory(DIR *);

extern MagickExport MagickBooleanType
  NTGetExecutionPath(char *),
  NTGetModulePath(const char *,char *),
  NTReportEvent(const char *,const MagickBooleanType),
  NTReportException(const char *,const MagickBooleanType);

extern MagickExport struct dirent
  *NTReadDirectory(DIR *);

extern MagickExport unsigned char
  *NTResourceToBlob(const char *);

extern MagickExport void
  NTErrorHandler(const ExceptionType,const char *,const char *),
  *NTGetLibrarySymbol(void *,const char *),
  *NTMapMemory(char *,size_t,int,int,int,MagickOffsetType),
  *NTOpenLibrary(const char *),
  NTSeekDirectory(DIR *,long),
  NTWarningHandler(const ExceptionType,const char *,const char *);

#endif /* !XS_VERSION */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif /* !C++ */

#endif /* !_MAGICK_NTBASE_H */
