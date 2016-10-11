/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                     IIIII  M   M   AAA    GGGG  EEEEE                       %
%                       I    MM MM  A   A  G      E                           %
%                       I    M M M  AAAAA  G  GG  EEE                         %
%                       I    M   M  A   A  G   G  E                           %
%                     IIIII  M   M  A   A   GGGG  EEEEE                       %
%                                                                             %
%                                                                             %
%                          ImageMagick Image Methods                          %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1992                                   %
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
#include "magick/attribute.h"
#include "magick/animate.h"
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/cache.h"
#include "magick/cache-private.h"
#include "magick/client.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/colorspace.h"
#include "magick/colorspace-private.h"
#include "magick/composite.h"
#include "magick/composite-private.h"
#include "magick/compress.h"
#include "magick/constitute.h"
#include "magick/deprecate.h"
#include "magick/display.h"
#include "magick/draw.h"
#include "magick/enhance.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/gem.h"
#include "magick/geometry.h"
#include "magick/list.h"
#include "magick/image-private.h"
#include "magick/magic.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/module.h"
#include "magick/monitor.h"
#include "magick/option.h"
#include "magick/paint.h"
#include "magick/pixel-private.h"
#include "magick/profile.h"
#include "magick/quantize.h"
#include "magick/random_.h"
#include "magick/segment.h"
#include "magick/semaphore.h"
#include "magick/signature.h"
#include "magick/string_.h"
#include "magick/timer.h"
#include "magick/utility.h"
#include "magick/version.h"
#include "magick/xwindow-private.h"

/*
  Constant declaration.
*/
const char
  *BackgroundColor = "#fff",  /* white */
  *BorderColor = "#dfdfdf",  /* gray */
  *DefaultTileFrame = "15x15+3+3",
  *DefaultTileGeometry = "120x120+4+3>",
  *DefaultTileLabel = "%f\n%wx%h\n%b",
  *ForegroundColor = "#000",  /* black */
  *LoadImageTag = "Load/Image",
  *LoadImagesTag = "Load/Images",
  *MatteColor = "#bdbdbd",  /* gray */
  *PSDensityGeometry = "72.0x72.0",
  *PSPageGeometry = "612x792>",
  *SaveImageTag = "Save/Image",
  *SaveImagesTag = "Save/Images";

