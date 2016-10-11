/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  M   M   AAA    GGGG  IIIII   CCCC  K   K                   %
%                  MM MM  A   A  G        I    C      K  K                    %
%                  M M M  AAAAA  G GGG    I    C      KKK                     %
%                  M   M  A   A  G   G    I    C      K  K                    %
%                  M   M  A   A   GGGG  IIIII   CCCC  K   K                   %
%                                                                             %
%                         W   W   AAA   N   N  DDDD                           %
%                         W   W  A   A  NN  N  D   D                          %
%                         W W W  AAAAA  N N N  D   D                          %
%                         WW WW  A   A  N  NN  D   D                          %
%                         W   W  A   A  N   N  DDDD                           %
%                                                                             %
%                    ImageMagick MagickWand Validation Tests                  %
%                                                                             %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 March 2003                                  %
%                                                                             %
%                                                                             %
%  Copyright 1999-2005 ImageMagick Studio LLC, a non-profit organization      %
%  dedicated to making software imaging solutions freely available.           %
%                                                                             %
%  This software and documentation is provided "as is," and the copyright     %
%  holders and contributing author(s) make no representations or warranties,  %
%  express or implied, including but not limited to, warranties of            %
%  merchantability or fitness for any particular purpose or that the use of   %
%  the software or documentation will not infringe any third party patents,   %
%  copyrights, trademarks or other rights.                                    %
%                                                                             %
%  The copyright holders and contributing author(s) will not be held liable   %
%  for any direct, indirect, special or consequential damages arising out of  %
%  any use of the software or documentation, even if advised of the           %
%  possibility of such damage.                                                %
%                                                                             %
%  Permission is hereby granted to use, copy, modify, and distribute this     %
%  source code, or portions hereof, documentation and executables, for any    %
%  purpose, without fee, subject to the following restrictions:               %
%                                                                             %
%    1. The origin of this source code must not be misrepresented.            %
%    2. Altered versions must be plainly marked as such and must not be       %
%       misrepresented as being the original source.                          %
%    3. This Copyright notice may not be removed or altered from any source   %
%       or altered source distribution.                                       %
%                                                                             %
%  The copyright holders and contributing author(s) specifically permit,      %
%  without fee, and encourage the use of this source code as a component for  %
%  supporting image processing in commercial products.  If you use this       %
%  source code in a product, acknowledgment is not required but would be      %
%  appreciated.                                                               %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
%
*/

/*
  Include declarations.
*/
#if !defined(_VISUALC_)
#include <wand/wand-config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#if defined(_VISUALC_)
#include <stdlib.h>
#include <sys\types.h>
#endif
#include <time.h>
#include <wand/magick-wand.h>

#define WandDelay   3

