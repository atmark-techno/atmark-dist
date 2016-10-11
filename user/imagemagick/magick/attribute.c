/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%        AAA   TTTTT  TTTTT  RRRR   IIIII  BBBB   U   U  TTTTT  EEEEE         %
%       A   A    T      T    R   R    I    B   B  U   U    T    E             %
%       AAAAA    T      T    RRRR     I    BBBB   U   U    T    EEE           %
%       A   A    T      T    R R      I    B   B  U   U    T    E             %
%       A   A    T      T    R  R   IIIII  BBBB    UUU     T    EEEEE         %
%                                                                             %
%                                                                             %
%                       Methods to Get/Set Attributes                         %
%                                                                             %
%                             Software Design                                 %
%                               John Cristy                                   %
%                              February 2000                                  %
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
%  The Attributes methods gets, sets, or destroys attributes associated
%  with a particular image (e.g. comments, copyright, author, etc).
%
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/attribute.h"
#include "magick/blob.h"
#include "magick/draw.h"
#include "magick/exception-private.h"
#include "magick/list.h"
#include "magick/memory_.h"
#include "magick/profile.h"
#include "magick/splay-tree.h"
#include "magick/string_.h"
#include "magick/utility.h"

/*
  Forward declarations.
*/
static char
  *TracePSClippath(unsigned char *,size_t,const unsigned long,
    const unsigned long),
  *TraceSVGClippath(unsigned char *,size_t,const unsigned long,
    const unsigned long);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e I m a g e A t t r i b u t e s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneImageAttributes() clones one or more image attributes.
