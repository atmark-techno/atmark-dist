/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               M   M   OOO   N   N  TTTTT   AAA    GGGG  EEEEE               %
%               MM MM  O   O  NN  N    T    A   A  G      E                   %
%               M M M  O   O  N N N    T    AAAAA  G  GG  EEE                 %
%               M   M  O   O  N  NN    T    A   A  G   G  E                   %
%               M   M   OOO   N   N    T    A   A   GGG   EEEEE               %
%                                                                             %
%                                                                             %
%                ImageMagick Methods to Create Image Thumbnails               %
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
%
*/

/*
  Include declarations.
*/
#include "magick/studio.h"
#include "magick/annotate.h"
#include "magick/attribute.h"
#include "magick/client.h"
#include "magick/color.h"
#include "magick/composite.h"
#include "magick/constitute.h"
#include "magick/decorate.h"
#include "magick/draw.h"
#include "magick/effect.h"
#include "magick/enhance.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/gem.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/memory_.h"
#include "magick/mogrify.h"
#include "magick/mogrify-private.h"
#include "magick/monitor.h"
#include "magick/montage.h"
#include "magick/option.h"
#include "magick/quantize.h"
#include "magick/resize.h"
#include "magick/resource_.h"
#include "magick/string_.h"
#include "magick/utility.h"
#include "magick/version.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C l o n e M o n t a g e I n f o                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CloneMontageInfo() makes a copy of the given montage info structure.  If
%  NULL is specified, a new image info structure is created initialized to
%  default values.
%
%  The format of the CloneMontageInfo method is:
%
%      MontageInfo *CloneMontageInfo(const ImageInfo *image_info,
%        const MontageInfo *montage_info)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o montage_info: The montage info.
%
%
*/
MagickExport MontageInfo *CloneMontageInfo(const ImageInfo *image_info,
  const MontageInfo *montage_info)
{
  MontageInfo
    *clone_info;

  clone_info=(MontageInfo *) AcquireMagickMemory(sizeof(*clone_info));
  if (clone_info == (MontageInfo *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,
      "UnableToAllocateMontageInfo",image_info->filename);
  GetMontageInfo(image_info,clone_info);
  if (montage_info == (MontageInfo *) NULL)
    return(clone_info);
  if (montage_info->geometry != (char *) NULL)
    clone_info->geometry=AcquireString(montage_info->geometry);
  if (montage_info->tile != (char *) NULL)
    clone_info->tile=AcquireString(montage_info->tile);
  if (montage_info->title != (char *) NULL)
    clone_info->title=AcquireString(montage_info->title);
  if (montage_info->frame != (char *) NULL)
    clone_info->frame=AcquireString(montage_info->frame);
  if (montage_info->texture != (char *) NULL)
    clone_info->texture=AcquireString(montage_info->texture);
  if (montage_info->font != (char *) NULL)
    clone_info->font=AcquireString(montage_info->font);
  clone_info->pointsize=montage_info->pointsize;
  clone_info->border_width=montage_info->border_width;
  clone_info->shadow=montage_info->shadow;
  clone_info->fill=montage_info->fill;
  clone_info->stroke=montage_info->stroke;
  clone_info->background_color=montage_info->background_color;
  clone_info->border_color=montage_info->border_color;
  clone_info->matte_color=montage_info->matte_color;
  clone_info->gravity=montage_info->gravity;
  (void) CopyMagickString(clone_info->filename,montage_info->filename,
    MaxTextExtent);
  clone_info->debug=IsEventLogging();
  return(clone_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y M o n t a g e I n f o                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyMontageInfo() deallocates memory associated with montage_info.
%
%  The format of the DestroyMontageInfo method is:
%
%      MontageInfo *DestroyMontageInfo(MontageInfo *montage_info)
%
%  A description of each parameter follows:
%
%    o montage_info: Specifies a pointer to an MontageInfo structure.
%
%
*/
MagickExport MontageInfo *DestroyMontageInfo(MontageInfo *montage_info)
{
  if (montage_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(montage_info != (MontageInfo *) NULL);
  assert(montage_info->signature == MagickSignature);
  if (montage_info->geometry != (char *) NULL)
    montage_info->geometry=(char *)
      RelinquishMagickMemory(montage_info->geometry);
  if (montage_info->tile != (char *) NULL)
    montage_info->tile=(char *) RelinquishMagickMemory(montage_info->tile);
  if (montage_info->title != (char *) NULL)
    montage_info->title=(char *) RelinquishMagickMemory(montage_info->title);
  if (montage_info->frame != (char *) NULL)
    montage_info->frame=(char *) RelinquishMagickMemory(montage_info->frame);
  if (montage_info->texture != (char *) NULL)
    montage_info->texture=(char *)
      RelinquishMagickMemory(montage_info->texture);
  if (montage_info->font != (char *) NULL)
    montage_info->font=(char *) RelinquishMagickMemory(montage_info->font);
  montage_info->signature=(~MagickSignature);
  montage_info=(MontageInfo *) RelinquishMagickMemory(montage_info);
  return(montage_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M o n t a g e I n f o                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMontageInfo() initializes montage_info to default values.
%
%  The format of the GetMontageInfo method is:
%
%      void GetMontageInfo(const ImageInfo *image_info,
%        MontageInfo *montage_info)
%
%  A description of each parameter follows:
%
%    o image_info: a structure of type ImageInfo.
%
%    o montage_info: Specifies a pointer to a MontageInfo structure.
%
%
*/
MagickExport void GetMontageInfo(const ImageInfo *image_info,
  MontageInfo *montage_info)
{
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  assert(montage_info != (MontageInfo *) NULL);
  (void) ResetMagickMemory(montage_info,0,sizeof(*montage_info));
  (void) CopyMagickString(montage_info->filename,image_info->filename,
    MaxTextExtent);
  montage_info->geometry=AcquireString(DefaultTileGeometry);
  if (image_info->font != (char *) NULL)
    montage_info->font=AcquireString(image_info->font);
  montage_info->gravity=CenterGravity;
  montage_info->pointsize=image_info->pointsize;
  montage_info->fill.opacity=OpaqueOpacity;
  montage_info->stroke.opacity=TransparentOpacity;
  montage_info->background_color=image_info->background_color;
  montage_info->border_color=image_info->border_color;
  montage_info->matte_color=image_info->matte_color;
  montage_info->debug=IsEventLogging();
  montage_info->signature=MagickSignature;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+    M o n t a g e I m a g e C o m m a n d                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MontageImageCommand() reads one or more images, applies one or more image
%  processing operations, and writes out the image in the same or
%  differing format.
%
%  The format of the MontageImageCommand method is:
%
%      MagickBooleanType MontageImageCommand(ImageInfo *image_info,int argc,
%        char **argv,char **metadata,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o argc: The number of elements in the argument vector.
%
%    o argv: A text array containing the command line arguments.
%
%    o metadata: any metadata is returned here.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

static void MontageUsage(void)
{
  const char
    **p;

  static const char
    *options[]=
    {
      "-adjoin              join images into a single multi-image file",
      "-affine matrix       affine transform matrix",
      " annotate geometry text",
      "                     annotate the image with text",
      "-authenticate value  decrypt image with this password",
      "-blue-primary point  chromaticity blue primary point",
      "-blur factor         apply a filter to blur the image",
      "-border geometry     surround image with a border of color",
      "-bordercolor color   border color",
      "-channel type        apply option to select image channels",
      "-clone index         clone an image",
      "-colors value        preferred number of colors in the image",
      "-colorspace type     alternate image colorsapce",
      "-comment string      annotate image with comment",
      "-compose operator    composite operator",
      "-compress type       type of pixel compression when writing the image",
      "-crop geometry       preferred size and location of the cropped image",
      "-debug events        display copious debugging information",
      "-define format:option",
      "                     define one or more image format options",
      "-density geometry    horizontal and vertical density of the image",
      "-depth value         image depth",
      "-display server      query font from this X server",
      "-dispose method      GIF disposal method",
      "-dither              apply Floyd/Steinberg error diffusion to image",
      "-draw string         annotate the image with a graphic primitive",
      "-encoding type       text encoding type",
      "-endian type         endianness (MSB or LSB) of the image",
      "-extract geometry    extract area from image",
      "-fill color          color to use when filling a graphic primitive",
      "-filter type         use this filter when resizing an image",
      "-flip                flip image in the vertical direction",
      "-flop                flop image in the horizontal direction",
      "-font name           render text with this font",
      "-frame geometry      surround image with an ornamental border",
      "-gamma value         level of gamma correction",
      "-geometry geometry   preferred tile and border sizes",
      "-gravity direction   which direction to gravitate towards",
      "-green-primary point chromaticity green primary point",
      "-help                print program options",
      "-interlace type      type of image interlacing scheme",
      "-label name          assign a label to an image",
      "-limit type value    pixel cache resource limit",
      "-log format          format of debugging information",
      "-matte               store matte channel if the image has one",
      "-mattecolor color    frame color",
      "-mode type           framing style",
      "-monitor             monitor progress",
      "-monochrome          transform image to black and white",
      "-page geometry       size and location of an image canvas (setting)",
      "-pointsize value     font point size",
      "-profile filename    add, delete, or apply an image profile",
      "-quality value       JPEG/MIFF/PNG compression level",
      "-quiet               suppress all error or warning messages",
      "-red-primary point   chromaticity red primary point",
      "-repage geometry     size and location of an image canvas (operator)",
      "-resize geometry     resize the image",
      "-rotate degrees      apply Paeth rotation to the image",
      "-sampling-factor geometry",
      "                     horizontal and vertical sampling factor",
      "-scenes range        image scene range",
      "-set attribute value set an image attribute",
      "-shadow              add a shadow beneath a tile to simulate depth",
      "-size geometry       width and height of image",
      "-strip               strip image of all profiles and comments",
      "-stroke color        color to use when stroking a graphic primitive",
      "-support factor      resize support: > 1.0 is blurry, < 1.0 is sharp",
      "-texture filename    name of texture to tile onto the image background",
      "-thumbnail geometry  create a thumbnail of the image",
      "-tile geometry       number of tiles per row and column",
      "-transform           affine transform image",
      "-transparent color   make this color transparent within the image",
      "-treedepth value     color tree depth",
      "-trim                trim image edges",
      "-type type           image type",
      "-verbose             print detailed information about the image",
      "-version             print version information",
      "-virtual-pixel method",
      "                     virtual pixel access method",
      "-white-point point   chromaticity white point",
      (char *) NULL
    };

  (void) printf("Version: %s\n",GetMagickVersion((unsigned long *) NULL));
  (void) printf("Copyright: %s\n\n",GetMagickCopyright());
  (void) printf("Usage: %s [options ...] file [ [options ...] file ...] file\n",
    GetClientName());
  (void) printf("\nWhere options include: \n");
  for (p=options; *p != (char *) NULL; p++)
    (void) printf("  %s\n",*p);
  (void) printf(
    "\nIn addition to those listed above, you can specify these standard X\n");
  (void) printf(
    "resources as command line options:  -background, -bordercolor,\n");
  (void) printf(
    "-borderwidth, -font, -mattecolor, or -title\n");
  (void) printf(
    "\nBy default, the image format of `file' is determined by its magic\n");
  (void) printf(
    "number.  To specify a particular image format, precede the filename\n");
  (void) printf(
    "with an image format name and a colon (i.e. ps:image) or specify the\n");
  (void) printf(
    "image type as the filename suffix (i.e. image.ps).  Specify 'file' as\n");
  (void) printf("'-' for standard input or output.\n");
  exit(0);
}

MagickExport MagickBooleanType MontageImageCommand(ImageInfo *image_info,
  int argc,char **argv,char **metadata,ExceptionInfo *exception)
{
#define DestroyMontage() \
{ \
  if (montage_image != (Image *) NULL) \
    montage_image=DestroyImageList(montage_image); \
  for ( ; k >= 0; k--) \
    image_stack[k]=DestroyImageList(image_stack[k]); \
  for (i=0; i < (long) argc; i++) \
    argv[i]=(char *) RelinquishMagickMemory(argv[i]); \
  argv=(char **) RelinquishMagickMemory(argv); \
}
#define ThrowMontageException(asperity,tag,option) \
{ \
  if (exception->severity == UndefinedException) \
    (void) ThrowMagickException(exception,GetMagickModule(),asperity,tag, \
      "`%s'",option); \
  DestroyMontage(); \
  return(MagickFalse); \
}
#define ThrowMontageInvalidArgumentException(option,argument) \
{ \
  (void) ThrowMagickException(exception,GetMagickModule(),OptionError, \
    "InvalidArgument","`%s': %s",argument,option); \
  DestroyMontage(); \
  return(MagickFalse); \
}

  char
    *option,
    *transparent_color;

  const char
    *format;

  Image
    *image_stack[MaxImageStackDepth+1],
    *montage_image;

  long
    first_scene,
    j,
    k,
    last_scene,
    scene;

  MagickBooleanType
    pend;

  MagickStatusType
    status;

  MontageInfo
    *montage_info;

  QuantizeInfo
    quantize_info;

  register long
    i;

  /*
    Set defaults.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(exception != (ExceptionInfo *) NULL);
  if (argc < 3)
   MontageUsage();
  format="%w,%h,%m";
  first_scene=0;
  j=1;
  k=0;
  image_stack[k]=NewImageList();
  last_scene=0;
  montage_image=NewImageList();
  montage_info=CloneMontageInfo(image_info,(MontageInfo *) NULL);
  option=(char *) NULL;
  pend=MagickFalse;
  GetQuantizeInfo(&quantize_info);
  quantize_info.number_colors=0;
  scene=0;
  status=MagickFalse;
  transparent_color=(char *) NULL;
  /*
    Parse command line.
  */
  ReadCommandlLine(argc,&argv);
  status=ExpandFilenames(&argc,&argv);
  if (status == MagickFalse)
    ThrowMontageException(ResourceLimitError,"MemoryAllocationFailed",
      strerror(errno));
  for (i=1; i < (long) (argc-1); i++)
  {
    option=argv[i];
    if (LocaleCompare(option,"(") == 0)
      {
        if (k == MaxImageStackDepth)
          ThrowMontageException(OptionError,"ParenthesisNestedTooDeeply",
            option);
        MogrifyImageStack(image_stack[k],MagickTrue,pend);
        k++;
        image_stack[k]=NewImageList();
        continue;
      }
    if (LocaleCompare(option,")") == 0)
      {
        if (k == 0)
          ThrowMontageException(OptionError,"UnableToParseExpression",option);
        if (image_stack[k] != (Image *) NULL)
          {
            MogrifyImageStack(image_stack[k],MagickTrue,MagickTrue);
            AppendImageToList(&image_stack[k-1],image_stack[k]);
          }
        k--;
        continue;
      }
    if (IsMagickOption(option) == MagickFalse)
      {
        Image
         *image;

        MogrifyImageStack(image_stack[k],MagickTrue,pend);
        for (scene=first_scene; scene <= last_scene ; scene++)
        {
          /*
            Option is a file name: begin by reading image from specified file.
          */
          (void) CopyMagickString(image_info->filename,argv[i],MaxTextExtent);
          if (first_scene != last_scene)
            {
              char
                filename[MaxTextExtent];

              /*
                Form filename for multi-part images.
              */
              (void) FormatMagickString(filename,MaxTextExtent,
                image_info->filename,scene);
              if (LocaleCompare(filename,image_info->filename) == 0)
                (void) FormatMagickString(filename,MaxTextExtent,"%s.%lu",
                  image_info->filename,scene);
              (void) CopyMagickString(image_info->filename,filename,
                MaxTextExtent);
            }
          (void) CloneString(&image_info->font,montage_info->font);
          image=ReadImage(image_info,exception);
          status&=(image != (Image *) NULL) &&
            (exception->severity < ErrorException);
          if (image == (Image *) NULL)
            continue;
          AppendImageToList(&image_stack[k],image);
        }
        continue;
      }
    pend=image_stack[k] != (Image *) NULL ? MagickTrue : MagickFalse;
    switch (*(option+1))
    {
      case 'a':
      {
        if (LocaleCompare("adjoin",option+1) == 0)
          break;
        if (LocaleCompare("affine",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("annotate",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            if (i == (long) (argc-1))
              ThrowMontageException(OptionError,"MissingArgument",option);
            i++;
            break;
          }
        if (LocaleCompare("authenticate",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'b':
      {
        if (LocaleCompare("background",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            (void) QueryColorDatabase(argv[i],
              &montage_info->background_color,exception);
            break;
          }
        if (LocaleCompare("blue-primary",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("blur",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("border",option+1) == 0)
          {
            montage_info->border_width=0;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            montage_info->border_width=(unsigned long) atol(argv[i]);
            break;
          }
        if (LocaleCompare("bordercolor",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            (void) QueryColorDatabase(argv[i],&montage_info->border_color,
              exception);
            break;
          }
        if (LocaleCompare("borderwidth",option+1) == 0)
          {
            montage_info->border_width=0;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            montage_info->border_width=(unsigned long) atol(argv[i]);
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'c':
      {
        if (LocaleCompare("cache",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("channel",option+1) == 0)
          {
            if (*option == '+')
              {
                long
                  channel;

                i++;
                if (i == (long) (argc-1))
                  ThrowMontageException(OptionError,"MissingArgument",option);
                channel=ParseChannelOption(argv[i]);
                if (channel < 0)
                  ThrowMontageException(OptionError,"UnrecognizedChannelType",
                    argv[i]);
                break;
              }
            break;
          }
        if (LocaleCompare("clone",option+1) == 0)
          {
            Image
              *clone_images;

            clone_images=image_stack[k];
            if (k != 0)
              clone_images=image_stack[k-1];
            if (clone_images == (Image *) NULL)
              ThrowMontageException(ImageError,"ImageSequenceRequired",option);
            if (*option == '+')
              clone_images=CloneImages(clone_images,"-1",exception);
            else
              {
                i++;
                if (i == (long) (argc-1))
                  ThrowMontageException(OptionError,"MissingArgument",option);
                if (IsSceneGeometry(argv[i],MagickFalse) == MagickFalse)
                  ThrowMontageInvalidArgumentException(option,argv[i]);
                clone_images=CloneImages(clone_images,argv[i],exception);
              }
            if (clone_images == (Image *) NULL)
              ThrowMontageException(OptionError,"NoSuchImage",option);
            MogrifyImageStack(image_stack[k],MagickTrue,MagickTrue);
            AppendImageToList(&image_stack[k],clone_images);
            break;
          }
        if (LocaleCompare("colors",option+1) == 0)
          {
            quantize_info.number_colors=0;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            quantize_info.number_colors=(unsigned long) atol(argv[i]);
          }
        if (LocaleCompare("colorspace",option+1) == 0)
          {
            long
              colorspace;

            quantize_info.colorspace=UndefinedColorspace;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            colorspace=ParseMagickOption(MagickColorspaceOptions,
              MagickFalse,argv[i]);
            if (colorspace < 0)
              ThrowMontageException(OptionError,"UnrecognizedColorspace",
                argv[i]);
            quantize_info.colorspace=(ColorspaceType) colorspace;
            if (quantize_info.colorspace == GRAYColorspace)
              {
                quantize_info.colorspace=GRAYColorspace;
                quantize_info.number_colors=256;
                quantize_info.tree_depth=8;
              }
            break;
          }
        if (LocaleCompare("comment",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("compose",option+1) == 0)
          {
            long
              compose;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            compose=ParseMagickOption(MagickCompositeOptions,MagickFalse,
              argv[i]);
            if (compose < 0)
              ThrowMontageException(OptionError,
                "UnrecognizedComposeOperator",argv[i]);
            break;
          }
        if (LocaleCompare("compress",option+1) == 0)
          {
            long
              compression;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            compression=ParseMagickOption(MagickCompressionOptions,
              MagickFalse,argv[i]);
            if (compression < 0)
              ThrowMontageException(OptionError,
                "UnrecognizedCompressionType",argv[i]);
            break;
          }
        if (LocaleCompare("crop",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'd':
      {
        if (LocaleCompare("debug",option+1) == 0)
          {
            LogEventType
              event_mask;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            event_mask=SetLogEventMask(argv[i]);
            if (event_mask == UndefinedEvents)
              ThrowMontageException(OptionError,"UnrecognizedEventType",
                option);
            break;
          }
        if (LocaleCompare("define",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (*option == '+')
              {
                const char
                  *define;

                define=GetImageOption(image_info,argv[i]);
                if (define == (const char *) NULL)
                  ThrowMontageException(OptionError,"NoSuchOption",argv[i]);
                break;
              }
            break;
          }
        if (LocaleCompare("density",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("depth",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("display",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("dispose",option+1) == 0)
          {
            long
              dispose;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            dispose=ParseMagickOption(MagickDisposeOptions,MagickFalse,argv[i]);
            if (dispose < 0)
              ThrowMontageException(OptionError,"UnrecognizedDisposeMethod",
                argv[i]);
            break;
          }
        if (LocaleCompare("dither",option+1) == 0)
          {
            quantize_info.dither=(MagickBooleanType) (*option == '-');
            break;
          }
        if (LocaleCompare("draw",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'e':
      {
        if (LocaleCompare("encoding",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("endian",option+1) == 0)
          {
            long
              endian;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            endian=ParseMagickOption(MagickEndianOptions,MagickFalse,
              argv[i]);
            if (endian < 0)
              ThrowMontageException(OptionError,"UnrecognizedEndianType",
                argv[i]);
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'f':
      {
        if (LocaleCompare("fill",option+1) == 0)
          {
            (void) QueryColorDatabase("none",&montage_info->fill,exception);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            (void) QueryColorDatabase(argv[i],&montage_info->fill,
              exception);
            break;
          }
        if (LocaleCompare("filter",option+1) == 0)
          {
            long
              filter;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            filter=ParseMagickOption(MagickFilterOptions,MagickFalse,
              argv[i]);
            if (filter < 0)
              ThrowMontageException(OptionError,"UnrecognizedImageFilter",
                argv[i]);
            break;
          }
        if (LocaleCompare("flip",option+1) == 0)
          break;
        if (LocaleCompare("flop",option+1) == 0)
          break;
        if (LocaleCompare("font",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            (void) CloneString(&montage_info->font,argv[i]);
            break;
          }
        if (LocaleCompare("format",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            format=argv[i];
            break;
          }
        if (LocaleCompare("frame",option+1) == 0)
          {
            (void) strcpy(argv[i]+1,"{1}");
            (void) CloneString(&montage_info->frame,(char *) NULL);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            (void) CloneString(&montage_info->frame,argv[i]);
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'g':
      {
        if (LocaleCompare("gamma",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("geometry",option+1) == 0)
          {
            (void) CloneString(&montage_info->geometry,(char *) NULL);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            (void) CloneString(&montage_info->geometry,argv[i]);
            break;
          }
        if (LocaleCompare("gravity",option+1) == 0)
          {
            long
              gravity;

            montage_info->gravity=UndefinedGravity;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            gravity=ParseMagickOption(MagickGravityOptions,MagickFalse,
              argv[i]);
            if (gravity < 0)
              ThrowMontageException(OptionError,"UnrecognizedGravityType",
                argv[i]);
            montage_info->gravity=(GravityType) gravity;
            break;
          }
        if (LocaleCompare("green-primary",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'h':
      {
        if (LocaleCompare("help",option+1) == 0)
          MontageUsage();
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'i':
      {
        if (LocaleCompare("interlace",option+1) == 0)
          {
            long
              interlace;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            interlace=ParseMagickOption(MagickInterlaceOptions,MagickFalse,
              argv[i]);
            if (interlace < 0)
              ThrowMontageException(OptionError,"UnrecognizedInterlaceType",
                argv[i]);
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'l':
      {
        if (LocaleCompare("label",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("limit",option+1) == 0)
          {
            long
              resource;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            resource=ParseMagickOption(MagickResourceOptions,MagickFalse,
              argv[i]);
            if (resource < 0)
              ThrowMontageException(OptionError,"UnrecognizedResourceType",
                argv[i]);
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if ((LocaleCompare("unlimited",argv[i]) != 0) &&
                (IsGeometry(argv[i]) == MagickFalse))
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("log",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if ((i == (long) argc) ||
                (strchr(argv[i],'%') == (char *) NULL))
              ThrowMontageException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'm':
      {
        if (LocaleCompare("matte",option+1) == 0)
          break;
        if (LocaleCompare("mattecolor",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            (void) QueryColorDatabase(argv[i],&montage_info->matte_color,
              exception);
            break;
          }
        if (LocaleCompare("mode",option+1) == 0)
          {
            MontageMode
              mode;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            mode=UndefinedMode;
            if (LocaleCompare("frame",argv[i]) == 0)
              {
                mode=FrameMode;
                (void) CloneString(&montage_info->frame,"15x15+3+3");
                montage_info->shadow=MagickTrue;
                break;
              }
            if (LocaleCompare("unframe",argv[i]) == 0)
              {
                mode=UnframeMode;
                montage_info->frame=(char *) NULL;
                montage_info->shadow=MagickFalse;
                montage_info->border_width=0;
                break;
              }
            if (LocaleCompare("concatenate",argv[i]) == 0)
              {
                mode=ConcatenateMode;
                montage_info->frame=(char *) NULL;
                montage_info->shadow=MagickFalse;
                montage_info->gravity=(GravityType) NorthWestGravity;
                (void) CloneString(&montage_info->geometry,"+0+0");
                montage_info->border_width=0;
                break;
              }
            if (mode == UndefinedMode)
              ThrowMontageException(OptionError,"UnrecognizedImageMode",
                argv[i]);
            break;
          }
        if (LocaleCompare("monitor",option+1) == 0)
          break;
        if (LocaleCompare("monochrome",option+1) == 0)
          {
            if (*option == '+')
              break;
            quantize_info.number_colors=2;
            quantize_info.tree_depth=8;
            quantize_info.colorspace=GRAYColorspace;
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'n':
      {
        if (LocaleCompare("noop",option+1) == 0)
          break;
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'p':
      {
        if (LocaleCompare("page",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("pointsize",option+1) == 0)
          {
            montage_info->pointsize=12;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            montage_info->pointsize=atof(argv[i]);
            break;
          }
        if (LocaleCompare("profile",option+1) == 0)
          {
            i++;
            if (i == (long) (argc-1))
              ThrowMontageException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'q':
      {
        if (LocaleCompare("quality",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("quiet",option+1) == 0)
          break;
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'r':
      {
        if (LocaleCompare("red-primary",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("render",option+1) == 0)
          break;
        if (LocaleCompare("repage",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("resize",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("rotate",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 's':
      {
        if (LocaleCompare("sampling-factor",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("scenes",option+1) == 0)
          {
            first_scene=0;
            last_scene=0;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            first_scene=atol(argv[i]);
            last_scene=first_scene;
            (void) sscanf(argv[i],"%ld-%ld",&first_scene,&last_scene);
            break;
          }
        if (LocaleCompare("set",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("shadow",option+1) == 0)
          {
            montage_info->shadow=(MagickBooleanType) (*option == '-');
            (void) strcpy(argv[i]+1,"{0}");
            break;
          }
        if (LocaleCompare("sharpen",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if ((i == (long) argc) || (IsGeometry(argv[i]) == MagickFalse))
              ThrowMontageException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("size",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("stroke",option+1) == 0)
          {
            (void) QueryColorDatabase("none",&montage_info->stroke,exception);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            (void) QueryColorDatabase(argv[i],&montage_info->stroke,
              exception);
            break;
          }
        if (LocaleCompare("strip",option+1) == 0)
          break;
        if (LocaleCompare("strokewidth",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("support",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 't':
      {
        if (LocaleCompare("texture",option+1) == 0)
          {
            (void) CloneString(&montage_info->texture,(char *) NULL);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            (void) CloneString(&montage_info->texture,argv[i]);
            break;
          }
        if (LocaleCompare("thumbnail",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("tile",option+1) == 0)
          {
            (void) strcpy(argv[i]+1,"{1}");
            (void) CloneString(&montage_info->tile,(char *) NULL);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            (void) CloneString(&montage_info->tile,argv[i]);
            break;
          }
        if (LocaleCompare("tint",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("transform",option+1) == 0)
          break;
        if (LocaleCompare("title",option+1) == 0)
          {
            (void) CloneString(&montage_info->title,(char *) NULL);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            (void) CloneString(&montage_info->title,argv[i]);
            break;
          }
        if (LocaleCompare("transform",option+1) == 0)
          break;
        if (LocaleCompare("transparent",option+1) == 0)
          {
            transparent_color=(char *) NULL;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            (void) CloneString(&transparent_color,argv[i]);
            break;
          }
        if (LocaleCompare("treedepth",option+1) == 0)
          {
            quantize_info.tree_depth=0;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            quantize_info.tree_depth=(unsigned long) atol(argv[i]);
            break;
          }
        if (LocaleCompare("trim",option+1) == 0)
          break;
        if (LocaleCompare("type",option+1) == 0)
          {
            long
              type;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            type=ParseMagickOption(MagickImageOptions,MagickFalse,argv[i]);
            if (type < 0)
              ThrowMontageException(OptionError,"UnrecognizedImageType",
                argv[i]);
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'v':
      {
        if (LocaleCompare("verbose",option+1) == 0)
          {
            break;
          }
        if (LocaleCompare("version",option+1) == 0)
          break;
        if (LocaleCompare("virtual-pixel",option+1) == 0)
          {
            long
              method;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            method=ParseMagickOption(MagickVirtualPixelOptions,MagickFalse,
              argv[i]);
            if (method < 0)
              ThrowMontageException(OptionError,
                "UnrecognizedVirtualPixelMethod",argv[i]);
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case 'w':
      {
        if (LocaleCompare("white-point",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMontageException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMontageInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
      }
      case '?':
        break;
      default:
        ThrowMontageException(OptionError,"UnrecognizedOption",option)
    }
    status=(MagickStatusType)
      ParseMagickOption(MagickMogrifyOptions,MagickFalse,option+1);
    if (status == MagickTrue)
      MogrifyImageStack(image_stack[k],MagickTrue,MagickTrue);
  }
  if (k != 0)
    ThrowMontageException(OptionError,"UnbalancedParenthesis",argv[i]);
  if (i != (long) (argc-1))
    ThrowMontageException(OptionError,"MissingAnImageFilename",argv[i]);
  if (image_stack[k] == (Image *) NULL)
    ThrowMontageException(OptionError,"MissingAnImageFilename",argv[argc-1]);
  MogrifyImageStack(image_stack[k],MagickTrue,MagickTrue);
  (void) CopyMagickString(montage_info->filename,argv[argc-1],MaxTextExtent);
  montage_image=MontageImages(image_stack[k],montage_info,exception);
  if (montage_image != (Image *) NULL)
    {
      /*
        Write image.
      */
      GetImageException(montage_image,exception);
      (void) CopyMagickString(image_info->filename,argv[argc-1],MaxTextExtent);
      (void) CopyMagickString(montage_image->magick_filename,argv[argc-1],
        MaxTextExtent);
      status&=WriteImages(image_info,montage_image,argv[argc-1],exception);
      if (metadata != (char **) NULL)
        {
          char
            *text;

          text=TranslateText(image_info,montage_image,format);
          if (text == (char *) NULL)
            ThrowMontageException(ResourceLimitError,"MemoryAllocationFailed",
              strerror(errno));
          (void) ConcatenateString(&(*metadata),text);
          (void) ConcatenateString(&(*metadata),"\n");
          text=(char *) RelinquishMagickMemory(text);
        }
    }
  montage_info=DestroyMontageInfo(montage_info);
  DestroyMontage();
  return(status != 0 ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   M o n t a g e I m a g e s                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Montageimages() is a layout manager that lets you tile one or more
%  thumbnails across an image canvas.
%
%  The format of the MontageImages method is:
%
%      Image *MontageImages(const Image *images,const MontageInfo *montage_info,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o images: Specifies a pointer to an array of Image structures.
%
%    o montage_info: Specifies a pointer to a MontageInfo structure.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

static void GetMontageGeometry(char *geometry,const unsigned long number_images,
  long *x_offset,long *y_offset,unsigned long *tiles_per_column,
  unsigned long *tiles_per_row)
{
  *tiles_per_column=0;
  *tiles_per_row=0;
  (void) GetGeometry(geometry,x_offset,y_offset,tiles_per_row,tiles_per_column);
  if ((*tiles_per_column == 0) && (*tiles_per_row == 0))
    *tiles_per_column=(unsigned long) sqrt((double) number_images);
  if (*tiles_per_column == 0)
    *tiles_per_column=(unsigned long)
      ceil((double) number_images/(*tiles_per_row));
  if (*tiles_per_row == 0)
    *tiles_per_row=(unsigned long)
      ceil((double) number_images/(*tiles_per_column));
}

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int SceneCompare(const void *x,const void *y)
{
  Image
    **image_1,
    **image_2;

  image_1=(Image **) x;
  image_2=(Image **) y;
  return((int) ((*image_1)->scene-(*image_2)->scene));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport Image *MontageImages(const Image *images,
  const MontageInfo *montage_info,ExceptionInfo *exception)
{
#define MontageImageTag  "Montage/Image"
#define TileImageTag  "Tile/Image"

  char
    tile_geometry[MaxTextExtent],
    *title;

  const ImageAttribute
    *attribute;

  DrawInfo
    *draw_info;

  FrameInfo
    frame_info;

  Image
    *image,
    **image_list,
    **master_list,
    *montage,
    *texture,
    *tile_image,
    *thumbnail;

  ImageInfo
    *image_info;

  long
    x,
    x_offset,
    y,
    y_offset;

  MagickBooleanType
    concatenate,
    status;

  MagickOffsetType
    count,
    tiles;

  MagickStatusType
    flags;

  MagickProgressMonitor
    progress_monitor;

  register long
    i;

  register PixelPacket
    *q;

  RectangleInfo
    bounds,
    geometry,
    extract_info;

  TypeMetric
    metrics;

  unsigned long
    bevel_width,
    border_width,
    height,
    images_per_page,
    max_height,
    number_images,
    number_lines,
    sans,
    tile,
    tiles_per_column,
    tiles_per_page,
    tiles_per_row,
    title_offset,
    total_tiles,
    width;

  /*
    Create image tiles.
  */
  assert(images != (Image *) NULL);
  assert(images->signature == MagickSignature);
  if (images->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",images->filename);
  assert(montage_info != (MontageInfo *) NULL);
  assert(montage_info->signature == MagickSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  number_images=GetImageListLength(images);
  master_list=ImageListToArray(images,exception);
  image_list=master_list;
  image=image_list[0];
  if (master_list == (Image **) NULL)
    ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
  thumbnail=NewImageList();
  for (i=0; i < (long) number_images; i++)
  {
    image=image_list[i];
    progress_monitor=SetImageProgressMonitor(image,(MagickProgressMonitor) NULL,
      image->client_data);
    flags=ParseSizeGeometry(image,montage_info->geometry,&geometry);
    if (image->filter == UndefinedFilter)
      thumbnail=ThumbnailImage(image,geometry.width,geometry.height,exception);
    else
      thumbnail=ZoomImage(image,geometry.width,geometry.height,exception);
    if (thumbnail == (Image *) NULL)
      break;
    image_list[i]=thumbnail;
    (void) SetImageProgressMonitor(image,progress_monitor,image->client_data);
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(i,number_images) != MagickFalse))
      {
        status=image->progress_monitor(TileImageTag,i,number_images,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  if (i < (long) number_images)
    {
      if (thumbnail == (Image *) NULL)
        i--;
      for (tile=0; (long) tile <= i; tile++)
        if (image_list[tile] != (Image *) NULL)
          image_list[tile]=DestroyImage(image_list[tile]);
      master_list=(Image **) RelinquishMagickMemory(master_list);
      return((Image *) NULL);
    }
  /*
    Sort image list by increasing tile number.
  */
  for (i=0; i < (long) number_images; i++)
    if (image_list[i]->scene == 0)
      break;
  if (i == (long) number_images)
    qsort((void *) image_list,(size_t) number_images,sizeof(*image_list),
      SceneCompare);
  /*
    Determine tiles per row and column.
  */
  tiles_per_column=(unsigned long) sqrt((double) number_images);
  tiles_per_row=(unsigned long) ceil((double) number_images/tiles_per_column);
  x_offset=0;
  y_offset=0;
  if (montage_info->tile != (char *) NULL)
    GetMontageGeometry(montage_info->tile,number_images,&x_offset,&y_offset,
      &tiles_per_column,&tiles_per_row);
  /*
    Determine tile sizes.
  */
  concatenate=MagickFalse;
  SetGeometry(image_list[0],&extract_info);
  extract_info.x=(long) montage_info->border_width;
  extract_info.y=(long) montage_info->border_width;
  if (montage_info->geometry != (char *) NULL)
    {
      /*
        Initialize tile geometry.
      */
      flags=GetGeometry(montage_info->geometry,&extract_info.x,&extract_info.y,
        &extract_info.width,&extract_info.height);
      if ((extract_info.x == 0) && (extract_info.y == 0))
        concatenate=(MagickBooleanType)
          (((flags & RhoValue) == 0) && ((flags & SigmaValue) == 0));
    }
  border_width=montage_info->border_width;
  bevel_width=0;
  if (montage_info->frame != (char *) NULL)
    {
      char
        absolute_geometry[MaxTextExtent];

      (void) ResetMagickMemory(&frame_info,0,sizeof(frame_info));
      frame_info.width=extract_info.width;
      frame_info.height=extract_info.height;
      (void) FormatMagickString(absolute_geometry,MaxTextExtent,"%s!",
        montage_info->frame);
      flags=ParseMetaGeometry(absolute_geometry,&frame_info.outer_bevel,
        &frame_info.inner_bevel,&frame_info.width,&frame_info.height);
      if ((flags & HeightValue) == 0)
        frame_info.height=frame_info.width;
      if ((flags & XiValue) == 0)
        frame_info.outer_bevel=(long) frame_info.width/3;
      if ((flags & PsiValue) == 0)
        frame_info.inner_bevel=frame_info.outer_bevel;
      frame_info.x=(long) frame_info.width;
      frame_info.y=(long) frame_info.height;
      bevel_width=(unsigned long)
        Max(frame_info.inner_bevel,frame_info.outer_bevel);
      border_width=(unsigned long) Max(frame_info.width,frame_info.height);
    }
  for (i=0; i < (long) number_images; i++)
  {
    if (image_list[i]->columns > extract_info.width)
      extract_info.width=image_list[i]->columns;
    if (image_list[i]->rows > extract_info.height)
      extract_info.height=image_list[i]->rows;
  }
  /*
    Initialize draw attributes.
  */
  image_info=CloneImageInfo((ImageInfo *) NULL);
  image_info->background_color=montage_info->background_color;
  image_info->border_color=montage_info->border_color;
  draw_info=CloneDrawInfo(image_info,(DrawInfo *) NULL);
  if (montage_info->font != (char *) NULL)
    (void) CloneString(&draw_info->font,montage_info->font);
  draw_info->pointsize=montage_info->pointsize;
  draw_info->gravity=CenterGravity;
  draw_info->stroke=montage_info->stroke;
  draw_info->fill=montage_info->fill;
  draw_info->text=AcquireString("");
  (void) GetTypeMetrics(image_list[0],draw_info,&metrics);
  texture=NewImageList();
  if (montage_info->texture != (char *) NULL)
    {
      (void) CopyMagickString(image_info->filename,montage_info->texture,
        MaxTextExtent);
      texture=ReadImage(image_info,exception);
    }
  /*
    Determine the number of lines in an next label.
  */
  title=TranslateText(image_info,image_list[0],montage_info->title);
  title_offset=0;
  if (montage_info->title != (char *) NULL)
    title_offset=(unsigned long) (2*(metrics.ascent-metrics.descent)*
      MultilineCensus(title)+2*extract_info.y);
  number_lines=0;
  for (i=0; i < (long) number_images; i++)
  {
    attribute=GetImageAttribute(image_list[i],"label");
    if (attribute == (ImageAttribute *) NULL)
      continue;
    if (MultilineCensus(attribute->value) > number_lines)
      number_lines=MultilineCensus(attribute->value);
  }
  /*
    Allocate next structure.
  */
  tile_image=AllocateImage(NULL);
  montage=AllocateImage(image_info);
  montage->scene=0;
  images_per_page=(number_images-1)/(tiles_per_row*tiles_per_column)+1;
  tiles=0;
  total_tiles=(unsigned long) number_images;
  for (i=0; i < (long) images_per_page; i++)
  {
    /*
      Determine bounding box.
    */
    tiles_per_page=tiles_per_row*tiles_per_column;
    x_offset=0;
    y_offset=0;
    if (montage_info->tile != (char *) NULL)
      GetMontageGeometry(montage_info->tile,number_images,&x_offset,&y_offset,
        &sans,&sans);
    tiles_per_page=tiles_per_row*tiles_per_column;
    y_offset+=(long) title_offset;
    max_height=0;
    bounds.width=0;
    bounds.height=0;
    width=0;
    for (tile=0; tile < tiles_per_page; tile++)
    {
      if (tile < number_images)
        {
          width=concatenate != MagickFalse ? image_list[tile]->columns :
            extract_info.width;
          if (image_list[tile]->rows > max_height)
            max_height=image_list[tile]->rows;
        }
      x_offset+=width+(extract_info.x+border_width)*2;
      if (x_offset > (long) bounds.width)
        bounds.width=(unsigned long) x_offset;
      if (((tile+1) == tiles_per_page) || (((tile+1) % tiles_per_row) == 0))
        {
          x_offset=0;
          if (montage_info->tile != (char *) NULL)
            GetMontageGeometry(montage_info->tile,number_images,&x_offset,&y,
              &sans,&sans);
          height=concatenate != MagickFalse ? max_height : extract_info.height;
          y_offset+=(unsigned long) (height+(extract_info.y+border_width)*2+
            (metrics.ascent-metrics.descent+4)*number_lines+
            (montage_info->shadow != MagickFalse ? 4 : 0));
          if (y_offset > (long) bounds.height)
            bounds.height=(unsigned long) y_offset;
          max_height=0;
        }
    }
    if (montage_info->shadow != MagickFalse)
      bounds.width+=4;
    /*
      Initialize montage image.
    */
    (void) CopyMagickString(montage->filename,montage_info->filename,
      MaxTextExtent);
    montage->columns=bounds.width;
    montage->rows=bounds.height;
    SetImageBackgroundColor(montage);
    /*
      Set montage geometry.
    */
    montage->montage=AcquireString((char *) NULL);
    count=1;
    for (tile=0; tile < Min(tiles_per_page,number_images); tile++)
      count+=strlen(image_list[tile]->filename)+1;
    montage->directory=(char *) AcquireMagickMemory((size_t) count);
    if ((montage->montage == (char *) NULL) ||
        (montage->directory == (char *) NULL))
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    x_offset=0;
    y_offset=0;
    if (montage_info->tile != (char *) NULL)
      GetMontageGeometry(montage_info->tile,number_images,&x_offset,&y_offset,
        &sans,&sans);
    y_offset+=(long) title_offset;
    (void) FormatMagickString(montage->montage,MaxTextExtent,"%ldx%ld%+ld%+ld",
      (long) (extract_info.width+(extract_info.x+border_width)*2),
      (long) (extract_info.height+(extract_info.y+border_width)*2+
      (metrics.ascent-metrics.descent+4)*number_lines+
      (montage_info->shadow != MagickFalse ? 4 : 0)),x_offset,y_offset);
    *montage->directory='\0';
    for (tile=0; tile < Min(tiles_per_page,number_images); tile++)
    {
      (void) ConcatenateMagickString(montage->directory,
        image_list[tile]->filename,MaxTextExtent);
      (void) ConcatenateMagickString(montage->directory,"\n",MaxTextExtent);
    }
    progress_monitor=SetImageProgressMonitor(montage,
      (MagickProgressMonitor) NULL,montage->client_data);
    if (texture != (Image *) NULL)
      (void) TextureImage(montage,texture);
    if (montage_info->title != (char *) NULL)
      {
        char
          geometry[MaxTextExtent];

        DrawInfo
          *clone_info;

        /*
          Annotate composite image with title.
        */
        clone_info=CloneDrawInfo(image_info,draw_info);
        (void) FormatMagickString(geometry,MaxTextExtent,"%lux%lu%+ld%+ld",
          montage->columns,(unsigned long) (2*(metrics.ascent-metrics.descent)),
          0L,(long) (extract_info.y+4));
        (void) CloneString(&clone_info->geometry,geometry);
        (void) CloneString(&clone_info->text,title);
        clone_info->pointsize*=2.0;
        (void) AnnotateImage(montage,clone_info);
        clone_info=DestroyDrawInfo(clone_info);
      }
    (void) SetImageProgressMonitor(montage,progress_monitor,
      montage->client_data);
    /*
      Copy tile to the composite.
    */
    x_offset=0;
    y_offset=0;
    if (montage_info->tile != (char *) NULL)
      GetMontageGeometry(montage_info->tile,number_images,&x_offset,&y_offset,
        &sans,&sans);
    x_offset+=extract_info.x;
    y_offset+=(long) title_offset+extract_info.y;
    max_height=0;
    for (tile=0; tile < Min(tiles_per_page,number_images); tile++)
    {
      /*
        Copy this tile to the composite.
      */
      image=CloneImage(image_list[tile],0,0,MagickTrue,exception);
      progress_monitor=SetImageProgressMonitor(image,
        (MagickProgressMonitor) NULL,image->client_data);
      width=concatenate != MagickFalse ? image->columns : extract_info.width;
      if (image->rows > max_height)
        max_height=image->rows;
      height=concatenate != MagickFalse ? max_height : extract_info.height;
      if (border_width != 0)
        {
          Image
            *border_image;

          RectangleInfo
            border_info;

          /*
            Put a border around the image.
          */
          border_info.width=border_width;
          border_info.height=border_width;
          if (montage_info->frame != (char *) NULL)
            {
              border_info.width=(width-image->columns+1)/2;
              border_info.height=(height-image->rows+1)/2;
            }
          border_image=BorderImage(image,&border_info,exception);
          if (border_image != (Image *) NULL)
            {
              image=DestroyImage(image);
              image=border_image;
            }
          if ((montage_info->frame != (char *) NULL) &&
              (image->compose == DstOutCompositeOp))
            (void) NegateImageChannel(image,OpacityChannel,MagickFalse);
        }
      /*
        Gravitate as specified by the tile gravity.
      */
      tile_image->columns=width;
      tile_image->rows=height;
      tile_image->gravity=montage_info->gravity;
      if (image->gravity != UndefinedGravity)
        tile_image->gravity=image->gravity;
      (void) FormatMagickString(tile_geometry,MaxTextExtent,"%lux%lu+0+0",
        image->columns,image->rows);
      flags=ParseGravityGeometry(tile_image,tile_geometry,&geometry);
      x=(long) (geometry.x+border_width);
      y=(long) (geometry.y+border_width);
      if ((montage_info->frame != (char *) NULL) && (bevel_width != 0))
        {
          FrameInfo
            extract_info;

          Image
            *frame_image;

          /*
            Put an ornamental border around this tile.
          */
          extract_info=frame_info;
          extract_info.width=width+2*frame_info.width;
          extract_info.height=height+2*frame_info.height;
          attribute=GetImageAttribute(image,"label");
          if (attribute != (const ImageAttribute *) NULL)
            extract_info.height+=(unsigned long) ((metrics.ascent-
              metrics.descent+4)*MultilineCensus(attribute->value));
          frame_image=FrameImage(image,&extract_info,exception);
          if (frame_image != (Image *) NULL)
            {
              image=DestroyImage(image);
              image=frame_image;
            }
          x=0;
          y=0;
        }
      if (LocaleCompare(image->magick,"NULL") != 0)
        {
          /*
            Composite background with tile.
          */
          if (montage_info->shadow != MagickFalse)
            {
              register long
                columns,
                rows;

              /*
                Put a shadow under the tile to show depth.
              */
              for (rows=0; rows < (long) (image->rows-4); rows++)
              {
                q=GetImagePixels(montage,(long) (x_offset+x+image->columns),
                  (long) (y_offset+y+rows+4),(unsigned long)
                  Min(extract_info.x,4),1);
                if (q == (PixelPacket *) NULL)
                  break;
                for (columns=0; columns < Min(extract_info.x,4); columns++)
                {
                  ModulateHSB(100.0,100.0,53.0,&q->red,&q->green,&q->blue);
                  q++;
                }
                if (SyncImagePixels(montage) == MagickFalse)
                  break;
              }
              for (rows=0; rows < (long) Min(extract_info.y,4); rows++)
              {
                q=GetImagePixels(montage,(long) (x_offset+x+Min(extract_info.x,
                  4)),(long) (y_offset+y+image->rows+rows),image->columns,1);
                if (q == (PixelPacket *) NULL)
                  break;
                for (columns=0; columns < (long) image->columns; columns++)
                {
                  ModulateHSB(100.0,100.0,53.0,&q->red,&q->green,&q->blue);
                  q++;
                }
                if (SyncImagePixels(montage) == MagickFalse)
                  break;
              }
            }
          (void) CompositeImage(montage,OverCompositeOp,image,x_offset+x,
            y_offset+y);
          attribute=GetImageAttribute(image,"label");
          if (attribute != (const ImageAttribute *) NULL)
            {
              char
                geometry[MaxTextExtent];

              /*
                Annotate composite tile with label.
              */
              (void) FormatMagickString(geometry,MaxTextExtent,
                "%lux%lu%+ld%+ld",(montage_info->frame ? image->columns :
                width)-2*border_width,(unsigned long) (metrics.ascent-
                metrics.descent+4)*MultilineCensus(attribute->value),x_offset+
                border_width,(montage_info->frame ? y_offset+height+
                border_width+4 : y_offset+extract_info.height+border_width+
                (montage_info->shadow != MagickFalse ? 4 : 0)));
              (void) CloneString(&draw_info->geometry,geometry);
              (void) CloneString(&draw_info->text,attribute->value);
              (void) AnnotateImage(montage,draw_info);
            }
        }
      x_offset+=width+(extract_info.x+border_width)*2;
      if (((tile+1) == tiles_per_page) || (((tile+1) % tiles_per_row) == 0))
        {
          x_offset=extract_info.x;
          y_offset+=(unsigned long) (height+(extract_info.y+border_width)*2+
            (metrics.ascent-metrics.descent+4)*number_lines+
            (montage_info->shadow != MagickFalse ? 4 : 0));
          max_height=0;
        }
      if ((images->progress_monitor != (MagickProgressMonitor) NULL) &&
          (QuantumTick(tiles,total_tiles) != MagickFalse))
        {
          status=images->progress_monitor(MontageImageTag,tiles,total_tiles,
            images->client_data);
          if (status == MagickFalse)
            break;
        }
      image=DestroyImage(image);
      image_list[tile]=DestroyImage(image_list[tile]);
      tiles++;
    }
    if ((i+1) < (long) images_per_page)
      {
        /*
          Allocate next image structure.
        */
        AllocateNextImage(image_info,montage);
        if (GetNextImageInList(montage) == (Image *) NULL)
          {
            montage=DestroyImageList(montage);
            return((Image *) NULL);
          }
        montage=GetNextImageInList(montage);
        image_list+=tiles_per_page;
        number_images-=tiles_per_page;
      }
  }
  tile_image=DestroyImage(tile_image);
  if (texture != (Image *) NULL)
    texture=DestroyImage(texture);
  master_list=(Image **) RelinquishMagickMemory(master_list);
  draw_info=DestroyDrawInfo(draw_info);
  image_info=DestroyImageInfo(image_info);
  while (GetPreviousImageInList(montage) != (Image *) NULL)
    montage=GetPreviousImageInList(montage);
  return(montage);
}
