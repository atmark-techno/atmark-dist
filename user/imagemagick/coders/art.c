/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                             AAA   RRRR   TTTTT                              %
%                            A   A  R   R    T                                %
%                            AAAAA  RRRR     T                                %
%                            A   A  R R      T                                %
%                            A   A  R  R     T                                %
%                                                                             %
%                                                                             %
%                Read/Write PFS: 1st Publisher Image Format.                  %
%                                                                             %
%                              Software Design                                %
%                              Jaroslav Fojtik                                %
%                                January 2001                                 %
%                                                                             %
%                                                                             %
%  Permission is hereby granted, free of charge, to any person obtaining a    %
%  copy of this software and associated documentation files ("ImageMagick"),  %
%  to deal in ImageMagick without restriction, including without limitation   %
%  the rights to use, copy, modify, merge, publish, distribute, sublicense,   %
%  and/or sell copies of ImageMagick, and to permit persons to whom the       %
%  ImageMagick is furnished to do so, subject to the following conditions:    %
%                                                                             %
%  The above copyright notice and this permission notice shall be included in %
%  all copies or substantial portions of ImageMagick.                         %
%                                                                             %
%  The software is provided "as is", without warranty of any kind, express or %
%  implied, including but not limited to the warranties of merchantability,   %
%  fitness for a particular purpose and noninfringement.  In no event shall   %
%  ImageMagick Studio be liable for any claim, damages or other liability,    %
%  whether in an action of contract, tort or otherwise, arising from, out of  %
%  or in connection with ImageMagick or the use or other dealings in          %
%  ImageMagick.                                                               %
%                                                                             %
%  Except as contained in this notice, the name of the ImageMagick Studio     %
%  shall not be used in advertising or otherwise to promote the sale, use or  %
%  other dealings in ImageMagick without prior written authorization from the %
%  ImageMagick Studio.                                                        %
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
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/static.h"
#include "magick/string_.h"


static void InsertRow(unsigned char *p,int y,Image *image)
{
unsigned long bit; long x;
register PixelPacket *q;
IndexPacket index;
register IndexPacket *indexes;


 switch (image->depth)
      {
      case 1:  /* Convert bitmap scanline. */
       {
       q=SetImagePixels(image,0,y,image->columns,1);
       if (q == (PixelPacket *) NULL)
       break;
       indexes=GetIndexes(image);
       for (x=0; x < ((long) image->columns-7); x+=8)
    {
    for (bit=0; bit < 8; bit++)
       {
       index=(IndexPacket) (((*p) & (0x80 >> bit)) != 0 ? 0x01 : 0x00);
       indexes[x+bit]=index;
       *q++=image->colormap[index];
       }
    p++;
    }
       if ((image->columns % 8) != 0)
     {
     for (bit=0; bit < (image->columns % 8); bit++)
         {
         index=(IndexPacket) (((*p) & (0x80 >> bit)) != 0 ? 0x01 : 0x00);
         indexes[x+bit]=index;
         *q++=image->colormap[index];
         }
     p++;
     }
        if (SyncImagePixels(image) == MagickFalse)
     break;
/*            if (image->previous == (Image *) NULL)
     if (QuantumTick(y,image->rows) != MagickFalse)
       ProgressMonitor(LoadImageTag,image->rows-y-1,image->rows);*/
      break;
      }
       }
}



/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d A R T I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadARTImage() reads an ART X image file and returns it.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadARTImage method is:
%
%      Image *ReadARTImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/
static Image *ReadARTImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  Image *image;
  unsigned long width,height,dummy;
  long ldblk;
  unsigned char *BImgBuff=NULL;
  unsigned char k;

  MagickBooleanType
    status;

  register long
    i;

  ssize_t
    count;

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
    Read ART image.
  */
  dummy=ReadBlobLSBShort(image);
  width=ReadBlobLSBShort(image);
  dummy=ReadBlobLSBShort(image);
  height=ReadBlobLSBShort(image);

  ldblk=(long) ((width+7) / 8);
  k=(unsigned char) ((-ldblk) & 0x01);
  if (GetBlobSize(image) != (MagickSizeType) (8+(ldblk+(long) k)*height))
    ThrowReaderException(CorruptImageError,"ImproperImageHeader");
 image->columns=width;
 image->rows=height;
 image->depth=1;
 image->colors=1l << image->depth;

/* printf("ART header checked OK %d,%d\n",image->colors,image->depth); */

 if (AllocateImageColormap(image,image->colors) == MagickFalse) goto NoMemory;

/* ----- Load RLE compressed raster ----- */
 BImgBuff=(unsigned char *) AcquireMagickMemory((size_t) ldblk);  /*Ldblk was set in the check phase*/
 if (BImgBuff==NULL)
NoMemory:
  ThrowReaderException(ResourceLimitError,"MemoryAllocationFailed");
  for (i=0; i < (int) height; i++)
  {
    count=ReadBlob(image,(size_t) ldblk,BImgBuff);
    if (count != (ssize_t) ldblk)
      break;
    count=ReadBlob(image,(size_t) k,(unsigned char *) &dummy);
    if (count != (ssize_t) k)
      break;
    InsertRow(BImgBuff,i,image);
  }
  BImgBuff=(unsigned char *) RelinquishMagickMemory(BImgBuff);
  if (i < (long) height)
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
%   R e g i s t e r A R T I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterARTImage() adds attributes for the ART image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterARTImage method is:
%
%      RegisterARTImage(void)
%
*/
ModuleExport void RegisterARTImage(void)
{
  MagickInfo
    *entry;

  static const char
    *ARTNote=
    {
      "Format originally used on the Macintosh (MacPaint?) and later used\n"
      "for PFS: 1st Publisher clip art.  NOT the AOL ART format."
    };

  entry=SetMagickInfo("ART");
  entry->decoder=(DecoderHandler *) ReadARTImage;
  entry->description=AcquireString("PFS: 1st Publisher");
  entry->module=AcquireString("ART");
  entry->note=AcquireString(ARTNote);
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r A R T I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterARTImage() removes format registrations made by the
%  ART module from the list of supported formats.
%
%  The format of the UnregisterARTImage method is:
%
%      UnregisterARTImage(void)
%
*/
ModuleExport void UnregisterARTImage(void)
{
  (void) UnregisterMagickInfo("ART");
}
