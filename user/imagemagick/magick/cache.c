/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                      CCCC   AAA    CCCC  H   H  EEEEE                       %
%                     C      A   A  C      H   H  E                           %
%                     C      AAAAA  C      HHHHH  EEE                         %
%                     C      A   A  C      H   H  E                           %
%                      CCCC  A   A   CCCC  H   H  EEEEE                       %
%                                                                             %
%                                                                             %
%                      ImageMagick Pixel Cache Methods                        %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1999                                   %
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
#include "magick/color-private.h"
#include "magick/composite-private.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/resource_.h"
#include "magick/semaphore.h"
#include "magick/string_.h"
#include "magick/utility.h"
#if defined(HasZLIB)
#include "zlib.h"
#endif

/*
  Define declarations.
*/
#define DefaultNumberCacheViews  6UL

/*
  Typedef declarations.
*/
struct _NexusInfo
{
  MagickBooleanType
    available,
    mapped;

  unsigned long
    columns,
    rows;

  long
    x,
    y;

  MagickSizeType
    length;

  PixelPacket
    *cache,
    *pixels;

  IndexPacket
    *indexes;
};

/*
  Forward declarations.
*/
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static const PixelPacket
  *AcquirePixelCache(const Image *,const long,const long,const unsigned long,
    const unsigned long,ExceptionInfo *);

static IndexPacket
  *GetIndexesFromCache(const Image *);

static MagickBooleanType
  OpenCache(Image *,const MapMode,ExceptionInfo *),
  SyncPixelCache(Image *);

static PixelPacket
  AcquireOnePixelFromCache(const Image *,const long,const long,ExceptionInfo *),
  GetOnePixelFromCache(Image *,const long,const long),
  *GetPixelCache(Image *,const long,const long,const unsigned long,
    const unsigned long),
  *GetPixelsFromCache(const Image *),
  *SetPixelCache(Image *,const long,const long,const unsigned long,
    const unsigned long);

