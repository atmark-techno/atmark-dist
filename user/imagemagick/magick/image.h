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

  ImageMagick image methods.
*/
#ifndef _MAGICK_IMAGE_H
#define _MAGICK_IMAGE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <magick/color.h>

#define OpaqueOpacity  ((Quantum) 0UL)
#define TransparentOpacity  QuantumRange

typedef enum
{
  UndefinedTransmitType,
  FileTransmitType,
  BlobTransmitType,
  StreamTransmitType,
  ImageTransmitType
} TransmitType;

extern MagickExport const PixelPacket
  *AcquireImagePixels(const Image *,const long,const long,const unsigned long,
    const unsigned long,ExceptionInfo *);

extern MagickExport ExceptionType
  CatchImageException(Image *);

extern MagickExport Image
  *AllocateImage(const ImageInfo *),
  *AppendImages(const Image *,const MagickBooleanType,ExceptionInfo *),
  *AverageImages(const Image *,ExceptionInfo *),
  *CloneImage(const Image *,const unsigned long,const unsigned long,
    const MagickBooleanType,ExceptionInfo *),
  *CloneImages(const Image *,const char *,ExceptionInfo *),
  *CombineImages(const Image *,const ChannelType,ExceptionInfo *),
  *DestroyImage(Image *),
  *GetImageClipMask(const Image *,ExceptionInfo *),
  *NewMagickImage(const ImageInfo *,const unsigned long,const unsigned long,
    const MagickPixelPacket *),
  *ReferenceImage(Image *);

extern MagickExport ImageInfo
  *CloneImageInfo(const ImageInfo *),
  *DestroyImageInfo(ImageInfo *);

extern MagickExport ImageType
  GetImageType(const Image *,ExceptionInfo *);

extern MagickExport IndexPacket
  *GetIndexes(const Image *);

extern MagickExport MagickBooleanType
  AllocateImageColormap(Image *,const unsigned long),
  ClipImage(Image *),
  ClipPathImage(Image *,const char *,const MagickBooleanType),
  CycleColormapImage(Image *,const long),
  GradientImage(Image *,const PixelPacket *,const PixelPacket *),
  IsTaintImage(const Image *),
  IsMagickConflict(const char *),
  ListMagickInfo(FILE *,ExceptionInfo *),
  PlasmaImage(Image *,const SegmentInfo *,unsigned long,unsigned long),
  SeparateImageChannel(Image *,const ChannelType),
  SetImageClipMask(Image *,const Image *),
  SetImageExtent(Image *,const unsigned long,const unsigned long),
  SetImageInfo(ImageInfo *,const MagickBooleanType,ExceptionInfo *),
  SetImageType(Image *,const ImageType),
  SortColormapByIntensity(Image *),
  StripImage(Image *),
  SyncImage(Image *),
  SyncImagePixels(Image *),
  TextureImage(Image *,const Image *);

extern MagickExport PixelPacket
  AcquireOnePixel(const Image *,const long,const long,ExceptionInfo *),
  *GetImagePixels(Image *,const long,const long,const unsigned long,
    const unsigned long),
  GetOnePixel(Image *,const long,const long),
  *GetPixels(const Image *),
  *SetImagePixels(Image *,const long,const long,const unsigned long,
    const unsigned long);

extern MagickExport void
  AllocateNextImage(const ImageInfo *,Image *),
  DestroyImagePixels(Image *),
  GetImageException(Image *,ExceptionInfo *),
  GetImageInfo(ImageInfo *),
  ModifyImage(Image **,ExceptionInfo *),
  RelinquishImageResources(const Image *),
  SetImageBackgroundColor(Image *),
  SetImageInfoBlob(ImageInfo *,const void *,const size_t),
  SetImageInfoFile(ImageInfo *,FILE *),
  SetImageOpacity(Image *,const Quantum);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