%
%  The format of the CloneImageAttributes method is:
%
%      MagickBooleanType CloneImageAttributes(Image *image,
%        const Image *clone_image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o clone_image: The clone image.
%
*/
MagickExport MagickBooleanType CloneImageAttributes(Image *image,
  const Image *clone_image)
{
  const ImageAttribute
    *attribute;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(clone_image != (const Image *) NULL);
  assert(clone_image->signature == MagickSignature);
  if (clone_image->attributes == (void *) NULL)
    return(MagickTrue);
  ResetImageAttributeIterator(clone_image);
  attribute=GetNextImageAttribute(clone_image);
  while (attribute != (const ImageAttribute *) NULL)
  {
    (void) SetImageAttribute(image,attribute->key,attribute->value);
    attribute=GetNextImageAttribute(clone_image);
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e l e t e I m a g e O p t i o n                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DeleteImageAttribute() deletes an attribute from the image.
%
%  The format of the DeleteImageAttribute method is:
%
%      MagickBooleanType DeleteImageAttribute(Image *image,const char *key)
%
%  A description of each parameter follows:
%
%    o image: The image info.
%
%    o key: The image key.
%
*/
MagickExport MagickBooleanType DeleteImageAttribute(Image *image,
  const char *key)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->attributes == (void *) NULL)
    return(MagickFalse);
  return(RemoveNodeFromSplayTree((SplayTreeInfo *) image->attributes,key));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y I m a g e A t t r i b u t e s                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyImageAttributes() deallocates memory associated with the image
%  attribute list.
%
%  The format of the DestroyImageAttributes method is:
%
%      DestroyImageAttributes(Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/
MagickExport void DestroyImageAttributes(Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->attributes != (void *) NULL)
    image->attributes=(void *)
      DestroySplayTree((SplayTreeInfo *) image->attributes);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  F o r m a t I m a g e A t t r i b u t e                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FormatImageAttribute() permits formatted key/value pairs to be saved as an
%  image attribute.
%
%  The format of the FormatImageAttribute method is:
%
%      MagickBooleanType FormatImageAttribute(Image *image,const char *key,
%        const char *format,...)
%
%  A description of each parameter follows.
%
%   o  image:  The image.
%
%   o  key:  The attribute key.
%
%   o  format:  A string describing the format to use to write the remaining
%      arguments.
%
*/

MagickExport MagickBooleanType FormatImageAttributeList(Image *image,
  const char *key,const char *format,va_list operands)
{
  char
    value[MaxTextExtent];

  int
    n;

#if defined(HAVE_VSNPRINTF)
  n=vsnprintf(value,MaxTextExtent,format,operands);
#else
  n=vsprintf(value,format,operands);
#endif
  if (n < 0)
    value[MaxTextExtent-1]='\0';
  return(SetImageAttribute(image,key,value));
}

MagickExport MagickBooleanType FormatImageAttribute(Image *image,
  const char *key,const char *format,...)
{
  MagickBooleanType
    status;

  va_list
    operands;

  va_start(operands,format);
  status=FormatImageAttributeList(image,key,format,operands);
  va_end(operands);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e A t t r i b u t e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageAttribute() searches the list of image attributes and returns
%  a pointer to the attribute if it exists otherwise NULL.
%
%  The format of the GetImageAttribute method is:
%
%      const ImageAttribute *GetImageAttribute(const Image *image,
%        const char *key)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o key:  These character strings are the name of an image attribute to
%      return.
%
%
*/

static MagickBooleanType GenerateIPTCAttribute(const Image *image,
  const char *key)
{
  char
    *attribute;

  const StringInfo
    *profile;

  int
    count,
    dataset,
    record;

  register long
    i;

  size_t
    length;

  profile=GetImageProfile(image,"8bim");
  if (profile == (StringInfo *) NULL)
    return(MagickFalse);
  count=sscanf(key,"IPTC:%d:%d",&dataset,&record);
  if (count != 2)
    return(MagickFalse);
  for (i=0; i < (long) profile->length; i++)
  {
    if ((int) profile->datum[i] != 0x1c)
      continue;
    if ((int) profile->datum[i+1] != dataset)
      continue;
    if ((int) profile->datum[i+2] != record)
      continue;
    length=(size_t) (profile->datum[i+3] << 8);
    length|=profile->datum[i+4];
    attribute=(char *) AcquireMagickMemory(length+MaxTextExtent);
    if (attribute == (char *) NULL)
      continue;
    (void) CopyMagickString(attribute,(char *) profile->datum+i+5,length+1);
    (void) SetImageAttribute((Image *) image,key,(const char *) attribute);
    attribute=(char *) RelinquishMagickMemory(attribute);
    break;
  }
  return((MagickBooleanType) (i < (long) profile->length));
}

static unsigned char ReadByte(unsigned char **p,size_t *length)
{
  unsigned char
    c;

  if (*length < 1)
    return((unsigned char) 0xff);
  c=(*(*p)++);
  (*length)--;
  return(c);
}

static long ReadMSBLong(unsigned char **p,size_t *length)
{
  int
    c;

  long
    value;

  register long
    i;

  unsigned char
    buffer[4];

  if (*length < 4)
    return(-1);
  for (i=0; i < 4; i++)
  {
    c=(int) (*(*p)++);
    (*length)--;
    buffer[i]=(unsigned char) c;
  }
  value=(long) (buffer[0] << 24);
  value|=buffer[1] << 16;
  value|=buffer[2] << 8;
  value|=buffer[3];
  return(value);
}

static long ReadMSBShort(unsigned char **p,size_t *length)
{
  int
    c;

  long
    value;

  register long
    i;

  unsigned char
    buffer[2];

  if (*length < 2)
    return(-1);
  for (i=0; i < 2; i++)
  {
    c=(int) (*(*p)++);
    (*length)--;
    buffer[i]=(unsigned char) c;
  }
  value=(long) (buffer[0] << 8);
  value|=buffer[1];
  return(value);
}

static MagickBooleanType Generate8BIMAttribute(const Image *image,
  const char *key)
{
  char
    *attribute,
    format[MaxTextExtent],
    name[MaxTextExtent],
    *resource;

  const StringInfo
    *profile;

  long
    id,
    start,
    stop,
    sub_number;

  MagickBooleanType
    status;

  register long
    i;

  ssize_t
    count;

  size_t
    length;

  unsigned char
    *info;

  /*
    There's no newlines in path names, so it's safe as terminator.
  */
  profile=GetImageProfile(image,"iptc");
  if (profile == (StringInfo *) NULL)
    return(MagickFalse);
  count=(ssize_t) sscanf(key,"8BIM:%ld,%ld:%[^\n]\n%[^\n]",&start,&stop,name,
    format);
  if ((count != 2) && (count != 3) && (count != 4))
    return(MagickFalse);
  if (count < 4)
    (void) strcpy(format,"SVG");
  if (count < 3)
    *name='\0';
  sub_number=1;
  if (*name == '#')
    sub_number=atol(&name[1]);
  sub_number=Max(sub_number,1);
  resource=(char *) NULL;
  status=MagickFalse;
  length=profile->length;
  info=(unsigned char *) profile->datum;
  while ((length > 0) && (status == MagickFalse))
  {
    if (ReadByte(&info,&length) != (unsigned char) '8')
      continue;
    if (ReadByte(&info,&length) != (unsigned char) 'B')
      continue;
    if (ReadByte(&info,&length) != (unsigned char) 'I')
      continue;
    if (ReadByte(&info,&length) != (unsigned char) 'M')
      continue;
    id=ReadMSBShort(&info,&length);
    if (id < start)
      continue;
    if (id > stop)
      continue;
    if (resource != (char *) NULL)
      resource=(char *) RelinquishMagickMemory(resource);
    count=(ssize_t) ReadByte(&info,&length);
    if ((count != 0) && ((size_t) count <= length))
      {
        resource=(char *) AcquireMagickMemory((size_t) count+MaxTextExtent);
        if (resource != (char *) NULL)
          {
            for (i=0; i < (long) count; i++)
              resource[i]=(char) ReadByte(&info,&length);
            resource[count]='\0';
          }
      }
    if ((count & 0x01) == 0)
      (void) ReadByte(&info,&length);
    count=(ssize_t) ReadMSBLong(&info,&length);
    if ((*name != '\0') && (*name != '#'))
      if ((resource == (char *) NULL) || (LocaleCompare(name,resource) != 0))
        {
          /*
            No name match, scroll forward and try next.
          */
          info+=count;
          length-=count;
          continue;
        }
    if ((*name == '#') && (sub_number != 1))
      {
        /*
          No numbered match, scroll forward and try next.
        */
        sub_number--;
        info+=count;
        length-=count;
        continue;
      }
    /*
      We have the resource of interest.
    */
    attribute=(char *) AcquireMagickMemory((size_t) count+MaxTextExtent);
    if (attribute != (char *) NULL)
      {
        (void) CopyMagickMemory(attribute,(char *) info,(size_t) count);
        attribute[count]='\0';
        info+=count;
        length-=count;
        if ((id <= 1999) || (id >= 2999))
          (void) SetImageAttribute((Image *) image,key,(const char *) attribute);
        else
          {
            char
              *path;

            if (LocaleCompare("svg",format) == 0)
              path=TraceSVGClippath((unsigned char *) attribute,(size_t) count,
                image->columns,image->rows);
            else
              path=TracePSClippath((unsigned char *) attribute,(size_t) count,
                image->columns,image->rows);
            (void) SetImageAttribute((Image *) image,key,(const char *) path);
            path=(char *) RelinquishMagickMemory(path);
          }
        attribute=(char *) RelinquishMagickMemory(attribute);
        status=MagickTrue;
      }
  }
  if (resource != (char *) NULL)
    resource=(char *) RelinquishMagickMemory(resource);
  return(status);
}

#define DE_STACK_SIZE  16
#define EXIF_DELIMITER  "\n"
#define EXIF_NUM_FORMATS  12
#define EXIF_FMT_BYTE  1
#define EXIF_FMT_STRING  2
#define EXIF_FMT_USHORT  3
#define EXIF_FMT_ULONG  4
#define EXIF_FMT_URATIONAL  5
#define EXIF_FMT_SBYTE  6
#define EXIF_FMT_UNDEFINED  7
#define EXIF_FMT_SSHORT  8
#define EXIF_FMT_SLONG  9
#define EXIF_FMT_SRATIONAL  10
#define EXIF_FMT_SINGLE  11
#define EXIF_FMT_DOUBLE  12
#define TAG_EXIF_OFFSET  0x8769
#define TAG_INTEROP_OFFSET  0xa005

typedef struct _TagInfo
{
  unsigned short
    tag;

  const char
    *description;
} TagInfo;

static TagInfo
  tag_table[] =
  {
    {  0x001, "InteroperabilityIndex" },
    {  0x002, "InteroperabilityVersion" },
    {  0x100, "ImageWidth" },
    {  0x101, "ImageLength" },
    {  0x102, "BitsPerSample" },
    {  0x103, "Compression" },
    {  0x106, "PhotometricInterpretation" },
    {  0x10a, "FillOrder" },
    {  0x10d, "DocumentName" },
    {  0x10e, "ImageDescription" },
    {  0x10f, "Make" },
    {  0x110, "Model" },
    {  0x111, "StripOffsets" },
    {  0x112, "Orientation" },
    {  0x115, "SamplesPerPixel" },
    {  0x116, "RowsPerStrip" },
    {  0x117, "StripByteCounts" },
    {  0x11a, "XResolution" },
    {  0x11b, "YResolution" },
    {  0x11c, "PlanarConfiguration" },
    {  0x118, "MinSampleValue" },
    {  0x119, "MaxSampleValue" },
    {  0x11A, "XResolution" },
    {  0x11B, "YResolution" },
    {  0x11C, "PlanarConfiguration" },
    {  0x11D, "PageName" },
    {  0x11E, "XPosition" },
    {  0x11F, "YPosition" },
    {  0x120, "FreeOffsets" },
    {  0x121, "FreeByteCounts" },
    {  0x122, "GrayResponseUnit" },
    {  0x123, "GrayResponseCurve" },
    {  0x124, "T4Options" },
    {  0x125, "T6Options" },
    {  0x128, "ResolutionUnit" },
    {  0x12d, "TransferFunction" },
    {  0x131, "Software" },
    {  0x132, "DateTime" },
    {  0x13b, "Artist" },
    {  0x13e, "WhitePoint" },
    {  0x13f, "PrimaryChromaticities" },
    {  0x140, "ColorMap" },
    {  0x141, "HalfToneHints" },
    {  0x142, "TileWidth" },
    {  0x143, "TileLength" },
    {  0x144, "TileOffsets" },
    {  0x145, "TileByteCounts" },
    {  0x14a, "SubIFD" },
    {  0x14c, "InkSet" },
    {  0x14d, "InkNames" },
    {  0x14e, "NumberOfInks" },
    {  0x150, "DotRange" },
    {  0x151, "TargetPrinter" },
    {  0x152, "ExtraSample" },
    {  0x153, "SampleFormat" },
    {  0x154, "SMinSampleValue" },
    {  0x155, "SMaxSampleValue" },
    {  0x156, "TransferRange" },
    {  0x157, "ClipPath" },
    {  0x158, "XClipPathUnits" },
    {  0x159, "YClipPathUnits" },
    {  0x15a, "Indexed" },
    {  0x15b, "JPEGTables" },
    {  0x15f, "OPIProxy" },
    {  0x200, "JPEGProc" },
    {  0x201, "JPEGInterchangeFormat" },
    {  0x202, "JPEGInterchangeFormatLength" },
    {  0x203, "JPEGRestartInterval" },
    {  0x205, "JPEGLosslessPredictors" },
    {  0x206, "JPEGPointTransforms" },
    {  0x207, "JPEGQTables" },
    {  0x208, "JPEGDCTables" },
    {  0x209, "JPEGACTables" },
    {  0x211, "YCbCrCoefficients" },
    {  0x212, "YCbCrSubSampling" },
    {  0x213, "YCbCrPositioning" },
    {  0x214, "ReferenceBlackWhite" },
    {  0x2bc, "ExtensibleMetadataPlatform" },
    {  0x301, "Gamma" },
    {  0x302, "ICCProfileDescriptor" },
    {  0x303, "SRGBRenderingIntent" },
    {  0x320, "ImageTitle" },
    {  0x5001, "ResolutionXUnit" },
    {  0x5002, "ResolutionYUnit" },
    {  0x5003, "ResolutionXLengthUnit" },
    {  0x5004, "ResolutionYLengthUnit" },
    {  0x5005, "PrintFlags" },
    {  0x5006, "PrintFlagsVersion" },
    {  0x5007, "PrintFlagsCrop" },
    {  0x5008, "PrintFlagsBleedWidth" },
    {  0x5009, "PrintFlagsBleedWidthScale" },
    {  0x500A, "HalftoneLPI" },
    {  0x500B, "HalftoneLPIUnit" },
    {  0x500C, "HalftoneDegree" },
    {  0x500D, "HalftoneShape" },
    {  0x500E, "HalftoneMisc" },
    {  0x500F, "HalftoneScreen" },
    {  0x5010, "JPEGQuality" },
    {  0x5011, "GridSize" },
    {  0x5012, "ThumbnailFormat" },
    {  0x5013, "ThumbnailWidth" },
    {  0x5014, "ThumbnailHeight" },
    {  0x5015, "ThumbnailColorDepth" },
    {  0x5016, "ThumbnailPlanes" },
    {  0x5017, "ThumbnailRawBytes" },
    {  0x5018, "ThumbnailSize" },
    {  0x5019, "ThumbnailCompressedSize" },
    {  0x501a, "ColorTransferFunction" },
    {  0x501b, "ThumbnailData" },
    {  0x5020, "ThumbnailImageWidth" },
    {  0x5021, "ThumbnailImageHeight" },
    {  0x5022, "ThumbnailBitsPerSample" },
    {  0x5023, "ThumbnailCompression" },
    {  0x5024, "ThumbnailPhotometricInterp" },
    {  0x5025, "ThumbnailImageDescription" },
    {  0x5026, "ThumbnailEquipMake" },
    {  0x5027, "ThumbnailEquipModel" },
    {  0x5028, "ThumbnailStripOffsets" },
    {  0x5029, "ThumbnailOrientation" },
    {  0x502a, "ThumbnailSamplesPerPixel" },
    {  0x502b, "ThumbnailRowsPerStrip" },
    {  0x502c, "ThumbnailStripBytesCount" },
    {  0x502d, "ThumbnailResolutionX" },
    {  0x502e, "ThumbnailResolutionY" },
    {  0x502f, "ThumbnailPlanarConfig" },
    {  0x5030, "ThumbnailResolutionUnit" },
    {  0x5031, "ThumbnailTransferFunction" },
    {  0x5032, "ThumbnailSoftwareUsed" },
    {  0x5033, "ThumbnailDateTime" },
    {  0x5034, "ThumbnailArtist" },
    {  0x5035, "ThumbnailWhitePoint" },
    {  0x5036, "ThumbnailPrimaryChromaticities" },
    {  0x5037, "ThumbnailYCbCrCoefficients" },
    {  0x5038, "ThumbnailYCbCrSubsampling" },
    {  0x5039, "ThumbnailYCbCrPositioning" },
    {  0x503A, "ThumbnailRefBlackWhite" },
    {  0x503B, "ThumbnailCopyRight" },
    {  0x5090, "LuminanceTable" },
    {  0x5091, "ChrominanceTable" },
    {  0x5100, "FrameDelay" },
    {  0x5101, "LoopCount" },
    {  0x5110, "PixelUnit" },
    {  0x5111, "PixelPerUnitX" },
    {  0x5112, "PixelPerUnitY" },
    {  0x5113, "PaletteHistogram" },
    {  0x1000, "RelatedImageFileFormat" },
    {  0x1001, "RelatedImageLength" },
    {  0x1002, "RelatedImageWidth" },
    {  0x800d, "ImageID" },
    {  0x80e3, "Matteing" },
    {  0x80e4, "DataType" },
    {  0x80e5, "ImageDepth" },
    {  0x80e6, "TileDepth" },
    {  0x828d, "CFARepeatPatternDim" },
    {  0x828e, "CFAPattern" },
    {  0x828f, "BatteryLevel" },
    {  0x828d, "CFARepeatPatternDim" },
    {  0x828e, "CFAPattern" },
    {  0x828f, "BatteryLevel" },
    {  0x8298, "Copyright" },
    {  0x829a, "ExposureTime" },
    {  0x829d, "FNumber" },
    {  0x83bb, "IPTC/NAA" },
    {  0x84e3, "IT8RasterPadding" },
    {  0x84e5, "IT8ColorTable" },
    {  0x8649, "ImageResourceInformation" },
    {  0x8769, "ExifOffset" },
    {  0x8773, "InterColorProfile" },
    {  0x8822, "ExposureProgram" },
    {  0x8824, "SpectralSensitivity" },
    {  0x8825, "GPSInfo" },
    {  0x8827, "ISOSpeedRatings" },
    {  0x8828, "OECF" },
    {  0x8829, "Interlace" },
    {  0x882a, "TimeZoneOffset" },
    {  0x882b, "SelfTimerMode" },
    {  0x9000, "ExifVersion" },
    {  0x9003, "DateTimeOriginal" },
    {  0x9004, "DateTimeDigitized" },
    {  0x9101, "ComponentsConfiguration" },
    {  0x9102, "CompressedBitsPerPixel" },
    {  0x9201, "ShutterSpeedValue" },
    {  0x9202, "ApertureValue" },
    {  0x9203, "BrightnessValue" },
    {  0x9204, "ExposureBiasValue" },
    {  0x9205, "MaxApertureValue" },
    {  0x9206, "SubjectDistance" },
    {  0x9207, "MeteringMode" },
    {  0x9208, "LightSource" },
    {  0x9209, "Flash" },
    {  0x920a, "FocalLength" },
    {  0x920b, "FlashEnergy" },
    {  0x920c, "SpatialFrequencyResponse" },
    {  0x920d, "Noise" },
    {  0x9211, "ImageNumber" },
    {  0x9212, "SecurityClassification" },
    {  0x9213, "ImageHistory" },
    {  0x9214, "SubjectArea" },
    {  0x9215, "ExposureIndex" },
    {  0x9216, "TIFF/EPStandardID" },
    {  0x927c, "MakerNote" },
    {  0x9C9b, "WinXP-Title" },
    {  0x9C9c, "WinXP-Comments" },
    {  0x9C9d, "WinXP-Author" },
    {  0x9C9e, "WinXP-Keywords" },
    {  0x9C9f, "WinXP-Subject" },
    {  0x9286, "UserComment" },
    {  0x9290, "SubSecTime" },
    {  0x9291, "SubSecTimeOriginal" },
    {  0x9292, "SubSecTimeDigitized" },
    {  0xa000, "FlashPixVersion" },
    {  0xa001, "ColorSpace" },
    {  0xa002, "ExifImageWidth" },
    {  0xa003, "ExifImageLength" },
    {  0xa004, "RelatedSoundFile" },
    {  0xa005, "InteroperabilityOffset" },
    {  0xa20b, "FlashEnergy" },
    {  0xa20c, "SpatialFrequencyResponse" },
    {  0xa20d, "Noise" },
    {  0xa20e, "FocalPlaneXResolution" },
    {  0xa20f, "FocalPlaneYResolution" },
    {  0xa210, "FocalPlaneResolutionUnit" },
    {  0xa214, "SubjectLocation" },
    {  0xa215, "ExposureIndex" },
    {  0xa216, "TIFF/EPStandardID" },
    {  0xa217, "SensingMethod" },
    {  0xa300, "FileSource" },
    {  0xa301, "SceneType" },
    {  0xa302, "CFAPattern" },
    {  0xa401, "CustomRendered" },
    {  0xa402, "ExposureMode" },
    {  0xa403, "WhiteBalance" },
    {  0xa404, "DigitalZoomRatio" },
    {  0xa405, "FocalLengthIn35mmFilm" },
    {  0xa406, "SceneCaptureType" },
    {  0xa407, "GainControl" },
    {  0xa408, "Contrast" },
    {  0xa409, "Saturation" },
    {  0xa40a, "Sharpness" },
    {  0xa40b, "DeviceSettingDescription" },
    {  0xa40c, "SubjectDistanceRange" },
    {  0xa420, "ImageUniqueID" },
    {  0xc4a5, "PrintImageMatching" },
    {  0x0000, NULL}
  };

static int
  format_bytes[] = {0, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8};

static short ReadInt16(unsigned int msb_order,void *buffer)
{
  short
    value;

  if (msb_order != MagickFalse)
    {
      value=(short) ((((unsigned char *) buffer)[0] << 8) |
        ((unsigned char *) buffer)[1]);
      return(value);
    }
  value=(short) ((((unsigned char *) buffer)[1] << 8) |
    ((unsigned char *) buffer)[0]);
  return(value);
}

static long ReadInt32(unsigned int msb_order,void *buffer)
{
  long
    value;

  if (msb_order != MagickFalse)
    {
      value=(long) ((((unsigned char *) buffer)[0] << 24) |
        (((unsigned char *) buffer)[1] << 16) |
        (((unsigned char *) buffer)[2] << 8) | (((unsigned char *) buffer)[3]));
      return(value);
    }
  value=(long) ((((unsigned char *) buffer)[3] << 24) |
    (((unsigned char *) buffer)[2] << 16) |
    (((unsigned char *) buffer)[1] << 8 ) |
    (((unsigned char *) buffer)[0]));
  return(value);
}

static unsigned short ReadUint16(unsigned int msb_order,void *buffer)
{
  unsigned short
    value;

  if (msb_order != MagickFalse)
    {
      value=(unsigned short) ((((unsigned char *) buffer)[0] << 8) |
        ((unsigned char *) buffer)[1]);
      return(value);
    }
  value=(unsigned short) ((((unsigned char *) buffer)[1] << 8) |
    ((unsigned char *) buffer)[0]);
  return(value);
}

static unsigned long ReadUint32(unsigned int msb_order,void *buffer)
{
  return((unsigned long) ReadInt32(msb_order,buffer) & 0xffffffff);
}

static int GenerateEXIFAttribute(const Image *image,const char *specification)
{
  char
    *final,
    *value;

  const char
    *key;

  const StringInfo
    *profile;

  int
    all,
    id,
    level;

  register long
    i;

  size_t
    length;

  unsigned long
    offset;

  unsigned char
    *tiffp,
    *ifdstack[DE_STACK_SIZE],
    *ifdp,
    *info;

  unsigned int
    de,
    destack[DE_STACK_SIZE],
    msb_order,
    nde;

  unsigned long
    tag;

  /*
    If EXIF data exists, then try to parse the request for a tag.
  */
  profile=GetImageProfile(image,"exif");
  if (profile == (StringInfo *) NULL)
    return(MagickFalse);
  value=(char *) NULL;
  key=(&specification[5]);
  if ((key == (const char *) NULL) || (*key == '\0'))
    return(MagickFalse);
  while (isspace((int) ((unsigned char) *key)) != 0)
    key++;
  all=0;
  tag=(~0UL);
  final=AcquireString("");
  switch (*key)
  {
    /*
      Caller has asked for all the tags in the EXIF data.
    */
    case '*':
    {
      tag=0;
      all=1; /* return the data in description=value format */
      break;
    }
    case '!':
    {
      tag=0;
      all=2; /* return the data in tageid=value format */
      break;
    }
    /*
      Check for a hex based tag specification first.
    */
    case '#':
    {
      char
        c;

      unsigned long
        n;

      tag=0;
      key++;
      n=(unsigned long) strlen(key);
      if (n != 4)
        return(MagickFalse);
      else
        {
          /*
            Parse tag specification as a hex number.
          */
          n/=4;
          do
          {
            for (i=(long) n-1; i >= 0; i--)
            {
              c=(*key++);
              tag<<=4;
              if ((c >= '0') && (c <= '9'))
                tag|=(int) (c-'0');
              else
                if ((c >= 'A') && (c <= 'F'))
                  tag|=(int) (c-('A'-(char) 10));
                else
                  if ((c >= 'a') && (c <= 'f'))
                    tag|=(int) (c-('a'-(char) 10));
                  else
                    return(MagickFalse);
            }
          } while (*key != '\0');
        }
      break;
    }
    default:
    {
      /*
        Try to match the text with a tag name instead.
      */
      for (i=0; ; i++)
      {
        if (tag_table[i].tag == 0)
          break;
        if (LocaleCompare(tag_table[i].description,key) == 0)
          {
            tag=(unsigned long) tag_table[i].tag;
            break;
          }
      }
      break;
    }
  }
  if (tag == (~0UL))
    return(MagickFalse);
  length=profile->length;
  info=(unsigned char *) profile->datum;
  while (length != 0)
  {
    if ((int) ReadByte(&info,&length) != 0x45)
      continue;
    if ((int) ReadByte(&info,&length) != 0x78)
      continue;
    if ((int) ReadByte(&info,&length) != 0x69)
      continue;
    if ((int) ReadByte(&info,&length) != 0x66)
      continue;
    if ((int) ReadByte(&info,&length) != 0x00)
      continue;
    if ((int) ReadByte(&info,&length) != 0x00)
      continue;
    break;
  }
  if (length < 16)
    return(MagickFalse);
  tiffp=info;
  id=(int) ReadUint16(0,tiffp);
  msb_order=0;
  if (id == 0x4949) /* LSB */
    msb_order=0;
  else
    if (id == 0x4D4D) /* MSB */
      msb_order=1;
    else
      return(MagickFalse);
  if (ReadUint16(msb_order,tiffp+2) != 0x002a)
    return(MagickFalse);
  /*
    This is the offset to the first IFD.
  */
  offset=ReadUint32(msb_order,tiffp+4);
  if ((size_t) offset >= length)
    return(MagickFalse);
  /*
    Set the pointer to the first IFD and follow it were it leads.
  */
  ifdp=tiffp+offset;
  level=0;
  de=0;
  do
  {
    /*
      If there is anything on the stack then pop it off.
    */
    if (level > 0)
      {
        level--;
        ifdp=ifdstack[level];
        de=destack[level];
      }
    /*
      Determine how many entries there are in the current IFD.
    */
    nde=ReadUint16(msb_order,ifdp);
    for ( ; de < nde; de++)
    {
      long
        n,
        t,
        f,
        c;

      char
        *pde,
        *pval;

      pde=(char *) (ifdp+2+(12*de));
      t=(long) ReadUint16(msb_order,pde); /* get tag value */
      f=(long) ReadUint16(msb_order,pde+2); /* get the format */
      if ((f-1) >= EXIF_NUM_FORMATS)
        break;
      c=(long) ReadUint32(msb_order,pde+4); /* get number of components */
      n=c*format_bytes[f];
      if (n <= 4)
        pval=pde+8;
      else
        {
          unsigned long
            oval;

          /*
            The directory entry contains an offset.
          */
          oval=ReadUint32(msb_order,pde+8);
          if ((size_t) (oval+n) > length)
            continue;
          pval=(char *) (tiffp+oval);
        }
      if ((all != 0)  || (tag == (unsigned long) t))
        {
          char
            buffer[MaxTextExtent];

          switch (f)
          {
            case EXIF_FMT_SBYTE:
            {
              (void) FormatMagickString(buffer,MaxTextExtent,"%ld",
                (long) (*(char *) pval));
              value=AcquireString(buffer);
              break;
            }
            case EXIF_FMT_SSHORT:
            {
              (void) FormatMagickString(buffer,MaxTextExtent,"%hd",
                ReadUint16(msb_order,pval));
              value=AcquireString(buffer);
              break;
            }
            case EXIF_FMT_USHORT:
            {
              (void) FormatMagickString(buffer,MaxTextExtent,"%hu",
                ReadInt16(msb_order,pval));
              value=AcquireString(buffer);
              break;
            }
            case EXIF_FMT_ULONG:
            {
              (void) FormatMagickString(buffer,MaxTextExtent,"%lu",
                ReadUint32(msb_order,pval));
              value=AcquireString(buffer);
              break;
            }
            case EXIF_FMT_SLONG:
            {
              (void) FormatMagickString(buffer,MaxTextExtent,"%ld",
                ReadInt32(msb_order,pval));
              value=AcquireString(buffer);
              break;
            }
            case EXIF_FMT_URATIONAL:
            {
              (void) FormatMagickString(buffer,MaxTextExtent,"%ld/%ld",
                ReadUint32(msb_order,pval),
                ReadUint32(msb_order,4+(char *) pval));
              value=AcquireString(buffer);
              break;
            }
            case EXIF_FMT_SRATIONAL:
            {
              (void) FormatMagickString(buffer,MaxTextExtent,"%ld/%ld",
                ReadInt32(msb_order,pval),
                ReadInt32(msb_order,4+(char *) pval));
              value=AcquireString(buffer);
              break;
            }
            case EXIF_FMT_SINGLE:
            {
              (void) FormatMagickString(buffer,MaxTextExtent,"%f",
                (double) *(float *) pval);
              value=AcquireString(buffer);
              break;
            }
            case EXIF_FMT_DOUBLE:
            {
              (void) FormatMagickString(buffer,MaxTextExtent,"%f",
                *(double *) pval);
              value=AcquireString(buffer);
              break;
            }
            default:
            case EXIF_FMT_UNDEFINED:
            case EXIF_FMT_BYTE:
            case EXIF_FMT_STRING:
            {
              value=(char *) AcquireMagickMemory((size_t) n+1);
              if (value != (char *) NULL)
                {
                  long
                    a;

                  for (a=0; a < n; a++)
                  {
                    value[a]='.';
                    if (isprint((int) ((unsigned char) pval[a])) != 0)
                      value[a]=pval[a];
                  }
                  value[a]='\0';
                  break;
                }
              break;
            }
          }
          if (value != (char *) NULL)
            {
              const char
                *description;

              register long
                i;

              if (strlen(final) != 0)
                (void) ConcatenateString(&final,EXIF_DELIMITER);
              description=(const char *) NULL;
              switch (all)
              {
                case 1:
                {
                  description="unknown";
                  for (i=0; ; i++)
                  {
                    if (tag_table[i].tag == 0)
                      break;
                    if ((long) tag_table[i].tag == t)
                      {
                        description=tag_table[i].description;
                        break;
                      }
                  }
                  (void) FormatMagickString(buffer,MaxTextExtent,"%s=",
                    description);
                  (void) ConcatenateString(&final,buffer);
                  break;
                }
                case 2:
                {
                  (void) FormatMagickString(buffer,MaxTextExtent,"#%04lx=",t);
                  (void) ConcatenateString(&final,buffer);
                  break;
                }
              }
              (void) ConcatenateString(&final,value);
              value=(char *) RelinquishMagickMemory(value);
            }
        }
        if ((t == TAG_EXIF_OFFSET) || (t == TAG_INTEROP_OFFSET))
          {
            size_t
              offset;

            offset=(size_t) ReadUint32(msb_order,pval);
            if ((offset < length) && (level < (DE_STACK_SIZE-2)))
              {
                /*
                  Push our current directory state onto the stack.
                */
                ifdstack[level]=ifdp;
                de++; /* bump to the next entry */
                destack[level]=de;
                level++;
                /*
                  Push new state onto of stack to cause a jump.
                */
                ifdstack[level]=tiffp+offset;
                destack[level]=0;
                level++;
              }
            break; /* break out of the for loop */
          }
    }
  } while (level > 0);
  if (strlen(final) == 0)
    (void) ConcatenateString(&final,"unknown");
  (void) SetImageAttribute((Image *) image,specification,(const char *) final);
  final=(char *) RelinquishMagickMemory(final);
  return(MagickTrue);
}

MagickExport const ImageAttribute *GetImageAttribute(const Image *image,
  const char *key)
{
  register const ImageAttribute
    *p;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->attributes == (void *) NULL)
   return((const ImageAttribute *) NULL);
  if (key == (const char *) NULL)
    {
      ResetSplayTreeIterator((SplayTreeInfo *) image->attributes);
      p=(const ImageAttribute *)
        GetNextValueInSplayTree((SplayTreeInfo *) image->attributes);
      return(p);
    }
  p=(const ImageAttribute *)
    GetValueFromSplayTree((SplayTreeInfo *) image->attributes,key);
  if (p != (const ImageAttribute *) NULL)
    return(p);
  if (LocaleNCompare("iptc:",key,5) == 0)
    {
      if (GenerateIPTCAttribute(image,key) == MagickTrue)
        return(GetImageAttribute(image,key));
    }
  if (LocaleNCompare("8bim:",key,5) == 0)
    {
      if (Generate8BIMAttribute(image,key) == MagickTrue)
        return(GetImageAttribute(image,key));
    }
  if (LocaleNCompare("exif:",key,5) == 0)
    {
      if (GenerateEXIFAttribute(image,key) == MagickTrue)
        return(GetImageAttribute(image,key));
    }
  return(p);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e C l i p p i n g P a t h A t t r i b u t e                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageClippingPathAttribute() searches the list of image attributes and
%  returns a pointer to a clipping path if it exists otherwise NULL.
%
%  The format of the GetImageClippingPathAttribute method is:
%
%      const ImageAttribute *GetImageClippingPathAttribute(Image *image)
%
%  A description of each parameter follows:
%
%    o attribute:  Method GetImageClippingPathAttribute returns the clipping
%      path if it exists otherwise NULL.
%
%    o image: The image.
%
%
*/
MagickExport const ImageAttribute *GetImageClippingPathAttribute(Image *image)
{
  return(GetImageAttribute(image,"8BIM:1999,2998"));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t I m a g e I n f o A t t r i b u t e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageInfoAttribute() returns a "fake" attribute based on data in the
%  image info or image structures.
%
%  The format of the GetImageInfoAttribute method is:
%
%      const ImageAttribute *GetImageAttribute(const ImageInfo *image_info,
%        const Image *image,const char *key)
%
%  A description of each parameter follows:
%
%    o attribute:  Method GetImageInfoAttribute returns the attribute if it
%      exists otherwise NULL.
%
%    o image_info: The imageInfo.
%
%    o image: The image.
%
%    o key:  These character strings are the name of an image attribute to
%      return.
%
*/
MagickExport const ImageAttribute *GetImageInfoAttribute(
  const ImageInfo *image_info,const Image *image,const char *key)
{
  char
    attribute[MaxTextExtent],
    filename[MaxTextExtent];

  attribute[0]='\0';
  switch (*(key))
  {
    case 'b':
    {
      if (LocaleNCompare("base",key,2) == 0)
        {
          GetPathComponent(image->magick_filename,BasePath,filename);
          (void) CopyMagickString(attribute,filename,MaxTextExtent);
          break;
        }
      break;
    }
    case 'd':
    {
      if (LocaleNCompare("depth",key,2) == 0)
        {
          (void) FormatMagickString(attribute,MaxTextExtent,"%lu",image->depth);
          break;
        }
      if (LocaleNCompare("directory",key,2) == 0)
        {
          GetPathComponent(image->magick_filename,HeadPath,filename);
          (void) CopyMagickString(attribute,filename,MaxTextExtent);
          break;
        }
      break;
    }
    case 'e':
    {
      if (LocaleNCompare("extension",key,2) == 0)
        {
          GetPathComponent(image->magick_filename,ExtensionPath,filename);
          (void) CopyMagickString(attribute,filename,MaxTextExtent);
          break;
        }
      break;
    }
    case 'g':
    {
      if (LocaleNCompare("group",key,2) == 0)
        {
          (void) FormatMagickString(attribute,MaxTextExtent,"0x%lx",
            image_info->group);
          break;
        }
      break;
    }
    case 'h':
    {
      if (LocaleNCompare("height",key,2) == 0)
        {
          (void) FormatMagickString(attribute,MaxTextExtent,"%lu",
            image->magick_rows != 0 ? image->magick_rows : 256UL);
          break;
        }
      break;
    }
    case 'i':
    {
      if (LocaleNCompare("input",key,2) == 0)
        {
          (void) CopyMagickString(attribute,image->filename,MaxTextExtent);
          break;
        }
      break;
    }
    case 'm':
    {
      if (LocaleNCompare("magick",key,2) == 0)
        {
          (void) CopyMagickString(attribute,image->magick,MaxTextExtent);
          break;
        }
      break;
    }
    case 'n':
    {
      if (LocaleNCompare("name",key,2) == 0)
        {
          (void) CopyMagickString(attribute,filename,MaxTextExtent);
          break;
        }
     break;
    }
    case 's':
    {
      if (LocaleNCompare("size",key,2) == 0)
        {
          char
            format[MaxTextExtent];

          FormatSize(GetBlobSize(image),format);
          (void) FormatMagickString(attribute,MaxTextExtent,"%s",format);
          break;
        }
      if (LocaleNCompare("scene",key,2) == 0)
        {
          (void) FormatMagickString(attribute,MaxTextExtent,"%lu",image->scene);
          if (image_info->number_scenes != 0)
            (void) FormatMagickString(attribute,MaxTextExtent,"%lu",
              image_info->scene);
          break;
        }
      if (LocaleNCompare("scenes",key,6) == 0)
        {
          (void) FormatMagickString(attribute,MaxTextExtent,"%lu",
            (unsigned long) GetImageListLength(image));
          break;
        }
       break;
    }
    case 'o':
    {
      if (LocaleNCompare("output",key,2) == 0)
        {
          (void) CopyMagickString(attribute,image_info->filename,MaxTextExtent);
          break;
        }
     break;
    }
    case 'p':
    {
      if (LocaleNCompare("page",key,2) == 0)
        {
          register const Image
            *p;

          unsigned long
            page;

          p=image;
          for (page=1; GetPreviousImageInList(p) != (Image *) NULL; page++)
            p=GetPreviousImageInList(p);
          (void) FormatMagickString(attribute,MaxTextExtent,"%lu",page);
          break;
        }
      break;
    }
    case 'u':
    {
      if (LocaleNCompare("unique",key,2) == 0)
        {
          (void) CopyMagickString(filename,image_info->unique,MaxTextExtent);
          (void) CopyMagickString(attribute,filename,MaxTextExtent);
          break;
        }
      break;
    }
    case 'w':
    {
      if (LocaleNCompare("width",key,2) == 0)
        {
          (void) FormatMagickString(attribute,MaxTextExtent,"%lu",
            image->magick_columns != 0 ? image->magick_columns : 256UL);
          break;
        }
      break;
    }
    case 'x':
    {
      if (LocaleNCompare("xresolution",key,2) == 0)
        {
          (void) FormatMagickString(attribute,MaxTextExtent,"%g",
            image->x_resolution);
          break;
        }
      break;
    }
    case 'y':
    {
      if (LocaleNCompare("yresolution",key,2) == 0)
        {
          (void) FormatMagickString(attribute,MaxTextExtent,"%g",
            image->y_resolution);
          break;
        }
      break;
    }
    case 'z':
    {
      if (LocaleNCompare("zero",key,2) == 0)
        {
          (void) CopyMagickString(filename,image_info->zero,MaxTextExtent);
          (void) CopyMagickString(attribute,filename,MaxTextExtent);
          break;
        }
      break;
    }
  }
  if (strlen(image->magick_filename) != 0)
    return(GetImageAttribute(image,key));
  return((ImageAttribute *) NULL);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t N e x t I m a g e A t t r i b u t e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetNextImageAttribute() gets the next image attribute.
%
%  The format of the GetNextImageAttribute method is:
%
%      const ImageAttribute *GetNextImageAttribute(const Image *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/
MagickExport const ImageAttribute *GetNextImageAttribute(const Image *image)
{
  const ImageAttribute
    *attribute;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->attributes == (void *) NULL)
    return((const ImageAttribute *) NULL);
  attribute=(const ImageAttribute *)
    GetNextValueInSplayTree((SplayTreeInfo *) image->attributes);
  return(attribute);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s e t I m a g e A t t r i b u t e I t e r a t o r                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResetImageAttributeIterator() resets the image attributes iterator.  Use it
%  in conjunction with GetNextImageAttribute() to iterate over all the values
%  associated with an image.
%
%  The format of the ResetImageAttributeIterator method is:
%
%      ResetImageAttributeIterator(const ImageInfo *image)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
*/
MagickExport void ResetImageAttributeIterator(const Image *image)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->attributes == (void *) NULL)
    return;
  ResetSplayTreeIterator((SplayTreeInfo *) image->attributes);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t I m a g e A t t r i b u t e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetImageAttribute() searches the list of image attributes and replaces the
%  attribute value.  If it is not found in the list, the attribute name
%  and value is added to the list.   
%
%  The format of the SetImageAttribute method is:
%
%       MagickBooleanType SetImageAttribute(Image *image,const char *key,
%         const char *value)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o key: The key.
%
%    o value: The value.
%
*/

static void *DestroyAttribute(void *attribute)
{
  register ImageAttribute
    *p;

  p=(ImageAttribute *) attribute;
  if (p->value != (char *) NULL)
    p->value=(char *) RelinquishMagickMemory(p->value);
  return(RelinquishMagickMemory(p));
}

MagickExport  MagickBooleanType SetImageAttribute(Image *image,const char *key,
  const char *value)
{
  ImageAttribute
    *attribute;

  MagickBooleanType
    status;

  register const char
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((key == (const char *) NULL) || (*key == '\0'))
    return(MagickFalse);
  if (image->attributes == (void *) NULL)
    image->attributes=NewSplayTree(CompareSplayTreeString,
      RelinquishMagickMemory,DestroyAttribute);
  if (value == (const char *) NULL)
    return(DeleteImageAttribute(image,key));
  if (*value == '\0')
    return(MagickFalse);
  /*
    Add new image attribute.
  */
  attribute=(ImageAttribute *) AcquireMagickMemory(sizeof(*attribute));
  if (attribute == (ImageAttribute *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      key);
  (void) ResetMagickMemory(attribute,0,sizeof(*attribute));
  attribute->key=ConstantString(AcquireString(key));
  for (q=value; *q != '\0'; q++)
    if (((int) ((unsigned char) *q) < 32) &&
        (isspace((int) ((unsigned char) *q)) == 0))
      break;
  if (*q != '\0')
    attribute->value=ConstantString(AcquireString(value));
  else
    attribute->value=
      ConstantString(TranslateText((ImageInfo *) NULL,image,value));
  attribute->compression=MagickFalse;
  status=AddValueToSplayTree((SplayTreeInfo *) image->attributes,attribute->key,
    attribute);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T r a c e P S C l i p p a t h                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TracePSClipPath() traces a clip path and returns it as Postscript.
%
%  The format of the TracePSClipPath method is:
%
%      char *TracePSClipPath(unsigned char *blob,size_t length,
%        const unsigned long columns,const unsigned long rows)
%
%  A description of each parameter follows:
%
%    o blob: The blob.
%
%    o length: The length of the blob.
%
%    o columns: The image width.
%
%    o rows: The image height.
%
%
*/
static char *TracePSClippath(unsigned char *blob,size_t length,
  const unsigned long magick_unused(columns),
  const unsigned long magick_unused(rows))
{
  char
    *path,
    *message;

  long
    knot_count,
    selector,
    y;

  MagickBooleanType
    in_subpath;

  PointInfo
    first[3],
    last[3],
    point[3];

  register long
    i,
    x;

  path=AcquireString((char *) NULL);
  if (path == (char *) NULL)
    return((char *) NULL);
  message=AcquireString((char *) NULL);
  (void) FormatMagickString(message,MaxTextExtent,"/ClipImage\n");
  (void) ConcatenateString(&path,message);
  (void) FormatMagickString(message,MaxTextExtent,"{\n");
  (void) ConcatenateString(&path,message);
  (void) FormatMagickString(message,MaxTextExtent,"  /c {curveto} bind def\n");
  (void) ConcatenateString(&path,message);
  (void) FormatMagickString(message,MaxTextExtent,"  /l {lineto} bind def\n");
  (void) ConcatenateString(&path,message);
  (void) FormatMagickString(message,MaxTextExtent,"  /m {moveto} bind def\n");
  (void) ConcatenateString(&path,message);
  (void) FormatMagickString(message,MaxTextExtent,
    "  /v {currentpoint 6 2 roll curveto} bind def\n");
  (void) ConcatenateString(&path,message);
  (void) FormatMagickString(message,MaxTextExtent,
    "  /y {2 copy curveto} bind def\n");
  (void) ConcatenateString(&path,message);
  (void) FormatMagickString(message,MaxTextExtent,
    "  /z {closepath} bind def\n");
  (void) ConcatenateString(&path,message);
  (void) FormatMagickString(message,MaxTextExtent,"  newpath\n");
  (void) ConcatenateString(&path,message);
  /*
    The clipping path format is defined in "Adobe Photoshop File
    Formats Specification" version 6.0 downloadable from adobe.com.
  */
  knot_count=0;
  in_subpath=MagickFalse;
  while (length > 0)
  {
    selector=ReadMSBShort(&blob,&length);
    switch (selector)
    {
      case 0:
      case 3:
      {
        if (knot_count != 0)
          {
            blob+=24;
            length-=24;
            break;
          }
        /*
          Expected subpath length record.
        */
        knot_count=ReadMSBShort(&blob,&length);
        blob+=22;
        length-=22;
        break;
      }
      case 1:
      case 2:
      case 4:
      case 5:
      {
        if (knot_count == 0)
          {
            /*
              Unexpected subpath knot
            */
            blob+=24;
            length-=24;
            break;
          }
        /*
          Add sub-path knot
        */
        for (i=0; i < 3; i++)
        {
          y=ReadMSBLong(&blob,&length);
          x=ReadMSBLong(&blob,&length);
          point[i].x=(double) x/4096/4096;
          point[i].y=1.0-(double) y/4096/4096;
        }
        if (in_subpath == MagickFalse)
          {
            (void) FormatMagickString(message,MaxTextExtent,"  %g %g m\n",
              point[1].x,point[1].y);
            for (i=0; i < 3; i++)
            {
              first[i]=point[i];
              last[i]=point[i];
            }
          }
        else
          {
            /*
              Handle special cases when Bezier curves are used to describe
              corners and straight lines.
            */
            if ((last[1].x == last[2].x) && (last[1].y == last[2].y) &&
                (point[0].x == point[1].x) && (point[0].y == point[1].y))
              (void) FormatMagickString(message,MaxTextExtent,"  %g %g l\n",
                point[1].x,point[1].y);
            else
              if ((last[1].x == last[2].x) && (last[1].y == last[2].y))
                (void) FormatMagickString(message,MaxTextExtent,
                  "  %g %g %g %g v\n",point[0].x,point[0].y,point[1].x,
                  point[1].y);
              else
                if ((point[0].x == point[1].x) && (point[0].y == point[1].y))
                  (void) FormatMagickString(message,MaxTextExtent,
                    "  %g %g %g %g y\n",last[2].x,last[2].y,point[1].x,
                    point[1].y);
                else
                  (void) FormatMagickString(message,MaxTextExtent,
                    "  %g %g %g %g %g %g c\n",last[2].x,last[2].y,point[0].x,
                    point[0].y,point[1].x,point[1].y);
            for (i=0; i < 3; i++)
              last[i]=point[i];
          }
        (void) ConcatenateString(&path,message);
        in_subpath=MagickTrue;
        knot_count--;
        /*
          Close the subpath if there are no more knots.
        */
        if (knot_count == 0)
          {
            /*
              Same special handling as above except we compare to the
              first point in the path and close the path.
            */
            if ((last[1].x == last[2].x) && (last[1].y == last[2].y) &&
                (first[0].x == first[1].x) && (first[0].y == first[1].y))
              (void) FormatMagickString(message,MaxTextExtent,"  %g %g l z\n",
                first[1].x,first[1].y);
            else
              if ((last[1].x == last[2].x) && (last[1].y == last[2].y))
                (void) FormatMagickString(message,MaxTextExtent,
                  "  %g %g %g %g v z\n",first[0].x,first[0].y,first[1].x,
                  first[1].y);
              else
                if ((first[0].x == first[1].x) && (first[0].y == first[1].y))
                  (void) FormatMagickString(message,MaxTextExtent,
                    "  %g %g %g %g y z\n",last[2].x,last[2].y,first[1].x,
                    first[1].y);
                else
                  (void) FormatMagickString(message,MaxTextExtent,
                    "  %g %g %g %g %g %g c z\n",last[2].x,last[2].y,first[0].x,
                    first[0].y,first[1].x,first[1].y);
            (void) ConcatenateString(&path,message);
            in_subpath=MagickFalse;
          }
        break;
      }
      case 6:
      case 7:
      case 8:
      default:
      {
        blob+=24;
        length-=24;
        break;
      }
    }
  }
  /*
    Returns an empty PS path if the path has no knots.
  */
  (void) FormatMagickString(message,MaxTextExtent,"  eoclip\n");
  (void) ConcatenateString(&path,message);
  (void) FormatMagickString(message,MaxTextExtent,"} bind def");
  (void) ConcatenateString(&path,message);
  message=(char *) RelinquishMagickMemory(message);
  return(path);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T r a c e S V G C l i p p a t h                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TraceSVGClipPath() traces a clip path and returns it as SVG.
%
%  The format of the TraceSVGClipPath method is:
%
%      char *TraceSVGClipPath(unsigned char *blob,size_t length,
%        const unsigned long columns,const unsigned long rows)
%
%  A description of each parameter follows:
%
%    o blob: The blob.
%
%    o length: The length of the blob.
%
%    o columns: The image width.
%
%    o rows: The image height.
%
%
*/
static char *TraceSVGClippath(unsigned char *blob,size_t length,
  const unsigned long columns,const unsigned long rows)
{
  char
    *path,
    *message;

  long
    knot_count,
    selector,
    x,
    y;

  MagickBooleanType
    in_subpath;

  PointInfo
    first[3],
    last[3],
    point[3];

  register long
    i;

  path=AcquireString((char *) NULL);
  if (path == (char *) NULL)
    return((char *) NULL);
  message=AcquireString((char *) NULL);
  (void) FormatMagickString(message,MaxTextExtent,
    "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n");
  (void) ConcatenateString(&path,message);
  (void) FormatMagickString(message,MaxTextExtent,
    "<svg width=\"%lu\" height=\"%lu\">\n",columns,rows);
  (void) ConcatenateString(&path,message);
  (void) FormatMagickString(message,MaxTextExtent,"<g>\n");
  (void) ConcatenateString(&path,message);
  (void) FormatMagickString(message,MaxTextExtent,
    "<path style=\"fill:#00000000;stroke:#00000000;");
  (void) ConcatenateString(&path,message);
  (void) FormatMagickString(message,MaxTextExtent,
    "stroke-width:0;stroke-antialiasing:false\" d=\"\n");
  (void) ConcatenateString(&path,message);
  knot_count=0;
  in_subpath=MagickFalse;
  while (length != 0)
  {
    selector=ReadMSBShort(&blob,&length);
    switch (selector)
    {
      case 0:
      case 3:
      {
        if (knot_count != 0)
          {
            blob+=24;
            length-=24;
            break;
          }
        /*
          Expected subpath length record.
        */
        knot_count=ReadMSBShort(&blob,&length);
        blob+=22;
        length-=22;
        break;
      }
      case 1:
      case 2:
      case 4:
      case 5:
      {
        if (knot_count == 0)
          {
            /*
              Unexpected subpath knot.
            */
            blob+=24;
            length-=24;
          }
        else
          {
            /*
              Add sub-path knot
            */
            for (i=0; i < 3; i++)
            {
              y=ReadMSBLong(&blob,&length);
              x=ReadMSBLong(&blob,&length);
              point[i].x=(double) x*columns/4096/4096;
              point[i].y=(double) y*rows/4096/4096;
            }
            if (in_subpath == MagickFalse)
              {
                (void) FormatMagickString(message,MaxTextExtent,"M %g,%g\n",
                  point[1].x,point[1].y);
                for (i=0; i < 3; i++)
                {
                  first[i]=point[i];
                  last[i]=point[i];
                }
              }
            else
              {
                if ((last[1].x == last[2].x) && (last[1].y == last[2].y) &&
                    (point[0].x == point[1].x) && (point[0].y == point[1].y))
                  (void) FormatMagickString(message,MaxTextExtent,"L %g,%g\n",
                    point[1].x,point[1].y);
                else
                  (void) FormatMagickString(message,MaxTextExtent,
                    "C %g,%g %g,%g %g,%g\n",last[2].x,last[2].y,
                    point[0].x,point[0].y,point[1].x,point[1].y);
                for (i=0; i < 3; i++)
                  last[i]=point[i];
              }
            (void) ConcatenateString(&path,message);
            in_subpath=MagickTrue;
            knot_count--;
            /*
              Close the subpath if there are no more knots.
            */
            if (knot_count == 0)
              {
                if ((last[1].x == last[2].x) && (last[1].y == last[2].y) &&
                    (first[0].x == first[1].x) && (first[0].y == first[1].y))
                  (void) FormatMagickString(message,MaxTextExtent,"L %g,%g Z\n",
                    first[1].x,first[1].y);
                else
                  {
                    (void) FormatMagickString(message,MaxTextExtent,
                      "C %g,%g %g,%g %g,%g Z\n",last[2].x,last[2].y,
                      first[0].x,first[0].y,first[1].x,first[1].y);
                    (void) ConcatenateString(&path,message);
                  }
                in_subpath=MagickFalse;
              }
          }
          break;
      }
      case 6:
      case 7:
      case 8:
      default:
      {
        blob+=24;
        length-=24;
        break;
      }
    }
  }
  /*
    Return an empty SVG image if the path does not have knots.
  */
  (void) FormatMagickString(message,MaxTextExtent,"\"/>\n");
  (void) ConcatenateString(&path,message);
  (void) FormatMagickString(message,MaxTextExtent,"</g>\n");
  (void) ConcatenateString(&path,message);
  (void) FormatMagickString(message,MaxTextExtent,"</svg>\n");
  (void) ConcatenateString(&path,message);
  message=(char *) RelinquishMagickMemory(message);
  return(path);
}
