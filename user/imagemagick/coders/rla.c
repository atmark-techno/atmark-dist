/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                            RRRR   L       AAA                               %
%                            R   R  L      A   A                              %
%                            RRRR   L      AAAAA                              %
%                            R R    L      A   A                              %
%                            R  R   LLLLL  A   A                              %
%                                                                             %
%                                                                             %
%                      Read Alias/Wavefront Image Format.                     %
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
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d R L A I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadRLAImage() reads a run-length encoded Wavefront RLA image file
%  and returns it.  It allocates the memory necessary for the new Image
%  structure and returns a pointer to the new image.
%
%  Note:  This module was contributed by Lester Vecsey (master@internexus.net).
%
%  The format of the ReadRLAImage method is:
%
%      Image *ReadRLAImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadRLAImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  typedef struct _WindowFrame
  {
    short
      left,
      right,
      bottom,
      top;
  } WindowFrame;

  typedef struct _RLAInfo
  {
    WindowFrame
      window,
      active_window;

    short
      frame,
      storage_type,
      number_channels,
      number_matte_channels,
      number_auxiliary_channels,
      revision;

    char
      gamma[16],
      red_primary[24],
      green_primary[24],
      blue_primary[24],
      white_point[24];

    long
      job_number;

    char
      name[128],
      description[128],
      program[64],
      machine[32],
      user[32],
      date[20],
      aspect[24],
      aspect_ratio[8],
      chan[32];

    short
      field;

    char
      time[12],
      filter[32];

    short
      bits_per_channel,
      matte_type,
      matte_bits,
      auxiliary_type,
      auxiliary_bits;

    char
      auxiliary[32],
      space[36];

    long
      next;
  } RLAInfo;

  Image
    *image;

  int
    channel,
    length,
    runlength;

  long
    y;

  long
    *scanlines;

  MagickBooleanType
    status;

  register long
    i,
    x;

  register PixelPacket
    *q;

  ssize_t
    count;

  RLAInfo
    rla_info;

  unsigned char
    byte;

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
  rla_info.window.left=ReadBlobMSBShort(image);
  rla_info.window.right=ReadBlobMSBShort(image);
  rla_info.window.bottom=ReadBlobMSBShort(image);
  rla_info.window.top=ReadBlobMSBShort(image);
  rla_info.active_window.left=ReadBlobMSBShort(image);
  rla_info.active_window.right=ReadBlobMSBShort(image);
  rla_info.active_window.bottom=ReadBlobMSBShort(image);
  rla_info.active_window.top=ReadBlobMSBShort(image);
  rla_info.frame=ReadBlobMSBShort(image);
  rla_info.storage_type=ReadBlobMSBShort(image);
  rla_info.number_channels=ReadBlobMSBShort(image);
  rla_info.number_matte_channels=ReadBlobMSBShort(image);
  if (rla_info.number_channels == 0)
    rla_info.number_channels=3;
  rla_info.number_channels+=rla_info.number_matte_channels;
  rla_info.number_auxiliary_channels=ReadBlobMSBShort(image);
  rla_info.revision=ReadBlobMSBShort(image);
  count=ReadBlob(image,16,(unsigned char *) rla_info.gamma);
  count=ReadBlob(image,24,(unsigned char *) rla_info.red_primary);
  count=ReadBlob(image,24,(unsigned char *) rla_info.green_primary);
  count=ReadBlob(image,24,(unsigned char *) rla_info.blue_primary);
  count=ReadBlob(image,24,(unsigned char *) rla_info.white_point);
  rla_info.job_number=(long) ReadBlobMSBLong(image);
  count=ReadBlob(image,128,(unsigned char *) rla_info.name);
  count=ReadBlob(image,128,(unsigned char *) rla_info.description);
  count=ReadBlob(image,64,(unsigned char *) rla_info.program);
  count=ReadBlob(image,32,(unsigned char *) rla_info.machine);
  count=ReadBlob(image,32,(unsigned char *) rla_info.user);
  count=ReadBlob(image,20,(unsigned char *) rla_info.date);
  count=ReadBlob(image,24,(unsigned char *) rla_info.aspect);
  count=ReadBlob(image,8,(unsigned char *) rla_info.aspect_ratio);
  count=ReadBlob(image,32,(unsigned char *) rla_info.chan);
  rla_info.field=ReadBlobMSBShort(image);
  count=ReadBlob(image,12,(unsigned char *) rla_info.time);
  count=ReadBlob(image,32,(unsigned char *) rla_info.filter);
  rla_info.bits_per_channel=ReadBlobMSBShort(image);
  rla_info.matte_type=ReadBlobMSBShort(image);
  rla_info.matte_bits=ReadBlobMSBShort(image);
  rla_info.auxiliary_type=ReadBlobMSBShort(image);
  rla_info.auxiliary_bits=ReadBlobMSBShort(image);
  count=ReadBlob(image,32,(unsigned char *) rla_info.auxiliary);
  count=ReadBlob(image,36,(unsigned char *) rla_info.space);
  rla_info.next=(long) ReadBlobMSBLong(image);
  /*
    Initialize image structure.
  */
  image->matte=rla_info.number_matte_channels != 0 ? MagickTrue : MagickFalse;
  image->columns=rla_info.active_window.right-rla_info.active_window.left+1;
  image->rows=rla_info.active_window.top-rla_info.active_window.bottom+1;
  if (image_info->ping != MagickFalse)
    {
      CloseBlob(image);
      return(GetFirstImageInList(image));
    }
  scanlines=(long *) AcquireMagickMemory(image->rows*sizeof(long));
  if (scanlines == (long *) NULL)
    ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  if (*rla_info.description != '\0')
    (void) SetImageAttribute(image,"Comment",rla_info.description);
  /*
    Read offsets to each scanline data.
  */
  for (i=0; i < (long) image->rows; i++)
    scanlines[i]=(long) ReadBlobMSBLong(image);
  /*
    Read image data.
  */
  x=0;
  for (y=0; y < (long) image->rows; y++)
  {
    (void) SeekBlob(image,scanlines[image->rows-y-1],SEEK_SET);
    for (channel=0; channel < (int) rla_info.number_channels; channel++)
    {
      length=ReadBlobMSBShort(image);
      while (length > 0)
      {
        byte=ReadBlobByte(image);
        runlength=byte;
        if (byte > 127)
          runlength=byte-256;
        length--;
        if (length == 0)
          break;
        if (runlength < 0)
          {
            while (runlength < 0)
            {
              q=GetImagePixels(image,(long) (x % image->columns),
                (long) (y % image->rows),1,1);
              if (q == (PixelPacket *) NULL)
                break;
              byte=ReadBlobByte(image);
              length--;
              switch (channel)
              {
                case 0:
                {
                  q->red=ScaleCharToQuantum(byte);
                  break;
                }
                case 1:
                {
                  q->green=ScaleCharToQuantum(byte);
                  break;
                }
                case 2:
                {
                  q->blue=ScaleCharToQuantum(byte);
                  break;
                }
                case 3:
                default:
                {
                  q->opacity=(Quantum) (QuantumRange-ScaleCharToQuantum(byte));
                  break;
                }
              }
              if (SyncImagePixels(image) == MagickFalse)
                break;
              x++;
              runlength++;
            }
            continue;
          }
        byte=ReadBlobByte(image);
        length--;
        runlength++;
        do
        {
          q=GetImagePixels(image,(long) (x % image->columns),
            (long) (y % image->rows),1,1);
          if (q == (PixelPacket *) NULL)
            break;
          switch (channel)
          {
            case 0:
            {
              q->red=ScaleCharToQuantum(byte);
              break;
            }
            case 1:
            {
              q->green=ScaleCharToQuantum(byte);
              break;
            }
            case 2:
            {
              q->blue=ScaleCharToQuantum(byte);
              break;
            }
            case 3:
            default:
            {
              q->opacity=(Quantum) (QuantumRange-ScaleCharToQuantum(byte));
              break;
            }
          }
          if (SyncImagePixels(image) == MagickFalse)
            break;
          x++;
          runlength--;
        }
        while (runlength > 0);
      }
    }
    if (QuantumTick(y,image->rows) != MagickFalse)
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
  CloseBlob(image);
  return(GetFirstImageInList(image));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r R L A I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterRLAImage() adds attributes for the RLA image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterRLAImage method is:
%
%      RegisterRLAImage(void)
%
*/
ModuleExport void RegisterRLAImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("RLA");
  entry->decoder=(DecoderHandler *) ReadRLAImage;
  entry->adjoin=MagickFalse;
  entry->description=AcquireString("Alias/Wavefront image");
  entry->module=AcquireString("RLA");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r R L A I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterRLAImage() removes format registrations made by the
%  RLA module from the list of supported formats.
%
%  The format of the UnregisterRLAImage method is:
%
%      UnregisterRLAImage(void)
%
*/
ModuleExport void UnregisterRLAImage(void)
{
  (void) UnregisterMagickInfo("RLA");
}
