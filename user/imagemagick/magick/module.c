/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  M   M   OOO   DDDD   U   U  L      EEEEE                   %
%                  MM MM  O   O  D   D  U   U  L      E                       %
%                  M M M  O   O  D   D  U   U  L      EEE                     %
%                  M   M  O   O  D   D  U   U  L      E                       %
%                  M   M   OOO   DDDD    UUU   LLLLL  EEEEE                   %
%                                                                             %
%                                                                             %
%                         ImageMagick Module Methods                          %
%                                                                             %
%                              Software Design                                %
%                              Bob Friesenhahn                                %
%                                March 2000                                   %
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
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/blob.h"
#include "magick/coder.h"
#include "magick/client.h"
#include "magick/configure.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/log.h"
#include "magick/hashmap.h"
#include "magick/magic.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/module.h"
#include "magick/semaphore.h"
#include "magick/splay-tree.h"
#include "magick/static.h"
#include "magick/string_.h"
#include "magick/token.h"
#include "magick/utility.h"
#if defined(SupportMagickModules)
#if defined(HasLTDL)
#include "ltdl.h"
typedef lt_dlhandle ModuleHandle;
#else
typedef void *ModuleHandle;
#endif

/*
  Define declarations.
*/
#if defined(HasLTDL)
#  define ModuleGlobExpression "*.la"
#else
#  if defined(_DEBUG)
#    define ModuleGlobExpression "IM_MOD_DB_*.dll"
#  else
#    define ModuleGlobExpression "IM_MOD_RL_*.dll"
#  endif
#endif

/*
  Global declarations.
*/
static SemaphoreInfo
  *module_semaphore = (SemaphoreInfo *) NULL;

static SplayTreeInfo
  *module_list = (SplayTreeInfo *) NULL;

static volatile MagickBooleanType
  instantiate_module = MagickFalse;

/*
  Forward declarations.
*/
static const ModuleInfo
  *RegisterModule(const ModuleInfo *,ExceptionInfo *);

static MagickBooleanType
  GetMagickModulePath(const char *,MagickModuleType,char *,ExceptionInfo *),
  InitializeModuleList(ExceptionInfo *),
  UnregisterModule(const ModuleInfo *,ExceptionInfo *);

