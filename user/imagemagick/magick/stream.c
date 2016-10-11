/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  SSSSS  TTTTT  RRRR   EEEEE   AAA   M   M                   %
%                  SS       T    R   R  E      A   A  MM MM                   %
%                   SSS     T    RRRR   EEE    AAAAA  M M M                   %
%                     SS    T    R R    E      A   A  M   M                   %
%                  SSSSS    T    R  R   EEEEE  A   A  M   M                   %
%                                                                             %
%                                                                             %
%                      ImageMagick Pixel Stream Methods                       %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 March 2000                                  %
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
#include "magick/blob-private.h"
#include "magick/cache.h"
#include "magick/cache-private.h"
#include "magick/constitute.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/memory_.h"
#include "magick/semaphore.h"
#include "magick/string_.h"

/*
  Typedef declaractions.
*/
typedef CacheInfo
  StreamInfo;

/*
  Declare pixel cache interfaces.
*/
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static const PixelPacket
  *AcquirePixelStream(const Image *,const long,const long,const unsigned long,
    const unsigned long,ExceptionInfo *);

static IndexPacket
  *GetIndexesFromStream(const Image *);

static PixelPacket
  AcquireOnePixelFromStream(const Image *,const long,const long,
    ExceptionInfo *),
  GetOnePixelFromStream(Image *,const long,const long),
  *GetPixelStream(Image *,const long,const long,const unsigned long,
    const unsigned long),
  *GetPixelsFromStream(const Image *),
  *SetPixelStream(Image *,const long,const long,const unsigned long,
    const unsigned long);

static MagickBooleanType
  SyncPixelStream(Image *);

