/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%           RRRR    EEEEE    GGG   IIIII  SSSSS  TTTTT  RRRR   Y   Y          %
%           R   R   E       G        I    SS       T    R   R   Y Y           %
%           RRRR    EEE     G GGG    I     SSS     T    RRRR     Y            %
%           R R     E       G   G    I       SS    T    R R      Y            %
%           R  R    EEEEE    GGG   IIIII  SSSSS    T    R  R     Y            %
%                                                                             %
%                                                                             %
%                            ImageMagick Registry.                            %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                October 2001                                 %
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
#include "magick/image.h"
#include "magick/list.h"
#include "magick/memory_.h"
#include "magick/registry.h"
#include "magick/semaphore.h"
#include "magick/splay-tree.h"
#include "magick/string_.h"
/*
  Typedef declaractions.
*/
typedef struct _RegistryInfo
{
  long
    id;

  RegistryType
    type;

  void
    *blob;

  size_t
    length;

  unsigned long
    signature;
} RegistryInfo;

/*
  Global declarations.
*/
static long
  id = 0;

static SemaphoreInfo
  *registry_semaphore = (SemaphoreInfo *) NULL;

static SplayTreeInfo
  *registry_list = (SplayTreeInfo *) NULL;

/*
  Forward declarations.
*/
static void
  *DestroyRegistryNode(void *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e l e t e M a g i c k R e g i s t r y                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DeleteMagickRegistry() deletes an entry in the registry as defined by the id.
%  It returns MagickTrue if the entry is deleted otherwise MagickFalse if no
%  entry is found in the registry that matches the id.
%
%  The format of the DeleteMagickRegistry method is:
%
%      MagickBooleanType DeleteMagickRegistry(const long id)
%
%  A description of each parameter follows:
%
%    o id: The registry id.
%
%
*/
MagickExport MagickBooleanType DeleteMagickRegistry(const long id)
{
  register const RegistryInfo
    *p;

  if (registry_list == (SplayTreeInfo *) NULL)
    return(MagickFalse);
  if (GetNumberOfNodesInSplayTree(registry_list) == 0)
    return(MagickFalse);
  AcquireSemaphoreInfo(&registry_semaphore);
  ResetSplayTreeIterator(registry_list);
  p=(const RegistryInfo *) GetNextValueInSplayTree(registry_list);
  while (p != (const RegistryInfo *) NULL)
  {
    if (p->id == id)
      break;
    p=(const RegistryInfo *) GetNextValueInSplayTree(registry_list);
  }
  RelinquishSemaphoreInfo(registry_semaphore);
  return(RemoveNodeByValueFromSplayTree(registry_list,p));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y M a g i c k R e g i s t r y                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyMagickRegistry() deallocates memory associated the magick registry.
%
%  The format of the DestroyMagickRegistry method is:
%
%       void DestroyMagickRegistry(void)
%
%
*/
MagickExport void DestroyMagickRegistry(void)
{
  AcquireSemaphoreInfo(&registry_semaphore);
  if (registry_list != (SplayTreeInfo *) NULL)
    registry_list=DestroySplayTree(registry_list);
  RelinquishSemaphoreInfo(registry_semaphore);
  registry_semaphore=DestroySemaphoreInfo(registry_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e F r o m M a g i c k R e g i s t y                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageFromMagickRegistry() gets an image from the registry as defined by
%  its name.  If the blob that matches the name is not found, NULL is returned.
%
%  The format of the GetImageFromMagickRegistry method is:
%
%      Image *GetImageFromMagickRegistry(const char *name,long *id,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o name: The name of the image to retrieve from the registry.
%
%    o id: The registry id.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *GetImageFromMagickRegistry(const char *name,long *id,
  ExceptionInfo *exception)
{
  Image
    *image;

  register const RegistryInfo
    *p;

  if ((registry_list == (SplayTreeInfo *) NULL) ||
      (GetNumberOfNodesInSplayTree(registry_list) == 0))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),RegistryError,
        "UnableToLocateRegistryImage","`%s'",name);
      return((Image *) NULL);
    }
  *id=(-1);
  image=NewImageList();
  AcquireSemaphoreInfo(&registry_semaphore);
  ResetSplayTreeIterator(registry_list);
  p=(const RegistryInfo *) GetNextValueInSplayTree(registry_list);
  while (p != (const RegistryInfo *) NULL)
  {
    if ((p->type == ImageRegistryType) &&
        (LocaleCompare(((Image *) p->blob)->filename,name) == 0))
      {
        *id=p->id;
        image=CloneImageList((Image *) p->blob,exception);
        break;
      }
    p=(const RegistryInfo *) GetNextValueInSplayTree(registry_list);
  }
  RelinquishSemaphoreInfo(registry_semaphore);
  if (image == (Image *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),RegistryError,
      "UnableToLocateRegistryImage","`%s'",name);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k R e g i s t r y                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickRegistry() gets a blob from the registry as defined by the id.  If
%  the blob that matches the id is not found, NULL is returned.
%
%  The format of the GetMagickRegistry method is:
%
%      const void *GetMagickRegistry(const long id,RegistryType *type,
%        size_t *length,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o id: The registry id.
%
%    o type: The registry type.
%
%    o length: The blob length in number of bytes.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport void *GetMagickRegistry(const long id,RegistryType *type,
  size_t *length,ExceptionInfo *exception)
{
  register const RegistryInfo
    *p;

  void
    *blob;

  assert(type != (RegistryType *) NULL);
  assert(length != (size_t *) NULL);
  assert(exception != (ExceptionInfo *) NULL);
  if ((registry_list == (SplayTreeInfo *) NULL) ||
      (GetNumberOfNodesInSplayTree(registry_list) == 0))
    {
      char
        reason[MaxTextExtent];

      (void) FormatMagickString(reason,MaxTextExtent,"id=%ld",id);
      (void) ThrowMagickException(exception,GetMagickModule(),RegistryError,
        "UnableToGetRegistryID","`%s'",reason);
      return((Image *) NULL);
    }
  blob=(void *) NULL;
  *type=UndefinedRegistryType;
  *length=0;
  AcquireSemaphoreInfo(&registry_semaphore);
  ResetSplayTreeIterator(registry_list);
  p=(const RegistryInfo *) GetNextValueInSplayTree(registry_list);
  while (p != (const RegistryInfo *) NULL)
  {
    if (id == p->id)
      break;
    p=(const RegistryInfo *) GetNextValueInSplayTree(registry_list);
  }
  if (p != (const RegistryInfo *) NULL)
    {
      switch (p->type)
      {
        case ImageRegistryType:
        {
          Image
            *image;

          image=(Image *) p->blob;
          blob=(void *) CloneImageList(image,exception);
          break;
        }
        case ImageInfoRegistryType:
        {
          ImageInfo
            *image_info;

          image_info=(ImageInfo *) p->blob;
          blob=(void *) CloneImageInfo(image_info);
          break;
        }
        default:
        {
          blob=(void *) AcquireMagickMemory(p->length);
          if (blob == (void *) NULL)
            {
              (void) ThrowMagickException(exception,GetMagickModule(),
                ResourceLimitError,"MemoryAllocationFailed","`%s'",
                strerror(errno));
              break;
            }
          (void) CopyMagickMemory(blob,p->blob,p->length);
          break;
        }
      }
      *type=p->type;
      *length=p->length;
    }
  RelinquishSemaphoreInfo(registry_semaphore);
  if (blob == (void *) NULL)
    {
      char
        reason[MaxTextExtent];

      (void) FormatMagickString(reason,MaxTextExtent,"id=%ld",id);
      (void) ThrowMagickException(exception,GetMagickModule(),RegistryError,
        "UnableToGetRegistryID","`%s'",reason);
    }
  return(blob);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t M a g i c k R e g i s t r y                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetMagickRegistry() sets a blob into the registry and returns a unique ID.
%  If an error occurs, -1 is returned.
%
%  The format of the SetMagickRegistry method is:
%
%      long SetMagickRegistry(const RegistryType type,const void *blob,
%        const size_t length,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o type: The registry type.
%
%    o blob: The address of a Binary Large OBject.
%
%    o length: For a registry type of ImageRegistryType use sizeof(Image)
%      otherise the blob length in number of bytes.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

static void *DestroyRegistryNode(void *registry_info)
{
  register RegistryInfo
    *p;
                                                                                
  p=(RegistryInfo *) registry_info;
  switch (p->type)
  {
    case ImageRegistryType:
    {
      p->blob=DestroyImage((Image *) p->blob);
      break;
    }
    case ImageInfoRegistryType:
    {
      p->blob=DestroyImageInfo((ImageInfo *) p->blob);
      break;
    }
    default:
    {
      p->blob=(char *) RelinquishMagickMemory(p->blob);
      break;
    }
  }
  return(RelinquishMagickMemory(p));
}

MagickExport long SetMagickRegistry(const RegistryType type,const void *blob,
  const size_t length,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  RegistryInfo
    *registry_info;

  void
    *clone_blob;

  switch (type)
  {
    case ImageRegistryType:
    {
      Image
        *image;

      image=(Image *) blob;
      if (length != sizeof(Image))
        {
          (void) ThrowMagickException(exception,GetMagickModule(),RegistryError,
            "UnableToSetRegistry","`%s'",strerror(errno));
          return(-1);
        }
      if (image->signature != MagickSignature)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),RegistryError,
            "UnableToSetRegistry","`%s'",strerror(errno));
          return(-1);
        }
      clone_blob=(void *) CloneImageList(image,exception);
      if (clone_blob == (void *) NULL)
        return(-1);
      break;
    }
    case ImageInfoRegistryType:
    {
      ImageInfo
        *image_info;

      image_info=(ImageInfo *) blob;
      if (length != sizeof(ImageInfo))
        {
          (void) ThrowMagickException(exception,GetMagickModule(),RegistryError,
            "UnableToSetRegistry","`%s'",strerror(errno));
          return(-1);
        }
      if (image_info->signature != MagickSignature)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),RegistryError,
            "UnableToSetRegistry","`%s'",strerror(errno));
          return(-1);
        }
      clone_blob=(void *) CloneImageInfo(image_info);
      if (clone_blob == (void *) NULL)
        return(-1);
      break;
    }
    default:
    {
      clone_blob=(void *) AcquireMagickMemory(length);
      if (clone_blob == (void *) NULL)
        return(-1);
      (void) CopyMagickMemory(clone_blob,blob,length);
    }
  }
  registry_info=(RegistryInfo *) AcquireMagickMemory(sizeof(*registry_info));
  if (registry_info == (RegistryInfo *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,
      "UnableToAllocateRegistryInfo",strerror(errno));
  (void) ResetMagickMemory(registry_info,0,sizeof(*registry_info));
  registry_info->type=type;
  registry_info->blob=clone_blob;
  registry_info->length=length;
  registry_info->signature=MagickSignature;
  AcquireSemaphoreInfo(&registry_semaphore);
  registry_info->id=id++;
  if (registry_list == (SplayTreeInfo *) NULL)
    {
      registry_list=NewSplayTree((int (*)(const void *,const void *)) NULL,
        (void *(*)(void *)) NULL,DestroyRegistryNode);
      if (registry_list == (SplayTreeInfo *) NULL)
        ThrowMagickFatalException(ResourceLimitFatalError,
          "UnableToAllocateRegistryInfo",strerror(errno));
    }
  status=AddValueToSplayTree(registry_list,(const void *) id,registry_info);
  if (status == MagickFalse)
    (void) ThrowMagickException(exception,GetMagickModule(),ResourceLimitError,
      "MemoryAllocationFailed","`%s'",strerror(errno));
  RelinquishSemaphoreInfo(registry_semaphore);
  return(registry_info->id);
}
