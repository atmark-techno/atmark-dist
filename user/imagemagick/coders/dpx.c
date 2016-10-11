/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            DDDD   PPPP   X   X                              %
%                            D   D  P   P   X X                               %
%                            D   D  PPPP    XXX                               %
%                            D   D  P       X X                               %
%                            DDDD   P      X   X                              %
%                                                                             %
%                                                                             %
%                     Read/Write SMTPE DPX Image Format.                      %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                March 2001                                   %
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
#include "magick/version.h"

/*
  Typedef declaration.
*/
typedef struct _DPXFileInfo
{
  unsigned long
    magic,
    image_offset;

  char
    version[8];

  unsigned long
    file_size,
    ditto_key,
    generic_size,
    industry_size,
    user_size;

  char
    filename[100],
    timestamp[24],
    creator[100],
    project[200],
    copyright[200];

  unsigned long
    encrypt_key;

  char
    reserve[104];
} DPXFileInfo;

typedef struct _DPXFilmInfo
{
  char
    id[2],
    type[2],
    offset[2],
    prefix[6],
    count[4],
    format[32];

  unsigned long
    frame_position,
    sequence_length,
    held_count;

  float
    frame_rate,
    shutter_angle;

  char
    frame_id[32],
    slate[100],
    reserve[56];
} DPXFilmInfo;

typedef struct _DPXImageElement
{
  unsigned long
    data_sign,
    low_data;

  float
    low_quantity;

  unsigned long
    high_data;

  float
    high_quantity;

  unsigned char
    descriptor,
    transfer,
    colorimetric,
    bit_size;

  unsigned short
    packing,
    encoding;

  unsigned long
    data_offset,
    end_of_line_padding,
    end_of_image_padding;

  unsigned char
    description[32];
} DPXImageElement;

typedef struct _DPXImageInfo
{
  unsigned short
    orientation,
    number_elements;

  unsigned long
    pixels_per_line,
    lines_per_element;

  DPXImageElement
    image_element[8];

  unsigned char
    reserve[52];
} DPXImageInfo;

typedef struct _DPXOrientationInfo
{
  unsigned long
    x_offset,
    y_offset;

  float
    x_center,
    y_center;

  unsigned long
    x_size,
    y_size;

  char
    filename[100],
    timestamp[24],
    device[32],
    serial[32];

  unsigned short
    border[4];

  unsigned long
    aspect_ratio[2];

  unsigned char
    reserve[28];
} DPXOrientationInfo;

typedef struct _DPXTelevisionInfo
{
  unsigned long
    time_code,
    user_bits;

  unsigned char
    interlace,
    field_number,
    video_signal,
    padding;

  float
    horizontal_sample_rate,
    vertical_sample_rate,
    frame_rate,
    time_offset,
    gamma,
    black_level,
    black_gain,
    break_point,
    white_level,
    integration_times;

  char
    reserve[76];
} DPXTelevisionInfo;

typedef struct _DPXUserInfo
{
  char
    id[32];
} DPXUserInfo;

typedef struct DPXInfo
{
  DPXFileInfo
    file;

  DPXImageInfo
    image;

  DPXOrientationInfo
    orientation;

  DPXFilmInfo
    film;

  DPXTelevisionInfo
    television;

  DPXUserInfo
    user;
} DPXInfo;

/*
  Forward declaractions.
*/
static MagickBooleanType
  WriteDPXImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s D P X                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsDPX() returns MagickTrue if the image format type, identified by the
