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

  Methods to Get/Set/Destroy Image Text Attributes.
*/
#ifndef _MAGICK_ATTRIBUTE_H
#define _MAGICK_ATTRIBUTE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <stdarg.h>

typedef struct _ImageAttribute
{
  char
    *key,
    *value;
                                                                                
  MagickBooleanType
    compression;
                                                                                
  struct _ImageAttribute
    *previous,
    *next;  /* deprecated */
} ImageAttribute;

extern MagickExport const ImageAttribute
  *GetImageAttribute(const Image *,const char *),
  *GetImageClippingPathAttribute(Image *),
  *GetImageInfoAttribute(const ImageInfo *,const Image *,const char *),
  *GetNextImageAttribute(const Image *);

extern MagickExport MagickBooleanType
  CloneImageAttributes(Image *,const Image *),
  DeleteImageAttribute(Image *,const char *),
  FormatImageAttribute(Image *,const char *,const char *,...)
    magick_attribute((format (printf,3,4))),
  FormatImageAttributeList(Image *,const char *,const char *,va_list)
    magick_attribute((format (printf,3,0))),
  SetImageAttribute(Image *,const char *,const char *);

extern MagickExport void
  DestroyImageAttributes(Image *),
  ResetImageAttributeIterator(const Image *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
