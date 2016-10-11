/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            RRRR    AAA   W   W                              %
%                            R   R  A   A  W   W                              %
%                            RRRR   AAAAA  W W W                              %
%                            R R    A   A  WW WW                              %
%                            R  R   A   A  W   W                              %
%                                                                             %
%                                                                             %
%                       Read/Write RAW Image Format.                          %
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
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/colorspace.h"
#include "magick/constitute.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/pixel.h"
#include "magick/static.h"
#include "magick/statistic.h"
#include "magick/string_.h"

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteRAWImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d R A W I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadRAWImage() reads an image of raw samples and returns it.  It allocates
%  the memory necessary for the new Image structure and returns a pointer to
%  the new image.
%
%  The format of the ReadRAWImage method is:
%
%      Image *ReadRAWImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadRAWImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  Image
    *image;

  long
    j,
    y;

  MagickBooleanType
    status;

  register long
    i,
    x;

  register PixelPacket
    *q;

  ssize_t
    count;

  size_t
    packet_size;

  unsigned char
    *scanline;

  unsigned long
    depth,
    width;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  image=AllocateImage(image_info);
  if ((image->columns == 0) || (image->rows == 0))
    ThrowReaderException(OptionError,"MustSpecifyImageSize");
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  switch (*image->magick)
  {
    case 'C':
    case 'c':
    case 'M':
    case 'm':
    case 'Y':
    case 'y':
    case 'K':
    case 'k':
    {
      image->colorspace=CMYKColorspace;
      break;
    }
    default:
      break;
  }
  for (i=0; i < image->offset; i++)
    (void) ReadBlobByte(image);
  /*
    Allocate memory for a scanline.
  */
  depth=GetImageQuantumDepth(image,MagickTrue);
  packet_size=(size_t) (depth/8);
  scanline=(unsigned char *)
    AcquireMagickMemory(packet_size*image->extract_info.width);
  if (scanline == (unsigned char *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  if (image_info->number_scenes != 0)
    while (image->scene < image_info->scene)
    {
      /*
        Skip to next image.
      */
      image->scene++;
      for (y=0; y < (long) image->rows; y++)
        count=ReadBlob(image,packet_size*image->extract_info.width,scanline);
    }
  x=(long) (packet_size*image->extract_info.x);
  do
  {
    /*
      Convert raster image to pixel packets.
    */
    if (AllocateImageColormap(image,1 << image->depth) == MagickFalse)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    if ((image_info->ping != MagickFalse) && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    for (y=0; y < image->extract_info.y; y++)
      count=ReadBlob(image,packet_size*image->extract_info.width,scanline);
    for (y=0; y < (long) image->rows; y++)
    {
      if ((y > 0) || (GetPreviousImageInList(image) == (Image *) NULL))
        count=ReadBlob(image,packet_size*image->extract_info.width,scanline);
      q=SetImagePixels(image,0,y,image->columns,1);
      if (q == (PixelPacket *) NULL)
        break;
      (void) ExportQuantumPixels(image,GrayQuantum,0,scanline+x);
      if (SyncImagePixels(image) == MagickFalse)
        break;
      if (image->previous == (Image *) NULL)
        if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
            (QuantumTick(y,image->rows) != MagickFalse))
          {
            status=image->progress_monitor(LoadImageTag,y,image->rows,
              image->client_data);
            if (status == MagickFalse)
              break;
          }
    }
    width=image->extract_info.height-image->rows-image->extract_info.y;
    for (j=0; j < (long) width; j++)
      count=ReadBlob(image,packet_size*image->extract_info.width,scanline);
    if (EOFBlob(image) != MagickFalse)
      {
        ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
          image->filename);
        break;
      }
    if (image->colorspace == CMYKColorspace)
      image->storage_class=DirectClass;
    /*
      Proceed to next image.
    */
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    count=ReadBlob(image,packet_size*image->extract_info.width,scanline);
    if (count != 0)
      {
        /*
          Allocate next image structure.
        */
        AllocateNextImage(image_info,image);
        if (GetNextImageInList(image) == (Image *) NULL)
          {
            image=DestroyImageList(image);
            return((Image *) NULL);
          }
        image=SyncNextImageInList(image);
        if (image->progress_monitor != (MagickProgressMonitor) NULL)
          {
            status=image->progress_monitor(LoadImagesTag,TellBlob(image),
              GetBlobSize(image),image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
  } while (count != 0);
  scanline=(unsigned char *) RelinquishMagickMemory(scanline);
  CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r R A W I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterRAWImage() adds attributes for the RAW image format to the list of
%  supported formats.  The attributes include the image format tag, a method to
%  read and/or write the format, whether the format supports the saving of
%  more than one frame to the same file or blob, whether the format supports
%  native in-memory I/O, and a brief description of the format.
%
%  The format of the RegisterRAWImage method is:
%
%      RegisterRAWImage(void)
%
*/
ModuleExport void RegisterRAWImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("R");
  entry->decoder=(DecoderHandler *) ReadRAWImage;
  entry->encoder=(EncoderHandler *) WriteRAWImage;
  entry->raw=MagickTrue;
  entry->endian_support=MagickTrue;
  entry->description=AcquireString("Raw red samples");
  entry->module=AcquireString("RAW");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("C");
  entry->decoder=(DecoderHandler *) ReadRAWImage;
  entry->encoder=(EncoderHandler *) WriteRAWImage;
  entry->raw=MagickTrue;
  entry->endian_support=MagickTrue;
  entry->description=AcquireString("Raw cyan samples");
  entry->module=AcquireString("RAW");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("G");
  entry->decoder=(DecoderHandler *) ReadRAWImage;
  entry->encoder=(EncoderHandler *) WriteRAWImage;
  entry->raw=MagickTrue;
  entry->endian_support=MagickTrue;
  entry->description=AcquireString("Raw green samples");
  entry->module=AcquireString("RAW");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("M");
  entry->decoder=(DecoderHandler *) ReadRAWImage;
  entry->encoder=(EncoderHandler *) WriteRAWImage;
  entry->raw=MagickTrue;
  entry->endian_support=MagickTrue;
  entry->description=AcquireString("Raw magenta samples");
  entry->module=AcquireString("RAW");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("B");
  entry->decoder=(DecoderHandler *) ReadRAWImage;
  entry->encoder=(EncoderHandler *) WriteRAWImage;
  entry->raw=MagickTrue;
  entry->endian_support=MagickTrue;
  entry->description=AcquireString("Raw blue samples");
  entry->module=AcquireString("RAW");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("Y");
  entry->decoder=(DecoderHandler *) ReadRAWImage;
  entry->encoder=(EncoderHandler *) WriteRAWImage;
  entry->raw=MagickTrue;
  entry->endian_support=MagickTrue;
  entry->description=AcquireString("Raw yellow samples");
  entry->module=AcquireString("RAW");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("A");
  entry->decoder=(DecoderHandler *) ReadRAWImage;
  entry->encoder=(EncoderHandler *) WriteRAWImage;
  entry->raw=MagickTrue;
  entry->endian_support=MagickTrue;
  entry->description=AcquireString("Raw alpha samples");
  entry->module=AcquireString("RAW");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("O");
  entry->decoder=(DecoderHandler *) ReadRAWImage;
  entry->encoder=(EncoderHandler *) WriteRAWImage;
  entry->raw=MagickTrue;
  entry->endian_support=MagickTrue;
  entry->description=AcquireString("Raw opacity samples");
  entry->module=AcquireString("RAW");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("K");
  entry->decoder=(DecoderHandler *) ReadRAWImage;
  entry->encoder=(EncoderHandler *) WriteRAWImage;
  entry->raw=MagickTrue;
  entry->endian_support=MagickTrue;
  entry->description=AcquireString("Raw black samples");
  entry->module=AcquireString("RAW");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r R A W I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterRAWImage() removes format registrations made by the RAW module
%  from the list of supported formats.
%
%  The format of the UnregisterRAWImage method is:
%
%      UnregisterRAWImage(void)
%
*/
ModuleExport void UnregisterRAWImage(void)
{
  (void) UnregisterMagickInfo("R");
  (void) UnregisterMagickInfo("C");
  (void) UnregisterMagickInfo("G");
  (void) UnregisterMagickInfo("M");
  (void) UnregisterMagickInfo("B");
  (void) UnregisterMagickInfo("Y");
  (void) UnregisterMagickInfo("A");
  (void) UnregisterMagickInfo("O");
  (void) UnregisterMagickInfo("K");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e R A W I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteRAWImage() writes an image to a file as raw intensity values.
%
%  The format of the WriteRAWImage method is:
%
%      MagickBooleanType WriteRAWImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteRAWImage(const ImageInfo *image_info,Image *image)
{
  long
    y;

  MagickOffsetType
    scene;

  QuantumType
    quantum_type;

  MagickBooleanType
    status;

  register const PixelPacket
    *p;

  size_t
    packet_size;

  unsigned char
    *scanline;

  unsigned long
    depth;

  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
  if (status == MagickFalse)
    return(status);
  switch (*image->magick)
  {
    case 'A':
    case 'a':
    {
      quantum_type=AlphaQuantum;
      break;
    }
    case 'B':
    case 'b':
    {
      quantum_type=BlueQuantum;
      break;
    }
    case 'C':
    case 'c':
    {
      quantum_type=CyanQuantum;
      if (image->colorspace == CMYKColorspace)
        break;
      ThrowWriterException(ImageError,"ColorSeparatedImageRequired");
    }
    case 'g':
    case 'G':
    {
      quantum_type=GreenQuantum;
      break;
    }
    case 'I':
    case 'i':
    {
      quantum_type=IndexQuantum;
      break;
    }
    case 'K':
    case 'k':
    {
      quantum_type=BlackQuantum;
      if (image->colorspace == CMYKColorspace)
        break;
      ThrowWriterException(ImageError,"ColorSeparatedImageRequired");
    }
    case 'M':
    case 'm':
    {
      quantum_type=MagentaQuantum;
      if (image->colorspace == CMYKColorspace)
        break;
      ThrowWriterException(ImageError,"ColorSeparatedImageRequired");
    }
    case 'o':
    case 'O':
    {
      quantum_type=OpacityQuantum;
      break;
    }
    case 'R':
    case 'r':
    {
      quantum_type=RedQuantum;
      break;
    }
    case 'Y':
    case 'y':
    {
      quantum_type=YellowQuantum;
      if (image->colorspace == CMYKColorspace)
        break;
      ThrowWriterException(ImageError,"ColorSeparatedImageRequired");
    }
    default:
    {
      quantum_type=GrayQuantum;
      break;
    }
  }
  scene=0;
  do
  {
    /*
      Allocate memory for scanline.
    */
    depth=GetImageQuantumDepth(image,MagickTrue);
    packet_size=(size_t) (depth/8);
    scanline=(unsigned char *)
      AcquireMagickMemory(packet_size*image->columns);
    if (scanline == (unsigned char *) NULL)
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    /*
      Convert MIFF to RAW raster scanline.
    */
    for (y=0; y < (long) image->rows; y++)
    {
      p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
      if (p == (const PixelPacket *) NULL)
        break;
      (void) ImportQuantumPixels(image,quantum_type,0,scanline);
      (void) WriteBlob(image,packet_size*image->columns,scanline);
      if (image->previous == (Image *) NULL)
        if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
            (QuantumTick(y,image->rows) != MagickFalse))
          {
            status=image->progress_monitor(SaveImageTag,y,image->rows,
              image->client_data);
            if (status == MagickFalse)
              break;
          }
    }
    scanline=(unsigned char *) RelinquishMagickMemory(scanline);
    if (GetNextImageInList(image) == (Image *) NULL)
      break;
    image=SyncNextImageInList(image);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        status=image->progress_monitor(SaveImagesTag,scene,
          GetImageListLength(image),image->client_data);
        if (status == MagickFalse)
          break;
      }
    scene++;
  } while (image_info->adjoin != MagickFalse);
  CloseBlob(image);
  return(MagickTrue);
}
