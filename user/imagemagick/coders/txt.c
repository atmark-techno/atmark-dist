/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            TTTTT  X   X  TTTTT                              %
%                              T     X X     T                                %
%                              T      X      T                                %
%                              T     X X     T                                %
%                              T    X   X    T                                %
%                                                                             %
%                                                                             %
%                      Render Text Onto A Canvas Image.                       %
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
#include "magick/annotate.h"
#include "magick/blob.h"
#include "magick/blob-private.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/colorspace.h"
#include "magick/constitute.h"
#include "magick/draw.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/option.h"
#include "magick/pixel-private.h"
#include "magick/static.h"
#include "magick/string_.h"

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteTXTImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s T X T                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsTXT() returns MagickTrue if the image format type, identified by the magick
%  string, is TXT.
%
%  The format of the IsTXT method is:
%
%      MagickBooleanType IsTXT(const unsigned char *magick,const size_t length)
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
static MagickBooleanType IsTXT(const unsigned char *magick,const size_t length)
{
#define MagickID  "# ImageMagick pixel enumeration:"

  char
    format[MaxTextExtent];

  ssize_t
    count;

  unsigned long
    columns,
    max_value,
    rows;

  if (length < 32)
    return(MagickFalse);
  count=(ssize_t) sscanf((char *) magick+32,"%lu,%lu,%lu,%s",&columns,&rows,
    &max_value,format);
  if ((LocaleNCompare((char *) magick,MagickID,strlen(MagickID)) != 0) ||
      (count != 4))
    return(MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d T E X T I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadTEXTImage() reads a text file and returns it as an image.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadTEXTImage method is:
%
%      Image *ReadTEXTImage(const ImageInfo *image_info,
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
static Image *ReadTEXTImage(const ImageInfo *image_info,
  ExceptionInfo *exception)
{
  char
    filename[MaxTextExtent],
    geometry[MaxTextExtent],
    *p,
    text[MaxTextExtent];

  DrawInfo
    *draw_info;

  Image
    *image,
    *texture;

  long
    offset;

  MagickBooleanType
    status;

  PointInfo
    delta;

  RectangleInfo
    page;

  TypeMetric
    metrics;

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
    Set the page geometry.
  */
  delta.x=DefaultResolution;
  delta.y=DefaultResolution;
  if ((image->x_resolution == 0.0) || (image->y_resolution == 0.0))
    {
      GeometryInfo
        geometry_info;

      MagickStatusType
        flags;

      flags=ParseGeometry(PSDensityGeometry,&geometry_info);
      image->x_resolution=geometry_info.rho;
      image->y_resolution=geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        image->y_resolution=image->x_resolution;
    }
  page.width=612;
  page.height=792;
  page.x=43;
  page.y=43;
  if (image_info->page != (char *) NULL)
    (void) ParseAbsoluteGeometry(image_info->page,&page);
  /*
    Initialize Image structure.
  */
  image->columns=(unsigned long)
    (((page.width*image->x_resolution)/delta.x)+0.5);
  image->rows=(unsigned long) (((page.height*image->y_resolution)/delta.y)+0.5);
  image->page.x=0;
  image->page.y=0;
  texture=(Image *) NULL;
  if (image_info->texture != (char *) NULL)
    {
      ImageInfo
        *read_info;

      read_info=CloneImageInfo(image_info);
      SetImageInfoBlob(read_info,(void *) NULL,0);
      (void) CopyMagickString(read_info->filename,image_info->texture,
        MaxTextExtent);
      texture=ReadImage(read_info,exception);
      read_info=DestroyImageInfo(read_info);
    }
  /*
    Annotate the text image.
  */
  SetImageBackgroundColor(image);
  draw_info=CloneDrawInfo(image_info,(DrawInfo *) NULL);
  draw_info->fill=image_info->pen;
  (void) CloneString(&draw_info->text,image_info->filename);
  (void) FormatMagickString(geometry,MaxTextExtent,"0x0%+ld%+ld",page.x,page.y);
  (void) CloneString(&draw_info->geometry,geometry);
  status=GetTypeMetrics(image,draw_info,&metrics);
  if (status == MagickFalse)
    ThrowReaderException(TypeError,"UnableToGetTypeMetrics");
  page.y=(long) (page.y+metrics.ascent+0.5);
  (void) FormatMagickString(geometry,MaxTextExtent,"0x0%+ld%+ld",page.x,page.y);
  (void) CloneString(&draw_info->geometry,geometry);
  (void) CopyMagickString(filename,image_info->filename,MaxTextExtent);
  if (*draw_info->text != '\0')
    *draw_info->text='\0';
  p=ReadBlobString(image,text);
  for (offset=2*page.y; p != (char *) NULL; )
  {
    /*
      Annotate image with text.
    */
    (void) ConcatenateString(&draw_info->text,text);
    (void) ConcatenateString(&draw_info->text,"\n");
    offset+=(long) (metrics.ascent-metrics.descent);
    if (image->previous == (Image *) NULL)
      if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
          (QuantumTick(offset,image->rows) != MagickFalse))
        {
          status=image->progress_monitor(LoadImageTag,offset,image->rows,
            image->client_data);
          if (status == MagickFalse)
            break;
        }
    p=ReadBlobString(image,text);
    if ((offset < (long) image->rows) && (p != (char *) NULL))
      continue;
    if (texture != (Image *) NULL)
      {
        MagickProgressMonitor
          progress_monitor;

        progress_monitor=SetImageProgressMonitor(image,
          (MagickProgressMonitor) NULL,image->client_data);
        (void) TextureImage(image,texture);
        (void) SetImageProgressMonitor(image,progress_monitor,
          image->client_data);
      }
    (void) AnnotateImage(image,draw_info);
    if (p == (char *) NULL)
      break;
    /*
      Page is full-- allocate next image structure.
    */
    *draw_info->text='\0';
    offset=2*page.y;
    AllocateNextImage(image_info,image);
    if (GetNextImageInList(image) == (Image *) NULL)
      {
        image=DestroyImageList(image);
        return((Image *) NULL);
      }
    image->next->columns=image->columns;
    image->next->rows=image->rows;
    image=SyncNextImageInList(image);
    (void) CopyMagickString(image->filename,filename,MaxTextExtent);
    SetImageBackgroundColor(image);
    if (image->progress_monitor != (MagickProgressMonitor) NULL)
      {
        status=image->progress_monitor(LoadImagesTag,TellBlob(image),
          GetBlobSize(image),image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  if (texture != (Image *) NULL)
    {
      MagickProgressMonitor
        progress_monitor;

      progress_monitor=SetImageProgressMonitor(image,
        (MagickProgressMonitor) NULL,image->client_data);
      (void) TextureImage(image,texture);
      (void) SetImageProgressMonitor(image,progress_monitor,image->client_data);
    }
  (void) AnnotateImage(image,draw_info);
  if (texture != (Image *) NULL)
    texture=DestroyImage(texture);
  draw_info=DestroyDrawInfo(draw_info);
  CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d T X T I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadTXTImage() reads a text file and returns it as an image.  It allocates
%  the memory necessary for the new Image structure and returns a pointer to
%  the new image.
%
%  The format of the ReadTXTImage method is:
%
%      Image *ReadTXTImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadTXTImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  char
    format[MaxTextExtent],
    text[MaxTextExtent];

  Image
    *image;

  long
    x,
    y;

  LongPixelPacket
    pixel;

  MagickBooleanType
    status;

  Quantum
    *scale;

  register long
    i;

  register PixelPacket
    *q;

  ssize_t
    count;

  unsigned long
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
  (void) ReadBlobString(image,text);
  count=(ssize_t) sscanf(text+32,"%lu,%lu,%lu,%s",&image->columns,&image->rows,
    &max_value,format);
  if ((LocaleNCompare(text,"# ImageMagick pixel enumeration:",32) != 0) ||
      (count != 4))
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
  if (LocaleNCompare(format,"CMYK",4) == 0)
    image->colorspace=CMYKColorspace;
  image->matte=(MagickBooleanType) (strchr(format,'A') != (char *) NULL);
  scale=(Quantum *) NULL;
  if (max_value != (unsigned long) QuantumRange)
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
  pixel.opacity=OpaqueOpacity;
  while (ReadBlobString(image,text) != (char *) NULL)
  {
    if (image->matte == MagickFalse)
      {
        count=(ssize_t) sscanf(text,"%ld,%ld: (%lu,%lu,%lu)",&x,&y,&pixel.red,
          &pixel.green,&pixel.blue);
        if (count != 5)
          continue;
      }
    else
      {
        count=(ssize_t) sscanf(text,"%ld,%ld: (%lu,%lu,%lu,%lu)",&x,&y,
          &pixel.red,&pixel.green,&pixel.blue,&pixel.opacity);
        if (count != 6)
          continue;
      }
    if (scale != (Quantum *) NULL)
      {
        pixel.red=scale[pixel.red];
        pixel.green=scale[pixel.green];
        pixel.blue=scale[pixel.blue];
        pixel.opacity=scale[pixel.opacity];
      }
    q=SetImagePixels(image,x,y,1,1);
    if (q == (PixelPacket *) NULL)
      continue;
    q->red=(Quantum) pixel.red;
    q->green=(Quantum) pixel.green;
    q->blue=(Quantum) pixel.blue;
    q->opacity=(Quantum) pixel.opacity;
    (void) SyncImagePixels(image);
  }
  if (scale != (Quantum *) NULL)
    scale=(Quantum *) RelinquishMagickMemory(scale);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r T X T I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterTXTImage() adds attributes for the TXT image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterTXTImage method is:
%
%      RegisterTXTImage(void)
%
*/
ModuleExport void RegisterTXTImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("TEXT");
  entry->decoder=(DecoderHandler *) ReadTEXTImage;
  entry->encoder=(EncoderHandler *) WriteTXTImage;
  entry->raw=MagickTrue;
  entry->endian_support=MagickTrue;
  entry->description=AcquireString("Text");
  entry->module=AcquireString("TXT");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("TXT");
  entry->decoder=(DecoderHandler *) ReadTXTImage;
  entry->encoder=(EncoderHandler *) WriteTXTImage;
  entry->description=AcquireString("Text");
  entry->magick=(MagickHandler *) IsTXT;
  entry->module=AcquireString("TXT");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r T X T I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterTXTImage() removes format registrations made by the
%  TXT module from the list of supported formats.
%
%  The format of the UnregisterTXTImage method is:
%
%      UnregisterTXTImage(void)
%
*/
ModuleExport void UnregisterTXTImage(void)
{
  (void) UnregisterMagickInfo("TEXT");
  (void) UnregisterMagickInfo("TXT");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e T X T I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteTXTImage writes the pixel values as text numbers.
%
%  The format of the WriteTXTImage method is:
%
%      MagickBooleanType WriteTXTImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteTXTImage(const ImageInfo *image_info,Image *image)
{
  char
    buffer[MaxTextExtent],
    format[MaxTextExtent],
    tuple[MaxTextExtent];

  long
    y;

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  MagickPixelPacket
    pixel;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
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
  status=OpenBlob(image_info,image,WriteBlobMode,&image->exception);
  if (status == MagickFalse)
    return(status);
  if (image->colorspace == CMYKColorspace)
    (void) strcpy(format,"CMYK");
  else
    (void) strcpy(format,"RGB");
  if (image->matte != MagickFalse)
    (void) ConcatenateMagickString(format,"A",MaxTextExtent);
  (void) FormatMagickString(buffer,MaxTextExtent,
    "# ImageMagick pixel enumeration: %lu,%lu,%lu,%s\n",image->columns,
    image->rows,image->depth == QuantumDepth ? (unsigned long) QuantumRange :
    (1L << image->depth)-1,format);
  (void) WriteBlobString(image,buffer);
  scene=0;
  do
  {
    /*
      Convert MIFF to TXT raster pixels.
    */
    GetMagickPixelPacket(image,&pixel);
    for (y=0; y < (long) image->rows; y++)
    {
      p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
      if (p == (const PixelPacket *) NULL)
        break;
      indexes=GetIndexes(image);
      for (x=0; x < (long) image->columns; x++)
      {
        (void) FormatMagickString(buffer,MaxTextExtent,"%ld,%ld: ",x,y);
        (void) WriteBlobString(image,buffer);
        SetMagickPixelPacket(p,indexes+x,&pixel);
        GetColorTuple(&pixel,MagickFalse,tuple);
        (void) ConcatenateMagickString(tuple," ",MaxTextExtent);
        (void) WriteBlobString(image,tuple);
        (void) QueryColorname(image,p,SVGCompliance,tuple,&image->exception);
        (void) WriteBlobString(image,tuple);
        (void) WriteBlobString(image,"\n");
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
