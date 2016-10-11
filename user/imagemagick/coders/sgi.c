/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            SSSSS   GGGG  IIIII                              %
%                            SS     G        I                                %
%                             SSS   G  GG    I                                %
%                               SS  G   G    I                                %
%                            SSSSS   GGG   IIIII                              %
%                                                                             %
%                                                                             %
%                      Read/Write Irix RGB Image Format.                      %
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
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/static.h"
#include "magick/string_.h"

/*
  Typedef declaractions.
*/
typedef struct _SGIInfo
{
  unsigned short
    magic;

  unsigned char
    storage,
    bytes_per_pixel;

  unsigned short
    dimension,
    columns,
    rows,
    depth;

  unsigned long
    minimum_value,
    maximum_value;

  unsigned char
    filler[492];
} SGIInfo;

/*
  Forward declarations.
*/
static MagickBooleanType
  WriteSGIImage(const ImageInfo *,Image *);
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I s S G I                                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsSGI() returns MagickTrue if the image format type, identified by the
%  magick string, is SGI.
%
%  The format of the IsSGI method is:
%
%      MagickBooleanType IsSGI(const unsigned char *magick,const size_t length)
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
static MagickBooleanType IsSGI(const unsigned char *magick,const size_t length)
{
  if (length < 2)
    return(MagickFalse);
  if (memcmp(magick,"\001\332",2) == 0)
    return(MagickTrue);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d S G I I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadSGIImage() reads a SGI RGB image file and returns it.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadSGIImage method is:
%
%      Image *ReadSGIImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/

static void SGIDecode(const unsigned long width,const size_t bytes_per_pixel,
  unsigned char *max_packets,unsigned char *pixels)
{
  register unsigned char
    *p,
    *q;

  register long
    i;

  ssize_t
    count;

  unsigned long
    pixel;

  p=max_packets;
  q=pixels;
  if (bytes_per_pixel == 2)
    {
      for (i=0; i < (long) width; )
      {
        pixel=(unsigned long) (*p++) << 8;
        pixel|=(*p++);
        count=(ssize_t) (pixel & 0x7f);
        i+=count;
        if (count == 0)
          break;
        if ((pixel & 0x80) != 0)
          for ( ; count != 0; count--)
          {
            *q=(*p++);
            *(q+1)=(*p++);
            q+=8;
          }
        else
          {
            pixel=(unsigned long) (*p++) << 8;
            pixel|=(*p++);
            for ( ; count != 0; count--)
            {
              *q=(unsigned char) (pixel >> 8);
              *(q+1)=(unsigned char) pixel;
              q+=8;
            }
          }
      }
      return;
    }
  for (i=0; i < (long) width; )
  {
    pixel=(unsigned long) (*p++);
    count=(ssize_t) (pixel & 0x7f);
    if (count == 0)
      break;
    i+=count;
    if ((pixel & 0x80) != 0)
      for ( ; count != 0; count--)
      {
        *q=(*p++);
        q+=4;
      }
    else
      {
        pixel=(unsigned long) (*p++);
        for ( ; count != 0; count--)
        {
          *q=(unsigned char) pixel;
          q+=4;
        }
      }
  }
}

static Image *ReadSGIImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  Image
    *image;

  long
    y,
    z;

  MagickBooleanType
    status;

  MagickSizeType
    number_pixels;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  register PixelPacket
    *q;

  register unsigned char
    *p;

  ssize_t
    count;

  SGIInfo
    iris_info;

  size_t
    bytes_per_pixel;

  unsigned char
    *iris_pixels;

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
    Read SGI raster header.
  */
  iris_info.magic=ReadBlobMSBShort(image);
  do
  {
    /*
      Verify SGI identifier.
    */
    if (iris_info.magic != 0x01DA)
      ThrowReaderException(CorruptImageError,"ImproperImageHeader");
    iris_info.storage=(unsigned char) ReadBlobByte(image);
    if ((int) iris_info.storage == 0x01)
      image->compression=RLECompression;
    iris_info.bytes_per_pixel=(unsigned char) ReadBlobByte(image);
    iris_info.dimension=ReadBlobMSBShort(image);
    iris_info.columns=ReadBlobMSBShort(image);
    iris_info.rows=ReadBlobMSBShort(image);
    iris_info.depth=ReadBlobMSBShort(image);
    image->columns=iris_info.columns;
    image->rows=iris_info.rows;
    image->depth=(unsigned long) (iris_info.depth <= 8 ? 8 : QuantumDepth);
    if (iris_info.depth < 3)
      {
        image->storage_class=PseudoClass;
        image->colors=256;
      }
    if ((image_info->ping != MagickFalse)  && (image_info->number_scenes != 0))
      if (image->scene >= (image_info->scene+image_info->number_scenes-1))
        break;
    iris_info.minimum_value=ReadBlobMSBLong(image);
    iris_info.maximum_value=ReadBlobMSBLong(image);
    count=ReadBlob(image,sizeof(iris_info.filler),iris_info.filler);
    /*
      Allocate SGI pixels.
    */
    bytes_per_pixel=(size_t) iris_info.bytes_per_pixel;
    number_pixels=(MagickSizeType) iris_info.columns*iris_info.rows;
    if ((4*bytes_per_pixel*number_pixels) != ((MagickSizeType) (size_t)
        (4*bytes_per_pixel*number_pixels)))
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    iris_pixels=(unsigned char *)
      AcquireMagickMemory(4*bytes_per_pixel*iris_info.columns*iris_info.rows);
    if (iris_pixels == (unsigned char *) NULL)
      ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
    if ((int) iris_info.storage != 0x01)
      {
        unsigned char
          *scanline;

        /*
          Read standard image format.
        */
        scanline=(unsigned char *)
          AcquireMagickMemory(bytes_per_pixel*iris_info.columns);
        if (scanline == (unsigned char *) NULL)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        for (z=0; z < (int) iris_info.depth; z++)
        {
          p=iris_pixels+bytes_per_pixel*z;
          for (y=0; y < (long) iris_info.rows; y++)
          {
            count=ReadBlob(image,bytes_per_pixel*iris_info.columns,
              scanline);
            if (EOFBlob(image) != MagickFalse)
              break;
            if (bytes_per_pixel == 2)
              for (x=0; x < (long) iris_info.columns; x++)
              {
                *p=scanline[2*x];
                *(p+1)=scanline[2*x+1];
                p+=8;
              }
            else
              for (x=0; x < (long) iris_info.columns; x++)
              {
                *p=scanline[x];
                p+=4;
              }
          }
        }
        scanline=(unsigned char *) RelinquishMagickMemory(scanline);
      }
    else
      {
        ssize_t
          offset,
          *offsets;

        unsigned char
          *max_packets;

        unsigned int
          data_order;

        unsigned long
          *runlength;

        /*
          Read runlength-encoded image format.
        */
        offsets=(ssize_t *) AcquireMagickMemory((size_t) iris_info.rows*
          iris_info.depth*sizeof(*offsets));
        max_packets=(unsigned char *)
          AcquireMagickMemory(4*(size_t) iris_info.columns+10);
        runlength=(unsigned long *) AcquireMagickMemory(iris_info.rows*
          iris_info.depth*sizeof(*runlength));
        if ((offsets == (ssize_t *) NULL) ||
            (max_packets == (unsigned char *) NULL) ||
            (runlength == (unsigned long *) NULL))
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        for (i=0; i < (int) (iris_info.rows*iris_info.depth); i++)
          offsets[i]=(ssize_t) ReadBlobMSBLong(image);
        for (i=0; i < (int) (iris_info.rows*iris_info.depth); i++)
          runlength[i]=ReadBlobMSBLong(image);
        /*
          Check data order.
        */
        offset=0;
        data_order=MagickFalse;
        for (y=0; ((y < (long) iris_info.rows) && (data_order == MagickFalse)); y++)
          for (z=0; ((z < (int) iris_info.depth) && (data_order == MagickFalse)); z++)
          {
            if (offsets[y+z*iris_info.rows] < offset)
              data_order=MagickTrue;
            offset=offsets[y+z*iris_info.rows];
          }
        offset=(ssize_t) (512+4*bytes_per_pixel*2*(iris_info.rows*
          iris_info.depth));
        if (data_order == 1)
          {
            for (z=0; z < (int) iris_info.depth; z++)
            {
              p=iris_pixels;
              for (y=0; y < (long) iris_info.rows; y++)
              {
                if (offset != offsets[y+z*iris_info.rows])
                  {
                    offset=offsets[y+z*iris_info.rows];
                    (void) SeekBlob(image,(long) offset,SEEK_SET);
                  }
                count=ReadBlob(image,(size_t) runlength[y+z*iris_info.rows],
                  max_packets);
                if (EOFBlob(image) != MagickFalse)
                  break;
                offset+=runlength[y+z*iris_info.rows];
                SGIDecode(iris_info.columns,bytes_per_pixel,max_packets,
                  p+bytes_per_pixel*z);
                p+=(iris_info.columns*4*bytes_per_pixel);
              }
            }
          }
        else
          {
            MagickOffsetType
              position;
           
            position=TellBlob(image);
            p=iris_pixels;
            for (y=0; y < (long) iris_info.rows; y++)
            {
              for (z=0; z < (int) iris_info.depth; z++)
              {
                if (offset != offsets[y+z*iris_info.rows])
                  {
                    offset=offsets[y+z*iris_info.rows];
                    (void) SeekBlob(image,(long) offset,SEEK_SET);
                  }
                count=ReadBlob(image,(size_t) runlength[y+z*iris_info.rows],
                  max_packets);
                if (EOFBlob(image) != MagickFalse)
                  break;
                offset+=runlength[y+z*iris_info.rows];
                SGIDecode(iris_info.columns,bytes_per_pixel,max_packets,
                  p+bytes_per_pixel*z);
              }
              p+=(iris_info.columns*4*bytes_per_pixel);
            }
            (void) SeekBlob(image,position,SEEK_SET);
          }
        runlength=(unsigned long *) RelinquishMagickMemory(runlength);
        max_packets=(unsigned char *) RelinquishMagickMemory(max_packets);
        offsets=(ssize_t *) RelinquishMagickMemory(offsets);
      }
    /*
      Initialize image structure.
    */
    image->matte=(MagickBooleanType) (iris_info.depth == 4);
    image->columns=iris_info.columns;
    image->rows=iris_info.rows;
    /*
      Convert SGI raster image to pixel packets.
    */
    if (image->storage_class == DirectClass)
      {
        /*
          Convert SGI image to DirectClass pixel packets.
        */
        if (bytes_per_pixel == 2)
          {
            for (y=0; y < (long) image->rows; y++)
            {
              p=iris_pixels+(image->rows-y-1)*8*image->columns;
              q=SetImagePixels(image,0,y,image->columns,1);
              if (q == (PixelPacket *) NULL)
                break;
              for (x=0; x < (long) image->columns; x++)
              {
                q->red=ScaleShortToQuantum((*(p+0) << 8) | (*(p+1)));
                q->green=ScaleShortToQuantum((*(p+2) << 8) | (*(p+3)));
                q->blue=ScaleShortToQuantum((*(p+4) << 8) | (*(p+5)));
                q->opacity=OpaqueOpacity;
                if (image->matte != MagickFalse)
                  q->opacity=(Quantum) (QuantumRange-
                    ScaleShortToQuantum((*(p+6) << 8) | (*(p+7))));
                p+=8;
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
          }
        else
          for (y=0; y < (long) image->rows; y++)
          {
            p=iris_pixels+(image->rows-y-1)*4*image->columns;
            q=SetImagePixels(image,0,y,image->columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            for (x=0; x < (long) image->columns; x++)
            {
              q->red=ScaleCharToQuantum(*p);
              q->green=ScaleCharToQuantum(*(p+1));
              q->blue=ScaleCharToQuantum(*(p+2));
              q->opacity=OpaqueOpacity;
              if (image->matte != MagickFalse)
                q->opacity=(Quantum) (QuantumRange-ScaleCharToQuantum(*(p+3)));
              p+=4;
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
      }
    else
      {
        /*
          Create grayscale map.
        */
        if (AllocateImageColormap(image,image->colors) == MagickFalse)
          ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
        /*
          Convert SGI image to PseudoClass pixel packets.
        */
        if (bytes_per_pixel == 2)
          {
            for (y=0; y < (long) image->rows; y++)
            {
              p=iris_pixels+(image->rows-y-1)*8*image->columns;
              q=SetImagePixels(image,0,y,image->columns,1);
              if (q == (PixelPacket *) NULL)
                break;
              indexes=GetIndexes(image);
              for (x=0; x < (long) image->columns; x++)
              {
                indexes[x]=(IndexPacket) (*p << 8);
                indexes[x]|=(*(p+1));
                p+=8;
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
          }
        else
          for (y=0; y < (long) image->rows; y++)
          {
            p=iris_pixels+(image->rows-y-1)*4*image->columns;
            q=SetImagePixels(image,0,y,image->columns,1);
            if (q == (PixelPacket *) NULL)
              break;
            indexes=GetIndexes(image);
            for (x=0; x < (long) image->columns; x++)
            {
              indexes[x]=(IndexPacket) (*p);
              p+=4;
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
        (void) SyncImage(image);
      }
    iris_pixels=(unsigned char *) RelinquishMagickMemory(iris_pixels);
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
    iris_info.magic=ReadBlobMSBShort(image);
    if (iris_info.magic == 0x01DA)
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
  } while (iris_info.magic == 0x01DA);
  CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r S G I I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterSGIImage() adds attributes for the SGI image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterSGIImage method is:
%
%      RegisterSGIImage(void)
%
*/
ModuleExport void RegisterSGIImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("SGI");
  entry->decoder=(DecoderHandler *) ReadSGIImage;
  entry->encoder=(EncoderHandler *) WriteSGIImage;
  entry->magick=(MagickHandler *) IsSGI;
  entry->description=AcquireString("Irix RGB image");
  entry->module=AcquireString("SGI");
  entry->seekable_stream=MagickTrue;
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r S G I I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterSGIImage() removes format registrations made by the
%  SGI module from the list of supported formats.
%
%  The format of the UnregisterSGIImage method is:
%
%      UnregisterSGIImage(void)
%
*/
ModuleExport void UnregisterSGIImage(void)
{
  (void) UnregisterMagickInfo("SGI");
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e S G I I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteSGIImage() writes an image in SGI RGB encoded image format.
%
%  The format of the WriteSGIImage method is:
%
%      MagickBooleanType WriteSGIImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/

static size_t SGIEncode(unsigned char *pixels,size_t length,
  unsigned char *packets)
{
  short
    runlength;

  register unsigned char
    *p,
    *q;

  unsigned char
    *limit,
    *mark;

  p=pixels;
  limit=p+length*4;
  q=packets;
  while (p < limit)
  {
    mark=p;
    p+=8;
    while ((p < limit) && ((*(p-8) != *(p-4)) || (*(p-4) != *p)))
      p+=4;
    p-=8;
    length=(size_t) (p-mark) >> 2;
    while (length != 0)
    {
      runlength=(short) (length > 126 ? 126 : length);
      length-=runlength;
      *q++=(unsigned char) (0x80 | runlength);
      for ( ; runlength > 0; runlength--)
      {
        *q++=(*mark);
        mark+=4;
      }
    }
    mark=p;
    p+=4;
    while ((p < limit) && (*p == *mark))
      p+=4;
    length=(size_t) (p-mark) >> 2;
    while (length != 0)
    {
      runlength=(short) (length > 126 ? 126 : length);
      length-=runlength;
      *q++=(unsigned char) runlength;
      *q++=(*mark);
    }
  }
  *q++='\0';
  return((size_t) (q-packets));
}

static MagickBooleanType WriteSGIImage(const ImageInfo *image_info,Image *image)
{
  long
    y,
    z;

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  MagickSizeType
    number_pixels;

  SGIInfo
    iris_info;

  register const PixelPacket
    *p;

  register long
    i,
    x;

  register unsigned char
    *q;

  unsigned char
    *iris_pixels,
    *packets;

  /*
    Open output image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((image->columns > 65535UL) || (image->rows > 65535UL))
    ThrowWriterException(ImageError,"WidthOrHeightExceedsLimit");
  status=OpenBlob(image_info,image,WriteBinaryBlobMode,&image->exception);
  if (status == MagickFalse)
    return(status);
  scene=0;
  do
  {
    /*
      Initialize SGI raster file header.
    */
    if (image_info->colorspace != LogColorspace)
      (void) SetImageColorspace(image,RGBColorspace);
    iris_info.magic=0x01DA;
    if (image->compression == NoCompression)
      iris_info.storage=(unsigned char) 0x00;
    else
      iris_info.storage=(unsigned char) 0x01;
    iris_info.bytes_per_pixel=(unsigned char) 1;  /* one byte per pixel */
    iris_info.dimension=3;
    iris_info.columns=(unsigned short) image->columns;
    iris_info.rows=(unsigned short) image->rows;
    if (image->matte != MagickFalse)
      iris_info.depth=4;
    else
      {
        if ((image_info->type != TrueColorType) &&
            (IsGrayImage(image,&image->exception) != MagickFalse))
          {
            iris_info.dimension=2;
            iris_info.depth=1;
          }
        else
          iris_info.depth=3;
      }
    iris_info.minimum_value=0;
    iris_info.maximum_value=(unsigned long) ScaleQuantumToChar(QuantumRange);
    for (i=0; i < (int) sizeof(iris_info.filler); i++)
      iris_info.filler[i]=(unsigned char) 0;
    /*
      Write SGI header.
    */
    (void) WriteBlobMSBShort(image,iris_info.magic);
    (void) WriteBlobByte(image,iris_info.storage);
    (void) WriteBlobByte(image,iris_info.bytes_per_pixel);
    (void) WriteBlobMSBShort(image,iris_info.dimension);
    (void) WriteBlobMSBShort(image,iris_info.columns);
    (void) WriteBlobMSBShort(image,iris_info.rows);
    (void) WriteBlobMSBShort(image,iris_info.depth);
    (void) WriteBlobMSBLong(image,iris_info.minimum_value);
    (void) WriteBlobMSBLong(image,iris_info.maximum_value);
    (void) WriteBlob(image,sizeof(iris_info.filler),iris_info.filler);
    /*
      Allocate SGI pixels.
    */
    number_pixels=(MagickSizeType) image->columns*image->rows;
    if ((4*number_pixels) != ((MagickSizeType) (size_t) (4*number_pixels)))
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    iris_pixels=(unsigned char *)
      AcquireMagickMemory(4*(size_t) image->columns*image->rows);
    if (iris_pixels == (unsigned char *) NULL)
      ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
    /*
      Convert image pixels to uncompressed SGI pixels.
    */
    for (y=0; y < (long) image->rows; y++)
    {
      p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
      if (p == (const PixelPacket *) NULL)
        break;
      q=iris_pixels+((iris_info.rows-1)-y)*(iris_info.columns*4);
      for (x=0; x < (long) image->columns; x++)
      {
        *q++=ScaleQuantumToChar(p->red);
        *q++=ScaleQuantumToChar(p->green);
        *q++=ScaleQuantumToChar(p->blue);
        *q++=ScaleQuantumToChar(QuantumRange-p->opacity);
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
    if (image->compression == NoCompression)
      {
        unsigned char
          *scanline;

        /*
          Write uncompressed SGI pixels.
        */
        scanline=(unsigned char *)
          AcquireMagickMemory((size_t) iris_info.columns);
        if (scanline == (unsigned char *) NULL)
          ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
        for (z=0; z < (int) iris_info.depth; z++)
        {
          q=iris_pixels+z;
          for (y=0; y < (long) iris_info.rows; y++)
          {
            for (x=0; x < (long) iris_info.columns; x++)
            {
              scanline[x]=(*q);
              q+=4;
            }
            (void) WriteBlob(image,(size_t) iris_info.columns,scanline);
          }
        }
        scanline=(unsigned char *) RelinquishMagickMemory(scanline);
      }
    else
      {
        ssize_t
          offset,
          *offsets;

        size_t
          length,
          number_packets;

        unsigned long
          *runlength;

        /*
          Convert SGI uncompressed pixels.
        */
        offsets=(ssize_t *) AcquireMagickMemory(iris_info.rows*
          iris_info.depth*sizeof(*offsets));
        packets=(unsigned char *)
          AcquireMagickMemory(4*(2*(size_t) iris_info.columns+10)*image->rows);
        runlength=(unsigned long *) AcquireMagickMemory(iris_info.rows*
          iris_info.depth*sizeof(*runlength));
        if ((offsets == (ssize_t *) NULL) ||
            (packets == (unsigned char *) NULL) ||
            (runlength == (unsigned long *) NULL))
          ThrowWriterException(ResourceLimitError,"MemoryAllocationFailed");
        offset=512+4*2*((ssize_t) iris_info.rows*iris_info.depth);
        number_packets=0;
        q=iris_pixels;
        for (y=0; y < (long) iris_info.rows; y++)
        {
          for (z=0; z < (long) iris_info.depth; z++)
          {
            length=SGIEncode(q+z,(size_t) iris_info.columns,packets+
              number_packets);
            number_packets+=length;
            offsets[y+z*iris_info.rows]=offset;
            runlength[y+z*iris_info.rows]=(unsigned long) length;
            offset+=length;
          }
          q+=(iris_info.columns*4);
        }
        /*
          Write out line start and length tables and runlength-encoded pixels.
        */
        for (i=0; i < (long) (iris_info.rows*iris_info.depth); i++)
          (void) WriteBlobMSBLong(image,(unsigned long) offsets[i]);
        for (i=0; i < (long) (iris_info.rows*iris_info.depth); i++)
          (void) WriteBlobMSBLong(image,runlength[i]);
        (void) WriteBlob(image,number_packets,packets);
        /*
          Free resources.
        */
        runlength=(unsigned long *) RelinquishMagickMemory(runlength);
        packets=(unsigned char *) RelinquishMagickMemory(packets);
        offsets=(ssize_t *) RelinquishMagickMemory(offsets);
      }
    iris_pixels=(unsigned char *) RelinquishMagickMemory(iris_pixels);
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
