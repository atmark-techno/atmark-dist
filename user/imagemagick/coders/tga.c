/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            TTTTT   GGGG   AAA                               %
%                              T    G      A   A                              %
%                              T    G  GG  AAAAA                              %
%                              T    G   G  A   A                              %
%                              T     GGG   A   A                              %
%                                                                             %
%                                                                             %
%                    Read/Write Truevision Targa Image Format.                %
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
  WriteTGAImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d T G A I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadTGAImage() reads a Truevision TGA image file and returns it.
%  It allocates the memory necessary for the new Image structure and returns
%  a pointer to the new image.
%
%  The format of the ReadTGAImage method is:
%
%      Image *ReadTGAImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadTGAImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
#define TGAColormap 1
#define TGARGB 2
#define TGAMonochrome 3
#define TGARLEColormap  9
#define TGARLERGB  10
#define TGARLEMonochrome  11

  typedef struct _TGAInfo
  {
    unsigned char
      id_length,
      colormap_type,
      image_type;

    unsigned short
      colormap_index,
      colormap_length;

    unsigned char
      colormap_size;

    unsigned short
      x_origin,
      y_origin,
      width,
      height;

    unsigned char
      bits_per_pixel,
      attributes;
  } TGAInfo;

  Image
    *image;

  IndexPacket
    index;

  long
    y;

  MagickBooleanType
    status;

  PixelPacket
    pixel;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  register PixelPacket
    *q;

  ssize_t
    count;

  TGAInfo
    tga_info;

  unsigned char
    j,
    k,
    runlength;

  unsigned long
    base,
    flag,
    offset,
    real,
    skip;

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
    Read TGA header information.
  */
  count=ReadBlob(image,1,&tga_info.id_length);
  tga_info.colormap_type=(unsigned char) ReadBlobByte(image);
  tga_info.image_type=(unsigned char) ReadBlobByte(image);
  do
  {
    if ((count != 1) ||
        ((tga_info.image_type != TGAColormap) &&
         (tga_info.image_type != TGARGB) &&
         (tga_info.image_type != TGAMonochrome) &&
         (tga_info.image_type != TGARLEColormap) &&
         (tga_info.image_type != TGARLERGB) &&
         (tga_info.image_type != TGARLEMonochrome)) ||
        (((tga_info.image_type == TGAColormap) ||
         (tga_info.image_type == TGARLEColormap)) &&
         (tga_info.colormap_type == 0)))
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    tga_info.colormap_index=ReadBlobLSBShort(image);
    tga_info.colormap_length=ReadBlobLSBShort(image);
    tga_info.colormap_size=(unsigned char) ReadBlobByte(image);
    tga_info.x_origin=ReadBlobLSBShort(image);
    tga_info.y_origin=ReadBlobLSBShort(image);
    tga_info.width=ReadBlobLSBShort(image);
    tga_info.height=ReadBlobLSBShort(image);
    tga_info.bits_per_pixel=(unsigned char) ReadBlobByte(image);
    tga_info.attributes=(unsigned char) ReadBlobByte(image);
    /*
      Initialize image structure.
    */
    image->matte=(tga_info.attributes & 0x0FU) != 0 ? MagickTrue : MagickFalse;
    image->columns=tga_info.width;
    image->rows=tga_info.height;
    if ((tga_info.image_type != TGAColormap) &&
        (tga_info.image_type != TGARLEColormap))
      image->depth=(unsigned long) ((tga_info.bits_per_pixel <= 8) ? 8 :
        (tga_info.bits_per_pixel <= 16) ? 5 :
        (tga_info.bits_per_pixel == 24) ? 8 :
        (tga_info.bits_per_pixel == 32) ? 8 : 8);
    else
      image->depth=(unsigned long) ((tga_info.colormap_size <= 8) ? 8 :
        (tga_info.colormap_size <= 16) ? 5 :
        (tga_info.colormap_size == 24) ? 8 :
        (tga_info.colormap_size == 32) ? 8 : 8);
    if ((tga_info.image_type == TGAColormap) ||
        (tga_info.image_type == TGAMonochrome) ||
        (tga_info.image_type == TGARLEColormap) ||
        (tga_info.image_type == TGARLEMonochrome))
      image->storage_class=PseudoClass;
    image->compression=NoCompression;
    if ((tga_info.image_type == TGARLEColormap) ||
        (tga_info.image_type == TGARLEMonochrome))
      image->compression=RLECompression;
    if (image->storage_class == PseudoClass)
      {
        if (tga_info.colormap_type != 0)
          image->colors=tga_info.colormap_length;
        else
          {
            image->colors=0x01U << tga_info.bits_per_pixel;
            if (AllocateImageColormap(image,image->colors) == MagickFalse)
              ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
          }
      }
    if (tga_info.id_length != 0)
      {
        char
          *comment;

        /*
          TGA image comment.
        */
        comment=(char *) AcquireMagickMemory(tga_info.id_length+MaxTextExtent);
        if (comment == (char *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        count=ReadBlob(image,tga_info.id_length,(unsigned char *) comment);
        comment[tga_info.id_length]='\0';
        (void) SetImageAttribute(image,"Comment",comment);
        comment=(char *) RelinquishMagickMemory(comment);
      }
    (void) ResetMagickMemory(&pixel,0,sizeof(pixel));
    pixel.opacity=TransparentOpacity;
    if (tga_info.colormap_type != 0)
      {
        /*
          Read TGA raster colormap.
        */
        if (AllocateImageColormap(image,image->colors) == MagickFalse)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        for (i=0; i < (long) image->colors; i++)
        {
          switch (tga_info.colormap_size)
          {
            case 8:
            default:
            {
              /*
                Gray scale.
              */
              pixel.red=ScaleCharToQuantum(ReadBlobByte(image));
              pixel.green=pixel.red;
              pixel.blue=pixel.red;
              break;
            }
            case 15:
            case 16:
            {
              /*
                5 bits each of red green and blue.
              */
              j=(unsigned char) ReadBlobByte(image);
              k=(unsigned char) ReadBlobByte(image);
              pixel.red=ScaleAnyToQuantum((int) (k & 0x7c) >> 2,31);
              pixel.green=ScaleAnyToQuantum(((int) (k & 0x03) << 3)+
                ((int) (j & 0xe0) >> 5),31);
              pixel.blue=ScaleAnyToQuantum((int) (j & 0x1f),31);
              break;
            }
            case 24:
            case 32:
            {
              /*
                8 bits each of blue, green and red.
              */
              pixel.blue=ScaleCharToQuantum(ReadBlobByte(image));
              pixel.green=ScaleCharToQuantum(ReadBlobByte(image));
              pixel.red=ScaleCharToQuantum(ReadBlobByte(image));
              break;
            }
          }
          image->colormap[i]=pixel;
        }
      }
    if ((image_info->ping != MagickFalse) && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    /*
      Convert TGA pixels to pixel packets.
    */
    base=0;
    flag=0;
    skip=MagickFalse;
    real=0;
    index=0;
    runlength=0;
    offset=0;
    for (y=0; y < (long) image->rows; y++)
    {
      real=offset;
      if (((unsigned char) (tga_info.attributes & 0x20) >> 5) == 0)
        real=image->rows-real-1;
      q=SetImagePixels(image,0,(long) real,image->columns,1);
      if (q == (PixelPacket *) NULL)
        break;
      indexes=GetIndexes(image);
      for (x=0; x < (long) image->columns; x++)
      {
        if ((tga_info.image_type == TGARLEColormap) ||
            (tga_info.image_type == TGARLERGB) ||
            (tga_info.image_type == TGARLEMonochrome))
          {
            if (runlength != 0)
              {
                runlength--;
                skip=flag != 0;
              }
            else
              {
                count=ReadBlob(image,1,&runlength);
                if (count == 0)
                  ThrowReaderException(CorruptImageError,
                    "UnableToReadImageData");
                flag=runlength & 0x80;
                if (flag != 0)
                  runlength-=128;
                skip=MagickFalse;
              }
          }
        if (skip == MagickFalse)
          switch (tga_info.bits_per_pixel)
          {
            case 8:
            default:
            {
              /*
                Gray scale.
              */
              index=(IndexPacket) ReadBlobByte(image);
              if (tga_info.colormap_type != 0)
                pixel=image->colormap[ConstrainColormapIndex(image,index)];
              else
                {
                  pixel.red=ScaleCharToQuantum(index);
                  pixel.green=ScaleCharToQuantum(index);
                  pixel.blue=ScaleCharToQuantum(index);
                }
              break;
            }
            case 15:
            case 16:
            {
              /*
                5 bits each of red green and blue.
              */
              j=(unsigned char) ReadBlobByte(image);
              k=(unsigned char) ReadBlobByte(image);
              pixel.red=ScaleAnyToQuantum((int) (k & 0x7c) >> 2,31);
              pixel.red=ScaleAnyToQuantum(((int) (k & 0x03) << 3)+
                ((int) (j & 0xe0) >> 5),31);
              pixel.blue=ScaleAnyToQuantum((int) (j & 0x1f),31);
              if (image->storage_class == PseudoClass)
                index=ConstrainColormapIndex(image,((unsigned long) k << 8)+j);
              break;
            }
            case 24:
            case 32:
            {
              /*
                8 bits each of blue green and red.
              */
              pixel.blue=ScaleCharToQuantum(ReadBlobByte(image));
              pixel.green=ScaleCharToQuantum(ReadBlobByte(image));
              pixel.red=ScaleCharToQuantum(ReadBlobByte(image));
              if (tga_info.bits_per_pixel == 32)
                pixel.opacity=QuantumRange-ScaleCharToQuantum(ReadBlobByte(image));
              break;
            }
          }
        if (status == MagickFalse)
          ThrowReaderException(CorruptImageError,"UnableToReadImageData");
        if (image->storage_class == PseudoClass)
          indexes[x]=index;
        *q++=pixel;
      }
      if (((unsigned char) (tga_info.attributes & 0xc0) >> 6) == 4)
        offset+=4;
      else
        if (((unsigned char) (tga_info.attributes & 0xc0) >> 6) == 2)
          offset+=2;
        else
          offset++;
      if (offset >= image->rows)
        {
          base++;
          offset=base;
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
      {
        ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
          image->filename);
        break;
      }
    /*
      Proceed to next image.
    */
    if (image_info->number_scenes != 0)
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    count=ReadBlob(image,1,&tga_info.id_length);
    tga_info.colormap_type=(unsigned char) ReadBlobByte(image);
    tga_info.image_type=(unsigned char) ReadBlobByte(image);
    status=((tga_info.image_type == TGAColormap) ||
      (tga_info.image_type == TGARGB) ||
      (tga_info.image_type == TGAMonochrome) ||
      (tga_info.image_type == TGARLEColormap) ||
      (tga_info.image_type == TGARLERGB) ||
      (tga_info.image_type == TGARLEMonochrome)) ? MagickTrue : MagickFalse;
    if (status == MagickTrue)
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
  } while (status == MagickTrue);
  CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r T G A I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterTGAImage() adds attributes for the TGA image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterTGAImage method is:
%
%      RegisterTGAImage(void)
%
*/
ModuleExport void RegisterTGAImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("ICB");
  entry->decoder=(DecoderHandler *) ReadTGAImage;
  entry->encoder=(EncoderHandler *) WriteTGAImage;
  entry->description=AcquireString("Truevision Targa image");
  entry->module=AcquireString("TGA");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("TGA");
  entry->decoder=(DecoderHandler *) ReadTGAImage;
  entry->encoder=(EncoderHandler *) WriteTGAImage;
  entry->description=AcquireString("Truevision Targa image");
  entry->module=AcquireString("TGA");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("VDA");
  entry->decoder=(DecoderHandler *) ReadTGAImage;
  entry->encoder=(EncoderHandler *) WriteTGAImage;
  entry->description=AcquireString("Truevision Targa image");
  entry->module=AcquireString("TGA");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("VST");
  entry->decoder=(DecoderHandler *) ReadTGAImage;
  entry->encoder=(EncoderHandler *) WriteTGAImage;
  entry->description=AcquireString("Truevision Targa image");
  entry->module=AcquireString("TGA");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r T G A I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterTGAImage() removes format registrations made by the
%  TGA module from the list of supported formats.
%
%  The format of the UnregisterTGAImage method is:
%
%      UnregisterTGAImage(void)
%
*/
ModuleExport void UnregisterTGAImage(void)
{
  (void) UnregisterMagickInfo("ICB");
  (void) UnregisterMagickInfo("TGA");
  (void) UnregisterMagickInfo("VDA");
  (void) UnregisterMagickInfo("VST");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e T G A I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteTGAImage() writes a image in the Truevision Targa rasterfile
%  format.
%
%  The format of the WriteTGAImage method is:
%
%      MagickBooleanType WriteTGAImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteTGAImage(const ImageInfo *image_info,Image *image)
{
#define TargaColormap 1
#define TargaRGB 2
#define TargaMonochrome 3
#define TargaRLEColormap  9
#define TargaRLERGB  10
#define TargaRLEMonochrome  11

  typedef struct _TargaInfo
  {
    unsigned char
      id_length,
      colormap_type,
      image_type;

    unsigned short
      colormap_index,
      colormap_length;

    unsigned char
      colormap_size;

    unsigned short
      x_origin,
      y_origin,
      width,
      height;

    unsigned char
      bits_per_pixel,
      attributes;
  } TargaInfo;

  const ImageAttribute
    *attribute;

  long
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
    x;

  register long
    i;

  register unsigned char
    *q;

  ssize_t
    count;

  TargaInfo
    targa_info;

  unsigned char
    *targa_pixels;

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
      Initialize TGA raster file header.
    */
    (void) SetImageColorspace(image,RGBColorspace);
    targa_info.id_length=0;
    attribute=GetImageAttribute(image,"Comment");
    if (attribute != (const ImageAttribute *) NULL)
      targa_info.id_length=(unsigned char) Min(strlen(attribute->value),255);
    targa_info.colormap_type=0;
    targa_info.colormap_index=0;
    targa_info.colormap_length=0;
    targa_info.colormap_size=0;
    targa_info.x_origin=0;
    targa_info.y_origin=0;
    targa_info.width=(unsigned short) image->columns;
    targa_info.height=(unsigned short) image->rows;
    targa_info.bits_per_pixel=8;
    targa_info.attributes=0;
    if ((image_info->type != TrueColorType) &&
        (image_info->type != TrueColorMatteType) &&
        (image_info->type != PaletteType) &&
        (image->matte == MagickFalse) &&
        (IsGrayImage(image,&image->exception) != MagickFalse))
      targa_info.image_type=TargaMonochrome;
    else
      if ((image->storage_class == DirectClass) || (image->colors > 256))
        {
          /*
            Full color TGA raster.
          */
          targa_info.image_type=TargaRGB;
          targa_info.bits_per_pixel=24;
          if (image->matte != MagickFalse)
            {
              targa_info.bits_per_pixel=32;
              targa_info.attributes=8;  /* # of alpha bits */
            }
        }
      else
        {
          /*
            Colormapped TGA raster.
          */
          targa_info.image_type=TargaColormap;
          targa_info.colormap_type=1;
          targa_info.colormap_length=(unsigned short) image->colors;
          targa_info.colormap_size=24;
        }
    /*
      Write TGA header.
    */
    (void) WriteBlobByte(image,targa_info.id_length);
    (void) WriteBlobByte(image,targa_info.colormap_type);
    (void) WriteBlobByte(image,targa_info.image_type);
    (void) WriteBlobLSBShort(image,targa_info.colormap_index);
    (void) WriteBlobLSBShort(image,targa_info.colormap_length);
    (void) WriteBlobByte(image,targa_info.colormap_size);
    (void) WriteBlobLSBShort(image,targa_info.x_origin);
    (void) WriteBlobLSBShort(image,targa_info.y_origin);
    (void) WriteBlobLSBShort(image,targa_info.width);
    (void) WriteBlobLSBShort(image,targa_info.height);
    (void) WriteBlobByte(image,targa_info.bits_per_pixel);
    (void) WriteBlobByte(image,targa_info.attributes);
    if (targa_info.id_length != 0)
      (void) WriteBlob(image,targa_info.id_length,(unsigned char *)
        attribute->value);
    if (targa_info.image_type == TargaColormap)
      {
        unsigned char
          *targa_colormap;

        /*
          Dump colormap to file (blue, green, red byte order).
        */
        targa_colormap=(unsigned char *)
          AcquireMagickMemory(3*targa_info.colormap_length);
        if (targa_colormap == (unsigned char *) NULL)
          ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
        q=targa_colormap;
        for (i=0; i < (long) image->colors; i++)
        {
          *q++=ScaleQuantumToChar(image->colormap[i].blue);
          *q++=ScaleQuantumToChar(image->colormap[i].green);
          *q++=ScaleQuantumToChar(image->colormap[i].red);
        }
        (void) WriteBlob(image,3*targa_info.colormap_length,targa_colormap);
        targa_colormap=(unsigned char *) RelinquishMagickMemory(targa_colormap);
      }
    /*
      Convert MIFF to TGA raster pixels.
    */
    count=(ssize_t) ((targa_info.bits_per_pixel*targa_info.width) >> 3);
    targa_pixels=(unsigned char *) AcquireMagickMemory((size_t) count);
    if (targa_pixels == (unsigned char *) NULL)
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    for (y=(long) (image->rows-1); y >= 0; y--)
    {
      p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
      if (p == (const PixelPacket *) NULL)
        break;
      q=targa_pixels;
      indexes=GetIndexes(image);
      for (x=0; x < (long) image->columns; x++)
      {
        if (targa_info.image_type == TargaColormap)
          *q++=(unsigned char) indexes[x];
        else
          if (targa_info.image_type == TargaMonochrome)
            *q++=(unsigned char) ScaleQuantumToChar(PixelIntensityToQuantum(p));
          else
            {
              *q++=ScaleQuantumToChar(p->blue);
              *q++=ScaleQuantumToChar(p->green);
              *q++=ScaleQuantumToChar(p->red);
              if (image->matte != MagickFalse)
                *q++=QuantumRange-ScaleQuantumToChar(p->opacity);
              if (image->colorspace == CMYKColorspace)
                *q++=ScaleQuantumToChar(indexes[x]);
            }
        p++;
      }
      (void) WriteBlob(image,(size_t) (q-targa_pixels),targa_pixels);
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
    targa_pixels=(unsigned char *) RelinquishMagickMemory(targa_pixels);
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
