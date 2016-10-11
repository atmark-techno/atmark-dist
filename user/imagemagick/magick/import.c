/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                 IIIII  M   M  PPPP    OOO   RRRR   TTTTT                    %
%                   I    MM MM  P   P  O   O  R   R    T                      %
%                   I    M M M  PPPP   O   O  RRRR     T                      %
%                   I    M   M  P      O   O  R R      T                      %
%                 IIIII  M   M  P       OOO   R  R     T                      %
%                                                                             %
%                                                                             %
%               Import image to a machine independent format.                 %
%                                                                             %
%                           Software Design                                   %
%                             John Cristy                                     %
%                              July 1992                                      %
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
%  Import is an X Window System window dumping utility.  Import allows X
%  users to store window images in a specially formatted dump file.  This
%  file can then be read by the Display utility for redisplay, printing,
%  editing, formatting, archiving, image processing, etc.  The target
%  window can be specified by id or name or be selected by clicking the
%  mouse in the desired window.  The keyboard bell is rung once at the
%  beginning of the dump and twice when the dump is completed.
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
#include "magick/constitute.h"
#include "magick/decorate.h"
#include "magick/draw.h"
#include "magick/effect.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/gem.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/import.h"
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
#include "magick/xwindow.h"
#include "magick/xwindow-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I m p o r t I m a g e C o m m a n d                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ImportImageCommand() reads an image from any visible window on an X server
%  and outputs it as an image file. You can capture a single window, the
%  entire screen, or any rectangular portion of the screen.  You can use the
%  display utility for redisplay, printing, editing, formatting, archiving,
%  image processing, etc. of the captured image.</dd>
%
%  The target window can be specified by id, name, or may be selected by
%  clicking the mouse in the desired window. If you press a button and then
%  drag, a rectangle will form which expands and contracts as the mouse moves.
%  To save the portion of the screen defined by the rectangle, just release
%  the button. The keyboard bell is rung once at the beginning of the screen
%  capture and twice when it completes.
%
%  The format of the ImportImageCommand method is:
%
%      MagickBooleanType ImportImageCommand(ImageInfo *image_info,int argc,
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

static void ImportUsage(void)
{
  const char
    **p;

  static const char
    *options[]=
    {
      "-adjoin              join images into a single multi-image file",
      "-annotate geometry text",
      "                     annotate the image with text",
      "-border              include image borders in the output image",
      "-channel type        apply option to select image channels",
      "-colors value        preferred number of colors in the image",
      "-colorspace type     alternate image colorspace",
      "-comment string      annotate image with comment",
      "-compress type       type of pixel compression when writing the image",
      "-crop geometry       preferred size and location of the cropped image",
      "-debug events        display copious debugging information",
      "-define format:option",
      "                     define one or more image format options",
      "-delay value         display the next image after pausing",
      "-density geometry    horizontal and vertical density of the image",
      "-depth value         image depth",
      "-descend             obtain image by descending window hierarchy",
      "-display server      X server to contact",
      "-dispose method      GIF disposal method",
      "-dither              apply Floyd/Steinberg error diffusion to image",
      "-encoding type       text encoding type",
      "-endian type         endianness (MSB or LSB) of the image",
      "-frame               include window manager frame",
      "-geometry geometry   perferred size or location of the image",
      "-gravity direction   which direction to gravitate towards",
      "-help                print program options",
      "-interlace type      None, Line, Plane, or Partition",
      "-label name          assign a label to an image",
      "-limit type value    Area, Disk, Map, or Memory resource limit",
      "-log format          format of debugging information",
      "-monitor             monitor progress",
      "-monochrome          transform image to black and white",
      "-negate              replace every pixel with its complementary color ",
      "-page geometry       size and location of an image canvas",
      "-pause value         seconds delay between snapshots",
      "-pointsize value     font point size",
      "-quality value       JPEG/MIFF/PNG compression level",
      "-quiet               suppress all error or warning messages",
      "-repage geometry     size and location of an image canvas",
      "-resize geometry     resize the image",
      "-rotate degrees      apply Paeth rotation to the image",
      "-sampling-factor geometry",
      "                     horizontal and vertical sampling factor",
      "-scene value         image scene number",
      "-screen              select image from root window",
      "-set attribute value set an image attribute",
      "-silent              operate silently, i.e. don't ring any bells ",
      "-snaps value         number of screen snapshots",
      "-strip               strip image of all profiles and comments",
      "-support factor      resize support: > 1.0 is blurry, < 1.0 is sharp",
      "-thumbnail geometry  create a thumbnail of the image",
      "-transparent color   make this color transparent within the image",
      "-treedepth value     color tree depth",
      "-trim                trim image edges",
      "-type type           image type",
      "-verbose             print detailed information about the image",
      "-version             print version information",
      "-virtual-pixel method",
      "                     Constant, Edge, Mirror, or Tile",
      "-window id           select window with this id or name",
      (char *) NULL
    };

  (void) printf("Version: %s\n",GetMagickVersion((unsigned long *) NULL));
  (void) printf("Copyright: %s\n\n",GetMagickCopyright());
  (void) printf("Usage: %s [options ...] [ file ]\n",
    GetClientName());
  (void) printf("\nWhere options include:\n");
  for (p=options; *p != (char *) NULL; p++)
    (void) printf("  %s\n",*p);
  (void) printf(
  "\nBy default, 'file' is written in the MIFF image format.  To\n");
  (void) printf(
    "specify a particular image format, precede the filename with an image\n");
  (void) printf(
    "format name and a colon (i.e. ps:image) or specify the image type as\n");
  (void) printf(
    "the filename suffix (i.e. image.ps).  Specify 'file' as '-' for\n");
  (void) printf("standard input or output.\n");
  exit(0);
}

MagickExport MagickBooleanType ImportImageCommand(ImageInfo *image_info,
  int argc,char **argv,char **magick_unused(metadata),ExceptionInfo *exception)
{
#if defined(HasX11)
#define DestroyImport() \
{ \
  XDestroyResourceInfo(&resource_info); \
  if (display != (Display *) NULL) \
    { \
      XCloseDisplay(display); \
      display=(Display *) NULL; \
    } \
  for ( ; k >= 0; k--) \
    image_stack[k]=DestroyImageList(image_stack[k]); \
  for (i=0; i < (long) argc; i++) \
    argv[i]=(char *) RelinquishMagickMemory(argv[i]); \
  argv=(char **) RelinquishMagickMemory(argv); \
}
#define ThrowImportException(asperity,tag,option) \
{ \
  if (exception->severity == UndefinedException) \
    (void) ThrowMagickException(exception,GetMagickModule(),asperity,tag, \
      "`%s'",option); \
  DestroyImport(); \
  return(MagickFalse); \
}
#define ThrowImportInvalidArgumentException(option,argument) \
{ \
  (void) ThrowMagickException(exception,GetMagickModule(),OptionError, \
    "InvalidArgument","`%s': %s",argument,option); \
  DestroyImport(); \
  return(MagickFalse); \
}

  char
    *filename,
    *option,
    *resource_value,
    *server_name,
    *target_window;

  Display
    *display;

  Image
    *image_stack[MaxImageStackDepth+1];

  long
    j,
    k,
    snapshots;

  MagickStatusType
    pend,
    status;

  QuantizeInfo
    *quantize_info;

  register long
    i;

  XImportInfo
    ximage_info;

  XResourceInfo
    resource_info;

  XrmDatabase
    resource_database;

  /*
    Set defaults.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(exception != (ExceptionInfo *) NULL);
  display=(Display *) NULL;
  j=1;
  k=0;
  image_stack[k]=NewImageList();
  option=(char *) NULL;
  pend=MagickFalse;
  resource_database=(XrmDatabase) NULL;
  (void) ResetMagickMemory(&resource_info,0,sizeof(resource_info));
  server_name=(char *) NULL;
  status=MagickTrue;
  SetNotifyHandlers;
  /*
    Check for server name specified on the command line.
  */
  ReadCommandlLine(argc,&argv);
  status=ExpandFilenames(&argc,&argv);
  if (status == MagickFalse)
    ThrowImportException(ResourceLimitError,"MemoryAllocationFailed",
      strerror(errno));
  for (i=1; i < (long) argc; i++)
  {
    /*
      Check command line for server name.
    */
    option=argv[i];
    if (IsMagickOption(option) == MagickFalse)
      continue;
    if (LocaleCompare("display",option+1) == 0)
      {
        /*
          User specified server name.
        */
        i++;
        if (i == (long) argc)
          ThrowImportException(OptionError,"MissingArgument",option);
        server_name=argv[i];
        break;
      }
  }
  /*
    Get user defaults from X resource database.
  */
  display=XOpenDisplay(server_name);
  if (display == (Display *) NULL)
    ThrowImportException(XServerError,"UnableToOpenXServer",
      XDisplayName(server_name));
  (void) XSetErrorHandler(XError);
  resource_database=XGetResourceDatabase(display,GetClientName());
  XGetImportInfo(&ximage_info);
  XGetResourceInfo(resource_database,GetClientName(),&resource_info);
  image_info=resource_info.image_info;
  quantize_info=resource_info.quantize_info;
  resource_value=XGetResourceInstance(resource_database,GetClientName(),
    "border","False");
  ximage_info.borders=IsTrue(resource_value);
  resource_value=XGetResourceInstance(resource_database,GetClientName(),
    "delay","0");
  resource_info.delay=(unsigned int) atoi(resource_value);
  image_info->density=XGetResourceInstance(resource_database,GetClientName(),
    "density",(char *) NULL);
  resource_value=XGetResourceInstance(resource_database,GetClientName(),
    "descend","True");
  ximage_info.descend=IsTrue(resource_value);
  resource_value=XGetResourceInstance(resource_database,GetClientName(),
    "frame","False");
  ximage_info.frame=IsTrue(resource_value);
  resource_value=XGetResourceInstance(resource_database,GetClientName(),
    "interlace","none");
  image_info->interlace=UndefinedInterlace;
  if (LocaleCompare("None",resource_value) == 0)
    image_info->interlace=NoInterlace;
  if (LocaleCompare("Line",resource_value) == 0)
    image_info->interlace=LineInterlace;
  if (LocaleCompare("Plane",resource_value) == 0)
    image_info->interlace=PlaneInterlace;
  if (LocaleCompare("Partition",resource_value) == 0)
    image_info->interlace=PartitionInterlace;
  if (image_info->interlace == UndefinedInterlace)
    ThrowImportException(OptionError,"Unrecognized interlace type",
      resource_value);
  image_info->page=XGetResourceInstance(resource_database,GetClientName(),
    "pageGeometry",(char *) NULL);
  resource_value=XGetResourceInstance(resource_database,GetClientName(),
    "pause","0");
  resource_info.pause=(unsigned int) atol(resource_value);
  resource_value=XGetResourceInstance(resource_database,GetClientName(),
    "quality","85");
  image_info->quality=(unsigned long) atol(resource_value);
  resource_value=XGetResourceInstance(resource_database,GetClientName(),
    "screen","False");
  ximage_info.screen=IsTrue(resource_value);
  resource_value=XGetResourceInstance(resource_database,GetClientName(),
    "silent","False");
  ximage_info.silent=IsTrue(resource_value);
  resource_value=XGetResourceInstance(resource_database,GetClientName(),
    "verbose","False");
  image_info->verbose=IsTrue(resource_value);
  resource_value=XGetResourceInstance(resource_database,GetClientName(),
    "dither","True");
  quantize_info->dither=IsTrue(resource_value);
  snapshots=1;
  status=MagickTrue;
  filename=(char *) NULL;
  target_window=(char *) NULL;
  /*
    Check command syntax.
  */
  for (i=1; i < (long) argc; i++)
  {
    option=argv[i];
    if (LocaleCompare(option,"(") == 0)
      {
        if (k == MaxImageStackDepth)
          ThrowImportException(OptionError,"ParenthesisNestedTooDeeply",
            option);
        MogrifyImageStack(image_stack[k],MagickTrue,pend);
        k++;
        image_stack[k]=NewImageList();
        continue;
      }
    if (LocaleCompare(option,")") == 0)
      {
        if (k == 0)
          ThrowImportException(OptionError,"UnableToParseExpression",option);
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

        unsigned long
          scene;

        /*
          Read image from X server.
        */
        MogrifyImageStack(image_stack[k],MagickFalse,pend);
        filename=argv[i];
        if (target_window != (char *) NULL)
          (void) CopyMagickString(image_info->filename,target_window,
            MaxTextExtent);
        for (scene=0; scene < (unsigned long) Max(snapshots,1); scene++)
        {
          (void) sleep(resource_info.pause);
          image=XImportImage(image_info,&ximage_info);
          status&=(image != (Image *) NULL) &&
            (exception->severity < ErrorException);
          if (image == (Image *) NULL)
            continue;
          (void) CopyMagickString(image->filename,filename,MaxTextExtent);
          (void) strcpy(image->magick,"PS");
          image->scene=scene;
          AppendImageToList(&image_stack[k],image);
          MogrifyImageStack(image_stack[k],MagickFalse,MagickTrue);
        }
        continue;
      }
    pend=image_stack[k] != (Image *) NULL ? MagickTrue : MagickFalse;
    switch(*(option+1))
    {
      case 'a':
      {
        if (LocaleCompare("adjoin",option+1) == 0)
          break;
        if (LocaleCompare("annotate",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowImportException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowImportInvalidArgumentException(option,argv[i]);
            if (i == (long) (argc-1))
              ThrowImportException(OptionError,"MissingArgument",option);
            i++;
            break;
          }
        ThrowImportException(OptionError,"UnrecognizedOption",option);
      }
      case 'b':
      {
        if (LocaleCompare("border",option+1) == 0)
          {
            ximage_info.borders=(MagickBooleanType) (*option == '-');
            (void) strcpy(argv[i]+1,"{0}");
            break;
          }
        if (LocaleCompare("bordercolor",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowImportException(OptionError,"UnrecognizedOption",option);
      }
      case 'c':
      {
        if (LocaleCompare("cache",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowImportInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("channel",option+1) == 0)
          {
            long
              channel;

            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowImportException(OptionError,"MissingArgument",option);
            channel=ParseChannelOption(argv[i]);
            if (channel < 0)
              ThrowImportException(OptionError,"UnrecognizedChannelType",
                argv[i]);
            break;
          }
        if (LocaleCompare("colors",option+1) == 0)
          {
            quantize_info->number_colors=0;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowImportInvalidArgumentException(option,argv[i]);
            quantize_info->number_colors=(unsigned long) atol(argv[i]);
            break;
          }
        if (LocaleCompare("colorspace",option+1) == 0)
          {
            long
              colorspace;

            quantize_info->colorspace=UndefinedColorspace;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            colorspace=ParseMagickOption(MagickColorspaceOptions,MagickFalse,
              argv[i]);
            if (colorspace < 0)
              ThrowImportException(OptionError,"UnrecognizedColorspace",
                argv[i]);
            quantize_info->colorspace=(ColorspaceType) colorspace;
            if (quantize_info->colorspace == GRAYColorspace)
              {
                quantize_info->colorspace=GRAYColorspace;
                quantize_info->number_colors=256;
                quantize_info->tree_depth=8;
              }
            break;
          }
        if (LocaleCompare("comment",option+1) == 0)
          {
            if (*option == '-')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            status=SetImageOption(image_info,"Comment",argv[i]);
            if (status == MagickFalse)
              ThrowImportException(OptionError,"UnrecognizedOption",argv[i]);
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
              ThrowImportException(OptionError,"MissingArgument",option);
            compression=ParseMagickOption(MagickCompressionOptions,
              MagickFalse,argv[i]);
            if (compression < 0)
              ThrowImportException(OptionError,"UnrecognizedImageCompression",
                argv[i]);
            break;
          }
        if (LocaleCompare("crop",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowImportInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowImportException(OptionError,"UnrecognizedOption",option);
      }
      case 'd':
      {
        if (LocaleCompare("debug",option+1) == 0)
          {
            LogEventType
              event_mask;

            (void) SetLogEventMask("None");
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            event_mask=SetLogEventMask(argv[i]);
            if (event_mask == UndefinedEvents)
              ThrowImportException(OptionError,"UnrecognizedEventType",option);
            break;
          }
        if (LocaleCompare("define",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if (*option == '+')
              {
                const char
                  *define;

                define=GetImageOption(image_info,argv[i]);
                if (define == (char *) NULL)
                  ThrowImportException(OptionError,"NoSuchOption",argv[i]);
                break;
              }
            break;
          }
        if (LocaleCompare("delay",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowImportInvalidArgumentException(option,argv[i]);
            status=SetImageOption(image_info,"delay",argv[i]);
            if (status == MagickFalse)
              ThrowImportException(OptionError,"UnrecognizedOption",argv[i]);
            break;
          }
        if (LocaleCompare("density",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowImportInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("depth",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowImportInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("descend",option+1) == 0)
          {
            ximage_info.descend=(MagickBooleanType) (*option == '-');
            break;
          }
        if (LocaleCompare("display",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
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
              ThrowImportException(OptionError,"MissingArgument",option);
            dispose=ParseMagickOption(MagickDisposeOptions,MagickFalse,argv[i]);
            if (dispose < 0)
              ThrowImportException(OptionError,"UnrecognizedDisposeMethod",
                argv[i]);
            break;
          }
        if (LocaleCompare("dither",option+1) == 0)
          {
            quantize_info->dither=(MagickBooleanType) (*option == '-');
            break;
          }
        ThrowImportException(OptionError,"UnrecognizedOption",option);
      }
      case 'e':
      {
        if (LocaleCompare("encoding",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
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
              ThrowImportException(OptionError,"MissingArgument",option);
            endian=ParseMagickOption(MagickEndianOptions,MagickFalse,
              argv[i]);
            if (endian < 0)
              ThrowImportException(OptionError,"UnrecognizedEndianType",
                argv[i]);
            break;
          }
        ThrowImportException(OptionError,"UnrecognizedOption",option);
      }
      case 'f':
      {
        if (LocaleCompare("frame",option+1) == 0)
          {
            ximage_info.frame=(MagickBooleanType) (*option == '-');
            (void) strcpy(argv[i]+1,"{0}");
            break;
          }
        ThrowImportException(OptionError,"UnrecognizedOption",option);
      }
      case 'g':
      {
        if (LocaleCompare("geometry",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowImportInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("gravity",option+1) == 0)
          {
            long
              gravity;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            gravity=ParseMagickOption(MagickGravityOptions,MagickFalse,argv[i]);
            if (gravity < 0)
              ThrowImportException(OptionError,"UnrecognizedGravityType",
                argv[i]);
            break;
          }
        ThrowImportException(OptionError,"UnrecognizedOption",option);
      }
      case 'h':
      {
        if (LocaleCompare("help",option+1) == 0)
          ImportUsage();
        ThrowImportException(OptionError,"UnrecognizedOption",option);
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
              ThrowImportException(OptionError,"MissingArgument",option);
            interlace=ParseMagickOption(MagickInterlaceOptions,MagickFalse,
              argv[i]);
            if (interlace < 0)
              ThrowImportException(OptionError,"UnrecognizedInterlaceType",
                argv[i]);
            break;
          }
        ThrowImportException(OptionError,"UnrecognizedOption",option);
      }
      case 'l':
      {
        if (LocaleCompare("label",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            status=SetImageOption(image_info,"label",argv[i]);
            if (status == MagickFalse)
              ThrowImportException(OptionError,"UnrecognizedOption",argv[i]);
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
              ThrowImportException(OptionError,"MissingArgument",option);
            resource=ParseMagickOption(MagickResourceOptions,MagickFalse,
              argv[i]);
            if (resource < 0)
              ThrowImportException(OptionError,"UnrecognizedResourceType",
                argv[i]);
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if ((LocaleCompare("unlimited",argv[i]) != 0) &&
                (IsGeometry(argv[i]) == MagickFalse))
              ThrowImportInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("log",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if ((i == (long) argc) || (strchr(argv[i],'%') == (char *) NULL))
              ThrowImportException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowImportException(OptionError,"UnrecognizedOption",option);
      }
      case 'm':
      {
        if (LocaleCompare("monitor",option+1) == 0)
          break;
        if (LocaleCompare("monochrome",option+1) == 0)
          {
            if (*option == '+')
              break;
            quantize_info->number_colors=2;
            quantize_info->tree_depth=8;
            quantize_info->colorspace=GRAYColorspace;
            break;
          }
        ThrowImportException(OptionError,"UnrecognizedOption",option);
      }
      case 'n':
      {
        if (LocaleCompare("negate",option+1) == 0)
          break;
        ThrowImportException(OptionError,"UnrecognizedOption",option);
      }
      case 'p':
      {
        if (LocaleCompare("page",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            status=SetImageOption(image_info,"page",argv[i]);
            if (status == MagickFalse)
              ThrowImportException(OptionError,"UnrecognizedOption",argv[i]);
            break;
          }
        if (LocaleCompare("pause",option+1) == 0)
          {
            resource_info.pause=0;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowImportInvalidArgumentException(option,argv[i]);
            resource_info.pause=(unsigned int) atoi(argv[i]);
            break;
          }
        if (LocaleCompare("ping",option+1) == 0)
          ThrowImportException(OptionError,"DeprecatedOption",option);
        if (LocaleCompare("pointsize",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowImportInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowImportException(OptionError,"UnrecognizedOption",option);
      }
      case 'q':
      {
        if (LocaleCompare("quality",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowImportInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("quiet",option+1) == 0)
          break;
        ThrowImportException(OptionError,"UnrecognizedOption",option);
      }
      case 'r':
      {
        if (LocaleCompare("repage",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowImportException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowImportInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("resize",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowImportInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("rotate",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowImportInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowImportException(OptionError,"UnrecognizedOption",option);
      }
      case 's':
      {
        if (LocaleCompare("sampling-factor",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowImportInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("scene",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowImportInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("set",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("screen",option+1) == 0)
          {
            ximage_info.screen=(MagickBooleanType) (*option == '-');
            break;
          }
        if (LocaleCompare("silent",option+1) == 0)
          {
            ximage_info.silent=(MagickBooleanType) (*option == '-');
            break;
          }
        if (LocaleCompare("snaps",option+1) == 0)
          {
            (void) strcpy(argv[i]+1,"{1}");
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowImportInvalidArgumentException(option,argv[i]);
            snapshots=atol(argv[i]);
            break;
          }
        if (LocaleCompare("strip",option+1) == 0)
          break;
        if (LocaleCompare("support",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowImportInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowImportException(OptionError,"UnrecognizedOption",option);
      }
      case 't':
      {
        if (LocaleCompare("thumnail",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowImportInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("transparent",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("treedepth",option+1) == 0)
          {
            quantize_info->tree_depth=0;
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowImportException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowImportInvalidArgumentException(option,argv[i]);
            quantize_info->tree_depth=(unsigned long) atol(argv[i]);
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
              ThrowImportException(OptionError,"MissingArgument",option);
            type=ParseMagickOption(MagickImageOptions,MagickFalse,argv[i]);
            if (type < 0)
              ThrowImportException(OptionError,"UnrecognizedImageType",argv[i]);
            break;
          }
        ThrowImportException(OptionError,"UnrecognizedOption",option);
      }
      case 'w':
      {
        i++;
        if (i == (long) argc)
          ThrowImportException(OptionError,"MissingArgument",option);
        (void) CloneString(&target_window,argv[i]);
        break;
      }
      case 'v':
      {
        if (LocaleCompare("verbose",option+1) == 0)
          break;
        if (LocaleCompare("version",option+1) == 0)
          break;
        ThrowImportException(OptionError,"UnrecognizedOption",option);
      }
      case '?':
        break;
      default:
        ThrowImportException(OptionError,"UnrecognizedOption",option);
    }
    status=(MagickStatusType)
      ParseMagickOption(MagickMogrifyOptions,MagickFalse,option+1);
    if (status == MagickTrue)
      MogrifyImageStack(image_stack[k],MagickTrue,MagickTrue);
  }
  if (k != 0)
    ThrowImportException(OptionError,"UnbalancedParenthesis",argv[i]);
  if (i != argc)
    ThrowImportException(OptionError,"MissingAnImageFilename",argv[i]);
  if (image_stack[k] == (Image *) NULL)
    ThrowImportException(OptionError,"MissingAnImageFilename",argv[argc-1]);
  MogrifyImageStack(image_stack[k],MagickTrue,MagickTrue)
  GetImageException(image_stack[k],exception);
  status&=WriteImages(image_info,image_stack[k],filename,exception);
  DestroyImport();
  return(status != 0 ? MagickTrue : MagickFalse);
#else
  (void) ThrowMagickException(exception,GetMagickModule(),MissingDelegateError,
    "XWindowLibraryIsNotAvailable","`%s'",image_info->filename);
  ImportUsage();
  return(MagickFalse);
#endif
}
