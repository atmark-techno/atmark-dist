/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            X   X  PPPP   M   M                              %
%                             X X   P   P  MM MM                              %
%                              X    PPPP   M M M                              %
%                             X X   P      M   M                              %
%                            X   X  P      M   M                              %
%                                                                             %
%                                                                             %
%                  Read/Write X Windows system Pixmap Format.                 %
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
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/colorspace.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/quantize.h"
#include "magick/resize.h"
#include "magick/resource_.h"
#include "magick/static.h"
#include "magick/string_.h"
#include "magick/utility.h"


/*
  Forward declarations.
*/
static MagickBooleanType
  WritePICONImage(const ImageInfo *,Image *),
  WriteXPMImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s X P M                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsXPM() returns MagickTrue if the image format type, identified by the
%  magick string, is XPM.
%
%  The format of the IsXPM method is:
%
%      MagickBooleanType IsXPM(const unsigned char *magick,const size_t length)
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
static MagickBooleanType IsXPM(const unsigned char *magick,const size_t length)
{
  if (length < 9)
    return(MagickFalse);
  if (LocaleNCompare((char *) magick+1,"* XPM *",7) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d X P M I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadXPMImage() reads an X11 pixmap image file and returns it.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadXPMImage method is:
%
%      Image *ReadXPMImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/

static char *ParseColor(char *data)
{
#define NumberTargets  6

  static const char
    *targets[NumberTargets] = { "c ", "g ", "g4 ", "m ", "b ", "s " };

  register char
    *p,
    *r;

  register const char
    *q;

  register long
    i;

  for (i=0; i < NumberTargets; i++)
  {
    r=data;
    for (q=targets[i]; *r != '\0'; r++)
    {
      if (*r != *q)
        continue;
      if (isspace((int) ((unsigned char) (*(r-1)))) == 0)
        continue;
      p=r;
      for ( ; ; )
      {
        if (*q == '\0')
          return(r);
        if (*p++ != *q++)
          break;
      }
      q=targets[i];
    }
  }
  return((char *) NULL);
}

static Image *ReadXPMImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  char
    key[MaxTextExtent],
    **keys,
    target[MaxTextExtent],
    **textlist,
    *xpm_buffer;

  Image
    *image;

  ssize_t
    count;

  long
    j,
    none,
    y;

  MagickBooleanType
    status;

  register char
    *p,
    *q;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *r;

  register long
    i;

  size_t
    length;

  unsigned long
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
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  /*
    Read XPM file.
  */
  length=MaxTextExtent;
  xpm_buffer=(char *) AcquireMagickMemory((size_t) length);
  p=xpm_buffer;
  if (xpm_buffer != (char *) NULL)
    while (ReadBlobString(image,p) != (char *) NULL)
    {
      if (*p == '#')
        if ((p == xpm_buffer) || (*(p-1) == '\n'))
          continue;
      if ((*p == '}') && (*(p+1) == ';'))
        break;
      p+=strlen(p);
      if ((size_t) (p-xpm_buffer+MaxTextExtent) < length)
        continue;
      length<<=1;
      xpm_buffer=(char *) ResizeMagickMemory(xpm_buffer,
        (length+MaxTextExtent)*sizeof(*xpm_buffer));
      if (xpm_buffer == (char *) NULL)
        break;
      p=xpm_buffer+strlen(xpm_buffer);
    }
  if (xpm_buffer == (char *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  /*
    Remove comments.
  */
  count=0;
  for (p=xpm_buffer; *p != '\0'; p++)
  {
    if (*p != '"')
      continue;
    count=(ssize_t) sscanf(p+1,"%lu %lu %lu %lu",&image->columns,&image->rows,
      &image->colors,&width);
    if (count == 4)
      break;
  }
  if ((count != 4) || (width > 2) || (image->columns == 0) ||
      (image->rows == 0) || (image->colors == 0))
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  image->depth=16;
  /*
    Remove unquoted characters.
  */
  i=0;
  for ( ; *p != '\0'; p++)
  {
    if (*p != '"')
      continue;
    for (q=p+1; *q != '\0'; q++)
      if (*q == '"')
        break;
    (void) CopyMagickString(xpm_buffer+i,p+1,(size_t) (q-p));
    i+=q-p-1;
    xpm_buffer[i++]='\n';
    p=q+1;
  }
  xpm_buffer[i]='\0';
  textlist=StringToList(xpm_buffer);
  xpm_buffer=(char *) RelinquishMagickMemory(xpm_buffer);
  if (textlist == (char **) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  /*
    Initialize image structure.
  */
  keys=(char **) AcquireMagickMemory((size_t) image->colors*sizeof(*keys));
  if ((AllocateImageColormap(image,image->colors) == MagickFalse) ||
      (keys == (char **) NULL))
    {
      for (i=0; textlist[i] != (char *) NULL; i++)
        textlist[i]=(char *) RelinquishMagickMemory(textlist[i]);
      textlist=(char **) RelinquishMagickMemory(textlist);
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Read image colormap.
  */
  i=1;
  none=(-1);
  for (j=0; j < (long) image->colors; j++)
  {
    p=textlist[i++];
    if (p == (char *) NULL)
      break;
    keys[j]=(char *) AcquireMagickMemory((size_t) width+1);
    if (keys[j] == (char *) NULL)
      {
        for (i=0; textlist[i] != (char *) NULL; i++)
          textlist[i]=(char *) RelinquishMagickMemory(textlist[i]);
        textlist=(char **) RelinquishMagickMemory(textlist);
        keys=(char **) RelinquishMagickMemory(keys);
        ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
      }
    (void) CopyMagickString(keys[j],p,(size_t) width+1);
    /*
      Parse color.
    */
    (void) strcpy(target,"gray");
    q=ParseColor(p+width);
    if (q != (char *) NULL)
      {
        while ((isspace((int) ((unsigned char) *q)) == 0) && (*q != '\0'))
          q++;
        (void) CopyMagickString(target,q,MaxTextExtent);
        q=ParseColor(target);
        if (q != (char *) NULL)
          *q='\0';
      }
    StripString(target);
    if (LocaleCompare(target,"none") == 0)
      {
        image->storage_class=DirectClass;
        image->matte=MagickTrue;
        none=j;
        (void) strcpy(target,"black");
      }
    if (QueryColorDatabase(target,&image->colormap[j],exception) == MagickFalse)
      break;
  }
  if (j < (long) image->colors)
    {
      for (i=0; textlist[i] != (char *) NULL; i++)
        textlist[i]=(char *) RelinquishMagickMemory(textlist[i]);
      textlist=(char **) RelinquishMagickMemory(textlist);
      ThrowReaderException(CorruptImageError,"CorruptImage");
    }
  j=0;
  key[width]='\0';
  if (image_info->ping == MagickFalse)
    {
      /*
        Read image pixels.
      */
      for (y=0; y < (long) image->rows; y++)
      {
        p=textlist[i++];
        if (p == (char *) NULL)
          break;
        r=SetImagePixels(image,0,y,image->columns,1);
        if (r == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) image->columns; x++)
        {
          (void) CopyMagickString(key,p,(size_t) width+1);
          if (strcmp(key,keys[j]) != 0)
            for (j=0; j < (long) Max(image->colors-1,1); j++)
              if (strcmp(key,keys[j]) == 0)
                break;
          if (image->storage_class == PseudoClass)
            indexes[x]=(IndexPacket) j;
          *r=image->colormap[j];
          r->opacity=(Quantum)
            (j == (long) none ? TransparentOpacity : OpaqueOpacity);
          r++;
          p+=width;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
      if (y < (long) image->rows)
        ThrowReaderException(CorruptImageError,"NotEnoughPixelData");
    }
  /*
    Free resources.
  */
  for (i=0; i < (long) image->colors; i++)
    keys[i]=(char *) RelinquishMagickMemory(keys[i]);
  keys=(char **) RelinquishMagickMemory(keys);
  for (i=0; textlist[i] != (char *) NULL; i++)
    textlist[i]=(char *) RelinquishMagickMemory(textlist[i]);
  textlist=(char **) RelinquishMagickMemory(textlist);
  CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r X P M I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterXPMImage() adds attributes for the XPM image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterXPMImage method is:
%
%      RegisterXPMImage(void)
%
*/
ModuleExport void RegisterXPMImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("PICON");
  entry->decoder=(DecoderHandler *) ReadXPMImage;
  entry->encoder=(EncoderHandler *) WritePICONImage;
  entry->adjoin=MagickFalse;
  entry->description=AcquireString("Personal Icon");
  entry->module=AcquireString("XPM");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PM");
  entry->decoder=(DecoderHandler *) ReadXPMImage;
  entry->encoder=(EncoderHandler *) WriteXPMImage;
  entry->adjoin=MagickFalse;
  entry->stealth=MagickTrue;
  entry->description=AcquireString("X Windows system pixmap (color)");
  entry->module=AcquireString("XPM");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("XPM");
  entry->decoder=(DecoderHandler *) ReadXPMImage;
  entry->encoder=(EncoderHandler *) WriteXPMImage;
  entry->magick=(MagickHandler *) IsXPM;
  entry->adjoin=MagickFalse;
  entry->description=AcquireString("X Windows system pixmap (color)");
  entry->module=AcquireString("XPM");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r X P M I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterXPMImage() removes format registrations made by the
%  XPM module from the list of supported formats.
%
%  The format of the UnregisterXPMImage method is:
%
%      UnregisterXPMImage(void)
%
*/
ModuleExport void UnregisterXPMImage(void)
{
  (void) UnregisterMagickInfo("PICON");
  (void) UnregisterMagickInfo("PM");
  (void) UnregisterMagickInfo("XPM");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e P I C O N I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Procedure WritePICONImage() writes an image to a file in the Personal Icon
%  format.
%
%  The format of the WritePICONImage method is:
%
%      MagickBooleanType WritePICONImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WritePICONImage(const ImageInfo *image_info,
  Image *image)
{
#define ColormapExtent  155
#define GraymapExtent  95
#define PiconGeometry  "48x48>"

  static unsigned char
    Colormap[]=
    {
      0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x06, 0x00, 0x05, 0x00, 0xf4, 0x05,
      0x00, 0x00, 0x00, 0x00, 0x2f, 0x4f, 0x4f, 0x70, 0x80, 0x90, 0x7e, 0x7e,
      0x7e, 0xdc, 0xdc, 0xdc, 0xff, 0xff, 0xff, 0x00, 0x00, 0x80, 0x00, 0x00,
      0xff, 0x1e, 0x90, 0xff, 0x87, 0xce, 0xeb, 0xe6, 0xe6, 0xfa, 0x00, 0xff,
      0xff, 0x80, 0x00, 0x80, 0xb2, 0x22, 0x22, 0x2e, 0x8b, 0x57, 0x32, 0xcd,
      0x32, 0x00, 0xff, 0x00, 0x98, 0xfb, 0x98, 0xff, 0x00, 0xff, 0xff, 0x00,
      0x00, 0xff, 0x63, 0x47, 0xff, 0xa5, 0x00, 0xff, 0xd7, 0x00, 0xff, 0xff,
      0x00, 0xee, 0x82, 0xee, 0xa0, 0x52, 0x2d, 0xcd, 0x85, 0x3f, 0xd2, 0xb4,
      0x8c, 0xf5, 0xde, 0xb3, 0xff, 0xfa, 0xcd, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x21, 0xf9, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00,
      0x00, 0x00, 0x06, 0x00, 0x05, 0x00, 0x00, 0x05, 0x18, 0x20, 0x10, 0x08,
      0x03, 0x51, 0x18, 0x07, 0x92, 0x28, 0x0b, 0xd3, 0x38, 0x0f, 0x14, 0x49,
      0x13, 0x55, 0x59, 0x17, 0x96, 0x69, 0x1b, 0xd7, 0x85, 0x00, 0x3b,
    },
    Graymap[]=
    {
      0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x04, 0x00, 0x04, 0x00, 0xf3, 0x0f,
      0x00, 0x00, 0x00, 0x00, 0x12, 0x12, 0x12, 0x21, 0x21, 0x21, 0x33, 0x33,
      0x33, 0x45, 0x45, 0x45, 0x54, 0x54, 0x54, 0x66, 0x66, 0x66, 0x78, 0x78,
      0x78, 0x87, 0x87, 0x87, 0x99, 0x99, 0x99, 0xab, 0xab, 0xab, 0xba, 0xba,
      0xba, 0xcc, 0xcc, 0xcc, 0xde, 0xde, 0xde, 0xed, 0xed, 0xed, 0xff, 0xff,
      0xff, 0x21, 0xf9, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00,
      0x00, 0x00, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04, 0x0c, 0x10, 0x04, 0x31,
      0x48, 0x31, 0x07, 0x25, 0xb5, 0x58, 0x73, 0x4f, 0x04, 0x00, 0x3b,
    };

#define MaxCixels  92

  static const char
    Cixel[MaxCixels+1] = " .XoO+@#$%&*=-;:>,<1234567890qwertyuipasdfghjk"
                         "lzxcvbnmMNBVCZASDFGHJKLPIUYTREWQ!~^/()_`'][{}|";

  char
    buffer[MaxTextExtent],
    basename[MaxTextExtent],
    name[MaxTextExtent],
    symbol[MaxTextExtent];

  Image
    *picon,
    *map;

  ImageInfo
    *blob_info;

  long
    j,
    k,
    y;

  MagickBooleanType
    status,
    transparent;

  RectangleInfo
    geometry;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  register PixelPacket
    *q;

  unsigned long
    characters_per_pixel,
    colors;

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
  (void) SetImageColorspace(image,RGBColorspace);
  SetGeometry(image,&geometry);
  (void) ParseMetaGeometry(PiconGeometry,&geometry.x,&geometry.y,
    &geometry.width,&geometry.height);
  picon=ResizeImage(image,geometry.width,geometry.height,TriangleFilter,1.0,
    &image->exception);
  blob_info=CloneImageInfo(image_info);
  (void) AcquireUniqueFilename(blob_info->filename);
  if ((image_info->type != TrueColorType) &&
      (IsGrayImage(image,&image->exception) != MagickFalse))
    map=BlobToImage(blob_info,Graymap,GraymapExtent,&image->exception);
  else
    map=BlobToImage(blob_info,Colormap,ColormapExtent,&image->exception);
  (void) RelinquishUniqueFileResource(blob_info->filename);
  blob_info=DestroyImageInfo(blob_info);
  if ((picon == (Image *) NULL) || (map == (Image *) NULL))
    return(MagickFalse);
  status=MapImage(picon,map,image_info->dither);
  map=DestroyImage(map);
  transparent=MagickFalse;
  if (picon->storage_class == PseudoClass)
    {
      CompressImageColormap(picon);
      if (picon->matte != MagickFalse)
        transparent=MagickTrue;
    }
  else
    {
      /*
        Convert DirectClass to PseudoClass picon.
      */
      if (picon->matte != MagickFalse)
        {
          /*
            Map all the transparent pixels.
          */
          for (y=0; y < (long) picon->rows; y++)
          {
            q=GetImagePixels(picon,0,y,picon->columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) picon->columns; x++)
            {
              if (q->opacity == TransparentOpacity)
                transparent=MagickTrue;
              else
                q->opacity=OpaqueOpacity;
              q++;
            }
            if (SyncImagePixels(picon) == MagickFalse)
              break;
          }
        }
      (void) SetImageType(picon,PaletteType);
    }
  colors=picon->colors;
  if (transparent != MagickFalse)
    {
      colors++;
      picon->colormap=(PixelPacket *) ResizeMagickMemory((void **)
        &picon->colormap,(size_t) colors*sizeof(*picon->colormap));
      if (picon->colormap == (PixelPacket *) NULL)
        ThrowWriterException(ResourceLimitError,"MemoryAllocationError");
      for (y=0; y < (long) picon->rows; y++)
      {
        q=GetImagePixels(picon,0,y,picon->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(picon);
        for (x=0; x < (long) picon->columns; x++)
        {
          if (q->opacity == TransparentOpacity)
            indexes[x]=(IndexPacket) picon->colors;
          q++;
        }
        if (SyncImagePixels(picon) == MagickFalse)
          break;
      }
    }
  /*
    Compute the character per pixel.
  */
  characters_per_pixel=1;
  for (k=MaxCixels; (long) colors > k; k*=MaxCixels)
    characters_per_pixel++;
  /*
    XPM header.
  */
  (void) WriteBlobString(image,"/* XPM */\n");
  GetPathComponent(picon->filename,BasePath,basename);
  (void) FormatMagickString(buffer,MaxTextExtent,
    "static char *%s[] = {\n",basename);
  (void) WriteBlobString(image,buffer);
  (void) WriteBlobString(image,"/* columns rows colors chars-per-pixel */\n");
  (void) FormatMagickString(buffer,MaxTextExtent,"\"%lu %lu %lu %ld\",\n",
    picon->columns,picon->rows,colors,characters_per_pixel);
  (void) WriteBlobString(image,buffer);
  for (i=0; i < (long) colors; i++)
  {
    /*
      Define XPM color.
    */
    picon->colormap[i].opacity=OpaqueOpacity;
    (void) QueryColorname(picon,picon->colormap+i,XPMCompliance,name,
      &picon->exception);
    if (transparent != MagickFalse)
      {
        if (i == (long) (colors-1))
          (void) strcpy(name,"grey75");
      }
    /*
      Write XPM color.
    */
    k=i % MaxCixels;
    symbol[0]=Cixel[k];
    for (j=1; j < (long) characters_per_pixel; j++)
    {
      k=((i-k)/MaxCixels) % MaxCixels;
      symbol[j]=Cixel[k];
    }
    symbol[j]='\0';
    (void) FormatMagickString(buffer,MaxTextExtent,"\"%s c %s\",\n",
       symbol,name);
    (void) WriteBlobString(image,buffer);
  }
  /*
    Define XPM pixels.
  */
  (void) WriteBlobString(image,"/* pixels */\n");
  for (y=0; y < (long) picon->rows; y++)
  {
    p=AcquireImagePixels(picon,0,y,picon->columns,1,&picon->exception);
    if (p == (const PixelPacket *) NULL)
      break;
    indexes=GetIndexes(picon);
    (void) WriteBlobString(image,"\"");
    for (x=0; x < (long) picon->columns; x++)
    {
      k=(long) (indexes[x] % MaxCixels);
      symbol[0]=Cixel[k];
      for (j=1; j < (long) characters_per_pixel; j++)
      {
        k=(((int) indexes[x]-k)/MaxCixels) % MaxCixels;
        symbol[j]=Cixel[k];
      }
      symbol[j]='\0';
      (void) CopyMagickString(buffer,symbol,MaxTextExtent);
      (void) WriteBlobString(image,buffer);
    }
    (void) FormatMagickString(buffer,MaxTextExtent,"\"%s\n",
      (y == (long) (picon->rows-1) ? "" : ","));
    (void) WriteBlobString(image,buffer);
    if (QuantumTick(y,picon->rows) != MagickFalse)
      if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
          (QuantumTick(y,picon->rows) != MagickFalse))
        {
          status=image->progress_monitor(SaveImageTag,y,picon->rows,
            image->client_data);
          if (status == MagickFalse)
            break;
        }
  }
  picon=DestroyImage(picon);
  (void) WriteBlobString(image,"};\n");
  CloseBlob(image);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e X P M I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Procedure WriteXPMImage() writes an image to a file in the X pixmap format.
%
%  The format of the WriteXPMImage method is:
%
%      MagickBooleanType WriteXPMImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteXPMImage(const ImageInfo *image_info,Image *image)
{
#define MaxCixels  92

  static const char
    Cixel[MaxCixels+1] = " .XoO+@#$%&*=-;:>,<1234567890qwertyuipasdfghjk"
                         "lzxcvbnmMNBVCZASDFGHJKLPIUYTREWQ!~^/()_`'][{}|";

  char
    buffer[MaxTextExtent],
    basename[MaxTextExtent],
    name[MaxTextExtent],
    symbol[MaxTextExtent];

  long
    j,
    k,
    y;

  MagickBooleanType
    status,
    transparent;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  register PixelPacket
    *q;

  unsigned long
    characters_per_pixel,
    colors;

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
  (void) SetImageColorspace(image,RGBColorspace);
  transparent=MagickFalse;
  if (image->storage_class == PseudoClass)
    {
      CompressImageColormap(image);
      if (image->matte != MagickFalse)
        transparent=MagickTrue;
    }
  else
    {
      /*
        Convert DirectClass to PseudoClass image.
      */
      if (image->matte != MagickFalse)
        {
          /*
            Map all the transparent pixels.
          */
          for (y=0; y < (long) image->rows; y++)
          {
            q=GetImagePixels(image,0,y,image->columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) image->columns; x++)
            {
              if (q->opacity == TransparentOpacity)
                transparent=MagickTrue;
              else
                q->opacity=OpaqueOpacity;
              q++;
            }
            if (SyncImagePixels(image) == MagickFalse)
              break;
          }
        }
      (void) SetImageType(image,PaletteType);
    }
  colors=image->colors;
  if (transparent != MagickFalse)
    {
      colors++;
      image->colormap=(PixelPacket *) ResizeMagickMemory(image->colormap,
        (size_t) colors*sizeof(*image->colormap));
      if (image->colormap == (PixelPacket *) NULL)
        ThrowWriterException(ResourceLimitError,"MemoryAllocationError");
      for (y=0; y < (long) image->rows; y++)
      {
        q=GetImagePixels(image,0,y,image->columns,1);
        if (q == (PixelPacket *) NULL)
          break;
        indexes=GetIndexes(image);
        for (x=0; x < (long) image->columns; x++)
        {
          if (q->opacity == TransparentOpacity)
            indexes[x]=(IndexPacket) image->colors;
          q++;
        }
        if (SyncImagePixels(image) == MagickFalse)
          break;
      }
    }
  /*
    Compute the character per pixel.
  */
  characters_per_pixel=1;
  for (k=MaxCixels; (long) colors > k; k*=MaxCixels)
    characters_per_pixel++;
  /*
    XPM header.
  */
  (void) WriteBlobString(image,"/* XPM */\n");
  GetPathComponent(image->filename,BasePath,basename);
  (void) FormatMagickString(buffer,MaxTextExtent,
    "static char *%s[] = {\n",basename);
  (void) WriteBlobString(image,buffer);
  (void) WriteBlobString(image,"/* columns rows colors chars-per-pixel */\n");
  (void) FormatMagickString(buffer,MaxTextExtent,"\"%lu %lu %lu %ld\",\n",
    image->columns,image->rows,colors,characters_per_pixel);
  (void) WriteBlobString(image,buffer);
  for (i=0; i < (long) colors; i++)
  {
    /*
      Define XPM color.
    */
    image->colormap[i].opacity=OpaqueOpacity;
    (void) QueryColorname(image,image->colormap+i,XPMCompliance,name,
      &image->exception);
    if (transparent != MagickFalse)
      {
        if (i == (long) (colors-1))
          (void) strcpy(name,"None");
      }
    /*
      Write XPM color.
    */
    k=i % MaxCixels;
    symbol[0]=Cixel[k];
    for (j=1; j < (long) characters_per_pixel; j++)
    {
      k=((i-k)/MaxCixels) % MaxCixels;
      symbol[j]=Cixel[k];
    }
    symbol[j]='\0';
    (void) FormatMagickString(buffer,MaxTextExtent,"\"%s c %s\",\n",
      symbol,name);
    (void) WriteBlobString(image,buffer);
  }
  /*
    Define XPM pixels.
  */
  (void) WriteBlobString(image,"/* pixels */\n");
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
    if (p == (const PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    (void) WriteBlobString(image,"\"");
    for (x=0; x < (long) image->columns; x++)
    {
      k=(long) (indexes[x] % MaxCixels);
      symbol[0]=Cixel[k];
      for (j=1; j < (long) characters_per_pixel; j++)
      {
        k=(((int) indexes[x]-k)/MaxCixels) % MaxCixels;
        symbol[j]=Cixel[k];
      }
      symbol[j]='\0';
      (void) CopyMagickString(buffer,symbol,MaxTextExtent);
      (void) WriteBlobString(image,buffer);
    }
    (void) FormatMagickString(buffer,MaxTextExtent,"\"%s\n",
      (y == (long) (image->rows-1) ? "" : ","));
    (void) WriteBlobString(image,buffer);
    if (QuantumTick(y,image->rows) != MagickFalse)
      if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
          (QuantumTick(y,image->rows) != MagickFalse))
        {
          status=image->progress_monitor(SaveImageTag,y,image->rows,
            image->client_data);
          if (status == MagickFalse)
            break;
        }
  }
  (void) WriteBlobString(image,"};\n");
  CloseBlob(image);
  return(MagickTrue);
}
