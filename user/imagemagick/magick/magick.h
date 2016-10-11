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

  ImageMagick application programming interface declarations.
*/
#ifndef _MAGICK_MAGICK_H
#define _MAGICK_MAGICK_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef Image
  *DecoderHandler(const ImageInfo *,ExceptionInfo *);

typedef MagickBooleanType
  EncoderHandler(const ImageInfo *,Image *);

typedef MagickBooleanType
  MagickHandler(const unsigned char *,const size_t);

typedef struct _MagickInfo
{
  char
    *name,
    *description,
    *version,
    *note,
    *module;

  ImageInfo
    *image_info;

  DecoderHandler
    *decoder;

  EncoderHandler
    *encoder;

  MagickHandler
    *magick;

  void
    *client_data;

  MagickBooleanType
    adjoin,
    raw,
    endian_support,
    blob_support,
    seekable_stream,
    thread_support,
    stealth;

  struct _MagickInfo
    *previous,
    *next;  /* deprecated, use GetMagickInfoList() */

  unsigned long
    signature;
} MagickInfo;

extern MagickExport char
  **GetMagickList(const char *,unsigned long *,ExceptionInfo *),
  *MagickToMime(const char *);

extern MagickExport const char
  *GetImageMagick(const unsigned char *,const size_t),
  *GetMagickDescription(const MagickInfo *);

extern MagickExport DecoderHandler
  *GetMagickDecoder(const MagickInfo *);

extern MagickExport EncoderHandler
  *GetMagickEncoder(const MagickInfo *);

extern MagickExport MagickBooleanType
  GetMagickAdjoin(const MagickInfo *),
  GetMagickBlobSupport(const MagickInfo *),
  GetMagickEndianSupport(const MagickInfo *),
  GetMagickSeekableStream(const MagickInfo *),
  GetMagickThreadSupport(const MagickInfo *),
  IsMagickInstantiated(void),
  UnregisterMagickInfo(const char *);

extern const MagickExport MagickInfo
  *GetMagickInfo(const char *,ExceptionInfo *),
  **GetMagickInfoList(const char *,unsigned long *,ExceptionInfo *);

extern MagickExport MagickInfo
  *RegisterMagickInfo(MagickInfo *),
  *SetMagickInfo(const char *);

extern MagickExport void
  DestroyMagick(void),
  DestroyMagickList(void),
  InitializeMagick(const char *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
