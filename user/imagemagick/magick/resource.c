/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%           RRRR    EEEEE   SSSSS   OOO   U   U  RRRR    CCCC  EEEEE          %
%           R   R   E       SS     O   O  U   U  R   R  C      E              %
%           RRRR    EEE      SSS   O   O  U   U  RRRR   C      EEE            %
%           R R     E          SS  O   O  U   U  R R    C      E              %
%           R  R    EEEEE   SSSSS   OOO    UUU   R  R    CCCC  EEEEE          %
%                                                                             %
%                                                                             %
%                        Get/Set ImageMagick Resources.                       %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                               September 2002                                %
%                                                                             %
%                                                                             %
%  Copyright 1999-2005 ImageMagick Studio LLC, a non-profit organization      %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  You may not use this file except in compliance with the License.  You may  %
%  obtain a copy of the License at                                            %
%                                                                             %
%    http://www.imagemagick.org/script/license.php                            %
%                                                                             %
%  Unless required by applicable law or agreed to in writing, software        %
%  distributed under the License is distributed on an "AS IS" BASIS,          %
%  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   %
%  See the License for the specific language governing permissions and        %
%  limitations under the License.                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/hashmap.h"
#include "magick/log.h"
#include "magick/image.h"
#include "magick/memory_.h"
#include "magick/option.h"
#include "magick/random_.h"
#include "magick/resource_.h"
#include "magick/semaphore.h"
#include "magick/signature.h"
#include "magick/string_.h"
#include "magick/splay-tree.h"
#include "magick/token.h"
#include "magick/utility.h"

/*
  Define  declarations.
*/
#define ResourceInfinity  (~0UL)
#define BytesToMegabytes(value) ((unsigned long) ((value)/1024/1024))
#define BytesToGigabytes(value) ((unsigned long) ((value)/1024/1024/1024))
#define GigabytesToBytes(value) ((MagickSizeType) (value)*1024*1024*1024)
#define MegabytesToBytes(value) ((MagickSizeType) (value)*1024*1024)

/*
  Typedef declarations.
*/
typedef struct _ResourceInfo
{
  MagickOffsetType
    area,
    memory,
    map,
    disk,
    file;

  unsigned long
    area_limit,
    memory_limit,
    map_limit,
    disk_limit,
    file_limit;
} ResourceInfo;

/*
  Global declarations.
*/
static ResourceInfo
  resource_info =
    { 0, 0, 0, 0, 0, 384, 768, 1536, ResourceInfinity, 768 };

static SemaphoreInfo
  *resource_semaphore = (SemaphoreInfo *) NULL;

static SplayTreeInfo
  *temporary_resources = (SplayTreeInfo *) NULL;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e M a g i c k R e s o u r c e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireMagickResource() acquires resources of the specified type.
