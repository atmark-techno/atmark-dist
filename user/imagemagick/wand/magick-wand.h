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

  ImageMagick MagickWand API.
*/

#ifndef _WAND_MAGICK_WAND_H
#define _WAND_MAGICK_WAND_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if defined(_VISUALC_)
#  if defined(_MT) && defined(_DLL) && !defined(_LIB)
#    pragma warning( disable: 4273 )
#    if !defined(_WANDLIB_)
#      define WandExport __declspec(dllimport)
#    else
#     define WandExport __declspec(dllexport)
#    endif
#  else
#    define WandExport
#  endif

#  pragma warning(disable : 4018)
#  pragma warning(disable : 4244)
#  pragma warning(disable : 4142)
#else
#  define WandExport
#endif

typedef struct _MagickWand
  MagickWand;

#include "magick/ImageMagick.h"
#include "wand/drawing-wand.h"
#include "wand/magick-attribute.h"
#include "wand/magick-image.h"
#include "wand/pixel-iterator.h"
#include "wand/pixel-wand.h"

extern WandExport MagickBooleanType
  IsMagickWand(const MagickWand *),
  MagickClearException(MagickWand *);

extern WandExport MagickWand
  *CloneMagickWand(const MagickWand *),
  *DestroyMagickWand(MagickWand *),
  *NewMagickWand(void);

extern WandExport void
  ClearMagickWand(MagickWand *),
  MagickWandGenesis(void),
  MagickWandTerminus(void),
  *MagickRelinquishMemory(void *),
  MagickResetIterator(MagickWand *),
  MagickSetFirstIterator(MagickWand *),
  MagickSetLastIterator(MagickWand *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
