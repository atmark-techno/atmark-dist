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

  ImageMagick private image drawing methods.
*/
#ifndef _MAGICK_DRAW_PRIVATE_H
#define _MAGICK_DRAW_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include "magick/image.h"
#include "magick/memory_.h"

static inline PixelPacket GetFillColor(const DrawInfo *draw_info,
  const long x,const long y)
{
  Image
    *pattern;

  PixelPacket
    fill_color;

  pattern=draw_info->fill_pattern;
  if (pattern == (Image *) NULL)
    return(draw_info->fill);
  fill_color=AcquireOnePixel(pattern,
    (x-pattern->extract_info.x) % pattern->columns,
    (y-pattern->extract_info.y) % pattern->rows,&pattern->exception);
  if (pattern->matte == MagickFalse)
    fill_color.opacity=OpaqueOpacity;
  return(fill_color);
}

static inline PixelPacket GetStrokeColor(const DrawInfo *draw_info,
  const long x,const long y)
{
  Image
    *pattern;

  PixelPacket
    stroke_color;

  pattern=draw_info->stroke_pattern;
  if (pattern == (Image *) NULL)
    return(draw_info->stroke);
  stroke_color=AcquireOnePixel(pattern,
    (x-pattern->extract_info.x) % pattern->columns,
    (y-pattern->extract_info.y) % pattern->rows,&pattern->exception);
  return(stroke_color);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
