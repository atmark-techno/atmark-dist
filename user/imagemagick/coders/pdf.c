/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            PPPP   DDDD   FFFFF                              %
%                            P   P  D   D  F                                  %
%                            PPPP   D   D  FFF                                %
%                            P      D   D  F                                  %
%                            P      DDDD   F                                  %
%                                                                             %
%                                                                             %
%                   Read/Write Portable Document Format.                      %
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
#include "magick/compress.h"
#include "magick/constitute.h"
#include "magick/delegate.h"
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
#include "magick/resource_.h"
#include "magick/resize.h"
#include "magick/static.h"
#include "magick/string_.h"
#include "magick/transform.h"
#include "magick/utility.h"
#include "magick/version.h"
#if defined(HasTIFF)
#define CCITTParam  "-1"
#else
#define CCITTParam  "0"
#endif

/*
  Forward declarations.
*/
static MagickBooleanType
  WritePDFImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s P D F                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsPDF() returns MagickTrue if the image format type, identified by the
%  magick string, is PDF.
%
%  The format of the IsPDF method is:
%
%      MagickBooleanType IsPDF(const unsigned char *magick,const size_t offset)
%
%  A description of each parameter follows:
%
%    o magick: This string is generally the first few bytes of an image file
%      or blob.
%
%    o offset: Specifies the offset of the magick string.
%
%
*/
static MagickBooleanType IsPDF(const unsigned char *magick,const size_t offset)
{
  if (offset < 5)
    return(MagickFalse);
  if (LocaleNCompare((char *) magick,"%PDF-",5) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d P D F I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadPDFImage() reads a Portable Document Format image file and
%  returns it.  It allocates the memory necessary for the new Image structure
%  and returns a pointer to the new image.
%
%  The format of the ReadPDFImage method is:
%
%      Image *ReadPDFImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadPDFImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
#define CropBox  "CropBox"
#define DeviceCMYK  "DeviceCMYK"
#define MediaBox  "MediaBox"
#define RenderPostscriptText  "  Rendering Postscript...  "

  char
    command[MaxTextExtent],
    density[MaxTextExtent],
    filename[MaxTextExtent],
    geometry[MaxTextExtent],
    options[MaxTextExtent],
    postscript_filename[MaxTextExtent];

  const DelegateInfo
    *delegate_info;

  Image
    *image,
    *next_image;

  ImageInfo
    *read_info;

  int
    file;

  MagickBooleanType
    cmyk,
    status;

  PointInfo
    delta;

  RectangleInfo
    bounding_box,
    page;

  register char
    *p;

  register long
    c;

  SegmentInfo
    bounds;

  ssize_t
    count;

  unsigned long
    height,
    scene,
    width;

  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  /*
    Open image file.
  */
  image=AllocateImage(image_info);
  status=OpenBlob(image_info,image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  /*
    Set the page density.
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
  (void) FormatMagickString(density,MaxTextExtent,"%gx%g",
    image->x_resolution,image->y_resolution);
  /*
    Determine page geometry from the PDF media box.
  */
  cmyk=MagickFalse;
  count=0;
  (void) ResetMagickMemory(&bounding_box,0,sizeof(bounding_box));
  (void) ResetMagickMemory(&bounds,0,sizeof(bounds));
  (void) ResetMagickMemory(&page,0,sizeof(page));
  (void) ResetMagickMemory(command,0,sizeof(command));
  for (p=command; (c=ReadBlobByte(image)) != EOF; )
  {
    if (image_info->page != (char *) NULL)
      continue;
    /*
      Note PDF elements.
    */
    *p++=(char) c;
    if ((c != (int) '/') && ((size_t) (p-command) < (MaxTextExtent/2)))
      continue;
    *p='\0';
    p=command;
    /*
      Is this a CMYK document?
    */
    if (LocaleNCompare(DeviceCMYK,command,strlen(DeviceCMYK)) == 0)
      cmyk=MagickTrue;
    if (LocaleNCompare(CropBox,command,strlen(CropBox)) == 0)
      {
        /*
          Note region defined by crop box.
        */
        count=(ssize_t) sscanf(command,"CropBox [%lf %lf %lf %lf",
          &bounds.x1,&bounds.y1,&bounds.x2,&bounds.y2);
        if (count != 4)
          count=(ssize_t) sscanf(command,"CropBox[%lf %lf %lf %lf",
            &bounds.x1,&bounds.y1,&bounds.x2,&bounds.y2);
      }
    if (LocaleNCompare(MediaBox,command,strlen(MediaBox)) == 0)
      {
        /*
          Note region defined by media box.
        */
        count=(ssize_t) sscanf(command,"MediaBox [%lf %lf %lf %lf",
          &bounds.x1,&bounds.y1,&bounds.x2,&bounds.y2);
        if (count != 4)
          count=(ssize_t) sscanf(command,"MediaBox[%lf %lf %lf %lf",
            &bounds.x1,&bounds.y1,&bounds.x2,&bounds.y2);
      }
    if (count != 4)
      continue;
    /*
      Set PDF render geometry.
    */
    width=(unsigned long) (bounds.x2-bounds.x1+0.5);
    height=(unsigned long) (bounds.y2-bounds.y1+0.5);
    if (width > page.width)
      page.width=width;
    if (height > page.height)
      page.height=height;
  }
  CloseBlob(image);
  /*
    Create Ghostscript control file.
  */
  file=AcquireUniqueFileResource(postscript_filename);
  if (file == -1)
    {
      ThrowFileException(exception,FileOpenError,"UnableToCreateTemporaryFile",
        image_info->filename);
      image=DestroyImage(image);
      return((Image *) NULL);
    }
  (void) write(file," ",1);
  file=close(file)-1;
  /*
    Render Postscript with the Ghostscript delegate.
  */
  if ((page.width == 0) || (page.height == 0))
    (void) ParseAbsoluteGeometry(PSPageGeometry,&page);
  if (image_info->page != (char *) NULL)
    (void) ParseAbsoluteGeometry(image_info->page,&page);
  page.width=(unsigned long)
    (page.width*image->x_resolution/delta.x+0.5);
  page.height=(unsigned long)
    (page.height*image->y_resolution/delta.y+0.5);
  image=DestroyImage(image);
  (void) FormatMagickString(geometry,MaxTextExtent,"%lux%lu",
    page.width,page.height);
  if (image_info->monochrome != MagickFalse)
    delegate_info=GetDelegateInfo("ps:mono",(char *) NULL,exception);
  else
     if (cmyk != MagickFalse)
       delegate_info=GetDelegateInfo("ps:cmyk",(char *) NULL,exception);
     else
       delegate_info=GetDelegateInfo("ps:color",(char *) NULL,exception);
  if (delegate_info == (const DelegateInfo *) NULL)
    {
      (void) RelinquishUniqueFileResource(postscript_filename);
      return((Image *) NULL);
    }
  *options='\0';
  read_info=CloneImageInfo(image_info);
  *read_info->magick='\0';
  if (read_info->number_scenes != 0)
    {
      if (read_info->number_scenes != 1)
        (void) FormatMagickString(options,MaxTextExtent,"-dLastPage=%lu",
          read_info->scene+read_info->number_scenes);
      else
        (void) FormatMagickString(options,MaxTextExtent,
          "-dFirstPage=%lu -dLastPage=%lu",read_info->scene+1,read_info->scene+
          read_info->number_scenes);
      read_info->number_scenes=0;
      if (read_info->scenes != (char *) NULL)
        *read_info->scenes='\0';
    }
  if (read_info->authenticate != (char *) NULL)
    (void) FormatMagickString(options+strlen(options),MaxTextExtent,
      " -sPDFPassword=%s",read_info->authenticate);
  (void) CopyMagickString(filename,read_info->filename,MaxTextExtent);
  (void) AcquireUniqueFilename(read_info->filename);
  (void) FormatMagickString(command,MaxTextExtent,
    GetDelegateCommands(delegate_info),
    read_info->antialias != MagickFalse ? 4 : 1,
    read_info->antialias != MagickFalse ? 4 : 1,geometry,density,options,
    read_info->filename,postscript_filename,image_info->filename);
  status=InvokePostscriptDelegate(read_info->verbose,command);
  image=ReadImage(read_info,exception);
  (void) RelinquishUniqueFileResource(postscript_filename);
  (void) RelinquishUniqueFileResource(read_info->filename);
  read_info=DestroyImageInfo(read_info);
  if (image == (Image *) NULL)
    ThrowReaderException(DelegateError,"PostscriptDelegateFailed");
  if (LocaleCompare(image->magick,"BMP") == 0)
    {
      Image
        *cmyk_image;

      cmyk_image=ConsolidateCMYKImages(image,&image->exception);
      if (cmyk_image != (Image *) NULL)
        {
          image=DestroyImageList(image);
          image=cmyk_image;
        }
    }
  scene=0;
  do
  {
    (void) CopyMagickString(image->filename,filename,MaxTextExtent);
    image->page=page;
    image->scene=scene++;
    next_image=SyncNextImageInList(image);
    if (next_image != (Image *) NULL)
      image=next_image;
  } while (next_image != (Image *) NULL);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r P D F I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterPDFImage() adds attributes for the PDF image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterPDFImage method is:
%
%      RegisterPDFImage(void)
%
*/
ModuleExport void RegisterPDFImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("EPDF");
  entry->decoder=(DecoderHandler *) ReadPDFImage;
  entry->encoder=(EncoderHandler *) WritePDFImage;
  entry->adjoin=MagickFalse;
  entry->blob_support=MagickFalse;
  entry->seekable_stream=MagickTrue;
  entry->description=AcquireString("Encapsulated Portable Document Format");
  entry->module=AcquireString("PDF");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PDF");
  entry->decoder=(DecoderHandler *) ReadPDFImage;
  entry->encoder=(EncoderHandler *) WritePDFImage;
  entry->magick=(MagickHandler *) IsPDF;
  entry->blob_support=MagickFalse;
  entry->seekable_stream=MagickTrue;
  entry->description=AcquireString("Portable Document Format");
  entry->module=AcquireString("PDF");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r P D F I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterPDFImage() removes format registrations made by the
%  PDF module from the list of supported formats.
%
%  The format of the UnregisterPDFImage method is:
%
%      UnregisterPDFImage(void)
%
*/
ModuleExport void UnregisterPDFImage(void)
{
  (void) UnregisterMagickInfo("EPDF");
  (void) UnregisterMagickInfo("PDF");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e P D F I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WritePDFImage() writes an image in the Portable Document image
%  format.
%
%  The format of the WritePDFImage method is:
%
%      MagickBooleanType WritePDFImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/

static char *EscapeParenthesis(const char *text)
{
  register char
    *p;

  register long
    i;

  static char
    buffer[MaxTextExtent];

  unsigned long
    escapes;

  escapes=0;
  p=buffer;
  for (i=0; i < Min((long) strlen(text),(long) (MaxTextExtent-escapes-1)); i++)
  {
    if ((text[i] == '(') || (text[i] == ')'))
      {
        *p++='\\';
        escapes++;
      }
    *p++=text[i];
  }
  *p='\0';
  return(buffer);
}

static MagickBooleanType WritePDFImage(const ImageInfo *image_info,Image *image)
{
#define CFormat  "/Filter [ /%s ]\n"
#define ObjectsPerImage  12

  char
    buffer[MaxTextExtent],
    date[MaxTextExtent],
    **labels,
    page_geometry[MaxTextExtent];

  CompressionType
    compression;

  const ImageAttribute
    *attribute;

  GeometryInfo
    geometry_info;

  long
    count,
    y;

  Image
    *tile_image;

  MagickBooleanType
    matte,
    status;

  MagickOffsetType
    offset,
    scene,
    *xref;

  MagickSizeType
    number_pixels;

  MagickStatusType
    flags;

  PointInfo
    delta,
    resolution,
    scale;

  RectangleInfo
    geometry,
    media_info,
    page_info;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register unsigned char
    *q;

  register long
    i,
    x;

  size_t
    length;

  struct tm
    *time_meridian;

  time_t
    seconds;

  unsigned char
    *pixels;

  unsigned long
    info_id,
    object,
    pages_id,
    quality,
    root_id,
    text_size;

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
  compression=image->compression;
  quality=(unsigned long)
    (image->quality == UndefinedCompressionQuality ? 75 : image->quality);
  switch (compression)
  {
#if !defined(HasJPEG)
    case JPEGCompression:
    {
      compression=RLECompression;
      (void) ThrowMagickException(&image->exception,GetMagickModule(),
        MissingDelegateError,"JPEGLibraryIsNotAvailable","`%s'",
        image->filename);
      break;
    }
#endif
#if !defined(HasJP2)
    case JPEG2000Compression:
    {
      compression=RLECompression;
      (void) ThrowMagickException(&image->exception,GetMagickModule(),
        MissingDelegateError,"JP2LibraryIsNotAvailable","`%s'",image->filename);
      break;
    }
#endif
#if !defined(HasZLIB)
    case ZipCompression:
    {
      compression=RLECompression;
      (void) ThrowMagickException(&image->exception,GetMagickModule(),
        MissingDelegateError,"ZipLibraryIsNotAvailable","`%s'",image->filename);
      break;
    }
#endif
    default:
      break;
  }
  /*
    Allocate X ref memory.
  */
  xref=(MagickOffsetType *) AcquireMagickMemory(2048*sizeof(*xref));
  if (xref == (MagickOffsetType *) NULL)
    ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
  (void) ResetMagickMemory(xref,0,2048*sizeof(*xref));
  /*
    Write Info object.
  */
  object=0;
  if (compression == JPEG2000Compression)
    (void) WriteBlobString(image,"%PDF-1.5 \n");
  else
    if (image->matte != MagickFalse)
      (void) WriteBlobString(image,"%PDF-1.4 \n");
    else
      (void) WriteBlobString(image,"%PDF-1.3 \n");
  xref[object++]=TellBlob(image);
  info_id=object;
  (void) FormatMagickString(buffer,MaxTextExtent,"%lu 0 obj\n",object);
  (void) WriteBlobString(image,buffer);
  (void) WriteBlobString(image,"<<\n");
  (void) FormatMagickString(buffer,MaxTextExtent,"/Title (%s)\n",
    EscapeParenthesis(image->filename));
  (void) WriteBlobString(image,buffer);
  seconds=time((time_t *) NULL);
  time_meridian=localtime(&seconds);
  (void) FormatMagickString(date,MaxTextExtent,"D:%04d%02d%02d%02d%02d%02d",
    time_meridian->tm_year+1900,time_meridian->tm_mon+1,time_meridian->tm_mday,
    time_meridian->tm_hour,time_meridian->tm_min,time_meridian->tm_sec);
  (void) FormatMagickString(buffer,MaxTextExtent,"/CreationDate (%s)\n",date);
  (void) WriteBlobString(image,buffer);
  (void) FormatMagickString(buffer,MaxTextExtent,"/ModDate (%s)\n",date);
  (void) WriteBlobString(image,buffer);
  (void) FormatMagickString(buffer,MaxTextExtent,"/Producer (%s)\n",
    EscapeParenthesis(GetMagickVersion((unsigned long *) NULL)));
  (void) WriteBlobString(image,buffer);
  (void) WriteBlobString(image,">>\n");
  (void) WriteBlobString(image,"endobj\n");
  /*
    Write Catalog object.
  */
  xref[object++]=TellBlob(image);
  root_id=object;
  (void) FormatMagickString(buffer,MaxTextExtent,"%lu 0 obj\n",object);
  (void) WriteBlobString(image,buffer);
  (void) WriteBlobString(image,"<<\n");
  (void) WriteBlobString(image,"/Type /Catalog\n");
  (void) FormatMagickString(buffer,MaxTextExtent,"/Pages %lu 0 R\n",object+1);
  (void) WriteBlobString(image,buffer);
  (void) WriteBlobString(image,">>\n");
  (void) WriteBlobString(image,"endobj\n");
  /*
    Write Pages object.
  */
  xref[object++]=TellBlob(image);
  pages_id=object;
  (void) FormatMagickString(buffer,MaxTextExtent,"%lu 0 obj\n",object);
  (void) WriteBlobString(image,buffer);
  (void) WriteBlobString(image,"<<\n");
  (void) WriteBlobString(image,"/Type /Pages\n");
  (void) FormatMagickString(buffer,MaxTextExtent,"/Kids [ %lu 0 R ",object+1);
  (void) WriteBlobString(image,buffer);
  count=(long) (pages_id+ObjectsPerImage+1);
  if (image_info->adjoin != MagickFalse)
    {
      Image
        *kid_image;

      /*
        Predict page object id's.
      */
      kid_image=image;
      for ( ; GetNextImageInList(kid_image) != (Image *) NULL; count+=ObjectsPerImage)
      {
        (void) FormatMagickString(buffer,MaxTextExtent,"%ld 0 R ",count);
        (void) WriteBlobString(image,buffer);
        kid_image=GetNextImageInList(kid_image);
      }
      xref=(MagickOffsetType *)
        ResizeMagickMemory(xref,(size_t) (count+2048)*sizeof(*xref));
      if (xref == (MagickOffsetType *) NULL)
        ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    }
  (void) WriteBlobString(image,"]\n");
  (void) FormatMagickString(buffer,MaxTextExtent,"/Count %lu\n",
    (count-pages_id)/ObjectsPerImage);
  (void) WriteBlobString(image,buffer);
  (void) WriteBlobString(image,">>\n");
  (void) WriteBlobString(image,"endobj\n");
  scene=0;
  do
  {
    /*
      Scale relative to dots-per-inch.
    */
    delta.x=DefaultResolution;
    delta.y=DefaultResolution;
    flags=ParseGeometry(PSDensityGeometry,&geometry_info);
    resolution.x=geometry_info.rho;
    resolution.y=geometry_info.sigma;
    if ((flags & SigmaValue) == 0)
      resolution.y=resolution.x;
    if (image->x_resolution != 0.0)
      resolution.x=image->x_resolution;
    if (image->y_resolution != 0.0)
      resolution.y=image->y_resolution;
    if (image_info->density != (char *) NULL)
      {
        flags=ParseGeometry(image_info->density,&geometry_info);
        resolution.x=geometry_info.rho;
        resolution.y=geometry_info.sigma;
        if ((flags & SigmaValue) == 0)
          resolution.y=resolution.x;
      }
    if (image->units == PixelsPerCentimeterResolution)
      {
        resolution.x*=2.54;
        resolution.y*=2.54;
      }
    SetGeometry(image,&geometry);
    (void) FormatMagickString(page_geometry,MaxTextExtent,"%lux%lu",
      image->columns,image->rows);
    if (image_info->page != (char *) NULL)
      (void) CopyMagickString(page_geometry,image_info->page,MaxTextExtent);
    else
      if ((image->page.width != 0) && (image->page.height != 0))
        (void) FormatMagickString(page_geometry,MaxTextExtent,"%lux%lu%+ld%+ld",
          image->page.width,image->page.height,image->page.x,image->page.y);
      else
        if ((image->gravity != UndefinedGravity) &&
            (LocaleCompare(image_info->magick,"PDF") == 0))
          (void) strcpy(page_geometry,PSPageGeometry);
    (void) ParseMetaGeometry(page_geometry,&geometry.x,&geometry.y,
      &geometry.width,&geometry.height);
    scale.x=(double) (geometry.width*delta.x)/resolution.x;
    geometry.width=(unsigned long) (scale.x+0.5);
    scale.y=(double) (geometry.height*delta.y)/resolution.y;
    geometry.height=(unsigned long) (scale.y+0.5);
    (void) ParseAbsoluteGeometry(page_geometry,&media_info);
    (void) ParseGravityGeometry(image,page_geometry,&page_info);
    if (image->gravity != UndefinedGravity)
      {
        geometry.x=(-page_info.x);
        geometry.y=(long) (media_info.height+page_info.y-image->rows);
      }
    text_size=0;
    attribute=GetImageAttribute(image,"Label");
    if (attribute != (const ImageAttribute *) NULL)
      text_size=(unsigned long)
        (MultilineCensus(attribute->value)*image_info->pointsize+12);
    /*
      Write Page object.
    */
    xref[object++]=TellBlob(image);
    (void) FormatMagickString(buffer,MaxTextExtent,"%lu 0 obj\n",object);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"<<\n");
    (void) WriteBlobString(image,"/Type /Page\n");
    (void) FormatMagickString(buffer,MaxTextExtent,"/Parent %lu 0 R\n",
      pages_id);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"/Resources <<\n");
    (void) FormatMagickString(buffer,MaxTextExtent,
      "/Font << /F%lu %lu 0 R >>\n",image->scene,object+4);
    (void) WriteBlobString(image,buffer);
    matte=image->matte;
    if (IsGrayImage(image,&image->exception) != MagickFalse)
      matte=MagickFalse;
    if (compression == JPEG2000Compression)
      {
	      (void) SetImageColorspace(image,RGBColorspace);
        matte=MagickFalse;
      }
    (void) FormatMagickString(buffer,MaxTextExtent,
      "/XObject << /Im%lu %lu 0 R >>\n",image->scene,object+
      (matte != MagickFalse ? 7 : 5));
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,"/ProcSet %lu 0 R >>\n",
      object+3);
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,"/MediaBox [0 0 %lu %lu]\n",
      media_info.width,media_info.height);
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,
      "/CropBox [%ld %ld %lu %lu]\n",geometry.x,geometry.y,geometry.x+
      geometry.width,geometry.y+geometry.height);
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,"/Contents %lu 0 R\n",
      object+1);
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,"/Thumb %lu 0 R\n",
      object+(matte != MagickFalse ? 10 : 8));
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,">>\n");
    (void) WriteBlobString(image,"endobj\n");
    /*
      Write Contents object.
    */
    xref[object++]=TellBlob(image);
    (void) FormatMagickString(buffer,MaxTextExtent,"%lu 0 obj\n",object);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"<<\n");
    (void) FormatMagickString(buffer,MaxTextExtent,"/Length %lu 0 R\n",
      object+1);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,">>\n");
    (void) WriteBlobString(image,"stream\n");
    offset=TellBlob(image);
    (void) WriteBlobString(image,"q\n");
    labels=(char **) NULL;
    attribute=GetImageAttribute(image,"Label");
    if (attribute != (const ImageAttribute *) NULL)
      labels=StringToList(attribute->value);
    if (labels != (char **) NULL)
      {
        for (i=0; labels[i] != (char *) NULL; i++)
        {
          (void) WriteBlobString(image,"BT\n");
          (void) FormatMagickString(buffer,MaxTextExtent,"/F%lu %g Tf\n",
            image->scene,image_info->pointsize);
          (void) WriteBlobString(image,buffer);
          (void) FormatMagickString(buffer,MaxTextExtent,"%ld %ld Td\n",
            geometry.x,(long) (geometry.y+geometry.height+i*
            image_info->pointsize+12));
          (void) WriteBlobString(image,buffer);
          (void) FormatMagickString(buffer,MaxTextExtent,"(%s) Tj\n",
            labels[i]);
          (void) WriteBlobString(image,buffer);
          (void) WriteBlobString(image,"ET\n");
          labels[i]=(char *) RelinquishMagickMemory(labels[i]);
        }
        labels=(char **) RelinquishMagickMemory(labels);
      }
    (void) FormatMagickString(buffer,MaxTextExtent,"%g 0 0 %g %ld %ld cm\n",
      scale.x,scale.y,geometry.x,geometry.y);
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,"/Im%lu Do\n",image->scene);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"Q\n");
    offset=TellBlob(image)-offset;
    (void) WriteBlobString(image,"endstream\n");
    (void) WriteBlobString(image,"endobj\n");
    /*
      Write Length object.
    */
    xref[object++]=TellBlob(image);
    (void) FormatMagickString(buffer,MaxTextExtent,"%lu 0 obj\n",object);
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,"%lu\n",
      (unsigned long) offset);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"endobj\n");
    /*
      Write Procset object.
    */
    xref[object++]=TellBlob(image);
    (void) FormatMagickString(buffer,MaxTextExtent,"%lu 0 obj\n",object);
    (void) WriteBlobString(image,buffer);
    if ((image->storage_class == DirectClass) || (image->colors > 256))
      (void) strcpy(buffer,"[ /PDF /Text /ImageC");
    else
      if (compression == FaxCompression)
        (void) strcpy(buffer,"[ /PDF /Text /ImageB");
      else
        (void) strcpy(buffer,"[ /PDF /Text /ImageI");
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image," ]\n");
    (void) WriteBlobString(image,"endobj\n");
    /*
      Write Font object.
    */
    xref[object++]=TellBlob(image);
    (void) FormatMagickString(buffer,MaxTextExtent,"%lu 0 obj\n",object);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"<<\n");
    (void) WriteBlobString(image,"/Type /Font\n");
    (void) WriteBlobString(image,"/Subtype /Type1\n");
    (void) FormatMagickString(buffer,MaxTextExtent,"/Name /F%lu\n",
      image->scene);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"/BaseFont /Helvetica\n");
    (void) WriteBlobString(image,"/Encoding /MacRomanEncoding\n");
    (void) WriteBlobString(image,">>\n");
    (void) WriteBlobString(image,"endobj\n");
    if (matte != MagickFalse)
      {
        /*
          Write softmask object.
        */
        xref[object++]=TellBlob(image);
        (void) FormatMagickString(buffer,MaxTextExtent,"%lu 0 obj\n",object);
        (void) WriteBlobString(image,buffer);
        (void) WriteBlobString(image,"<<\n");
        (void) WriteBlobString(image,"/Type /XObject\n");
        (void) WriteBlobString(image,"/Subtype /Image\n");
        (void) FormatMagickString(buffer,MaxTextExtent,"/Name /Ma%lu\n",
          image->scene);
        (void) WriteBlobString(image,buffer);
        switch (compression)
        {
          case NoCompression:
          {
            (void) FormatMagickString(buffer,MaxTextExtent,CFormat,
              "ASCII85Decode");
            break;
          }
          case LZWCompression:
          {
            (void) FormatMagickString(buffer,MaxTextExtent,CFormat,"LZWDecode");
            break;
          }
          case ZipCompression:
          {
            (void) FormatMagickString(buffer,MaxTextExtent,CFormat,
              "FlateDecode");
            break;
          }
          default:
          {
            (void) FormatMagickString(buffer,MaxTextExtent,CFormat,
              "RunLengthDecode");
            break;
          }
        }
        (void) WriteBlobString(image,buffer);
        (void) FormatMagickString(buffer,MaxTextExtent,"/Width %lu\n",
          image->columns);
        (void) WriteBlobString(image,buffer);
        (void) FormatMagickString(buffer,MaxTextExtent,"/Height %lu\n",
          image->rows);
        (void) WriteBlobString(image,buffer);
        (void) WriteBlobString(image,"/ColorSpace /DeviceGray\n");
        (void) FormatMagickString(buffer,MaxTextExtent,"/BitsPerComponent %d\n",
          compression == FaxCompression ? 1 : 8);
        (void) WriteBlobString(image,buffer);
        (void) FormatMagickString(buffer,MaxTextExtent,"/Length %lu 0 R\n",
          object+1);
        (void) WriteBlobString(image,buffer);
        (void) WriteBlobString(image,">>\n");
        (void) WriteBlobString(image,"stream\n");
        offset=TellBlob(image);
        number_pixels=(MagickSizeType) image->columns*image->rows;
        switch (compression)
        {
          case RLECompression:
          default:
          {
            /*
              Allocate pixel array.
            */
            length=(size_t) number_pixels;
            pixels=(unsigned char *) AcquireMagickMemory(length);
            if (pixels == (unsigned char *) NULL)
              {
                image=DestroyImage(image);
                ThrowWriterException(ResourceLimitError,
                  "MemoryAllocationFailed");
              }
            /*
              Dump Runlength encoded pixels.
            */
            q=pixels;
            for (y=0; y < (long) image->rows; y++)
            {
              p=AcquireImagePixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              for (x=0; x < (long) image->columns; x++)
              {
                *q++=ScaleQuantumToChar(QuantumRange-p->opacity);
                p++;
              }
            }
#if defined(HasZLIB)
            if (compression == ZipCompression)
              status=ZLIBEncodeImage(image,length,quality,pixels);
            else
#endif
              if (compression == LZWCompression)
                status=LZWEncodeImage(image,length,pixels);
              else
                status=PackbitsEncodeImage(image,length,pixels);
            pixels=(unsigned char *) RelinquishMagickMemory(pixels);
            if (status == MagickFalse)
              {
                CloseBlob(image);
                return(MagickFalse);
              }
            break;
          }
          case NoCompression:
          {
            /*
              Dump uncompressed PseudoColor packets.
            */
            Ascii85Initialize(image);
            for (y=0; y < (long) image->rows; y++)
            {
              p=AcquireImagePixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              for (x=0; x < (long) image->columns; x++)
              {
                Ascii85Encode(image,ScaleQuantumToChar(QuantumRange-p->opacity));
                p++;
              }
            }
            Ascii85Flush(image);
            break;
          }
        }
        offset=TellBlob(image)-offset;
        (void) WriteBlobString(image,"\nendstream\n");
        (void) WriteBlobString(image,"endobj\n");
        /*
          Write Length object.
        */
        xref[object++]=TellBlob(image);
        (void) FormatMagickString(buffer,MaxTextExtent,"%lu 0 obj\n",object);
        (void) WriteBlobString(image,buffer);
        (void) FormatMagickString(buffer,MaxTextExtent,"%lu\n",
          (unsigned long) offset);
        (void) WriteBlobString(image,buffer);
        (void) WriteBlobString(image,"endobj\n");
      }
    /*
      Write XObject object.
    */
    xref[object++]=TellBlob(image);
    (void) FormatMagickString(buffer,MaxTextExtent,"%lu 0 obj\n",object);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"<<\n");
    (void) WriteBlobString(image,"/Type /XObject\n");
    (void) WriteBlobString(image,"/Subtype /Image\n");
    (void) FormatMagickString(buffer,MaxTextExtent,"/Name /Im%lu\n",
      image->scene);
    (void) WriteBlobString(image,buffer);
    switch (compression)
    {
      case NoCompression:
      {
        (void) FormatMagickString(buffer,MaxTextExtent,CFormat,"ASCII85Decode");
        break;
      }
      case JPEGCompression:
      {
        (void) FormatMagickString(buffer,MaxTextExtent,CFormat,"DCTDecode");
        if (image->colorspace != CMYKColorspace)
          break;
        (void) WriteBlobString(image,buffer);
        (void) strcpy(buffer,"/Decode [1 0 1 0 1 0 1 0]\n");
        break;
      }
      case JPEG2000Compression:
      {
        (void) FormatMagickString(buffer,MaxTextExtent,CFormat,"JPXDecode");
        if (image->colorspace != CMYKColorspace)
          break;
        (void) WriteBlobString(image,buffer);
        (void) strcpy(buffer,"/Decode [1 0 1 0 1 0 1 0]\n");
        break;
      }
      case LZWCompression:
      {
         (void) FormatMagickString(buffer,MaxTextExtent,CFormat,"LZWDecode");
         break;
      }
      case ZipCompression:
      {
         (void) FormatMagickString(buffer,MaxTextExtent,CFormat,"FlateDecode");
         break;
      }
      case FaxCompression:
      {
        (void) strcpy(buffer,"/Filter [ /CCITTFaxDecode ]\n");
        (void) WriteBlobString(image,buffer);
        (void) FormatMagickString(buffer,MaxTextExtent,
          "/DecodeParms [ << >> << /K %s /Columns %ld /Rows %ld >> ]\n",
          CCITTParam,image->columns,image->rows);
        break;
      }
      default:
      {
        (void) FormatMagickString(buffer,MaxTextExtent,CFormat,
          "RunLengthDecode");
        break;
      }
    }
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,"/Width %lu\n",
      image->columns);
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,"/Height %lu\n",image->rows);
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,"/ColorSpace %lu 0 R\n",
      object+2);
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,"/BitsPerComponent %d\n",
      compression == FaxCompression ? 1 : 8);
    (void) WriteBlobString(image,buffer);
    if (matte != MagickFalse)
      {
        (void) FormatMagickString(buffer,MaxTextExtent,"/SMask %lu 0 R\n",
          object-2);
        (void) WriteBlobString(image,buffer);
      }
    (void) FormatMagickString(buffer,MaxTextExtent,"/Length %lu 0 R\n",
      object+1);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,">>\n");
    (void) WriteBlobString(image,"stream\n");
    offset=TellBlob(image);
    number_pixels=(MagickSizeType) image->columns*image->rows;
    if ((4*number_pixels) != (MagickSizeType) ((size_t) (4*number_pixels)))
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    if ((compression == FaxCompression) ||
        ((image_info->type != TrueColorType) &&
         (IsGrayImage(image,&image->exception) != MagickFalse)))

      {
        switch (compression)
        {
          case FaxCompression:
          {
            if (LocaleCompare(CCITTParam,"0") == 0)
              {
                (void) HuffmanEncodeImage(image_info,image);
                break;
              }
            (void) Huffman2DEncodeImage(image_info,image);
            break;
          }
          case JPEGCompression:
          {
            status=JPEGEncodeImage(image_info,image);
            if (status == MagickFalse)
              ThrowWriterException(CoderError,image->exception.reason);
            break;
          }
          case JPEG2000Compression:
          {
            status=JP2EncodeImage(image_info,image);
            if (status == MagickFalse)
              ThrowWriterException(CoderError,image->exception.reason);
            break;
          }
          case RLECompression:
          default:
          {
            /*
              Allocate pixel array.
            */
            length=(size_t) number_pixels;
            pixels=(unsigned char *) AcquireMagickMemory(length);
            if (pixels == (unsigned char *) NULL)
              ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
            /*
              Dump Runlength encoded pixels.
            */
            q=pixels;
            for (y=0; y < (long) image->rows; y++)
            {
              p=AcquireImagePixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              for (x=0; x < (long) image->columns; x++)
              {
                *q++=ScaleQuantumToChar(PixelIntensityToQuantum(p));
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
#if defined(HasZLIB)
            if (compression == ZipCompression)
              status=ZLIBEncodeImage(image,length,quality,pixels);
            else
#endif
              if (compression == LZWCompression)
                status=LZWEncodeImage(image,length,pixels);
              else
                status=PackbitsEncodeImage(image,length,pixels);
            pixels=(unsigned char *) RelinquishMagickMemory(pixels);
            if (status == MagickFalse)
              {
                CloseBlob(image);
                return(MagickFalse);
              }
            break;
          }
          case NoCompression:
          {
            /*
              Dump uncompressed PseudoColor packets.
            */
            Ascii85Initialize(image);
            for (y=0; y < (long) image->rows; y++)
            {
              p=AcquireImagePixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              for (x=0; x < (long) image->columns; x++)
              {
                Ascii85Encode(image,
                  ScaleQuantumToChar(PixelIntensityToQuantum(p)));
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
            Ascii85Flush(image);
            break;
          }
        }
      }
    else
      if ((image->storage_class == DirectClass) || (image->colors > 256) ||
          (compression == JPEGCompression) ||
          (compression == JPEG2000Compression))
        switch (compression)
        {
          case JPEGCompression:
          {
            status=JPEGEncodeImage(image_info,image);
            if (status == MagickFalse)
              ThrowWriterException(CoderError,image->exception.reason);
            break;
          }
          case JPEG2000Compression:
          {
            status=JP2EncodeImage(image_info,image);
            if (status == MagickFalse)
              ThrowWriterException(CoderError,image->exception.reason);
            break;
          }
          case RLECompression:
          default:
          {
            /*
              Allocate pixel array.
            */
            length=(size_t) ((image->colorspace == CMYKColorspace ? 4 : 3)*
              number_pixels);
            pixels=(unsigned char *) AcquireMagickMemory(length);
            if (pixels == (unsigned char *) NULL)
              ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
            /*
              Dump runoffset encoded pixels.
            */
            q=pixels;
            for (y=0; y < (long) image->rows; y++)
            {
              p=AcquireImagePixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              indexes=GetIndexes(image);
              for (x=0; x < (long) image->columns; x++)
              {
                *q++=ScaleQuantumToChar(p->red);
                *q++=ScaleQuantumToChar(p->green);
                *q++=ScaleQuantumToChar(p->blue);
                if (image->colorspace == CMYKColorspace)
                  *q++=ScaleQuantumToChar(indexes[x]);
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
#if defined(HasZLIB)
            if (compression == ZipCompression)
              status=ZLIBEncodeImage(image,length,quality,pixels);
            else
#endif
              if (compression == LZWCompression)
                status=LZWEncodeImage(image,length,pixels);
              else
                status=PackbitsEncodeImage(image,length,pixels);
            pixels=(unsigned char *) RelinquishMagickMemory(pixels);
            if (status == MagickFalse)
              {
                CloseBlob(image);
                return(MagickFalse);
              }
            break;
          }
          case NoCompression:
          {
            /*
              Dump uncompressed DirectColor packets.
            */
            Ascii85Initialize(image);
            for (y=0; y < (long) image->rows; y++)
            {
              p=AcquireImagePixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              indexes=GetIndexes(image);
              for (x=0; x < (long) image->columns; x++)
              {
                Ascii85Encode(image,ScaleQuantumToChar(p->red));
                Ascii85Encode(image,ScaleQuantumToChar(p->green));
                Ascii85Encode(image,ScaleQuantumToChar(p->blue));
                if (image->colorspace == CMYKColorspace)
                  Ascii85Encode(image,ScaleQuantumToChar(indexes[x]));
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
            Ascii85Flush(image);
            break;
          }
        }
      else
        {
          /*
            Dump number of colors and colormap.
          */
          switch (compression)
          {
            case RLECompression:
            default:
            {
              /*
                Allocate pixel array.
              */
              length=(size_t) number_pixels;
              pixels=(unsigned char *) AcquireMagickMemory(length);
              if (pixels == (unsigned char *) NULL)
                ThrowWriterException(ResourceLimitError,
                  "MemoryAllocationFailed");
              /*
                Dump Runlength encoded pixels.
              */
              q=pixels;
              for (y=0; y < (long) image->rows; y++)
              {
                p=AcquireImagePixels(image,0,y,image->columns,1,
                  &image->exception);
                if (p == (const PixelPacket *) NULL)
                  break;
                indexes=GetIndexes(image);
                for (x=0; x < (long) image->columns; x++)
                  *q++=(unsigned char) indexes[x];
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
#if defined(HasZLIB)
              if (compression == ZipCompression)
                status=ZLIBEncodeImage(image,length,quality,pixels);
              else
#endif
                if (compression == LZWCompression)
                  status=LZWEncodeImage(image,length,pixels);
                else
                  status=PackbitsEncodeImage(image,length,pixels);
              pixels=(unsigned char *) RelinquishMagickMemory(pixels);
              if (status == MagickFalse)
                {
                  CloseBlob(image);
                  return(MagickFalse);
                }
              break;
            }
            case NoCompression:
            {
              /*
                Dump uncompressed PseudoColor packets.
              */
              Ascii85Initialize(image);
              for (y=0; y < (long) image->rows; y++)
              {
                p=AcquireImagePixels(image,0,y,image->columns,1,
                  &image->exception);
                if (p == (const PixelPacket *) NULL)
                  break;
                indexes=GetIndexes(image);
                for (x=0; x < (long) image->columns; x++)
                  Ascii85Encode(image,(unsigned char) indexes[x]);
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
              Ascii85Flush(image);
              break;
            }
          }
        }
    offset=TellBlob(image)-offset;
    (void) WriteBlobString(image,"\nendstream\n");
    (void) WriteBlobString(image,"endobj\n");
    /*
      Write Length object.
    */
    xref[object++]=TellBlob(image);
    (void) FormatMagickString(buffer,MaxTextExtent,"%lu 0 obj\n",object);
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,"%lu\n",
      (unsigned long) offset);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"endobj\n");
    /*
      Write Colorspace object.
    */
    xref[object++]=TellBlob(image);
    (void) FormatMagickString(buffer,MaxTextExtent,"%lu 0 obj\n",object);
    (void) WriteBlobString(image,buffer);
    if (image->colorspace == CMYKColorspace)
      (void) strcpy(buffer,"/DeviceCMYK\n");
    else
      if ((compression == FaxCompression) ||
          ((image_info->type != TrueColorType) &&
           (IsGrayImage(image,&image->exception) != MagickFalse)))
          (void) strcpy(buffer,"/DeviceGray\n");
      else
        if ((image->storage_class == DirectClass) || (image->colors > 256) ||
            (compression == JPEGCompression) ||
            (compression == JPEG2000Compression))
          (void) strcpy(buffer,"/DeviceRGB\n");
        else
          (void) FormatMagickString(buffer,MaxTextExtent,
            "[ /Indexed /DeviceRGB %lu %lu 0 R ]\n",
            image->colors-1,object+3);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"endobj\n");
    /*
      Write Thumb object.
    */
    SetGeometry(image,&geometry);
    (void) ParseMetaGeometry("106x106+0+0>",&geometry.x,&geometry.y,
      &geometry.width,&geometry.height);
    tile_image=ResizeImage(image,geometry.width,geometry.height,TriangleFilter,
      1.0,&image->exception);
    if (tile_image == (Image *) NULL)
      ThrowWriterException(ResourceLimitError,image->exception.reason);
    xref[object++]=TellBlob(image);
    (void) FormatMagickString(buffer,MaxTextExtent,"%lu 0 obj\n",object);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"<<\n");
    switch (compression)
    {
      case NoCompression:
      {
        (void) FormatMagickString(buffer,MaxTextExtent,CFormat,"ASCII85Decode");
        break;
      }
      case JPEGCompression:
      {
        (void) FormatMagickString(buffer,MaxTextExtent,CFormat,"DCTDecode");
        if (image->colorspace != CMYKColorspace)
          break;
        (void) WriteBlobString(image,buffer);
        (void) strcpy(buffer,"/Decode [1 0 1 0 1 0 1 0]\n");
        break;
      }
      case JPEG2000Compression:
      {
        (void) FormatMagickString(buffer,MaxTextExtent,CFormat,"JPXDecode");
        if (image->colorspace != CMYKColorspace)
          break;
        (void) WriteBlobString(image,buffer);
        (void) strcpy(buffer,"/Decode [1 0 1 0 1 0 1 0]\n");
        break;
      }
      case LZWCompression:
      {
        (void) FormatMagickString(buffer,MaxTextExtent,CFormat,"LZWDecode");
        break;
      }
      case ZipCompression:
      {
        (void) FormatMagickString(buffer,MaxTextExtent,CFormat,"FlateDecode");
        break;
      }
      case FaxCompression:
      {
        (void) strcpy(buffer,"/Filter [ /CCITTFaxDecode ]\n");
        (void) WriteBlobString(image,buffer);
        (void) FormatMagickString(buffer,MaxTextExtent,
          "/DecodeParms [ << >> << /K %s /Columns %lu /Rows %lu >> ]\n",
          CCITTParam,image->columns,image->rows);
        break;
      }
      default:
      {
        (void) FormatMagickString(buffer,MaxTextExtent,CFormat,
          "RunLengthDecode");
        break;
      }
    }
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,"/Width %lu\n",
      tile_image->columns);
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,"/Height %lu\n",
      tile_image->rows);
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,"/ColorSpace %lu 0 R\n",
      object-1);
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,"/BitsPerComponent %d\n",
      compression == FaxCompression ? 1 : 8);
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,"/Length %lu 0 R\n",
      object+1);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,">>\n");
    (void) WriteBlobString(image,"stream\n");
    offset=TellBlob(image);
    number_pixels=(MagickSizeType) tile_image->columns*tile_image->rows;
    if ((compression == FaxCompression) ||
        ((image_info->type != TrueColorType) &&
         (IsGrayImage(tile_image,&image->exception) != MagickFalse)))
      {
        switch (compression)
        {
          case FaxCompression:
          {
            if (LocaleCompare(CCITTParam,"0") == 0)
              (void) HuffmanEncodeImage(image_info,tile_image);
            else
              (void) Huffman2DEncodeImage(image_info,tile_image);
            break;
          }
          case JPEGCompression:
          {
            status=JPEGEncodeImage(image_info,tile_image);
            if (status == MagickFalse)
              ThrowWriterException(CoderError,tile_image->exception.reason);
            break;
          }
          case JPEG2000Compression:
          {
            status=JP2EncodeImage(image_info,tile_image);
            if (status == MagickFalse)
              ThrowWriterException(CoderError,tile_image->exception.reason);
            break;
          }
          case RLECompression:
          default:
          {
            /*
              Allocate pixel array.
            */
            length=(size_t) number_pixels;
            pixels=(unsigned char *) AcquireMagickMemory(length);
            if (pixels == (unsigned char *) NULL)
              {
                tile_image=DestroyImage(tile_image);
                ThrowWriterException(ResourceLimitError,
                  "MemoryAllocationFailed");
              }
            /*
              Dump Runlength encoded pixels.
            */
            q=pixels;
            for (y=0; y < (long) tile_image->rows; y++)
            {
              p=AcquireImagePixels(tile_image,0,y,tile_image->columns,1,
                &tile_image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              for (x=0; x < (long) tile_image->columns; x++)
              {
                *q++=ScaleQuantumToChar(PixelIntensityToQuantum(p));
                p++;
              }
            }
#if defined(HasZLIB)
            if (compression == ZipCompression)
              status=ZLIBEncodeImage(image,length,quality,pixels);
            else
#endif
              if (compression == LZWCompression)
                status=LZWEncodeImage(image,length,pixels);
              else
                status=PackbitsEncodeImage(image,length,pixels);
            pixels=(unsigned char *) RelinquishMagickMemory(pixels);
            if (status == MagickFalse)
              {
                CloseBlob(image);
                return(MagickFalse);
              }
            break;
          }
          case NoCompression:
          {
            /*
              Dump uncompressed PseudoColor packets.
            */
            Ascii85Initialize(image);
            for (y=0; y < (long) tile_image->rows; y++)
            {
              p=AcquireImagePixels(tile_image,0,y,tile_image->columns,1,
                &tile_image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              for (x=0; x < (long) tile_image->columns; x++)
              {
                Ascii85Encode(image,
                  ScaleQuantumToChar(PixelIntensityToQuantum(p)));
                p++;
              }
            }
            Ascii85Flush(image);
            break;
          }
        }
      }
    else
      if ((tile_image->storage_class == DirectClass) ||
          (tile_image->colors > 256) || (compression == JPEGCompression) ||
          (compression == JPEG2000Compression))
        switch (compression)
        {
          case JPEGCompression:
          {
            status=JPEGEncodeImage(image_info,tile_image);
            if (status == MagickFalse)
              ThrowWriterException(CoderError,tile_image->exception.reason);
            break;
          }
          case JPEG2000Compression:
          {
            status=JP2EncodeImage(image_info,tile_image);
            if (status == MagickFalse)
              ThrowWriterException(CoderError,tile_image->exception.reason);
            break;
          }
          case RLECompression:
          default:
          {
            /*
              Allocate pixel array.
            */
            length=(size_t) ((tile_image->colorspace == CMYKColorspace ? 4 : 3)*
              number_pixels);
            pixels=(unsigned char *) AcquireMagickMemory(length);
            if (pixels == (unsigned char *) NULL)
              {
                tile_image=DestroyImage(tile_image);
                ThrowWriterException(ResourceLimitError,
                  "MemoryAllocationFailed");
              }
            /*
              Dump runoffset encoded pixels.
            */
            q=pixels;
            for (y=0; y < (long) tile_image->rows; y++)
            {
              p=AcquireImagePixels(tile_image,0,y,tile_image->columns,1,
                &tile_image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              indexes=GetIndexes(tile_image);
              for (x=0; x < (long) tile_image->columns; x++)
              {
                *q++=ScaleQuantumToChar(p->red);
                *q++=ScaleQuantumToChar(p->green);
                *q++=ScaleQuantumToChar(p->blue);
                if (image->colorspace == CMYKColorspace)
                  *q++=ScaleQuantumToChar(indexes[x]);
                p++;
              }
            }
#if defined(HasZLIB)
            if (compression == ZipCompression)
              status=ZLIBEncodeImage(image,length,quality,pixels);
            else
#endif
              if (compression == LZWCompression)
                status=LZWEncodeImage(image,length,pixels);
              else
                status=PackbitsEncodeImage(image,length,pixels);
            pixels=(unsigned char *) RelinquishMagickMemory(pixels);
            if (status == MagickFalse)
              {
                CloseBlob(image);
                return(MagickFalse);
              }
            break;
          }
          case NoCompression:
          {
            /*
              Dump uncompressed DirectColor packets.
            */
            Ascii85Initialize(image);
            for (y=0; y < (long) tile_image->rows; y++)
            {
              p=AcquireImagePixels(tile_image,0,y,tile_image->columns,1,
                &tile_image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              indexes=GetIndexes(tile_image);
              for (x=0; x < (long) tile_image->columns; x++)
              {
                Ascii85Encode(image,ScaleQuantumToChar(p->red));
                Ascii85Encode(image,ScaleQuantumToChar(p->green));
                Ascii85Encode(image,ScaleQuantumToChar(p->blue));
                if (image->colorspace == CMYKColorspace)
                  Ascii85Encode(image,ScaleQuantumToChar(indexes[x]));
                p++;
              }
            }
            Ascii85Flush(image);
            break;
          }
        }
      else
        {
          /*
            Dump number of colors and colormap.
          */
          switch (compression)
          {
            case RLECompression:
            default:
            {
              /*
                Allocate pixel array.
              */
              length=(size_t) number_pixels;
              pixels=(unsigned char *) AcquireMagickMemory(length);
              if (pixels == (unsigned char *) NULL)
                {
                  tile_image=DestroyImage(tile_image);
                  ThrowWriterException(ResourceLimitError,
                    "MemoryAllocationFailed");
                }
              /*
                Dump Runlength encoded pixels.
              */
              q=pixels;
              for (y=0; y < (long) tile_image->rows; y++)
              {
                p=AcquireImagePixels(tile_image,0,y,tile_image->columns,1,
                  &tile_image->exception);
                if (p == (const PixelPacket *) NULL)
                  break;
                indexes=GetIndexes(tile_image);
                for (x=0; x < (long) tile_image->columns; x++)
                  *q++=(unsigned char) indexes[x];
              }
#if defined(HasZLIB)
              if (compression == ZipCompression)
                status=ZLIBEncodeImage(image,length,quality,pixels);
              else
#endif
                if (compression == LZWCompression)
                  status=LZWEncodeImage(image,length,pixels);
                else
                  status=PackbitsEncodeImage(image,length,pixels);
              pixels=(unsigned char *) RelinquishMagickMemory(pixels);
              if (status == MagickFalse)
                {
                  CloseBlob(image);
                  return(MagickFalse);
                }
              break;
            }
            case NoCompression:
            {
              /*
                Dump uncompressed PseudoColor packets.
              */
              Ascii85Initialize(image);
              for (y=0; y < (long) tile_image->rows; y++)
              {
                p=AcquireImagePixels(tile_image,0,y,tile_image->columns,1,
                  &tile_image->exception);
                if (p == (const PixelPacket *) NULL)
                  break;
                indexes=GetIndexes(tile_image);
                for (x=0; x < (long) tile_image->columns; x++)
                  Ascii85Encode(image,(unsigned char) indexes[x]);
              }
              Ascii85Flush(image);
              break;
            }
          }
        }
    tile_image=DestroyImage(tile_image);
    offset=TellBlob(image)-offset;
    (void) WriteBlobString(image,"\nendstream\n");
    (void) WriteBlobString(image,"endobj\n");
    /*
      Write Length object.
    */
    xref[object++]=TellBlob(image);
    (void) FormatMagickString(buffer,MaxTextExtent,"%lu 0 obj\n",object);
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,"%lu\n",
      (unsigned long) offset);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"endobj\n");
    /*
      Write Colormap object.
    */
    xref[object++]=TellBlob(image);
    (void) FormatMagickString(buffer,MaxTextExtent,"%lu 0 obj\n",object);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"<<\n");
    if ((image->storage_class != DirectClass) && (image->colors <= 256) &&
        (compression != FaxCompression))
      {
        if (compression == NoCompression)
          (void) WriteBlobString(image,"/Filter [ /ASCII85Decode ]\n");
        (void) FormatMagickString(buffer,MaxTextExtent,"/Length %lu 0 R\n",
          object+1);
        (void) WriteBlobString(image,buffer);
        (void) WriteBlobString(image,">>\n");
        (void) WriteBlobString(image,"stream\n");
        offset=TellBlob(image);
        if (compression == NoCompression)
          Ascii85Initialize(image);
        for (i=0; i < (long) image->colors; i++)
        {
          if (compression == NoCompression)
            {
              Ascii85Encode(image,ScaleQuantumToChar(image->colormap[i].red));
              Ascii85Encode(image,ScaleQuantumToChar(image->colormap[i].green));
              Ascii85Encode(image,ScaleQuantumToChar(image->colormap[i].blue));
              continue;
            }
          (void) WriteBlobByte(image,
            ScaleQuantumToChar(image->colormap[i].red));
          (void) WriteBlobByte(image,
            ScaleQuantumToChar(image->colormap[i].green));
          (void) WriteBlobByte(image,
            ScaleQuantumToChar(image->colormap[i].blue));
        }
        if (compression == NoCompression)
          Ascii85Flush(image);
      }
    offset=TellBlob(image)-offset;
    (void) WriteBlobString(image,"\nendstream\n");
    (void) WriteBlobString(image,"endobj\n");
    /*
      Write Length object.
    */
    xref[object++]=TellBlob(image);
    (void) FormatMagickString(buffer,MaxTextExtent,"%lu 0 obj\n",object);
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,"%lu\n",
      (unsigned long) offset);
    (void) WriteBlobString(image,buffer);
    (void) WriteBlobString(image,"endobj\n");
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
  /*
    Write Xref object.
  */
  offset=TellBlob(image)-xref[0]+10;
  (void) WriteBlobString(image,"xref\n");
  (void) FormatMagickString(buffer,MaxTextExtent,"0 %lu\n",object+1);
  (void) WriteBlobString(image,buffer);
  (void) WriteBlobString(image,"0000000000 65535 f \n");
  for (i=0; i < (long) object; i++)
  {
    (void) FormatMagickString(buffer,MaxTextExtent,"%010lu 00000 n \n",
      (unsigned long) xref[i]);
    (void) WriteBlobString(image,buffer);
  }
  (void) WriteBlobString(image,"trailer\n");
  (void) WriteBlobString(image,"<<\n");
  (void) FormatMagickString(buffer,MaxTextExtent,"/Size %lu\n",object+1);
  (void) WriteBlobString(image,buffer);
  (void) FormatMagickString(buffer,MaxTextExtent,"/Info %lu 0 R\n",info_id);
  (void) WriteBlobString(image,buffer);
  (void) FormatMagickString(buffer,MaxTextExtent,"/Root %lu 0 R\n",root_id);
  (void) WriteBlobString(image,buffer);
  (void) WriteBlobString(image,">>\n");
  (void) WriteBlobString(image,"startxref\n");
  (void) FormatMagickString(buffer,MaxTextExtent,"%lu\n",
    (unsigned long) offset);
  (void) WriteBlobString(image,buffer);
  (void) WriteBlobString(image,"%%EOF\n");
  xref=(MagickOffsetType *) RelinquishMagickMemory(xref);
  CloseBlob(image);
  return(MagickTrue);
}
