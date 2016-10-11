#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <magick/ImageMagick.h>

int main(int argc,char **argv)
{
  ExceptionInfo
    exception;

  Image
    *image,
    *images,
    *resize_image,
    *thumbnails;

  ImageInfo
    *image_info;

  /*
    Initialize the image info structure and read an image.
  */
  InitializeMagick(*argv);
  GetExceptionInfo(&exception);
  image_info=CloneImageInfo((ImageInfo *) NULL);
  (void) strcpy(image_info->filename,"image.gif");
  images=ReadImage(image_info,&exception);
  if (exception.severity != UndefinedException)
    CatchException(&exception);
  if (images == (Image *) NULL)
    exit(1);
  /*
    Turn the images into a thumbnail sequence.
  */
  thumbnails=NewImageList();
  while ((image=RemoveFirstImageFromList(&images)) != (Image *) NULL)
  {
    resize_image=ResizeImage(image,106,80,LanczosFilter,1.0,&exception);
    if (resize_image == (Image *) NULL)
      MagickError(exception.severity,exception.reason,exception.description);
    (void) AppendImageToList(&thumbnails,resize_image);
    DestroyImage(image);
  }
  /*
    Write the image as MIFF and destroy it.
  */
  (void) strcpy(thumbnails->filename,"image.png");
  WriteImage(image_info,thumbnails);
  thumbnails=DestroyImageList(thumbnails);
  image_info=DestroyImageInfo(image_info);
  DestroyExceptionInfo(&exception);
  DestroyMagick();
  return(0);
}
