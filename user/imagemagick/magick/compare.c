/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%               CCCC   OOO   M   M  PPPP    AAA   RRRR    EEEEE               %
%              C      O   O  MM MM  P   P  A   A  R   R   E                   %
%              C      O   O  M M M  PPPP   AAAAA  RRRR    EEE                 %
%              C      O   O  M   M  P      A   A  R R     E                   %
%               CCCC   OOO   M   M  P      A   A  R  R    EEEEE               %
%                                                                             %
%                                                                             %
%                         Image Comparison Methods                            %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                               December 2003                                 %
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
#include "magick/client.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/colorspace.h"
#include "magick/colorspace-private.h"
#include "magick/compare.h"
#include "magick/composite-private.h"
#include "magick/constitute.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/memory_.h"
#include "magick/mogrify.h"
#include "magick/mogrify-private.h"
#include "magick/option.h"
#include "magick/pixel-private.h"
#include "magick/resource_.h"
#include "magick/string_.h"
#include "magick/utility.h"
#include "magick/version.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o m p a r e I m a g e C h a n n e l s                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompareImageChannels() compares one or more image channels of an image
%  to a reconstructed image and returns the difference image.
%
%  The format of the CompareImageChannels method is:
%
%      Image *CompareImageChannels(const Image *image,
%        const Image *reconstruct_image,const ChannelType channel,
%        const MetricType metric,double *distortion,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o reconstruct_image: The reconstruct image.
%
%    o channel: The channel.
%
%    o metric: The metric.
%
%    o distortion: The computed distortion between the images.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

MagickExport Image *CompareImages(Image *image,const Image *reconstruct_image,
  const MetricType metric,double *distortion,ExceptionInfo *exception)
{
  Image
    *difference_image;

  difference_image=CompareImageChannels(image,reconstruct_image,AllChannels,
    metric,distortion,exception);
  return(difference_image);
}

MagickExport Image *CompareImageChannels(Image *image,
  const Image *reconstruct_image,const ChannelType channel,
  const MetricType metric,double *distortion,ExceptionInfo *exception)
{
  Image
    *difference_image;

  long
    y;

  MagickPixelPacket
    composite,
    red,
    source,
    white;

  MagickStatusType
    difference;

  register const PixelPacket
    *p,
    *q;

  register IndexPacket
    *difference_indexes,
    *indexes,
    *reconstruct_indexes;

  register long
    x;

  register PixelPacket
    *r;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(reconstruct_image != (const Image *) NULL);
  assert(reconstruct_image->signature == MagickSignature);
  assert(distortion != (double *) NULL);
  *distortion=0.0;
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((reconstruct_image->columns != image->columns) ||
      (reconstruct_image->rows != image->rows))
    ThrowImageException(ImageError,"ImageSizeDiffers");
  if (image->colorspace != reconstruct_image->colorspace)
    ThrowImageException(ImageError,"ImageColorspaceDiffers");
  if (image->matte != reconstruct_image->matte)
    ThrowImageException(ImageError,"ImageOpacityDiffers");
  difference_image=CloneImage(image,image->columns,image->rows,MagickTrue,
    exception);
  if (difference_image == (Image *) NULL)
    return((Image *) NULL);
  difference_image->storage_class=DirectClass;
  (void) QueryMagickColor("#f1001e00",&red,exception);
  (void) QueryMagickColor("#ffffff00",&white,exception);
  if (difference_image->colorspace == CMYKColorspace)
    {
      RGBtoCMYK(&red);
      RGBtoCMYK(&white);
    }
  /*
    Generate difference image.
  */
  GetMagickPixelPacket(reconstruct_image,&source);
  GetMagickPixelPacket(difference_image,&composite);
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    q=AcquireImagePixels(reconstruct_image,0,y,reconstruct_image->columns,1,
      exception);
    r=SetImagePixels(difference_image,0,y,difference_image->columns,1);
    if ((p == (const PixelPacket *) NULL) ||
        (q == (const PixelPacket *) NULL) || (r == (PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    reconstruct_indexes=GetIndexes(reconstruct_image);
    difference_indexes=GetIndexes(difference_image);
    for (x=0; x < (long) image->columns; x++)
    {
      difference=MagickFalse;
      if ((channel & RedChannel) != 0)
        if (p->red != q->red)
          difference=MagickTrue;
      if ((channel & GreenChannel) != 0)
        if (p->green != q->green)
          difference=MagickTrue;
      if ((channel & BlueChannel) != 0)
        if (p->blue != q->blue)
          difference=MagickTrue;
      if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
        if (p->opacity != q->opacity)
          difference=MagickTrue;
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        if (indexes[x] != reconstruct_indexes[x])
          difference=MagickTrue;
      SetMagickPixelPacket(q,reconstruct_indexes+x,&source);
      if (difference != MagickFalse)
        MagickPixelCompositeOver(&source,9.0*QuantumRange/10.0,&red,
          (MagickRealType) red.opacity,&composite);
      else
        MagickPixelCompositeOver(&source,9.0*QuantumRange/10.0,&white,
          (MagickRealType) white.opacity,&composite);
      SetPixelPacket(&composite,r,difference_indexes+x);
      p++;
      q++;
      r++;
    }
    if (SyncImagePixels(difference_image) == MagickFalse)
      break;
  }
  (void) GetImageChannelDistortion(image,reconstruct_image,channel,metric,
    distortion,exception);
  return(difference_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C o m p a r e I m a g e C o m m a n d                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CompareImageCommand() compares two images and returns the difference between
%  them as a distortion metric and as a new image visually annotating their
%  differences.
%
%  The format of the CompareImageCommand method is:
%
%      MagickBooleanType CompareImageCommand(ImageInfo *image_info,int argc,
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

static void CompareUsage(void)
{
  const char
    **p;

  static const char
    *options[]=
    {
      "-authenticate value  decrypt image with this password",
      "-channel type        apply option to select image channels",
      "-colorspace type     alternate image colorspace",
      "-compress type       type of pixel compression when writing the image",
      "-debug events        display copious debugging information",
      "-define format:option",
      "                     define one or more image format options",
      "-density geometry    horizontal and vertical density of the image",
      "-depth value         image depth",
      "-extract geometry    extract area from image",
      "-help                print program options",
      "-interlace type      type of image interlacing scheme",
      "-limit type value    pixel cache resource limit",
      "-log format          format of debugging information",
      "-metric type         measure differences between images with this metric",
      "-monitor             monitor progress",
      "-profile filename    add, delete, or apply an image profile",
      "-quality value       JPEG/MIFF/PNG compression level",
      "-quiet               suppress all error or warning messages",
      "-sampling-factor geometry",
      "                     horizontal and vertical sampling factor",
      "-set attribute value set an image attribute",
      "-size geometry       width and height of image",
      "-type type           image type",
      "-verbose             print detailed information about the image",
      "-version             print version information",
      "-virtual-pixel method",
      "                     virtual pixel access method",
      (char *) NULL
    };

  (void) printf("Version: %s\n",GetMagickVersion((unsigned long *) NULL));
  (void) printf("Copyright: %s\n\n",GetMagickCopyright());
  (void) printf("Usage: %s [options ...] image reconstruct difference\n",
    GetClientName());
  (void) printf("\nWhere options include:\n");
  for (p=options; *p != (char *) NULL; p++)
    (void) printf("  %s\n",*p);
  exit(0);
}

MagickExport MagickBooleanType CompareImageCommand(ImageInfo *image_info,
  int argc,char **argv,char **metadata,ExceptionInfo *exception)
{
#define DestroyCompare() \
{ \
  for ( ; k >= 0; k--) \
    image_stack[k]=DestroyImageList(image_stack[k]); \
  for (i=0; i < (long) argc; i++) \
    argv[i]=(char *) RelinquishMagickMemory(argv[i]); \
  argv=(char **) RelinquishMagickMemory(argv); \
}
#define ThrowCompareException(asperity,tag,option) \
{ \
  if (exception->severity < (asperity)) \
    (void) ThrowMagickException(exception,GetMagickModule(),asperity,tag, \
      "`%s'",option); \
  DestroyCompare(); \
  return(MagickFalse); \
}
#define ThrowCompareInvalidArgumentException(option,argument) \
{ \
  (void) ThrowMagickException(exception,GetMagickModule(),OptionError, \
    "InvalidArgument","`%s': %s",argument,option); \
  DestroyCompare(); \
  return(MagickFalse); \
}

  char
    *filename,
    *option;

  const char
    *format;

  ChannelType
    channel;

  double
    distortion;

  Image
    *difference_image,
    *image,
    *image_stack[MaxImageStackDepth+1],
    *reconstruct_image;

  long
    j,
    k;

  MagickStatusType
    pend,
    status;

  MetricType
    metric;

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
    CompareUsage();
  channel=AllChannels;
  difference_image=NewImageList();
  distortion=0.0;
  format="%w,%h,%m";
  j=1;
  k=0;
  image_stack[k]=NewImageList();
  metric=UndefinedMetric;
  option=(char *) NULL;
  pend=MagickFalse;
  reconstruct_image=NewImageList();
  status=MagickTrue;
  /*
    Compare an image.
  */
  ReadCommandlLine(argc,&argv);
  status=ExpandFilenames(&argc,&argv);
  if (status == MagickFalse)
    ThrowCompareException(ResourceLimitError,"MemoryAllocationFailed",
      strerror(errno));
  for (i=1; i < (long) (argc-1); i++)
  {
    option=argv[i];
    if (LocaleCompare(option,"(") == 0)
      {
        if (k == MaxImageStackDepth)
          ThrowCompareException(OptionError,"ParenthesisNestedTooDeeply",
            option);
        MogrifyImageStack(image_stack[k],MagickTrue,pend);
        k++;
        image_stack[k]=NewImageList();
        continue;
      }
    if (LocaleCompare(option,")") == 0)
      {
        if (k == 0)
          ThrowCompareException(OptionError,"UnableToParseExpression",option);
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
        /*
          Read input image.
        */
        MogrifyImageStack(image_stack[k],MagickTrue,pend);
        filename=argv[i];
        (void) CopyMagickString(image_info->filename,filename,MaxTextExtent);
        image=ReadImage(image_info,exception);
        status&=(image != (Image *) NULL) &&
          (exception->severity < ErrorException);
        if (image == (Image *) NULL)
          continue;
        AppendImageToList(&image_stack[k],image);
        continue;
      }
    pend=image_stack[k] != (Image *) NULL ? MagickTrue : MagickFalse;
    switch (*(option+1))
    {
      case 'c':
      {
        if (LocaleCompare("cache",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompareException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompareInvalidArgumentException(option,argv[i]);
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
              ThrowCompareException(OptionError,"MissingArgument",option);
            channel=ParseChannelOption(argv[i]);
            if (channel < 0)
              ThrowCompareException(OptionError,"UnrecognizedChannelType",
                argv[i]);
            break;
          }
        if (LocaleCompare("colorspace",option+1) == 0)
          {
            long
              colorspace;

            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowCompareException(OptionError,"MissingArgument",option);
            colorspace=ParseMagickOption(MagickColorspaceOptions,MagickFalse,
              argv[i]);
            if (colorspace < 0)
              ThrowCompareException(OptionError,"UnrecognizedColorspace",
                argv[i]);
            break;
          }
        if (LocaleCompare("compress",option+1) == 0)
          {
            long
              compression;

            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowCompareException(OptionError,"MissingArgument",option);
            compression=ParseMagickOption(MagickCompressionOptions,
              MagickFalse,argv[i]);
            if (compression < 0)
              ThrowCompareException(OptionError,
                "UnrecognizedImageCompression",argv[i]);
            break;
          }
        ThrowCompareException(OptionError,"UnrecognizedOption",option)
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
              ThrowCompareException(OptionError,"MissingArgument",option);
            event_mask=SetLogEventMask(argv[i]);
            if (event_mask == UndefinedEvents)
              ThrowCompareException(OptionError,"UnrecognizedEventType",
                option);
            break;
          }
        if (LocaleCompare("define",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowCompareException(OptionError,"MissingArgument",option);
            if (*option == '+')
              {
                const char
                  *define;

                define=GetImageOption(image_info,argv[i]);
                if (define == (const char *) NULL)
                  ThrowCompareException(OptionError,"NoSuchOption",argv[i]);
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
              ThrowCompareException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompareInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("depth",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompareException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompareInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowCompareException(OptionError,"UnrecognizedOption",option)
      }
      case 'e':
      {
        if (LocaleCompare("extract",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowCompareException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompareInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowCompareException(OptionError,"UnrecognizedOption",option)
      }
      case 'f':
      {
        if (LocaleCompare("format",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompareException(OptionError,"MissingArgument",option);
            format=argv[i];
            break;
          }
        ThrowCompareException(OptionError,"UnrecognizedOption",option)
      }
      case 'h':
      {
        if (LocaleCompare("help",option+1) == 0)
          CompareUsage();
        ThrowCompareException(OptionError,"UnrecognizedOption",option)
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
              ThrowCompareException(OptionError,"MissingArgument",option);
            interlace=ParseMagickOption(MagickInterlaceOptions,MagickFalse,
              argv[i]);
            if (interlace < 0)
              ThrowCompareException(OptionError,"UnrecognizedInterlaceType",
                argv[i]);
            break;
          }
        ThrowCompareException(OptionError,"UnrecognizedOption",option)
      }
      case 'l':
      {
        if (LocaleCompare("limit",option+1) == 0)
          {
            long
              resource;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompareException(OptionError,"MissingArgument",option);
            resource=ParseMagickOption(MagickResourceOptions,MagickFalse,
              argv[i]);
            if (resource < 0)
              ThrowCompareException(OptionError,"UnrecognizedResourceType",
                argv[i]);
            i++;
            if (i == (long) argc)
              ThrowCompareException(OptionError,"MissingArgument",option);
            if ((LocaleCompare("unlimited",argv[i]) != 0) &&
                (IsGeometry(argv[i]) == MagickFalse))
              ThrowCompareInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("log",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if ((i == (long) argc) || (strchr(argv[i],'%') == (char *) NULL))
              ThrowCompareException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowCompareException(OptionError,"UnrecognizedOption",option)
      }
      case 'm':
      {
        if (LocaleCompare("metric",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompareException(OptionError,"MissingArgument",option);
            metric=(MetricType) ParseMagickOption(MagickMetricOptions,
              MagickTrue,argv[i]);
            if (metric <= UndefinedMetric)
              ThrowCompareException(OptionError,"UnrecognizedMetricType",
                argv[i]);
            break;
          }
        if (LocaleCompare("monitor",option+1) == 0)
          break;
        ThrowCompareException(OptionError,"UnrecognizedOption",option)
      }
      case 'p':
      {
        if (LocaleCompare("profile",option+1) == 0)
          {
            i++;
            if (i == (long) (argc-1))
              ThrowCompareException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowCompareException(OptionError,"UnrecognizedOption",option)
      }
      case 'q':
      {
        if (LocaleCompare("quality",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowCompareException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompareInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("quiet",option+1) == 0)
          break;
        ThrowCompareException(OptionError,"UnrecognizedOption",option)
      }
      case 's':
      {
        if (LocaleCompare("sampling-factor",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompareException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompareInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("set",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowCompareException(OptionError,"MissingArgument",option);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompareException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("size",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompareException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowCompareInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowCompareException(OptionError,"UnrecognizedOption",option)
      }
      case 't':
      {
        if (LocaleCompare("type",option+1) == 0)
          {
            long
              type;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowCompareException(OptionError,"MissingArgument",option);
            type=ParseMagickOption(MagickImageOptions,MagickFalse,argv[i]);
            if (type < 0)
              ThrowCompareException(OptionError,"UnrecognizedImageType",
                argv[i]);
            break;
          }
        ThrowCompareException(OptionError,"UnrecognizedOption",option)
      }
      case 'v':
      {
        if (LocaleCompare("verbose",option+1) == 0)
          break;
        if (LocaleCompare("version",option+1) == 0)
          break;
        if (LocaleCompare("virtual-pixel",option+1) == 0)
          {
            long
              method;

            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowCompareException(OptionError,"MissingArgument",option);
            method=ParseMagickOption(MagickVirtualPixelOptions,MagickFalse,
              argv[i]);
            if (method < 0)
              ThrowCompareException(OptionError,
                "UnrecognizedVirtualPixelMethod",argv[i]);
            break;
          }
        ThrowCompareException(OptionError,"UnrecognizedOption",option)
      }
      case '?':
        break;
      default:
        ThrowCompareException(OptionError,"UnrecognizedOption",option)
    }
    status=(MagickStatusType)
      ParseMagickOption(MagickMogrifyOptions,MagickFalse,option+1);
    if (status == MagickTrue)
      MogrifyImageStack(image_stack[k],MagickTrue,MagickTrue);
  }
  if (k != 0)
    ThrowCompareException(OptionError,"UnbalancedParenthesis",argv[i]);
  if (i != (long) (argc-1))
    ThrowCompareException(OptionError,"MissingAnImageFilename",argv[i]);
  if ((image_stack[k] == (Image *) NULL) ||
      (GetImageListLength(image_stack[k]) < 2))
    ThrowCompareException(OptionError,"MissingAnImageFilename",argv[i]);
  MogrifyImageStack(image_stack[k],MagickTrue,MagickTrue);
  image=GetImageFromList(image_stack[k],0);
  reconstruct_image=GetImageFromList(image_stack[k],1);
  difference_image=CompareImageChannels(image,reconstruct_image,channel,
    metric,&distortion,exception);
  if (difference_image != (Image *) NULL)
    {
      if (image_info->verbose != MagickFalse)
        (void) IsImagesEqual(image,reconstruct_image);
      status&=WriteImages(image_info,difference_image,argv[argc-1],exception);
      if (metadata != (char **) NULL)
        {
          char
            *text;

          text=TranslateText(image_info,difference_image,format);
          if (text == (char *) NULL)
            ThrowCompareException(ResourceLimitError,"MemoryAllocationFailed",
              strerror(errno));
          (void) ConcatenateString(&(*metadata),text);
          (void) ConcatenateString(&(*metadata),"\n");
          text=(char *) RelinquishMagickMemory(text);
        }
      difference_image=DestroyImageList(difference_image);
    }
  if (metric != UndefinedMetric)
    (void) fprintf(stdout,"%g dB\n",distortion);
  DestroyCompare();
  return(status != 0 ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t I m a g e C h a n n e l D i s t o r t i o n                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageChannelDistrortion() compares one or more image channels of an image
%  to a reconstructed image and returns the specified distortion metric.
%
%  The format of the CompareImageChannels method is:
%
%      MagickBooleanType GetImageChhannelDistortion(const Image *image,
%        const Image *reconstruct_image,const ChannelType channel,
%        const MetricType metric,double *distortion,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o reconstruct_image: The reconstruct image.
%
%    o channel: The channel.
%
%    o metric: The metric.
%
%    o distortion: The computed distortion between the images.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

MagickExport MagickBooleanType GetImageDistortion(Image *image,
  const Image *reconstruct_image,const MetricType metric,double *distortion,
  ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  status=GetImageChannelDistortion(image,reconstruct_image,AllChannels,
    metric,distortion,exception);
  return(status);
}

static MagickRealType GetMeanAbsoluteError(const Image *image,
  const Image *reconstruct_image,const ChannelType channel,
  ExceptionInfo *exception)
{
  IndexPacket
    *indexes,
    *reconstruct_indexes;

  long
    y;

  MagickRealType
    area,
    distortion;

  register const PixelPacket
    *p,
    *q;

  register long
    x;

  area=0.0;
  distortion=0.0;
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    q=AcquireImagePixels(reconstruct_image,0,y,reconstruct_image->columns,1,
      exception);
    if ((p == (const PixelPacket *) NULL) || (q == (const PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    reconstruct_indexes=GetIndexes(reconstruct_image);
    for (x=0; x < (long) image->columns; x++)
    {
      if ((channel & RedChannel) != 0)
        {
          distortion+=fabs(p->red-(double) q->red);
          area++;
        }
      if ((channel & GreenChannel) != 0)
        {
          distortion+=fabs(p->green-(double) q->green);
          area++;
        }
      if ((channel & BlueChannel) != 0)
        {
          distortion+=fabs(p->blue-(double) q->blue);
          area++;
        }
      if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
        {
          distortion+=fabs(p->opacity-(double) q->opacity);
          area++;
        }
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        {
          distortion+=fabs(indexes[x]-(double) reconstruct_indexes[x]);
          area++;
        }
      p++;
      q++;
    }
  }
  return(distortion/area);
}

static MagickRealType GetMeanSquaredError(const Image *image,
  const Image *reconstruct_image,const ChannelType channel,
  ExceptionInfo *exception)
{
  IndexPacket
    *indexes,
    *reconstruct_indexes;

  long
    y;

  MagickRealType
    area,
    distance,
    distortion;

  register const PixelPacket
    *p,
    *q;

  register long
    x;

  area=0.0;
  distortion=0.0;
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    q=AcquireImagePixels(reconstruct_image,0,y,reconstruct_image->columns,1,
      exception);
    if ((p == (const PixelPacket *) NULL) || (q == (const PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    reconstruct_indexes=GetIndexes(reconstruct_image);
    for (x=0; x < (long) image->columns; x++)
    {
      if ((channel & RedChannel) != 0)
        {
          distance=p->red-(MagickRealType) q->red;
          distortion+=distance*distance;
          area++;
        }
      if ((channel & GreenChannel) != 0)
        {
          distance=p->green-(MagickRealType) q->green;
          distortion+=distance*distance;
          area++;
        }
      if ((channel & BlueChannel) != 0)
        {
          distance=p->blue-(MagickRealType) q->blue;
          distortion+=distance*distance;
          area++;
        }
      if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
        {
          distance=p->opacity-(MagickRealType) q->opacity;
          distortion+=distance*distance;
          area++;
        }
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        {
          distance=indexes[x]-(MagickRealType) reconstruct_indexes[x];
          distortion+=distance*distance;
          area++;
        }
      p++;
      q++;
    }
  }
  return(distortion/area);
}

static MagickRealType GetPeakAbsoluteError(const Image *image,
  const Image *reconstruct_image,const ChannelType channel,
  ExceptionInfo *exception)
{
  IndexPacket
    *indexes,
    *reconstruct_indexes;

  long
    y;

  MagickRealType
    distance,
    distortion;

  register const PixelPacket
    *p,
    *q;

  register long
    x;

  distortion=0.0;
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    q=AcquireImagePixels(reconstruct_image,0,y,reconstruct_image->columns,1,
      exception);
    if ((p == (const PixelPacket *) NULL) || (q == (const PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    reconstruct_indexes=GetIndexes(reconstruct_image);
    for (x=0; x < (long) image->columns; x++)
    {
      if ((channel & RedChannel) != 0)
        {
          distance=fabs(p->red-(double) q->red);
          if (distance > distortion)
            distortion=distance;
        }
      if ((channel & GreenChannel) != 0)
        {
          distance=fabs(p->green-(double) q->green);
          if (distance > distortion)
            distortion=distance;
        }
      if ((channel & BlueChannel) != 0)
        {
          distance=fabs(p->blue-(double) q->blue);
          if (distance > distortion)
            distortion=distance;
        }
      if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
        {
          distance=fabs(p->opacity-(double) q->opacity);
          if (distance > distortion)
            distortion=distance;
        }
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        {
          distance=fabs(indexes[x]-(double) reconstruct_indexes[x]);
          if (distance > distortion)
            distortion=distance;
        }
      p++;
      q++;
    }
  }
  return(distortion);
}

static MagickRealType GetPeakSignalToNoiseRatio(const Image *image,
  const Image *reconstruct_image,const ChannelType channel,
  ExceptionInfo *exception)
{
  MagickRealType
    distortion;

  distortion=GetMeanSquaredError(image,reconstruct_image,channel,exception);
  return(20.0*log10(QuantumRange/sqrt((double) distortion)));
}

static MagickRealType GetRootMeanSquaredError(const Image *image,
  const Image *reconstruct_image,const ChannelType channel,
  ExceptionInfo *exception)
{
  MagickRealType
    distortion;

  distortion=GetMeanSquaredError(image,reconstruct_image,channel,exception);
  return(sqrt((double) distortion));
}

MagickExport MagickBooleanType GetImageChannelDistortion(Image *image,
  const Image *reconstruct_image,const ChannelType channel,
  const MetricType metric,double *distortion,ExceptionInfo *exception)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(reconstruct_image != (const Image *) NULL);
  assert(reconstruct_image->signature == MagickSignature);
  assert(distortion != (double *) NULL);
  *distortion=0.0;
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((reconstruct_image->columns != image->columns) ||
      (reconstruct_image->rows != image->rows))
    ThrowBinaryException(ImageError,"ImageSizeDiffers",image->filename);
  if (image->colorspace != reconstruct_image->colorspace)
    ThrowBinaryException(ImageError,"ImageColorspaceDiffers",image->filename);
  if (image->matte != reconstruct_image->matte)
    ThrowBinaryException(ImageError,"ImageOpacityDiffers",image->filename);
  /*
    Get image distortion.
  */
  switch (metric)
  {
    case MeanAbsoluteErrorMetric:
    {
      *distortion=(double) GetMeanAbsoluteError(image,reconstruct_image,channel,
        exception);
      break;
    }
    case MeanSquaredErrorMetric:
    {
      *distortion=(double) GetMeanSquaredError(image,reconstruct_image,channel,
        exception);
      break;
    }
    case PeakAbsoluteErrorMetric:
    default:
    {
      *distortion=(double) GetPeakAbsoluteError(image,reconstruct_image,channel,
        exception);
      break;
    }
    case PeakSignalToNoiseRatioMetric:
    {
      *distortion=(double) GetPeakSignalToNoiseRatio(image,reconstruct_image,
        channel,exception);
      break;
    }
    case RootMeanSquaredErrorMetric:
    {
      *distortion=(double) GetRootMeanSquaredError(image,reconstruct_image,
        channel,exception);
      break;
    }
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  I s I m a g e s E q u a l                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsImagesEqual() measures the difference between colors at each pixel
%  location of two images.  A value other than 0 means the colors match
%  exactly.  Otherwise an error measure is computed by summing over all
%  pixels in an image the distance squared in RGB space between each image
%  pixel and its corresponding pixel in the reconstruct image.  The error
%  measure is assigned to these image members:
%
%    o mean_error_per_pixel:  The mean error for any single pixel in
%      the image.
%
%    o normalized_mean_error:  The normalized mean quantization error for
%      any single pixel in the image.  This distance measure is normalized to
%      a range between 0 and 1.  It is independent of the range of red, green,
%      and blue values in the image.
%
%    o normalized_maximum_error:  The normalized maximum quantization
%      error for any single pixel in the image.  This distance measure is
%      normalized to a range between 0 and 1.  It is independent of the range
%      of red, green, and blue values in your image.
%
%  A small normalized mean square error, accessed as
%  image->normalized_mean_error, suggests the images are very similiar in
%  spatial layout and color.
%
%  The format of the IsImagesEqual method is:
%
%      MagickBooleanType IsImagesEqual(Image *image,
%        const Image *reconstruct_image)
%
%  A description of each parameter follows.
%
%    o image: The image.
%
%    o reconstruct_image: The reconstruct image.
%
*/
MagickExport MagickBooleanType IsImagesEqual(Image *image,
  const Image *reconstruct_image)
{
  IndexPacket
    *indexes,
    *reconstruct_indexes;

  long
    y;

  MagickBooleanType
    status;

  MagickRealType
    area,
    distance,
    maximum_error,
    mean_error,
    mean_error_per_pixel;

  register const PixelPacket
    *p,
    *q;

  register long
    x;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  assert(reconstruct_image != (const Image *) NULL);
  assert(reconstruct_image->signature == MagickSignature);
  if ((reconstruct_image->columns != image->columns) ||
      (reconstruct_image->rows != image->rows))
    ThrowBinaryException(ImageError,"ImageSizeDiffers",image->filename);
  if (image->colorspace != reconstruct_image->colorspace)
    ThrowBinaryException(ImageError,"ImageColorspaceDiffers",image->filename);
  if (image->matte != reconstruct_image->matte)
    ThrowBinaryException(ImageError,"ImageOpacityDiffers",image->filename);
  area=0.0;
  maximum_error=0.0;
  mean_error_per_pixel=0.0;
  mean_error=0.0;
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,&image->exception);
    q=AcquireImagePixels(reconstruct_image,0,y,reconstruct_image->columns,1,
      &image->exception);
    if ((p == (const PixelPacket *) NULL) || (q == (const PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    reconstruct_indexes=GetIndexes(reconstruct_image);
    for (x=0; x < (long) image->columns; x++)
    {
      distance=fabs(p->red-(double) q->red);
      mean_error_per_pixel+=distance;
      mean_error+=distance*distance;
      if (distance > maximum_error)
        maximum_error=distance;
      area++;
      distance=fabs(p->green-(double) q->green);
      mean_error_per_pixel+=distance;
      mean_error+=distance*distance;
      if (distance > maximum_error)
        maximum_error=distance;
      area++;
      distance=fabs(p->blue-(double) q->blue);
      mean_error_per_pixel+=distance;
      mean_error+=distance*distance;
      if (distance > maximum_error)
        maximum_error=distance;
      area++;
      if (image->matte != MagickFalse)
        {
          distance=fabs(p->opacity-(double) q->opacity);
          mean_error_per_pixel+=distance;
          mean_error+=distance*distance;
          if (distance > maximum_error)
            maximum_error=distance;
          area++;
        }
      if (image->colorspace == CMYKColorspace)
        {
          distance=fabs(indexes[x]-(double) reconstruct_indexes[x]);
          mean_error_per_pixel+=distance;
          mean_error+=distance*distance;
          if (distance > maximum_error)
            maximum_error=distance;
          area++;
        }
      p++;
      q++;
    }
  }
  image->error.mean_error_per_pixel=(double) (mean_error_per_pixel/area);
  image->error.normalized_mean_error=(double)
    (mean_error/area/QuantumRange/QuantumRange);
  image->error.normalized_maximum_error=(double) (maximum_error/QuantumRange);
  status=(MagickBooleanType)
    (image->error.mean_error_per_pixel == 0.0 ? MagickTrue : MagickFalse);
  return(status);
}