const double
  DefaultResolution = 72.0;

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A l l o c a t e I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% AllocateImage() returns a pointer to an image structure initialized to
% default values.
%
%  The format of the AllocateImage method is:
%
%      Image *AllocateImage(const ImageInfo *image_info)
%
%  A description of each parameter follows:
%
%    o image_info: Many of the image default values are set from this
%      structure.  For example, filename, compression, depth, background color,
%      and others.
%
%
*/
MagickExport Image *AllocateImage(const ImageInfo *image_info)
{
  Image
    *allocate_image;

  /*
    Allocate image structure.
  */
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  allocate_image=(Image *) AcquireMagickMemory(sizeof(*allocate_image));
  if (allocate_image == (Image *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      image_info->filename);
  (void) ResetMagickMemory(allocate_image,0,sizeof(*allocate_image));
  /*
    Initialize Image structure.
  */
  (void) CopyMagickString(allocate_image->magick,"MIFF",MaxTextExtent);
  allocate_image->storage_class=DirectClass;
  allocate_image->depth=QuantumDepth;
  allocate_image->colorspace=RGBColorspace;
  allocate_image->interlace=NoInterlace;
  allocate_image->ticks_per_second=UndefinedTicksPerSecond;
  allocate_image->compression=NoCompression;
  allocate_image->compose=OverCompositeOp;
  allocate_image->blur=1.0;
  GetExceptionInfo(&allocate_image->exception);
  (void) QueryColorDatabase(BackgroundColor,&allocate_image->background_color,
    &allocate_image->exception);
  (void) QueryColorDatabase(BorderColor,&allocate_image->border_color,
    &allocate_image->exception);
  (void) QueryColorDatabase(MatteColor,&allocate_image->matte_color,
    &allocate_image->exception);
  allocate_image->x_resolution=DefaultResolution;
  allocate_image->y_resolution=DefaultResolution;
  allocate_image->units=PixelsPerInchResolution;
  GetTimerInfo(&allocate_image->timer);
  GetCacheInfo(&allocate_image->cache);
  allocate_image->blob=CloneBlobInfo((BlobInfo *) NULL);
  allocate_image->debug=IsEventLogging();
  allocate_image->reference_count=1;
  allocate_image->signature=MagickSignature;
  if (image_info == (ImageInfo *) NULL)
    return(allocate_image);
  /*
    Transfer image info.
  */
  SetBlobExempt(allocate_image,image_info->file != (FILE *) NULL ?
    MagickTrue : MagickFalse);
  (void) CopyMagickString(allocate_image->filename,image_info->filename,
    MaxTextExtent);
  (void) CopyMagickString(allocate_image->magick_filename,image_info->filename,
    MaxTextExtent);
  (void) CopyMagickString(allocate_image->magick,image_info->magick,
    MaxTextExtent);
  if (image_info->size != (char *) NULL)
    {
      (void) ParseAbsoluteGeometry(image_info->size,
        &allocate_image->extract_info);
      allocate_image->columns=allocate_image->extract_info.width;
      allocate_image->rows=allocate_image->extract_info.height;
      allocate_image->offset=allocate_image->extract_info.x;
      allocate_image->extract_info.x=0;
      allocate_image->extract_info.y=0;
    }
  if (image_info->extract != (char *) NULL)
    {
      (void) ParseAbsoluteGeometry(image_info->extract,
        &allocate_image->extract_info);
      Swap(allocate_image->columns,allocate_image->extract_info.width);
      Swap(allocate_image->rows,allocate_image->extract_info.height);
    }
  if (image_info->colorspace != UndefinedColorspace)
    allocate_image->colorspace=image_info->colorspace;
  allocate_image->compression=image_info->compression;
  allocate_image->quality=image_info->quality;
  allocate_image->endian=image_info->endian;
  allocate_image->interlace=image_info->interlace;
  allocate_image->units=image_info->units;
  if (image_info->density != (char *) NULL)
    {
      GeometryInfo
        geometry_info;

      MagickStatusType
        flags;

      flags=ParseGeometry(image_info->density,&geometry_info);
      allocate_image->x_resolution=geometry_info.rho;
      allocate_image->y_resolution=geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        allocate_image->y_resolution=allocate_image->x_resolution;
    }
  if (image_info->page != (char *) NULL)
    {
      char
        *geometry;

      allocate_image->page=allocate_image->extract_info;
      geometry=GetPageGeometry(image_info->page);
      (void) ParseAbsoluteGeometry(geometry,&allocate_image->page);
      geometry=(char *) RelinquishMagickMemory(geometry);
    }
  if (image_info->depth != 0)
    allocate_image->depth=image_info->depth;
  allocate_image->background_color=image_info->background_color;
  allocate_image->border_color=image_info->border_color;
  allocate_image->matte_color=image_info->matte_color;
  allocate_image->progress_monitor=image_info->progress_monitor;
  allocate_image->client_data=image_info->client_data;
  if (image_info->cache != (void *) NULL)
    ClonePixelCacheMethods(allocate_image->cache,image_info->cache);
  return(allocate_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A l l o c a t e I m a g e C o l o r m a p                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AllocateImageColormap() allocates an image colormap and initializes
%  it to a linear gray colorspace.  If the image already has a colormap,
%  it is replaced.  AllocateImageColormap() returns MagickTrue if successful,
%  otherwise MagickFalse if there is not enough memory.
%
%  The format of the AllocateImageColormap method is:
%
%      MagickBooleanType AllocateImageColormap(Image *image,
%        const unsigned long colors)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o colors: The number of colors in the image colormap.
%
%
*/
MagickExport MagickBooleanType AllocateImageColormap(Image *image,
  const unsigned long colors)
{
  register long
    i;

  size_t
    length;

  unsigned long
    pixel;

  /*
    Allocate image colormap.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  image->storage_class=PseudoClass;
  image->colors=Min(colors,MaxColormapSize);
  length=(size_t) colors*sizeof(PixelPacket);
  if (image->colormap == (PixelPacket *) NULL)
    image->colormap=(PixelPacket *) AcquireMagickMemory(length);
  else
    image->colormap=(PixelPacket *) ResizeMagickMemory(image->colormap,length);
  if (image->colormap == (PixelPacket *) NULL)
    return(MagickFalse);
  for (i=0; i < (long) image->colors; i++)
  {
    pixel=(unsigned long) (i*(QuantumRange/Max(colors-1,1)));
    image->colormap[i].red=(Quantum) pixel;
    image->colormap[i].green=(Quantum) pixel;
    image->colormap[i].blue=(Quantum) pixel;
    image->colormap[i].opacity=OpaqueOpacity;
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A l l o c a t e N e x t I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AllocateNextImage() initializes the next image in a sequence to
%  default values.  The next member of image points to the newly allocated
%  image.  If there is a memory shortage, next is assigned NULL.
%
%  The format of the AllocateNextImage method is:
%
%      void AllocateNextImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows:
%
%    o image_info: Many of the image default values are set from this
%      structure.  For example, filename, compression, depth, background color,
%      and others.
%
%    o image: The image.
%
%
*/
MagickExport void AllocateNextImage(const ImageInfo *image_info,Image *image)
{
  /*
    Allocate image structure.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  image->next=AllocateImage(image_info);
  if (GetNextImageInList(image) == (Image *) NULL)
    return;
  (void) CopyMagickString(GetNextImageInList(image)->filename,image->filename,
    MaxTextExtent);
  if (image_info != (ImageInfo *) NULL)
    (void) CopyMagickString(GetNextImageInList(image)->filename,
      image_info->filename,MaxTextExtent);
  DestroyBlob(GetNextImageInList(image));
  image->next->blob=ReferenceBlob(image->blob);
  image->next->endian=image->endian;
  image->next->scene=image->scene+1;
  image->next->previous=image;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     A p p e n d I m a g e s                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  The AppendImages() method takes a set of images and appends them to each
%  other top-to-bottom if the stack parameter is true, otherwise left-to-right.
%
%  The format of the AppendImage method is:
%
%      Image *AppendImages(const Image *image,const MagickBooleanType stack,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image sequence.
%
%    o stack: A value other than 0 stacks the images top-to-bottom.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *AppendImages(const Image *image,
  const MagickBooleanType stack,ExceptionInfo *exception)
{
#define AppendImageTag  "Append/Image"

  Image
    *append_image;

  long
    n,
    y;

  MagickBooleanType
    status;

  register IndexPacket
    *append_indexes,
    *indexes;

  register const Image
    *next;

  register const PixelPacket
    *p;

  register long
    i,
    x;

  register PixelPacket
    *q;

  unsigned long
    height,
    number_images,
    width;

  /*
    Ensure the image have the same column width.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  number_images=1;
  width=image->columns;
  height=image->rows;
  next=GetNextImageInList(image);
  for ( ; next != (Image *) NULL; next=GetNextImageInList(next))
  {
    number_images++;
    if (stack != MagickFalse)
      {
        if (next->columns > width)
          width=next->columns;
        height+=next->rows;
        continue;
      }
    width+=next->columns;
    if (next->rows > height)
      height=next->rows;
  }
  /*
    Initialize append next attributes.
  */
  append_image=CloneImage(image,width,height,MagickTrue,exception);
  if (append_image == (Image *) NULL)
    return((Image *) NULL);
  append_image->storage_class=DirectClass;
  SetImageBackgroundColor(append_image);
  if (stack != MagickFalse)
    {
      /*
        Stack top-to-bottom.
      */
      i=0;
      for (n=0; n < (long) number_images; n++)
      {
        if (image->matte != MagickFalse)
          append_image->matte=MagickTrue;
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,exception);
          q=SetImagePixels(append_image,0,i++,append_image->columns,1);
          if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
            break;
          indexes=GetIndexes(image);
          append_indexes=GetIndexes(append_image);
          for (x=0; x < (long) image->columns; x++)
          {
            q->red=p->red;
            q->green=p->green;
            q->blue=p->blue;
            if (image->matte != MagickFalse)
              q->opacity=p->opacity;
            if (image->colorspace == CMYKColorspace)
              append_indexes[x]=indexes[x];
            p++;
            q++;
          }
          if (SyncImagePixels(append_image) == MagickFalse)
            break;
        }
        if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
            (QuantumTick(n,number_images) != MagickFalse))
          {
            status=image->progress_monitor(AppendImageTag,n,number_images,
              image->client_data);
            if (status == MagickFalse)
              break;
          }
        image=GetNextImageInList(image);
      }
      return(append_image);
    }
  /*
    Stack left-to-right.
  */
  i=0;
  for (n=0; n < (long) number_images; n++)
  {
    if (image->matte != MagickFalse)
      append_image->matte=MagickTrue;
    for (y=0; y < (long) image->rows; y++)
    {
      p=AcquireImagePixels(image,0,y,image->columns,1,exception);
      q=SetImagePixels(append_image,i,y,image->columns,1);
      if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
        break;
      indexes=GetIndexes(image);
      append_indexes=GetIndexes(append_image);
      for (x=0; x < (long) image->columns; x++)
      {
        q->red=p->red;
        q->green=p->green;
        q->blue=p->blue;
        if (image->matte != MagickFalse)
          q->opacity=p->opacity;
        if (image->colorspace == CMYKColorspace)
          append_indexes[x]=indexes[x];
        p++;
        q++;
      }
      if (SyncImagePixels(append_image) == MagickFalse)
        break;
    }
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(n,number_images) != MagickFalse))
      {
        status=image->progress_monitor(AppendImageTag,n,number_images,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
    i+=image->columns;
    image=GetNextImageInList(image);
  }
  return(append_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     A v e r a g e I m a g e s                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  The Average() method takes a set of images and averages them together.
%  Each image in the set must have the same width and height.  Average()
%  returns a single image with each corresponding pixel component of
%  each image averaged.   On failure, a NULL image is returned and
%  exception describes the reason for the failure.
%
%  The format of the AverageImage method is:
%
%      Image *AverageImages(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image sequence.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *AverageImages(const Image *image,ExceptionInfo *exception)
{
#define AverageImageTag  "Average/Image"

  Image
    *average_image;

  long
    n,
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    *pixels;

  MagickSizeType
    length;

  register const Image
    *next;

  register const PixelPacket
    *p;

  register long
    i,
    x;

  register PixelPacket
    *q;

  unsigned long
    number_images;

  /*
    Ensure the image are the same size.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  for (next=image; next != (Image *) NULL; next=GetNextImageInList(next))
  {
    if ((next->columns != image->columns) || (next->rows != image->rows))
      ThrowImageException(OptionError,"ImageWidthsOrHeightsDiffer");
  }
  /*
    Allocate sum accumulation buffer.
  */
  length=(MagickSizeType) image->columns*image->rows*sizeof(*pixels);
  if (length != (MagickSizeType) ((size_t) length))
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  pixels=(MagickPixelPacket *) AcquireMagickMemory((size_t) length);
  if (pixels == (MagickPixelPacket *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  (void) ResetMagickMemory(pixels,0,(size_t) length);
  /*
    Initialize average next attributes.
  */
  average_image=CloneImage(image,0,0,MagickTrue,exception);
  if (average_image == (Image *) NULL)
    {
      pixels=(MagickPixelPacket *) RelinquishMagickMemory(pixels);
      return((Image *) NULL);
    }
  average_image->storage_class=DirectClass;
  /*
    Compute sum over each pixel color component.
  */
  number_images=GetImageListLength(image);
  for (n=0; n < (long) number_images; n++)
  {
    i=0;
    for (y=0; y < (long) image->rows; y++)
    {
      p=AcquireImagePixels(image,0,y,image->columns,1,exception);
      if (p == (const PixelPacket *) NULL)
        break;
      for (x=0; x < (long) image->columns; x++)
      {
        pixels[i].red+=p->red;
        pixels[i].green+=p->green;
        pixels[i].blue+=p->blue;
        pixels[i].opacity+=p->opacity;
        p++;
        i++;
      }
    }
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(n,number_images) != MagickFalse))
      {
        status=image->progress_monitor(AverageImageTag,n,number_images,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
    image=GetNextImageInList(image);
  }
  if (n < (long) number_images)
    return(average_image);
  /*
    Average next pixels.
  */
  i=0;
  for (y=0; y < (long) average_image->rows; y++)
  {
    q=GetImagePixels(average_image,0,y,average_image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    for (x=0; x < (long) average_image->columns; x++)
    {
      q->red=(Quantum) (pixels[i].red/number_images+0.5);
      q->green=(Quantum) (pixels[i].green/number_images+0.5);
      q->blue=(Quantum) (pixels[i].blue/number_images+0.5);
      q->opacity=(Quantum) (pixels[i].opacity/number_images+0.5);
      q++;
      i++;
    }
    if (SyncImagePixels(average_image) == MagickFalse)
      break;
  }
  pixels=(MagickPixelPacket *) RelinquishMagickMemory(pixels);
  return(average_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C a t c h I m a g e E x c e p t i o n                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CatchImageException() returns if no exceptions are found in the image
%  sequence, otherwise it determines the most severe exception and reports
%  it as a warning or error depending on the severity.
%
%  The format of the CatchImageException method is:
%
%      ExceptionType CatchImageException(Image *image)
%
%  A description of each parameter follows:
%
%    o image: An image sequence.
%
%
*/
MagickExport ExceptionType CatchImageException(Image *image)
{
  ExceptionInfo
    exception;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  GetExceptionInfo(&exception);
  GetImageException(image,&exception);
  CatchException(&exception);
  DestroyExceptionInfo(&exception);
  return(exception.severity);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l i p P a t h I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ClipPathImage() sets the image clip mask based any clipping path information
%  if it exists.
%
%  The format of the ClipImage method is:
%
%      MagickBooleanType ClipPathImage(Image *image,const char *pathname,
%        const MagickBooleanType inside)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o pathname: name of clipping path resource. If name is preceded by #, use
%      clipping path numbered by name.
%
%    o inside: if non-zero, later operations take effect inside clipping path.
%      Otherwise later operations take effect outside clipping path.
%
%
*/

MagickExport MagickBooleanType ClipImage(Image *image)
{
  return(ClipPathImage(image,"#1",MagickTrue));
}

MagickExport MagickBooleanType ClipPathImage(Image *image,const char *pathname,
  const MagickBooleanType inside)
{
#define ClipPathImageTag  "ClipPath/Image"

  char
    *key;

  const ImageAttribute
    *attribute;

  Image
    *clip_mask;

  ImageInfo
    *image_info;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(pathname != NULL);
  key=AcquireString(pathname);
  (void) FormatMagickString(key,MaxTextExtent,"8BIM:1999,2998:%s",pathname);
  attribute=GetImageAttribute(image,key);
  if (attribute == (const ImageAttribute *) NULL)
    {
      ThrowFileException(&image->exception,OptionError,"NoClipPathDefined",
        image->filename);
      return(MagickFalse);
    }
  image_info=CloneImageInfo((ImageInfo *) NULL);
  clip_mask=BlobToImage(image_info,attribute->value,strlen(attribute->value),
    &image->exception);
  image_info=DestroyImageInfo(image_info);
  if (clip_mask == (Image *) NULL)
    return(MagickFalse);
  if (clip_mask->storage_class == PseudoClass)
    {
      (void) SyncImage(clip_mask);
      clip_mask->storage_class=DirectClass;
    }
  if (inside == MagickFalse)
    (void) NegateImage(clip_mask,MagickFalse);
  (void) FormatMagickString(clip_mask->magick_filename,MaxTextExtent,
    "8BIM:1999,2998:%s\nPS",pathname);
  (void) SetImageClipMask(image,clip_mask);
  clip_mask=DestroyImage(clip_mask);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneImage() copies an image and returns the copy as a new image object.
%  If the specified columns and rows is 0, an exact copy of the image is
%  returned, otherwise the pixel data is undefined and must be initialized
%  with the SetImagePixels() and SyncImagePixels() methods.  On failure,
%  a NULL image is returned and exception describes the reason for the
%  failure.
%
%  The format of the CloneImage method is:
%
%      Image *CloneImage(const Image *image,const unsigned long columns,
%        const unsigned long rows,const MagickBooleanType orphan,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o columns: The number of columns in the cloned image.
%
%    o rows: The number of rows in the cloned image.
%
%    o orphan:  With a value other than 0, the cloned image is an orphan.  An
%      orphan is a stand-alone image that is not assocated with an image list.
%      In effect, the next and previous members of the cloned image is set to
%      NULL.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *CloneImage(const Image *image,const unsigned long columns,
  const unsigned long rows,const MagickBooleanType orphan,
  ExceptionInfo *exception)
{
  Image
    *clone_image;

  MagickRealType
    scale;

  size_t
    length;

  /*
    Clone the image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  clone_image=(Image *) AcquireMagickMemory(sizeof(*clone_image));
  if (clone_image == (Image *) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  (void) ResetMagickMemory(clone_image,0,sizeof(*clone_image));
  clone_image->storage_class=image->storage_class;
  clone_image->colorspace=image->colorspace;
  clone_image->compression=image->compression;
  clone_image->quality=image->quality;
  clone_image->taint=image->taint;
  clone_image->matte=image->matte;
  clone_image->columns=image->columns;
  clone_image->rows=image->rows;
  clone_image->depth=image->depth;
  if (image->colormap != (PixelPacket *) NULL)
    {
      /*
        Allocate and copy the image colormap.
      */
      clone_image->colors=image->colors;
      length=(size_t) image->colors*sizeof(PixelPacket);
      clone_image->colormap=(PixelPacket *) AcquireMagickMemory(length);
      if (clone_image->colormap == (PixelPacket *) NULL)
        ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
      length=(size_t) image->colors*sizeof(PixelPacket);
      (void) CopyMagickMemory(clone_image->colormap,image->colormap,length);
    }
  clone_image->background_color=image->background_color;
  clone_image->border_color=image->border_color;
  clone_image->matte_color=image->matte_color;
  clone_image->gamma=image->gamma;
  clone_image->chromaticity=image->chromaticity;
  clone_image->rendering_intent=image->rendering_intent;
  clone_image->units=image->units;
  clone_image->montage=(char *) NULL;
  clone_image->directory=(char *) NULL;
  (void) CloneString(&clone_image->geometry,image->geometry);
  clone_image->offset=image->offset;
  clone_image->x_resolution=image->x_resolution;
  clone_image->y_resolution=image->y_resolution;
  clone_image->page=image->page;
  clone_image->extract_info=image->extract_info;
  clone_image->bias=image->bias;
  clone_image->blur=image->blur;
  clone_image->fuzz=image->fuzz;
  clone_image->filter=image->filter;
  clone_image->interlace=image->interlace;
  clone_image->endian=image->endian;
  clone_image->gravity=image->gravity;
  clone_image->compose=image->compose;
  clone_image->signature=MagickSignature;
  clone_image->scene=image->scene;
  clone_image->dispose=image->dispose;
  clone_image->delay=image->delay;
  clone_image->ticks_per_second=image->ticks_per_second;
  clone_image->iterations=image->iterations;
  clone_image->total_colors=image->total_colors;
  (void) CloneImageAttributes(clone_image,image);
  (void) CloneImageProfiles(clone_image,image);
  clone_image->error=image->error;
  clone_image->semaphore=(SemaphoreInfo *) NULL;
  GetTimerInfo(&clone_image->timer);
  GetExceptionInfo(&clone_image->exception);
  InheritException(&clone_image->exception,&image->exception);
  clone_image->progress_monitor=image->progress_monitor;
  clone_image->client_data=image->client_data;
  clone_image->start_loop=image->start_loop;
  if (image->ascii85 != (void *) NULL)
    Ascii85Initialize(clone_image);
  clone_image->magick_columns=image->magick_columns;
  clone_image->magick_rows=image->magick_rows;
  (void) CopyMagickString(clone_image->magick_filename,image->magick_filename,
    MaxTextExtent);
  (void) CopyMagickString(clone_image->magick,image->magick,MaxTextExtent);
  (void) CopyMagickString(clone_image->filename,image->filename,MaxTextExtent);
  clone_image->reference_count=1;
  clone_image->previous=NewImageList();
  clone_image->list=NewImageList();
  clone_image->next=NewImageList();
  clone_image->clip_mask=NewImageList();
  clone_image->blob=ReferenceBlob(image->blob);
  clone_image->debug=IsEventLogging();
  if (orphan == MagickFalse)
    {
      if (GetPreviousImageInList(image) != (Image *) NULL)
        clone_image->previous->next=clone_image;
      if (GetNextImageInList(image) != (Image *) NULL)
        clone_image->next->previous=clone_image;
    }
  if (((columns == 0) && (rows == 0)) ||
      ((columns == image->columns) && (rows == image->rows)))
    {
      if (image->montage != (char *) NULL)
        (void) CloneString(&clone_image->montage,image->montage);
      if (image->directory != (char *) NULL)
        (void) CloneString(&clone_image->directory,image->directory);
      if (image->clip_mask != (Image *) NULL)
        clone_image->clip_mask=CloneImage(image->clip_mask,0,0,MagickTrue,
          exception);
    }
  if ((columns == 0) && (rows == 0))
    {
      clone_image->cache=ReferenceCache(image->cache);
      return(clone_image);
    }
  clone_image->columns=columns;
  clone_image->rows=rows;
  scale=(MagickRealType) clone_image->columns/(MagickRealType) image->columns;
  clone_image->page.width=(unsigned long) (scale*clone_image->page.width+0.5);
  clone_image->page.x=(long) (scale*clone_image->page.x+0.5);
  scale=(MagickRealType) clone_image->rows/(MagickRealType) image->rows;
  clone_image->page.height=(unsigned long) (scale*clone_image->page.height+0.5);
  clone_image->page.y=(long) (clone_image->page.y*scale+0.5);
  GetCacheInfo(&clone_image->cache);
  (void) SyncCache((Image *) image);  /* required for clone of a clone */
  return(clone_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e I m a g e s                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneImages() clones one or more images from an image sequence.
%
%  The format of the CloneImages method is:
%
%      Image *CloneImages(const Image *image,const char *scenes,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o scenes: This character string specifies which scenes to clone
%      (e.g. 1,3-5,7-6,2).
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport Image *CloneImages(const Image *images,const char *scenes,
  ExceptionInfo *exception)
{
  char
    *q;

  const Image
    *next;

  Image
    *clone_images,
    *image;

  long
    first,
    last,
    quantum;

  register const char
    *p;

  register long
    i;

  assert(images != (const Image *) NULL);
  assert(images->signature == MagickSignature);
  assert(scenes != (char *) NULL);
  if (images->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  clone_images=NewImageList();
  p=scenes;
  for (q=(char *) scenes; *q != '\0'; p++)
  {
    while ((isspace((int) ((unsigned char) *p)) != 0) || (*p == ','))
      p++;
    first=strtol(p,&q,10);
    if (first < 0)
      first+=GetImageListLength(images);
    last=first;
    while (isspace((int) ((unsigned char) *q)) != 0)
      q++;
    if (*q == '-')
      {
        last=strtol(q+1,&q,10);
        if (last < 0)
          last+=GetImageListLength(images);
      }
    quantum=first > last ? -1 : 1;
    for (p=q; first != (last+quantum); first+=quantum)
    {
      i=0;
      for (next=images; next != (Image *) NULL; next=GetNextImageInList(next))
      {
        if (i == (long) first)
          {
            image=CloneImage(next,0,0,MagickTrue,exception);
            if (image == (Image *) NULL)
              break;
            AppendImageToList(&clone_images,image);
          }
        i++;
      }
    }
  }
  if (clone_images == (Image *) NULL)
    return((Image *) NULL);
  return(GetFirstImageInList(clone_images));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e I m a g e I n f o                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneImageInfo() makes a copy of the given image info structure.  If
%  NULL is specified, a new image info structure is created initialized to
%  default values.
%
%  The format of the CloneImageInfo method is:
%
%      ImageInfo *CloneImageInfo(const ImageInfo *image_info)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%
*/
MagickExport ImageInfo *CloneImageInfo(const ImageInfo *image_info)
{
  ImageInfo
    *clone_info;

  clone_info=(ImageInfo *) AcquireMagickMemory(sizeof(*clone_info));
  if (clone_info == (ImageInfo *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      image_info->filename);
  GetImageInfo(clone_info);
  if (image_info == (ImageInfo *) NULL)
    return(clone_info);
  clone_info->compression=image_info->compression;
  clone_info->temporary=image_info->temporary;
  clone_info->adjoin=image_info->adjoin;
  clone_info->antialias=image_info->antialias;
  clone_info->scene=image_info->scene;
  clone_info->number_scenes=image_info->number_scenes;
  clone_info->depth=image_info->depth;
  if (image_info->size != (char *) NULL)
    (void) CloneString(&clone_info->size,image_info->size);
  if (image_info->extract != (char *) NULL)
    (void) CloneString(&clone_info->extract,image_info->extract);
  if (image_info->scenes != (char *) NULL)
    (void) CloneString(&clone_info->scenes,image_info->scenes);
  if (image_info->page != (char *) NULL)
    (void) CloneString(&clone_info->page,image_info->page);
  clone_info->interlace=image_info->interlace;
  clone_info->endian=image_info->endian;
  clone_info->units=image_info->units;
  clone_info->quality=image_info->quality;
  if (image_info->sampling_factor != (char *) NULL)
    (void) CloneString(&clone_info->sampling_factor,
      image_info->sampling_factor);
  if (image_info->server_name != (char *) NULL)
    (void) CloneString(&clone_info->server_name,image_info->server_name);
  if (image_info->font != (char *) NULL)
    (void) CloneString(&clone_info->font,image_info->font);
  if (image_info->texture != (char *) NULL)
    (void) CloneString(&clone_info->texture,image_info->texture);
  if (image_info->density != (char *) NULL)
    (void) CloneString(&clone_info->density,image_info->density);
  clone_info->pointsize=image_info->pointsize;
  clone_info->fuzz=image_info->fuzz;
  clone_info->pen=image_info->pen;
  clone_info->background_color=image_info->background_color;
  clone_info->border_color=image_info->border_color;
  clone_info->matte_color=image_info->matte_color;
  clone_info->dither=image_info->dither;
  clone_info->monochrome=image_info->monochrome;
  clone_info->colors=image_info->colors;
  clone_info->colorspace=image_info->colorspace;
  clone_info->type=image_info->type;
  clone_info->preview_type=image_info->preview_type;
  clone_info->group=image_info->group;
  clone_info->ping=image_info->ping;
  clone_info->verbose=image_info->verbose;
  if (image_info->view != (char *) NULL)
    (void) CloneString(&clone_info->view,image_info->view);
  if (image_info->authenticate != (char *) NULL)
    (void) CloneString(&clone_info->authenticate,image_info->authenticate);
  if (image_info->attributes != (Image *) NULL)
    clone_info->attributes=CloneImage(image_info->attributes,0,0,MagickTrue,
      &image_info->attributes->exception);
  (void) CloneImageOptions(clone_info,image_info);
  clone_info->progress_monitor=image_info->progress_monitor;
  clone_info->client_data=image_info->client_data;
  clone_info->cache=image_info->cache;
  if (image_info->cache != (void *) NULL)
    clone_info->cache=ReferenceCache(image_info->cache);
  SetImageInfoFile(clone_info,image_info->file);
  SetImageInfoBlob(clone_info,image_info->blob,image_info->length);
  clone_info->stream=image_info->stream;
  (void) CopyMagickString(clone_info->magick,image_info->magick,MaxTextExtent);
  (void) CopyMagickString(clone_info->unique,image_info->unique,MaxTextExtent);
  (void) CopyMagickString(clone_info->zero,image_info->zero,MaxTextExtent);
  (void) CopyMagickString(clone_info->filename,image_info->filename,
    MaxTextExtent);
  clone_info->subimage=image_info->scene;
  clone_info->subrange=image_info->number_scenes;
  clone_info->channel=image_info->channel;
  clone_info->debug=IsEventLogging();
  clone_info->signature=image_info->signature;
  return(clone_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     C o m b i n e I m a g e s                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CombineImages() combines one or more images into a single image.  The
%  grayscale value of the pixels of each image in the sequence is assigned in
%  order to the specified channels of the combined image.   The typical
%  ordering would be image 1 => Red, 2 => Green, 3 => Blue, etc.
%
%  The format of the CombineImages method is:
%
%      Image *CombineImages(const Image *image,const ChannelType channel,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *CombineImages(const Image *image,const ChannelType channel,
  ExceptionInfo *exception)
{
#define CombineImageTag  "Combine/Image"

  Image
    *combine_image;

  long
    y;

  MagickBooleanType
    status;

  PixelPacket
    *pixels;

  register const Image
    *next;

  register const PixelPacket
    *p;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Ensure the image are the same size.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  for (next=image; next != (Image *) NULL; next=GetNextImageInList(next))
  {
    if ((next->columns != image->columns) || (next->rows != image->rows))
      ThrowImageException(OptionError,"ImagesAreNotTheSameSize");
  }
  combine_image=CloneImage(image,0,0,MagickTrue,exception);
  if (combine_image == (Image *) NULL)
    return((Image *) NULL);
  combine_image->storage_class=DirectClass;
  if ((channel & OpacityChannel) != 0)
    combine_image->matte=MagickTrue;
  if ((channel & BlackChannel) != 0)
    combine_image->colorspace=CMYKColorspace;
  SetImageBackgroundColor(combine_image);
  for (y=0; y < (long) combine_image->rows; y++)
  {
    pixels=GetImagePixels(combine_image,0,y,combine_image->columns,1);
    if (pixels == (PixelPacket *) NULL)
      break;
    next=image;
    if (((channel & RedChannel) != 0) && (next != (Image *) NULL))
      {
        p=AcquireImagePixels(next,0,y,next->columns,1,exception);
        if (p == (const PixelPacket *) NULL)
          break;
        q=pixels;
        for (x=0; x < (long) combine_image->columns; x++)
        {
          q->red=PixelIntensityToQuantum(p);
          p++;
          q++;
        }
        next=GetNextImageInList(next);
      }
    if (((channel & GreenChannel) != 0) && (next != (Image *) NULL))
      {
        p=AcquireImagePixels(next,0,y,next->columns,1,exception);
        if (p == (const PixelPacket *) NULL)
          break;
        q=pixels;
        for (x=0; x < (long) combine_image->columns; x++)
        {
          q->green=PixelIntensityToQuantum(p);
          p++;
          q++;
        }
        next=GetNextImageInList(next);
      }
    if (((channel & BlueChannel) != 0) && (next != (Image *) NULL))
      {
        p=AcquireImagePixels(next,0,y,next->columns,1,exception);
        if (p == (const PixelPacket *) NULL)
          break;
        q=pixels;
        for (x=0; x < (long) combine_image->columns; x++)
        {
          q->blue=PixelIntensityToQuantum(p);
          p++;
          q++;
        }
        next=GetNextImageInList(next);
      }
    if (((channel & BlackChannel) != 0) && (next != (Image *) NULL))
      {
        IndexPacket
          *indexes;

        p=AcquireImagePixels(next,0,y,next->columns,1,exception);
        if (p == (const PixelPacket *) NULL)
          break;
        indexes=GetIndexes(combine_image);
        for (x=0; x < (long) combine_image->columns; x++)
        {
          indexes[x]=PixelIntensityToQuantum(p);
          p++;
        }
        next=GetNextImageInList(next);
      }
    if (((channel & OpacityChannel) != 0) && (next != (Image *) NULL))
      {
        p=AcquireImagePixels(next,0,y,next->columns,1,exception);
        if (p == (const PixelPacket *) NULL)
          break;
        q=pixels;
        for (x=0; x < (long) combine_image->columns; x++)
        {
          q->opacity=PixelIntensityToQuantum(p);
          p++;
          q++;
        }
        next=GetNextImageInList(next);
      }
    if (SyncImagePixels(combine_image) == MagickFalse)
      break;
    if ((combine_image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,combine_image->rows) != MagickFalse))
      {
        status=image->progress_monitor(CombineImageTag,y,combine_image->rows,
          combine_image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(combine_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     C y c l e C o l o r m a p I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CycleColormap() displaces an image's colormap by a given number of
%  positions.  If you cycle the colormap a number of times you can produce
%  a psychodelic effect.
%
%  The format of the CycleColormapImage method is:
%
%      MagickBooleanType CycleColormapImage(Image *image,const long displace)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o displace:  displace the colormap this amount.
%
%
*/
MagickExport MagickBooleanType CycleColormapImage(Image *image,
  const long displace)
{
  long
    index,
    y;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->storage_class == DirectClass)
    (void) SetImageType(image,PaletteType);
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
    {
      index=(long) ((indexes[x]+displace) % image->colors);
      if (index < 0)
        index+=image->colors;
      indexes[x]=(IndexPacket) index;
      q->red=image->colormap[index].red;
      q->green=image->colormap[index].green;
      q->blue=image->colormap[index].blue;
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyImage() dereferences an image, deallocating memory associated with
%  the image if the reference count becomes zero.
%
%  The format of the DestroyImage method is:
%
%      Image *DestroyImage(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%
*/
MagickExport Image *DestroyImage(Image *image)
{
  MagickBooleanType
    destroy;

  /*
    Dereference image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  destroy=MagickFalse;
  AcquireSemaphoreInfo(&image->semaphore);
  image->reference_count--;
  if (image->reference_count == 0)
    destroy=MagickTrue;
  RelinquishSemaphoreInfo(image->semaphore);
  if (destroy == MagickFalse)
    return((Image *) NULL);
  /*
    Destroy image.
  */
  AcquireSemaphoreInfo(&image->semaphore);
  DestroyImagePixels(image);
  if (image->clip_mask != (Image *) NULL)
    image->clip_mask=DestroyImage(image->clip_mask);
  if (image->montage != (char *) NULL)
    image->montage=(char *) RelinquishMagickMemory(image->montage);
  if (image->directory != (char *) NULL)
    image->directory=(char *) RelinquishMagickMemory(image->directory);
  if (image->colormap != (PixelPacket *) NULL)
    image->colormap=(PixelPacket *) RelinquishMagickMemory(image->colormap);
  if (image->geometry != (char *) NULL)
    image->geometry=(char *) RelinquishMagickMemory(image->geometry);
  DestroyImageAttributes(image);
  DestroyImageProfiles(image);
  DestroyExceptionInfo(&image->exception);
  if (image->ascii85 != (_Ascii85Info_*) NULL)
    image->ascii85=(_Ascii85Info_ *) RelinquishMagickMemory(image->ascii85);
  DestroyBlob(image);
  image->signature=(~MagickSignature);
  RelinquishSemaphoreInfo(image->semaphore);
  image->semaphore=(SemaphoreInfo *)
    DestroySemaphoreInfo((SemaphoreInfo *) image->semaphore);
  image=(Image *) RelinquishMagickMemory(image);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y I m a g e I n f o                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyImageInfo() deallocates memory associated with an ImageInfo
%  structure.
%
%  The format of the DestroyImageInfo method is:
%
%      ImageInfo *DestroyImageInfo(ImageInfo *image_info)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%
*/
MagickExport ImageInfo *DestroyImageInfo(ImageInfo *image_info)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (image_info->size != (char *) NULL)
    image_info->size=(char *) RelinquishMagickMemory(image_info->size);
  if (image_info->extract != (char *) NULL)
    image_info->extract=(char *) RelinquishMagickMemory(image_info->extract);
  if (image_info->scenes != (char *) NULL)
    image_info->scenes=(char *) RelinquishMagickMemory(image_info->scenes);
  if (image_info->page != (char *) NULL)
    image_info->page=(char *) RelinquishMagickMemory(image_info->page);
  if (image_info->sampling_factor != (char *) NULL)
    image_info->sampling_factor=(char *)
      RelinquishMagickMemory(image_info->sampling_factor);
  if (image_info->server_name != (char *) NULL)
    image_info->server_name=(char *)
      RelinquishMagickMemory(image_info->server_name);
  if (image_info->font != (char *) NULL)
    image_info->font=(char *) RelinquishMagickMemory(image_info->font);
  if (image_info->texture != (char *) NULL)
    image_info->texture=(char *) RelinquishMagickMemory(image_info->texture);
  if (image_info->density != (char *) NULL)
    image_info->density=(char *) RelinquishMagickMemory(image_info->density);
  if (image_info->view != (char *) NULL)
    image_info->view=(char *) RelinquishMagickMemory(image_info->view);
  if (image_info->authenticate != (char *) NULL)
    image_info->authenticate=(char *)
      RelinquishMagickMemory(image_info->authenticate);
  if (image_info->attributes != (Image *) NULL)
    image_info->attributes=DestroyImage(image_info->attributes);
  DestroyImageOptions(image_info);
  if (image_info->cache != (void *) NULL)
    image_info->cache=DestroyCacheInfo(image_info->cache);
  image_info->signature=(~MagickSignature);
  image_info=(ImageInfo *) RelinquishMagickMemory(image_info);
  return(image_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e C l i p M a s k                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageClipMask() returns the clip mask associated with the image.
%
%  The format of the GetImageClipMask method is:
%
%      Image *GetImageClipMask(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/
MagickExport Image *GetImageClipMask(const Image *image,
  ExceptionInfo *exception)
{
  assert(image != (const Image *) NULL);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image->signature == MagickSignature);
  if (image->clip_mask == (Image *) NULL)
    return((Image *) NULL);
  return(CloneImage(image->clip_mask,0,0,MagickTrue,exception));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e E x c e p t i o n                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageException() traverses an image sequence and returns any
%  error more severe than noted by the exception parameter.
%
%  The format of the GetImageException method is:
%
%      void GetImageException(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: Specifies a pointer to a list of one or more images.
%
%    o exception: return the highest severity exception.
%
%
*/
MagickExport void GetImageException(Image *image,ExceptionInfo *exception)
{
  register Image
    *next;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  for (next=image; next != (Image *) NULL; next=GetNextImageInList(next))
  {
    if (next->exception.severity == UndefinedException)
      continue;
    if (next->exception.severity > exception->severity)
      InheritException(exception,&next->exception);
    next->exception.severity=UndefinedException;
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e I n f o                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageInfo() initializes image_info to default values.
%
%  The format of the GetImageInfo method is:
%
%      void GetImageInfo(ImageInfo *image_info)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%
*/
MagickExport void GetImageInfo(ImageInfo *image_info)
{
  ExceptionInfo
    exception;

  /*
    File and image dimension members.
  */
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image_info != (ImageInfo *) NULL);
  (void) ResetMagickMemory(image_info,0,sizeof(*image_info));
  image_info->adjoin=MagickTrue;
  image_info->interlace=NoInterlace;
  image_info->channel=(ChannelType)
    ((long) AllChannels &~ (long) OpacityChannel);
  image_info->quality=UndefinedCompressionQuality;
  image_info->antialias=MagickTrue;
  image_info->pointsize=12;
  image_info->dither=MagickTrue;
  GetExceptionInfo(&exception);
  (void) QueryColorDatabase(BackgroundColor,&image_info->background_color,
    &exception);
  (void) QueryColorDatabase(BorderColor,&image_info->border_color,&exception);
  (void) QueryColorDatabase(MatteColor,&image_info->matte_color,&exception);
  DestroyExceptionInfo(&exception);
  image_info->debug=IsEventLogging();
  if (GetMonitorHandler() != (MonitorHandler) NULL)
    image_info->progress_monitor=MagickMonitor;
  image_info->signature=MagickSignature;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e T y p e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageType() returns the potential type of image:
%
%        Bilevel        Grayscale       GrayscaleMatte
%        Palette        PaletteMatte    TrueColor
%        TrueColorMatte ColorSeparation ColorSeparationMatte
%
%  To ensure the image type matches its potential, use SetImageType():
%
%    (void) SetImageType(image,GetImageType(image));
%
%  The format of the GetImageType method is:
%
%      ImageType GetImageType(const Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport ImageType GetImageType(const Image *image,ExceptionInfo *exception)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->colorspace == CMYKColorspace)
    {
      if (image->matte == MagickFalse)
        return(ColorSeparationType);
      return(ColorSeparationMatteType);
    }
  if (IsGrayImage(image,exception) != MagickFalse)
    {
      if (IsMonochromeImage(image,exception) != MagickFalse)
        return(BilevelType);
      if (image->matte != MagickFalse)
        return(GrayscaleMatteType);
      return(GrayscaleType);
    }
  if (IsPaletteImage(image,exception) != MagickFalse)
    {
      if (image->matte != MagickFalse)
        return(PaletteMatteType);
      return(PaletteType);
    }
  if (IsOpaqueImage(image,exception) == MagickFalse)
    return(TrueColorMatteType);
  return(TrueColorType);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
+     G r a d i e n t I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GradientImage() applies a continuously smooth color transitions along a
%  vector from one color to another.
%
%  Note, the interface of this method will change in the future to support
%  more than one transistion.
%
%  The format of the GradientImage method is:
%
%      MagickBooleanType GradientImage(Image *image,
%        const PixelPacket *start_color,const PixelPacket *stop_color)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o start_color: The start color.
%
%    o stop_color: The stop color.
%
%
*/
MagickExport MagickBooleanType GradientImage(Image *image,
  const PixelPacket *start_color,const PixelPacket *stop_color)
{
#define GradientImageTag  "Gradient/Image"

  long
    y;

  MagickBooleanType
    status;

  MagickRealType
    alpha,
    offset,
    scale;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Determine (Hue, Saturation, Brightness) gradient.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(start_color != (const PixelPacket *) NULL);
  assert(stop_color != (const PixelPacket *) NULL);
  /*
    Generate gradient pixels.
  */
  offset=0.0;
  scale=1.0/(image->columns*image->rows-1);
  for (y=0; y < (long) image->rows; y++)
  {
    q=SetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      alpha=QuantumRange*scale*offset;
      MagickCompositeOver(start_color,alpha,stop_color,(MagickRealType)
        stop_color->opacity,q);
      q++;
      offset++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(GradientImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     I s T a i n t I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsTaintImage() returns a value other than 0 if any pixel in the image
%  has been altered since it was first constituted.
%
%  The format of the IsTaintImage method is:
%
%      MagickBooleanType IsTaintImage(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%
*/
MagickExport MagickBooleanType IsTaintImage(const Image *image)
{
  char
    magick[MaxTextExtent],
    filename[MaxTextExtent];

  register const Image
    *p;

  assert(image != (Image *) NULL);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image->signature == MagickSignature);
  (void) CopyMagickString(magick,image->magick,MaxTextExtent);
  (void) CopyMagickString(filename,image->filename,MaxTextExtent);
  for (p=image; p != (Image *) NULL; p=GetNextImageInList(p))
  {
    if (p->taint != MagickFalse)
      return(MagickTrue);
    if (LocaleCompare(p->magick,magick) != 0)
      return(MagickTrue);
    if (LocaleCompare(p->filename,filename) != 0)
      return(MagickTrue);
  }
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M o d i f y I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ModifyImage() ensures that there is only a single reference to the image
%  to be modified, updating the provided image pointer to point to a clone of
%  the original image if necessary.
%
%  The format of the ModifyImage method is:
%
%      ModifyImage(Image *image,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport void ModifyImage(Image **image,ExceptionInfo *exception)
{
  Image
    *clone_image;

  MagickBooleanType
    clone;

  assert(image != (Image **) NULL);
  assert(*image != (Image *) NULL);
  assert((*image)->signature == MagickSignature);
  if ((*image)->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      (*image)->filename);
  clone=MagickFalse;
  AcquireSemaphoreInfo(&(*image)->semaphore);
  if ((*image)->reference_count > 1)
    clone=MagickTrue;
  RelinquishSemaphoreInfo((*image)->semaphore);
  if (clone == MagickFalse)
    return;
  clone_image=CloneImage(*image,0,0,MagickTrue,exception);
  AcquireSemaphoreInfo(&(*image)->semaphore);
  (*image)->reference_count--;
  RelinquishSemaphoreInfo((*image)->semaphore);
  *image=clone_image;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%   N e w M a g i c k I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  NewMagickImage() creates a blank image canvas of the specified size and
%  background color.
%
%  The format of the NewMagickImage method is:
%
%      Image *NewMagickImage(const ImageInfo *image_info,
%        const unsigned long width,const unsigned long height,
%        const MagickPixelPacket *background)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o width: The image width.
%
%    o height: The image height.
%
%    o background: The image color.
%
*/
MagickExport Image *NewMagickImage(const ImageInfo *image_info,
  const unsigned long width,const unsigned long height,
  const MagickPixelPacket *background)
{
  Image
    *image;

  long
    y;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  assert(image_info != (const ImageInfo *) NULL);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image_info->signature == MagickSignature);
  assert(background != (const MagickPixelPacket *) NULL);
  image=AllocateImage(image_info);
  image->columns=width;
  image->rows=height;
  image->colorspace=background->colorspace;
  image->matte=background->matte;
  image->fuzz=background->fuzz;
  image->depth=background->depth;
  for (y=0; y < (long) image->rows; y++)
  {
    q=SetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
    {
      SetPixelPacket(background,q,indexes+x);
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
  }
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     P l a s m a I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  PlasmaImage() initializes an image with plasma fractal values.  The image
%  must be initialized with a base color and the random number generator
%  seeded before this method is called.
%
%  The format of the PlasmaImage method is:
%
%      MagickBooleanType PlasmaImage(Image *image,const SegmentInfo *segment,
%        unsigned long attenuate,unsigned long depth)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o segment:   Define the region to apply plasma fractals values.
%
%    o attenuate: Define the plasma attenuation factor.
%
%    o depth: Limit the plasma recursion depth.
%
%
*/

static inline Quantum PlasmaPixel(const MagickRealType pixel,
  const MagickRealType noise)
{
  MagickRealType
    value;

  value=pixel+noise*GetRandomValue()-noise/2;
  if (value <= 0.0)
    return(0);
  if (value >= QuantumRange)
    return(QuantumRange);
  return((Quantum) (value+0.5));
}

MagickExport MagickBooleanType PlasmaImage(Image *image,
  const SegmentInfo *segment,unsigned long attenuate,unsigned long depth)
{
  long
    x,
    x_mid,
    y,
    y_mid;

  MagickRealType
    plasma;

  PixelPacket
    u,
    v;

  register PixelPacket
    *q;

  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(segment != (SegmentInfo *) NULL);
  if (((segment->x2-segment->x1) == 0.0) && ((segment->y2-segment->y1) == 0.0))
    return(MagickTrue);
  if (depth != 0)
    {
      SegmentInfo
        local_info;

      /*
        Divide the area into quadrants and recurse.
      */
      depth--;
      attenuate++;
      x_mid=(long) (segment->x1+segment->x2+0.5)/2;
      y_mid=(long) (segment->y1+segment->y2+0.5)/2;
      local_info=(*segment);
      local_info.x2=(double) x_mid;
      local_info.y2=(double) y_mid;
      (void) PlasmaImage(image,&local_info,attenuate,depth);
      local_info=(*segment);
      local_info.y1=(double) y_mid;
      local_info.x2=(double) x_mid;
      (void) PlasmaImage(image,&local_info,attenuate,depth);
      local_info=(*segment);
      local_info.x1=(double) x_mid;
      local_info.y2=(double) y_mid;
      (void) PlasmaImage(image,&local_info,attenuate,depth);
      local_info=(*segment);
      local_info.x1=(double) x_mid;
      local_info.y1=(double) y_mid;
      return(PlasmaImage(image,&local_info,attenuate,depth));
    }
  image->storage_class=DirectClass;
  x_mid=(long) (segment->x1+segment->x2+0.5)/2;
  y_mid=(long) (segment->y1+segment->y2+0.5)/2;
  if ((segment->x1 == (double) x_mid) && (segment->x2 == (double) x_mid) &&
      (segment->y1 == (double) y_mid) && (segment->y2 == (double) y_mid))
    return(MagickFalse);
  /*
    Average pixels and apply plasma.
  */
  plasma=QuantumRange/(2.0*attenuate);
  if ((segment->x1 != (double) x_mid) || (segment->x2 != (double) x_mid))
    {
      /*
        Left pixel.
      */
      x=(long) (segment->x1+0.5);
      u=GetOnePixel(image,x,(long) (segment->y1+0.5));
      v=GetOnePixel(image,x,(long) (segment->y2+0.5));
      q=SetImagePixels(image,x,y_mid,1,1);
      if (q == (PixelPacket *) NULL)
        return(MagickTrue);
      q->red=PlasmaPixel(((double) u.red+v.red)/2,plasma);
      q->green=PlasmaPixel(((double) u.green+v.green)/2,plasma);
      q->blue=PlasmaPixel(((double) u.blue+v.blue)/2,plasma);
      (void) SyncImagePixels(image);
      if (segment->x1 != segment->x2)
        {
          /*
            Right pixel.
          */
          x=(long) (segment->x2+0.5);
          u=GetOnePixel(image,x,(long) (segment->y1+0.5));
          v=GetOnePixel(image,x,(long) (segment->y2+0.5));
          q=SetImagePixels(image,x,y_mid,1,1);
          if (q == (PixelPacket *) NULL)
            return(MagickTrue);
          q->red=PlasmaPixel(((double) u.red+v.red)/2,plasma);
          q->green=PlasmaPixel(((double) u.green+v.green)/2,plasma);
          q->blue=PlasmaPixel(((double) u.blue+v.blue)/2,plasma);
          (void) SyncImagePixels(image);
        }
    }
  if ((segment->y1 != (double) y_mid) || (segment->y2 != (double) y_mid))
    {
      if ((segment->x1 != (double) x_mid) || (segment->y2 != (double) y_mid))
        {
          /*
            Bottom pixel.
          */
          y=(long) (segment->y2+0.5);
          u=GetOnePixel(image,(long) (segment->x1+0.5),y);
          v=GetOnePixel(image,(long) (segment->x2+0.5),y);
          q=SetImagePixels(image,x_mid,y,1,1);
          if (q == (PixelPacket *) NULL)
            return(MagickTrue);
          q->red=PlasmaPixel(((double) u.red+v.red)/2,plasma);
          q->green=PlasmaPixel(((double) u.green+v.green)/2,plasma);
          q->blue=PlasmaPixel(((double) u.blue+v.blue)/2,plasma);
          (void) SyncImagePixels(image);
        }
      if (segment->y1 != segment->y2)
        {
          /*
            Top pixel.
          */
          y=(long) (segment->y1+0.5);
          u=GetOnePixel(image,(long) (segment->x1+0.5),y);
          v=GetOnePixel(image,(long) (segment->x2+0.5),y);
          q=SetImagePixels(image,x_mid,y,1,1);
          if (q == (PixelPacket *) NULL)
            return(MagickTrue);
          q->red=PlasmaPixel(((double) u.red+v.red)/2,plasma);
          q->green=PlasmaPixel(((double) u.green+v.green)/2,plasma);
          q->blue=PlasmaPixel(((double) u.blue+v.blue)/2,plasma);
          (void) SyncImagePixels(image);
        }
    }
  if ((segment->x1 != segment->x2) || (segment->y1 != segment->y2))
    {
      /*
        Middle pixel.
      */
      x=(long) (segment->x1+0.5);
      y=(long) (segment->y1+0.5);
      u=GetOnePixel(image,x,y);
      x=(long) (segment->x2+0.5);
      y=(long) (segment->y2+0.5);
      v=GetOnePixel(image,x,y);
      q=SetImagePixels(image,x_mid,y_mid,1,1);
      if (q == (PixelPacket *) NULL)
        return(MagickTrue);
      q->red=PlasmaPixel(((double) u.red+v.red)/2,plasma);
      q->green=PlasmaPixel(((double) u.green+v.green)/2,plasma);
      q->blue=PlasmaPixel(((double) u.blue+v.blue)/2,plasma);
      (void) SyncImagePixels(image);
    }
  if (((segment->x2-segment->x1) < 3.0) && ((segment->y2-segment->y1) < 3.0))
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e f e r e n c e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReferenceImage() increments the reference count associated with an image
%  returning a pointer to the image.
%
%  The format of the ReferenceImage method is:
%
%      Image *ReferenceImage(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%
*/
MagickExport Image *ReferenceImage(Image *image)
{
  assert(image != (Image *) NULL);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image->signature == MagickSignature);
  AcquireSemaphoreInfo(&image->semaphore);
  image->reference_count++;
  RelinquishSemaphoreInfo(image->semaphore);
  return(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e l i n q u i s h I m a g e R e s o u r c e s                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RelinquishImageResources() relinquishes image resources.
%
%  The format of the RelinquishImageResources() method is:
%
%      void RelinquishImageResources(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/
MagickExport void RelinquishImageResources(const Image *image)
{
  RelinquishCacheResources(image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     S e p a r a t e I m a g e C h a n n e l                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Separate a channel from the image and return it as a grayscale image.  A
%  channel is a particular color component of each pixel in the image.
%
%  The format of the SeparateImage method is:
%
%      MagickBooleanType SeparateImageChannel(Image *image,
%        const ChannelType channel)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: Identify which channel to extract: RedSeparate, GreenSeparate,
%      BlueSeparate, OpacitySeparate, CyanSeparate, MagentaSeparate,
%      YellowSeparate, or BlackSeparate.
%
%
*/
MagickExport MagickBooleanType SeparateImageChannel(Image *image,
  const ChannelType channel)
{
#define SeparateImageTag  "Separate/Image"

  long
    y;

  MagickBooleanType
    status;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Separate DirectClass packets.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  image->storage_class=DirectClass;
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    switch (channel)
    {
      case RedChannel:
      {
        for (x=(long) image->columns-1; x >= 0; x--)
        {
          q->green=q->red;
          q->blue=q->red;
          q++;
        }
        break;
      }
      case GreenChannel:
      {
        for (x=(long) image->columns-1; x >= 0; x--)
        {
          q->red=q->green;
          q->blue=q->green;
          q++;
        }
        break;
      }
      case BlueChannel:
      {
        for (x=(long) image->columns-1; x >= 0; x--)
        {
          q->red=q->blue;
          q->green=q->blue;
          q++;
        }
        break;
      }
      case OpacityChannel:
      {
        for (x=(long) image->columns-1; x >= 0; x--)
        {
          q->red=q->opacity;
          q->green=q->opacity;
          q->blue=q->opacity;
          q->opacity=OpaqueOpacity;
          q++;
        }
        image->matte=MagickFalse;
        break;
      }
      case IndexChannel:
      {
        if ((image->storage_class != PseudoClass) &&
            (image->colorspace != CMYKColorspace))
          break;
        for (x=(long) image->columns-1; x >= 0; x--)
        {
          q->red=indexes[x];
          q->green=indexes[x];
          q->blue=indexes[x];
          q->opacity=OpaqueOpacity;
          q++;
        }
        image->matte=MagickFalse;
        break;
      }
      default:
        break;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(SeparateImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  image->colorspace=RGBColorspace;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%   S e t  m a g e B a c k g r o u n d C o l o r                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageBackgroundColor() initializes the image pixels to the image
%  background color.  The background color is defined by the background_color
%  member of the image structure.
%
%  The format of the SetImage method is:
%
%      void SetImageBackgroundColor(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/
MagickExport void SetImageBackgroundColor(Image *image)
{
  long
    y;

  MagickPixelPacket
    background;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  assert(image != (Image *) NULL);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image->signature == MagickSignature);
  if (image->background_color.opacity != OpaqueOpacity)
    image->matte=MagickTrue;
  GetMagickPixelPacket(image,&background);
  SetMagickPixelPacket(&image->background_color,(IndexPacket *) NULL,
    &background);
  if (image->colorspace == CMYKColorspace)
    RGBtoCMYK(&background);
  for (y=0; y < (long) image->rows; y++)
  {
    q=SetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=(long) image->columns-1; x >= 0; x--)
    {
      SetPixelPacket(&background,q,indexes);
      q++;
      indexes++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e C l i p M a s k                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageClipMask() associates a clip mask with the image.  The clip mask
%  must be the same dimensions as the image.  Set any pixel component of
%  the clip mask to TransparentOpacity to prevent that corresponding image
%  pixel component from being updated when SyncImagePixels() is applied.
%
%  The format of the SetImageClipMask method is:
%
%      MagickBooleanType SetImageClipMask(Image *image,const Image *clip_mask)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o clip_mask: The image clip mask.
%
%
*/
MagickExport MagickBooleanType SetImageClipMask(Image *image,
  const Image *clip_mask)
{
  assert(image != (Image *) NULL);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image->signature == MagickSignature);
  if (clip_mask != (const Image *) NULL)
    if ((clip_mask->columns != image->columns) ||
        (clip_mask->rows != image->rows))
      ThrowBinaryException(ImageError,"ImageSizeDiffers",image->filename);
  if (image->clip_mask != (Image *) NULL)
    image->clip_mask=DestroyImage(image->clip_mask);
  image->clip_mask=NewImageList();
  if (clip_mask == (Image *) NULL)
    return(MagickTrue);
  image->storage_class=DirectClass;
  image->clip_mask=CloneImage(clip_mask,0,0,MagickTrue,&image->exception);
  if (image->clip_mask == (Image *) NULL)
    return(MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e E x t e n t                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageExtent() sets the image size (i.e. columns & rows).
%
%  The format of the SetImageExtent method is:
%
%      MagickBooleanType SetImageExtent(Image *image,const unsigned long columns,
%        const unsigned long rows)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o columns:  The image width in pixels.
%
%    o rows:  The image height in pixels.
%
*/
MagickExport MagickBooleanType SetImageExtent(Image *image,
  const unsigned long columns,const unsigned long rows)
{
  register PixelPacket
    *p;

  image->columns=columns;
  image->rows=rows;
  (void) ParseAbsoluteGeometry("0x0+0+0",&image->page);
  p=SetImagePixels(image,0,0,1,1);
  return(p == (PixelPacket *) NULL ? MagickFalse : MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S e t I m a g e I n f o                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageInfo() initializes the `magick' field of the ImageInfo structure.
%  It is set to a type of image format based on the prefix or suffix of the
%  filename.  For example, `ps:image' returns PS indicating a Postscript image.
%  JPEG is returned for this filename: `image.jpg'.  The filename prefix has
%  precendence over the suffix.  Use an optional index enclosed in brackets
%  after a file name to specify a desired scene of a multi-resolution image
%  format like Photo CD (e.g. img0001.pcd[4]).  A True (non-zero) return value
%  indicates success.
%
%  The format of the SetImageInfo method is:
%
%      MagickBooleanType SetImageInfo(ImageInfo *image_info,
%        const MagickBooleanType rectify,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info..
%
%    o rectify: an unsigned value other than zero rectifies the attribute for
%      multi-frame support (user may want multi-frame but image format may not
%      support it).
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType SetImageInfo(ImageInfo *image_info,
  const MagickBooleanType rectify,ExceptionInfo *exception)
{
  char
    extension[MaxTextExtent],
    filename[MaxTextExtent],
    magic[MaxTextExtent],
    *q,
    subimage[MaxTextExtent];

  const MagicInfo
    *magic_info;

  const MagickInfo
    *magick_info;

  ExceptionInfo
    sans_exception;

  Image
    *image;

  MagickBooleanType
    status;

  register const char
    *p;

  ssize_t
    count;

  unsigned char
    magick[2*MaxTextExtent];

  /*
    Look for 'image.format' in filename.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  *subimage='\0';
  GetPathComponent(image_info->filename,SubimagePath,subimage);
  if (*subimage != '\0')
    {
      /*
        Look for scene specification (e.g. img0001.pcd[4]).
      */
      if (IsSceneGeometry(subimage,MagickFalse) == MagickFalse)
        {
          if (IsGeometry(subimage) != MagickFalse)
            (void) CloneString(&image_info->extract,subimage);
        }
      else
        {
          unsigned long
            first,
            last;

          (void) CloneString(&image_info->scenes,subimage);
          image_info->scene=(unsigned long) atol(image_info->scenes);
          image_info->number_scenes=image_info->scene;
          p=image_info->scenes;
          for (q=(char *) image_info->scenes; *q != '\0'; p++)
          {
            while ((isspace((int) ((unsigned char) *p)) != 0) || (*p == ','))
              p++;
            first=(unsigned long) strtol(p,&q,10);
            last=first;
            while (isspace((int) ((unsigned char) *q)) != 0)
              q++;
            if (*q == '-')
              last=(unsigned long) strtol(q+1,&q,10);
            if (first > last)
              Swap(first,last);
            if (first < image_info->scene)
              image_info->scene=first;
            if (last > image_info->number_scenes)
              image_info->number_scenes=last;
            p=q;
          }
          image_info->number_scenes-=image_info->scene-1;
          image_info->subimage=image_info->scene;
          image_info->subrange=image_info->number_scenes;
        }
    }
  *extension='\0';
  GetPathComponent(image_info->filename,ExtensionPath,extension);
#if defined(HasZLIB)
  if (*extension != '\0')
    if ((LocaleCompare(extension,"gz") == 0) ||
        (LocaleCompare(extension,"Z") == 0) ||
        (LocaleCompare(extension,"wmz") == 0))
      {
        char
          path[MaxTextExtent];

        (void) CopyMagickString(path,image_info->filename,MaxTextExtent);
        path[strlen(path)-strlen(extension)-1]='\0';
        GetPathComponent(path,ExtensionPath,extension);
      }
#endif
#if defined(HasBZLIB)
  if (*extension != '\0')
    if (LocaleCompare(extension,"bz2") == 0)
      {
        char
          path[MaxTextExtent];

        (void) CopyMagickString(path,image_info->filename,MaxTextExtent);
        path[strlen(path)-strlen(extension)-1]='\0';
        GetPathComponent(path,ExtensionPath,extension);
      }
#endif
  image_info->affirm=MagickFalse;
  if (*extension != '\0')
    {
      /*
        User specified image format.
      */
      (void) CopyMagickString(magic,extension,MaxTextExtent);
      LocaleUpper(magic);
      /*
        SGI and RGB are ambiguous;  TMP must be set explicitly.
      */
      if (((LocaleNCompare(image_info->magick,"SGI",3) != 0) ||
          (LocaleCompare(magic,"RGB") != 0)) &&
          (LocaleCompare(magic,"TMP") != 0))
        (void) CopyMagickString(image_info->magick,magic,MaxTextExtent);
    }
  /*
    Look for explicit 'format:image' in filename.
  */
  *magic='\0';
  GetPathComponent(image_info->filename,MagickPath,magic);
  if (*magic == '\0')
    (void) CopyMagickString(magic,image_info->magick,MaxTextExtent);
  else
    {
      /*
        User specified image format.
      */
      if (LocaleCompare(magic,"GRADATION") == 0)
        (void) strcpy(magic,"GRADIENT");
      LocaleUpper(magic);
      if (IsMagickConflict(magic) == MagickFalse)
        {
          (void) CopyMagickString(image_info->magick,magic,MaxTextExtent);
          if (LocaleCompare(magic,"TMP") != 0)
            image_info->affirm=MagickTrue;
          else
            image_info->temporary=MagickTrue;
        }
    }
  GetExceptionInfo(&sans_exception);
  magick_info=GetMagickInfo(magic,&sans_exception);
  if ((magick_info == (const MagickInfo *) NULL) ||
      (GetMagickEndianSupport(magick_info) == MagickFalse))
    image_info->endian=UndefinedEndian;
  DestroyExceptionInfo(&sans_exception);
  GetPathComponent(image_info->filename,CanonicalPath,filename);
  (void) CopyMagickString(image_info->filename,filename,MaxTextExtent);
  if (rectify != MagickFalse)
    {
      register char
        *p;

      /*
        Rectify multi-image file support.
      */
      (void) CopyMagickString(filename,image_info->filename,MaxTextExtent);
      for (p=strchr(filename,'%'); p != (char *) NULL; p=strchr(p+1,'%'))
      {
        char
          *q;

        q=(char *) p+1;
        if (*q == '0')
          (void) strtol(q,&q,10);
        if ((*q == '%') || (*q == 'd') || (*q == 'o') || (*q == 'x'))
          {
            char
              format[MaxTextExtent];

            (void) CopyMagickString(format,p,MaxTextExtent);
            (void) FormatMagickString(p,MaxTextExtent,format,image_info->scene);
            break;
          }
      }
      if ((LocaleCompare(filename,image_info->filename) != 0) &&
          (strchr(filename,'%') == (char *) NULL))
        image_info->adjoin=MagickFalse;
      magick_info=GetMagickInfo(magic,exception);
      if (magick_info != (const MagickInfo *) NULL)
        if (GetMagickAdjoin(magick_info) == MagickFalse)
          image_info->adjoin=MagickFalse;
      return(MagickTrue);
    }
  if (image_info->affirm != MagickFalse)
    return(MagickTrue);
  /*
    Determine the image format from the first few bytes of the file.
  */
  image=AllocateImage(image_info);
  if (image == (Image *) NULL)
    return(MagickFalse);
  (void) CopyMagickString(image->filename,image_info->filename,MaxTextExtent);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImage(image);
      return(MagickFalse);
    }
  if ((IsBlobSeekable(image) == MagickFalse) ||
      (IsBlobExempt(image) != MagickFalse))
    {
      /*
        Copy standard input or pipe to temporary file.
      */
      *filename='\0';
      status=ImageToFile(image,filename,exception);
      CloseBlob(image);
      if (status == MagickFalse)
        {
          image=DestroyImage(image);
          return(MagickFalse);
        }
      SetImageInfoFile(image_info,(FILE *) NULL);
      (void) CopyMagickString(image->filename,filename,MaxTextExtent);
      status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
      if (status == MagickFalse)
        {
          image=DestroyImage(image);
          return(MagickFalse);
        }
      (void) CopyMagickString(image_info->filename,filename,MaxTextExtent);
      image_info->temporary=MagickTrue;
    }
  (void) ResetMagickMemory(magick,0,sizeof(magick));
  count=ReadBlob(image,2*MaxTextExtent,magick);
  CloseBlob(image);
  image=DestroyImage(image);
  /*
    Check magic.xml configuration file.
  */
  GetExceptionInfo(&sans_exception);
  magic_info=GetMagicInfo(magick,(size_t) count,&sans_exception);
  if ((magic_info != (const MagicInfo *) NULL) &&
      (GetMagicName(magic_info) != (char *) NULL))
    {
      (void) CopyMagickString(image_info->magick,GetMagicName(magic_info),
        MaxTextExtent);
      magick_info=GetMagickInfo(image_info->magick,&sans_exception);
      if ((magick_info == (const MagickInfo *) NULL) ||
          (GetMagickEndianSupport(magick_info) == MagickFalse))
        image_info->endian=UndefinedEndian;
      DestroyExceptionInfo(&sans_exception);
      return(MagickTrue);
    }
  p=GetImageMagick(magick,2*MaxTextExtent);
  if (p != (const char *) NULL)
    (void) CopyMagickString(image_info->magick,p,MaxTextExtent);
  magick_info=GetMagickInfo(image_info->magick,&sans_exception);
  if ((magick_info == (const MagickInfo *) NULL) ||
      (GetMagickEndianSupport(magick_info) == MagickFalse))
    image_info->endian=UndefinedEndian;
  DestroyExceptionInfo(&sans_exception);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e I n f o B l o b                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageInfoBlob() sets the image info blob member.
%
%  The format of the SetImageInfoBlob method is:
%
%      void SetImageInfoBlob(ImageInfo *image_info,const void *blob,
%        const size_t length)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o blob: The blob.
%
%    o length: The blob length.
%
*/
MagickExport void SetImageInfoBlob(ImageInfo *image_info,const void *blob,
  const size_t length)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  image_info->blob=(void *) blob;
  image_info->length=length;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e I n f o F i l e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageInfoFile() sets the image info file member.
%
%  The format of the SetImageInfoFile method is:
%
%      void SetImageInfoFile(ImageInfo *image_info,FILE *file)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o file: The file.
%
%
*/
MagickExport void SetImageInfoFile(ImageInfo *image_info,FILE *file)
{
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  image_info->file=file;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     S e t I m a g e O p a c i t y                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageOpacity() attenuates the opacity channel of an image.  If the
%  image pixels are opaque, they are set to the specified opacity level.
%  Otherwise, the pixel oapcity values are blended with the supplied
%  transparency value.
%
%  The format of the SetImageOpacity method is:
%
%      void SetImageOpacity(Image *image,const Quantum opacity)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o opacity: The level of transparency: 0 is fully opaque and QuantumRange is
%      fully transparent.
%
%
*/
MagickExport void SetImageOpacity(Image *image,const Quantum opacity)
{
  long
    y;

  register long
    x;

  register PixelPacket
    *q;

  assert(image != (Image *) NULL);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image->signature == MagickSignature);
  if (image->matte != MagickFalse)
    {
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        for (x=(long) image->columns-1; x >= 0; x--)
        {
          q->opacity=(Quantum) (QuantumScale*opacity*q->opacity+0.5);
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      return;
    }
  image->matte=MagickTrue;
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      q->opacity=opacity;
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e T y p e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageType() sets the type of image.  Choose from these types:
%
%        Bilevel        Grayscale       GrayscaleMatte
%        Palette        PaletteMatte    TrueColor
%        TrueColorMatte ColorSeparation ColorSeparationMatte
%        OptimizeType
%
%  The format of the SetImageType method is:
%
%      MagickBooleanType SetImageType(Image *image,const ImageType image_type)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o image_type: Image type.
%
%
*/
MagickExport MagickBooleanType SetImageType(Image *image,
  const ImageType image_type)
{
  QuantizeInfo
    quantize_info;

  assert(image != (Image *) NULL);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image->signature == MagickSignature);
  switch (image_type)
  {
    case BilevelType:
    {
      if ((image->colorspace == RGBColorspace) &&
          (image->storage_class == PseudoClass) &&
          (IsMonochromeImage(image,&image->exception) != MagickFalse))
        break;
      if (image->colorspace != RGBColorspace)
        (void) SetImageColorspace(image,RGBColorspace);
      GetQuantizeInfo(&quantize_info);
      quantize_info.colorspace=GRAYColorspace;
      quantize_info.tree_depth=8;
      quantize_info.number_colors=2;
      (void) QuantizeImage(&quantize_info,image);
      break;
    }
    case GrayscaleType:
    {
      if ((image->colorspace == RGBColorspace) &&
          (IsGrayImage(image,&image->exception) != MagickFalse))
        break;
      (void) SetImageColorspace(image,GRAYColorspace);
      break;
    }
    case GrayscaleMatteType:
    {
      if ((image->colorspace == RGBColorspace) &&
          (IsGrayImage(image,&image->exception) != MagickFalse) &&
          (image->matte != MagickFalse))
        break;
      (void) SetImageColorspace(image,GRAYColorspace);
      if (image->matte == MagickFalse)
        SetImageOpacity(image,OpaqueOpacity);
      break;
    }
    case PaletteType:
    {
      if ((image->colorspace == RGBColorspace) &&
          (image->storage_class == PseudoClass))
        break;
      if (image->colorspace != RGBColorspace)
        (void) SetImageColorspace(image,RGBColorspace);
      GetQuantizeInfo(&quantize_info);
      (void) QuantizeImage(&quantize_info,image);
      break;
    }
    case PaletteMatteType:
    {
      if ((image->colorspace == RGBColorspace) &&
          (image->storage_class == PseudoClass) &&
          (image->matte != MagickFalse))
        break;
      if (image->colorspace != RGBColorspace)
        (void) SetImageColorspace(image,RGBColorspace);
      if (image->matte == MagickFalse)
        SetImageOpacity(image,OpaqueOpacity);
      GetQuantizeInfo(&quantize_info);
      quantize_info.colorspace=TransparentColorspace;
      (void) QuantizeImage(&quantize_info,image);
      break;
    }
    case TrueColorType:
    {
      if ((image->colorspace == RGBColorspace) &&
          (image->storage_class == DirectClass))
        break;
      if (image->colorspace != RGBColorspace)
        (void) SetImageColorspace(image,RGBColorspace);
      image->storage_class=DirectClass;
      break;
    }
    case TrueColorMatteType:
    {
      if ((image->colorspace == RGBColorspace) &&
          (image->storage_class == DirectClass) &&
          (image->matte != MagickFalse))
        break;
      if (image->colorspace != RGBColorspace)
        (void) SetImageColorspace(image,RGBColorspace);
      image->storage_class=DirectClass;
      if (image->matte == MagickFalse)
        SetImageOpacity(image,OpaqueOpacity);
      break;
    }
    case ColorSeparationType:
    {
      if (image->colorspace == CMYKColorspace)
        break;
      (void) SetImageColorspace(image,CMYKColorspace);
      image->storage_class=DirectClass;
      break;
    }
    case ColorSeparationMatteType:
    {
      if ((image->colorspace == CMYKColorspace) &&
          (image->matte != MagickFalse))
        break;
      if (image->colorspace != CMYKColorspace)
        {
          if (image->colorspace != RGBColorspace)
            (void) SetImageColorspace(image,RGBColorspace);
          (void) SetImageColorspace(image,CMYKColorspace);
        }
      image->storage_class=DirectClass;
      if (image->matte == MagickFalse)
        SetImageOpacity(image,OpaqueOpacity);
      break;
    }
    case OptimizeType:
    default:
      break;
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S o r t C o l o r m a p B y I n t e n t s i t y                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SortColormapByIntensity() sorts the colormap of a PseudoClass image by
%  decreasing color intensity.
%
%  The format of the SortColormapByIntensity method is:
%
%      MagickBooleanType SortColormapByIntensity(Image *image)
%
%  A description of each parameter follows:
%
%    o image: A pointer to an Image structure.
%
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int IntensityCompare(const void *x,const void *y)
{
  const PixelPacket
    *color_1,
    *color_2;

  int
    intensity;

  color_1=(const PixelPacket *) x;
  color_2=(const PixelPacket *) y;
  intensity=(int) PixelIntensityToQuantum(color_2)-
    (int) PixelIntensityToQuantum(color_1);
  return(intensity);
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport MagickBooleanType SortColormapByIntensity(Image *image)
{
  IndexPacket
    index;

  long
    y;

  register long
    x;

  register IndexPacket
    *indexes;

  register PixelPacket
    *q;

  register long
    i;

  unsigned short
    *pixels;

  assert(image != (Image *) NULL);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image->signature == MagickSignature);
  if (image->storage_class != PseudoClass)
    return(MagickTrue);
  /*
    Allocate memory for pixel indexes.
  */
  pixels=(unsigned short *)
    AcquireMagickMemory((size_t) image->colors*sizeof(*pixels));
  if (pixels == (unsigned short *) NULL)
    ThrowBinaryException(ResourceLimitError,"MemoryAllocationFailed",
      image->filename);
  /*
    Assign index values to colormap entries.
  */
  for (i=0; i < (long) image->colors; i++)
    image->colormap[i].opacity=(IndexPacket) i;
  /*
    Sort image colormap by decreasing color popularity.
  */
  qsort((void *) image->colormap,(size_t) image->colors,
    sizeof(*image->colormap),IntensityCompare);
  /*
    Update image colormap indexes to sorted colormap order.
  */
  for (i=0; i < (long) image->colors; i++)
    pixels[image->colormap[i].opacity]=(unsigned short) i;
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
    {
      index=(IndexPacket) pixels[indexes[x]];
      indexes[x]=index;
      *q++=image->colormap[index];
    }
  }
  pixels=(unsigned short *) RelinquishMagickMemory(pixels);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S t r i p I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StripImage() strips an image of all profiles and comments.
%
%  The format of the StripImage method is:
%
%      MagickBooleanType StripImage(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%
*/
MagickExport MagickBooleanType StripImage(Image *image)
{
  DestroyImageProfiles(image);
  (void) SetImageAttribute(image,"Comment",(char *) NULL);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   S y n c I m a g e                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SyncImage() initializes the red, green, and blue intensities of each pixel
%  as defined by the colormap index.
%
%  The format of the SyncImage method is:
%
%      MagickBooleanType SyncImage(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%
*/
MagickExport MagickBooleanType SyncImage(Image *image)
{
  IndexPacket
    index;

  long
    y;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  assert(image != (Image *) NULL);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image->signature == MagickSignature);
  if (image->storage_class == DirectClass)
    return(MagickFalse);
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
    {
      index=ConstrainColormapIndex(image,indexes[x]);
      q->red=image->colormap[index].red;
      q->green=image->colormap[index].green;
      q->blue=image->colormap[index].blue;
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
  }
  return(y == (long) image->rows ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%     T e x t u r e I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TextureImage() repeatedly tiles the texture image across and down the image
%  canvas.
%
%  The format of the TextureImage method is:
%
%      MagickBooleanType TextureImage(Image *image,const Image *texture)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o texture: This image is the texture to layer on the background.
%
%
*/
MagickExport MagickBooleanType TextureImage(Image *image,const Image *texture)
{
#define TextureImageTag  "Texture/Image"

  const PixelPacket
    *pixels;

  long
    x,
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    composite,
    source;

  register long
    z;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes,
    *texture_indexes;

  register PixelPacket
    *q;

  unsigned long
    width;

  /*
    Tile texture onto the image background.
  */
  assert(image != (Image *) NULL);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(image->signature == MagickSignature);
  if (texture == (const Image *) NULL)
    return(MagickFalse);
  image->storage_class=DirectClass;
  GetMagickPixelPacket(image,&source);
  GetMagickPixelPacket(texture,&composite);
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(texture,0,y % texture->rows,texture->columns,1,
      &image->exception);
    q=GetImagePixels(image,0,y,image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    texture_indexes=GetIndexes(texture);
    indexes=GetIndexes(image);
    pixels=p;
    for (x=0; x < (long) image->columns; x+=texture->columns)
    {
      width=texture->columns;
      if ((unsigned long) (x+width) > image->columns)
        width=image->columns-x;
      p=pixels;
      for (z=0; z < (long) width; z++)
      {
        SetMagickPixelPacket(p,texture_indexes+x+z,&source);
        SetMagickPixelPacket(q,indexes+x+z,&composite);
        MagickPixelCompositeOver(&source,(MagickRealType)
          (texture->matte != MagickFalse ? source.opacity : OpaqueOpacity),
          &composite,(MagickRealType)
          (image->matte != MagickFalse ? composite.opacity : OpaqueOpacity),
          &composite);
        SetPixelPacket(&composite,q,indexes+x+z);
        p++;
        q++;
      }
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(TextureImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(MagickTrue);
}