static void
  DestroyPixelStream(Image *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   A c q u i r e O n e P i x e l F r o m S t r e a m                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireOnePixelFromStream() returns a single pixel at the specified (x.y)
%  location.  The image background color is returned if an error occurs.
%
%  The format of the AcquireOnePixelFromStream() method is:
%
%      PixelPacket *AcquireOnePixelFromStream(const Image image,const long x,
%        const long y,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pixels: Method GetOnePixelFromStream returns a pixel at the specified
%      (x,y) location.
%
%    o image: The image.
%
%    o x,y:  These values define the location of the pixel to return.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
static PixelPacket AcquireOnePixelFromStream(const Image *image,const long x,
  const long y,ExceptionInfo *exception)
{
  register const PixelPacket
    *pixel;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  pixel=AcquirePixelStream(image,x,y,1,1,exception);
  if (pixel != (PixelPacket *) NULL)
    return(*pixel);
  return(image->background_color);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   A c q u i r e P i x e l S t r e a m                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquirePixelStream() gets pixels from the in-memory or disk pixel cache as
%  defined by the geometry parameters.   A pointer to the pixels is returned if
%  the pixels are transferred, otherwise a NULL is returned.  For streams this
%  method is a no-op.
%
%  The format of the AcquirePixelStream() method is:
%
%      const PixelPacket *AcquirePixelStream(const Image *image,const long x,
%        const long y,const unsigned long columns,const unsigned long rows,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
static const PixelPacket *AcquirePixelStream(const Image *image,const long x,
  const long y,const unsigned long columns,const unsigned long rows,
  ExceptionInfo *exception)
{
  MagickSizeType
    number_pixels;

  size_t
    length;

  StreamInfo
    *stream_info;

  /*
    Validate pixel cache geometry.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((x < 0) || (y < 0) || ((x+(long) columns) > (long) image->columns) ||
      ((y+(long) rows) > (long) image->rows) || (columns == 0) || (rows == 0))
    {
      (void) ThrowMagickException(exception,GetMagickModule(),StreamError,
        "ImageDoesNotContainTheStreamGeometry","`%s'",image->filename);
      return((PixelPacket *) NULL);
    }
  stream_info=(StreamInfo *) image->cache;
  assert(stream_info->signature == MagickSignature);
  if (stream_info->type == UndefinedCache)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),StreamError,
        "PixelCacheIsNotOpen","`%s'",image->filename);
      return((PixelPacket *) NULL);
    }
  /*
    Pixels are stored in a temporary buffer until they are synced to the cache.
  */
  number_pixels=(MagickSizeType) columns*rows;
  length=(size_t) number_pixels*sizeof(PixelPacket);
  if ((image->storage_class == PseudoClass) ||
      (image->colorspace == CMYKColorspace))
    length+=number_pixels*sizeof(IndexPacket);
  if (stream_info->pixels == (PixelPacket *) NULL)
    stream_info->pixels=(PixelPacket *) AcquireMagickMemory(length);
  else
    if ((MagickSizeType) length != stream_info->length)
      stream_info->pixels=(PixelPacket *)
        ResizeMagickMemory(stream_info->pixels,length);
  if (stream_info->pixels == (void *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,
      "UnableToAllocateCacheInfo",image->filename);
  stream_info->length=(MagickSizeType) length;
  stream_info->indexes=(IndexPacket *) NULL;
  if ((image->storage_class == PseudoClass) ||
      (image->colorspace == CMYKColorspace))
    stream_info->indexes=(IndexPacket *) (stream_info->pixels+number_pixels);
  return(stream_info->pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y P i x e l S t r e a m                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyPixelStream() deallocates memory associated with the pixel stream.
%
%  The format of the DestroyPixelStream() method is:
%
%      void DestroyPixelStream(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%
*/
static void DestroyPixelStream(Image *image)
{
  StreamInfo
    *stream_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  stream_info=(StreamInfo *) image->cache;
  assert(stream_info->signature == MagickSignature);
  AcquireSemaphoreInfo(&stream_info->semaphore);
  stream_info->reference_count--;
  if (stream_info->reference_count > 0)
    {
      RelinquishSemaphoreInfo(stream_info->semaphore);
      return;
    }
  if (stream_info->pixels != (PixelPacket *) NULL)
    stream_info->pixels=(PixelPacket *)
      RelinquishMagickMemory(stream_info->pixels);
  RelinquishSemaphoreInfo(stream_info->semaphore);
  stream_info->semaphore=DestroySemaphoreInfo(stream_info->semaphore);
  stream_info=(StreamInfo *) RelinquishMagickMemory(stream_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t I n d e x e s F r o m S t r e a m                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetIndexesFromStream() returns the indexes associated with the last call to
%  SetPixelStream() or GetPixelStream().
%
%  The format of the GetIndexesFromStream() method is:
%
%      IndexPacket *GetIndexesFromStream(const Image *image)
%
%  A description of each parameter follows:
%
%    o indexes: Method GetIndexesFromStream() returns the indexes associated
%      with the last call to SetPixelStream() or GetPixelStream().
%
%    o image: The image.
%
%
*/
static IndexPacket *GetIndexesFromStream(const Image *image)
{
  StreamInfo
    *stream_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  stream_info=(StreamInfo *) image->cache;
  assert(stream_info->signature == MagickSignature);
  return(stream_info->indexes);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t O n e P i x e l F r o m S t r e a m                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOnePixelFromStream() returns a single pixel at the specified (x,y)
%  location.  The image background color is returned if an error occurs.
%
%  The format of the GetOnePixelFromStream() method is:
%
%      PixelPacket *GetOnePixelFromStream(const Image image,const long x,
%        const long y)
%
%  A description of each parameter follows:
%
%    o pixels: Method GetOnePixelFromStream returns a pixel at the specified
%      (x,y) location.
%
%    o image: The image.
%
%    o x,y:  These values define the location of the pixel to return.
%
*/
static PixelPacket GetOnePixelFromStream(Image *image,const long x,const long y)
{
  register PixelPacket
    *pixel;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  pixel=GetPixelStream(image,x,y,1,1);
  if (pixel != (PixelPacket *) NULL)
    return(*pixel);
  return(image->background_color);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t P i x e l S t r e a m                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPixelStream() gets pixels from the in-memory or disk pixel cache as
%  defined by the geometry parameters.   A pointer to the pixels is returned if
%  the pixels are transferred, otherwise a NULL is returned.  For streams
%  this method is a no-op.
%
%  The format of the GetPixelStream() method is:
%
%      PixelPacket *GetPixelStream(Image *image,const long x,const long y,
%        const unsigned long columns,const unsigned long rows)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
%
*/
static PixelPacket *GetPixelStream(Image *image,const long x,const long y,
  const unsigned long columns,const unsigned long rows)
{
  PixelPacket
    *pixels;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  pixels=SetPixelStream(image,x,y,columns,rows);
  return(pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t P i x e l F r o m S t e a m                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPixelsFromStream() returns the pixels associated with the last call to
%  SetPixelStream() or GetPixelStream().
%
%  The format of the GetPixelsFromStream() method is:
%
%      PixelPacket *GetPixelsFromStream(const Image image)
%
%  A description of each parameter follows:
%
%    o pixels: Method GetPixelsFromStream returns the pixels associated with
%      the last call to SetPixelStream() or GetPixelStream().
%
%    o image: The image.
%
%
*/
static PixelPacket *GetPixelsFromStream(const Image *image)
{
  StreamInfo
    *stream_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  stream_info=(StreamInfo *) image->cache;
  assert(stream_info->signature == MagickSignature);
  return(stream_info->pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d S t r e a m                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadStream() makes the image pixels available to a user supplied
%  callback method immediately upon reading a scanline with the ReadImage()
%  method.
%
%  The format of the ReadStream() method is:
%
%      Image *ReadStream(const ImageInfo *image_info,StreamHandler stream,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o stream: a callback method.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *ReadStream(const ImageInfo *image_info,StreamHandler stream,
  ExceptionInfo *exception)
{
  CacheMethods
    cache_methods;

  Image
    *image;

  ImageInfo
    *read_info;

  /*
    Stream image pixels.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  read_info=CloneImageInfo(image_info);
  GetCacheInfo(&read_info->cache);
  GetCacheMethods(&cache_methods);
  cache_methods.acquire_pixel_handler=AcquirePixelStream;
  cache_methods.get_pixel_handler=GetPixelStream;
  cache_methods.set_pixel_handler=SetPixelStream;
  cache_methods.sync_pixel_handler=SyncPixelStream;
  cache_methods.get_pixels_from_handler=GetPixelsFromStream;
  cache_methods.get_indexes_from_handler=GetIndexesFromStream;
  cache_methods.acquire_one_pixel_from_handler=AcquireOnePixelFromStream;
  cache_methods.get_one_pixel_from_handler=GetOnePixelFromStream;
  cache_methods.destroy_pixel_handler=DestroyPixelStream;
  SetPixelCacheMethods(read_info->cache,&cache_methods);
  read_info->stream=stream;
  image=ReadImage(read_info,exception);
  read_info=DestroyImageInfo(read_info);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S e t P i x e l S t r e a m                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetPixelStream() allocates an area to store image pixels as defined by the
%  region rectangle and returns a pointer to the area.  This area is
%  subsequently transferred from the pixel cache with method SyncPixelStream().
%  A pointer to the pixels is returned if the pixels are transferred,
%  otherwise a NULL is returned.
%
%  The format of the SetPixelStream() method is:
%
%      PixelPacket *SetPixelStream(Image *image,const long x,const long y,
%        const unsigned long columns,const unsigned long rows)
%
%  A description of each parameter follows:
%
%    o pixels: Method SetPixelStream returns a pointer to the pixels is
%      returned if the pixels are transferred, otherwise a NULL is returned.
%
%    o image: The image.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
%
*/
static PixelPacket *SetPixelStream(Image *image,const long x,const long y,
  const unsigned long columns,const unsigned long rows)
{
  MagickSizeType
    number_pixels;

  size_t
    length;

  StreamHandler
    stream_handler;

  StreamInfo
    *stream_info;

  /*
    Validate pixel cache geometry.
  */
  assert(image != (Image *) NULL);
  if ((x < 0) || (y < 0) || ((x+(long) columns) > (long) image->columns) ||
      ((y+(long) rows) > (long) image->rows) || (columns == 0) || (rows == 0))
    {
      (void) ThrowMagickException(&image->exception,GetMagickModule(),
        StreamError,"ImageDoesNotContainTheStreamGeometry","`%s'",
        image->filename);
      return((PixelPacket *) NULL);
    }
  stream_handler=GetBlobStreamHandler(image);
  if (stream_handler == (StreamHandler) NULL)
    {
      (void) ThrowMagickException(&image->exception,GetMagickModule(),
        StreamError,"NoStreamHandlerIsDefined","`%s'",image->filename);
      return((PixelPacket *) NULL);
    }
  stream_info=(StreamInfo *) image->cache;
  assert(stream_info->signature == MagickSignature);
  if ((image->storage_class != GetCacheClass(image->cache)) ||
      (image->colorspace != GetCacheColorspace(image->cache)))
    {
      if (GetCacheClass(image->cache) == UndefinedClass)
        (void) stream_handler(image,(const void *) NULL,
          (size_t) stream_info->columns);
      stream_info->storage_class=image->storage_class;
      stream_info->colorspace=image->colorspace;
      stream_info->columns=image->columns;
      stream_info->rows=image->rows;
      image->cache=stream_info;
    }
  /*
    Pixels are stored in a temporary buffer until they are synced to the cache.
  */
  stream_info->columns=columns;
  stream_info->rows=rows;
  number_pixels=(MagickSizeType) columns*rows;
  length=(size_t) number_pixels*sizeof(PixelPacket);
  if ((image->storage_class == PseudoClass) ||
      (image->colorspace == CMYKColorspace))
    length+=number_pixels*sizeof(IndexPacket);
  if (stream_info->pixels == (PixelPacket *) NULL)
    {
      stream_info->pixels=(PixelPacket *) AcquireMagickMemory(length);
      stream_info->length=(MagickSizeType) length;
    }
  else
    if (stream_info->length < (MagickSizeType) length)
      {
        stream_info->pixels=(PixelPacket *)
          ResizeMagickMemory(stream_info->pixels,length);
        stream_info->length=(MagickSizeType) length;
      }
  if (stream_info->pixels == (void *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,
      "UnableToAllocateImagePixels",image->filename);
  stream_info->indexes=(IndexPacket *) NULL;
  if ((image->storage_class == PseudoClass) ||
      (image->colorspace == CMYKColorspace))
    stream_info->indexes=(IndexPacket *) (stream_info->pixels+number_pixels);
  return(stream_info->pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S y n c P i x e l S t r e a m                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncPixelStream() calls the user supplied callback method with the latest
%  stream of pixels.
%
%  The format of the SyncPixelStream method is:
%
%      MagickBooleanType SyncPixelStream(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%
*/
static MagickBooleanType SyncPixelStream(Image *image)
{
  MagickBooleanType
    status;

  StreamHandler
    stream_handler;

  StreamInfo
    *stream_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  stream_info=(StreamInfo *) image->cache;
  assert(stream_info->signature == MagickSignature);
  stream_handler=GetBlobStreamHandler(image);
  if (stream_handler == (StreamHandler) NULL)
    {
      (void) ThrowMagickException(&image->exception,GetMagickModule(),
        StreamError,"NoStreamHandlerIsDefined","`%s'",image->filename);
      return(MagickFalse);
    }
  status=stream_handler(image,stream_info->pixels,(size_t)
    stream_info->columns);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e S t r e a m                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteStream() makes the image pixels available to a user supplied
%  callback method immediately upon writing pixel data with the WriteImage()
%  method.
%
%  The format of the WriteStream() method is:
%
%      MagickBooleanType WriteStream(const ImageInfo *image_info,Image *,
%        StreamHandler stream)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o stream: A callback method.
%
%
*/
MagickExport MagickBooleanType WriteStream(const ImageInfo *image_info,
  Image *image,StreamHandler stream)
{
  ImageInfo
    *write_info;

  MagickBooleanType
    status;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  write_info=CloneImageInfo(image_info);
  write_info->stream=stream;
  status=WriteImage(write_info,image);
  write_info=DestroyImageInfo(write_info);
  return(status);
}
