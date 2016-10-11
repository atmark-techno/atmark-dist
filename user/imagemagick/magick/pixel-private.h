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

  ImageMagick image color methods.
*/
#ifndef _MAGICK_PIXEL_PRIVATE_H
#define _MAGICK_PIXEL_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <magick/image.h>
#include <magick/color.h>
#include <magick/image-private.h>
#include <magick/exception-private.h>

static inline void SetMagickPixelPacket(const PixelPacket *color,
  const IndexPacket *index,MagickPixelPacket *pixel)
{
  pixel->red=(MagickRealType) color->red;
  pixel->green=(MagickRealType) color->green;
  pixel->blue=(MagickRealType) color->blue;
  pixel->opacity=(MagickRealType) (pixel->matte != MagickFalse ?
    color->opacity : OpaqueOpacity);
  pixel->index=(MagickRealType) ((pixel->colorspace == CMYKColorspace) &&
    (index != (const IndexPacket *) NULL) ? *index : 0);
}

static inline void SetPixelPacket(const MagickPixelPacket *pixel,
  PixelPacket *color,IndexPacket *index)
{
  color->red=RoundToQuantum(pixel->red);
  color->green=RoundToQuantum(pixel->green);
  color->blue=RoundToQuantum(pixel->blue);
  color->opacity=OpaqueOpacity;
  if (pixel->matte != MagickFalse)
    color->opacity=RoundToQuantum(pixel->opacity);
  if ((pixel->colorspace == CMYKColorspace) && (index != (IndexPacket *) NULL))
    *index=RoundToQuantum(pixel->index);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