static void
  TagToCoderModuleName(const char *,char *),
  TagToFilterModuleName(const char *,char *),
  TagToModuleName(const char *,const char *,char *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y M o d u l e L i s t                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyMagickList() unregisters any previously loaded modules and exits
%  the module loaded environment.
%
%  The format of the DestroyMagickList module is:
%
%      void DestroyMagickList(void)
%
*/
MagickExport void DestroyModuleList(void)
{
  /*
    Deestroy magick modules.
  */
  AcquireSemaphoreInfo(&module_semaphore);
#if defined(SupportMagickModules)
  if (module_list != (SplayTreeInfo *) NULL)
    module_list=DestroySplayTree(module_list);
  if (instantiate_module != MagickFalse)
    (void) lt_dlexit();
#endif
  instantiate_module=MagickFalse;
  RelinquishSemaphoreInfo(module_semaphore);
  module_semaphore=DestroySemaphoreInfo(module_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x e c u t e M o d u l e P r o c e s s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExecuteModuleProcess() executes a dynamic modules.
%
%  The format of the ExecuteModuleProcess module is:
%
%      MagickBooleanType ExecuteModuleProcess(const char *tag,Image **image,
%        const int argc,char **argv)
%
%  A description of each parameter follows:
%
%    o tag: a character string that represents the name of the particular
%      module.
%
%    o image: The image.
%
%    o argc: Specifies a pointer to an integer describing the number of
%      elements in the argument vector.
%
%    o argv: Specifies a pointer to a text array containing the command line
%      arguments.
%
*/
MagickExport MagickBooleanType ExecuteModuleProcess(const char *tag,
  Image **image,const int argc,char **argv)
{
  char
    name[MaxTextExtent],
    path[MaxTextExtent];

  MagickBooleanType
    (*module)(Image **,const int,char **),
    status;

  ModuleHandle
    handle;

  /*
    Find the module.
  */
  assert(image != (Image **) NULL);
  assert((*image)->signature == MagickSignature);
  if ((*image)->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",(*image)->filename);
#if !defined(BuildMagickModules)
  status=ExecuteStaticModuleProcess(tag,image,argc,argv);
  if (status != MagickFalse)
    return(status);
#endif
  status=MagickFalse;
  TagToFilterModuleName(tag,name);
  status=GetMagickModulePath(name,MagickFilterModule,path,&(*image)->exception);
  if (status == MagickFalse)
    return(status);
  /*
    Open the module.
  */
  handle=lt_dlopen(path);
  if (handle == (ModuleHandle) NULL)
    {
      (void) ThrowMagickException(&(*image)->exception,GetMagickModule(),
        ModuleError,"UnableToLoadModule","`%s': %s",name,lt_dlerror());
      return(status);
    }
  /*
    Locate the module.
  */
#if !defined(MagickMethodPrefix)
  (void) FormatMagickString(name,MaxTextExtent,"%sImage",tag);
#else
  (void) FormatMagickString(name,MaxTextExtent,"%s%sImage",MagickMethodPrefix,
    tag);
#endif
  /*
    Execute the module.
  */
  module=(MagickBooleanType (*)(Image **,const int,char **))
    lt_dlsym(handle,name);
  if (module != (MagickBooleanType (*)(Image **,const int,char **)) NULL)
    {
      if ((*image)->debug != MagickFalse)
        (void) LogMagickEvent(ModuleEvent,GetMagickModule(),
          "Invoking \"%s\" dynamic filter module",tag);
      status=(*module)(image,argc,argv);
      if ((*image)->debug != MagickFalse)
        (void) LogMagickEvent(ModuleEvent,GetMagickModule(),"\"%s\" completes",
          tag);
    }
  /*
    Close the module.
  */
  (void) lt_dlclose(handle);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M o d u l e I n f o                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetModuleInfo() returns a pointer to a ModuleInfo structure that matches the
%  specified tag.  If tag is NULL, the head of the module list is returned. If
%  no modules are loaded, or the requested module is not found, NULL is
%  returned.
%
%  The format of the GetModuleInfo module is:
%
%      const ModuleInfo *GetModuleInfo(const char *tag,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o tag: a character string that represents the image format we are
%      looking for.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport const ModuleInfo *GetModuleInfo(const char *tag,
  ExceptionInfo *exception)
{
  register const ModuleInfo
    *p;

  if ((module_list == (SplayTreeInfo *) NULL) ||
      (instantiate_module == MagickFalse))
    if (InitializeModuleList(exception) == MagickFalse)
      return((const ModuleInfo *) NULL);
  if ((module_list == (SplayTreeInfo *) NULL) ||
      (GetNumberOfNodesInSplayTree(module_list) == 0))
    return((const ModuleInfo *) NULL);
  if ((tag == (const char *) NULL) || (LocaleCompare(tag,"*") == 0))
    {
#if defined(SupportMagickModules)
      if (LocaleCompare(tag,"*") == 0)
        (void) OpenModules(exception);
#endif
      ResetSplayTreeIterator(module_list);
      return((const ModuleInfo *) GetNextValueInSplayTree(module_list));
    }
  AcquireSemaphoreInfo(&module_semaphore);
  ResetSplayTreeIterator(module_list);
  p=(const ModuleInfo *) GetNextValueInSplayTree(module_list);
  while (p != (const ModuleInfo *) NULL)
  {
    if (LocaleCompare(p->tag,tag) == 0)
      break;
    p=(const ModuleInfo *) GetNextValueInSplayTree(module_list);
  }
  if (p == (ModuleInfo *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
      "NoSuchElement","`%s'",tag);
  RelinquishSemaphoreInfo(module_semaphore);
  return((const ModuleInfo *) p);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M o d u l e I n f o L i s t                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetModuleInfoList() returns any modules that match the specified pattern.
%
%  The format of the GetModuleInfoList function is:
%
%      const ModuleInfo **GetModuleInfoList(const char *pattern,
%        unsigned long *number_modules,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_modules:  This integer returns the number of modules in the list.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int ModuleInfoCompare(const void *x,const void *y)
{
  const ModuleInfo
    **p,
    **q;

  p=(const ModuleInfo **) x,
  q=(const ModuleInfo **) y;
  if (LocaleCompare((*p)->path,(*q)->path) == 0)
    return(LocaleCompare((*p)->tag,(*q)->tag));
  return(LocaleCompare((*p)->path,(*q)->path));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport const ModuleInfo **GetModuleInfoList(const char *pattern,
  unsigned long *number_modules,ExceptionInfo *exception)
{
  const ModuleInfo
    **modules;

  register const ModuleInfo
    *p;

  register long
    i;

  /*
    Allocate module list.
  */
  assert(pattern != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  assert(number_modules != (unsigned long *) NULL);
  *number_modules=0;
  p=GetModuleInfo("*",exception);
  if (p == (const ModuleInfo *) NULL)
    return((const ModuleInfo **) NULL);
  modules=(const ModuleInfo **) AcquireMagickMemory((size_t)
    (GetNumberOfNodesInSplayTree(module_list)+1)*sizeof(*modules));
  if (modules == (const ModuleInfo **) NULL)
    return((const ModuleInfo **) NULL);
  /*
    Generate module list.
  */
  AcquireSemaphoreInfo(&module_semaphore);
  ResetSplayTreeIterator(module_list);
  p=(const ModuleInfo *) GetNextValueInSplayTree(module_list);
  for (i=0; p != (const ModuleInfo *) NULL; )
  {
    if ((p->stealth == MagickFalse) &&
        (GlobExpression(p->tag,pattern) != MagickFalse))
      modules[i++]=p;
    p=(const ModuleInfo *) GetNextValueInSplayTree(module_list);
  }
  RelinquishSemaphoreInfo(module_semaphore);
  qsort((void *) modules,(size_t) i,sizeof(*modules),ModuleInfoCompare);
  modules[i]=(ModuleInfo *) NULL;
  *number_modules=(unsigned long) i;
  return(modules);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M o d u l e L i s t                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetModuleList() returns any image format modules that match the specified
%  pattern.
%
%  The format of the GetModuleList function is:
%
%      char **GetModuleList(const char *pattern,unsigned long *number_modules,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_modules:  This integer returns the number of modules in the
%      list.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int ModuleCompare(const void *x,const void *y)
{
  register const char
    **p,
    **q;

   p=(const char **) x;
  q=(const char **) y;
  return(LocaleCompare(*p,*q));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport char **GetModuleList(const char *pattern,
  unsigned long *number_modules,ExceptionInfo *exception)
{
  char
    **modules,
    filename[MaxTextExtent],
    module_path[MaxTextExtent],
    path[MaxTextExtent];

  DIR
    *directory;

  MagickBooleanType
    status;

  register long
    i;

  struct dirent
    *entry;

  unsigned long
    max_entries;

  /*
    Locate all modules in the coder path.
  */
  TagToCoderModuleName("magick",filename);
  status=GetMagickModulePath(filename,MagickCoderModule,module_path,
    exception);
  if (status == MagickFalse)
    return((char **) NULL);
  GetPathComponent(module_path,HeadPath,path);
  max_entries=255;
  modules=(char **)
    AcquireMagickMemory((size_t) (max_entries+1)*sizeof(*modules));
  if (modules == (char **) NULL)
    return((char **) NULL);
  *modules=(char *) NULL;
  directory=opendir(path);
  if (directory == (DIR *) NULL)
    return((char **) NULL);
  i=0;
  entry=readdir(directory);
  while (entry != (struct dirent *) NULL)
  {
    if (GlobExpression(entry->d_name,ModuleGlobExpression) == MagickFalse)
      {
        entry=readdir(directory);
        continue;
      }
    if (GlobExpression(entry->d_name,pattern) == MagickFalse)
      continue;
    if (i >= (long) max_entries)
      {
        max_entries<<=1;
        modules=(char **)
          ResizeMagickMemory(modules,(size_t) max_entries*sizeof(*modules));
        if (modules == (char **) NULL)
          break;
      }
    /*
      Add new module name to list.
    */
    modules[i]=AcquireString((char *) NULL);
    GetPathComponent(entry->d_name,BasePath,modules[i]);
    LocaleUpper(modules[i]);
    if (LocaleNCompare("IM_MOD_",modules[i],7) == 0)
      {
        (void) CopyMagickString(modules[i],modules[i]+10,MaxTextExtent);
        modules[i][strlen(modules[i])-1]='\0';
      }
    i++;
    entry=readdir(directory);
  }
  (void) closedir(directory);
  qsort((void *) modules,(size_t) i,sizeof(*modules),ModuleCompare);
  modules[i]=(char *) NULL;
  *number_modules=(unsigned long) i;
  return(modules);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  G e t M a g i c k M o d u l e P a t h                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickModulePath() finds a module with the specified module type and
%  filename.
%
%  The format of the GetMagickModulePath module is:
%
%      MagickBooleanType GetMagickModulePath(const char *filename,
%        MagickModuleType module_type,char *path,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: The module file name.
%
%    o module_type: The module type: MagickCoderModule or MagickFilterModule.
%
%    o path: The path associated with the filename.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
static MagickBooleanType GetMagickModulePath(const char *filename,
  MagickModuleType module_type,char *path,ExceptionInfo *exception)
{
  const char
    *module_path;

  register char
    *q;

  register const char
    *p;

  assert(filename != (const char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",filename);
  assert(path != (char *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  (void) CopyMagickString(path,filename,MaxTextExtent);
  module_path=(const char *) NULL;
  switch (module_type)
  {
    case MagickCoderModule:
    default:
    {
      (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
        "Searching for coder module file \"%s\" ...",filename);
      module_path=getenv("MAGICK_CODER_MODULE_PATH");
      break;
    }
    case MagickFilterModule:
    {
      (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
        "Searching for filter module file \"%s\" ...",filename);
      module_path=getenv("MAGICK_FILTER_MODULE_PATH");
      break;
    }
  }
  if (module_path != (const char *) NULL)
    for (p=module_path-1; p != (char *) NULL; )
    {
      (void) CopyMagickString(path,p+1,MaxTextExtent);
      q=strchr(path,DirectoryListSeparator);
      if (q != (char *) NULL)
        *q='\0';
      q=path+strlen(path)-1;
      if ((q >= path) && (*q != *DirectorySeparator))
        (void) ConcatenateMagickString(path,DirectorySeparator,MaxTextExtent);
      (void) ConcatenateMagickString(path,filename,MaxTextExtent);
      if (IsAccessible(path) != MagickFalse)
        return(MagickTrue);
      p=strchr(p+1,DirectoryListSeparator);
    }
#if defined(UseInstalledMagick)
#if defined(MagickCoderModulesPath)
  {
    const char
      *directory;

    /*
      Search hard coded paths.
    */
    switch (module_type)
    {
      case MagickCoderModule:
      default:
      {
        directory=MagickCoderModulesPath;
        break;
      }
      case MagickFilterModule:
      {
        directory=MagickFilterModulesPath;
        break;
      }
    }
    (void) FormatMagickString(path,MaxTextExtent,"%s%s",directory,filename);
    if (IsAccessible(path) == MagickFalse)
      {
        ThrowFileException(exception,ConfigureWarning,"UnableToOpenModuleFile",
          path);
        return(MagickFalse);
      }
    return(MagickTrue);
  }
#else
#if defined(__WINDOWS__)
  {
    const char
      *key,
      *value;

    /*
      Locate path via registry key.
    */
    switch (module_type)
    {
      case MagickCoderModule:
      default:
      {
        key="CoderModulesPath";
        break;
      }
      case MagickFilterModule:
      {
        key="FilterModulesPath";
        break;
      }
    }
    value=NTRegistryKeyLookup(key);
    if (value == (char *) NULL)
      {
        ThrowMagickException(exception,GetMagickModule(),ConfigureError,
          "RegistryKeyLookupFailed","`%s'",key);
        return (MagickFalse);
      }
    (void) FormatMagickString(path,MaxTextExtent,"%s%s%s",value,
      DirectorySeparator,filename);
    if (IsAccessible(path) == MagickFalse)
      {
        ThrowFileException(exception,ConfigureWarning,"UnableToOpenModuleFile",
          path);
        return(MagickFalse);
      }
    return(MagickTrue);
  }
#endif
#endif
#if !defined(MagickCoderModulesPath) && !defined(__WINDOWS__)
# error MagickCoderModulesPath or WIN32 must be defined when UseInstalledMagick is defined
#endif
#else
  {
    const char
      *home;

    home=getenv("MAGICK_HOME");
    if (home != (char *) NULL)
      {
        /*
          Search MAGICK_HOME.
        */
#if !defined(POSIX)
        (void) FormatMagickString(path,MaxTextExtent,"%s%s%s",home,
          DirectorySeparator,filename);
#else
        const char
          *directory;

        switch (module_type)
        {
          case MagickCoderModule:
          default:
          {
            directory=MagickCoderModulesSubdir;
            break;
          }
          case MagickFilterModule:
          {
            directory=MagickFilterModulesSubdir;
            break;
          }
        }
        (void) FormatMagickString(path,MaxTextExtent,"%s/lib/%s/%s",home,
          directory,filename);
#endif
        if (IsAccessible(path) != MagickFalse)
          return(MagickTrue);
      }
  }
  if (*GetClientPath() != '\0')
    {
      /*
        Search based on executable directory.
      */
#if !defined(POSIX)
      (void) FormatMagickString(path,MaxTextExtent,"%s%s%s",GetClientPath(),
        DirectorySeparator,filename);
#else
      char
        prefix[MaxTextExtent];

      const char
        *directory;

      switch (module_type)
      {
        case MagickCoderModule:
        default:
        {
          directory="modules";
          break;
        }
        case MagickFilterModule:
        {
          directory="filters";
          break;
        }
      }
      (void) CopyMagickString(prefix,GetClientPath(),MaxTextExtent);
      ChopPathComponents(prefix,1);
      (void) FormatMagickString(path,MaxTextExtent,
        "%s/lib/%s/modules-Q%d/%s/%s",prefix,MagickLibSubdir,QuantumDepth,
        directory,filename);
#endif
      if (IsAccessible(path) != MagickFalse)
        return(MagickTrue);
    }
#if defined(__WINDOWS__)
  {
    /*
      Search module path.
    */
    if (NTGetModulePath("CORE_RL_magick_.dll",path) != MagickFalse)
      if (IsAccessible(path) != MagickFalse)
        return(MagickTrue);
    if (NTGetModulePath("Magick.dll",path) != MagickFalse)
      if (IsAccessible(path) != MagickFalse)
        return(MagickTrue);
  }
#endif
  {
    const char
      *home;

    home=getenv("HOME");
    if (home != (char *) NULL)
      {
        /*
          Search $HOME/.magick.
        */
        (void) FormatMagickString(path,MaxTextExtent,"%s%s%s%s",home,
          *home == '/' ? "/.magick" : "",DirectorySeparator,filename);
        if (IsAccessible(path) != MagickFalse)
          return(MagickTrue);
      }
  }
  /*
    Search current directory.
  */
  if (IsAccessible(path) != MagickFalse)
    return(MagickTrue);
  if (exception->severity < ConfigureError)
    ThrowFileException(exception,ConfigureWarning,"UnableToOpenModuleFile",
      path);
  return(MagickFalse);
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I n i t i a l i z e M o d u l e L i s t                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InitializeModuleList() initializes the module loader.
%
%  The format of the InitializeModuleList() method is:
%
%      InitializeModuleList(Exceptioninfo *exception)
%
%  A description of each parameter follows.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

static void *DestroyModuleNode(void *module_info)
{
  ExceptionInfo
    exception;

  register ModuleInfo
    *p;

  GetExceptionInfo(&exception);
  p=(ModuleInfo *) module_info;
  if (UnregisterModule(p,&exception) == MagickFalse)
    CatchException(&exception);
  if (p->tag != (char *) NULL)
    p->tag=(char *) RelinquishMagickMemory(p->tag);
  if (p->path != (char *) NULL)
    p->path=(char *) RelinquishMagickMemory(p->path);
  DestroyExceptionInfo(&exception);
  return(RelinquishMagickMemory(p));
}

static MagickBooleanType InitializeModuleList(
  ExceptionInfo *magick_unused(exception))
{
  if ((module_list == (SplayTreeInfo *) NULL) &&
      (instantiate_module == MagickFalse))
    {
      AcquireSemaphoreInfo(&module_semaphore);
      if ((module_list == (SplayTreeInfo *) NULL) &&
          (instantiate_module == MagickFalse))
        {
          MagickBooleanType
            status;

          ModuleInfo
            *module_info;

          module_list=NewSplayTree(CompareSplayTreeString,
            RelinquishMagickMemory,DestroyModuleNode);
          if (module_list == (SplayTreeInfo *) NULL)
            ThrowMagickFatalException(ResourceLimitFatalError,
              "MemoryAllocationFailed",strerror(errno));
          module_info=(ModuleInfo *) AcquireMagickMemory(sizeof(*module_info));
          if (module_info == (ModuleInfo *) NULL)
            ThrowMagickFatalException(ResourceLimitFatalError,
              "MemoryAllocationFailed",strerror(errno));
          (void) ResetMagickMemory(module_info,0,sizeof(*module_info));
          module_info->tag=ConstantString(AcquireString("[boot-strap]"));
          module_info->stealth=MagickTrue;
          (void) time(&module_info->load_time);
          module_info->signature=MagickSignature;
          status=AddValueToSplayTree(module_list,
            ConstantString(AcquireString(module_info->tag)),module_info);
          if (status == MagickFalse)
            ThrowMagickFatalException(ResourceLimitFatalError,
              "MemoryAllocationFailed",strerror(errno));
          if (lt_dlinit() != 0)
            ThrowMagickFatalException(ModuleFatalError,
              "UnableToInitializeModuleLoader",lt_dlerror());
          instantiate_module=MagickTrue;
        }
      RelinquishSemaphoreInfo(module_semaphore);
    }
  return(module_list != (SplayTreeInfo *) NULL ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L i s t M o d u l e I n f o                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListModuleInfo() lists the module info to a file.
%
%  The format of the ListModuleInfo module is:
%
%      MagickBooleanType ListModuleInfo(FILE *file,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o file:  An pointer to a FILE.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType ListModuleInfo(FILE *file,
  ExceptionInfo *exception)
{
  const ModuleInfo
    **module_info;

  register long
    i;

  unsigned long
    number_modules;

  if (file == (const FILE *) NULL)
    file=stdout;
  module_info=GetModuleInfoList("*",&number_modules,exception);
  if (module_info == (const ModuleInfo **) NULL)
    return(MagickFalse);
  if (module_info[0]->path != (char *) NULL)
    {
      char
        path[MaxTextExtent];

      GetPathComponent(module_info[0]->path,HeadPath,path);
      (void) fprintf(file,"\nPath: %s\n\n",path);
    }
  (void) fprintf(file,"Module\n");
  (void) fprintf(file,"-------------------------------------------------"
    "------------------------------\n");
  for (i=0; i < (long) number_modules; i++)
  {
    if (module_info[i]->stealth != MagickFalse)
      continue;
    (void) fprintf(file,"%s",module_info[i]->tag);
    (void) fprintf(file,"\n");
  }
  (void) fflush(file);
  module_info=(const ModuleInfo **)
    RelinquishMagickMemory((void *) module_info);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   O p e n M o d u l e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OpenModule() loads a module, and invokes its registration module.  It
%  returns MagickTrue on success, and MagickFalse if there is an error.
%
%  The format of the OpenModule module is:
%
%      MagickBooleanType OpenModule(const char *module,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o module: a character string that indicates the module to load.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType OpenModule(const char *module,
  ExceptionInfo *exception)
{
  char
    filename[MaxTextExtent],
    module_name[MaxTextExtent],
    name[MaxTextExtent],
    path[MaxTextExtent];

  MagickBooleanType
    status;

  ModuleHandle
    handle;

  ModuleInfo
    *module_info;

  register const CoderInfo
    *p;

  /*
    Assign module name from alias.
  */
  assert(module != (const char *) NULL);
  module_info=(ModuleInfo *) GetModuleInfo(module,exception);
  if (module_info != (ModuleInfo *) NULL)
    return(MagickTrue);
  if ((exception->severity == OptionWarning) ||
      (exception->severity == ModuleError))
    (void) SetExceptionInfo(exception,UndefinedException);
  (void) CopyMagickString(module_name,module,MaxTextExtent);
  p=GetCoderInfo(module,exception);
  if (p != (CoderInfo *) NULL)
    (void) CopyMagickString(module_name,p->name,MaxTextExtent);
  /*
    Find module file.
  */
  handle=(ModuleHandle) NULL;
  TagToCoderModuleName(module_name,filename);
  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
    "Searching for module \"%s\" using filename \"%s\"",module_name,filename);
  *path='\0';
  status=GetMagickModulePath(filename,MagickCoderModule,path,exception);
  if (status == MagickFalse)
    return(status);
  /*
    Load module
  */
  handle=lt_dlopen(path);
  if (handle == (ModuleHandle) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ModuleError,
        "UnableToLoadModule","`%s': %s",path,lt_dlerror());
      return(MagickFalse);
    }
  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
    "Opening module at path \"%s\"",path);
  /*
    Add module to module module list.
  */
  module_info=(ModuleInfo *) AcquireMagickMemory(sizeof(*module_info));
  if (module_info == (ModuleInfo *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,
      "UnableToAllocateModuleInfo",module_name);
  (void) ResetMagickMemory(module_info,0,sizeof(*module_info));
  module_info->path=ConstantString(AcquireString(path));
  module_info->tag=ConstantString(AcquireString(module_name));
  module_info->signature=MagickSignature;
  module_info->handle=handle;
  (void) time(&module_info->load_time);
  if (RegisterModule(module_info,exception) == (ModuleInfo *) NULL)
    return(MagickFalse);
  /*
    Locate and record RegisterFORMATImage function
  */
  TagToModuleName(module_name,"Register%sImage",name);
  module_info->register_module=(void (*)(void)) lt_dlsym(handle,name);
  if (module_info->register_module == (void (*)(void)) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ModuleError,
        "UnableToRegisterImageFormat","`%s': %s",module_name,lt_dlerror());
      return(MagickFalse);
    }
  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
    "Method \"%s\" in module \"%s\" at address %p",name,module_name,
    (void *) module_info->register_module);
  /*
    Locate and record UnregisterFORMATImage function
  */
  TagToModuleName(module_name,"Unregister%sImage",name);
  module_info->unregister_module=(void (*)(void)) lt_dlsym(handle,name);
  if (module_info->unregister_module == (void (*)(void)) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ModuleError,
        "UnableToRegisterImageFormat","`%s': %s",module_name,lt_dlerror());
      return(MagickFalse);
    }
  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
    "Method \"%s\" in module \"%s\" at address %p",name,module_name,
    (void *) module_info->unregister_module);
  /*
    Execute RegisterFORMATImage module.
  */
  module_info->register_module();
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   O p e n M o d u l e s                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OpenModules() loads all available modules.
%
%  The format of the OpenModules module is:
%
%      MagickBooleanType OpenModules(ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType OpenModules(ExceptionInfo *exception)
{
  char
    **modules;

  register long
    i;

  unsigned long
    number_modules;

  /*
    Load all modules.
  */
  (void) GetMagickInfo((char *) NULL,exception);
  number_modules=0;
  modules=GetModuleList("*",&number_modules,exception);
  if (modules == (char **) NULL)
    return(MagickFalse);
  for (i=0; i < (long) number_modules; i++)
    (void) OpenModule(modules[i],exception);
  /*
    Free resources.
  */
  for (i=0; i < (long) number_modules; i++)
    modules[i]=(char *) RelinquishMagickMemory(modules[i]);
  modules=(char **) RelinquishMagickMemory(modules);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r M o d u l e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterModule() adds an entry to the module list.  It returns a pointer to
%  the registered entry on success.
%
%  The format of the RegisterModule module is:
%
%      ModuleInfo *RegisterModule(const ModuleInfo *module_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o info: a pointer to the registered entry is returned.
%
%    o module_info: a pointer to the ModuleInfo structure to register.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
static const ModuleInfo *RegisterModule(const ModuleInfo *module_info,
  ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  assert(module_info != (ModuleInfo *) NULL);
  assert(module_info->signature == MagickSignature);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",module_info->tag);
  if (module_list == (SplayTreeInfo *) NULL)
    return((const ModuleInfo *) NULL);
  status=AddValueToSplayTree(module_list,
    ConstantString(AcquireString(module_info->tag)),module_info);
  if (status == MagickFalse)
    (void) ThrowMagickException(exception,GetMagickModule(),ResourceLimitError,
      "MemoryAllocationFailed","`%s'",module_info->tag);
  return(module_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  T a g T o C o d e r M o d u l e N a m e                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TagToCoderModuleName() munges a module tag and obtains the filename of the
%  corresponding module module.
%
%  The format of the TagToCoderModuleName module is:
%
%      char *TagToCoderModuleName(const char *tag,char *name)
%
%  A description of each parameter follows:
%
%    o tag: a character string representing the module tag.
%
%    o name: return the module module name here.
%
*/
static void TagToCoderModuleName(const char *tag,char *name)
{
  assert(tag != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",tag);
  assert(name != (char *) NULL);
#if defined(HasLTDL)
  (void) FormatMagickString(name,MaxTextExtent,"%s.la",tag);
  (void) LocaleLower(name);
#else
#if defined(__WINDOWS__)
  if (LocaleNCompare("IM_MOD_",tag,7) == 0)
    (void) CopyMagickString(name,tag,MaxTextExtent);
  else
    {
#if defined(_DEBUG)
      (void) FormatMagickString(name,MaxTextExtent,"IM_MOD_DB_%s_.dll",tag);
#else
      (void) FormatMagickString(name,MaxTextExtent,"IM_MOD_RL_%s_.dll",tag);
#endif
    }
#endif
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  T a g T o F i l t e r M o d u l e N a m e                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TagToFilterModuleName() munges a module tag and returns the filename of the
%  corresponding filter module.
%
%  The format of the TagToFilterModuleName module is:
%
%      void TagToFilterModuleName(const char *tag,char name)
%
%  A description of each parameter follows:
%
%    o tag: a character string representing the module tag.
%
%    o name: return the filter name here.
%
*/
static void TagToFilterModuleName(const char *tag,char *name)
{
  assert(tag != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",tag);
  assert(name != (char *) NULL);
#if !defined(HasLTDL)
  (void) FormatMagickString(name,MaxTextExtent,"%s.dll",tag);
#else
  (void) FormatMagickString(name,MaxTextExtent,"%s.la",tag);
  (void) LocaleLower(name);
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T a g T o M o d u l e N a m e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TagToModuleName() munges the module tag name and returns an upper-case tag
%  name as the input string, and a user-provided format.
%
%  The format of the TagToModuleName module is:
%
%      TagToModuleName(const char *tag,const char *format,char *module)
%
%  A description of each parameter follows:
%
%    o tag: the module tag.
%
%    o format: a sprintf-compatible format string containing %s where the
%      upper-case tag name is to be inserted.
%
%    o module: pointer to a destination buffer for the formatted result.
%
*/
static void TagToModuleName(const char *tag,const char *format,char *module)
{
  char
    name[MaxTextExtent];

  assert(tag != (const char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",tag);
  assert(format != (const char *) NULL);
  assert(module != (char *) NULL);
  (void) CopyMagickString(name,tag,MaxTextExtent);
  LocaleUpper(name);
#if !defined(MagickMethodPrefix)
  (void) FormatMagickString(module,MaxTextExtent,format,name);
#else
  {
    char
      prefix_format[MaxTextExtent];

    (void) FormatMagickString(prefix_format,MaxTextExtent,"%s%s",
      MagickMethodPrefix,format);
    (void) FormatMagickString(module,MaxTextExtent,prefix_format,name);
  }
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n R e g i s t e r M o d u l e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterModule() unloads a module, and invokes its de-registration module.
%  Returns MagickTrue on success, and MagickFalse if there is an error.
%
%  The format of the UnregisterModule module is:
%
%      MagickBooleanType UnregisterModule(const ModuleInfo *module_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o module_info: The module info.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
static MagickBooleanType UnregisterModule(const ModuleInfo *module_info,
  ExceptionInfo *exception)
{
  char
    name[MaxTextExtent];

  /*
    Locate and execute UnregisterFORMATImage module.
  */
  assert(module_info != (const ModuleInfo *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",module_info->tag);
  assert(exception != (ExceptionInfo *) NULL);
  if (module_info->unregister_module == NULL)
    return(MagickTrue);
  module_info->unregister_module();
  if (lt_dlclose((ModuleHandle) module_info->handle) != 0)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),ModuleError,
        "UnableToCloseModule","`%s': %s",name,lt_dlerror());
      return(MagickFalse);
    }
  return(MagickTrue);
}
#else
MagickExport MagickBooleanType ExecuteModuleProcess(const char *tag,
  Image **image,const int argc,char **argv)
{
  MagickBooleanType
    status;

  status=MagickFalse;
#if !defined(BuildMagickModules)
  {
    MagickBooleanType
      AnalyzeImage(Image **,const int,char **),
      (*method)(Image **,const int,char **);

    method=NULL;
    if (LocaleCompare("analyze",tag) == 0)
      method=AnalyzeImage;
    if (method != NULL)
      status=(*method)(image,argc,argv);
  }
#endif
  return(status);
}
#endif
