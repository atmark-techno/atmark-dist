/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                               PPPP   SSSSS                                  %
%                               P   P  SS                                     %
%                               PPPP    SSS                                   %
%                               P         SS                                  %
%                               P      SSSSS                                  %
%                                                                             %
%                                                                             %
%                         Read/Write Postscript Format.                       %
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
#include "magick/profile.h"
#include "magick/resource_.h"
#include "magick/static.h"
#include "magick/string_.h"
#include "magick/token.h"
#include "magick/transform.h"
#include "magick/utility.h"

/*
  Forward declarations.
*/
static MagickBooleanType
  WritePSImage(const ImageInfo *,Image *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s P S                                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsPS() returns MagickTrue if the image format type, identified by the
%  magick string, is PS.
%
%  The format of the IsPS method is:
%
%      MagickBooleanType IsPS(const unsigned char *magick,const size_t length)
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
static MagickBooleanType IsPS(const unsigned char *magick,const size_t length)
{
  if (length < 4)
    return(MagickFalse);
  if (memcmp(magick,"%!",2) == 0)
    return(MagickTrue);
  if (memcmp(magick,"\004%!",3) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d P S I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadPSImage() reads a Postscript image file and returns it.  It allocates
%  the memory necessary for the new Image structure and returns a pointer
%  to the new image.
%
%  The format of the ReadPSImage method is:
%
%      Image *ReadPSImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/

static inline int ProfileInteger(Image *image,short int *hex_digits)
{
  int
    c,
    value;

  register long
    i;

  value=0;
  for (i=0; i < 2; )
  {
    c=ReadBlobByte(image);
    if (c == EOF)
      {
        value=(-1);
        break;
      }
    c&=0xff;
    if (isxdigit(c) == MagickFalse)
      continue;
    value=(int) ((unsigned long) value << 4)+hex_digits[c];
    i++;
  }
  return(value);
}

static Image *ReadPSImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
#define BoundingBox  "BoundingBox:"
#define BeginDocument  "BeginDocument:"
#define DocumentMedia  "DocumentMedia:"
#define EndDocument  "EndDocument:"
#define HiResBoundingBox  "HiResBoundingBox:"
#define PageBoundingBox  "PageBoundingBox:"
#define PageMedia  "PageMedia:"
#define Pages  "Pages:"
#define PhotoshopProfile  "BeginPhotoshop:"
#define PostscriptLevel  "!PS-"
#define RenderPostscriptText  "  Rendering Postscript...  "
#define XMLProfile  "begin_xml_packet:"

  char
    command[MaxTextExtent],
    density[MaxTextExtent],
    filename[MaxTextExtent],
    geometry[MaxTextExtent],
    options[MaxTextExtent],
    postscript_filename[MaxTextExtent],
    *q,
    token[MaxTextExtent],
    translate_geometry[MaxTextExtent];

  const DelegateInfo
    *delegate_info;

  GeometryInfo
    geometry_info;

  Image
    *image,
    *next_image;

  ImageInfo
    *read_info;

  int
    c,
    file;

  MagickBooleanType
    cmyk,
    skip,
    status;

  MagickStatusType
    flags;

  PointInfo
    delta;

  RectangleInfo
    page;

  register char
    *p;

  register long
    i;

  SegmentInfo
    bounds;

  short int
    hex_digits[256];

  ssize_t
    count;

  StringInfo
    *iptc_profile,
    *xml_profile;

  unsigned long
    height,
    length,
    pages,
    scene,
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
    Initialize hex values.
  */
  hex_digits[(int) '0']=0;
  hex_digits[(int) '1']=1;
  hex_digits[(int) '2']=2;
  hex_digits[(int) '3']=3;
  hex_digits[(int) '4']=4;
  hex_digits[(int) '5']=5;
  hex_digits[(int) '6']=6;
  hex_digits[(int) '7']=7;
  hex_digits[(int) '8']=8;
  hex_digits[(int) '9']=9;
  hex_digits[(int) 'a']=10;
  hex_digits[(int) 'b']=11;
  hex_digits[(int) 'c']=12;
  hex_digits[(int) 'd']=13;
  hex_digits[(int) 'e']=14;
  hex_digits[(int) 'f']=15;
  hex_digits[(int) 'A']=10;
  hex_digits[(int) 'B']=11;
  hex_digits[(int) 'C']=12;
  hex_digits[(int) 'D']=13;
  hex_digits[(int) 'E']=14;
  hex_digits[(int) 'F']=15;
  /*
    Set the page density.
  */
  delta.x=DefaultResolution;
  delta.y=DefaultResolution;
  if ((image->y_resolution == 0.0) || (image->y_resolution == 0.0))
    {
      flags=ParseGeometry(PSDensityGeometry,&geometry_info);
      image->y_resolution=geometry_info.rho;
      image->y_resolution=geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        image->y_resolution=image->y_resolution;
    }
  (void) FormatMagickString(density,MaxTextExtent,"%gx%g",image->y_resolution,
    image->y_resolution);
  /*
    Determine page geometry from the Postscript bounding box.
  */
  (void) ResetMagickMemory(&bounds,0,sizeof(bounds));
  (void) ResetMagickMemory(&page,0,sizeof(page));
  (void) ResetMagickMemory(command,0,sizeof(command));
  iptc_profile=(StringInfo *) NULL;
  xml_profile=(StringInfo *) NULL;
  skip=MagickFalse;
  cmyk=MagickFalse;
  pages=(~0UL);
  for (p=command; (c=ReadBlobByte(image)) != EOF; )
  {
    if (image_info->page != (char *) NULL)
      continue;
    /*
      Note document structuring comments.
    */
    *p++=(char) c;
    if ((c != (int) '%') && ((size_t) (p-command) < (MaxTextExtent/2)))
      continue;
    *p='\0';
    p=command;
    /*
      Skip %%BeginDocument thru %%EndDocument.
    */
    if (LocaleNCompare(BeginDocument,command,strlen(BeginDocument)) == 0)
      skip=MagickTrue;
    if (LocaleNCompare(EndDocument,command,strlen(EndDocument)) == 0)
      skip=MagickFalse;
    if (skip != MagickFalse)
      continue;
    if (LocaleNCompare(PostscriptLevel,command,strlen(PostscriptLevel)) == 0)
      if (GlobExpression(command,"*EPSF-*") != MagickFalse)
        pages=1;
    if (LocaleNCompare(Pages,command,strlen(Pages)) == 0)
      (void) sscanf(command,"Pages: %lu",&pages);
    if (LocaleNCompare(PhotoshopProfile,command,strlen(PhotoshopProfile)) == 0)
      {
        /*
          Read image profile.
        */
        count=(ssize_t) sscanf(command,"BeginPhotoshop: %lu",&length);
        if (count != 1)
          continue;
        iptc_profile=AcquireStringInfo(length);
        for (i=0; i < (long) length; i++)
          iptc_profile->datum[i]=(unsigned char)
            ProfileInteger(image,hex_digits);
      }
    if (LocaleNCompare(XMLProfile,command,strlen(XMLProfile)) == 0)
      {
        /*
          Read image profile.
        */
        count=(ssize_t) sscanf(command,"begin_xml_packet: %lu",&length);
        if (count != 1)
          continue;
        xml_profile=AcquireStringInfo(length);
        for (i=0; i < (long) length; i++)
          xml_profile->datum[i]=(unsigned char)
            ProfileInteger(image,hex_digits);
      }
    /*
      Is this a CMYK document?
    */
    for (q=p; *q != '\0'; )
    {
      GetMagickToken(q,&q,token);
      if (LocaleCompare(token,"cmyk") == 0)
        cmyk=MagickTrue;
      if (LocaleCompare(token,"cyan") == 0)
        cmyk=MagickTrue;
    }
    /*
      Note region defined by bounding box.
    */
    if ((bounds.x1 != 0.0) || (bounds.y1 != 0.0) || (bounds.x2 != 0.0) ||
        (bounds.y2 != 0.0))
      continue;
    count=0;
    if (LocaleNCompare(BoundingBox,command,strlen(BoundingBox)) == 0)
      count=(ssize_t) sscanf(command,"BoundingBox: %lf %lf %lf %lf",&bounds.x1,
        &bounds.y1,&bounds.x2,&bounds.y2);
    if (LocaleNCompare(DocumentMedia,command,strlen(DocumentMedia)) == 0)
      count=(ssize_t) sscanf(command,"DocumentMedia: %*s %lf %lf",&bounds.x2,
        &bounds.y2)+2;
    if (LocaleNCompare(HiResBoundingBox,command,strlen(HiResBoundingBox)) == 0)
      count=(ssize_t) sscanf(command,"HiResBoundingBox: %lf %lf %lf %lf",
        &bounds.x1,&bounds.y1,&bounds.x2,&bounds.y2);
    if (LocaleNCompare(PageBoundingBox,command,strlen(PageBoundingBox)) == 0)
      count=(ssize_t) sscanf(command,"PageBoundingBox: %lf %lf %lf %lf",
        &bounds.x1,&bounds.y1,&bounds.x2,&bounds.y2);
    if (LocaleNCompare(PageMedia,command,strlen(PageMedia)) == 0)
      count=(ssize_t) sscanf(command,"PageMedia: %*s %lf %lf",&bounds.x2,
        &bounds.y2)+2;
    if (count != 4)
      continue;
    /*
      Set Postscript render geometry.
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
      ThrowFileException(&image->exception,FileOpenError,"UnableToOpenFile",
        image_info->filename);
      image=DestroyImageList(image);
      return((Image *) NULL);
    }
  (void) CopyMagickString(command,"/setpagedevice {pop} bind 1 index where {"
    "dup wcheck {3 1 roll put} {pop def} ifelse} {def} ifelse\n",MaxTextExtent);
  (void) write(file,command,strlen(command));
  (void) FormatMagickString(translate_geometry,MaxTextExtent,
    "%g %g translate\n",-bounds.x1,-bounds.y1);
  (void) write(file,translate_geometry,strlen(translate_geometry));
  file=close(file)-1;
  /*
    Render Postscript with the Ghostscript delegate.
  */
  if ((page.width == 0) || (page.height == 0))
    (void) ParseAbsoluteGeometry(PSPageGeometry,&page);
  if (image_info->page != (char *) NULL)
    (void) ParseAbsoluteGeometry(image_info->page,&page);
  page.width=(unsigned long)
    (page.width*image->y_resolution/delta.x+0.5);
  page.height=(unsigned long)
    (page.height*image->y_resolution/delta.y+0.5);
  image=DestroyImageList(image);
  (void) FormatMagickString(geometry,MaxTextExtent,"%lux%lu",
    page.width,page.height);
  if (image_info->monochrome != MagickFalse)
    delegate_info=GetDelegateInfo("ps:mono",(char *) NULL,exception);
  else
    if (cmyk != MagickFalse)
      delegate_info=GetDelegateInfo("ps:cmyk",(char *) NULL,exception);
    else
      if ((pages == 1) && ((image_info->channel & OpacityChannel) != 0))
        delegate_info=GetDelegateInfo("ps:alpha",(char *) NULL,exception);
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
          "-dFirstPage=%lu -dLastPage=%lu",read_info->scene+1,
          read_info->scene+read_info->number_scenes);
      read_info->number_scenes=0;
      if (read_info->scenes != (char *) NULL)
        *read_info->scenes='\0';
    }
  (void) CopyMagickString(filename,read_info->filename,MaxTextExtent);
  (void) AcquireUniqueFilename(read_info->filename);
  (void) FormatMagickString(command,MaxTextExtent,
    GetDelegateCommands(delegate_info),
    read_info->antialias != MagickFalse ? 4 : 1,
    read_info->antialias != MagickFalse ? 4 : 1,geometry,density,options,
    read_info->filename,postscript_filename,image_info->filename);
  status=InvokePostscriptDelegate(read_info->verbose,command);
  image=ReadImage(read_info,exception);
  if (image == (Image *) NULL)
    {
      (void) ConcatenateMagickString(command," -c showpage",MaxTextExtent);
      status=InvokePostscriptDelegate(read_info->verbose,command);
      *read_info->magick='\0';
      image=ReadImage(read_info,exception);
    }
  (void) RelinquishUniqueFileResource(postscript_filename);
  (void) RelinquishUniqueFileResource(read_info->filename);
  if (image == (Image *) NULL)
    {
      /*
        Ghostscript has failed-- try the Display Postscript Extension.
      */
      (void) FormatMagickString(read_info->filename,MaxTextExtent,"dps:%s",
        filename);
      image=ReadImage(read_info,exception);
      if (image != (Image *) NULL)
        return(GetFirstImageInList(image));
      ThrowReaderException(DelegateError,"PostscriptDelegateFailed");
    }
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
    if (iptc_profile != (StringInfo *) NULL)
      (void) SetImageProfile(image,"iptc",iptc_profile);
    if (xml_profile != (StringInfo *) NULL)
      (void) SetImageProfile(image,"xml",xml_profile);
    image->page=page;
    image->scene=scene++;
    next_image=SyncNextImageInList(image);
    if (next_image != (Image *) NULL)
      image=next_image;
  } while (next_image != (Image *) NULL);
  if (iptc_profile != (StringInfo *) NULL)
    iptc_profile=DestroyStringInfo(iptc_profile);
  if (xml_profile != (StringInfo *) NULL)
    xml_profile=DestroyStringInfo(xml_profile);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r P S I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterPSImage() adds attributes for the PS image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterPSImage method is:
%
%      RegisterPSImage(void)
%
*/
ModuleExport void RegisterPSImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("EPI");
  entry->decoder=(DecoderHandler *) ReadPSImage;
  entry->encoder=(EncoderHandler *) WritePSImage;
  entry->magick=(MagickHandler *) IsPS;
  entry->adjoin=MagickFalse;
  entry->blob_support=MagickFalse;
  entry->seekable_stream=MagickTrue;
  entry->description=
    AcquireString("Encapsulated PostScript Interchange format");
  entry->module=AcquireString("PS");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("EPS");
  entry->decoder=(DecoderHandler *) ReadPSImage;
  entry->encoder=(EncoderHandler *) WritePSImage;
  entry->magick=(MagickHandler *) IsPS;
  entry->adjoin=MagickFalse;
  entry->blob_support=MagickFalse;
  entry->seekable_stream=MagickTrue;
  entry->description=AcquireString("Encapsulated PostScript");
  entry->module=AcquireString("PS");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("EPSF");
  entry->decoder=(DecoderHandler *) ReadPSImage;
  entry->encoder=(EncoderHandler *) WritePSImage;
  entry->magick=(MagickHandler *) IsPS;
  entry->adjoin=MagickFalse;
  entry->blob_support=MagickFalse;
  entry->seekable_stream=MagickTrue;
  entry->description=AcquireString("Encapsulated PostScript");
  entry->module=AcquireString("PS");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("EPSI");
  entry->decoder=(DecoderHandler *) ReadPSImage;
  entry->encoder=(EncoderHandler *) WritePSImage;
  entry->magick=(MagickHandler *) IsPS;
  entry->adjoin=MagickFalse;
  entry->blob_support=MagickFalse;
  entry->seekable_stream=MagickTrue;
  entry->description=
    AcquireString("Encapsulated PostScript Interchange format");
  entry->module=AcquireString("PS");
  (void) RegisterMagickInfo(entry);
  entry=SetMagickInfo("PS");
  entry->decoder=(DecoderHandler *) ReadPSImage;
  entry->encoder=(EncoderHandler *) WritePSImage;
  entry->magick=(MagickHandler *) IsPS;
  entry->module=AcquireString("PS");
  entry->blob_support=MagickFalse;
  entry->seekable_stream=MagickTrue;
  entry->description=AcquireString("PostScript");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r P S I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterPSImage() removes format registrations made by the
%  PS module from the list of supported formats.
%
%  The format of the UnregisterPSImage method is:
%
%      UnregisterPSImage(void)
%
*/
ModuleExport void UnregisterPSImage(void)
{
  (void) UnregisterMagickInfo("EPI");
  (void) UnregisterMagickInfo("EPS");
  (void) UnregisterMagickInfo("EPSF");
  (void) UnregisterMagickInfo("EPSI");
  (void) UnregisterMagickInfo("PS");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e P S I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WritePSImage translates an image to encapsulated Postscript
%  Level I for printing.  If the supplied geometry is null, the image is
%  centered on the Postscript page.  Otherwise, the image is positioned as
%  specified by the geometry.
%
%  The format of the WritePSImage method is:
%
%      MagickBooleanType WritePSImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o image: The image.
%
%
*/
static MagickBooleanType WritePSImage(const ImageInfo *image_info,Image *image)
{
#define WriteRunlengthPacket(image,pixel,length,p) \
{ \
  if ((image->matte != MagickFalse) && (p->opacity == TransparentOpacity)) \
    (void) FormatMagickString(buffer,MaxTextExtent,"ffffff%02lX", \
      (unsigned long) Min(length,0xff)); \
  else \
    (void) FormatMagickString(buffer,MaxTextExtent,"%02X%02X%02X%02lX", \
      ScaleQuantumToChar(pixel.red),ScaleQuantumToChar(pixel.green), \
      ScaleQuantumToChar(pixel.blue),(unsigned long) Min(length,0xff)); \
  (void) WriteBlobString(image,buffer); \
}

  static const char
    *PostscriptProlog[]=
    {
      "%%BeginProlog",
      "%",
      "% Display a color image.  The image is displayed in color on",
      "% Postscript viewers or printers that support color, otherwise",
      "% it is displayed as grayscale.",
      "%",
      "/DirectClassPacket",
      "{",
      "  %",
      "  % Get a DirectClass packet.",
      "  %",
      "  % Parameters:",
      "  %   red.",
      "  %   green.",
      "  %   blue.",
      "  %   length: number of pixels minus one of this color (optional).",
      "  %",
      "  currentfile color_packet readhexstring pop pop",
      "  compression 0 eq",
      "  {",
      "    /number_pixels 3 def",
      "  }",
      "  {",
      "    currentfile byte readhexstring pop 0 get",
      "    /number_pixels exch 1 add 3 mul def",
      "  } ifelse",
      "  0 3 number_pixels 1 sub",
      "  {",
      "    pixels exch color_packet putinterval",
      "  } for",
      "  pixels 0 number_pixels getinterval",
      "} bind def",
      "",
      "/DirectClassImage",
      "{",
      "  %",
      "  % Display a DirectClass image.",
      "  %",
      "  systemdict /colorimage known",
      "  {",
      "    columns rows 8",
      "    [",
      "      columns 0 0",
      "      rows neg 0 rows",
      "    ]",
      "    { DirectClassPacket } false 3 colorimage",
      "  }",
      "  {",
      "    %",
      "    % No colorimage operator;  convert to grayscale.",
      "    %",
      "    columns rows 8",
      "    [",
      "      columns 0 0",
      "      rows neg 0 rows",
      "    ]",
      "    { GrayDirectClassPacket } image",
      "  } ifelse",
      "} bind def",
      "",
      "/GrayDirectClassPacket",
      "{",
      "  %",
      "  % Get a DirectClass packet;  convert to grayscale.",
      "  %",
      "  % Parameters:",
      "  %   red",
      "  %   green",
      "  %   blue",
      "  %   length: number of pixels minus one of this color (optional).",
      "  %",
      "  currentfile color_packet readhexstring pop pop",
      "  color_packet 0 get 0.299 mul",
      "  color_packet 1 get 0.587 mul add",
      "  color_packet 2 get 0.114 mul add",
      "  cvi",
      "  /gray_packet exch def",
      "  compression 0 eq",
      "  {",
      "    /number_pixels 1 def",
      "  }",
      "  {",
      "    currentfile byte readhexstring pop 0 get",
      "    /number_pixels exch 1 add def",
      "  } ifelse",
      "  0 1 number_pixels 1 sub",
      "  {",
      "    pixels exch gray_packet put",
      "  } for",
      "  pixels 0 number_pixels getinterval",
      "} bind def",
      "",
      "/GrayPseudoClassPacket",
      "{",
      "  %",
      "  % Get a PseudoClass packet;  convert to grayscale.",
      "  %",
      "  % Parameters:",
      "  %   index: index into the colormap.",
      "  %   length: number of pixels minus one of this color (optional).",
      "  %",
      "  currentfile byte readhexstring pop 0 get",
      "  /offset exch 3 mul def",
      "  /color_packet colormap offset 3 getinterval def",
      "  color_packet 0 get 0.299 mul",
      "  color_packet 1 get 0.587 mul add",
      "  color_packet 2 get 0.114 mul add",
      "  cvi",
      "  /gray_packet exch def",
      "  compression 0 eq",
      "  {",
      "    /number_pixels 1 def",
      "  }",
      "  {",
      "    currentfile byte readhexstring pop 0 get",
      "    /number_pixels exch 1 add def",
      "  } ifelse",
      "  0 1 number_pixels 1 sub",
      "  {",
      "    pixels exch gray_packet put",
      "  } for",
      "  pixels 0 number_pixels getinterval",
      "} bind def",
      "",
      "/PseudoClassPacket",
      "{",
      "  %",
      "  % Get a PseudoClass packet.",
      "  %",
      "  % Parameters:",
      "  %   index: index into the colormap.",
      "  %   length: number of pixels minus one of this color (optional).",
      "  %",
      "  currentfile byte readhexstring pop 0 get",
      "  /offset exch 3 mul def",
      "  /color_packet colormap offset 3 getinterval def",
      "  compression 0 eq",
      "  {",
      "    /number_pixels 3 def",
      "  }",
      "  {",
      "    currentfile byte readhexstring pop 0 get",
      "    /number_pixels exch 1 add 3 mul def",
      "  } ifelse",
      "  0 3 number_pixels 1 sub",
      "  {",
      "    pixels exch color_packet putinterval",
      "  } for",
      "  pixels 0 number_pixels getinterval",
      "} bind def",
      "",
      "/PseudoClassImage",
      "{",
      "  %",
      "  % Display a PseudoClass image.",
      "  %",
      "  % Parameters:",
      "  %   class: 0-PseudoClass or 1-Grayscale.",
      "  %",
      "  currentfile buffer readline pop",
      "  token pop /class exch def pop",
      "  class 0 gt",
      "  {",
      "    currentfile buffer readline pop",
      "    token pop /depth exch def pop",
      "    /grays columns 8 add depth sub depth mul 8 idiv string def",
      "    columns rows depth",
      "    [",
      "      columns 0 0",
      "      rows neg 0 rows",
      "    ]",
      "    { currentfile grays readhexstring pop } image",
      "  }",
      "  {",
      "    %",
      "    % Parameters:",
      "    %   colors: number of colors in the colormap.",
      "    %   colormap: red, green, blue color packets.",
      "    %",
      "    currentfile buffer readline pop",
      "    token pop /colors exch def pop",
      "    /colors colors 3 mul def",
      "    /colormap colors string def",
      "    currentfile colormap readhexstring pop pop",
      "    systemdict /colorimage known",
      "    {",
      "      columns rows 8",
      "      [",
      "        columns 0 0",
      "        rows neg 0 rows",
      "      ]",
      "      { PseudoClassPacket } false 3 colorimage",
      "    }",
      "    {",
      "      %",
      "      % No colorimage operator;  convert to grayscale.",
      "      %",
      "      columns rows 8",
      "      [",
      "        columns 0 0",
      "        rows neg 0 rows",
      "      ]",
      "      { GrayPseudoClassPacket } image",
      "    } ifelse",
      "  } ifelse",
      "} bind def",
      "",
      "/DisplayImage",
      "{",
      "  %",
      "  % Display a DirectClass or PseudoClass image.",
      "  %",
      "  % Parameters:",
      "  %   x & y translation.",
      "  %   x & y scale.",
      "  %   label pointsize.",
      "  %   image label.",
      "  %   image columns & rows.",
      "  %   class: 0-DirectClass or 1-PseudoClass.",
      "  %   compression: 0-none or 1-RunlengthEncoded.",
      "  %   hex color packets.",
      "  %",
      "  gsave",
      "  /buffer 512 string def",
      "  /byte 1 string def",
      "  /color_packet 3 string def",
      "  /pixels 768 string def",
      "",
      "  currentfile buffer readline pop",
      "  token pop /x exch def",
      "  token pop /y exch def pop",
      "  x y translate",
      "  currentfile buffer readline pop",
      "  token pop /x exch def",
      "  token pop /y exch def pop",
      "  currentfile buffer readline pop",
      "  token pop /pointsize exch def pop",
      "  /Times-Roman findfont pointsize scalefont setfont",
      (char *) NULL
    },
    *PostscriptEpilog[]=
    {
      "  x y scale",
      "  currentfile buffer readline pop",
      "  token pop /columns exch def",
      "  token pop /rows exch def pop",
      "  currentfile buffer readline pop",
      "  token pop /class exch def pop",
      "  currentfile buffer readline pop",
      "  token pop /compression exch def pop",
      "  class 0 gt { PseudoClassImage } { DirectClassImage } ifelse",
      "  grestore",
      (char *) NULL
    };

  char
    buffer[MaxTextExtent],
    date[MaxTextExtent],
    **labels,
    page_geometry[MaxTextExtent];

  const char
    **q;

  const ImageAttribute
    *attribute;

  GeometryInfo
    geometry_info;

  IndexPacket
    index,
    polarity;

  long
    j,
    y;

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  MagickStatusType
    flags;

  PixelPacket
    pixel;

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

  register long
    i,
    x;

  SegmentInfo
    bounds;

  ssize_t
    count;

  size_t
    length;

  time_t
    timer;

  unsigned long
    bit,
    byte,
    page,
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
  (void) ResetMagickMemory(&bounds,0,sizeof(bounds));
  page=1;
  scene=0;
  do
  {
    /*
      Scale relative to dots-per-inch.
    */
    (void) SetImageColorspace(image,RGBColorspace);
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
            (LocaleCompare(image_info->magick,"PS") == 0))
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
    count=0;
    if (page == 1)
      {
        /*
          Output Postscript header.
        */
        if (LocaleCompare(image_info->magick,"PS") == 0)
          (void) strcpy(buffer,"%!PS-Adobe-3.0\n");
        else
          (void) strcpy(buffer,"%!PS-Adobe-3.0 EPSF-3.0\n");
        (void) WriteBlobString(image,buffer);
        (void) WriteBlobString(image,"%%Creator: (ImageMagick)\n");
        (void) FormatMagickString(buffer,MaxTextExtent,"%%%%Title: (%s)\n",
          image->filename);
        (void) WriteBlobString(image,buffer);
        timer=time((time_t *) NULL);
        (void) localtime(&timer);
        (void) CopyMagickString(date,ctime(&timer),MaxTextExtent);
        date[strlen(date)-1]='\0';
        (void) FormatMagickString(buffer,MaxTextExtent,
          "%%%%CreationDate: (%s)\n",date);
        (void) WriteBlobString(image,buffer);
        bounds.x1=(double) geometry.x;
        bounds.y1=(double) geometry.y;
        bounds.x2=(double) geometry.x+scale.x;
        bounds.y2=(double) geometry.y+(geometry.height+text_size);
        if ((image_info->adjoin != MagickFalse) &&
            (GetNextImageInList(image) != (Image *) NULL))
          (void) strcpy(buffer,"%%%%BoundingBox: (atend)\n");
        else
          {
            (void) FormatMagickString(buffer,MaxTextExtent,
              "%%%%BoundingBox: %ld %ld %ld %ld\n",(long) (bounds.x1+0.5),
              (long) (bounds.y1+0.5),(long) (bounds.x2+0.5),
              (long) (bounds.y2+0.5));
            (void) WriteBlobString(image,buffer);
            (void) FormatMagickString(buffer,MaxTextExtent,
              "%%%%HiResBoundingBox: %g %g %g %g\n",bounds.x1,bounds.y1,
              bounds.x2,bounds.y2);
          }
        (void) WriteBlobString(image,buffer);
        attribute=GetImageAttribute(image,"Label");
        if (attribute != (const ImageAttribute *) NULL)
          (void) WriteBlobString(image,
            "%%DocumentNeededResources: font Times-Roman\n");
        (void) WriteBlobString(image,"%%DocumentData: Clean7Bit\n");
        (void) WriteBlobString(image,"%%LanguageLevel: 1\n");
        if (LocaleCompare(image_info->magick,"PS") != 0)
          (void) WriteBlobString(image,"%%Pages: 1\n");
        else
          {
            /*
              Compute the number of pages.
            */
            (void) WriteBlobString(image,"%%Orientation: Portrait\n");
            (void) WriteBlobString(image,"%%PageOrder: Ascend\n");
            (void) FormatMagickString(buffer,MaxTextExtent,"%%%%Pages: %lu\n",
              image_info->adjoin != MagickFalse ? (unsigned long)
              GetImageListLength(image) : 1UL);
            (void) WriteBlobString(image,buffer);
          }
        (void) WriteBlobString(image,"%%EndComments\n");
        (void) WriteBlobString(image,"\n%%BeginDefaults\n");
        (void) WriteBlobString(image,"%%EndDefaults\n\n");
        if ((LocaleCompare(image_info->magick,"EPI") == 0) ||
            (LocaleCompare(image_info->magick,"EPSI") == 0))
          {
            Image
              *preview_image;

            long
              y;

            register long
              x;

            /*
              Create preview image.
            */
            preview_image=CloneImage(image,0,0,MagickTrue,&image->exception);
            if (preview_image == (Image *) NULL)
              ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
            /*
              Dump image as bitmap.
            */
            (void) SetImageType(preview_image,BilevelType);
            polarity=(IndexPacket) (PixelIntensityToQuantum(
              &preview_image->colormap[0]) < (QuantumRange/2));
            if (preview_image->colors == 2)
              polarity=(IndexPacket)
                (PixelIntensityToQuantum(&preview_image->colormap[0]) >
                 PixelIntensityToQuantum(&preview_image->colormap[1]));
            (void) FormatMagickString(buffer,MaxTextExtent,
              "%%%%BeginPreview: %lu %lu %lu %lu\n%%  ",preview_image->columns,
              preview_image->rows,1L,(((preview_image->columns+7) >> 3)*
              preview_image->rows+35)/36);
            (void) WriteBlobString(image,buffer);
            count=0;
            for (y=0; y < (long) image->rows; y++)
            {
              p=AcquireImagePixels(preview_image,0,y,preview_image->columns,1,
                &preview_image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              indexes=GetIndexes(preview_image);
              bit=0;
              byte=0;
              for (x=0; x < (long) preview_image->columns; x++)
              {
                byte<<=1;
                if (indexes[x] == polarity)
                  byte|=0x01;
                bit++;
                if (bit == 8)
                  {
                    (void) FormatMagickString(buffer,MaxTextExtent,"%02X",
                      (unsigned int) (byte & 0xff));
                    (void) WriteBlobString(image,buffer);
                    count++;
                    if (count == 36)
                      {
                        (void) WriteBlobString(image,"\n%  ");
                        count=0;
                      };
                    bit=0;
                    byte=0;
                  }
              }
              if (bit != 0)
                {
                  byte<<=(8-bit);
                  (void) FormatMagickString(buffer,MaxTextExtent,"%02X",
                    (unsigned int) (byte & 0xff));
                  (void) WriteBlobString(image,buffer);
                  count++;
                  if (count == 36)
                    {
                      (void) WriteBlobString(image,"\n%  ");
                      count=0;
                    };
                };
            }
            (void) WriteBlobString(image,"\n%%EndPreview\n");
            preview_image=DestroyImage(preview_image);
          }
        /*
          Output Postscript commands.
        */
        for (q=PostscriptProlog; *q; q++)
        {
          (void) FormatMagickString(buffer,MaxTextExtent,"%s\n",*q);
          (void) WriteBlobString(image,buffer);
        }
        attribute=GetImageAttribute(image,"Label");
        if (attribute != (const ImageAttribute *) NULL)
          for (j=(long) MultilineCensus(attribute->value)-1; j >= 0; j--)
          {
            (void) WriteBlobString(image,"  /label 512 string def\n");
            (void) WriteBlobString(image,"  currentfile label readline pop\n");
            (void) FormatMagickString(buffer,MaxTextExtent,
              "  0 y %g add moveto label show pop\n",j*image_info->pointsize+
              12);
            (void) WriteBlobString(image,buffer);
          }
        for (q=PostscriptEpilog; *q; q++)
        {
          (void) FormatMagickString(buffer,MaxTextExtent,"%s\n",*q);
          (void) WriteBlobString(image,buffer);
        }
        if (LocaleCompare(image_info->magick,"PS") == 0)
          (void) WriteBlobString(image,"  showpage\n");
        (void) WriteBlobString(image,"} bind def\n");
        (void) WriteBlobString(image,"%%EndProlog\n");
      }
    (void) FormatMagickString(buffer,MaxTextExtent,"%%%%Page:  1 %lu\n",page++);
    (void) WriteBlobString(image,buffer);
    (void) FormatMagickString(buffer,MaxTextExtent,
      "%%%%PageBoundingBox: %ld %ld %ld %ld\n",geometry.x,geometry.y,
      geometry.x+(long) geometry.width,geometry.y+(long) (geometry.height+
      text_size));
    (void) WriteBlobString(image,buffer);
    if ((double) geometry.x < bounds.x1)
      bounds.x1=(double) geometry.x;
    if ((double) geometry.y < bounds.y1)
      bounds.y1=(double) geometry.y;
    if ((double) (geometry.x+geometry.width-1) > bounds.x2)
      bounds.x2=(double) geometry.x+geometry.width-1;
    if ((double) (geometry.y+(geometry.height+text_size)-1) > bounds.y2)
      bounds.y2=(double) geometry.y+(geometry.height+text_size)-1;
    attribute=GetImageAttribute(image,"Label");
    if (attribute != (const ImageAttribute *) NULL)
      (void) WriteBlobString(image,"%%%%PageResources: font Times-Roman\n");
    if (LocaleCompare(image_info->magick,"PS") != 0)
      (void) WriteBlobString(image,"userdict begin\n");
    (void) WriteBlobString(image,"DisplayImage\n");
    /*
      Output image data.
    */
    (void) FormatMagickString(buffer,MaxTextExtent,"%ld %ld\n%g %g\n%f\n",
      geometry.x,geometry.y,scale.x,scale.y,image_info->pointsize);
    (void) WriteBlobString(image,buffer);
    labels=(char **) NULL;
    attribute=GetImageAttribute(image,"Label");
    if (attribute != (const ImageAttribute *) NULL)
      labels=StringToList(attribute->value);
    if (labels != (char **) NULL)
      {
        for (i=0; labels[i] != (char *) NULL; i++)
        {
          (void) FormatMagickString(buffer,MaxTextExtent,"%s \n",
            labels[i]);
          (void) WriteBlobString(image,buffer);
          labels[i]=(char *) RelinquishMagickMemory(labels[i]);
        }
        labels=(char **) RelinquishMagickMemory(labels);
      }
    (void) ResetMagickMemory(&pixel,0,sizeof(pixel));
    pixel.opacity=TransparentOpacity;
    i=0;
    index=0;
    x=0;
    if ((image_info->type != TrueColorType) &&
        (IsGrayImage(image,&image->exception) != MagickFalse))
      {
        (void) FormatMagickString(buffer,MaxTextExtent,"%lu %lu\n1\n1\n1\n%d\n",
          image->columns,image->rows,
          IsMonochromeImage(image,&image->exception) != MagickFalse ? 1 : 8);
        (void) WriteBlobString(image,buffer);
        if (IsMonochromeImage(image,&image->exception) == MagickFalse)
          {
            /*
              Dump image as grayscale.
            */
            i++;
            for (y=0; y < (long) image->rows; y++)
            {
              p=AcquireImagePixels(image,0,y,image->columns,1,
                &image->exception);
              if (p == (const PixelPacket *) NULL)
                break;
              for (x=0; x < (long) image->columns; x++)
              {
                (void) FormatMagickString(buffer,MaxTextExtent,"%02X",
                  ScaleQuantumToChar(PixelIntensityToQuantum(p)));
                (void) WriteBlobString(image,buffer);
                i++;
                if (i == 36)
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
          }
        else
          {
            long
              y;

            /*
              Dump image as bitmap.
            */
            (void) SetImageType(image,BilevelType);
            polarity=(IndexPacket) (PixelIntensityToQuantum(
              &image->colormap[0]) < (QuantumRange/2));
            if (image->colors == 2)
              polarity=(IndexPacket)
                (PixelIntensityToQuantum(&image->colormap[1]) <
                 PixelIntensityToQuantum(&image->colormap[0]));
            count=0;
            for (y=0; y < (long) image->rows; y++)
            {
              p=AcquireImagePixels(image,0,y,image->columns,1,
                &image->exception);
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
                    (void) FormatMagickString(buffer,MaxTextExtent,"%02X",
                      (unsigned int) (byte & 0xff));
                    (void) WriteBlobString(image,buffer);
                    count++;
                    if (count == 36)
                      {
                        (void) WriteBlobByte(image,'\n');
                        count=0;
                      };
                    bit=0;
                    byte=0;
                  }
                p++;
              }
              if (bit != 0)
                {
                  byte<<=(8-bit);
                  (void) FormatMagickString(buffer,MaxTextExtent,"%02X",
                    (unsigned int) (byte & 0xff));
                  (void) WriteBlobString(image,buffer);
                  count++;
                  if (count == 36)
                    {
                      (void) WriteBlobByte(image,'\n');
                      count=0;
                    };
                };
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
          }
        if (count != 0)
          (void) WriteBlobByte(image,'\n');
      }
    else
      if ((image->storage_class == DirectClass) ||
          (image->matte != MagickFalse))
        {
          /*
            Dump DirectClass image.
          */
          (void) FormatMagickString(buffer,MaxTextExtent,"%lu %lu\n0\n%d\n",
            image->columns,image->rows,(int)
            (image->compression == RLECompression));
          (void) WriteBlobString(image,buffer);
          switch (image->compression)
          {
            case RLECompression:
            {
              /*
                Dump runlength-encoded DirectColor packets.
              */
              for (y=0; y < (long) image->rows; y++)
              {
                p=AcquireImagePixels(image,0,y,image->columns,1,
                  &image->exception);
                if (p == (const PixelPacket *) NULL)
                  break;
                pixel=(*p);
                length=255;
                for (x=0; x < (long) image->columns; x++)
                {
                  if ((p->red == pixel.red) && (p->green == pixel.green) &&
                      (p->blue == pixel.blue) && (p->opacity == pixel.opacity) &&
                      (length < 255) && (x < (long) (image->columns-1)))
                    length++;
                  else
                    {
                      if (x > 0)
                        {
                          WriteRunlengthPacket(image,pixel,length,p);
                          i++;
                          if (i == 9)
                            {
                              (void) WriteBlobByte(image,'\n');
                              i=0;
                            }
                        }
                      length=0;
                    }
                  pixel=(*p);
                  p++;
                }
                WriteRunlengthPacket(image,pixel,length,p);
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
            case NoCompression:
            default:
            {
              /*
                Dump uncompressed DirectColor packets.
              */
              i=0;
              for (y=0; y < (long) image->rows; y++)
              {
                p=AcquireImagePixels(image,0,y,image->columns,1,
                  &image->exception);
                if (p == (const PixelPacket *) NULL)
                  break;
                for (x=0; x < (long) image->columns; x++)
                {
                  if ((image->matte != MagickFalse) &&
                      (p->opacity == TransparentOpacity))
                    (void) strcpy(buffer,"ffffff");
                  else
                    (void) FormatMagickString(buffer,MaxTextExtent,
                      "%02X%02X%02X",ScaleQuantumToChar(p->red),
                      ScaleQuantumToChar(p->green),ScaleQuantumToChar(p->blue));
                  (void) WriteBlobString(image,buffer);
                  i++;
                  if (i == 12)
                    {
                      i=0;
                      (void) WriteBlobByte(image,'\n');
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
              break;
            }
          }
          (void) WriteBlobByte(image,'\n');
        }
      else
        {
          /*
            Dump PseudoClass image.
          */
          (void) FormatMagickString(buffer,MaxTextExtent,"%lu %lu\n%d\n%d\n0\n",
            image->columns,image->rows,
            (int) (image->storage_class == PseudoClass),
            (int) (image->compression == RLECompression));
          (void) WriteBlobString(image,buffer);
          /*
            Dump number of colors and colormap.
          */
          (void) FormatMagickString(buffer,MaxTextExtent,"%lu\n",image->colors);
          (void) WriteBlobString(image,buffer);
          for (i=0; i < (long) image->colors; i++)
          {
            (void) FormatMagickString(buffer,MaxTextExtent,"%02X%02X%02X\n",
              ScaleQuantumToChar(image->colormap[i].red),
              ScaleQuantumToChar(image->colormap[i].green),
              ScaleQuantumToChar(image->colormap[i].blue));
            (void) WriteBlobString(image,buffer);
          }
          switch (image->compression)
          {
            case RLECompression:
            {
              /*
                Dump runlength-encoded PseudoColor packets.
              */
              i=0;
              for (y=0; y < (long) image->rows; y++)
              {
                p=AcquireImagePixels(image,0,y,image->columns,1,
                  &image->exception);
                if (p == (const PixelPacket *) NULL)
                  break;
                indexes=GetIndexes(image);
                index=(*indexes);
                length=255;
                for (x=0; x < (long) image->columns; x++)
                {
                  if ((index == indexes[x]) && (length < 255) &&
                      (x < ((long) image->columns-1)))
                    length++;
                  else
                    {
                      if (x > 0)
                        {
                          (void) FormatMagickString(buffer,MaxTextExtent,
                            "%02lX%02lX",(unsigned long) index,(unsigned long)
                            Min(length,0xff));
                          (void) WriteBlobString(image,buffer);
                          i++;
                          if (i == 18)
                            {
                              (void) WriteBlobByte(image,'\n');
                              i=0;
                            }
                        }
                      length=0;
                    }
                  index=indexes[x];
                  pixel=(*p);
                  p++;
                }
                (void) FormatMagickString(buffer,MaxTextExtent,"%02lX%02lX",
                  (unsigned long) index,(unsigned long) Min(length,0xff));
                (void) WriteBlobString(image,buffer);
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
            case NoCompression:
            default:
            {
              /*
                Dump uncompressed PseudoColor packets.
              */
              i=0;
              for (y=0; y < (long) image->rows; y++)
              {
                p=AcquireImagePixels(image,0,y,image->columns,1,
                  &image->exception);
                if (p == (const PixelPacket *) NULL)
                  break;
                indexes=GetIndexes(image);
                for (x=0; x < (long) image->columns; x++)
                {
                  (void) FormatMagickString(buffer,MaxTextExtent,"%02lX",
                    (unsigned long) indexes[x]);
                  (void) WriteBlobString(image,buffer);
                  i++;
                  if (i == 36)
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
              break;
            }
          }
          (void) WriteBlobByte(image,'\n');
        }
    if (LocaleCompare(image_info->magick,"PS") != 0)
      (void) WriteBlobString(image,"end\n");
    (void) WriteBlobString(image,"%%PageTrailer\n");
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
  (void) WriteBlobString(image,"%%Trailer\n");
  if (page > 2)
    {
      (void) FormatMagickString(buffer,MaxTextExtent,
        "%%%%BoundingBox: %ld %ld %ld %ld\n",(long) (bounds.x1+0.5),
        (long) (bounds.y1+0.5),(long) (bounds.x2+0.5),(long) (bounds.y2+0.5));
      (void) WriteBlobString(image,buffer);
      (void) FormatMagickString(buffer,MaxTextExtent,
        "%%%%HiResBoundingBox: %g %g %g %g\n",bounds.x1,bounds.y1,bounds.x2,
        bounds.y2);
      (void) WriteBlobString(image,buffer);
    }
  (void) WriteBlobString(image,"%%EOF\n");
  CloseBlob(image);
  return(MagickTrue);
}
