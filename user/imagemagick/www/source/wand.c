  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <time.h>
  #include <wand/magick-wand.h>
  
  int main(int argc,char **argv)
  {
  #define ThrowWandException(wand) \
  { \
    char \
      *description; \
   \
    ExceptionType \
      severity; \
   \
    description=MagickGetException(wand,&severity); \
    (void) fprintf(stderr,"%s %s %ld %s\n",GetMagickModule(),description); \
    description=(char *) MagickRelinquishMemory(description); \
    exit(-1); \
  }
  
    MagickBooleanType
      status;
  
    MagickWand
      *magick_wand;
  
    /*
      Read an image.
    */
    magick_wand=NewMagickWand();  
    status=MagickReadImage(magick_wand,"image.gif");
    if (status == MagickFalse)
      ThrowWandException(magick_wand);
    /*
      Turn the images into a thumbnail sequence.
    */
    MagickResetIterator(magick_wand);
    while (MagickNextImage(magick_wand) != MagickFalse)
      MagickResizeImage(magick_wand,106,80,LanczosFilter,1.0);
    /*
      Write the image as MIFF and destroy it.
    */
    status=MagickWriteImages(magick_wand,"image.png",MagickTrue);
    if (status == MagickFalse)
      ThrowWandException(magick_wand);
    magick_wand=DestroyMagickWand(magick_wand);
    return(0);
  }