int main(int argc,char **argv)
{
#define ThrowAPIException(wand) \
{ \
  description=MagickGetException(wand,&severity); \
  (void) fprintf(stderr,"%s %s %lu %s\n",GetMagickModule(),description); \
  description=(char *) MagickRelinquishMemory(description); \
  exit(-1); \
}

  char
    *description;

  DrawingWand
    *drawing_wand;

  ExceptionType
    severity;

  MagickWand
    *clone_wand,
    *magick_wand;
 
  PixelIterator
    *iterator;

  PixelWand
    *background,
    *border,
    *fill,
    **pixels;

  register long
    i;

  unsigned int
    status;

  unsigned long
    columns,
    delay,
    number_wands,
    rows;

  magick_wand=NewMagickWand();
  (void) MagickSetSize(magick_wand,640,480);
  (void) MagickGetSize(magick_wand,&columns,&rows);
  if ((columns != 640) || (rows != 480))
    {
      (void) fprintf(stderr,"Unexpected magick wand size\n");
      exit(1);
    }
  (void) fprintf(stdout,"Reading images...\n");
  status=MagickReadImage(magick_wand,"sequence.miff");
  if (status == MagickFalse)
    ThrowAPIException(magick_wand);
  if (MagickGetNumberImages(magick_wand) != 5)
    (void) fprintf(stderr,"read %lu images; expected 5\n",
      (unsigned long) MagickGetNumberImages(magick_wand));
  (void) fprintf(stdout,"Iterate forward...\n");
  MagickResetIterator(magick_wand);
  while (MagickNextImage(magick_wand) != MagickFalse)
    (void) fprintf(stdout,"index %ld scene %lu\n",
      MagickGetImageIndex(magick_wand),MagickGetImageScene(magick_wand));
  (void) fprintf(stdout,"Iterate reverse...\n");
  while (MagickPreviousImage(magick_wand) != MagickFalse)
    (void) fprintf(stdout,"index %ld scene %lu\n",
      MagickGetImageIndex(magick_wand),MagickGetImageScene(magick_wand));
  (void) fprintf(stdout,"Remove scene 1...\n");
  (void) MagickSetImageIndex(magick_wand,1);
  clone_wand=MagickGetImage(magick_wand);
  status=MagickRemoveImage(magick_wand);
  if (status == MagickFalse)
    ThrowAPIException(magick_wand);
  MagickResetIterator(magick_wand);
  while (MagickNextImage(magick_wand) != MagickFalse)
    (void) fprintf(stdout,"index %ld scene %lu\n",
      MagickGetImageIndex(magick_wand),MagickGetImageScene(magick_wand));
  (void) fprintf(stdout,"Insert scene 1 back in sequence...\n");
  (void) MagickSetImageIndex(magick_wand,0);
  status=MagickAddImage(magick_wand,clone_wand);
  if (status == MagickFalse)
    ThrowAPIException(magick_wand);
  MagickResetIterator(magick_wand);
  while (MagickNextImage(magick_wand) != MagickFalse)
    (void) fprintf(stdout,"index %ld scene %lu\n",
      MagickGetImageIndex(magick_wand),MagickGetImageScene(magick_wand));
  (void) fprintf(stdout,"Set scene 2 to scene 1...\n");
  (void) MagickSetImageIndex(magick_wand,2);
  status=MagickSetImage(magick_wand,clone_wand);
  clone_wand=DestroyMagickWand(clone_wand);
  if (status == MagickFalse)
    ThrowAPIException(magick_wand);
  MagickResetIterator(magick_wand);
  while (MagickNextImage(magick_wand) != MagickFalse)
    (void) fprintf(stdout,"index %ld scene %lu\n",
      MagickGetImageIndex(magick_wand),MagickGetImageScene(magick_wand));
  (void) fprintf(stdout,"Apply image processing options...\n");
  status=MagickCropImage(magick_wand,60,60,10,10);
  if (status == MagickFalse)
    ThrowAPIException(magick_wand);
  MagickResetIterator(magick_wand);
  background=NewPixelWand();
  (void) PixelSetColor(background,"#000000");
  if (status == MagickFalse)
    ThrowAPIException(magick_wand);
  status=MagickRotateImage(magick_wand,background,90.0);
  if (status == MagickFalse)
    ThrowAPIException(magick_wand);
  border=NewPixelWand();
  (void) PixelSetColor(background,"green");
  (void) PixelSetColor(border,"black");
  status=MagickColorFloodfillImage(magick_wand,background,0.01*QuantumRange,
    border,0,0);
  if (status == MagickFalse)
    ThrowAPIException(magick_wand);
  background=DestroyPixelWand(background);
  border=DestroyPixelWand(border);
  drawing_wand=NewDrawingWand();
  (void) PushDrawingWand(drawing_wand);
  (void) DrawRotate(drawing_wand,45);
  (void) DrawSetFontSize(drawing_wand,18);
  fill=NewPixelWand();
  (void) PixelSetColor(fill,"green");
  (void) DrawSetFillColor(drawing_wand,fill);
  fill=DestroyPixelWand(fill);
  (void) DrawAnnotation(drawing_wand,15,5,(const unsigned char *) "Magick");
  (void) PopDrawingWand(drawing_wand);
  (void) MagickSetImageIndex(magick_wand,1);
  status=MagickDrawImage(magick_wand,drawing_wand);
  if (status == MagickFalse)
    ThrowAPIException(magick_wand);
  status=MagickAnnotateImage(magick_wand,drawing_wand,70,5,90,"Image");
  if (status == MagickFalse)
    ThrowAPIException(magick_wand);
  drawing_wand=DestroyDrawingWand(drawing_wand);
  {
    unsigned char
      pixels[27],
      primary_colors[27] =
      {
          0,   0,   0,
          0,   0, 255,
          0, 255,   0,
          0, 255, 255,
        255, 255, 255,
        255,   0,   0,
        255,   0, 255,
        255, 255,   0,
        128, 128, 128,
      };

    (void) MagickSetImageIndex(magick_wand,2);
    status=MagickSetImagePixels(magick_wand,10,10,3,3,"RGB",CharPixel,
      primary_colors);
    if (status == MagickFalse)
      ThrowAPIException(magick_wand);
    status=MagickGetImagePixels(magick_wand,10,10,3,3,"RGB",CharPixel,pixels);
    if (status == MagickFalse)
      ThrowAPIException(magick_wand);
    for (i=0; i < 9; i++)
      if (pixels[i] != primary_colors[i])
        {
          (void) fprintf(stderr,"Get pixels does not match set pixels\n");
          exit(1);
        }
  }
  (void) MagickSetImageIndex(magick_wand,3);
  status=MagickResizeImage(magick_wand,50,50,UndefinedFilter,1.0);
  if (status == MagickFalse)
    ThrowAPIException(magick_wand);
  (void) MagickSetImageIndex(magick_wand,4);
  (void) fprintf(stdout,"Utilitize pixel iterator to draw diagonal...\n");
  iterator=NewPixelIterator(magick_wand);
  if (iterator == (PixelIterator *) NULL)
    ThrowAPIException(magick_wand);
  pixels=PixelGetNextIteratorRow(iterator,&number_wands);
  for (i=0; pixels != (PixelWand **) NULL; i++)
  {
    (void) PixelSetColor(pixels[i],"#224466");
    (void) PixelSyncIterator(iterator);
    pixels=PixelGetNextIteratorRow(iterator,&number_wands);
  }
  (void) PixelSyncIterator(iterator);
  iterator=DestroyPixelIterator(iterator);
  (void) fprintf(stdout,"Write to image.miff...\n");
  status=MagickWriteImages(magick_wand,"image.miff",MagickTrue);
  if (status == MagickFalse)
    ThrowAPIException(magick_wand);
  (void) fprintf(stdout,"Change image format from \"MIFF\" to \"GIF\"...\n");
  status=MagickSetImageFormat(magick_wand,"GIF");
  if (status == MagickFalse)
     ThrowAPIException(magick_wand);
  (void) fprintf(stdout,"Set delay between frames to %d seconds...\n",
    WandDelay);
  status=MagickSetImageDelay(magick_wand,100*WandDelay);
  if (status == MagickFalse)
    ThrowAPIException(magick_wand);
  delay=MagickGetImageDelay(magick_wand);
  if (delay != (100*WandDelay))
    {
      (void) fprintf( stderr,"Get delay does not match set delay\n");
      exit(1);
    }
  (void) fprintf(stdout,"Write to image_0.gif...\n");
  status=MagickWriteImages(magick_wand,"image.gif",MagickTrue);
  if (status == MagickFalse)
    ThrowAPIException(magick_wand);
  magick_wand=DestroyMagickWand(magick_wand);
  (void) fprintf(stdout,"Wand tests pass.\n");
  return(0);
}
