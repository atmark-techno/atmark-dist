/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%       H   H  IIIII  SSSSS  TTTTT   OOO    GGGG  RRRR    AAA   M   M         %
%       H   H    I    SS       T    O   O  G      R   R  A   A  MM MM         %
%       HHHHH    I     SSS     T    O   O  G  GG  RRRR   AAAAA  M M M         %
%       H   H    I       SS    T    O   O  G   G  R R    A   A  M   M         %
%       H   H  IIIII  SSSSS    T     OOO    GGG   R  R   A   A  M   M         %
%                                                                             %
%                                                                             %
%                          Write A Histogram Image.                           %
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
#include "magick/attribute.h"
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/constitute.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/image-private.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/resource_.h"
#include "magick/static.h"
#include "magick/statistic.h"
#include "magick/string_.h"

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteHISTOGRAMImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r H I S T O G R A M I m a g e                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterHISTOGRAMImage() adds attributes for the Histogram image format
%  to the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterHISTOGRAMImage method is:
%
%      RegisterHISTOGRAMImage(void)
%
*/
ModuleExport void RegisterHISTOGRAMImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("HISTOGRAM");
  entry->encoder=(EncoderHandler *) WriteHISTOGRAMImage;
  entry->adjoin=MagickFalse;
  entry->description=AcquireString("Histogram of the image");
  entry->module=AcquireString("HISTOGRAM");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r H I S T O G R A M I m a g e                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterHISTOGRAMImage() removes format registrations made by the
%  HISTOGRAM module from the list of supported formats.
%
%  The format of the UnregisterHISTOGRAMImage method is:
%
%      UnregisterHISTOGRAMImage(void)
%
*/
ModuleExport void UnregisterHISTOGRAMImage(void)
{
  (void) UnregisterMagickInfo("HISTOGRAM");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e H I S T O G R A M I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteHISTOGRAMImage() writes an image to a file in Histogram format.
%  The image shows a histogram of the color (or gray) values in the image.  The
%  image consists of three overlaid histograms:  a red one for the red channel,
%  a green one for the green channel, and a blue one for the blue channel.  The
%  image comment contains a list of unique pixel values and the number of times
%  each occurs in the image.
%
%  This method is strongly based on a similar one written by
%  muquit@warm.semcor.com which in turn is based on ppmhistmap of netpbm.
%
%  The format of the WriteHISTOGRAMImage method is:
%
%      MagickBooleanType WriteHISTOGRAMImage(const ImageInfo *image_info,
%        Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteHISTOGRAMImage(const ImageInfo *image_info,
  Image *image)
{
#define HistogramDensity  "256x200"

  ChannelType
    channel;

  char
    filename[MaxTextExtent];

  FILE
    *file;

  Image
    *histogram_image;

  int
    unique_file;

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    *histogram;

  MagickRealType
    maximum,
    scale;

  RectangleInfo
    geometry;

  register const PixelPacket
    *p;

  register long
    x;

  register PixelPacket
    *q,
    *r;

  size_t
    length;

  /*
    Allocate histogram image.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  (void) SetImageDepth(image,image->depth);
  SetGeometry(image,&geometry);
  if (image_info->density == (char *) NULL)
    (void) ParseAbsoluteGeometry(HistogramDensity,&geometry);
  else
    (void) ParseAbsoluteGeometry(image_info->density,&geometry);
  histogram_image=CloneImage(image,geometry.width,geometry.height,MagickTrue,
    &image->exception);
  if (histogram_image == (Image *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  histogram_image->storage_class=DirectClass;
  /*
    Allocate histogram count arrays.
  */
  length=(size_t) Max((unsigned long) ScaleQuantumToChar(QuantumRange)+1,
    histogram_image->columns);
  histogram=(MagickPixelPacket *)
    AcquireMagickMemory(length*sizeof(*histogram));
  if (histogram == (MagickPixelPacket *) NULL)
    {
      histogram_image=DestroyImage(histogram_image);
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Initialize histogram count arrays.
  */
  channel=image_info->channel;
  (void) ResetMagickMemory(histogram,0,length*sizeof(*histogram));
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
    if (p == (const PixelPacket *) NULL)
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      if ((channel & RedChannel) != 0)
        histogram[ScaleQuantumToChar(p->red)].red++;
      if ((channel & GreenChannel) != 0)
        histogram[ScaleQuantumToChar(p->green)].green++;
      if ((channel & BlueChannel) != 0)
        histogram[ScaleQuantumToChar(p->blue)].blue++;
      p++;
    }
  }
  maximum=histogram[0].red;
  for (x=0; x < (long) histogram_image->columns; x++)
  {
    if ((channel & RedChannel) != 0)
      if (maximum < histogram[x].red)
        maximum=histogram[x].red;
    if ((channel & GreenChannel) != 0)
      if (maximum < histogram[x].green)
        maximum=histogram[x].green;
    if ((channel & BlueChannel) != 0)
      if (maximum < histogram[x].blue)
        maximum=histogram[x].blue;
  }
  scale=(MagickRealType) histogram_image->rows/maximum;
  /*
    Initialize histogram image.
  */
  (void) QueryColorDatabase("#000000",&histogram_image->background_color,
    &image->exception);
  SetImageBackgroundColor(histogram_image);
  for (x=0; x < (long) histogram_image->columns; x++)
  {
    q=GetImagePixels(histogram_image,x,0,1,histogram_image->rows);
    if (q == (PixelPacket *) NULL)
      break;
    if ((channel & RedChannel) != 0)
      {
        y=(long) (histogram_image->rows-scale*histogram[x].red+0.5);
        r=q+y;
        for ( ; y < (long) histogram_image->rows; y++)
        {
          r->red=QuantumRange;
          r++;
        }
      }
    if ((channel & GreenChannel) != 0)
      {
        y=(long) (histogram_image->rows-scale*histogram[x].green+0.5);
        r=q+y;
        for ( ; y < (long) histogram_image->rows; y++)
        {
          r->green=QuantumRange;
          r++;
        }
      }
    if ((channel & BlueChannel) != 0)
      {
        y=(long) (histogram_image->rows-scale*histogram[x].blue+0.5);
        r=q+y;
        for ( ; y < (long) histogram_image->rows; y++)
        {
          r->blue=QuantumRange;
          r++;
        }
      }
    if (SyncImagePixels(histogram_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,histogram_image->rows) != MagickFalse))
      {
        status=image->progress_monitor(SaveImageTag,y,histogram_image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  /*
    Free resources.
  */
  histogram=(MagickPixelPacket *) RelinquishMagickMemory(histogram);
  file=(FILE *) NULL;
  unique_file=AcquireUniqueFileResource(filename);
  if (unique_file != -1)
    file=fdopen(unique_file,"wb");
  if ((unique_file != -1) && (file != (FILE *) NULL))
    {
      char
        command[MaxTextExtent];

      /*
        Add a histogram as an image comment.
      */
      (void) GetNumberColors(image,file,&image->exception);
      (void) fclose(file);
      (void) FormatMagickString(command,MaxTextExtent,"@%s",filename);
      (void) SetImageAttribute(histogram_image,"Comment",command);
    }
  (void) RelinquishUniqueFileResource(filename);
  /*
    Write Histogram image as MIFF.
  */
  (void) CopyMagickString(filename,histogram_image->filename,MaxTextExtent);
  (void) FormatMagickString(histogram_image->filename,MaxTextExtent,
    "miff:%s",filename);
  status=WriteImage(image_info,histogram_image);
  histogram_image=DestroyImage(histogram_image);
  return(status);
}
