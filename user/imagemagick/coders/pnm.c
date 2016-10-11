/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            PPPP   N   N  M   M                              %
%                            P   P  NN  N  MM MM                              %
%                            PPPP   N N N  M M M                              %
%                            P      N  NN  M   M                              %
%                            P      N   N  M   M                              %
%                                                                             %
%                                                                             %
%               Read/Write PBMPlus Portable Anymap Image Format.              %
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
#include "magick/colorspace.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/static.h"
#include "magick/string_.h"

/*
  Forward declarations.
*/
static MagickBooleanType
  WritePNMImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s P N M                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsPNM() returns MagickTrue if the image format type, identified by the
%  magick string, is PNM.
%
%  The format of the IsPNM method is:
%
%      MagickBooleanType IsPNM(const unsigned char *magick,const size_t length)
%
%  A description of each parameter follows:
%
%    o magick: This string is generally the first few bytes of an image file
%      or blob.
%
%    o length: Specifies the length of the magick string.
%
%
*/
static MagickBooleanType IsPNM(const unsigned char *magick,const size_t length)
{
  if (length < 2)
    return(MagickFalse);
  if ((*magick == (unsigned char) 'P') &&
      (isdigit((int) magick[1]) != MagickFalse))
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d P N M I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadPNMImage() reads a Portable Anymap image file and returns it.
%  It allocates the memory necessary for the new Image structure and returns
%  a pointer to the new image.
%
%  The format of the ReadPNMImage method is:
%
%      Image *ReadPNMImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/

static unsigned long PNMInteger(Image *image,const unsigned int base)
{
#define P7Comment  "END_OF_COMMENTS\n"

  char
    *comment;

  int
    c;

  register char
    *p;

  size_t
    length;

  unsigned long
    value;

  /*
    Skip any leading whitespace.
  */
  length=MaxTextExtent;
  comment=(char *) NULL;
  p=comment;
  do
  {
    c=ReadBlobByte(image);
    if (c == EOF)
      return(0);
    if (c == (int) '#')
      {
        /*
          Read comment.
        */
        if (comment == (char *) NULL)
          comment=AcquireString((char *) NULL);
        p=comment+strlen(comment);
        for ( ; (c != EOF) && (c != (int) '\n'); p++)
        {
          if ((size_t) (p-comment+1) >= length)
            {
              length<<=1;
              comment=(char *) ResizeMagickMemory(comment,
                (length+MaxTextExtent)*sizeof(*comment));
              if (comment == (char *) NULL)
                break;
              p=comment+strlen(comment);
            }
          c=ReadBlobByte(image);
          *p=(char) c;
          *(p+1)='\0';
        }
        if (comment == (char *) NULL)
          return(0);
        continue;
      }
  } while (isdigit(c) == MagickFalse);
  if (comment != (char *) NULL)
    {
      if (strlen(comment) > strlen(P7Comment))
        {
          p-=strlen(P7Comment);
          if (LocaleCompare(p,P7Comment) == 0)
            *p='\0';
        }
      (void) SetImageAttribute(image,"Comment",comment);
      comment=(char *) RelinquishMagickMemory(comment);
    }
  if (base == 2)
    return((unsigned long) (c-(int) '0'));
  /*
    Evaluate number.
  */
  value=0;
  do
  {
    value*=10;
    value+=c-(int) '0';
    c=ReadBlobByte(image);
    if (c == EOF)
      return(value);
  }
  while (isdigit(c) != MagickFalse);
  return(value);
}

static Image *ReadPNMImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
#define PushCharPixel(pixel,p) \
{ \
  pixel=(unsigned long) (*(p)); \
  (p)++; \
}
#define PushShortPixel(pixel,p) \
{ \
  pixel=(unsigned long) ((*(p) << 8) | *((p)+1)); \
  (p)+=2; \
}

  char
    format;

  Image
    *image;

  long
    y;

  LongPixelPacket
    pixel;

  MagickBooleanType
    status;

  Quantum
    *scale;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  register long
    i;

  register unsigned char
    *p;

  size_t
    packets;

  ssize_t
    count;

  unsigned char
    *pixels;

  unsigned long
    index,
    max_value;

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
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  /*
    Read PNM image.
  */
  count=ReadBlob(image,1,(unsigned char *) &format);
  do
  {
    /*
      Initialize image structure.
    */
    if ((count != 1) || (format != 'P'))
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    format=(char) ReadBlobByte(image);
    if (format == '7')
      (void) PNMInteger(image,10);
    image->columns=PNMInteger(image,10);
    image->rows=PNMInteger(image,10);
    if ((format == '1') || (format == '4'))
      max_value=1;  /* bitmap */
    else
      max_value=PNMInteger(image,10);
    image->depth=max_value < 256 ? 8UL : QuantumDepth;
    if ((format != '3') && (format != '6'))
      {
        image->storage_class=PseudoClass;
        image->colors=(unsigned long) (max_value >= MaxColormapSize ?
          MaxColormapSize : max_value+1);
      }
    if ((image->columns == 0) || (image->rows == 0))
      ThrowReaderException(CorruptImageError,"NegativeOrZeroImageSize");
    scale=(Quantum *) NULL;
    if (image->storage_class == PseudoClass)
      {
        /*
          Create colormap.
        */
        if (AllocateImageColormap(image,image->colors) == MagickFalse)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        if ((format == '7') && (image->colors == 256))
          {
            /*
              Initialize 332 colormap.
            */
            i=0;
            for (pixel.red=0; pixel.red < 8; pixel.red++)
              for (pixel.green=0; pixel.green < 8; pixel.green++)
                for (pixel.blue=0; pixel.blue < 4; pixel.blue++)
                {
                  image->colormap[i].red=ScaleAnyToQuantum(pixel.red,0x07);
                  image->colormap[i].green=ScaleAnyToQuantum(pixel.green,0x07);
                  image->colormap[i].blue=ScaleAnyToQuantum(pixel.blue,0x03);
                  i++;
                }
          }
      }
    if ((image->storage_class != PseudoClass) || (max_value > QuantumRange))
      {
        /*
          Compute pixel scaling table.
        */
        scale=(Quantum *)
          AcquireMagickMemory((size_t) (max_value+1)*sizeof(*scale));
        if (scale == (Quantum *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        for (i=0; i <= (long) max_value; i++)
          scale[i]=ScaleAnyToQuantum(i,max_value);
      }
    if ((image_info->ping != MagickFalse) && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    /*
      Convert PNM pixels to runlength-encoded MIFF packets.
    */
    switch (format)
    {
      case '1':
      {
        /*
          Convert PBM image to pixel packets.
        */
        for (y=0; y < (long) image->rows; y++)
        {
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          indexes=GetIndexes(image);
          for (x=0; x < (long) image->columns; x++)
          {
            index=(IndexPacket) (PNMInteger(image,2) == 0);
            if ((unsigned long) index >= image->colors)
              {
                (void) ThrowMagickException(&image->exception,GetMagickModule(),
                  CorruptImageError,"InvalidColormapIndex","`%s'",
                  image->filename);
                index=0;
              }
            indexes[x]=(IndexPacket) index;
            *q++=image->colormap[index];
          }
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
        break;
      }
      case '2':
      {
        unsigned long
          intensity;

        /*
          Convert PGM image to pixel packets.
        */
        for (y=0; y < (long) image->rows; y++)
        {
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          indexes=GetIndexes(image);
          for (x=0; x < (long) image->columns; x++)
          {
            intensity=PNMInteger(image,10);
            if (scale != (Quantum *) NULL)
              intensity=scale[intensity];
            index=(IndexPacket) intensity;
            if ((unsigned long) index >= image->colors)
              {
                (void) ThrowMagickException(&image->exception,GetMagickModule(),
                  CorruptImageError,"InvalidColormapIndex","`%s'",
                  image->filename);
                index=0;
              }
            indexes[x]=(IndexPacket) index;
            *q++=image->colormap[index];
          }
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
        break;
      }
      case '3':
      {
        /*
          Convert PNM image to pixel packets.
        */
        for (y=0; y < (long) image->rows; y++)
        {
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x++)
          {
            pixel.red=PNMInteger(image,10);
            pixel.green=PNMInteger(image,10);
            pixel.blue=PNMInteger(image,10);
            if (scale != (Quantum *) NULL)
              {
                pixel.red=scale[pixel.red];
                pixel.green=scale[pixel.green];
                pixel.blue=scale[pixel.blue];
              }
            q->red=(Quantum) pixel.red;
            q->green=(Quantum) pixel.green;
            q->blue=(Quantum) pixel.blue;
            q++;
          }
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
        break;
      }
      case '4':
      {
        unsigned long
          bit,
          byte;

        /*
          Convert PBM raw image to pixel packets.
        */
        for (y=0; y < (long) image->rows; y++)
        {
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          indexes=GetIndexes(image);
          bit=0;
          byte=0;
          for (x=0; x < (long) image->columns; x++)
          {
            if (bit == 0)
              byte=(unsigned long) ReadBlobByte(image);
            index=(IndexPacket) ((byte & 0x80) != 0 ? 0x00 : 0x01);
            indexes[x]=(IndexPacket) index;
            *q++=image->colormap[index];
            bit++;
            if (bit == 8)
              bit=0;
            byte<<=1;
          }
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
        if (EOFBlob(image) != MagickFalse)
          ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
            image->filename);
        break;
      }
      case '5':
      case '7':
      {
        /*
          Convert PGM raw image to pixel packets.
        */
        packets=(size_t) (image->depth <= 8 ? 1 : 2);
        pixels=(unsigned char *)
          AcquireMagickMemory((size_t) packets*image->columns);
        if (pixels == (unsigned char *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        for (y=0; y < (long) image->rows; y++)
        {
          count=ReadBlob(image,(size_t) packets*image->columns,pixels);
          if (count != (ssize_t) (packets*image->columns))
            ThrowReaderException(CorruptImageError,"UnableToReadImageData");
          p=pixels;
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          indexes=GetIndexes(image);
          if (image->depth <= 8)
            for (x=0; x < (long) image->columns; x++)
            {
              index=(IndexPacket) (*p++);
              if ((unsigned long) index >= image->colors)
                {
                  (void) ThrowMagickException(&image->exception,
                    GetMagickModule(),CorruptImageError,"InvalidColormapIndex",
                    "`%s'",image->filename);
                  index=0;
                }
              indexes[x]=(IndexPacket) index;
              *q++=image->colormap[index];
            }
          else
            for (x=0; x < (long) image->columns; x++)
            {
              PushShortPixel(index,p);
              if (index >= image->colors)
                {
                  (void) ThrowMagickException(&image->exception,
                    GetMagickModule(),CorruptImageError,"InvalidColormapIndex",
                    "`%s'",image->filename);
                  index=0;
                }
              indexes[x]=(IndexPacket) index;
              *q++=image->colormap[index];
            }
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
        pixels=(unsigned char *) RelinquishMagickMemory(pixels);
        if (EOFBlob(image) != MagickFalse)
          (void) ThrowMagickException(exception,GetMagickModule(),
            CorruptImageError,"UnexpectedEndOfFile","`%s'",image->filename);
        break;
      }
      case '6':
      {
        /*
          Convert PNM raster image to pixel packets.
        */
        packets=(size_t) (image->depth <= 8 ? 3 : 6);
        pixels=(unsigned char *) AcquireMagickMemory(packets*image->columns);
        if (pixels == (unsigned char *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        for (y=0; y < (long) image->rows; y++)
        {
          count=ReadBlob(image,packets*image->columns,pixels);
          if (count != (ssize_t) (packets*image->columns))
            ThrowReaderException(CorruptImageError,"UnableToReadImageData");
          p=pixels;
          q=SetImagePixels(image,0,y,image->columns,1);
          if (q == (PixelPacket *) NULL)
            break;
          if (image->depth <= 8)
            for (x=0; x < (long) image->columns; x++)
            {
              PushCharPixel(pixel.red,p);
              PushCharPixel(pixel.green,p);
              PushCharPixel(pixel.blue,p);
              if (scale != (Quantum *) NULL)
                {
                  pixel.red=scale[pixel.red];
                  pixel.green=scale[pixel.green];
                  pixel.blue=scale[pixel.blue];
                }
              q->red=(Quantum) pixel.red;
              q->green=(Quantum) pixel.green;
              q->blue=(Quantum) pixel.blue;
              q++;
            }
          else
            for (x=0; x < (long) image->columns; x++)
            {
              PushShortPixel(pixel.red,p);
              PushShortPixel(pixel.green,p);
              PushShortPixel(pixel.blue,p);
              if (scale != (Quantum *) NULL)
                {
                  pixel.red=scale[pixel.red];
                  pixel.green=scale[pixel.green];
                  pixel.blue=scale[pixel.blue];
                }
              q->red=(Quantum) pixel.red;
              q->green=(Quantum) pixel.green;
              q->blue=(Quantum) pixel.blue;
              q++;
            }
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
        pixels=(unsigned char *) RelinquishMagickMemory(pixels);
        if (EOFBlob(image) != MagickFalse)
          (void) ThrowMagickException(exception,GetMagickModule(),
            CorruptImageError,"UnexpectedEndOfFile","`%s'",image->filename);
        break;
      }
      default:
        ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    }
    if (scale != (Quantum *) NULL)
      scale=(Quantum *) RelinquishMagickMemory(scale);
    /*
      Proceed to next image.
    */
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    if ((format == '1') || (format == '2') || (format == '3'))
      do
      {
        /*
          Skip to end of line.
        */
        count=ReadBlob(image,1,(unsigned char *) &format);
        if (count == 0)
          break;
        if ((count != 0) && (format == 'P'))
          break;
      } while (format != '\n');
    count=ReadBlob(image,1,(unsigned char *) &format);
    if ((count == 1) && (format == 'P'))
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
  } while ((count == 1) && (format == 'P'));
  CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r P N M I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterPNMImage() adds attributes for the PNM image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterPNMImage method is:
%
%      RegisterPNMImage(void)
%
*/
ModuleExport void RegisterPNMImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("P7");
  entry->decoder=(DecoderHandler *) ReadPNMImage;
  entry->encoder=(EncoderHandler *) WritePNMImage;
  entry->description=AcquireString("Xv thumbnail format");
  entry->module=AcquireString("PNM");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PBM");
  entry->decoder=(DecoderHandler *) ReadPNMImage;
  entry->encoder=(EncoderHandler *) WritePNMImage;
  entry->description=AcquireString("Portable bitmap format (black and white)");
  entry->module=AcquireString("PNM");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PGM");
  entry->decoder=(DecoderHandler *) ReadPNMImage;
  entry->encoder=(EncoderHandler *) WritePNMImage;
  entry->description=AcquireString("Portable graymap format (gray scale)");
  entry->module=AcquireString("PNM");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PNM");
  entry->decoder=(DecoderHandler *) ReadPNMImage;
  entry->encoder=(EncoderHandler *) WritePNMImage;
  entry->magick=(MagickHandler *) IsPNM;
  entry->description=AcquireString("Portable anymap");
  entry->module=AcquireString("PNM");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PPM");
  entry->decoder=(DecoderHandler *) ReadPNMImage;
  entry->encoder=(EncoderHandler *) WritePNMImage;
  entry->description=AcquireString("Portable pixmap format (color)");
  entry->module=AcquireString("PNM");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r P N M I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterPNMImage() removes format registrations made by the
%  PNM module from the list of supported formats.
%
%  The format of the UnregisterPNMImage method is:
%
%      UnregisterPNMImage(void)
%
*/
ModuleExport void UnregisterPNMImage(void)
{
  (void) UnregisterMagickInfo("P7");
  (void) UnregisterMagickInfo("PBM");
  (void) UnregisterMagickInfo("PGM");
  (void) UnregisterMagickInfo("PNM");
  (void) UnregisterMagickInfo("PPM");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e P N M I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Procedure WritePNMImage() writes an image to a file in the PNM rasterfile
%  format.
%
%  The format of the WritePNMImage method is:
%
%      MagickBooleanType WritePNMImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WritePNMImage(const ImageInfo *image_info,Image *image)
{
  char
    buffer[MaxTextExtent];

  const ImageAttribute
    *attribute;

  IndexPacket
    index;

  int
    format;

  long
    j,
    y;

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

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
  scene=0;
  do
  {
    /*
      Write PNM file header.
    */
    (void) SetImageColorspace(image,RGBColorspace);
    format=6;
    if (LocaleCompare(image_info->magick,"PGM") == 0)
      format=5;
    else
      if (LocaleCompare(image_info->magick,"PBM") == 0)
        format=4;
      else
        if ((LocaleCompare(image_info->magick,"PNM") == 0) &&
            (image_info->type != TrueColorType) &&
            (IsGrayImage(image,&image->exception) != MagickFalse))
          {
            format=5;
            if (IsMonochromeImage(image,&image->exception) != MagickFalse)
              format=4;
          }
    if (image->compression == NoCompression)
      format-=3;
    if (LocaleCompare(image_info->magick,"P7") != 0)
      (void) FormatMagickString(buffer,MaxTextExtent,"P%d\n",format);
    else
      {
        format=7;
        (void) strcpy(buffer,"P7 332\n");
      }
    (void) WriteBlobString(image,buffer);
    attribute=GetImageAttribute(image,"Comment");
    if (attribute != (const ImageAttribute *) NULL)
      {
        register char
          *p;

        /*
          Write comments to file.
        */
        (void) WriteBlobByte(image,'#');
        for (p=attribute->value; *p != '\0'; p++)
        {
          (void) WriteBlobByte(image,(unsigned char) *p);
          if ((*p == '\r') && (*(p+1) != '\0'))
            (void) WriteBlobByte(image,'#');
          if ((*p == '\n') && (*(p+1) != '\0'))
            (void) WriteBlobByte(image,'#');
        }
        (void) WriteBlobByte(image,'\n');
      }
    if (format != 7)
      {
        (void) FormatMagickString(buffer,MaxTextExtent,"%lu %lu\n",
          image->columns,image->rows);
        (void) WriteBlobString(image,buffer);
      }
    /*
      Convert runlength encoded to PNM raster pixels.
    */
    switch (format)
    {
      case 1:
      {
        IndexPacket
          polarity;

        /*
          Convert image to a PBM image.
        */
        (void) SetImageType(image,BilevelType);
        polarity=(IndexPacket)
          (PixelIntensityToQuantum(&image->colormap[0]) < (QuantumRange/2));
        if (image->colors == 2)
          polarity=(IndexPacket)
            (PixelIntensityToQuantum(&image->colormap[0]) <
             PixelIntensityToQuantum(&image->colormap[1]));
        i=0;
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          indexes=GetIndexes(image);
          for (x=0; x < (long) image->columns; x++)
          {
            (void) FormatMagickString(buffer,MaxTextExtent,"%u ",
              indexes[x] == polarity ? 0x00 : 0x01);
            (void) WriteBlobString(image,buffer);
            i++;
            if (i == 36)
              {
                (void) WriteBlobByte(image,'\n');
                i=0;
              }
          }
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
        if (i != 0)
          (void) WriteBlobByte(image,'\n');
        break;
      }
      case 2:
      {
        /*
          Convert image to a PGM image.
        */
        if (image->depth <= 8)
          (void) WriteBlobString(image,"255\n");
        else
          (void) WriteBlobString(image,"65535\n");
        i=0;
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x++)
          {
            index=PixelIntensityToQuantum(p);
            if (image->depth <= 8)
              (void) FormatMagickString(buffer,MaxTextExtent," %u",
                ScaleQuantumToChar(index));
            else
              (void) FormatMagickString(buffer,MaxTextExtent," %u",
                ScaleQuantumToShort(index));
            (void) WriteBlobString(image,buffer);
            i++;
            if (i == 12)
              {
                (void) WriteBlobByte(image,'\n');
                i=0;
              }
            p++;
          }
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
        if (i != 0)
          (void) WriteBlobByte(image,'\n');
        break;
      }
      case 3:
      {
        /*
          Convert image to a PNM image.
        */
        if (image->depth <= 8)
          (void) WriteBlobString(image,"255\n");
        else
          (void) WriteBlobString(image,"65535\n");
        i=0;
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x++)
          {
            if (image->depth <= 8)
              (void) FormatMagickString(buffer,MaxTextExtent,"%u %u %u ",
                ScaleQuantumToChar(p->red),ScaleQuantumToChar(p->green),
                ScaleQuantumToChar(p->blue));
            else
              (void) FormatMagickString(buffer,MaxTextExtent,"%u %u %u ",
                ScaleQuantumToShort(p->red),ScaleQuantumToShort(p->green),
                ScaleQuantumToShort(p->blue));
            (void) WriteBlobString(image,buffer);
            i++;
            if (i == 4)
              {
                (void) WriteBlobByte(image,'\n');
                i=0;
              }
            p++;
          }
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
        if (i != 0)
          (void) WriteBlobByte(image,'\n');
        break;
      }
      case 4:
      {
        IndexPacket
          polarity;

        unsigned long
          bit,
          byte;

        /*
          Convert image to a PBM image.
        */
        (void) SetImageType(image,BilevelType);
        polarity=(IndexPacket)
          (PixelIntensityToQuantum(&image->colormap[0]) < (QuantumRange/2));
        if (image->colors == 2)
          polarity=(IndexPacket)
            (PixelIntensityToQuantum(&image->colormap[0]) <
             PixelIntensityToQuantum(&image->colormap[1]));
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          indexes=GetIndexes(image);
          bit=0;
          byte=0;
          for (x=0; x < (long) image->columns; x++)
          {
            byte<<=1;
            if (indexes[x] != polarity)
              byte|=0x01;
            bit++;
            if (bit == 8)
              {
                (void) WriteBlobByte(image,(unsigned char) byte);
                bit=0;
                byte=0;
              }
            p++;
          }
          if (bit != 0)
            (void) WriteBlobByte(image,(unsigned char) (byte << (8-bit)));
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
        break;
      }
      case 5:
      {
        /*
          Convert image to a PGM image.
        */
        if (image->depth <= 8)
          (void) WriteBlobString(image,"255\n");
        else
          (void) WriteBlobString(image,"65535\n");
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          if (image->depth <= 8)
            for (x=0; x < (long) image->columns; x++)
            {
              (void) WriteBlobByte(image,
                ScaleQuantumToChar(PixelIntensityToQuantum(p)));
              p++;
            }
          else
            for (x=0; x < (long) image->columns; x++)
            {
              (void) WriteBlobMSBShort(image,
                ScaleQuantumToShort(PixelIntensityToQuantum(p)));
              p++;
            }
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
        break;
      }
      case 6:
      {
        register unsigned char
          *q;

        size_t
          packets;

        unsigned char
          *pixels;

        /*
          Allocate memory for pixels.
        */
        packets=(size_t) (image->depth <= 8 ? 3 : 6);
        pixels=(unsigned char *) AcquireMagickMemory(packets*image->columns);
        if (pixels == (unsigned char *) NULL)
          ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
        /*
          Convert image to a PNM image.
        */
        if (image->depth <= 8)
          (void) WriteBlobString(image,"255\n");
        else
          (void) WriteBlobString(image,"65535\n");
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          q=pixels;
          if (image->depth <= 8)
            for (x=0; x < (long) image->columns; x++)
            {
              *q++=ScaleQuantumToChar(p->red);
              *q++=ScaleQuantumToChar(p->green);
              *q++=ScaleQuantumToChar(p->blue);
              p++;
            }
          else
            for (x=0; x < (long) image->columns; x++)
            {
              *q++=(unsigned char) (ScaleQuantumToShort(p->red) >> 8);
              *q++=(unsigned char) ScaleQuantumToShort(p->red);
              *q++=(unsigned char) (ScaleQuantumToShort(p->green) >> 8);
              *q++=(unsigned char) ScaleQuantumToShort(p->green);
              *q++=(unsigned char) (ScaleQuantumToShort(p->blue) >> 8);
              *q++=(unsigned char) ScaleQuantumToShort(p->blue);
              p++;
            }
          (void) WriteBlob(image,(size_t) (q-pixels),pixels);
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
        pixels=(unsigned char *) RelinquishMagickMemory(pixels);
        break;
      }
      case 7:
      {
        static const long
          dither_red[2][16]=
          {
            {-16,  4, -1, 11,-14,  6, -3,  9,-15,  5, -2, 10,-13,  7, -4,  8},
            { 15, -5,  0,-12, 13, -7,  2,-10, 14, -6,  1,-11, 12, -8,  3, -9}
          },
          dither_green[2][16]=
          {
            { 11,-15,  7, -3,  8,-14,  4, -2, 10,-16,  6, -4,  9,-13,  5, -1},
            {-12, 14, -8,  2, -9, 13, -5,  1,-11, 15, -7,  3,-10, 12, -6,  0}
          },
          dither_blue[2][16]=
          {
            { -3,  9,-13,  7, -1, 11,-15,  5, -4,  8,-14,  6, -2, 10,-16,  4},
            {  2,-10, 12, -8,  0,-12, 14, -6,  3, -9, 13, -7,  1,-11, 15, -5}
          };

        long
          value;

        LongPixelPacket
          pixel;

        Quantum
          quantum;

        unsigned long
          *blue_map[2][16],
          *green_map[2][16],
          *red_map[2][16];

        /*
          Allocate and initialize dither maps.
        */
        for (i=0; i < 2; i++)
          for (j=0; j < 16; j++)
          {
            red_map[i][j]=(unsigned long *)
              AcquireMagickMemory(256*sizeof(*red_map));
            green_map[i][j]=(unsigned long *)
              AcquireMagickMemory(256*sizeof(*green_map));
            blue_map[i][j]=(unsigned long *)
              AcquireMagickMemory(256*sizeof(*blue_map));
            if ((red_map[i][j] == (unsigned long *) NULL) ||
                (green_map[i][j] == (unsigned long *) NULL) ||
                (blue_map[i][j] == (unsigned long *) NULL))
              ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
          }
        /*
          Initialize dither tables.
        */
        for (i=0; i < 2; i++)
          for (j=0; j < 16; j++)
            for (x=0; x < 256; x++)
            {
              value=x-16;
              if (x < 48)
                value=x/2+8;
              value+=dither_red[i][j];
              red_map[i][j][x]=(unsigned long)
                ((value < 0) ? 0 : (value > 255) ? 255 : value);
              value=x-16;
              if (x < 48)
                value=x/2+8;
              value+=dither_green[i][j];
              green_map[i][j][x]=(unsigned long)
                ((value < 0) ? 0 : (value > 255) ? 255 : value);
              value=x-32;
              if (x < 112)
                value=x/2+24;
              value+=2*dither_blue[i][j];
              blue_map[i][j][x]=(unsigned long)
                ((value < 0) ? 0 : (value > 255) ? 255 : value);
            }
        /*
          Convert image to a P7 image.
        */
        (void) WriteBlobString(image,"#END_OF_COMMENTS\n");
        (void) FormatMagickString(buffer,MaxTextExtent,"%lu %lu 255\n",
          image->columns,image->rows);
        (void) WriteBlobString(image,buffer);
        i=0;
        j=0;
        for (y=0; y < (long) image->rows; y++)
        {
          p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
          if (p == (const PixelPacket *) NULL)
            break;
          for (x=0; x < (long) image->columns; x++)
          {
            pixel.red=(unsigned long) ScaleQuantumToChar(p->red);
            pixel.green=(unsigned long) ScaleQuantumToChar(p->green);
            pixel.blue=(unsigned long) ScaleQuantumToChar(p->blue);
            if (image_info->dither == MagickFalse)
              quantum=(Quantum) ((pixel.red & 0xe0) |
                ((pixel.green & 0xe0) >> 3) | ((pixel.blue & 0xc0) >> 6));
            else
              quantum=(Quantum) ((red_map[i][j][pixel.red] & 0xe0) |
                ((green_map[i][j][pixel.green] & 0xe0) >> 3) |
                ((blue_map[i][j][pixel.blue] & 0xc0) >> 6));
            (void) WriteBlobByte(image,(unsigned char) quantum);
            p++;
            j++;
            if (j == 16)
              j=0;
          }
          i++;
          if (i == 2)
            i=0;
          if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
              (QuantumTick(y,image->rows) != MagickFalse))
            {
              status=image->progress_monitor(SaveImageTag,y,image->rows,
                image->client_data);
              if (status == MagickFalse)
                break;
            }
        }
        /*
          Free allocated memory.
        */
        for (i=0; i < 2; i++)
          for (j=0; j < 16; j++)
          {
            green_map[i][j]=(unsigned long *)
              RelinquishMagickMemory(green_map[i][j]);
            blue_map[i][j]=(unsigned long *)
              RelinquishMagickMemory(blue_map[i][j]);
            red_map[i][j]=(unsigned long *)
              RelinquishMagickMemory(red_map[i][j]);
          }
        break;
      }
    }
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