%  MagickFalse is returned if the specified resource is exhausted otherwise
%  MagickTrue.
%
%  The format of the AcquireMagickResource() method is:
%
%      MagickBooleanType AcquireMagickResource(const ResourceType type,
%        const MagickSizeType size)
%
%  A description of each parameter follows:
%
%    o type: The type of resource.
%
%    o size: The number of bytes needed from for this resource.
%
%
*/
MagickExport MagickBooleanType AcquireMagickResource(const ResourceType type,
  const MagickSizeType size)
{
  char
    resource_current[MaxTextExtent],
    resource_limit[MaxTextExtent],
    resource_request[MaxTextExtent];

  MagickBooleanType
    status;

  MagickSizeType
    limit;

  status=MagickFalse;
  FormatSize(size,resource_request);
  AcquireSemaphoreInfo(&resource_semaphore);
  switch (type)
  {
    case AreaResource:
    {
      resource_info.area=(MagickOffsetType) size;
      limit=MegabytesToBytes(resource_info.area_limit);
      status=(resource_info.area_limit == ResourceInfinity) || (size < limit) ?
        MagickTrue : MagickFalse;
      FormatSize((MagickSizeType) resource_info.area,resource_current);
      FormatSize(MegabytesToBytes(resource_info.area_limit),resource_limit);
      break;
    }
    case MemoryResource:
    {
      resource_info.memory+=size;
      limit=MegabytesToBytes(resource_info.memory_limit);
      status=(resource_info.memory_limit == ResourceInfinity) ||
        ((MagickSizeType) resource_info.memory < limit) ?
        MagickTrue : MagickFalse;
      FormatSize((MagickSizeType) resource_info.memory,resource_current);
      FormatSize(MegabytesToBytes(resource_info.memory_limit),resource_limit);
      break;
    }
    case MapResource:
    {
      resource_info.map+=size;
      limit=MegabytesToBytes(resource_info.map_limit);
      status=(resource_info.map_limit == ResourceInfinity) ||
        ((MagickSizeType) resource_info.map < limit) ?
        MagickTrue : MagickFalse;
      FormatSize((MagickSizeType) resource_info.map,resource_current);
      FormatSize(MegabytesToBytes(resource_info.map_limit),resource_limit);
      break;
    }
    case DiskResource:
    {
      resource_info.disk+=size;
      limit=GigabytesToBytes(resource_info.disk_limit);
      status=(resource_info.disk_limit == ResourceInfinity) ||
        ((MagickSizeType) resource_info.disk < limit) ?
        MagickTrue : MagickFalse;
      FormatSize((MagickSizeType) resource_info.disk,resource_current);
      FormatSize(GigabytesToBytes(resource_info.disk_limit),resource_limit);
      break;
    }
    case FileResource:
    {
      resource_info.file+=size;
      limit=resource_info.file_limit;
      status=(resource_info.file_limit == ResourceInfinity) ||
        ((MagickSizeType) resource_info.file < limit) ?
        MagickTrue : MagickFalse;
      FormatSize((MagickSizeType) resource_info.file,resource_current);
      FormatSize((MagickSizeType) resource_info.file_limit,resource_limit);
      break;
    }
    default:
      break;
  }
  RelinquishSemaphoreInfo(resource_semaphore);
  (void) LogMagickEvent(ResourceEvent,GetMagickModule(),"%s: %s/%s/%s",
    MagickOptionToMnemonic(MagickResourceOptions,(long) type),resource_request,
    resource_current,resource_limit);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   A s y n c h r o n o u s D e s t r o y M a g i c k R e s o u r c e s       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AsynchronousDestroyMagickResources() destroys the resource environment.
%  It differs from DestroyMagickResources() in that it can be called from a
%  asynchronous signal handler.
%
%  The format of the DestroyMagickResources() method is:
%
%      DestroyMagickResources(void)
%
%
*/
MagickExport void AsynchronousDestroyMagickResources(void)
{
  const char
    *path;

  if (temporary_resources == (SplayTreeInfo *) NULL)
    return;
  /*
    Remove any lingering temporary files.
  */
  ResetSplayTreeIterator(temporary_resources);
  path=(const char *) GetNextKeyInSplayTree(temporary_resources);
  while (path != (const char *) NULL)
  {
    (void) remove(path);
    path=(const char *) GetNextKeyInSplayTree(temporary_resources);
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e U n i q u e F i l e R e s o u r c e                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireUniqueFileResource() returns a unique file name, and returns a file
%  descriptor for the file open for reading and writing.
%
%  The format of the AcquireUniqueFileResource() method is:
%
%      int AcquireUniqueFileResource(char *path)
%
%  A description of each parameter follows:
%
%   o  path:  Specifies a pointer to an array of characters.  The unique path
%      name is returned in this array.
%
%
*/

static void *DestroyTemporaryResources(void *temporary_resource)
{
  remove((char *) temporary_resource);
  return((void *) NULL);
}

static MagickBooleanType GetPathTemplate(char *path)
{
  const char
    *directory;

  int
    status;

  struct stat
    file_info;

  (void) strcpy(path,"magick-XXXXXXXX");
  directory=getenv("MAGICK_TMPDIR");
  if (directory == (const char *) NULL)
    directory=getenv("TMPDIR");
#if defined(__WINDOWS__)
  if (directory == (const char *) NULL)
    directory=getenv("TMP");
  if (directory == (const char *) NULL)
    directory=getenv("TEMP");
#endif
#if defined(__VMS)
   directory=getenv("MTMPDIR");
#endif
#if defined(P_tmpdir)
  if (directory == (const char *) NULL)
    directory=P_tmpdir;
#endif
  if (directory == (const char *) NULL)
    return(MagickTrue);
  if (strlen(directory) > (MaxTextExtent-15))
    return(MagickTrue);
  status=stat(directory,&file_info);
  if ((status != 0) || !S_ISDIR(file_info.st_mode))
    return(MagickTrue);
  if (directory[strlen(directory)-1] == *DirectorySeparator)
    (void) FormatMagickString(path,MaxTextExtent,"%smagick-XXXXXXXX",directory);
  else
    (void) FormatMagickString(path,MaxTextExtent,"%s%smagick-XXXXXXXX",
      directory,DirectorySeparator);
  return(MagickTrue);
}

MagickExport int AcquireUniqueFileResource(char *path)
{
#if !defined(O_NOFOLLOW)
#define O_NOFOLLOW 0
#endif
#if !defined(TMP_MAX)
# define TMP_MAX  238328
#endif

  char
    *resource;

  int
    c,
    file;

  register char
    *p;

  register long
    i;

  unsigned char
    key[8];

  assert(path != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  file=(-1);
  for (i=0; i < TMP_MAX; i++)
  {
    /*
      Get temporary pathname.
    */
    (void) GetPathTemplate(path);
#if defined(HAVE_MKSTEMP)
    file=mkstemp(path);
    if (file != -1)
      break;
#endif
    GetRandomKey(key,8);
    p=path+strlen(path)-8;
    for (i=0; i < 8; i++)
    {
      c=(int) (key[i] & 0x1f);
      *p++=(char) (c > 9 ? c+(int) 'a'-10 : c+(int) '0');
    }
    file=open(path,O_RDWR | O_CREAT | O_EXCL | O_BINARY | O_NOFOLLOW,S_MODE);
    if ((file > 0) || (errno != EEXIST))
      break;
  }
  (void) LogMagickEvent(ResourceEvent,GetMagickModule(),"%s",path);
  if (file == -1)
    return(file);
  (void) AcquireMagickResource(FileResource,1);
  if (temporary_resources == (SplayTreeInfo *) NULL)
    temporary_resources=NewSplayTree(CompareSplayTreeString,
      RelinquishMagickMemory,DestroyTemporaryResources);
  resource=ConstantString(AcquireString(path));
  (void) AddValueToSplayTree(temporary_resources,resource,resource);
  return(file);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y M a g i c k R e s o u r c e s                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyMagickResources() destroys the resource environment.
%
%  The format of the DestroyMagickResources() method is:
%
%      DestroyMagickResources(void)
%
%
*/
MagickExport void DestroyMagickResources(void)
{
  AcquireSemaphoreInfo(&resource_semaphore);
  if (temporary_resources != (SplayTreeInfo *) NULL)
    temporary_resources=DestroySplayTree(temporary_resources);
  RelinquishSemaphoreInfo(resource_semaphore);
  resource_semaphore=DestroySemaphoreInfo(resource_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k R e s o u r c e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickResource() returns the the specified resource in megabytes.
%
%  The format of the GetMagickResource() method is:
%
%      unsigned long GetMagickResource(const ResourceType type)
%
%  A description of each parameter follows:
%
%    o type: The type of resource.
%
%
*/
MagickExport unsigned long GetMagickResource(const ResourceType type)
{
  unsigned long
    resource;

  resource=0;
  AcquireSemaphoreInfo(&resource_semaphore);
  switch (type)
  {
    case AreaResource:
    {
      resource=BytesToMegabytes(resource_info.area);
      break;
    }
    case MemoryResource:
    {
      resource=BytesToMegabytes(resource_info.memory);
      break;
    }
    case MapResource:
    {
      resource=BytesToMegabytes(resource_info.map);
      break;
    }
    case DiskResource:
    {
      resource=BytesToGigabytes(resource_info.disk);
      break;
    }
    case FileResource:
    {
      resource=(unsigned long) resource_info.file;
      break;
    }
    default:
      break;
  }
  RelinquishSemaphoreInfo(resource_semaphore);
  return(resource);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k R e s o u r c e L i m i t                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickResource() returns the the specified resource limit in megabytes.
%
%  The format of the GetMagickResourceLimit() method is:
%
%      unsigned long GetMagickResourceLimit(const ResourceType type)
%
%  A description of each parameter follows:
%
%    o type: The type of resource.
%
*/
MagickExport unsigned long GetMagickResourceLimit(const ResourceType type)
{
  unsigned long
    resource;

  resource=0;
  AcquireSemaphoreInfo(&resource_semaphore);
  switch (type)
  {
    case AreaResource:
    {
      resource=resource_info.area_limit;
      break;
    }
    case MemoryResource:
    {
      resource=resource_info.memory_limit;
      break;
    }
    case MapResource:
    {
      resource=resource_info.map_limit;
      break;
    }
    case DiskResource:
    {
      resource=resource_info.disk_limit;
      break;
    }
    case FileResource:
    {
      resource=resource_info.file_limit;
      break;
    }
    default:
      break;
  }
  RelinquishSemaphoreInfo(resource_semaphore);
  return(resource);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I n i t i a l i z e M a g i c k R e s o u r c e s                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InitializeMagickResources() initializes the resource environment.
%
%  The format of the InitializeMagickResources() method is:
%
%      InitializeMagickResources(void)
%
%
*/
MagickExport void InitializeMagickResources(void)
{
  const char
    *p;

  long
    files,
    pages,
    pagesize;

  unsigned long
    memory;

  /*
    Set Magick resource limits.
  */
  pagesize=(-1);
#if defined(HAVE_SYSCONF) && defined(_SC_PAGESIZE)
  pagesize=sysconf(_SC_PAGESIZE);
#elif defined(HAVE_GETPAGESIZE) && defined(POSIX)
  pagesize=getpagesize();
#endif
  if (pagesize <= 0)
    pagesize=4096;
  pages=(-1);
#if defined(HAVE_SYSCONF) && defined(_SC_PHYS_PAGES)
  pages=sysconf(_SC_PHYS_PAGES);
#endif
  if (pages <= 0)
    pages=62500;
  memory=(unsigned long) ((pages+512)/1024)*((pagesize+512)/1024);
#if defined(PixelCacheThreshold)
  memory=PixelCacheThreshold;
#endif
  (void) SetMagickResourceLimit(AreaResource,memory/2);
  (void) SetMagickResourceLimit(MemoryResource,memory);
  (void) SetMagickResourceLimit(MapResource,2*memory);
  p=getenv("MAGICK_AREA_LIMIT");
  if (p != (const char *) NULL)
    (void) SetMagickResourceLimit(AreaResource,(unsigned long) atol(p));
  p=getenv("MAGICK_MEMORY_LIMIT");
  if (p != (const char *) NULL)
    (void) SetMagickResourceLimit(MemoryResource,(unsigned long) atol(p));
  p=getenv("MAGICK_MAP_LIMIT");
  if (p != (const char *) NULL)
    (void) SetMagickResourceLimit(MapResource,(unsigned long) atol(p));
  p=getenv("MAGICK_DISK_LIMIT");
  if (p != (const char *) NULL)
    (void) SetMagickResourceLimit(DiskResource,(unsigned long) atol(p));
  files=(-1);
#if defined(HAVE_SYSCONF) && defined(_SC_OPEN_MAX)
  files=sysconf(_SC_OPEN_MAX);
#elif defined(HAVE_GETDTABLESIZE) && defined(POSIX)
  files=getdtablesize();
#endif
  if (files <= 0)
    files=64;
  (void) SetMagickResourceLimit(FileResource,(unsigned long) Max(3*files/4,8));
  p=getenv("MAGICK_FILES_LIMIT");
  if (p != (const char *) NULL)
    (void) SetMagickResourceLimit(FileResource,(unsigned long) atol(p));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L i s t M a g i c k R e s o u r c e I n f o                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListMagickResourceInfo() lists the resource info to a file.
%
%  The format of the ListMagickResourceInfo method is:
%
%      MagickBooleanType ListMagickResourceInfo(FILE *file,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o file:  An pointer to a FILE.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType ListMagickResourceInfo(FILE *file,
  ExceptionInfo *exception)
{
  char
    area_limit[MaxTextExtent],
    disk_limit[MaxTextExtent],
    file_limit[MaxTextExtent],
    map_limit[MaxTextExtent],
    memory_limit[MaxTextExtent];

  if (file == (const FILE *) NULL)
    file=stdout;
  AcquireSemaphoreInfo(&resource_semaphore);
  FormatSize(MegabytesToBytes(resource_info.area_limit),area_limit);
  FormatSize(GigabytesToBytes(resource_info.disk_limit),disk_limit);
  FormatSize((MagickSizeType) resource_info.file_limit,file_limit);
  FormatSize(MegabytesToBytes(resource_info.map_limit),map_limit);
  FormatSize(MegabytesToBytes(resource_info.memory_limit),memory_limit);
  (void) fprintf(file,"File       Area     Memory        Map       Disk\n");
  (void) fprintf(file,"------------------------------------------------\n");
  (void) fprintf(file,"%4s %10s %10s %10s %10s\n",file_limit,area_limit,
    memory_limit,map_limit,disk_limit);
  (void) fflush(file);
  RelinquishSemaphoreInfo(resource_semaphore);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e l i n q u i s h M a g i c k R e s o u r c e                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RelinquishMagickResource() relinquishes resources of the specified type.
%
%  The format of the RelinquishMagickResource() method is:
%
%      void RelinquishMagickResource(const ResourceType type,
%        const MagickSizeType size)
%
%  A description of each parameter follows:
%
%    o type: The type of resource.
%
%    o size: The size of the resource.
%
%
*/
MagickExport void RelinquishMagickResource(const ResourceType type,
  const MagickSizeType size)
{
  char
    resource_current[MaxTextExtent],
    resource_limit[MaxTextExtent],
    resource_request[MaxTextExtent];

  FormatSize(size,resource_request);
  AcquireSemaphoreInfo(&resource_semaphore);
  switch (type)
  {
    case AreaResource:
    {
      resource_info.area=size;
      FormatSize((MagickSizeType) resource_info.area,resource_current);
      FormatSize(MegabytesToBytes(resource_info.area_limit),resource_limit);
      break;
    }
    case MemoryResource:
    {
      resource_info.memory-=size;
      FormatSize((MagickSizeType) resource_info.memory,resource_current);
      FormatSize(MegabytesToBytes(resource_info.memory_limit),resource_limit);
      break;
    }
    case MapResource:
    {
      resource_info.map-=size;
      FormatSize((MagickSizeType) resource_info.map,resource_current);
      FormatSize(MegabytesToBytes(resource_info.map_limit),resource_limit);
      break;
    }
    case DiskResource:
    {
      resource_info.disk-=size;
      FormatSize((MagickSizeType) resource_info.disk,resource_current);
      FormatSize(GigabytesToBytes(resource_info.disk_limit),resource_limit);
      break;
    }
    case FileResource:
    {
      resource_info.file-=size;
      FormatSize((MagickSizeType) resource_info.file,resource_current);
      FormatSize((MagickSizeType) resource_info.file_limit,resource_limit);
      break;
    }
    default:
      break;
  }
  RelinquishSemaphoreInfo(resource_semaphore);
  (void) LogMagickEvent(ResourceEvent,GetMagickModule(),"%s: %s/%s/%s",
    MagickOptionToMnemonic(MagickResourceOptions,(long) type),resource_request,
    resource_current,resource_limit);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%    R e l i n q u i s h U n i q u e F i l e R e s o u r c e                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RelinquishUniqueFileResource() relinquishes a unique file resource.
%
%  The format of the RelinquishUniqueFileResource() method is:
%
%      MagickBooleanType RelinquishUniqueFileResource(const char *path)
%
%  A description of each parameter follows:
%
%    o name: the name of the temporary resource.
%
%
*/
MagickExport MagickBooleanType RelinquishUniqueFileResource(const char *path)
{
  char
    cache_path[MaxTextExtent];

  assert(path != (const char *) NULL);
  (void) LogMagickEvent(ResourceEvent,GetMagickModule(),"%s",path);
  if (temporary_resources != (SplayTreeInfo *) NULL)
    {
      register char
        *p;

      ResetSplayTreeIterator(temporary_resources);
      p=(char *) GetNextKeyInSplayTree(temporary_resources);
      while (p != (char *) NULL)
      {
        if (LocaleCompare(p,path) == 0)
          break;
        p=(char *) GetNextKeyInSplayTree(temporary_resources);
      }
      if (p != (char *) NULL)
        {
          (void) RemoveNodeFromSplayTree(temporary_resources,p);
          RelinquishMagickResource(FileResource,1);
        }
    }
  (void) CopyMagickString(cache_path,path,MaxTextExtent);
  AppendImageFormat("cache",cache_path);
  (void) remove(cache_path);
  return(remove(path) == 0 ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t M a g i c k R e s o u r c e L i m i t                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetMagickResourceLimit() sets the limit for a particular resource in
%  megabytes.
%
%  The format of the SetMagickResourceLimit() method is:
%
%      MagickBooleanType SetMagickResourceLimit(const ResourceType type,
%        const unsigned long limit)
%
%  A description of each parameter follows:
%
%    o type: The type of resource.
%
%    o limit: The maximum limit for the resource.
%
%
*/
MagickExport MagickBooleanType SetMagickResourceLimit(const ResourceType type,
  const unsigned long limit)
{
  AcquireSemaphoreInfo(&resource_semaphore);
  switch (type)
  {
    case AreaResource:
    {
      resource_info.area_limit=limit;
      break;
    }
    case MemoryResource:
    {
      resource_info.memory_limit=limit;
      break;
    }
    case MapResource:
    {
      resource_info.map_limit=limit;
      break;
    }
    case DiskResource:
    {
      resource_info.disk_limit=limit;
      break;
    }
    case FileResource:
    {
      resource_info.file_limit=limit;
      break;
    }
    default:
      break;
  }
  RelinquishSemaphoreInfo(resource_semaphore);
  return(MagickTrue);
}
