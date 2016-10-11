/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                             CCCC  IIIII  N   N                              %
%                            C        I    NN  N                              %
%                            C        I    N N N                              %
%                            C        I    N  NN                              %
%                             CCCC  IIIII  N   N                              %
%                                                                             %
%                                                                             %
%                    Read/Write Kodak Cineon Image Format.                    %
%                Cineon Image Format is a subset of SMTPE DPX                 %
%                                                                             %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                             Kelly Bergougnoux                               %
%                               October 2003                                  %
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
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Cineon image file format draft is available at
%  http://www.cineon.com/ff_draft.php.
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
  Typedef declaration.
*/
typedef struct _CINFileInfo
{
  unsigned long
    magic,
    image_offset,
    generic_length,
    industry_length,
    user_length,
    file_size;

  char
    version[8],
    filename[100],
    date[12],
    time[12],
    reserve[36];
} CINFileInfo;

/*
  Forward declaractions.
*/
static MagickBooleanType
  WriteCINImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s C I N E O N                                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsCIN() returns MagickTrue if the image format type, identified by the magick
%  string, is CIN.
%
%  The format of the IsCIN method is:
%
%      MagickBooleanType IsCIN(const unsigned char *magick,const size_t length)
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
static MagickBooleanType IsCIN(const unsigned char *magick,const size_t length)
{
  if (length < 4)
    return(MagickFalse);
  if (memcmp(magick,"\200\052\137\327",4) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d C I N E O N I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadCINImage() reads an CIN X image file and returns it.  It allocates
%  the memory necessary for the new Image structure and returns a point to the
%  new image.
%
%  The format of the ReadCINImage method is:
%
%      Image *ReadCINImage(const ImageInfo *image_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadCINImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
#define MonoColorType  1
#define RGBColorType  3

  char
    magick[4];

  Image
    *image;

  long
    y;

  MagickBooleanType
    status;

  QuantumType
    quantum_type;

  register const unsigned char
    *p;

  register long
    i;

  register PixelPacket
    *q;

  size_t
    length;

  ssize_t
    count;

  unsigned char
    colortype,
    *pixels;

  unsigned long
    headersize;

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
    Read CIN image.
  */
  count=ReadBlob(image,4,(unsigned char *) magick);
  if ((count != 4) ||
      ((LocaleNCompare((char *) magick,"\200\052\137\327",4) != 0)))
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  headersize=ReadBlobLong(image);
  for (i=0; i < 185; i++)
    (void) ReadBlobByte(image);
  colortype=(unsigned char) ReadBlobByte(image);
  for (i=0; i < 4; i++)
    (void) ReadBlobByte(image);
  image->depth=ReadBlobByte(image);
  (void) ReadBlobByte(image);
  image->columns=ReadBlobLong(image);
  image->rows=ReadBlobLong(image);
  (void) SeekBlob(image,(MagickOffsetType) headersize,SEEK_SET);
  if (image_info->ping)
    {
      CloseBlob(image);
      return(image);
    }
  /*
    Convert CIN raster image to pixel packets.
  */
  length=4*image->columns*sizeof(*pixels);
  if (image->depth >= 12)
    length=6*image->columns*sizeof(*pixels);
  pixels=(unsigned char *) AcquireMagickMemory(length);
  if (pixels == (unsigned char *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  quantum_type=RGBPadQuantum;
  if (colortype == MonoColorType)
    quantum_type=GrayQuantum;
  for (y=0; y < (long) image->rows; y++)
  {
    q=SetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    p=ReadBlobStream(image,length,pixels,&count);
    if ((size_t) count != length)
      break;
    status=ExportQuantumPixels(image,quantum_type,0,p);
    if (status == MagickFalse)
      break;
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
  if (EOFBlob(image))
    ThrowFileException(exception,CorruptImageError,"UnexpectedEndOfFile",
      image->filename);
  image->colorspace=LogColorspace;
  CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r C I N E O N I m a g e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterCINImage() adds attributes for the CIN image format to the list of
%  of supported formats.  The attributes include the image format tag, a method
%  to read and/or write the format, whether the format supports the saving of
%  more than one frame to the same file or blob, whether the format supports
%  native in-memory I/O, and a brief description of the format.
%
%  The format of the RegisterCINImage method is:
%
%      RegisterCINImage(void)
%
*/
ModuleExport void RegisterCINImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("CIN");
  entry->decoder=(DecoderHandler *) ReadCINImage;
  entry->encoder=(EncoderHandler *) WriteCINImage;
  entry->magick=(MagickHandler *) IsCIN;
  entry->description=AcquireString("Cineon Image File");
  entry->module=AcquireString("CIN");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r C I N E O N I m a g e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterCINImage() removes format registrations made by the CIN module
%  from the list of supported formats.
%
%  The format of the UnregisterCINImage method is:
%
%      UnregisterCINImage(void)
%
*/
ModuleExport void UnregisterCINImage(void)
{
  (void) UnregisterMagickInfo("CINEON");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e C I N E O N I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteCINImage() writes an image in CIN encoded image format.
%
%  The format of the WriteCINImage method is:
%
%      MagickBooleanType WriteCINImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteCINImage(const ImageInfo *image_info,Image *image)
{
  long
    y;

  MagickBooleanType
    status;

  PixelPacket
    max_color,
    min_color;

  register const PixelPacket
    *p;

  register long
    i,
    x;

  size_t
    length;

  ssize_t
    count;

  unsigned char
    *pixels;

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
  image->depth=10;
  (void) SetImageColorspace(image,LogColorspace);
  /*
    Compute range for each color component (10-bits).
  */
  max_color.red=0;
	max_color.green=0;
	max_color.blue=0;
  min_color.red=QuantumRange;
	min_color.green=QuantumRange;
	min_color.blue=QuantumRange;
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
    if (p == (const PixelPacket *) NULL)
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      if (p->red > max_color.red)
        max_color.red=p->red;
      if (p->green > max_color.green)
        max_color.green=p->green;
      if (p->blue > max_color.blue)
        max_color.blue=p->blue;
      if (p->red < min_color.red)
        min_color.red=p->red;
      if (p->green < min_color.green)
        min_color.green=p->green;
      if (p->blue < min_color.blue)
        min_color.blue=p->blue;
      p++;
    }
  }
  max_color.red/=64;
  max_color.green/=64;
  max_color.blue/=64;
  min_color.red/=64;
  min_color.green/=64;
  min_color.blue/=64;
  /*
    Write image header.
  */
  (void) WriteBlobLong(image,0x802A5FD7UL);  /* magick number */
  (void) WriteBlobLong(image,0x0800);  /* Offset to image */
  (void) WriteBlobLong(image,0x400);  /* Generic section header length */
  (void) WriteBlobLong(image,0x400);  /* Industry Specific lenght */
  (void) WriteBlobLong(image,0x0);  /* variable length section */
  (void) WriteBlobLong(image,4*image->columns*image->rows+0x2000);  /* total image file size */
  (void) WriteBlobString(image,"V4.5");  /* version */
  (void) WriteBlobLong(image,0x0);
  (void) WriteBlobString(image,image->filename);
  for (i=0 ; i < (long) (100-strlen(image->filename)); i++ )
    (void) WriteBlobByte(image,0);
  (void) WriteBlobString(image,"yyyy:mm:dd  ");  /* creation date. */
  (void) WriteBlobString(image,"hh:mm:ssxxx ");  /* creation time. */
  for (i=0 ; i < 36 ; i++)
    (void) WriteBlobByte(image,0);
  (void) WriteBlobByte(image,0);  /* 0 left to right top to bottom */
  (void) WriteBlobByte(image,3);  /* 3 channels */
  (void) WriteBlobByte(image,0x0);  /* alignment */
  (void) WriteBlobByte(image,0x0);  /* red */
  (void) WriteBlobByte(image,0);  /* Channel 1 designator byte 0 */
  (void) WriteBlobByte(image,1);  /* Channel 1 designator byte 1 */
  (void) WriteBlobByte(image,image->depth);  /* Bits per pixel... */
  (void) WriteBlobByte(image,0);  /* alignment */
  (void) WriteBlobLong(image,image->columns);  /* pixels per line */
  (void) WriteBlobLong(image,image->rows);  /* lines per image */
  (void) WriteBlobLong(image,min_color.red);  /* Minimum data value */
  (void) WriteBlobLong(image,0x0);  /* Minimum quantity represented */
  (void) WriteBlobLong(image,max_color.red);  /* Maximum data value */
  (void) WriteBlobLong(image,0x40000000);  /* Maximum quantity represented */
  (void) WriteBlobByte(image,0);  /* green */
  (void) WriteBlobByte(image,2);
  (void) WriteBlobByte(image,image->depth);
  (void) WriteBlobByte(image,0);
  (void) WriteBlobLong(image,image->columns);
  (void) WriteBlobLong(image,image->rows);
  (void) WriteBlobLong(image,min_color.green);
  (void) WriteBlobLong(image,0x0);
  (void) WriteBlobLong(image,max_color.green);
  (void) WriteBlobLong(image,0x40000000);
  (void) WriteBlobByte(image,0);  /* blue */
  (void) WriteBlobByte(image,3);
  (void) WriteBlobByte(image,image->depth);
  (void) WriteBlobByte(image,0);
  (void) WriteBlobLong(image,image->columns);
  (void) WriteBlobLong(image,image->rows);
  (void) WriteBlobLong(image,min_color.blue);
  (void) WriteBlobLong(image,0x0);
  (void) WriteBlobLong(image,max_color.blue);
  (void) WriteBlobLong(image,0x40000000);
  /*
	  Pad channel 4-8.
  */
  for (i=0; i < (5*28); i++)
    (void) WriteBlobByte(image,0x00);
  (void) WriteBlobLong(image,0x4EFF0000);  /* White point(colour temp.) x */
  (void) WriteBlobLong(image,0x4EFF0000);  /* White point(colour temp.) y */
  (void) WriteBlobLong(image,0x4EFF0000);  /* Red primary chromaticity x */
  (void) WriteBlobLong(image,0x4EFF0000);  /* Red primary chromaticity y */
  (void) WriteBlobLong(image,0x4EFF0000);  /* Green primary chromaticity x */
  (void) WriteBlobLong(image,0x4EFF0000);  /* Green primary chromaticity y */
  (void) WriteBlobLong(image,0x4EFF0000);  /* Blue primary chromaticity x */
  (void) WriteBlobLong(image,0x4EFF0000);  /* Blue primary chromaticity y */
  /*
    Label text.
  */
  for (i=0; i < (200+28); i++)
    (void) WriteBlobByte(image,0x00);
  (void) WriteBlobByte(image,0);  /* pixel interleave (rgbrgbr...) */
  (void) WriteBlobByte(image,5);  /* packing longword (32bit) boundaries */
  (void) WriteBlobByte(image,0);  /* data unsigned */
  (void) WriteBlobByte(image,0);  /* image sense: positive image */
  (void) WriteBlobLong(image,0x0);  /* end of line padding */
  (void) WriteBlobLong(image,0x0);  /* end of channel padding */
  /*
    Reseved for future Use.
  */
  for (i=0; i < 20; i++)
    (void) WriteBlobByte(image,0x00);
  (void) WriteBlobLong(image,0x0);  /* x offset */
  (void) WriteBlobLong(image,0x0);  /* y offset */
  (void) WriteBlobString(image,image->filename);
  for (i=0 ; i < (long) (100-strlen(image->filename)); i++ )
    (void) WriteBlobByte(image,0);
  /*
    Date.
  */
  for (i=0 ; i < 12; i++ )
    (void) WriteBlobByte(image,0);
  /*
    Time.
  */
  for (i =0 ; i < 12; i++ )
    (void) WriteBlobByte(image,0);
  (void) WriteBlobString(image,"ImageMagick");
  for (i=0; i < (64-11); i++)
    (void) WriteBlobByte(image,0);
  for (i=0; i < 32; i++)
    (void) WriteBlobByte(image,0);
  for (i=0; i < 32; i++)
    (void) WriteBlobByte(image,0);
  (void) WriteBlobLong(image,0x4326AB85);  /* X input device pitch */
  (void) WriteBlobLong(image,0x4326AB85);  /* Y input device pitch */
  (void) WriteBlobLong(image,0x3F800000);  /* Image gamma */
  /*
    Reserved for future use.
  */
  for (i=0; i < 40; i++)
    (void) WriteBlobByte(image,0);
  for (i=0 ; i < 4; i++)
    (void) WriteBlobByte(image,0);
  (void) WriteBlobLong(image,0x0);
  (void) WriteBlobLong(image,0x0);
  for (i=0 ; i < 32; i++)
    (void) WriteBlobByte(image,0);
  (void) WriteBlobLong(image,0x0);
  (void) WriteBlobLong(image,0x0);
  for (i=0 ; i < 32; i++)
    (void) WriteBlobByte(image,0);
  for (i=0 ; i < 200; i++)
    (void) WriteBlobByte(image,0);
  for (i=0 ; i < 740; i++)
    (void) WriteBlobByte(image,0);
  /*
    Convert pixel packets to CIN raster image.
  */
  length=4*image->columns*sizeof(*pixels);
  if (image->depth >= 12)
    length=6*image->columns*sizeof(*pixels);
  pixels=(unsigned char *) AcquireMagickMemory(length);
  if (pixels == (unsigned char *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
    if (p == (const PixelPacket *) NULL)
      break;
    status=ImportQuantumPixels(image,RGBPadQuantum,0,pixels);
    if (status == MagickFalse)
      break;
    count=WriteBlob(image,length,pixels);
    if (count != (ssize_t) length)
      break;
  }
  pixels=(unsigned char *) RelinquishMagickMemory(pixels);
  CloseBlob(image);
  return(status);
}
