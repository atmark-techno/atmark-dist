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

  ImageMagick Cache public methods.
*/
#ifndef _MAGICK_CACHE_H
#define _MAGICK_CACHE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include "magick/blob.h"

typedef enum
{
  UndefinedVirtualPixelMethod,
  BackgroundVirtualPixelMethod,
  ConstantVirtualPixelMethod,  /* deprecated */
  EdgeVirtualPixelMethod,
  MirrorVirtualPixelMethod,
  TileVirtualPixelMethod,
  TransparentVirtualPixelMethod
} VirtualPixelMethod;

extern MagickExport const PixelPacket
  *AcquireCacheNexus(const Image *,const long,const long,const unsigned long,
    const unsigned long,const unsigned long,ExceptionInfo *);

extern MagickExport MagickSizeType
  GetPixelCacheArea(const Image *);

extern MagickExport MagickBooleanType
  PersistCache(Image *,const char *,const MagickBooleanType,MagickOffsetType *,
    ExceptionInfo *),
  SetImageVirtualPixelMethod(const Image *,const VirtualPixelMethod),
  SyncCache(Image *),
  SyncCacheNexus(Image *,const unsigned long);

extern MagickExport PixelPacket
  *GetCacheNexus(Image *,const long,const long,const unsigned long,
    const unsigned long,const unsigned long),
  *SetCacheNexus(Image *,const long,const long,const unsigned long,
    const unsigned long,const unsigned long);

extern MagickExport VirtualPixelMethod
  GetImageVirtualPixelMethod(const Image *);

extern MagickExport void
  RelinquishCacheResources(const Image *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