%  magick string, is DPX.
%
%  The format of the IsDPX method is:
%
%      MagickBooleanType IsDPX(const unsigned char *magick,const size_t length)
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
static MagickBooleanType IsDPX(const unsigned char *magick,const size_t length)
{
  if (length < 4)
    return(MagickFalse);
  if (memcmp(magick,"SDPX",4) == 0)
    return(MagickTrue);
  if (memcmp(magick,"XPDS",4) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d D P X I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadDPXImage() reads an DPX X image file and returns it.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadDPXImage method is:
%
%      Image *ReadDPXImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadDPXImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
#define MonoColorType  6
#define RGBColorType  50

  char
    magick[4];

  DPXInfo
    dpx;

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

  ssize_t
    count;

  size_t
    length;

  unsigned char
    colortype,
    single[16],
    *pixels;

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
    Read DPX file header.
  */
  count=ReadBlob(image,4,(unsigned char *) magick);
  if ((count != 4) || ((LocaleNCompare(magick,"SDPX",4) != 0) &&
      (LocaleNCompare((char *) magick,"XPDS",4) != 0)))
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  image->endian=LSBEndian;
  if (LocaleNCompare(magick,"SDPX",4) == 0)
    image->endian=MSBEndian;
  dpx.file.image_offset=ReadBlobLong(image);
  (void) ReadBlob(image,8,(unsigned char *) dpx.file.version);
  (void) SetImageAttribute(image,"dpx:file.version",dpx.file.version);
  dpx.file.file_size=ReadBlobLong(image);
  dpx.file.ditto_key=ReadBlobLong(image);
  if (dpx.file.ditto_key != (unsigned long) (~0))
    (void) FormatImageAttribute(image,"dpx:file.ditto.key","%lu",
      dpx.file.ditto_key);
  dpx.file.generic_size=ReadBlobLong(image);
  dpx.file.industry_size=ReadBlobLong(image);
  dpx.file.user_size=ReadBlobLong(image);
  (void) ReadBlob(image,100,(unsigned char *) dpx.file.filename);
  (void) SetImageAttribute(image,"dpx:file.filename",dpx.file.filename);
  if (*dpx.file.filename != '\0')
    (void) FormatImageAttribute(image,"dpx:file.filename","%.100s",
      dpx.file.filename);
  (void) ReadBlob(image,24,(unsigned char *) dpx.file.timestamp);
  if (*dpx.file.timestamp != '\0')
    (void) FormatImageAttribute(image,"dpx:file.tilestamp","%.24s",
      dpx.file.timestamp);
  (void) ReadBlob(image,100,(unsigned char *) dpx.file.creator);
  if (*dpx.file.creator != '\0')
    (void) FormatImageAttribute(image,"dpx:file.creator","%.100s",
      dpx.file.creator);
  (void) ReadBlob(image,200,(unsigned char *) dpx.file.project);
  if (*dpx.file.project != '\0')
    (void) FormatImageAttribute(image,"dpx:file.project","%.200s",
      dpx.file.project);
  (void) ReadBlob(image,200,(unsigned char *) dpx.file.copyright);
  if (*dpx.file.copyright != '\0')
    (void) FormatImageAttribute(image,"dpx:file.copyright","%.200s",
      dpx.file.copyright);
  dpx.file.encrypt_key=ReadBlobLong(image);
  if (dpx.file.encrypt_key != (unsigned long) (~0))
    (void) FormatImageAttribute(image,"dpx:file.encrypt_key","%lu",
      dpx.file.encrypt_key);
  (void) ReadBlob(image,104,(unsigned char *) dpx.file.reserve);
  /*
    Read DPX image header.
  */
  dpx.image.orientation=ReadBlobShort(image);
  if (dpx.image.orientation != (unsigned short) (~0))
    (void) FormatImageAttribute(image,"dpx:image.orientation","%d",
      dpx.image.orientation);
  switch (dpx.image.orientation)
  {
    default:
    case 0:  image->orientation=TopLeftOrientation; break;
    case 1:  image->orientation=TopRightOrientation; break;
    case 2:  image->orientation=BottomLeftOrientation; break;
    case 3:  image->orientation=BottomRightOrientation; break;
    case 4:  image->orientation=LeftTopOrientation; break;
    case 5:  image->orientation=RightTopOrientation; break;
    case 6:  image->orientation=LeftBottomOrientation; break;
    case 7:  image->orientation=RightBottomOrientation; break;
  }
  dpx.image.number_elements=ReadBlobShort(image);
  dpx.image.pixels_per_line=ReadBlobLong(image);
  image->columns=dpx.image.pixels_per_line;
  dpx.image.lines_per_element=ReadBlobLong(image);
  image->rows=dpx.image.lines_per_element;
  for (i=0; i < 8; i++)
  {
    dpx.image.image_element[i].data_sign=ReadBlobLong(image);
    dpx.image.image_element[i].low_data=ReadBlobLong(image);
    (void) ReadBlob(image,4,single);
    dpx.image.image_element[i].low_quantity=((float *) single)[0];
    dpx.image.image_element[i].high_data=ReadBlobLong(image);
    (void) ReadBlob(image,4,single);
    dpx.image.image_element[i].high_quantity=((float *) single)[0];
    dpx.image.image_element[i].descriptor=ReadBlobByte(image);
    dpx.image.image_element[i].transfer=ReadBlobByte(image);
    dpx.image.image_element[i].colorimetric=ReadBlobByte(image);
    dpx.image.image_element[i].bit_size=ReadBlobByte(image);
    dpx.image.image_element[i].packing=ReadBlobShort(image);
    dpx.image.image_element[i].encoding=ReadBlobShort(image);
    dpx.image.image_element[i].data_offset=ReadBlobLong(image);
    dpx.image.image_element[i].end_of_line_padding=ReadBlobLong(image);
    dpx.image.image_element[i].end_of_image_padding=ReadBlobLong(image);
    (void) ReadBlob(image,32,(unsigned char *)
      dpx.image.image_element[i].description);
  }
  (void) ReadBlob(image,52,(unsigned char *) dpx.image.reserve);
  colortype=dpx.image.image_element[0].descriptor;
  image->depth=dpx.image.image_element[0].bit_size;
  /*
    Read DPX orientation header.
  */
  dpx.orientation.x_offset=ReadBlobLong(image);
  dpx.orientation.y_offset=ReadBlobLong(image);
  (void) ReadBlob(image,4,single);
  dpx.orientation.x_center=((float *) single)[0];
  (void) ReadBlob(image,4,single);
  dpx.orientation.y_center=((float *) single)[0];
  dpx.orientation.x_size=ReadBlobLong(image);
  dpx.orientation.y_size=ReadBlobLong(image);
  (void) ReadBlob(image,100,(unsigned char *) dpx.orientation.filename);
  if (*dpx.orientation.filename != '\0')
    (void) FormatImageAttribute(image,"dpx:orientation.copyright","%.100s",
      dpx.orientation.filename);
  (void) ReadBlob(image,24,(unsigned char *) dpx.orientation.timestamp);
  if (*dpx.orientation.timestamp != '\0')
    (void) FormatImageAttribute(image,"dpx:orientation.timestamp","%.24s",
      dpx.orientation.timestamp);
  (void) ReadBlob(image,32,(unsigned char *) dpx.orientation.device);
  if (*dpx.orientation.device != '\0')
    (void) FormatImageAttribute(image,"dpx:orientation.device","%.32s",
      dpx.orientation.device);
  (void) ReadBlob(image,32,(unsigned char *) dpx.orientation.serial);
  if (*dpx.orientation.serial != '\0')
    (void) FormatImageAttribute(image,"dpx:orientation.serial","%.32s",
      dpx.orientation.serial);
  for (i=0; i < 4; i++)
    dpx.orientation.border[i]=ReadBlobShort(image);
  for (i=0; i < 2; i++)
    dpx.orientation.aspect_ratio[i]=ReadBlobLong(image);
  (void) ReadBlob(image,28,(unsigned char *) dpx.orientation.reserve);
  /*
    Read DPX film header.
  */
  (void) ReadBlob(image,2,(unsigned char *) dpx.film.id);
  if (*dpx.film.type != '\0')
    (void) FormatImageAttribute(image,"dpx:film.id","%.2s",dpx.film.id);
  (void) ReadBlob(image,2,(unsigned char *) dpx.film.type);
  if (*dpx.film.type != '\0')
    (void) FormatImageAttribute(image,"dpx:film.type","%.2s",dpx.film.type);
  (void) ReadBlob(image,2,(unsigned char *) dpx.film.offset);
  if (*dpx.film.offset != '\0')
    (void) FormatImageAttribute(image,"dpx:film.offset","%.2s",dpx.film.offset);
  (void) ReadBlob(image,6,(unsigned char *) dpx.film.prefix);
  if (*dpx.film.prefix != '\0')
    (void) FormatImageAttribute(image,"dpx:film.prefix","%.6s",dpx.film.prefix);
  (void) ReadBlob(image,4,(unsigned char *) dpx.film.count);
  if (*dpx.film.count != '\0')
    (void) FormatImageAttribute(image,"dpx:film.count","%.4s",dpx.film.count);
  (void) ReadBlob(image,32,(unsigned char *) dpx.film.format);
  if (*dpx.film.format != '\0')
    (void) FormatImageAttribute(image,"dpx:film.format","%.4s",dpx.film.format);
  dpx.film.frame_position=ReadBlobLong(image);
  dpx.film.sequence_length=ReadBlobLong(image);
  dpx.film.held_count=ReadBlobLong(image);
  (void) ReadBlob(image,4,single);
  dpx.film.frame_rate=((float *) single)[0];
  (void) ReadBlob(image,4,single);
  dpx.film.shutter_angle=((float *) single)[0];
  (void) ReadBlob(image,32,(unsigned char *) dpx.film.frame_id);
  if (*dpx.film.frame_id != '\0')
    (void) FormatImageAttribute(image,"dpx:film.frame_id","%.32s",
      dpx.film.frame_id);
  (void) ReadBlob(image,100,(unsigned char *) dpx.film.slate);
  if (*dpx.film.slate != '\0')
    (void) FormatImageAttribute(image,"dpx:film.slate","%.100s",
      dpx.film.slate);
  (void) ReadBlob(image,56,(unsigned char *) dpx.film.reserve);
  /*
    Read DPX televison header.
  */
  dpx.television.time_code=ReadBlobLong(image);
  dpx.television.user_bits=ReadBlobLong(image);
  dpx.television.interlace=ReadBlobByte(image);
  dpx.television.field_number=ReadBlobByte(image);
  dpx.television.video_signal=ReadBlobByte(image);
  dpx.television.padding=ReadBlobByte(image);
  (void) ReadBlob(image,4,single);
  dpx.television.horizontal_sample_rate=((float *) single)[0];
  (void) ReadBlob(image,4,single);
  dpx.television.vertical_sample_rate=((float *) single)[0];
  (void) ReadBlob(image,4,single);
  dpx.television.frame_rate=((float *) single)[0];
  (void) ReadBlob(image,4,single);
  dpx.television.time_offset=((float *) single)[0];
  (void) ReadBlob(image,4,single);
  dpx.television.gamma=((float *) single)[0];
  (void) ReadBlob(image,4,single);
  dpx.television.black_level=((float *) single)[0];
  (void) ReadBlob(image,4,single);
  dpx.television.black_gain=((float *) single)[0];
  (void) ReadBlob(image,4,single);
  dpx.television.break_point=((float *) single)[0];
  (void) ReadBlob(image,4,single);
  dpx.television.white_level=((float *) single)[0];
  (void) ReadBlob(image,4,single);
  dpx.television.integration_times=((float *) single)[0];
  (void) ReadBlob(image,76,(unsigned char *) dpx.television.reserve);
  /*
    Read DPX user header.
  */
  (void) ReadBlob(image,32,(unsigned char *) dpx.user.id);
  if (*dpx.user.id != '\0')
    (void) FormatImageAttribute(image,"dpx:user.id","%.32s",dpx.user.id);
  /*
    Read DPX image header.
  */
  for (i=0; i < (long) (dpx.file.image_offset-2080); i++)
    (void) ReadBlobByte(image);
  if (image_info->ping != MagickFalse)
    {
      CloseBlob(image);
      return(GetFirstImageInList(image));
    }
  /*
    Convert DPX raster image to pixel packets.
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
%   R e g i s t e r D P X I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterDPXImage() adds attributes for the DPX image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterDPXImage method is:
%
%      RegisterDPXImage(void)
%
*/
ModuleExport void RegisterDPXImage(void)
{
  MagickInfo
    *entry;

  static const char
    *DPXNote =
    {
      "File Format for Digital Moving-Picture Exchange (DPX) Version 2.0.\n"
      "See SMPTE 268M-2003 specification at http://www.smtpe.org\n"
    };

  entry=SetMagickInfo("DPX");
  entry->decoder=(DecoderHandler *) ReadDPXImage;
  entry->encoder=(EncoderHandler *) WriteDPXImage;
  entry->magick=(MagickHandler *) IsDPX;
  entry->description=AcquireString("SMPTE 268M-2003 (DPX)");
  entry->note=AcquireString(DPXNote);
  entry->module=AcquireString("DPX");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r D P X I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterDPXImage() removes format registrations made by the
%  DPX module from the list of supported formats.
%
%  The format of the UnregisterDPXImage method is:
%
%      UnregisterDPXImage(void)
%
*/
ModuleExport void UnregisterDPXImage(void)
{
  (void) UnregisterMagickInfo("DPX");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e D P X I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteDPXImage() writes an image in DPX encoded image format.
%
%  The format of the WriteDPXImage method is:
%
%      MagickBooleanType WriteDPXImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteDPXImage(const ImageInfo *image_info,Image *image)
{
  const ImageAttribute
    *attribute;

  DPXInfo
    dpx;

  long
    y;

  MagickBooleanType
    status;

  register const PixelPacket
    *p;

  register long
    i;

  size_t
    length;

  ssize_t
    count;

  struct tm
    *time_meridian;

  time_t
    seconds;

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
  (void) ResetMagickMemory(&dpx.file,0,sizeof(dpx.file));
  (void) SetImageColorspace(image,LogColorspace);
  /*
    Write file header.
  */
  dpx.file.magic=0x53445058UL;
  (void) WriteBlobLong(image,dpx.file.magic);
  dpx.file.image_offset=0x2000UL;
  (void) WriteBlobLong(image,dpx.file.image_offset);
  (void) CopyMagickString(dpx.file.version,"V2.0",MaxTextExtent);
  (void) WriteBlob(image,8,(unsigned char *) &dpx.file.version);
  dpx.file.file_size=4UL*image->columns*image->rows+0x2000UL;
  (void) WriteBlobLong(image,dpx.file.file_size);
  dpx.file.ditto_key=1UL;  /* new frame */
  (void) WriteBlobLong(image,dpx.file.ditto_key);
  dpx.file.generic_size=0x00000680UL;
  (void) WriteBlobLong(image,dpx.file.generic_size);
  dpx.file.industry_size=0x00000180UL;
  (void) WriteBlobLong(image,dpx.file.industry_size);
  dpx.file.user_size=0x00001800UL;
  (void) WriteBlobLong(image,dpx.file.user_size);
  (void) CopyMagickString(dpx.file.filename,"V2.0",sizeof(dpx.file.filename));
  (void) WriteBlob(image,100,(unsigned char *) dpx.file.filename);
  seconds=time((time_t *) NULL);
  time_meridian=localtime(&seconds);
  (void) strftime(dpx.file.timestamp,24,"%Y:%m:%d:%H:%M:%S%Z",time_meridian);
  (void) WriteBlob(image,24,(unsigned char *) dpx.file.timestamp);
  (void) CopyMagickString(dpx.file.creator,
    GetMagickVersion((unsigned long *) NULL),100);
  (void) WriteBlob(image,100,(unsigned char *) dpx.file.creator);
  attribute=GetImageAttribute(image,"dpx:file.project");
  if (attribute != (const ImageAttribute *) NULL)
    (void) CopyMagickString(dpx.file.project,attribute->value,200);
  (void) WriteBlob(image,200,(unsigned char *) dpx.file.project);
  attribute=GetImageAttribute(image,"dpx:file.copyright");
  if (attribute != (const ImageAttribute *) NULL)
    (void) CopyMagickString(dpx.file.copyright,attribute->value,200);
  (void) WriteBlob(image,200,(unsigned char *) dpx.file.copyright);
  dpx.file.encrypt_key=(~0UL);
  (void) WriteBlobLong(image,dpx.file.encrypt_key);
  (void) WriteBlob(image,104,(unsigned char *) dpx.file.reserve);
  /*
    Write image header.
  */
  dpx.image.orientation=0x00;  /* left-to-right; top-to-bottom */
  (void) WriteBlobShort(image,dpx.image.orientation);
  dpx.image.number_elements=1UL;
  (void) WriteBlobShort(image,dpx.image.number_elements);
  (void) WriteBlobLong(image,image->columns);
  (void) WriteBlobLong(image,image->rows);
  for (i=0; i < 15; i++)
  {
    dpx.image.image_element[i].data_sign=0UL;
    (void) WriteBlobLong(image,dpx.image.image_element[i].data_sign);
    dpx.image.image_element[i].low_data=0UL;
    (void) WriteBlobLong(image,dpx.image.image_element[i].low_data);
    dpx.image.image_element[i].low_quantity=0.0f;
    (void) WriteBlob(image,4,(unsigned char *) &
      dpx.image.image_element[i].low_quantity);
    dpx.image.image_element[i].high_data=0UL;
    (void) WriteBlobLong(image,dpx.image.image_element[i].high_data);
    dpx.image.image_element[i].high_quantity=0.0f;
    (void) WriteBlob(image,4,(unsigned char *) &
      dpx.image.image_element[i].high_quantity);
    dpx.image.image_element[i].descriptor=0;
    if (i == 0)
      dpx.image.image_element[i].descriptor=RGBColorType;
    (void) WriteBlobByte(image,dpx.image.image_element[i].descriptor);
    dpx.image.image_element[i].transfer=0;
    (void) WriteBlobByte(image,dpx.image.image_element[i].transfer);
    dpx.image.image_element[i].colorimetric=0;
    (void) WriteBlobByte(image,dpx.image.image_element[i].colorimetric);
    dpx.image.image_element[i].bit_size=0;
    if (i == 0)
      dpx.image.image_element[i].bit_size=image->depth;
    (void) WriteBlobByte(image,dpx.image.image_element[i].bit_size);
    dpx.image.image_element[i].packing=0;
    (void) WriteBlobShort(image,dpx.image.image_element[i].packing);
    dpx.image.image_element[i].encoding=0;
    if (i == 0)
      dpx.image.image_element[i].encoding=1;
    (void) WriteBlobShort(image,dpx.image.image_element[i].encoding);
    dpx.image.image_element[i].data_offset=0UL;
    (void) WriteBlobLong(image,dpx.image.image_element[i].data_offset);
    dpx.image.image_element[i].end_of_line_padding=0UL;
    (void) WriteBlobLong(image,dpx.image.image_element[i].end_of_line_padding);
    (void) WriteBlob(image,32,(unsigned char *)
      dpx.image.image_element[i].description);
	}
  (void) WriteBlob(image,52,(unsigned char *) dpx.image.reserve);
  /*
    Write orientation header.
  */
  dpx.orientation.x_offset=0UL;
  (void) WriteBlobLong(image,dpx.orientation.x_offset);
  dpx.orientation.y_offset=0UL;
  (void) WriteBlobLong(image,dpx.orientation.y_offset);
  dpx.orientation.x_center=0.0f;
  (void) WriteBlob(image,4,(unsigned char *) &dpx.orientation.x_center);
  dpx.orientation.y_center=0.0f;
  (void) WriteBlob(image,4,(unsigned char *) &dpx.orientation.y_center);
  dpx.orientation.x_size=0UL;
  (void) WriteBlobLong(image,dpx.orientation.x_size);
  dpx.orientation.y_size=0UL;
  (void) WriteBlobLong(image,dpx.orientation.y_size);
  (void) WriteBlob(image,100,(unsigned char *) dpx.orientation.filename);
  (void) WriteBlob(image,24,(unsigned char *) dpx.orientation.timestamp);
  (void) WriteBlob(image,32,(unsigned char *) dpx.orientation.device);
  (void) WriteBlob(image,32,(unsigned char *) dpx.orientation.serial);
  for (i=0; i < 4; i++)
  {
    dpx.orientation.border[i]=0UL;
    (void) WriteBlobShort(image,dpx.orientation.border[i]);
  }
  for (i=0; i < 2; i++)
  {
    dpx.orientation.aspect_ratio[i]=0UL;
    (void) WriteBlobLong(image,dpx.orientation.aspect_ratio[i]);
  }
  (void) WriteBlob(image,28,(unsigned char *) dpx.orientation.reserve);
  for (i=0; i < (0x2000-2108); i++)
    (void) WriteBlobByte(image,0x00);
  /*
    Convert pixel packets to DPX raster image.
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