static void
  DestroyPixelCache(Image *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

/*
  Forward declaration.
*/
static PixelPacket
  *SetNexus(const Image *,const RectangleInfo *,const unsigned long);

static MagickBooleanType
  ReadCacheIndexes(CacheInfo *,const unsigned long,ExceptionInfo *),
  ReadCachePixels(CacheInfo *,const unsigned long,ExceptionInfo *),
  WriteCacheIndexes(CacheInfo *,const unsigned long,ExceptionInfo *),
  WriteCachePixels(CacheInfo *,const unsigned long,ExceptionInfo *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   A c q u i r e C a c h e N e x u s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireCacheNexus() acquires pixels from the in-memory or disk pixel cache
%  as defined by the geometry parameters.   A pointer to the pixels is
%  returned if the pixels are transferred, otherwise a NULL is returned.
%
%  The format of the AcquireCacheNexus() method is:
%
%      PixelPacket *AcquireCacheNexus(const Image *image,const long x,
%        const long y,const unsigned long columns,const unsigned long rows,
%        const unsigned long nexus,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
%    o nexus: specifies which cache nexus to acquire.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

static inline long EdgeX(const unsigned long columns,const long x)
{
  if (x < 0)
    return(0);
  if (x >= (long) columns)
    return((long) columns-1);
  return(x);
}

static inline long EdgeY(const unsigned long rows,const long y)
{
  if (y < 0)
    return(0);
  if (y >= (long) rows)
    return((long) rows-1);
  return(y);
}

static inline long TileX(const unsigned long columns,const long x)
{
  if (x < 0)
    return((long) columns+((x+1) % (long) columns)-1);
  if (x >= (long) columns)
    return(x % (long) columns);
  return(x);
}

static inline long TileY(const unsigned long rows,const long y)
{
  if (y < 0)
    return((long) rows+((y+1) % (long) rows)-1);
  if (y >= (long) rows)
    return(y % (long) rows);
  return(y);
}

static inline long MirrorX(const unsigned long columns,const long x)
{
  if ((x < 0)  || (x >= (long) columns))
    return((long) columns-TileX(columns,x)-1);
  return(x);
}

static inline long MirrorY(const unsigned long rows,const long y)
{
  if ((y < 0)  || (y >= (long) rows))
    return((long) rows-TileX(rows,y)-1);
  return(y);
}

static inline MagickBooleanType IsNexusInCore(const CacheInfo *cache_info,
  const unsigned long nexus)
{
  MagickOffsetType
    offset;

  register NexusInfo
    *nexus_info;

  nexus_info=cache_info->nexus_info+nexus;
  offset=(MagickOffsetType) nexus_info->y*cache_info->columns+nexus_info->x;
  if (nexus_info->pixels != (cache_info->pixels+offset))
    return(MagickFalse);
  return(MagickTrue);
}

MagickExport const PixelPacket *AcquireCacheNexus(const Image *image,
  const long x,const long y,const unsigned long columns,
  const unsigned long rows,const unsigned long nexus,ExceptionInfo *exception)
{
  CacheInfo
    *cache_info;

  IndexPacket
    *indexes,
    *nexus_indexes;

  MagickOffsetType
    offset;

  MagickSizeType
    length,
    number_pixels;

  PixelPacket
    *pixels;

  RectangleInfo
    region;

  register const PixelPacket
    *p;

  register long
    u,
    v;

  register PixelPacket
    *q;

  unsigned long
    image_nexus;

  /*
    Acquire pixels.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  if (cache_info->type == UndefinedCache)
    return((const PixelPacket *) NULL);
  region.x=x;
  region.y=y;
  region.width=columns;
  region.height=rows;
  pixels=SetNexus(image,&region,nexus);
  offset=(MagickOffsetType) region.y*cache_info->columns+region.x;
  length=(MagickSizeType) (region.height-1)*cache_info->columns+region.width-1;
  number_pixels=(MagickSizeType) cache_info->columns*cache_info->rows;
  if ((offset >= 0) && (((MagickSizeType) offset+length) < number_pixels))
    if ((x >= 0) && ((unsigned long) (x+columns) <= cache_info->columns) &&
        (y >= 0) && ((unsigned long) (y+rows) <= cache_info->rows))
      {
        MagickBooleanType
          status;

        /*
          Pixel request is inside cache extents.
        */
        if (IsNexusInCore(cache_info,nexus) != MagickFalse)
          return(pixels);
        status=ReadCachePixels(cache_info,nexus,exception);
        if ((cache_info->storage_class == PseudoClass) ||
            (cache_info->colorspace == CMYKColorspace))
          if (ReadCacheIndexes(cache_info,nexus,exception) == MagickFalse)
            status=MagickFalse;
        if (status == MagickFalse)
          return((const PixelPacket *) NULL);
        return(pixels);
      }
  /*
    Pixel request is outside cache extents.
  */
  indexes=GetNexusIndexes(cache_info,nexus);
  image_nexus=GetNexus(cache_info);
  if (image_nexus == 0)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),CacheError,
        "UnableToGetCacheNexus","`%s'",image->filename);
      return((const PixelPacket *) NULL);
    }
  q=pixels;
  for (v=0; v < (long) rows; v++)
  {
    for (u=0; u < (long) columns; u+=length)
    {
      length=(MagickSizeType) Min(cache_info->columns-(x+u),columns-u);
      if ((((x+u) < 0) || ((x+u) >= (long) cache_info->columns)) ||
          (((y+v) < 0) || ((y+v) >= (long) cache_info->rows)) || (length == 0))
        {
          /*
            Transfer a single pixel.
          */
          length=(MagickSizeType) 1;
          switch (cache_info->virtual_pixel_method)
          {
            case BackgroundVirtualPixelMethod:
            case ConstantVirtualPixelMethod:
            {
              p=AcquireCacheNexus(image,EdgeX(cache_info->columns,x+u),
                EdgeY(cache_info->rows,y+v),1UL,1UL,image_nexus,exception);
              cache_info->virtual_pixel=image->background_color;
              p=(&cache_info->virtual_pixel);
              break;
            }
            case EdgeVirtualPixelMethod:
            default:
            {
              p=AcquireCacheNexus(image,EdgeX(cache_info->columns,x+u),
                EdgeY(cache_info->rows,y+v),1UL,1UL,image_nexus,exception);
              break;
            }
            case MirrorVirtualPixelMethod:
            {
              p=AcquireCacheNexus(image,MirrorX(cache_info->columns,x+u),
                MirrorY(cache_info->rows,y+v),1UL,1UL,image_nexus,exception);
              break;
            }
            case TileVirtualPixelMethod:
            {
              p=AcquireCacheNexus(image,TileX(cache_info->columns,x+u),
                TileY(cache_info->rows,y+v),1UL,1UL,image_nexus,exception);
              break;
            }
            case TransparentVirtualPixelMethod:
            {
              p=AcquireCacheNexus(image,EdgeX(cache_info->columns,x+u),
                EdgeY(cache_info->rows,y+v),1UL,1UL,image_nexus,exception);
              cache_info->virtual_pixel.red=0;
              cache_info->virtual_pixel.green=0;
              cache_info->virtual_pixel.blue=0;
              cache_info->virtual_pixel.opacity=TransparentOpacity;
              p=(&cache_info->virtual_pixel);
              break;
            }
          }
          if (p == (const PixelPacket *) NULL)
            break;
          *q++=(*p);
          if (indexes == (IndexPacket *) NULL)
            continue;
          nexus_indexes=GetNexusIndexes(cache_info,image_nexus);
          if (nexus_indexes == (IndexPacket *) NULL)
            continue;
          *indexes++=(*nexus_indexes);
          continue;
        }
      /*
        Transfer a run of pixels.
      */
      p=AcquireCacheNexus(image,x+u,y+v,(unsigned long) length,1UL,image_nexus,
        exception);
      if (p == (const PixelPacket *) NULL)
        break;
      (void) CopyMagickMemory(q,p,(size_t) length*sizeof(*q));
      q+=length;
      if (indexes == (IndexPacket *) NULL)
        continue;
      nexus_indexes=GetNexusIndexes(cache_info,image_nexus);
      if (nexus_indexes == (IndexPacket *) NULL)
        continue;
      (void) CopyMagickMemory(indexes,nexus_indexes,(size_t) length*
        sizeof(*indexes));
      indexes+=length;
    }
  }
  DestroyCacheNexus(cache_info,image_nexus);
  return(pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e I m a g e P i x e l s                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireImagePixels() obtains a pixel region for read-only access. If the
%  region is successfully accessed, a pointer to it is returned, otherwise
%  NULL is returned. The returned pointer may point to a temporary working
%  copy of the pixels or it may point to the original pixels in memory.
%  Performance is maximized if the selected area is part of one row, or one
%  or more full rows, since then there is opportunity to access the pixels
%  in-place (without a copy) if the image is in RAM, or in a memory-mapped
%  file. The returned pointer should *never* be deallocated by the user.
%
%  Pixels accessed via the returned pointer represent a simple array of type
%  PixelPacket. If the image storage class is PsudeoClass, call GetIndexes()
%  after invoking GetImagePixels() to obtain the colormap indexes (of type
%  IndexPacket) corresponding to the region.
%
%  If you plan to modify the pixels, use GetImagePixels() instead.
%
%  The format of the AcquireImagePixels() method is:
%
%      const PixelPacket *AcquireImagePixels(const Image *image,const long x,
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
*/
MagickExport const PixelPacket *AcquireImagePixels(const Image *image,
  const long x,const long y,const unsigned long columns,
  const unsigned long rows,ExceptionInfo *exception)
{
  CacheInfo
    *cache_info;

  const PixelPacket
    *pixels;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  if (cache_info->methods.acquire_pixel_handler == (AcquirePixelHandler) NULL)
    return((const PixelPacket *) NULL);
  pixels=cache_info->methods.
    acquire_pixel_handler(image,x,y,columns,rows,exception);
  return(pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   A c q u i r e P i x e l C a c h e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquirePixelCache() acquires pixels from the in-memory or disk pixel
%  cache as defined by the geometry parameters.   A pointer to the pixels
%  is returned if the pixels are transferred, otherwise a NULL is returned.
%
%  The format of the AcquirePixelCache() method is:
%
%      const PixelPacket *AcquirePixelCache(const Image *image,const long x,
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
*/
static const PixelPacket *AcquirePixelCache(const Image *image,const long x,
  const long y,const unsigned long columns,const unsigned long rows,
  ExceptionInfo *exception)
{
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  return(AcquireCacheNexus(image,x,y,columns,rows,0,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e O n e P i x e l                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireOnePixel() returns a single pixel at the specified (x,y) location.
%  The image background color is returned if an error occurs.  If you plan to
%  modify the pixel, use GetOnePixel() instead.
%
%  The format of the AcquireOnePixel() method is:
%
%      PixelPacket AcquireOnePixel(const Image image,const long x,
%        const long y,ExceptionInfo exception)
%
%  A description of each parameter follows:
%
%    o pixels: AcquireOnePixel() returns a pixel at the specified (x,y)
%      location.
%
%    o image: The image.
%
%    o x,y:  These values define the location of the pixel to return.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport PixelPacket AcquireOnePixel(const Image *image,const long x,
  const long y,ExceptionInfo *exception)
{
  CacheInfo
    *cache_info;

  PixelPacket
    pixel;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  if (cache_info->methods.acquire_one_pixel_from_handler ==
      (AcquireOnePixelFromHandler) NULL)
    return(image->background_color);
  pixel=cache_info->methods.acquire_one_pixel_from_handler(image,x,y,exception);
  return(pixel);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   A c q u i r e O n e P i x e l F r o m C a c h e                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireOnePixelFromCache() returns a single pixel at the specified (x,y)
%  location.  The image background color is returned if an error occurs.
%
%  The format of the AcquireOnePixelFromCache() method is:
%
%      PixelPacket *AcquireOnePixelFromCache(const Image image,const long x,
%        const long y,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pixels: AcquireOnePixelFromCache returns a pixel at the specified (x,y)
%      location.
%
%    o image: The image.
%
%    o x,y:  These values define the location of the pixel to return.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
static PixelPacket AcquireOnePixelFromCache(const Image *image,const long x,
  const long y,ExceptionInfo *exception)
{
  register const PixelPacket
    *pixel;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  pixel=AcquirePixelCache(image,x,y,1UL,1UL,exception);
  if (pixel != (PixelPacket *) NULL)
    return(*pixel);
  return(image->background_color);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C l i p C a c h e N e x u s                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ClipCacheNexus() clips the image pixels of the in-memory or disk cache as
%  defined by the image clip mask.  The method returns MagickTrue if the pixel
%  region is clipped, otherwise MagickFalse.
%
%  The format of the ClipCacheNexus() method is:
%
%      MagickBooleanType ClipCacheNexus(Image *image,const unsigned long nexus)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o nexus: specifies which cache nexus to clip.
%
*/
static MagickBooleanType ClipCacheNexus(Image *image,const unsigned long nexus)
{
  CacheInfo
    *cache_info;

  long
    y;

  MagickBooleanType
    status;

  NexusInfo
    *nexus_info;

  register const PixelPacket
    *r;

  register IndexPacket
    *nexus_indexes,
    *indexes;

  register long
    x;

  register PixelPacket
    *p,
    *q;

  unsigned long
    image_nexus,
    mask_nexus;

  /*
    Apply clip mask.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  image_nexus=GetNexus(image->cache);
  mask_nexus=GetNexus(image->clip_mask->cache);
  if ((image_nexus == 0) || (mask_nexus == 0))
    ThrowBinaryException(CacheError,"UnableToGetCacheNexus",image->filename);
  cache_info=(CacheInfo *) image->cache;
  nexus_info=cache_info->nexus_info+nexus;
  p=GetCacheNexus(image,nexus_info->x,nexus_info->y,nexus_info->columns,
    nexus_info->rows,image_nexus);
  indexes=GetNexusIndexes(image->cache,image_nexus);
  q=nexus_info->pixels;
  nexus_indexes=nexus_info->indexes;
  r=AcquireCacheNexus(image->clip_mask,nexus_info->x,nexus_info->y,
    nexus_info->columns,nexus_info->rows,mask_nexus,&image->exception);
  if ((p != (PixelPacket *) NULL) && (r != (const PixelPacket *) NULL))
    for (y=0; y < (long) nexus_info->rows; y++)
    {
      for (x=0; x < (long) nexus_info->columns; x++)
      {
        if (PixelIntensityToQuantum(r) == TransparentOpacity)
          {
            q->red=p->red;
            q->green=p->green;
            q->blue=p->blue;
            q->opacity=p->opacity;
            if ((cache_info->storage_class == PseudoClass) ||
                (cache_info->colorspace == CMYKColorspace))
              *nexus_indexes=(*indexes);
          }
        if ((cache_info->storage_class == PseudoClass) ||
            (cache_info->colorspace == CMYKColorspace))
          {
            indexes++;
            nexus_indexes++;
          }
        p++;
        q++;
        r++;
      }
    }
  DestroyCacheNexus(image->cache,image_nexus);
  DestroyCacheNexus(image->clip_mask->cache,mask_nexus);
  status=(MagickBooleanType) ((p != (PixelPacket *) NULL) &&
    (q != (PixelPacket *) NULL));
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C l o n e C a c h e N e x u s                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneCacheNexus() clones the source cache nexus to the destination nexus.
%
%  The format of the CloneCacheNexus() method is:
%
%      MagickBooleanType CloneCacheNexus(CacheInfo *destination,
%        CacheInfo *source,const unsigned long nexus)
%
%  A description of each parameter follows:
%
%    o destination: The destination cache nexus.
%
%    o source: The source cache nexus.
%
%    o nexus: specifies which cache nexus to clone.
%
*/

static inline void AcquireNexusPixels(NexusInfo *nexus_info)
{
  assert(nexus_info != (NexusInfo *) NULL);
  assert(nexus_info->length == (MagickSizeType) ((size_t) nexus_info->length));
  nexus_info->cache=(PixelPacket *)
    MapBlob(-1,IOMode,0,(size_t) nexus_info->length);
  if (nexus_info->cache != (PixelPacket *) NULL)
    {
      nexus_info->mapped=MagickTrue;
      return;
    }
  nexus_info->cache=(PixelPacket *)
    AcquireMagickMemory((size_t) nexus_info->length);
  if (nexus_info->cache == (PixelPacket *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      strerror(errno));
  (void) ResetMagickMemory(nexus_info->cache,0,(size_t) nexus_info->length);
  nexus_info->mapped=MagickFalse;
}

static MagickBooleanType CloneCacheNexus(CacheInfo *destination,
  CacheInfo *source,const unsigned long nexus)
{
  MagickSizeType
    number_pixels;

  register const NexusInfo
    *p;

  register long
    id;

  register NexusInfo
    *q;

  destination->number_views=source->number_views;
  destination->nexus_info=(NexusInfo *) ResizeMagickMemory(
    destination->nexus_info,(size_t) destination->number_views*
    sizeof(*destination->nexus_info));
  if (destination->nexus_info == (NexusInfo *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      strerror(errno));
  (void) ResetMagickMemory(destination->nexus_info,0,(size_t)
    destination->number_views*sizeof(*destination->nexus_info));
  for (id=0; id < (long) source->number_views; id++)
  {
    p=source->nexus_info+id;
    q=destination->nexus_info+id;
    q->available=p->available;
    q->columns=p->columns;
    q->rows=p->rows;
    q->x=p->x;
    q->y=p->y;
    q->length=p->length;
    q->cache=p->cache;
    q->pixels=p->pixels;
    q->indexes=p->indexes;
    if (p->cache != (PixelPacket *) NULL)
      {
        AcquireNexusPixels(q);
        (void) CopyMagickMemory(q->cache,p->cache,(size_t) p->length);
        q->pixels=q->cache;
        q->indexes=(IndexPacket *) NULL;
        number_pixels=(MagickSizeType) q->columns*q->rows;
        if ((destination->storage_class == PseudoClass) ||
            (destination->colorspace == CMYKColorspace))
          q->indexes=(IndexPacket *) (q->pixels+number_pixels);
      }
  }
  if (nexus != 0)
    DestroyCacheNexus(source,nexus);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C l o n e P i x e l C a c h e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% %
%  ClonePixelCache() clones the source pixel cache to the destination cache.
%
%  The format of the ClonePixelCache() method is:
%
%      MagickBooleanType ClonePixelCache(CacheInfo *cache_info,
%        CacheInfo *source_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o cache_info: The pixel cache.
%
%    o source_info: The source pixel cache.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

static inline int OpenDiskCache(const char *filename,MapMode mode)
{
  int
    file;

  switch (mode)
  {
    case ReadMode:
    {
      file=open(filename,O_RDONLY | O_BINARY);
      break;
    }
    case WriteMode:
    {
      file=open(filename,O_WRONLY | O_CREAT | O_BINARY | O_EXCL,S_MODE);
      if (file == -1)
        file=open(filename,O_WRONLY | O_BINARY,S_MODE);
      break;
    }
    case IOMode:
    default:
    {
      file=open(filename,O_RDWR | O_CREAT | O_BINARY | O_EXCL,S_MODE);
      if (file == -1)
        file=open(filename,O_RDWR | O_BINARY,S_MODE);
      break;
    }
  }
  return(file);
}

static inline MagickOffsetType ReadCacheRegion(int file,unsigned char *buffer,
  MagickSizeType length,MagickOffsetType offset)
{
  register MagickOffsetType
    i;

  ssize_t
    count;

#if !defined(HAVE_PREAD)
  if ((MagickSeek(file,offset,SEEK_SET)) < 0)
    return((MagickOffsetType) -1);
#endif
  count=0;
  for (i=0; i < (MagickOffsetType) length; i+=count)
  {
#if !defined(HAVE_PREAD)
    count=read(file,buffer+i,(size_t) Min(length-i,(MagickSizeType)
      MagickMaxBufferSize));
#else
    count=pread(file,buffer+i,(size_t) Min(length-i,(MagickSizeType)
      MagickMaxBufferSize),(off_t) (offset+i));
#endif
    if (count > 0)
      continue;
    count=0;
    if (errno != EINTR)
      return((MagickOffsetType) -1);
  }
  return(i);
}

static inline MagickOffsetType WriteCacheRegion(int file,
  const unsigned char *buffer,MagickSizeType length,MagickOffsetType offset)
{
  register MagickOffsetType
    i;

  ssize_t
    count;

#if !defined(HAVE_PWRITE)
  if ((MagickSeek(file,offset,SEEK_SET)) < 0)
    return((MagickOffsetType) -1);
#endif
  count=0;
  for (i=0; i < (MagickOffsetType) length; i+=count)
  {
#if !defined(HAVE_PWRITE)
    count=write(file,buffer+i,(size_t) Min(length-i,(MagickSizeType)
      MagickMaxBufferSize));
#else
    count=pwrite(file,buffer+i,(size_t) Min(length-i,(MagickSizeType)
      MagickMaxBufferSize),(off_t) (offset+i));
#endif
    if (count > 0)
      continue;
    count=0;
    if (errno != EINTR)
      return((MagickOffsetType) -1);
  }
  return(i);
}

static MagickBooleanType CloneDiskToDiskPixels(CacheInfo *cache_info,
  CacheInfo *source_info,ExceptionInfo *exception)
{
  MagickOffsetType
    count,
    offset,
    source_offset;

  MagickSizeType
    length;

  register long
    y;

  register PixelPacket
    *pixels;

  unsigned long
    columns,
    rows;

  if (source_info->debug != MagickFalse)
    (void) LogMagickEvent(CacheEvent,GetMagickModule(),"disk => disk");
  if (cache_info->file == -1)
    {
      cache_info->file=OpenDiskCache(cache_info->cache_filename,IOMode);
      if (cache_info->file == -1)
        {
          ThrowFileException(exception,FileOpenError,"UnableToOpenFile",
            cache_info->cache_filename);
          return(MagickFalse);
        }
    }
  if (source_info->file == -1)
    {
      source_info->file=OpenDiskCache(source_info->cache_filename,IOMode);
      if (source_info->file == -1)
        {
          ThrowFileException(exception,FileOpenError,"UnableToOpenFile",
            source_info->cache_filename);
          return(MagickFalse);
        }
    }
  columns=Min(cache_info->columns,source_info->columns);
  rows=Min(cache_info->rows,source_info->rows);
  if (((cache_info->storage_class == PseudoClass) ||
       (cache_info->colorspace == CMYKColorspace)) &&
      ((source_info->storage_class == PseudoClass) ||
       (source_info->colorspace == CMYKColorspace)))
    {
      register IndexPacket
        *indexes;

      /*
        Clone cache indexes.
      */
      length=Max(cache_info->columns,source_info->columns)*sizeof(*indexes);
      indexes=(IndexPacket *) AcquireMagickMemory((size_t) length);
      if (indexes == (IndexPacket *) NULL)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),CacheError,
            "MemoryAllocationFailed","`%s'",source_info->cache_filename);
          return(MagickFalse);
        }
      (void) ResetMagickMemory(indexes,0,(size_t) length);
      length=columns*sizeof(*indexes);
      source_offset=(MagickOffsetType) source_info->columns*source_info->rows*
        sizeof(*pixels)+source_info->columns*rows*sizeof(*indexes);
      offset=(MagickOffsetType) cache_info->columns*cache_info->rows*
        sizeof(*pixels)+cache_info->columns*rows*sizeof(*indexes);
      for (y=0; y < (long) rows; y++)
      {
        source_offset-=source_info->columns*sizeof(*indexes);
        count=ReadCacheRegion(source_info->file,(unsigned char *) indexes,
          length,source_info->offset+source_offset);
        if ((MagickSizeType) count != length)
          break;
        offset-=cache_info->columns*sizeof(*indexes);
        count=WriteCacheRegion(cache_info->file,(unsigned char *) indexes,
          length,cache_info->offset+offset);
        if ((MagickSizeType) count != length)
          break;
      }
      if (y < (long) rows)
        {
          indexes=(IndexPacket *) RelinquishMagickMemory(indexes);
          ThrowFileException(exception,CacheError,"UnableToCloneCache",
            source_info->cache_filename);
          return(MagickFalse);
        }
      if (cache_info->columns > source_info->columns)
        {
          length=(cache_info->columns-source_info->columns)*sizeof(*indexes);
          (void) ResetMagickMemory(indexes,0,(size_t) length);
          offset=(MagickOffsetType) cache_info->columns*cache_info->rows*
            sizeof(*pixels)+(cache_info->columns*rows+columns)*sizeof(*indexes);
          for (y=0; y < (long) rows; y++)
          {
            offset-=cache_info->columns*sizeof(*indexes);
            count=WriteCacheRegion(cache_info->file,(unsigned char *) indexes,
              length,cache_info->offset+offset);
            if ((MagickSizeType) count != length)
              break;
          }
          if (y < (long) rows)
            {
              indexes=(IndexPacket *) RelinquishMagickMemory(indexes);
              ThrowFileException(exception,CacheError,"UnableToCloneCache",
                source_info->cache_filename);
              return(MagickFalse);
            }
        }
      indexes=(IndexPacket *) RelinquishMagickMemory(indexes);
    }
  /*
    Clone cache pixels.
  */
  length=Max(cache_info->columns,source_info->columns)*sizeof(*pixels);
  pixels=(PixelPacket *) AcquireMagickMemory((size_t) length);
  if (pixels == (PixelPacket *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),CacheError,
        "MemoryAllocationFailed","`%s'",source_info->cache_filename);
      return(MagickFalse);
    }
  (void) ResetMagickMemory(pixels,0,(size_t) length);
  length=columns*sizeof(*pixels);
  source_offset=(MagickOffsetType) source_info->columns*rows*sizeof(*pixels);
  offset=(MagickOffsetType) cache_info->columns*rows*sizeof(*pixels);
  for (y=0; y < (long) rows; y++)
  {
    source_offset-=source_info->columns*sizeof(*pixels);
    count=ReadCacheRegion(source_info->file,(unsigned char *) pixels,length,
      source_info->offset+source_offset);
    if ((MagickSizeType) count != length)
      break;
    offset-=cache_info->columns*sizeof(*pixels);
    count=WriteCacheRegion(cache_info->file,(unsigned char *) pixels,length,
      cache_info->offset+offset);
    if ((MagickSizeType) count != length)
      break;
  }
  if (y < (long) rows)
    {
      pixels=(PixelPacket *) RelinquishMagickMemory(pixels);
      ThrowFileException(exception,CacheError,"UnableToCloneCache",
        source_info->cache_filename);
      return(MagickFalse);
    }
  if (cache_info->columns > source_info->columns)
    {
      offset=(MagickOffsetType) (cache_info->columns*rows+columns)*
        sizeof(*pixels);
      length=(cache_info->columns-source_info->columns)*sizeof(*pixels);
      (void) ResetMagickMemory(pixels,0,(size_t) length);
      for (y=0; y < (long) rows; y++)
      {
        offset-=cache_info->columns*sizeof(*pixels);
        count=WriteCacheRegion(cache_info->file,(unsigned char *) pixels,length,
          cache_info->offset+offset);
        if ((MagickSizeType) count != length)
          break;
      }
      if (y < (long) rows)
        {
          pixels=(PixelPacket *) RelinquishMagickMemory(pixels);
          ThrowFileException(exception,CacheError,"UnableToCloneCache",
            source_info->cache_filename);
          return(MagickFalse);
        }
    }
  pixels=(PixelPacket *) RelinquishMagickMemory(pixels);
  return(MagickTrue);
}

static MagickBooleanType CloneDiskToMemoryPixels(CacheInfo *cache_info,
  CacheInfo *source_info,ExceptionInfo *exception)
{
  MagickOffsetType
    count,
    offset;

  MagickSizeType
    length;

  register long
    y;

  register PixelPacket
    *pixels,
    *q;

  unsigned long
    columns,
    rows;

  if (source_info->debug != MagickFalse)
    (void) LogMagickEvent(CacheEvent,GetMagickModule(),"disk => memory");
  if (source_info->file == -1)
    {
      source_info->file=OpenDiskCache(source_info->cache_filename,IOMode);
      if (source_info->file == -1)
        {
          ThrowFileException(exception,FileOpenError,"UnableToOpenFile",
            source_info->cache_filename);
          return(MagickFalse);
        }
    }
  columns=Min(cache_info->columns,source_info->columns);
  rows=Min(cache_info->rows,source_info->rows);
  if (((cache_info->storage_class == PseudoClass) ||
       (cache_info->colorspace == CMYKColorspace)) &&
      ((source_info->storage_class == PseudoClass) ||
       (source_info->colorspace == CMYKColorspace)))
    {
      register IndexPacket
        *indexes,
        *q;

      /*
        Clone cache indexes.
      */
      length=Max(cache_info->columns,source_info->columns)*sizeof(*indexes);
      indexes=(IndexPacket *) AcquireMagickMemory((size_t) length);
      if (indexes == (IndexPacket *) NULL)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),CacheError,
            "MemoryAllocationFailed","`%s'",source_info->cache_filename);
          return(MagickFalse);
        }
      (void) ResetMagickMemory(indexes,0,(size_t) length);
      length=columns*sizeof(IndexPacket);
      offset=(MagickOffsetType) source_info->columns*source_info->rows*
        sizeof(*pixels)+source_info->columns*rows*sizeof(*indexes);
      q=cache_info->indexes+cache_info->columns*rows;
      for (y=0; y < (long) rows; y++)
      {
        offset-=source_info->columns*sizeof(IndexPacket);
        count=ReadCacheRegion(source_info->file,(unsigned char *) indexes,
          length,source_info->offset+offset);
        if ((MagickSizeType) count != length)
          break;
        q-=cache_info->columns;
        (void) CopyMagickMemory(q,indexes,(size_t) length);
        if ((MagickSizeType) count != length)
          break;
      }
      if (y < (long) rows)
        {
          indexes=(IndexPacket *) RelinquishMagickMemory(indexes);
          ThrowFileException(exception,CacheError,"UnableToCloneCache",
            source_info->cache_filename);
          return(MagickFalse);
        }
      indexes=(IndexPacket *) RelinquishMagickMemory(indexes);
    }
  /*
    Clone cache pixels.
  */
  length=Max(cache_info->columns,source_info->columns)*sizeof(*pixels);
  pixels=(PixelPacket *) AcquireMagickMemory((size_t) length);
  if (pixels == (PixelPacket *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),CacheError,
        "MemoryAllocationFailed","`%s'",source_info->cache_filename);
      return(MagickFalse);
    }
  (void) ResetMagickMemory(pixels,0,(size_t) length);
  length=columns*sizeof(*pixels);
  offset=(MagickOffsetType) source_info->columns*rows*sizeof(*pixels);
  q=cache_info->pixels+cache_info->columns*rows;
  for (y=0; y < (long) rows; y++)
  {
    offset-=source_info->columns*sizeof(*pixels);
    count=ReadCacheRegion(source_info->file,(unsigned char *) pixels,length,
      source_info->offset+offset);
    if ((MagickSizeType) count != length)
      break;
    q-=cache_info->columns;
    (void) CopyMagickMemory(q,pixels,(size_t) length);
  }
  if (y < (long) rows)
    {
      pixels=(PixelPacket *) RelinquishMagickMemory(pixels);
      ThrowFileException(exception,CacheError,"UnableToCloneCache",
        source_info->cache_filename);
      return(MagickFalse);
    }
  pixels=(PixelPacket *) RelinquishMagickMemory(pixels);
  return(MagickTrue);
}

static MagickBooleanType CloneMemoryToDiskPixels(CacheInfo *cache_info,
  CacheInfo *source_info,ExceptionInfo *exception)
{
  MagickOffsetType
    count,
    offset;

  MagickSizeType
    length;

  register long
    y;

  register PixelPacket
    *p,
    *pixels;

  unsigned long
    columns,
    rows;

  if (source_info->debug != MagickFalse)
    (void) LogMagickEvent(CacheEvent,GetMagickModule(),"memory => disk");
  if (cache_info->file == -1)
    {
      cache_info->file=OpenDiskCache(cache_info->cache_filename,IOMode);
      if (cache_info->file == -1)
        {
          ThrowFileException(exception,FileOpenError,"UnableToOpenFile",
            cache_info->cache_filename);
          return(MagickFalse);
        }
    }
  columns=Min(cache_info->columns,source_info->columns);
  rows=Min(cache_info->rows,source_info->rows);
  if (((cache_info->storage_class == PseudoClass) ||
       (cache_info->colorspace == CMYKColorspace)) &&
      ((source_info->storage_class == PseudoClass) ||
       (source_info->colorspace == CMYKColorspace)))
    {
      register IndexPacket
        *p,
        *indexes;

      /*
        Clone cache indexes.
      */
      length=Max(cache_info->columns,source_info->columns)*sizeof(*indexes);
      indexes=(IndexPacket *) AcquireMagickMemory((size_t) length);
      if (indexes == (IndexPacket *) NULL)
        {
          (void) ThrowMagickException(exception,GetMagickModule(),CacheError,
            "MemoryAllocationFailed","`%s'",source_info->cache_filename);
          return(MagickFalse);
        }
      (void) ResetMagickMemory(indexes,0,(size_t) length);
      length=columns*sizeof(*indexes);
      p=source_info->indexes+source_info->columns*rows;
      offset=(MagickOffsetType) cache_info->columns*cache_info->rows*
        sizeof(*pixels)+cache_info->columns*rows*sizeof(*indexes);
      for (y=0; y < (long) rows; y++)
      {
        p-=source_info->columns;
        (void) CopyMagickMemory(indexes,p,(size_t) length);
        offset-=cache_info->columns*sizeof(*indexes);
        count=WriteCacheRegion(cache_info->file,(unsigned char *) indexes,
          length,cache_info->offset+offset);
        if ((MagickSizeType) count != length)
          break;
      }
      if (y < (long) rows)
        {
          indexes=(IndexPacket *) RelinquishMagickMemory(indexes);
          ThrowFileException(exception,CacheError,"UnableToCloneCache",
            source_info->cache_filename);
          return(MagickFalse);
        }
      if (cache_info->columns > source_info->columns)
        {
          length=(cache_info->columns-source_info->columns)*sizeof(*indexes);
          (void) ResetMagickMemory(indexes,0,(size_t) length);
          offset=(MagickOffsetType) cache_info->columns*cache_info->rows*
            sizeof(*pixels)+(cache_info->columns*rows+columns)*sizeof(*indexes);
          for (y=0; y < (long) rows; y++)
          {
            offset-=cache_info->columns*sizeof(*indexes);
            count=WriteCacheRegion(cache_info->file,(unsigned char *) indexes,
              length,cache_info->offset+offset);
            if ((MagickSizeType) count != length)
              break;
          }
          if (y < (long) rows)
            {
              indexes=(IndexPacket *) RelinquishMagickMemory(indexes);
              ThrowFileException(exception,CacheError,"UnableToCloneCache",
                source_info->cache_filename);
              return(MagickFalse);
            }
        }
      indexes=(IndexPacket *) RelinquishMagickMemory(indexes);
    }
  /*
    Clone cache pixels.
  */
  length=Max(cache_info->columns,source_info->columns)*sizeof(*pixels);
  pixels=(PixelPacket *) AcquireMagickMemory((size_t) length);
  if (pixels == (PixelPacket *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),CacheError,
        "MemoryAllocationFailed","`%s'",source_info->cache_filename);
      return(MagickFalse);
    }
  (void) ResetMagickMemory(pixels,0,(size_t) length);
  length=columns*sizeof(*pixels);
  p=source_info->pixels+source_info->columns*rows;
  offset=(MagickOffsetType) cache_info->columns*rows*sizeof(*pixels);
  for (y=0; y < (long) rows; y++)
  {
    p-=source_info->columns;
    (void) CopyMagickMemory(pixels,p,(size_t) length);
    offset-=cache_info->columns*sizeof(*pixels);
    count=WriteCacheRegion(cache_info->file,(unsigned char *) pixels,length,
      cache_info->offset+offset);
    if ((MagickSizeType) count != length)
      break;
  }
  if (y < (long) rows)
    {
      pixels=(PixelPacket *) RelinquishMagickMemory(pixels);
      ThrowFileException(exception,CacheError,"UnableToCloneCache",
        source_info->cache_filename);
      return(MagickFalse);
    }
  if (cache_info->columns > source_info->columns)
    {
      offset=(MagickOffsetType) (cache_info->columns*rows+columns)*
        sizeof(*pixels);
      length=(cache_info->columns-source_info->columns)*sizeof(*pixels);
      (void) ResetMagickMemory(pixels,0,(size_t) length);
      for (y=0; y < (long) rows; y++)
      {
        offset-=cache_info->columns*sizeof(*pixels);
        count=WriteCacheRegion(cache_info->file,(unsigned char *) pixels,length,
          cache_info->offset+offset);
        if ((MagickSizeType) count != length)
          break;
      }
      if (y < (long) rows)
        {
          pixels=(PixelPacket *) RelinquishMagickMemory(pixels);
          ThrowFileException(exception,CacheError,"UnableToCloneCache",
            source_info->cache_filename);
          return(MagickFalse);
        }
    }
  pixels=(PixelPacket *) RelinquishMagickMemory(pixels);
  return(MagickTrue);
}

static MagickBooleanType CloneMemoryToMemoryPixels(CacheInfo *cache_info,
  CacheInfo *source_info,ExceptionInfo *magick_unused(exception))
{
  register long
    y;

  register PixelPacket
    *pixels,
    *source_pixels;

  size_t
    length;

  unsigned long
    columns,
    rows;

  if (source_info->debug != MagickFalse)
    (void) LogMagickEvent(CacheEvent,GetMagickModule(),"memory => memory");
  columns=Min(cache_info->columns,source_info->columns);
  rows=Min(cache_info->rows,source_info->rows);
  if (((cache_info->storage_class == PseudoClass) ||
       (cache_info->colorspace == CMYKColorspace)) &&
      ((source_info->storage_class == PseudoClass) ||
       (source_info->colorspace == CMYKColorspace)))
    {
      register IndexPacket
        *indexes,
        *source_indexes;

      /*
        Clone cache indexes.
      */
      length=columns*sizeof(*indexes);
      if (cache_info->columns == source_info->columns)
        (void) CopyMagickMemory(cache_info->indexes,source_info->indexes,
          length*rows);
      else
        {
          source_indexes=source_info->indexes+source_info->columns*rows;
          indexes=cache_info->indexes+cache_info->columns*rows;
          for (y=0; y < (long) rows; y++)
          {
            source_indexes-=source_info->columns;
            indexes-=cache_info->columns;
            (void) CopyMagickMemory(indexes,source_indexes,length);
          }
          if (cache_info->columns > source_info->columns)
            {
              length=(cache_info->columns-source_info->columns)*
                sizeof(*indexes);
              indexes=cache_info->indexes+cache_info->columns*rows+
                source_info->columns;
              for (y=0; y < (long) rows; y++)
              {
                indexes-=cache_info->columns;
                (void) ResetMagickMemory(indexes,0,length);
              }
            }
        }
    }
  /*
    Clone cache pixels.
  */
  length=columns*sizeof(*pixels);
  if (cache_info->columns == source_info->columns)
    (void) CopyMagickMemory(cache_info->pixels,source_info->pixels,length*rows);
  else
    {
      source_pixels=source_info->pixels+source_info->columns*rows;
      pixels=cache_info->pixels+cache_info->columns*rows;
      for (y=0; y < (long) rows; y++)
      {
        source_pixels-=source_info->columns;
        pixels-=cache_info->columns;
        (void) CopyMagickMemory(pixels,source_pixels,length);
      }
      if (cache_info->columns > source_info->columns)
        {
          length=(cache_info->columns-source_info->columns)*sizeof(*pixels);
          pixels=cache_info->pixels+cache_info->columns*rows+
            source_info->columns;
          for (y=0; y < (long) rows; y++)
          {
            pixels-=cache_info->columns;
            (void) ResetMagickMemory(pixels,0,length);
          }
        }
    }
  return(MagickTrue);
}

static MagickBooleanType ClonePixelCache(CacheInfo *cache_info,
  CacheInfo *source_info,ExceptionInfo *exception)
{
  if ((cache_info->type != DiskCache) && (source_info->type != DiskCache))
    return(CloneMemoryToMemoryPixels(cache_info,source_info,exception));
  if ((cache_info->type == DiskCache) && (source_info->type == DiskCache))
    return(CloneDiskToDiskPixels(cache_info,source_info,exception));
  if (source_info->type == DiskCache)
    return(CloneDiskToMemoryPixels(cache_info,source_info,exception));
  return(CloneMemoryToDiskPixels(cache_info,source_info,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C l o n e P i x e l C a c h e M e t h o d s                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ClonePixelCacheMethods() clones the pixel cache methods from one cache to
%  another.
%
%  The format of the ClonePixelCacheMethods() method is:
%
%      void ClonePixelCacheMethods(Cache clone,const Cache cache)
%
%  A description of each parameter follows:
%
%    o clone: Specifies a pointer to a Cache structure.
%
%    o cache: The pixel cache.
%
*/
MagickExport void ClonePixelCacheMethods(Cache clone,const Cache cache)
{
  CacheInfo
    *cache_info,
    *source_info;

  assert(clone != (Cache) NULL);
  source_info=(CacheInfo *) clone;
  assert(source_info->signature == MagickSignature);
  if (source_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      source_info->filename);
  assert(cache != (Cache) NULL);
  cache_info=(CacheInfo *) cache;
  assert(cache_info->signature == MagickSignature);
  source_info->methods=cache_info->methods;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y C a c h e I n f o                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyCacheInfo() deallocates memory associated with the pixel cache.
%
%  The format of the DestroyCacheInfo() method is:
%
%      Cache DestroyCacheInfo(Cache cache)
%
%  A description of each parameter follows:
%
%    o cache: The pixel cache.
%
*/

static inline void RelinquishCachePixels(CacheInfo *cache_info)
{
  assert(cache_info != (CacheInfo *) NULL);
  switch (cache_info->type)
  {
    case MemoryCache:
    {
      if (cache_info->mapped == MagickFalse)
        (void) RelinquishMagickMemory(cache_info->pixels);
      else
        (void) UnmapBlob(cache_info->pixels,(size_t) cache_info->length);
      RelinquishMagickResource(MemoryResource,cache_info->length);
      break;
    }
    case MapCache:
    {
      (void) UnmapBlob(cache_info->pixels,(size_t) cache_info->length);
      RelinquishMagickResource(MapResource,cache_info->length);
    }
    case DiskCache:
    {
      if (cache_info->file != -1)
        (void) close(cache_info->file);
      cache_info->file=(-1);
      RelinquishMagickResource(DiskResource,cache_info->length);
      break;
    }
    default:
      break;
  }
  cache_info->type=UndefinedCache;
  cache_info->mapped=MagickFalse;
  cache_info->pixels=(PixelPacket *) NULL;
  cache_info->indexes=(IndexPacket *) NULL;
}

MagickExport Cache DestroyCacheInfo(Cache cache)
{
  char
    message[MaxTextExtent];

  CacheType
    type;

  CacheInfo
    *cache_info;

  assert(cache != (Cache) NULL);
  cache_info=(CacheInfo *) cache;
  assert(cache_info->signature == MagickSignature);
  if (cache_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      cache_info->filename);
  AcquireSemaphoreInfo(&cache_info->semaphore);
  cache_info->reference_count--;
  if (cache_info->reference_count > 0)
    {
      RelinquishSemaphoreInfo(cache_info->semaphore);
      return((Cache) NULL);
    }
  type=cache_info->type;
  RelinquishCachePixels(cache_info);
  if ((type == MapCache) || (type == DiskCache))
    (void) RelinquishUniqueFileResource(cache_info->cache_filename);
  if (cache_info->nexus_info != (NexusInfo *) NULL)
    {
      register long
        id;

      for (id=0; id < (long) cache_info->number_views; id++)
        DestroyCacheNexus(cache,(unsigned long) id);
      cache_info->nexus_info=(NexusInfo *)
        RelinquishMagickMemory(cache_info->nexus_info);
    }
  (void) FormatMagickString(message,MaxTextExtent,"destroy %s",
    cache_info->filename);
  if (cache_info->debug != MagickFalse)
    (void) LogMagickEvent(CacheEvent,GetMagickModule(),"%s",message);
  cache_info->signature=(~MagickSignature);
  RelinquishSemaphoreInfo(cache_info->semaphore);
  cache_info->semaphore=DestroySemaphoreInfo(cache_info->semaphore);
  cache_info=(CacheInfo *) RelinquishMagickMemory(cache_info);
  cache=(Cache) NULL;
  return(cache);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y C a c h e N e x u s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyCacheNexus() destroys a cache nexus.
%
%  The format of the DestroyCacheNexus() method is:
%
%      void DestroyCacheNexus(Cache cache,const unsigned long nexus)
%
%  A description of each parameter follows:
%
%    o cache: The pixel cache.
%
%    o nexus: specifies which cache nexus to destroy.
%
*/

static inline void RelinquishNexusPixels(NexusInfo *nexus_info)
{
  assert(nexus_info != (NexusInfo *) NULL);
  assert(nexus_info->length == (MagickSizeType) ((size_t) nexus_info->length));
  if (nexus_info->mapped == MagickFalse)
    (void) RelinquishMagickMemory(nexus_info->cache);
  else
    (void) UnmapBlob(nexus_info->cache,(size_t) nexus_info->length);
  nexus_info->mapped=MagickFalse;
  nexus_info->cache=(PixelPacket *) NULL;
  nexus_info->pixels=(PixelPacket *) NULL;
  nexus_info->indexes=(IndexPacket *) NULL;
}

MagickExport void DestroyCacheNexus(Cache cache,const unsigned long nexus)
{
  CacheInfo
    *cache_info;

  register NexusInfo
    *nexus_info;

  assert(cache != (Cache) NULL);
  cache_info=(CacheInfo *) cache;
  assert(cache_info->signature == MagickSignature);
  if (cache_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      cache_info->filename);
  nexus_info=cache_info->nexus_info+nexus;
  if (nexus_info->cache != (PixelPacket *) NULL)
    RelinquishNexusPixels(nexus_info);
  (void) ResetMagickMemory(nexus_info,0,sizeof(*nexus_info));
  nexus_info->available=MagickTrue;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y I m a g e P i x e l s                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyImagePixels() deallocates memory associated with the pixel cache.
%
%  The format of the DestroyImagePixels() method is:
%
%      void DestroyImagePixels(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/
MagickExport void DestroyImagePixels(Image *image)
{
  CacheInfo
    *cache_info;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  if (cache_info->methods.destroy_pixel_handler == (DestroyPixelHandler) NULL)
    return;
  cache_info->methods.destroy_pixel_handler(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y P i x e l C a c h e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyPixelCache() deallocates memory associated with the pixel cache.
%
%  The format of the DestroyPixelCache() method is:
%
%      void DestroyPixelCache(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/
static void DestroyPixelCache(Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->cache == (void *) NULL)
    return;
  image->cache=DestroyCacheInfo(image->cache);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t C a c h e C l a s s                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetCacheClass() returns the class type of the pixel cache.
%
%  The format of the GetCacheClass() method is:
%
%      ClassType GetCacheClass(Cache cache)
%
%  A description of each parameter follows:
%
%    o type: GetCacheClass returns DirectClass or PseudoClass.
%
%    o cache: The pixel cache.
%
*/
MagickExport ClassType GetCacheClass(const Cache cache)
{
  CacheInfo
    *cache_info;

  assert(cache != (Cache) NULL);
  cache_info=(CacheInfo *) cache;
  assert(cache_info->signature == MagickSignature);
  if (cache_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      cache_info->filename);
  return(cache_info->storage_class);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t C a c h e C o l o r s p a c e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetCacheColorspace() returns the class type of the pixel cache.
%
%  The format of the GetCacheColorspace() method is:
%
%      Colorspace GetCacheColorspace(Cache cache)
%
%  A description of each parameter follows:
%
%    o type: GetCacheColorspace returns DirectClass or PseudoClass.
%
%    o cache: The pixel cache.
%
*/
MagickExport ColorspaceType GetCacheColorspace(const Cache cache)
{
  CacheInfo
    *cache_info;

  assert(cache != (Cache) NULL);
  cache_info=(CacheInfo *) cache;
  assert(cache_info->signature == MagickSignature);
  if (cache_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      cache_info->filename);
  return(cache_info->colorspace);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t C a c h e I n f o                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetCacheInfo() initializes the Cache structure.
%
%  The format of the GetCacheInfo() method is:
%
%      void GetCacheInfo(Cache *cache)
%
%  A description of each parameter follows:
%
%    o cache: The pixel cache.
%
*/
MagickExport void GetCacheInfo(Cache *cache)
{
  CacheInfo
    *cache_info;

  assert(cache != (Cache) NULL);
  cache_info=(CacheInfo *) AcquireMagickMemory(sizeof(*cache_info));
  if (cache_info == (CacheInfo *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      strerror(errno));
  (void) ResetMagickMemory(cache_info,0,sizeof(*cache_info));
  cache_info->type=UndefinedCache;
  cache_info->colorspace=RGBColorspace;
  cache_info->reference_count=1;
  cache_info->file=(-1);
  cache_info->debug=IsEventLogging();
  cache_info->signature=MagickSignature;
  GetCacheMethods(&cache_info->methods);
  *cache=cache_info;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t C a c h e I n f o                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetCacheMethods() initializes the CacheMethods structure.
%
%  The format of the GetCacheInfo() method is:
%
%      void GetCacheMethods(CacheMethods *cache_methods)
%
%  A description of each parameter follows:
%
%    o cache_methods: Specifies a pointer to a CacheMethods structure.
%
*/
MagickExport void GetCacheMethods(CacheMethods *cache_methods)
{
  assert(cache_methods != (CacheMethods *) NULL);
  (void) ResetMagickMemory(cache_methods,0,sizeof(*cache_methods));
  cache_methods->acquire_pixel_handler=AcquirePixelCache;
  cache_methods->get_pixel_handler=GetPixelCache;
  cache_methods->set_pixel_handler=SetPixelCache;
  cache_methods->sync_pixel_handler=SyncPixelCache;
  cache_methods->get_pixels_from_handler=GetPixelsFromCache;
  cache_methods->get_indexes_from_handler=GetIndexesFromCache;
  cache_methods->acquire_one_pixel_from_handler=AcquireOnePixelFromCache;
  cache_methods->get_one_pixel_from_handler=GetOnePixelFromCache;
  cache_methods->destroy_pixel_handler=DestroyPixelCache;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t C a c h e N e x u s                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetCacheNexus() gets pixels from the in-memory or disk pixel cache as
%  defined by the geometry parameters.   A pointer to the pixels is returned
%  if the pixels are transferred, otherwise a NULL is returned.
%
%  The format of the GetCacheNexus() method is:
%
%      PixelPacket *GetCacheNexus(Image *image,const long x,const long y,
%        const unsigned long columns,const unsigned long rows)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
%    o nexus: specifies which cache nexus to return.
%
*/
MagickExport PixelPacket *GetCacheNexus(Image *image,const long x,const long y,
  const unsigned long columns,const unsigned long rows,
  const unsigned long nexus)
{
  CacheInfo
    *cache_info;

  PixelPacket
    *pixels;

  MagickBooleanType
    status;

  /*
    Transfer pixels from the cache.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  pixels=SetCacheNexus(image,x,y,columns,rows,nexus);
  if (pixels == (PixelPacket *) NULL)
    return((PixelPacket *) NULL);
  if (IsNexusInCore(image->cache,nexus) != MagickFalse)
    return(pixels);
  status=ReadCachePixels(image->cache,nexus,&image->exception);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  if ((cache_info->storage_class == PseudoClass) ||
      (cache_info->colorspace == CMYKColorspace))
    if (ReadCacheIndexes(image->cache,nexus,&image->exception) == MagickFalse)
      status=MagickFalse;
  if (status == MagickFalse)
    return((PixelPacket *) NULL);
  return(pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e P i x e l s                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImagePixels() obtains a pixel region for read/write access. If the
%  region is successfully accessed, a pointer to a PixelPacket array
%  representing the region is returned, otherwise NULL is returned.
%
%  The returned pointer may point to a temporary working copy of the pixels
%  or it may point to the original pixels in memory. Performance is maximized
%  if the selected area is part of one row, or one or more full rows, since
%  then there is opportunity to access the pixels in-place (without a copy)
%  if the image is in RAM, or in a memory-mapped file. The returned pointer
%  should *never* be deallocated by the user.
%
%  Pixels accessed via the returned pointer represent a simple array of type
%  PixelPacket. If the image storage class is PsudeoClass, call GetIndexes()
%  after invoking GetImagePixels() to obtain the colormap indexes (of type
%  IndexPacket) corresponding to the region.  Once the PixelPacket (and/or
%  IndexPacket) array has been updated, the changes must be saved back to
%  the underlying image using SyncPixelCache() or they may be lost.
%
%  The format of the GetImagePixels() method is:
%
%      PixelPacket *GetImagePixels(Image *image,const long x,const long y,
%        const unsigned long columns,const unsigned long rows)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
*/
MagickExport PixelPacket *GetImagePixels(Image *image,const long x,const long y,
  const unsigned long columns,const unsigned long rows)
{
  CacheInfo
    *cache_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  if (cache_info->methods.get_pixel_handler == (GetPixelHandler) NULL)
    return((PixelPacket *) NULL);
  return(cache_info->methods.get_pixel_handler(image,x,y,columns,rows));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e V i r t u a l P i x e l M e t h o d                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageVirtualPixelMethod() gets the "virtual pixels" method for the
%  image.  A virtual pixel is any pixel access that is outside the boundaries
%  of the image cache.
%
%  The format of the GetImageVirtualPixelMethod() method is:
%
%      VirtualPixelMethod GetImageVirtualPixelMethod(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/
MagickExport VirtualPixelMethod GetImageVirtualPixelMethod(const Image *image)
{
  CacheInfo
    *cache_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  return(cache_info->virtual_pixel_method);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I n d e x e s                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetIndexes() returns the colormap indexes associated with the last call to
%  SetImagePixels() or GetImagePixels(). NULL is returned if colormap indexes
%  are not available.
%
%  The format of the GetIndexes() method is:
%
%      IndexPacket *GetIndexes(const Image *image)
%
%  A description of each parameter follows:
%
%    o indexes: GetIndexes() returns the indexes associated with the last
%      call to SetImagePixels() or GetImagePixels().
%
%    o image: The image.
%
*/
MagickExport IndexPacket *GetIndexes(const Image *image)
{
  CacheInfo
    *cache_info;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  if (cache_info->methods.get_indexes_from_handler ==
       (GetIndexesFromHandler) NULL)
    return((IndexPacket *) NULL);
  return(cache_info->methods.get_indexes_from_handler(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t I n d e x e s F r o m C a c h e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetIndexesFromCache() returns the indexes associated with the last call to
%  SetPixelCache() or GetPixelCache().
%
%  The format of the GetIndexesFromCache() method is:
%
%      IndexPacket *GetIndexesFromCache(const Image *image)
%
%  A description of each parameter follows:
%
%    o indexes: GetIndexesFromCache() returns the indexes associated with the
%      last call to SetPixelCache() or GetPixelCache().
%
%    o image: The image.
%
*/
static IndexPacket *GetIndexesFromCache(const Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  return(GetNexusIndexes(image->cache,0));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t N e x u s                                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetNexus() returns an available cache nexus.
%
%  The format of the GetNexus() method is:
%
%      MagickBooleanType GetNexus(Cache cache)
%
%  A description of each parameter follows:
%
%    o id:  GetNexus returns an available cache nexus slot.
%
%    o cache: The pixel cache.
%
*/
MagickExport unsigned long GetNexus(Cache cache)
{
  CacheInfo
    *cache_info;

  register long
    id;

  assert(cache != (Cache) NULL);
  cache_info=(CacheInfo *) cache;
  assert(cache_info->signature == MagickSignature);
  assert(cache_info->number_views != 0UL);
  if (cache_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      cache_info->filename);
  for (id=1; id < (long) cache_info->number_views; id++)
    if (cache_info->nexus_info[id].available != MagickFalse)
      {
        cache_info->nexus_info[id].available=MagickFalse;
        return((unsigned long) id);
      }
  cache_info->number_views++;
  cache_info->nexus_info=(NexusInfo *) ResizeMagickMemory(
    cache_info->nexus_info,(size_t) cache_info->number_views*
    sizeof(*cache_info->nexus_info));
  if (cache_info->nexus_info == (NexusInfo *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,
      "MemoryAllocationFailed",strerror(errno));
  (void) ResetMagickMemory(&cache_info->nexus_info[id],0,
    sizeof(*cache_info->nexus_info));
  return((unsigned long) id);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t N e x u s I n d e x e s                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetNexusIndexes() returns the indexes associated with the specified cache
%  nexus.
%
%  The format of the GetNexusIndexes() method is:
%
%      IndexPacket *GetNexusIndexes(const Cache cache,const unsigned long nexus)
%
%  A description of each parameter follows:
%
%    o indexes: GetNexusIndexes returns the indexes associated with the
%      specified cache nexus.
%
%    o cache: The pixel cache.
%
%    o nexus: specifies which cache nexus to return the colormap indexes.
%
*/
MagickExport IndexPacket *GetNexusIndexes(const Cache cache,
  const unsigned long nexus)
{
  CacheInfo
    *cache_info;

  register NexusInfo
    *nexus_info;

  if (cache == (Cache) NULL)
    return((IndexPacket *) NULL);
  cache_info=(CacheInfo *) cache;
  assert(cache_info->signature == MagickSignature);
  if (cache_info->storage_class == UndefinedClass)
    return((IndexPacket *) NULL);
  nexus_info=cache_info->nexus_info+nexus;
  return(nexus_info->indexes);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t N e x u s P i x e l s                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetNexusPixels() returns the pixels associated with the specified cache
%  nexus.
%
%  The format of the GetNexusPixels() method is:
%
%      PixelPacket *GetNexusPixels(const Cache cache,const unsigned long nexus)
%
%  A description of each parameter follows:
%
%    o pixels: GetNexusPixels returns the pixels associated with the specified
%      cache nexus.
%
%    o nexus: specifies which cache nexus to return the pixels.
%
*/
MagickExport PixelPacket *GetNexusPixels(const Cache cache,
  const unsigned long nexus)
{
  CacheInfo
    *cache_info;

  register NexusInfo
    *nexus_info;

  if (cache == (Cache) NULL)
    return((PixelPacket *) NULL);
  cache_info=(CacheInfo *) cache;
  assert(cache_info->signature == MagickSignature);
  if (cache_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      cache_info->filename);
  if (cache_info->storage_class == UndefinedClass)
    return((PixelPacket *) NULL);
  nexus_info=cache_info->nexus_info+nexus;
  return(nexus_info->pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t O n e P i x e l                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOnePixel() returns a single pixel at the specified (x,y) location.
%  The image background color is returned if an error occurs.
%
%  The format of the GetOnePixel() method is:
%
%      PixelPacket *GetOnePixel(const Image image,const long x,const long y)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o x,y:  These values define the location of the pixel to return.
%
*/
MagickExport PixelPacket GetOnePixel(Image *image,const long x,const long y)
{
  CacheInfo
    *cache_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  if (cache_info->methods.get_one_pixel_from_handler ==
      (GetOnePixelFromHandler) NULL)
    return(image->background_color);
  return(cache_info->methods.get_one_pixel_from_handler(image,x,y));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t O n e P i x e l F r o m C a c h e                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetOnePixelFromCache() returns a single pixel at the specified (x,y)
%  location.  The image background color is returned if an error occurs.
%
%  The format of the GetOnePixelFromCache() method is:
%
%      PixelPacket GetOnePixelFromCache(const Image image,const long x,
%        const long y)
%
%  A description of each parameter follows:
%
%    o pixels: GetOnePixelFromCache returns a pixel at the specified (x,y)
%      location.
%
%    o image: The image.
%
%    o x,y:  These values define the location of the pixel to return.
%
*/
static PixelPacket GetOnePixelFromCache(Image *image,const long x,const long y)
{
  register PixelPacket
    *pixel;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  pixel=GetPixelCache(image,x,y,1UL,1UL);
  if (pixel != (PixelPacket *) NULL)
    return(*pixel);
  return(image->background_color);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t P i x e l s                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPixels() returns the pixels associated with the last call to
%  SetImagePixels() or GetImagePixels().
%
%  The format of the GetPixels() method is:
%
%      PixelPacket *GetPixels(const Image image)
%
%  A description of each parameter follows:
%
%    o pixels: GetPixels() returns the pixels associated with the last call
%      to SetImagePixels() or GetImagePixels().
%
%    o image: The image.
%
*/
MagickExport PixelPacket *GetPixels(const Image *image)
{
  CacheInfo
    *cache_info;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  if (cache_info->methods.get_pixels_from_handler ==
      (GetPixelsFromHandler) NULL)
    return((PixelPacket *) NULL);
  return(cache_info->methods.get_pixels_from_handler(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t P i x e l C a c h e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPixelCache() gets pixels from the in-memory or disk pixel cache as
%  defined by the geometry parameters.   A pointer to the pixels is returned
%  if the pixels are transferred, otherwise a NULL is returned.
%
%  The format of the GetPixelCache() method is:
%
%      PixelPacket *GetPixelCache(Image *image,const long x,const long y,
%        const unsigned long columns,const unsigned long rows)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
*/
static PixelPacket *GetPixelCache(Image *image,const long x,const long y,
  const unsigned long columns,const unsigned long rows)
{
  return(GetCacheNexus(image,x,y,columns,rows,0));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t P i x e l C a c h e A r e a                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPixelCacheArea() returns the area (width * height in pixels) consumed by
%  the current pixel cache.
%
%  The format of the GetPixelCacheArea() method is:
%
%      MatgickSizeType GetPixelCacheArea(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/
MagickExport MagickSizeType GetPixelCacheArea(const Image *image)
{
  CacheInfo
    *cache_info;

  register NexusInfo
    *nexus_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  if (cache_info->nexus_info == (NexusInfo *) NULL)
    return((MagickSizeType) cache_info->columns*cache_info->rows);
  nexus_info=cache_info->nexus_info+cache_info->id;
  return((MagickSizeType) nexus_info->columns*nexus_info->rows);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t P i x e l s F r o m C a c h e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPixelsFromCache() returns the pixels associated with the last call to
%  the SetPixelCache() or GetPixelCache() methods.
%
%  The format of the GetPixelsFromCache() method is:
%
%      PixelPacket *GetPixelsFromCache(const Image image)
%
%  A description of each parameter follows:
%
%    o pixels: GetPixelsFromCache() returns the pixels associated with the
%      last call to SetPixelCache() or GetPixelCache().
%
%    o image: The image.
%
*/
static PixelPacket *GetPixelsFromCache(const Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  return(GetNexusPixels(image->cache,0));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M o d i f y C a c h e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ModifyCache() ensures that there is only a single reference to the pixel
%  cache to be modified, updating the provided cache pointer to point to
%  a clone of the original pixel cache if necessary.
%
%  The format of the ModifyCache method is:
%
%      MagickBooleanType ModifyCache(Image *image,const unsigned long nexus)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o nexus: specifies which cache nexus to acquire.
%
*/
static MagickBooleanType ModifyCache(Image *image,const unsigned long nexus)
{
  CacheInfo
    *cache_info,
    *clone_info;

  Image
    clone_image;

  MagickBooleanType
    status;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  if (cache_info->reference_count <= 1)
    {
      AcquireSemaphoreInfo(&cache_info->semaphore);
      if (cache_info->reference_count <= 1)
        {
          RelinquishSemaphoreInfo(cache_info->semaphore);
          return(MagickTrue);
        }
      RelinquishSemaphoreInfo(cache_info->semaphore);
    }
  AcquireSemaphoreInfo(&cache_info->semaphore);
  cache_info->reference_count--;
  clone_image=(*image);
  clone_info=(CacheInfo *) clone_image.cache;
  GetCacheInfo(&image->cache);
  SetImageVirtualPixelMethod(image,clone_info->virtual_pixel_method);
  status=OpenCache(image,IOMode,&image->exception);
  if (status != MagickFalse)
    {
      status=CloneCacheNexus(image->cache,clone_image.cache,nexus);
      status|=ClonePixelCache(image->cache,clone_image.cache,&image->exception);
    }
  image->depth=QuantumDepth;
  RelinquishSemaphoreInfo(cache_info->semaphore);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   O p e n C a c h e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OpenCache() allocates the pixel cache.  This includes defining the cache
%  dimensions, allocating space for the image pixels and optionally the
%  colormap indexes, and memory mapping the cache if it is disk based.  The
%  cache nexus array is initialized as well.
%
%  The format of the OpenCache() method is:
%
%      MagickBooleanType OpenCache(Image *image,const MapMode mode,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o mode: ReadMode, WriteMode, or IOMode.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

static inline void AcquireCachePixels(CacheInfo *cache_info)
{
  assert(cache_info != (CacheInfo *) NULL);
  assert(cache_info->length == (MagickSizeType) ((size_t) cache_info->length));
  cache_info->pixels=(PixelPacket *) MapBlob(-1,IOMode,0,(size_t)
    cache_info->length);
  if (cache_info->pixels != (PixelPacket *) NULL)
    {
      cache_info->mapped=MagickTrue;
      return;
    }
  cache_info->pixels=(PixelPacket *)
    AcquireMagickMemory((size_t) cache_info->length);
  cache_info->mapped=MagickFalse;
}

static MagickBooleanType ExtendCache(int file,MagickSizeType length)
{
  MagickOffsetType
    count,
    offset;

  offset=(MagickOffsetType) MagickSeek(file,0,SEEK_END);
  if (offset < 0)
    return(MagickFalse);
  if ((MagickSizeType) offset >= length)
    return(MagickTrue);
  count=WriteCacheRegion(file,(const unsigned char *) "",1,(MagickOffsetType)
    (length-1));
  if (count != (MagickOffsetType) 1)
    return(MagickFalse);
  return(MagickTrue);
}

static MagickBooleanType OpenCache(Image *image,const MapMode mode,
  ExceptionInfo *exception)
{
  char
    format[MaxTextExtent],
    message[MaxTextExtent];

  CacheInfo
    *cache_info,
    source_info;

  int
    file;

  MagickBooleanType
    status;

  MagickSizeType
    length,
    number_pixels;

  size_t
    packet_size;

  unsigned long
    columns;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (void *) NULL);
  if ((image->columns == 0) || (image->rows == 0))
    ThrowBinaryException(ResourceLimitError,"NoPixelsDefinedInCache",
      image->filename);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  source_info=(*cache_info);
  (void) FormatMagickString(cache_info->filename,MaxTextExtent,"%s[%ld]",
    image->filename,GetImageIndexInList(image));
  cache_info->rows=image->rows;
  cache_info->columns=image->columns;
  number_pixels=(MagickSizeType) cache_info->columns*cache_info->rows;
  if (cache_info->nexus_info == (NexusInfo *) NULL)
    {
      register long
        id;

      /*
        Allocate cache nexus.
      */
      cache_info->number_views=DefaultNumberCacheViews;
      cache_info->nexus_info=(NexusInfo *) AcquireMagickMemory((size_t)
        cache_info->number_views*sizeof(*cache_info->nexus_info));
      if (cache_info->nexus_info == (NexusInfo *) NULL)
        ThrowMagickFatalException(ResourceLimitFatalError,
          "MemoryAllocationFailed",strerror(errno));
      (void) ResetMagickMemory(cache_info->nexus_info,0,(size_t)
        cache_info->number_views*sizeof(*cache_info->nexus_info));
      for (id=1; id < (long) cache_info->number_views; id++)
        cache_info->nexus_info[id].available=MagickTrue;
    }
  packet_size=sizeof(PixelPacket);
  if ((image->storage_class == PseudoClass) ||
      (image->colorspace == CMYKColorspace))
    packet_size+=sizeof(IndexPacket);
  length=number_pixels*packet_size;
  columns=(unsigned long) (length/cache_info->rows/packet_size);
  if (cache_info->columns != columns)
    ThrowBinaryException(ResourceLimitError,"PixelCacheAllocationFailed",
      image->filename);
  cache_info->length=length;
  status=AcquireMagickResource(AreaResource,cache_info->length);
  length=number_pixels*(sizeof(PixelPacket)+sizeof(IndexPacket));
  if ((status != MagickFalse) && (length == (MagickSizeType) ((size_t) length)))
    {
      status=AcquireMagickResource(MemoryResource,cache_info->length);
      if (((cache_info->type == UndefinedCache) && (status != MagickFalse)) ||
          (cache_info->type == MemoryCache))
        {
          AcquireCachePixels(cache_info);
          if (cache_info->pixels == (PixelPacket *) NULL)
            {
              RelinquishMagickResource(MemoryResource,cache_info->length);
              cache_info->pixels=source_info.pixels;
            }
          else
            {
              /*
                Create memory pixel cache.
              */
              FormatSize(cache_info->length,format);
              (void) FormatMagickString(message,MaxTextExtent,
                "open %s (memory, %s)",cache_info->filename,format);
              if (image->debug != MagickFalse)
                (void) LogMagickEvent(CacheEvent,GetMagickModule(),"%s",
                  message);
              cache_info->storage_class=image->storage_class;
              cache_info->colorspace=image->colorspace;
              cache_info->type=MemoryCache;
              cache_info->indexes=(IndexPacket *) NULL;
              if ((cache_info->storage_class == PseudoClass) ||
                  (cache_info->colorspace == CMYKColorspace))
                cache_info->indexes=(IndexPacket *)
                  (cache_info->pixels+number_pixels);
              if (source_info.storage_class != UndefinedClass)
                {
                  status|=ClonePixelCache(cache_info,&source_info,exception);
                  RelinquishCachePixels(&source_info);
                }
              return(MagickTrue);
            }
        }
      RelinquishMagickResource(MemoryResource,cache_info->length);
    }
  /*
    Create pixel cache on disk.
  */
  status=AcquireMagickResource(DiskResource,cache_info->length);
  if (status == MagickFalse)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),CacheError,
        "CacheResourcesExhausted","`%s'",image->filename);
      return(MagickFalse);
    }
  if (*cache_info->cache_filename == '\0')
    file=AcquireUniqueFileResource(cache_info->cache_filename);
  else
    file=OpenDiskCache(cache_info->cache_filename,mode);
  if (file == -1)
    {
      RelinquishMagickResource(DiskResource,cache_info->length);
      ThrowFileException(exception,CacheError,"UnableToOpenCache",
        image->filename);
      return(MagickFalse);
    }
  status=ExtendCache(file,(MagickSizeType) cache_info->offset+
    cache_info->length);
  if (status == MagickFalse)
    {
      file=close(file)-1;
      (void) RelinquishUniqueFileResource(cache_info->cache_filename);
      RelinquishMagickResource(DiskResource,cache_info->length);
      ThrowFileException(exception,CacheError,"UnableToExtendCache",
        image->filename);
      return(MagickFalse);
    }
  cache_info->storage_class=image->storage_class;
  cache_info->colorspace=image->colorspace;
  cache_info->file=file;
  length=number_pixels*(sizeof(PixelPacket)+sizeof(IndexPacket));
  status=AcquireMagickResource(AreaResource,cache_info->length);
  if ((status == MagickFalse) || (length != (MagickSizeType) ((size_t) length)))
    cache_info->type=DiskCache;
  else
    {
      status=AcquireMagickResource(MapResource,cache_info->length);
      if (((cache_info->type != UndefinedCache) || (status == MagickFalse)) &&
          (cache_info->type != MapCache) && (cache_info->type != MemoryCache))
        cache_info->type=DiskCache;
      else
        {
          cache_info->pixels=(PixelPacket *) MapBlob(file,mode,
            cache_info->offset,(size_t) cache_info->length);
          if (cache_info->pixels == (PixelPacket *) NULL)
            {
              RelinquishMagickResource(MapResource,cache_info->length);
              cache_info->pixels=source_info.pixels;
              cache_info->type=DiskCache;
            }
          else
            {
              /*
                Create file-backed memory-mapped pixel cache.
              */
              FormatSize(cache_info->length,format);
              (void) FormatMagickString(message,MaxTextExtent,
                "open %s (%s[%d], memory-mapped, %s)",cache_info->filename,
                cache_info->cache_filename,cache_info->file,format);
              if (image->debug != MagickFalse)
                (void) LogMagickEvent(CacheEvent,GetMagickModule(),"%s",
                  message);
              cache_info->file=close(cache_info->file)-1;
              cache_info->type=MapCache;
              cache_info->mapped=MagickTrue;
              cache_info->indexes=(IndexPacket *) NULL;
              if ((cache_info->storage_class == PseudoClass) ||
                  (cache_info->colorspace == CMYKColorspace))
                cache_info->indexes=(IndexPacket *)
                  (cache_info->pixels+number_pixels);
              if (source_info.type != UndefinedCache)
                {
                  status=ClonePixelCache(cache_info,&source_info,exception);
                  RelinquishCachePixels(&source_info);
                }
              return(MagickTrue);
            }
        }
      RelinquishMagickResource(MapResource,cache_info->length);
    }
  if ((source_info.type != UndefinedCache) && (mode != ReadMode))
    {
      status=ClonePixelCache(cache_info,&source_info,exception);
      RelinquishCachePixels(&source_info);
    }
  FormatSize(cache_info->length,format);
  (void) FormatMagickString(message,MaxTextExtent,"open %s (%s[%d], disk, %s)",
    cache_info->filename,cache_info->cache_filename,cache_info->file,format);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(CacheEvent,GetMagickModule(),"%s",message);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   P e r s i s t C a c h e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PersistCache() attaches to or initializes a persistent pixel cache.  A
%  persistent pixel cache is one that resides on disk and is not destroyed
%  when the program exits.
%
%  The format of the PersistCache() method is:
%
%      MagickBooleanType PersistCache(Image *image,const char *filename,
%        const MagickBooleanType attach,MagickOffsetType *offset,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o filename: The persistent pixel cache filename.
%
%    o initialize: A value other than zero initializes the persistent pixel
%      cache.
%
%    o offset: The offset in the persistent cache to store pixels.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType PersistCache(Image *image,const char *filename,
  const MagickBooleanType attach,MagickOffsetType *offset,
  ExceptionInfo *exception)
{
  CacheInfo
    *cache_info;

  Image
    clone_image;

  long
    pagesize;

  MagickBooleanType
    status;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (void *) NULL);
  assert(filename != (const char *) NULL);
  assert(offset != (MagickOffsetType *) NULL);
  if (SyncCache(image) == MagickFalse)
    return(MagickFalse);
  pagesize=(-1);
#if defined(HAVE_SYSCONF) && defined(_SC_PAGESIZE)
  pagesize=sysconf(_SC_PAGESIZE);
#elif defined(HAVE_GETPAGESIZE) && defined(POSIX)
  pagesize=getpagesize();
#endif
  if (pagesize <= 0)
    pagesize=4096;
  cache_info=(CacheInfo *) image->cache;
  if (attach != MagickFalse)
    {
      /*
        Attach persistent pixel cache.
      */
      (void) CopyMagickString(cache_info->cache_filename,filename,
        MaxTextExtent);
      cache_info->type=DiskCache;
      cache_info->offset=(*offset);
      if (OpenCache(image,ReadMode,exception) == MagickFalse)
        return(MagickFalse);
      cache_info=(CacheInfo *) ReferenceCache(cache_info);
      *offset+=cache_info->length+pagesize-(cache_info->length % pagesize);
      if (image->debug != MagickFalse)
        (void) LogMagickEvent(CacheEvent,GetMagickModule(),
          "Attach persistent cache");
      return(MagickTrue);
    }
  AcquireSemaphoreInfo(&cache_info->semaphore);
  if ((cache_info->reference_count == 1) &&
      (cache_info->type != MemoryCache))
    {
      /*
        Usurp resident persistent pixel cache.
      */
      status=(MagickBooleanType) rename(cache_info->cache_filename,filename);
      if (status == 0)
        {
          (void) CopyMagickString(cache_info->cache_filename,filename,
            MaxTextExtent);
          RelinquishSemaphoreInfo(cache_info->semaphore);
          cache_info=(CacheInfo *) ReferenceCache(cache_info);
          *offset+=cache_info->length+pagesize-(cache_info->length % pagesize);
          if (image->debug != MagickFalse)
            (void) LogMagickEvent(CacheEvent,GetMagickModule(),
              "Usurp resident persistent cache");
          return(MagickTrue);
        }
    }
  RelinquishSemaphoreInfo(cache_info->semaphore);
  /*
    Attach persistent pixel cache.
  */
  clone_image=(*image);
  GetCacheInfo(&image->cache);
  cache_info=(CacheInfo *) ReferenceCache(image->cache);
  (void) CopyMagickString(cache_info->cache_filename,filename,MaxTextExtent);
  cache_info->type=DiskCache;
  cache_info->offset=(*offset);
  status=OpenCache(image,IOMode,exception);
  if (status != MagickFalse)
    {
      status=CloneCacheNexus(image->cache,clone_image.cache,0);
      status|=ClonePixelCache(image->cache,clone_image.cache,&image->exception);
    }
  *offset+=cache_info->length+pagesize-(cache_info->length % pagesize);
  clone_image.cache=DestroyCacheInfo(clone_image.cache);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e a d C a c h e I n d e x e s                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadCacheIndexes() reads colormap indexes from the specified region of the
%  pixel cache.
%
%  The format of the ReadCacheIndexes() method is:
%
%      MagickBooleanType ReadCacheIndexes(CacheInfo *cache_info,
%        const unsigned long nexus,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o cache_info: The pixel cache.
%
%    o nexus: specifies which cache nexus to read the colormap indexes.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
static MagickBooleanType ReadCacheIndexes(CacheInfo *cache_info,
  const unsigned long nexus,ExceptionInfo *exception)
{
  MagickOffsetType
    count,
    offset;

  MagickSizeType
    length,
    number_pixels;

  register IndexPacket
    *indexes;

  register long
    y;

  register NexusInfo
    *nexus_info;

  unsigned long
    rows;

  assert(cache_info != (CacheInfo *) NULL);
  assert(cache_info->signature == MagickSignature);
  if (cache_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      cache_info->filename);
  if ((cache_info->storage_class != PseudoClass) &&
      (cache_info->colorspace != CMYKColorspace))
    return(MagickFalse);
  nexus_info=cache_info->nexus_info+nexus;
  if (IsNexusInCore(cache_info,nexus) != MagickFalse)
    return(MagickTrue);
  if ((QuantumTick(nexus_info->x,cache_info->columns) != MagickFalse) &&
      (QuantumTick(nexus_info->y,cache_info->rows) != MagickFalse))
    (void) LogMagickEvent(CacheEvent,GetMagickModule(),"%lux%lu%+ld%+ld",
      nexus_info->columns,nexus_info->rows,nexus_info->x,nexus_info->y);
  offset=(MagickOffsetType) nexus_info->y*cache_info->columns+nexus_info->x;
  length=(MagickSizeType) nexus_info->columns*sizeof(IndexPacket);
  rows=nexus_info->rows;
  number_pixels=length*rows;
  if ((cache_info->columns == nexus_info->columns) &&
      (number_pixels == (MagickSizeType) ((size_t) number_pixels)))
    {
      length=number_pixels;
      rows=1UL;
    }
  indexes=nexus_info->indexes;
  if (cache_info->type != DiskCache)
    {
      /*
        Read indexes from memory.
      */
      for (y=0; y < (long) rows; y++)
      {
        (void) CopyMagickMemory(indexes,cache_info->indexes+offset,(size_t)
          length);
        indexes+=nexus_info->columns;
        offset+=cache_info->columns;
      }
      return(MagickTrue);
    }
  /*
    Read indexes from disk.
  */
  if (cache_info->file == -1)
    {
      cache_info->file=OpenDiskCache(cache_info->cache_filename,IOMode);
      if (cache_info->file == -1)
        {
          ThrowFileException(exception,FileOpenError,"UnableToOpenFile",
            cache_info->cache_filename);
          return(MagickFalse);
        }
    }
  number_pixels=(MagickSizeType) cache_info->columns*cache_info->rows;
  for (y=0; y < (long) rows; y++)
  {
    count=ReadCacheRegion(cache_info->file,(unsigned char *) indexes,length,
      cache_info->offset+number_pixels*sizeof(PixelPacket)+offset*
      sizeof(*indexes));
    if ((MagickSizeType) count < length)
      break;
    indexes+=nexus_info->columns;
    offset+=cache_info->columns;
  }
  if (y < (long) rows)
    {
      ThrowFileException(exception,CacheError,"UnableToReadPixelCache",
        cache_info->cache_filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   R e a d C a c h e P i x e l s                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadCachePixels() reads pixels from the specified region of the pixel cache.
%
%  The format of the ReadCachePixels() method is:
%
%      MagickBooleanType ReadCachePixels(CacheInfo *cache_info,
%        const unsigned long nexus,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o cache_info: The pixel cache.
%
%    o nexus: specifies which cache nexus to read the pixels.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
static MagickBooleanType ReadCachePixels(CacheInfo *cache_info,
  const unsigned long nexus,ExceptionInfo *exception)
{
  MagickOffsetType
    count,
    offset;

  MagickSizeType
    length,
    number_pixels;

  register long
    y;

  register NexusInfo
    *nexus_info;

  register PixelPacket
    *pixels;

  unsigned long
    rows;

  assert(cache_info != (CacheInfo *) NULL);
  assert(cache_info->signature == MagickSignature);
  if (cache_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      cache_info->filename);
  nexus_info=cache_info->nexus_info+nexus;
  if (IsNexusInCore(cache_info,nexus) != MagickFalse)
    return(MagickTrue);
  if ((QuantumTick(nexus_info->x,cache_info->columns) != MagickFalse) &&
      (QuantumTick(nexus_info->y,cache_info->rows) != MagickFalse))
    (void) LogMagickEvent(CacheEvent,GetMagickModule(),"%lux%lu%+ld%+ld",
      nexus_info->columns,nexus_info->rows,nexus_info->x,nexus_info->y);
  offset=(MagickOffsetType) nexus_info->y*cache_info->columns+nexus_info->x;
  length=(MagickSizeType) nexus_info->columns*sizeof(PixelPacket);
  rows=nexus_info->rows;
  number_pixels=length*rows;
  if ((cache_info->columns == nexus_info->columns) &&
      (number_pixels == (MagickSizeType) ((size_t) number_pixels)))
    {
      length=number_pixels;
      rows=1UL;
    }
  pixels=nexus_info->pixels;
  if (cache_info->type != DiskCache)
    {
      /*
        Read pixels from memory.
      */
      for (y=0; y < (long) rows; y++)
      {
        (void) CopyMagickMemory(pixels,cache_info->pixels+offset,(size_t)
          length);
        pixels+=nexus_info->columns;
        offset+=cache_info->columns;
      }
      return(MagickTrue);
    }
  /*
    Read pixels from disk.
  */
  if (cache_info->file == -1)
    {
      cache_info->file=OpenDiskCache(cache_info->cache_filename,IOMode);
      if (cache_info->file == -1)
        {
          ThrowFileException(exception,FileOpenError,"UnableToOpenFile",
            cache_info->cache_filename);
          return(MagickFalse);
        }
    }
  for (y=0; y < (long) rows; y++)
  {
    count=ReadCacheRegion(cache_info->file,(unsigned char *) pixels,length,
      cache_info->offset+offset*sizeof(*pixels));
    if ((MagickSizeType) count < length)
      break;
    pixels+=nexus_info->columns;
    offset+=cache_info->columns;
  }
  if (y < (long) rows)
    {
      ThrowFileException(exception,CacheError,"UnableToReadPixelCache",
        cache_info->cache_filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e f e r e n c e C a c h e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReferenceCache() increments the reference count associated with the pixel
%  cache returning a pointer to the cache.
%
%  The format of the ReferenceCache method is:
%
%      Cache ReferenceCache(Cache cache_info)
%
%  A description of each parameter follows:
%
%    o cache_info: The pixel cache.
%
*/
MagickExport Cache ReferenceCache(Cache cache)
{
  CacheInfo
    *cache_info;

  assert(cache != (Cache *) NULL);
  cache_info=(CacheInfo *) cache;
  assert(cache_info->signature == MagickSignature);
  if (cache_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      cache_info->filename);
  AcquireSemaphoreInfo(&cache_info->semaphore);
  cache_info->reference_count++;
  RelinquishSemaphoreInfo(cache_info->semaphore);
  return(cache_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e l i n q u i s h C a c h e R e s o u r c e s                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RelinquishCacheResources() relinquishes cache resources.
%
%  The format of the RelinquishCacheResources() method is:
%
%      void RelinquishCacheResources(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/
MagickExport void RelinquishCacheResources(const Image *image)
{
  CacheInfo
    *cache_info;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  if (cache_info->type != DiskCache)
    return;
  if (cache_info->file != -1)
    (void) close(cache_info->file);
  cache_info->file=(-1);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S e t C a c h e N e x u s                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetCacheNexus() allocates an area to store image pixels as defined by the
%  region rectangle and returns a pointer to the area.  This area is
%  subsequently transferred from the pixel cache with SyncPixelCache().  A
%  pointer to the pixels is returned if the pixels are transferred, otherwise
%  a NULL is returned.
%
%  The format of the SetCacheNexus() method is:
%
%      PixelPacket *SetCacheNexus(Image *image,const long x,const long y,
%        const unsigned long columns,const unsigned long rows,
%        const unsigned long nexus)
%
%  A description of each parameter follows:
%
%    o pixels: SetCacheNexus() returns a pointer to the pixels if they are
%      transferred, otherwise a NULL is returned.
%
%    o image: The image.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
%    o nexus: specifies which cache nexus to set.
%
*/
MagickExport PixelPacket *SetCacheNexus(Image *image,const long x,const long y,
  const unsigned long columns,const unsigned long rows,
  const unsigned long nexus)
{
  CacheInfo
    *cache_info;

  MagickOffsetType
    offset;

  MagickSizeType
    number_pixels;

  RectangleInfo
    region;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  if (ModifyCache(image,nexus) == MagickFalse)
    return((PixelPacket *) NULL);
  if (SyncCache(image) == MagickFalse)
    return((PixelPacket *) NULL);
  /*
    Validate pixel cache geometry.
  */
  cache_info=(CacheInfo *) image->cache;
  offset=(MagickOffsetType) y*cache_info->columns+x;
  if (offset < 0)
    return((PixelPacket *) NULL);
  number_pixels=(MagickSizeType) cache_info->columns*cache_info->rows;
  offset+=(MagickOffsetType) (rows-1)*cache_info->columns+columns-1;
  if ((MagickSizeType) offset >= number_pixels)
    return((PixelPacket *) NULL);
  /*
    Return pixel cache.
  */
  region.x=x;
  region.y=y;
  region.width=columns;
  region.height=rows;
  return(SetNexus(image,&region,nexus));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e P i x e l s                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImagePixels() initializes a pixel region for write-only access.
%  If the region is successfully intialized a pointer to a PixelPacket
%  array representing the region is returned, otherwise NULL is returned.
%  The returned pointer may point to a temporary working buffer for the
%  pixels or it may point to the final location of the pixels in memory.
%
%  Write-only access means that any existing pixel values corresponding to
%  the region are ignored.  This is useful while the initial image is being
%  created from scratch, or if the existing pixel values are to be
%  completely replaced without need to refer to their pre-existing values.
%  The application is free to read and write the pixel buffer returned by
%  SetImagePixels() any way it pleases. SetImagePixels() does not initialize
%  the pixel array values. Initializing pixel array values is the
%  application's responsibility.
%
%  Performance is maximized if the selected area is part of one row, or
%  one or more full rows, since then there is opportunity to access the
%  pixels in-place (without a copy) if the image is in RAM, or in a
%  memory-mapped file. The returned pointer should *never* be deallocated
%  by the user.
%
%  Pixels accessed via the returned pointer represent a simple array of type
%  PixelPacket. If the image storage class is PsudeoClass, call GetIndexes()
%  after invoking GetImagePixels() to obtain the colormap indexes (of type
%  IndexPacket) corresponding to the region.  Once the PixelPacket (and/or
%  IndexPacket) array has been updated, the changes must be saved back to
%  the underlying image using SyncPixelCache() or they may be lost.

%
%  The format of the SetImagePixels() method is:
%
%      PixelPacket *SetImagePixels(Image *image,const long x,const long y,
%        const unsigned long columns,const unsigned long rows)
%
%  A description of each parameter follows:
%
%    o pixels: SetImagePixels returns a pointer to the pixels if they are
%      transferred, otherwise a NULL is returned.
%
%    o image: The image.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
*/
MagickExport PixelPacket *SetImagePixels(Image *image,const long x,const long y,
  const unsigned long columns,const unsigned long rows)
{
  CacheInfo
    *cache_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  if (cache_info->methods.set_pixel_handler == (SetPixelHandler) NULL)
    return((PixelPacket *) NULL);
  return(cache_info->methods.set_pixel_handler(image,x,y,columns,rows));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e V i r t u a l P i x e l M e t h o d                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageVirtualPixelMethod() sets the "virtual pixels" method for the
%  image.  A virtual pixel is any pixel access that is outside the boundaries
%  of the image cache.
%
%  The format of the SetImageVirtualPixelMethod() method is:
%
%      MagickBooleanType SetImageVirtualPixelMethod(const Image *image,
%        const VirtualPixelMethod method)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o method: choose from these access types:
%
%        EdgeVirtualPixelMethod:  the edge pixels of the image extend
%        infinitely.  Any pixel outside the image is assigned the same value as
%        the pixel at the edge closest to it.
%
%        TileVirtualPixelMethod:  the image extends periodically or tiled.  The
%        pixels wrap around the edges of the image.
%
%        MirrorVirtualPixelMethod:  mirror the image at the boundaries.
%
%        ConstantVirtualPixelMethod:  every value outside the image is a
%        constant as defines by the pixel parameter.
%
*/
MagickExport MagickBooleanType SetImageVirtualPixelMethod(const Image *image,
  const VirtualPixelMethod method)
{
  CacheInfo
    *cache_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  cache_info->virtual_pixel_method=method;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S e t N e x u s                                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetNexus() defines the region of the cache for the specified cache nexus.
%
%  The format of the SetNexus() method is:
%
%      PixelPacket SetNexus(const Image *image,const RectangleInfo *region,
%        const unsigned long nexus)
%
%  A description of each parameter follows:
%
%    o pixels: SetNexus() returns a pointer to the pixels associated with
%      the specified cache nexus.
%
%    o image: The image.
%
%    o nexus: specifies which cache nexus to set.
%
%    o region: A pointer to the RectangleInfo structure that defines the
%      region of this particular cache nexus.
%
*/
static PixelPacket *SetNexus(const Image *image,const RectangleInfo *region,
  const unsigned long nexus)
{
  CacheInfo
    *cache_info;

  MagickOffsetType
    offset;

  MagickSizeType
    length,
    number_pixels;

  register NexusInfo
    *nexus_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  cache_info->id=nexus;
  nexus_info=cache_info->nexus_info+nexus;
  nexus_info->columns=region->width;
  nexus_info->rows=region->height;
  nexus_info->x=region->x;
  nexus_info->y=region->y;
  if ((cache_info->type != DiskCache) && (image->clip_mask == (Image *) NULL))
    {
      offset=(MagickOffsetType) nexus_info->y*cache_info->columns+nexus_info->x;
      length=(MagickSizeType) (nexus_info->rows-1)*cache_info->columns+
        nexus_info->columns-1;
      number_pixels=(MagickSizeType) cache_info->columns*cache_info->rows;
      if ((offset >= 0) && (((MagickSizeType) offset+length) < number_pixels))
        if ((((unsigned long) (nexus_info->x+nexus_info->columns) <=
            cache_info->columns) && (nexus_info->rows == 1UL)) ||
            ((nexus_info->x == 0) &&
            ((nexus_info->columns % cache_info->columns) == 0)))
          {
            /*
              Pixels are accessed directly from memory.
            */
            nexus_info->pixels=cache_info->pixels+offset;
            nexus_info->indexes=(IndexPacket *) NULL;
            if ((cache_info->storage_class == PseudoClass) ||
                (cache_info->colorspace == CMYKColorspace))
              nexus_info->indexes=cache_info->indexes+offset;
            return(nexus_info->pixels);
          }
    }
  /*
    Pixels are stored in a cache area until they are synced to the cache.
  */
  number_pixels=(MagickSizeType) nexus_info->columns*nexus_info->rows;
  length=number_pixels*sizeof(PixelPacket);
  if ((cache_info->storage_class == PseudoClass) ||
      (cache_info->colorspace == CMYKColorspace))
    length+=number_pixels*sizeof(IndexPacket);
  length=Max(length,cache_info->columns*(sizeof(PixelPacket)+
    sizeof(IndexPacket)));
  if (nexus_info->cache == (PixelPacket *) NULL)
    {
      nexus_info->length=length;
      AcquireNexusPixels(nexus_info);
    }
  else
    if (nexus_info->length < length)
      {
        RelinquishNexusPixels(nexus_info);
        nexus_info->length=length;
        AcquireNexusPixels(nexus_info);
      }
  nexus_info->pixels=nexus_info->cache;
  nexus_info->indexes=(IndexPacket *) NULL;
  if ((cache_info->storage_class == PseudoClass) ||
      (cache_info->colorspace == CMYKColorspace))
    nexus_info->indexes=(IndexPacket *) (nexus_info->pixels+number_pixels);
  return(nexus_info->pixels);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S e t P i x e l C a c h e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetPixelCache() allocates an area to store image pixels as defined
%  by the region rectangle and returns a pointer to the area.  This area is
%  subsequently transferred from the pixel cache with SyncPixelCache().  A
%  pointer to the pixels is returned if the pixels are transferred, otherwise
%  a NULL is returned.
%
%  The format of the SetPixelCache() method is:
%
%      PixelPacket *SetPixelCache(Image *image,const long x,const long y,
%        const unsigned long columns,const unsigned long rows)
%
%  A description of each parameter follows:
%
%    o pixels: SetPixelCache() returns a pointer to the pixels if they are
%      transferred, otherwise a NULL is returned.
%
%    o image: The image.
%
%    o x,y,columns,rows:  These values define the perimeter of a region of
%      pixels.
%
*/
static PixelPacket *SetPixelCache(Image *image,const long x,const long y,
  const unsigned long columns,const unsigned long rows)
{
  return(SetCacheNexus(image,x,y,columns,rows,0));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S e t P i x e l C a c h e M e t h o d s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetPixelCacheMethods() sets the image pixel methods to the specified ones.
%
%  The format of the SetPixelCacheMethods() method is:
%
%      SetPixelCacheMethods(Cache *,CacheMethods *cache_methods)
%
%  A description of each parameter follows:
%
%    o cache: The pixel cache.
%
%    o cache_methods: Specifies a pointer to a CacheMethods structure.
%
*/
MagickExport void SetPixelCacheMethods(Cache cache,CacheMethods *cache_methods)
{
  CacheInfo
    *cache_info;

  /*
    Set image pixel methods.
  */
  assert(cache != (Cache) NULL);
  assert(cache_methods != (CacheMethods *) NULL);
  cache_info=(CacheInfo *) cache;
  assert(cache_info->signature == MagickSignature);
  if (cache_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      cache_info->filename);
  if (cache_methods->acquire_pixel_handler != (AcquirePixelHandler) NULL)
    cache_info->methods.acquire_pixel_handler=
      cache_methods->acquire_pixel_handler;
  if (cache_methods->get_pixel_handler != (GetPixelHandler) NULL)
    cache_info->methods.get_pixel_handler=cache_methods->get_pixel_handler;
  if (cache_methods->set_pixel_handler != (SetPixelHandler) NULL)
    cache_info->methods.set_pixel_handler=cache_methods->set_pixel_handler;
  if (cache_methods->sync_pixel_handler != (SyncPixelHandler) NULL)
    cache_info->methods.sync_pixel_handler=cache_methods->sync_pixel_handler;
  if (cache_methods->get_pixels_from_handler != (GetPixelsFromHandler) NULL)
    cache_info->methods.get_pixels_from_handler=
      cache_methods->get_pixels_from_handler;
  if (cache_methods->get_indexes_from_handler != (GetIndexesFromHandler) NULL)
    cache_info->methods.get_indexes_from_handler=
      cache_methods->get_indexes_from_handler;
  if (cache_methods->acquire_one_pixel_from_handler != (AcquireOnePixelFromHandler) NULL)
    cache_info->methods.acquire_one_pixel_from_handler=
      cache_methods->acquire_one_pixel_from_handler;
  if (cache_methods->get_one_pixel_from_handler != (GetOnePixelFromHandler) NULL)
    cache_info->methods.get_one_pixel_from_handler=
      cache_methods->get_one_pixel_from_handler;
  if (cache_methods->destroy_pixel_handler != (DestroyPixelHandler) NULL)
    cache_info->methods.destroy_pixel_handler=
      cache_methods->destroy_pixel_handler;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S y n c C a c h e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncCache() synchronizes the image with the pixel cache.
%
%  The format of the SyncCache() method is:
%
%      MagickBooleanType SyncCache(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/
MagickExport MagickBooleanType SyncCache(Image *image)
{
  CacheInfo
    *cache_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (void *) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  if ((image->storage_class != cache_info->storage_class) ||
      (image->colorspace != cache_info->colorspace) ||
      (image->columns != cache_info->columns) ||
      (image->rows != cache_info->rows) || (cache_info->number_views == 0))
    if (OpenCache(image,IOMode,&image->exception) == MagickFalse)
      return(MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S y n c C a c h e N e x u s                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncCacheNexus() saves the image pixels to the in-memory or disk cache.
%  The method returns MagickTrue if the pixel region is synced, otherwise
%  MagickFalse.
%
%  The format of the SyncCacheNexus() method is:
%
%      MagickBooleanType SyncCacheNexus(Image *image,const unsigned long nexus)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o nexus: specifies which cache nexus to sync.
%
*/
MagickExport MagickBooleanType SyncCacheNexus(Image *image,
  const unsigned long nexus)
{
  CacheInfo
    *cache_info;

  MagickBooleanType
    status;

  /*
    Transfer pixels to the cache.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->cache == (Cache) NULL)
    ThrowBinaryException(CacheError,"PixelCacheIsNotOpen",image->filename);
  image->taint=MagickTrue;
  if (IsNexusInCore(image->cache,nexus) != MagickFalse)
    return(MagickTrue);
  if (image->clip_mask != (Image *) NULL)
    if (ClipCacheNexus(image,nexus) == MagickFalse)
      return(MagickFalse);
  status=WriteCachePixels(image->cache,nexus,&image->exception);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  if ((cache_info->storage_class == PseudoClass) ||
      (cache_info->colorspace == CMYKColorspace))
    if (WriteCacheIndexes(image->cache,nexus,&image->exception) == MagickFalse)
      status=MagickFalse;
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S y n c I m a g e P i x e l s                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncImagePixels() saves the image pixels to the in-memory or disk cache. The
%  method returns MagickTrue if the pixel region is synced, otherwise
%  MagickFalse.
%
%  The format of the SyncImagePixels() method is:
%
%      MagickBooleanType SyncImagePixels(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/
MagickExport MagickBooleanType SyncImagePixels(Image *image)
{
  CacheInfo
    *cache_info;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(image->cache != (Cache) NULL);
  cache_info=(CacheInfo *) image->cache;
  assert(cache_info->signature == MagickSignature);
  if (cache_info->methods.sync_pixel_handler == (SyncPixelHandler) NULL)
    return(MagickFalse);
  return(cache_info->methods.sync_pixel_handler(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S y n c P i x e l C a c h e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncPixelCache() saves the image pixels to the in-memory or disk cache.
%  The method returns MagickTrue if the pixel region is synced, otherwise
%  MagickFalse.
%
%  The format of the SyncPixelCache() method is:
%
%      MagickBooleanType SyncPixelCache(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%
*/
static MagickBooleanType SyncPixelCache(Image *image)
{
  return(SyncCacheNexus(image,0));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   W r i t e C a c h e I n d e x e s                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteCacheIndexes() writes the colormap indexes to the specified region of
%  the pixel cache.
%
%  The format of the WriteCacheIndexes() method is:
%
%      MagickBooleanType WriteCacheIndexes(CacheInfo *cache_info,
%        const unsigned long nexus,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o cache_info: The pixel cache.
%
%    o nexus: specifies which cache nexus to write the colormap indexes.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
static MagickBooleanType WriteCacheIndexes(CacheInfo *cache_info,
  const unsigned long nexus,ExceptionInfo *exception)
{
  MagickOffsetType
    count,
    offset;

  MagickSizeType
    length,
    number_pixels;

  register IndexPacket
    *indexes;

  register long
    y;

  register NexusInfo
    *nexus_info;

  unsigned long
    rows;

  assert(cache_info != (CacheInfo *) NULL);
  assert(cache_info->signature == MagickSignature);
  if (cache_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      cache_info->filename);
  if ((cache_info->storage_class != PseudoClass) &&
      (cache_info->colorspace != CMYKColorspace))
    return(MagickFalse);
  nexus_info=cache_info->nexus_info+nexus;
  if (IsNexusInCore(cache_info,nexus) != MagickFalse)
    return(MagickTrue);
  if ((QuantumTick(nexus_info->x,cache_info->columns) != MagickFalse) &&
      (QuantumTick(nexus_info->y,cache_info->rows) != MagickFalse))
    (void) LogMagickEvent(CacheEvent,GetMagickModule(),"%lux%lu%+ld%+ld",
      nexus_info->columns,nexus_info->rows,nexus_info->x,nexus_info->y);
  offset=(MagickOffsetType) nexus_info->y*cache_info->columns+nexus_info->x;
  length=(MagickSizeType) nexus_info->columns*sizeof(IndexPacket);
  rows=nexus_info->rows;
  number_pixels=(MagickSizeType) length*rows;
  if ((cache_info->columns == nexus_info->columns) &&
      (number_pixels == (MagickSizeType) ((size_t) number_pixels)))
    {
      length=number_pixels;
      rows=1UL;
    }
  indexes=nexus_info->indexes;
  if (cache_info->type != DiskCache)
    {
      /*
        Write indexes to memory.
      */
      for (y=0; y < (long) rows; y++)
      {
        (void) CopyMagickMemory(cache_info->indexes+offset,indexes,(size_t)
          length);
        indexes+=nexus_info->columns;
        offset+=cache_info->columns;
      }
      return(MagickTrue);
    }
  /*
    Write indexes to disk.
  */
  if (cache_info->file == -1)
    {
      cache_info->file=OpenDiskCache(cache_info->cache_filename,IOMode);
      if (cache_info->file == -1)
        {
          ThrowFileException(exception,FileOpenError,"UnableToOpenFile",
            cache_info->cache_filename);
          return(MagickFalse);
        }
    }
  number_pixels=(MagickSizeType) cache_info->columns*cache_info->rows;
  for (y=0; y < (long) rows; y++)
  {
    count=WriteCacheRegion(cache_info->file,(unsigned char *) indexes,length,
      cache_info->offset+number_pixels*sizeof(PixelPacket)+offset*
      sizeof(*indexes));
    if ((MagickSizeType) count < length)
      break;
    indexes+=nexus_info->columns;
    offset+=cache_info->columns;
  }
  if (y < (long) rows)
    {
      ThrowFileException(exception,CacheError,"UnableToWritePixelCache",
        cache_info->cache_filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   W r i t e C a c h e P i x e l s                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteCachePixels() writes image pixels to the specified region of the pixel
%  cache.
%
%  The format of the WriteCachePixels() method is:
%
%      MagickBooleanType WriteCachePixels(CacheInfo *cache_info,
%        const unsigned long nexus,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o cache_info: The pixel cache.
%
%    o nexus: specifies which cache nexus to write the pixels.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
static MagickBooleanType WriteCachePixels(CacheInfo *cache_info,
  const unsigned long nexus,ExceptionInfo *exception)
{
  MagickOffsetType
    count,
    offset;

  MagickSizeType
    length,
    number_pixels;

  register long
    y;

  register NexusInfo
    *nexus_info;

  register PixelPacket
    *pixels;

  unsigned long
    rows;

  assert(cache_info != (CacheInfo *) NULL);
  assert(cache_info->signature == MagickSignature);
  if (cache_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      cache_info->filename);
  nexus_info=cache_info->nexus_info+nexus;
  if (IsNexusInCore(cache_info,nexus) != MagickFalse)
    return(MagickTrue);
  if ((QuantumTick(nexus_info->x,cache_info->columns) != MagickFalse) &&
      (QuantumTick(nexus_info->y,cache_info->rows) != MagickFalse))
    (void) LogMagickEvent(CacheEvent,GetMagickModule(),"%lux%lu%+ld%+ld",
      nexus_info->columns,nexus_info->rows,nexus_info->x,nexus_info->y);
  offset=(MagickOffsetType) nexus_info->y*cache_info->columns+nexus_info->x;
  length=(MagickSizeType) nexus_info->columns*sizeof(PixelPacket);
  rows=nexus_info->rows;
  number_pixels=length*rows;
  if ((cache_info->columns == nexus_info->columns) &&
      (number_pixels == (MagickSizeType) ((size_t) number_pixels)))
    {
      length=number_pixels;
      rows=1UL;
    }
  pixels=nexus_info->pixels;
  if (cache_info->type != DiskCache)
    {
      /*
        Write pixels to memory.
      */
      for (y=0; y < (long) rows; y++)
      {
        (void) CopyMagickMemory(cache_info->pixels+offset,pixels,(size_t)
          length);
        pixels+=nexus_info->columns;
        offset+=cache_info->columns;
      }
      return(MagickTrue);
    }
  /*
    Write pixels to disk.
  */
  if (cache_info->file == -1)
    {
      cache_info->file=OpenDiskCache(cache_info->cache_filename,IOMode);
      if (cache_info->file == -1)
        {
          ThrowFileException(exception,FileOpenError,"UnableToOpenFile",
            cache_info->cache_filename);
          return(MagickFalse);
        }
    }
  for (y=0; y < (long) rows; y++)
  {
    count=WriteCacheRegion(cache_info->file,(unsigned char *) pixels,length,
      cache_info->offset+offset*sizeof(*pixels));
    if ((MagickSizeType) count < length)
      break;
    pixels+=nexus_info->columns;
    offset+=cache_info->columns;
  }
  if (y < (long) rows)
    {
      ThrowFileException(exception,CacheError,"UnableToWritePixelCache",
        cache_info->cache_filename);
      return(MagickFalse);
    }
  return(MagickTrue);
}
