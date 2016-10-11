/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%              M   M   OOO   GGGGG  RRRR   IIIII  FFFFF  Y   Y                %
%              MM MM  O   O  G      R   R    I    F       Y Y                 %
%              M M M  O   O  G GGG  RRRR     I    FFF      Y                  %
%              M   M  O   O  G   G  R R      I    F        Y                  %
%              M   M   OOO   GGGG   R  R   IIIII  F        Y                  %
%                                                                             %
%                                                                             %
%                        ImageMagick Module Methods                           %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                March 2000                                   %
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
#include "magick/animate.h"
#include "magick/cache.h"
#include "magick/client.h"
#include "magick/coder.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/colorspace.h"
#include "magick/composite.h"
#include "magick/configure.h"
#include "magick/constitute.h"
#include "magick/decorate.h"
#include "magick/delegate.h"
#include "magick/display.h"
#include "magick/draw.h"
#include "magick/effect.h"
#include "magick/enhance.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/fx.h"
#include "magick/gem.h"
#include "magick/geometry.h"
#include "magick/identify.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/locale_.h"
#include "magick/log.h"
#include "magick/magic.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/module.h"
#include "magick/mogrify.h"
#include "magick/mogrify-private.h"
#include "magick/monitor.h"
#include "magick/montage.h"
#include "magick/option.h"
#include "magick/paint.h"
#include "magick/profile.h"
#include "magick/quantize.h"
#include "magick/random_.h"
#include "magick/resize.h"
#include "magick/resource_.h"
#include "magick/segment.h"
#include "magick/signature.h"
#include "magick/shear.h"
#include "magick/string_.h"
#include "magick/transform.h"
#include "magick/token.h"
#include "magick/type.h"
#include "magick/utility.h"
#include "magick/version.h"
#include "magick/xwindow-private.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     M o g r i f y I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MogrifyImage() applies image processing options to an image as prescribed
%  by command line options.
%
%  The format of the MogrifyImage method is:
%
%      MagickBooleanType MogrifyImage(ImageInfo *image_info,const int argc,
%        const char **argv,Image **image)
%
%  A description of each parameter follows:
%
%    o image_info: The image info..
%
%    o argc: Specifies a pointer to an integer describing the number of
%      elements in the argument vector.
%
%    o argv: Specifies a pointer to a text array containing the command line
%      arguments.
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType MogrifyImage(ImageInfo *image_info,
  const int argc,const char **argv,Image **image,ExceptionInfo *exception)
{
  const char
    *option;

  DrawInfo
    *draw_info;

  GeometryInfo
    geometry_info;

  Image
    *map_image,
    *region_image;

  long
    count;

  MagickBooleanType
    status;

  MagickPixelPacket
    fill;

  MagickStatusType
    flags;

  QuantizeInfo
    quantize_info;

  RectangleInfo
    geometry,
    region_geometry;

  register long
    i;

  /*
    Initialize method variables.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image **) NULL);
  assert((*image)->signature == MagickSignature);
  if ((*image)->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",(*image)->filename);
  if (argc < 0)
    return(MagickTrue);
  for (i=0; i < (long) argc; i++)
    if (strlen(argv[i]) > (MaxTextExtent/2-1))
      ThrowMagickFatalException(OptionFatalError,"OptionLengthExceedsLimit",
        argv[i]);
  draw_info=CloneDrawInfo(image_info,(DrawInfo *) NULL);
  GetMagickPixelPacket(*image,&fill);
  SetGeometryInfo(&geometry_info);
  GetQuantizeInfo(&quantize_info);
  map_image=NewImageList();
  quantize_info.number_colors=0;
  quantize_info.tree_depth=0;
  quantize_info.dither=MagickTrue;
  if (image_info->monochrome != MagickFalse)
    if (IsMonochromeImage(*image,exception) == MagickFalse)
      {
        quantize_info.number_colors=2;
        quantize_info.tree_depth=8;
        quantize_info.colorspace=GRAYColorspace;
      }
  SetGeometry(*image,&region_geometry);
  region_image=NewImageList();
  /*
    Transmogrify the image.
  */
  for (i=0; i < (long) argc; i++)
  {
    option=argv[i];
    if (IsMagickOption(option) == MagickFalse)
      continue;
    count=Max(ParseMagickOption(MagickCommandOptions,MagickFalse,option),0);
    if ((i+count) >= argc)
      break;
    (void) MogrifyImageInfo(image_info,(int) count+1,argv+i,exception);
    switch (*(option+1))
    {
      case 'a':
      {
        if (LocaleCompare("affine",option+1) == 0)
          {
            char
              *p;

            /*
              Draw affine matrix.
            */
            if (*option == '+')
              {
                GetAffineMatrix(&draw_info->affine);
                break;
              }
            p=(char *) argv[i+1];
            draw_info->affine.sx=strtod(p,&p);
            if (*p ==',')
              p++;
            draw_info->affine.rx=strtod(p,&p);
            if (*p ==',')
              p++;
            draw_info->affine.ry=strtod(p,&p);
            if (*p ==',')
              p++;
            draw_info->affine.sy=strtod(p,&p);
            if (*p ==',')
              p++;
            draw_info->affine.tx=strtod(p,&p);
            if (*p ==',')
              p++;
            draw_info->affine.ty=strtod(p,&p);
            break;
          }
        if (LocaleCompare("annotate",option+1) == 0)
          {
            char
              geometry[MaxTextExtent];

            /*
              Annotate image.
            */
            SetGeometryInfo(&geometry_info);
            flags=ParseGeometry(argv[i+1],&geometry_info);
            if ((flags & SigmaValue) == 0)
              geometry_info.sigma=geometry_info.rho;
            (void) CloneString(&draw_info->text,
              TranslateText(image_info,*image,argv[i+2]));
            (void) FormatMagickString(geometry,MaxTextExtent,"%+f%+f",
              geometry_info.xi,geometry_info.psi);
            (void) CloneString(&draw_info->geometry,geometry);
            draw_info->affine.sx=
              cos(DegreesToRadians(fmod(geometry_info.rho,360.0)));
            draw_info->affine.rx=
              sin(DegreesToRadians(fmod(geometry_info.rho,360.0)));
            draw_info->affine.ry=
              (-sin(DegreesToRadians(fmod(geometry_info.sigma,360.0))));
            draw_info->affine.sy=
              cos(DegreesToRadians(fmod(geometry_info.sigma,360.0)));
            (void) AnnotateImage(*image,draw_info);
            break;
          }
        if (LocaleCompare("antialias",option+1) == 0)
          {
            draw_info->stroke_antialias=
              (*option == '-') ? MagickTrue : MagickFalse;
            draw_info->text_antialias=
              (*option == '-') ? MagickTrue : MagickFalse;
            break;
          }
        break;
      }
      case 'b':
      {
        if (LocaleCompare("background",option+1) == 0)
          {
            (*image)->background_color=image_info->background_color;
            break;
          }
        if (LocaleCompare("bias",option+1) == 0)
          {
            (*image)->bias=StringToDouble(argv[i+1],QuantumRange);
            break;
          }
        if (LocaleCompare("black-threshold",option+1) == 0)
          {
            /*
              Threshold black image.
            */
            (void) BlackThresholdImage(*image,argv[i+1]);
            break;
          }
        if (LocaleCompare("blue-primary",option+1) == 0)
          {
            /*
              Blue chromaticity primary point.
            */
            if (*option == '+')
              {
                (*image)->chromaticity.blue_primary.x=0.0;
                (*image)->chromaticity.blue_primary.y=0.0;
                break;
              }
            flags=ParseGeometry(image_info->density,&geometry_info);
            (*image)->chromaticity.blue_primary.x=geometry_info.rho;
            (*image)->chromaticity.blue_primary.y=geometry_info.sigma;
            if ((flags & SigmaValue) == 0)
              (*image)->chromaticity.blue_primary.y=
                (*image)->chromaticity.blue_primary.x;
            break;
          }
        if (LocaleCompare("blur",option+1) == 0)
          {
            Image
              *blur_image;

            /*
              Gaussian blur image.
            */
            flags=ParseGeometry(argv[i+1],&geometry_info);
            if ((flags & SigmaValue) == 0)
              geometry_info.sigma=1.0;
            blur_image=BlurImageChannel(*image,image_info->channel,
              geometry_info.rho,geometry_info.sigma,exception);
            if (blur_image == (Image *) NULL)
              break;
            InheritException(&blur_image->exception,exception);
            *image=DestroyImage(*image);
            *image=blur_image;
            break;
          }
        if (LocaleCompare("border",option+1) == 0)
          {
            Image
              *border_image;

            /*
              Surround image with a border of solid color.
            */
            flags=ParsePageGeometry(*image,argv[i+1],&geometry);
            if ((flags & SigmaValue) == 0)
              geometry.height=geometry.width;
            border_image=BorderImage(*image,&geometry,exception);
            if (border_image == (Image *) NULL)
              break;
            InheritException(&border_image->exception,exception);
            *image=DestroyImage(*image);
            *image=border_image;
            break;
          }
        if (LocaleCompare("bordercolor",option+1) == 0)
          {
            (*image)->border_color=image_info->border_color;
            draw_info->border_color=image_info->border_color;
            break;
          }
        if (LocaleCompare("box",option+1) == 0)
          {
            (void) QueryColorDatabase(argv[i+1],&draw_info->undercolor,
              exception);
            break;
          }
        break;
      }
      case 'c':
      {
        if (LocaleCompare("charcoal",option+1) == 0)
          {
            Image
              *charcoal_image;

            /*
              Charcoal image.
            */
            flags=ParseGeometry(argv[i+1],&geometry_info);
            if ((flags & SigmaValue) == 0)
              geometry_info.sigma=1.0;
            charcoal_image=CharcoalImage(*image,geometry_info.rho,
              geometry_info.sigma,exception);
            if (charcoal_image == (Image *) NULL)
              break;
            InheritException(&charcoal_image->exception,exception);
            *image=DestroyImage(*image);
            *image=charcoal_image;
            break;
          }
        if (LocaleCompare("chop",option+1) == 0)
          {
            Image
              *chop_image;

            /*
              Chop the image.
            */
            (void) ParseGravityGeometry(*image,argv[i+1],&geometry);
            chop_image=ChopImage(*image,&geometry,exception);
            if (chop_image == (Image *) NULL)
              break;
            InheritException(&chop_image->exception,exception);
            *image=DestroyImage(*image);
            *image=chop_image;
            break;
          }
        if (LocaleCompare("clip",option+1) == 0)
          {
            if (*option == '+')
              {
                (void) SetImageClipMask(*image,(Image *) NULL);
                break;
              }
            (void) ClipImage(*image);
            break;
          }
        if (LocaleCompare("clip-path",option+1) == 0)
          {
            (void) ClipPathImage(*image,argv[i+1],(MagickBooleanType)
              (*option == '-'));
            break;
          }
        if (LocaleCompare("colorize",option+1) == 0)
          {
            Image
              *colorize_image;

            /*
              Colorize the image.
            */
            colorize_image=ColorizeImage(*image,argv[i+1],draw_info->fill,
              exception);
            if (colorize_image == (Image *) NULL)
              break;
            InheritException(&colorize_image->exception,exception);
            *image=DestroyImage(*image);
            *image=colorize_image;
            break;
          }
        if (LocaleCompare("colors",option+1) == 0)
          {
            quantize_info.number_colors=image_info->colors;
            break;
          }
        if (LocaleCompare("colorspace",option+1) == 0)
          {
            if (*option == '+')
              {
                quantize_info.colorspace=image_info->colorspace;
                (void) SetImageColorspace(*image,RGBColorspace);
                break;
              }
            quantize_info.colorspace=image_info->colorspace;
            if (quantize_info.colorspace == CMYKColorspace)
              {
                (void) SetImageColorspace(*image,CMYKColorspace);
                quantize_info.colorspace=RGBColorspace;
                break;
              }
            if (quantize_info.colorspace == GRAYColorspace)
              {
                (void) SetImageColorspace(*image,GRAYColorspace);
                quantize_info.colorspace=GRAYColorspace;
                break;
              }
            if (quantize_info.colorspace == RGBColorspace)
              {
                (void) SetImageColorspace(*image,RGBColorspace);
                quantize_info.colorspace=RGBColorspace;
                break;
              }
            break;
          }
        if (LocaleCompare("compose",option+1) == 0)
          {
            if (*option == '+')
              (*image)->compose=OverCompositeOp;
            else
              (*image)->compose=(CompositeOperator) ParseMagickOption(
                MagickCompositeOptions,MagickFalse,argv[i+1]);
            break;
          }
        if (LocaleCompare("compress",option+1) == 0)
          {
            (*image)->compression=image_info->compression;
            break;
          }
        if (LocaleCompare("contrast",option+1) == 0)
          {
            (void) ContrastImage(*image,(*option == '-') ? MagickTrue :
              MagickFalse);
            break;
          }
        if (LocaleCompare("convolve",option+1) == 0)
          {
            char
              *p,
              token[MaxTextExtent];

            double
              *kernel;

            Image
              *convolve_image;

            register long
              x;

            unsigned long
              order;

            /*
              Convolve image.
            */
            p=(char *) argv[i+1];
            for (x=0; *p != '\0'; x++)
            {
              GetMagickToken(p,&p,token);
              if (*token == ',')
                GetMagickToken(p,&p,token);
            }
            order=(unsigned long) sqrt((double) x+1.0);
            kernel=(double *) AcquireMagickMemory(order*order*sizeof(*kernel));
            if (kernel == (double *) NULL)
              ThrowMagickFatalException(ResourceLimitFatalError,
                "MemoryAllocationFailed",(*image)->filename);
            p=(char *) argv[i+1];
            for (x=0; *p != '\0'; x++)
            {
              GetMagickToken(p,&p,token);
              if (*token == ',')
                GetMagickToken(p,&p,token);
              kernel[x]=atof(token);
            }
            for ( ; x < (long) (order*order); x++)
              kernel[x]=0.0;
            convolve_image=ConvolveImage(*image,order,kernel,exception);
            kernel=(double *) RelinquishMagickMemory(kernel);
            if (convolve_image == (Image *) NULL)
              break;
            InheritException(&convolve_image->exception,exception);
            *image=DestroyImage(*image);
            *image=convolve_image;
            break;
          }
        if (LocaleCompare("crop",option+1) == 0)
          {
            if (*option == '+')
              break;
            break;
          }
        if (LocaleCompare("cycle",option+1) == 0)
          {
            /*
              Cycle an image colormap.
            */
            (void) CycleColormapImage(*image,atoi(argv[i+1]));
            break;
          }
        break;
      }
      case 'd':
      {
        if (LocaleCompare("density",option+1) == 0)
          {
            GeometryInfo
              geometry_info;

            /*
              Set image density.
            */
            (void) CloneString(&draw_info->density,image_info->density);
            flags=ParseGeometry(image_info->density,&geometry_info);
            (*image)->x_resolution=geometry_info.rho;
            (*image)->y_resolution=geometry_info.sigma;
            if ((flags & SigmaValue) == 0)
              (*image)->y_resolution=(*image)->x_resolution;
            break;
          }
        if (LocaleCompare("depth",option+1) == 0)
          {
            (*image)->depth=image_info->depth;
            break;
          }
        if (LocaleCompare("despeckle",option+1) == 0)
          {
            Image
              *despeckle_image;

            /*
              Reduce the speckles within an image.
            */
            despeckle_image=DespeckleImage(*image,exception);
            if (despeckle_image == (Image *) NULL)
              break;
            InheritException(&despeckle_image->exception,exception);
            *image=DestroyImage(*image);
            *image=despeckle_image;
            break;
          }
        if (LocaleCompare("display",option+1) == 0)
          {
            (void) CloneString(&draw_info->server_name,
              image_info->server_name);
            break;
          }
        if (LocaleCompare("dither",option+1) == 0)
          {
            quantize_info.dither=(*option == '-') ? MagickTrue : MagickFalse;
            break;
          }
        if (LocaleCompare("draw",option+1) == 0)
          {
            /*
              Draw image.
            */
            (void) CloneString(&draw_info->primitive,argv[i+1]);
            (void) DrawImage(*image,draw_info);
            break;
          }
        break;
      }
      case 'e':
      {
        if (LocaleCompare("edge",option+1) == 0)
          {
            Image
              *edge_image;

            /*
              Enhance edges in the image.
            */
            flags=ParseGeometry(argv[i+1],&geometry_info);
            if ((flags & SigmaValue) == 0)
              geometry_info.sigma=1.0;
            edge_image=EdgeImage(*image,geometry_info.rho,exception);
            if (edge_image == (Image *) NULL)
              break;
            InheritException(&edge_image->exception,exception);
            *image=DestroyImage(*image);
            *image=edge_image;
            break;
          }
        if (LocaleCompare("emboss",option+1) == 0)
          {
            Image
              *emboss_image;

            /*
              Gaussian embossen image.
            */
            flags=ParseGeometry(argv[i+1],&geometry_info);
            if ((flags & SigmaValue) == 0)
              geometry_info.sigma=1.0;
            emboss_image=EmbossImage(*image,geometry_info.rho,
              geometry_info.sigma,exception);
            if (emboss_image == (Image *) NULL)
              break;
            InheritException(&emboss_image->exception,exception);
            *image=DestroyImage(*image);
            *image=emboss_image;
            break;
          }
        if (LocaleCompare("encoding",option+1) == 0)
          {
            (void) CloneString(&draw_info->encoding,argv[i+1]);
            break;
          }
        if (LocaleCompare("endian",option+1) == 0)
          {
            (*image)->endian=image_info->endian;
            break;
          }
        if (LocaleCompare("enhance",option+1) == 0)
          {
            Image
              *enhance_image;

            /*
              Enhance image.
            */
            enhance_image=EnhanceImage(*image,exception);
            if (enhance_image == (Image *) NULL)
              break;
            InheritException(&enhance_image->exception,exception);
            *image=DestroyImage(*image);
            *image=enhance_image;
            break;
          }
        if (LocaleCompare("equalize",option+1) == 0)
          {
            /*
              Equalize image.
            */
            (void) EqualizeImage(*image);
            break;
          }
        if (LocaleCompare("evaluate",option+1) == 0)
          {
            double
              constant;

            MagickEvaluateOperator
              op;

            op=(MagickEvaluateOperator)
              ParseMagickOption(MagickEvaluateOptions,MagickFalse,argv[i+1]);
            constant=StringToDouble(argv[i+2],QuantumRange);
            (void) EvaluateImageChannel(*image,image_info->channel,op,constant,
              exception);
            break;
          }
        if (LocaleCompare("extent",option+1) == 0)
          {
            /*
              Set the image extent.
            */
            (void) ParseAbsoluteGeometry(argv[i+1],&geometry);
            (void) SetImageExtent(*image,geometry.width,geometry.height);
            break;
          }
        if (LocaleCompare("bordercolor",option+1) == 0)
          {
            (*image)->border_color=image_info->border_color;
            draw_info->border_color=image_info->border_color;
            break;
          }
        if (LocaleCompare("box",option+1) == 0)
          {
            (void) QueryColorDatabase(argv[i+1],&draw_info->undercolor,
              exception);
            break;
          }
        break;
      }
      case 'f':
      {
        if (LocaleCompare("family",option+1) == 0)
          {
            if (*option == '+')
              (void) CloneString(&draw_info->family,"");
           else
              (void) CloneString(&draw_info->family,argv[i+1]);
            break;
          }
        if (LocaleCompare("fill",option+1) == 0)
          {
            if (*option == '+')
              (void) QueryMagickColor("none",&fill,exception);
            else
              (void) QueryMagickColor(argv[i+1],&fill,exception);
            draw_info->fill=image_info->pen;
            break;
          }
        if (LocaleCompare("filter",option+1) == 0)
          {
            if (*option == '+')
              {
                (*image)->filter=UndefinedFilter;
                break;
              }
            (*image)->filter=(FilterTypes)
              ParseMagickOption(MagickFilterOptions,MagickFalse,argv[i+1]);
            break;
          }
        if (LocaleCompare("flip",option+1) == 0)
          {
            Image
              *flip_image;

            /*
              Flip image scanlines.
            */
            flip_image=FlipImage(*image,exception);
            if (flip_image == (Image *) NULL)
              break;
            InheritException(&flip_image->exception,exception);
            *image=DestroyImage(*image);
            *image=flip_image;
            break;
          }
        if (LocaleCompare("flop",option+1) == 0)
          {
            Image
              *flop_image;

            /*
              Flop image scanlines.
            */
            flop_image=FlopImage(*image,exception);
            if (flop_image == (Image *) NULL)
              break;
            InheritException(&flop_image->exception,exception);
            *image=DestroyImage(*image);
            *image=flop_image;
            break;
          }
        if (LocaleCompare("floodfill",option+1) == 0)
          {
            PixelPacket
              target;

            /*
              Floodfill image.
            */
            (void) ParsePageGeometry(*image,argv[i+1],&geometry);
            (void) QueryColorDatabase(argv[i+2],&target,exception);
            (void) ColorFloodfillImage(*image,draw_info,target,geometry.x,
              geometry.y,FloodfillMethod);
            break;
          }
        if (LocaleCompare("font",option+1) == 0)
          {
            (void) CloneString(&draw_info->font,image_info->font);
            break;
          }
        if (LocaleCompare("frame",option+1) == 0)
          {
            FrameInfo
              frame_info;

            Image
              *frame_image;

            /*
              Surround image with an ornamental border.
            */
            (void) ParsePageGeometry(*image,argv[i+1],&geometry);
            frame_info.width=geometry.width;
            frame_info.height=geometry.height;
            frame_info.outer_bevel=geometry.x;
            frame_info.inner_bevel=geometry.y;
            frame_info.x=(long) frame_info.width;
            frame_info.y=(long) frame_info.height;
            frame_info.width=(*image)->columns+2*frame_info.width;
            frame_info.height=(*image)->rows+2*frame_info.height;
            frame_image=FrameImage(*image,&frame_info,exception);
            if (frame_image == (Image *) NULL)
              break;
            InheritException(&frame_image->exception,exception);
            *image=DestroyImage(*image);
            *image=frame_image;
            break;
          }
        if (LocaleCompare("fuzz",option+1) == 0)
          {
            (*image)->fuzz=image_info->fuzz;
            break;
          }
        break;
      }
      case 'g':
      {
        if (LocaleCompare("gamma",option+1) == 0)
          {
            if (*option == '+')
              (*image)->gamma=atof(argv[i+1]);
            else
              {
                if (strchr(argv[i+1],',') != (char *) NULL)
                  (void) GammaImage(*image,argv[i+1]);
                else
                  (void) GammaImageChannel(*image,image_info->channel,
                    atof(argv[i+1]));
              }
            break;
          }
        if (LocaleCompare("gaussian",option+1) == 0)
          {
            Image
              *gaussian_image;

            /*
              Gaussian blur image.
            */
            flags=ParseGeometry(argv[i+1],&geometry_info);
            if ((flags & SigmaValue) == 0)
              geometry_info.sigma=1.0;
            gaussian_image=GaussianBlurImageChannel(*image,image_info->channel,
              geometry_info.rho,geometry_info.sigma,exception);
            if (gaussian_image == (Image *) NULL)
              break;
            InheritException(&gaussian_image->exception,exception);
            *image=DestroyImage(*image);
            *image=gaussian_image;
            break;
          }
        if (LocaleCompare("geometry",option+1) == 0)
          {
            Image
              *zoom_image;

            /*
              Resize image.
            */
            (void) CloneString(&(*image)->geometry,argv[i+1]);
            (void) ParseSizeGeometry(*image,(*image)->geometry,&geometry);
            zoom_image=ZoomImage(*image,geometry.width,geometry.height,
              exception);
            if (zoom_image == (Image *) NULL)
              break;
            InheritException(&zoom_image->exception,exception);
            *image=DestroyImage(*image);
            *image=zoom_image;
            break;
          }
        if (LocaleCompare("gravity",option+1) == 0)
          {
            if (*option == '+')
              {
                draw_info->gravity=UndefinedGravity;
                (*image)->gravity=UndefinedGravity;
                break;
              }
            (*image)->gravity=(GravityType)
              ParseMagickOption(MagickGravityOptions,MagickFalse,argv[i+1]);
            draw_info->gravity=(*image)->gravity;
            break;
          }
        if (LocaleCompare("green-primary",option+1) == 0)
          {
            /*
              Green chromaticity primary point.
            */
            if (*option == '+')
              {
                (*image)->chromaticity.green_primary.x=0.0;
                (*image)->chromaticity.green_primary.y=0.0;
                break;
              }
            flags=ParseGeometry(image_info->density,&geometry_info);
            (*image)->chromaticity.green_primary.x=geometry_info.rho;
            (*image)->chromaticity.green_primary.y=geometry_info.sigma;
            if ((flags & SigmaValue) == 0)
              (*image)->chromaticity.green_primary.y=
                (*image)->chromaticity.green_primary.x;
            break;
          }
        break;
      }
      case 'i':
      {
        if (LocaleCompare("identify",option+1) == 0)
          {
            char
              *text;

            const char
              *format;

            format=GetImageOption(image_info,"format");
            if (format == (char *) NULL)
              {
                (void) IdentifyImage(*image,stdout,MagickTrue);
                break;
              }
            text=TranslateText(image_info,*image,format);
            if (text == (char *) NULL)
              break;
            (void) fputs(text,stdout);
            (void) fputc('\n',stdout);
            text=(char *) RelinquishMagickMemory(text);
            break;
          }
        if (LocaleCompare("implode",option+1) == 0)
          {
            Image
              *implode_image;

            /*
              Implode image.
            */
            (void) ParseGeometry(argv[i+1],&geometry_info);
            implode_image=ImplodeImage(*image,geometry_info.rho,exception);
            if (implode_image == (Image *) NULL)
              break;
            InheritException(&implode_image->exception,exception);
            *image=DestroyImage(*image);
            *image=implode_image;
            break;
          }
        if (LocaleCompare("intent",option+1) == 0)
          {
            if (*option == '+')
              {
                (*image)->rendering_intent=UndefinedIntent;
                break;
              }
            (*image)->rendering_intent=(RenderingIntent)
              ParseMagickOption(MagickIntentOptions,MagickFalse,argv[i+1]);
            break;
          }
        break;
      }
      case 'l':
      {
        if (LocaleCompare("label",option+1) == 0)
          {
            if (*option == '+')
              {
                (void) RemoveImageOption(image_info,"Label");
                break;
              }
            (void) SetImageOption(image_info,"Label",
              TranslateText(image_info,*image,argv[i+1]));
            break;
          }
        if (LocaleCompare("lat",option+1) == 0)
          {
            Image
              *threshold_image;

            /*
              Local adaptive threshold image.
            */
            flags=ParseGeometry(argv[i+1],&geometry_info);
            if ((flags & PercentValue) != 0)
              geometry_info.xi=QuantumRange*geometry_info.xi/100.0;
            threshold_image=AdaptiveThresholdImage(*image,(unsigned long)
              geometry_info.rho,(unsigned long) geometry_info.sigma,
              (long) geometry_info.xi,exception);
            if (threshold_image == (Image *) NULL)
              break;
            InheritException(&threshold_image->exception,exception);
            *image=DestroyImage(*image);
            *image=threshold_image;
            break;
          }
        if (LocaleCompare("level",option+1) == 0)
          {
            GeometryInfo
              geometry_info;

            MagickRealType
              black_point,
              gamma,
              white_point;

            MagickStatusType
              flags;

            /*
              Parse levels.
            */
            flags=ParseGeometry(argv[i+1],&geometry_info);
            black_point=geometry_info.rho;
            white_point=(MagickRealType) QuantumRange;
            if ((flags & SigmaValue) != 0)
              white_point=geometry_info.sigma;
            gamma=1.0;
            if ((flags & XiValue) != 0)
              gamma=geometry_info.xi;
            if ((white_point <= 10.0) && (gamma > 10.0))
              {
                MagickRealType
                  swap;

                swap=gamma;
                gamma=white_point;
                white_point=swap;
              }
            if ((flags & PercentValue) != 0)
              {
                black_point*=(MagickRealType) QuantumRange/100.0f;
                white_point*=(MagickRealType) QuantumRange/100.0f;
              }
            if ((flags & SigmaValue) == 0)
              white_point=QuantumRange-black_point;
            (void) LevelImageChannel(*image,image_info->channel,black_point,
              white_point,gamma);
            break;
          }
        if (LocaleCompare("linewidth",option+1) == 0)
          {
            draw_info->stroke_width=atof(argv[i+1]);
            break;
          }
        if (LocaleCompare("loop",option+1) == 0)
          {
            /*
              Set image iterations.
            */
            (*image)->iterations=(unsigned long) atol(argv[i+1]);
            break;
          }
        break;
      }
      case 'm':
      {
        if (LocaleCompare("map",option+1) == 0)
          {
            ImageInfo
              *map_info;

            /*
              Transform image colors to match this set of colors.
            */
            if (*option == '+')
              break;
            map_info=CloneImageInfo(image_info);
            (void) CopyMagickString(map_info->filename,argv[i+1],
              MaxTextExtent);
            map_image=ReadImage(map_info,exception);
            CatchException(exception);
            map_info=DestroyImageInfo(map_info);
            break;
          }
        if (LocaleCompare("mask",option+1) == 0)
          {
            Image
              *mask;

            ImageInfo
              *mask_info;

            long
              y;

            register long
              x;

            register PixelPacket
              *q;

            if (*option == '+')
              {
                /*
                  Remove a clip mask.
                */
                (void) SetImageClipMask(*image,(Image *) NULL);
                break;
              }
            /*
              Set the image clip mask.
            */
            mask_info=CloneImageInfo(image_info);
            (void) CopyMagickString(mask_info->filename,argv[i+1],
              MaxTextExtent);
            mask=ReadImage(mask_info,exception);
            CatchException(exception);
            mask_info=DestroyImageInfo(mask_info);
            if (mask == (Image *) NULL)
              break;
            for (y=0; y < (long) mask->rows; y++)
            {
              q=GetImagePixels(mask,0,y,mask->columns,1);
              if (q == (PixelPacket *) NULL)
                break;
              for (x=0; x < (long) mask->columns; x++)
              {
                if (mask->matte == MagickFalse)
                  q->opacity=PixelIntensityToQuantum(q);
                q->red=q->opacity;
                q->green=q->opacity;
                q->blue=q->opacity;
                q++;
              }
              if (SyncImagePixels(mask) == MagickFalse)
                break;
            }
            mask->storage_class=DirectClass;
            mask->matte=MagickTrue;
            (void) SetImageClipMask(*image,mask);
            break;
          }
        if (LocaleCompare("matte",option+1) == 0)
          {
            if (*option == '-')
              if ((*image)->matte == MagickFalse)
                SetImageOpacity(*image,OpaqueOpacity);
            (*image)->matte=(*option == '-') ? MagickTrue : MagickFalse;
            break;
          }
        if (LocaleCompare("mattecolor",option+1) == 0)
          {
            (*image)->matte_color=image_info->matte_color;
            break;
          }
        if (LocaleCompare("median",option+1) == 0)
          {
            Image
              *median_image;

            /*
              Median filter image.
            */
            (void) ParseGeometry(argv[i+1],&geometry_info);
            median_image=MedianFilterImage(*image,geometry_info.rho,exception);
            if (median_image == (Image *) NULL)
              break;
            InheritException(&median_image->exception,exception);
            *image=DestroyImage(*image);
            *image=median_image;
            break;
          }
        if (LocaleCompare("modulate",option+1) == 0)
          {
            (void) ModulateImage(*image,argv[i+1]);
            break;
          }
        if (LocaleCompare("monochrome",option+1) == 0)
          {
            quantize_info.number_colors=2;
            quantize_info.tree_depth=8;
            quantize_info.colorspace=GRAYColorspace;
            break;
          }
        if (LocaleCompare("motion-blur",option+1) == 0)
          {
            Image
              *blur_image;

            /*
              MOtion blur image.
            */
            flags=ParseGeometry(argv[i+1],&geometry_info);
            if ((flags & SigmaValue) == 0)
              geometry_info.sigma=1.0;
            blur_image=MotionBlurImage(*image,geometry_info.rho,
              geometry_info.sigma,geometry_info.xi,exception);
            if (blur_image == (Image *) NULL)
              break;
            InheritException(&blur_image->exception,exception);
            *image=DestroyImage(*image);
            *image=blur_image;
            break;
          }
        break;
      }
      case 'n':
      {
        if (LocaleCompare("negate",option+1) == 0)
          {
            (void) NegateImageChannel(*image,image_info->channel,
              *option == '+' ? MagickTrue : MagickFalse);
            break;
          }
        if (LocaleCompare("noise",option+1) == 0)
          {
            Image
              *noisy_image;

            if (*option != '-')
              noisy_image=AddNoiseImage(*image,(NoiseType) ParseMagickOption(
                MagickNoiseOptions,MagickFalse,argv[i+1]),exception);
            else
              {
                (void) ParseGeometry(argv[i+1],&geometry_info);
                noisy_image=ReduceNoiseImage(*image,geometry_info.rho,
                  exception);
              }
            if (noisy_image == (Image *) NULL)
              break;
            InheritException(&noisy_image->exception,exception);
            *image=DestroyImage(*image);
            *image=noisy_image;
            break;
          }
        if (LocaleCompare("normalize",option+1) == 0)
          {
            (void) NormalizeImage(*image);
            break;
          }
        break;
      }
      case 'o':
      {
        if (LocaleCompare("opaque",option+1) == 0)
          {
            MagickPixelPacket
              target;

            (void) QueryMagickColor(argv[i+1],&target,exception);
            (void) PaintOpaqueImage(*image,&target,&fill);
            break;
          }
        if (LocaleCompare("ordered-dither",option+1) == 0)
          {
            /*
              Ordered-dither image.
            */
            (void) RandomThresholdImageChannel(*image,image_info->channel,
              argv[i+1],exception);
            break;
          }
        if (LocaleCompare("orient",option+1) == 0)
          {
            (*image)->orientation=image_info->orientation;
            break;
          }
        break;
      }
      case 'p':
      {
        if (LocaleCompare("paint",option+1) == 0)
          {
            Image
              *paint_image;

            /*
              Oil paint image.
            */
            (void) ParseGeometry(argv[i+1],&geometry_info);
            paint_image=OilPaintImage(*image,geometry_info.rho,exception);
            if (paint_image == (Image *) NULL)
              break;
            InheritException(&paint_image->exception,exception);
            *image=DestroyImage(*image);
            *image=paint_image;
            break;
          }
        if (LocaleCompare("pen",option+1) == 0)
          {
            if (*option == '+')
              (void) QueryMagickColor("none",&fill,exception);
            else
              (void) QueryMagickColor(argv[i+1],&fill,exception);
            draw_info->fill=image_info->pen;
            break;
          }
        if (LocaleCompare("pointsize",option+1) == 0)
          {
            if (*option == '+')
              (void) ParseGeometry("12",&geometry_info);
            else
              (void) ParseGeometry(argv[i+1],&geometry_info);
            draw_info->pointsize=geometry_info.rho;
            break;
          }
        if (LocaleCompare("posterize",option+1) == 0)
          {
            /*
              Posterize image.
            */
            (void) PosterizeImage(*image,(unsigned long) atol(argv[i+1]),
              quantize_info.dither);
            break;
          }
        if (LocaleCompare("raise",option+1) == 0)
          {
            /*
              Surround image with a raise of solid color.
            */
            (void) ParsePageGeometry(*image,argv[i+1],&geometry);
            (void) RaiseImage(*image,&geometry,(MagickBooleanType)
              (*option == '-'));
            break;
          }
        if (LocaleCompare("preview",option+1) == 0)
          {
            Image
              *preview_image;

            /*
              Preview image.
            */
            preview_image=PreviewImage(*image,image_info->preview_type,
              exception);
            if (preview_image == (Image *) NULL)
              break;
            InheritException(&preview_image->exception,exception);
            *image=DestroyImage(*image);
            *image=preview_image;
            break;
          }
        if (LocaleCompare("profile",option+1) == 0)
          {
            const char
              *name;

            const StringInfo
              *profile;

            Image
              *profile_image;

            ImageInfo
              *profile_info;

            if (*option == '+')
              {
                /*
                  Remove a profile from the image.
                */
                (void) ProfileImage(*image,argv[i+1],
                  (const unsigned char *) NULL,0,MagickTrue);
                break;
              }
            /*
              Associate a profile with the image.
            */
            profile_info=CloneImageInfo(image_info);
            profile_info->client_data=(void *) GetImageProfile(*image,"8bim");
            (void) CopyMagickString(profile_info->filename,argv[i+1],
              MaxTextExtent);
            profile_image=ReadImage(profile_info,exception);
            CatchException(exception);
            profile_info=DestroyImageInfo(profile_info);
            if (profile_image == (Image *) NULL)
              break;
            ResetImageProfileIterator(profile_image);
            name=GetNextImageProfile(profile_image);
            while (name != (const char *) NULL)
            {
              profile=GetImageProfile(profile_image,name);
              if (profile != (StringInfo *) NULL)
                (void) ProfileImage(*image,name,profile->datum,(unsigned long)
                  profile->length,MagickFalse);
              name=GetNextImageProfile(profile_image);
            }
            profile_image=DestroyImage(profile_image);
            break;
          }
        break;
      }
      case 'q':
      {
        if (LocaleCompare("quality",option+1) == 0)
          {
            /*
              Set image compression quality.
            */
            (*image)->quality=image_info->quality;
            break;
          }
        if (LocaleCompare("quiet",option+1) == 0)
          {
            static ErrorHandler
              error_handler = (WarningHandler) NULL;

            static WarningHandler
              warning_handler = (WarningHandler) NULL;

            if (*option == '+')
              {
                /*
                  Restore error or warning messages.
                */
                error_handler=SetErrorHandler((ErrorHandler) NULL);
                warning_handler=SetWarningHandler((WarningHandler) NULL);
                break;
              }
            /*
              Suppress error or warning messages.
            */
            error_handler=SetErrorHandler(error_handler);
            warning_handler=SetWarningHandler(warning_handler);
            break;
          }
        break;
      }
      case 'r':
      {
        if (LocaleCompare("radial-blur",option+1) == 0)
          {
            Image
              *blur_image;

            /*
              Radial blur image.
            */
            blur_image=RadialBlurImageChannel(*image,image_info->channel,
              atof(argv[i+1]),exception);
            if (blur_image == (Image *) NULL)
              break;
            InheritException(&blur_image->exception,exception);
            *image=DestroyImage(*image);
            *image=blur_image;
            break;
          }
        if (LocaleCompare("raise",option+1) == 0)
          {
            /*
              Surround image with a raise of solid color.
            */
            (void) ParsePageGeometry(*image,argv[i+1],&geometry);
            (void) RaiseImage(*image,&geometry,(MagickBooleanType)
              (*option == '-'));
            break;
          }
        if (LocaleCompare("random-threshold",option+1) == 0)
          {
            /*
              Threshold image.
            */
            (void) RandomThresholdImageChannel(*image,image_info->channel,
              argv[i+1],exception);
            break;
          }
        if (LocaleCompare("red-primary",option+1) == 0)
          {
            /*
              Red chromaticity primary point.
            */
            if (*option == '+')
              {
                (*image)->chromaticity.red_primary.x=0.0;
                (*image)->chromaticity.red_primary.y=0.0;
                break;
              }
            flags=ParseGeometry(image_info->density,&geometry_info);
            (*image)->chromaticity.red_primary.x=geometry_info.rho;
            (*image)->chromaticity.red_primary.y=geometry_info.sigma;
            if ((flags & SigmaValue) == 0)
              (*image)->chromaticity.red_primary.y=
                (*image)->chromaticity.red_primary.x;
            break;
          }
        if (LocaleCompare("region",option+1) == 0)
          {
            Image
              *crop_image;

            if (region_image != (Image *) NULL)
              {
                /*
                  Composite region.
                */
                (void) CompositeImage(region_image,
                  (*image)->matte != MagickFalse ? OverCompositeOp :
                  CopyCompositeOp,*image,region_geometry.x,region_geometry.y);
                InheritException(&region_image->exception,exception);
                *image=DestroyImage(*image);
                *image=region_image;
              }
            if (*option == '+')
              break;
            /*
              Apply transformations to a selected region of the image.
            */
            (void) ParseGravityGeometry(*image,argv[i+1],&region_geometry);
            crop_image=CropImage(*image,&region_geometry,exception);
            if (crop_image == (Image *) NULL)
              break;
            region_image=(*image);
            *image=crop_image;
            break;
          }
        if (LocaleCompare("render",option+1) == 0)
          {
            draw_info->render=(MagickBooleanType) (*option == '+');
            break;
          }
        if (LocaleCompare("repage",option+1) == 0)
          {
            if (*option == '+')
              {
                (void) ParseAbsoluteGeometry("0x0+0+0",&(*image)->page);
                break;
              }
            flags=ParseAbsoluteGeometry(argv[i+1],&geometry);
            if ((flags & AspectValue) == 0)
              (*image)->page=geometry;
            else
              {
                (*image)->page.width=geometry.width;
                (*image)->page.height=geometry.height;
                (*image)->page.x-=geometry.x;
                (*image)->page.y-=geometry.y;
              }
            break;
          }
        if (LocaleCompare("resample",option+1) == 0)
          {
            Image
              *resample_image;

            unsigned long
              height,
              width;

            /*
              Resize image.
            */
            if (((*image)->x_resolution == 0.0) ||
                ((*image)->y_resolution == 0.0))
              ThrowMagickFatalException(ImageError,"ImageResolutionNotDefined",
                (*image)->filename);
            flags=ParseGeometry(argv[i+1],&geometry_info);
            if ((flags & SigmaValue) == 0)
              geometry_info.sigma=geometry_info.rho;
            width=(unsigned long) (geometry_info.rho*(*image)->columns/
              (*image)->x_resolution+0.5);
            height=(unsigned long) (geometry_info.sigma*(*image)->rows/
              (*image)->y_resolution+0.5);
            (*image)->x_resolution=geometry_info.rho;
            (*image)->y_resolution=geometry_info.sigma;
            resample_image=ResizeImage(*image,width,height,(*image)->filter,
              (*image)->blur,exception);
            if (resample_image == (Image *) NULL)
              break;
            InheritException(&resample_image->exception,exception);
            *image=DestroyImage(*image);
            *image=resample_image;
            break;
          }
        if (LocaleCompare("resize",option+1) == 0)
          {
            Image
              *resize_image;

            /*
              Resize image.
            */
            (void) ParseSizeGeometry(*image,argv[i+1],&geometry);
            resize_image=ResizeImage(*image,geometry.width,geometry.height,
              (*image)->filter,(*image)->blur,exception);
            if (resize_image == (Image *) NULL)
              break;
            InheritException(&resize_image->exception,exception);
            *image=DestroyImage(*image);
            *image=resize_image;
            break;
          }
        if (LocaleCompare("roll",option+1) == 0)
          {
            Image
              *roll_image;

            /*
              Roll image.
            */
            (void) ParsePageGeometry(*image,argv[i+1],&geometry);
            roll_image=RollImage(*image,geometry.x,geometry.y,exception);
            if (roll_image == (Image *) NULL)
              break;
            InheritException(&roll_image->exception,exception);
            *image=DestroyImage(*image);
            *image=roll_image;
            break;
          }
        if (LocaleCompare("rotate",option+1) == 0)
          {
            Image
              *rotate_image;

            /*
              Check for conditional image rotation.
            */
            if (strchr(argv[i+1],'>') != (char *) NULL)
              if ((*image)->columns <= (*image)->rows)
                break;
            if (strchr(argv[i+1],'<') != (char *) NULL)
              if ((*image)->columns >= (*image)->rows)
                break;
            /*
              Rotate image.
            */
            (void) ParseGeometry(argv[i+1],&geometry_info);
            rotate_image=RotateImage(*image,geometry_info.rho,exception);
            if (rotate_image == (Image *) NULL)
              break;
            InheritException(&rotate_image->exception,exception);
            *image=DestroyImage(*image);
            *image=rotate_image;
            break;
          }
        break;
      }
      case 's':
      {
        if (LocaleCompare("sample",option+1) == 0)
          {
            Image
              *sample_image;

            /*
              Sample image with pixel replication.
            */
            (void) ParseSizeGeometry(*image,argv[i+1],&geometry);
            sample_image=SampleImage(*image,geometry.width,geometry.height,
              exception);
            if (sample_image == (Image *) NULL)
              break;
            InheritException(&sample_image->exception,exception);
            *image=DestroyImage(*image);
            *image=sample_image;
            break;
          }
        if (LocaleCompare("scale",option+1) == 0)
          {
            Image
              *scale_image;

            /*
              Resize image.
            */
            (void) ParseSizeGeometry(*image,argv[i+1],&geometry);
            scale_image=ScaleImage(*image,geometry.width,geometry.height,
              exception);
            if (scale_image == (Image *) NULL)
              break;
            *image=DestroyImage(*image);
            *image=scale_image;
            break;
          }
        if (LocaleCompare("separate",option+1) == 0)
          {
            (void) SeparateImageChannel(*image,image_info->channel);
            break;
          }
        if (LocaleCompare("sepia-tone",option+1) == 0)
          {
            double
              threshold;

            Image
              *sepia_image;

            /*
              Sepia-tone image.
            */
            threshold=StringToDouble(argv[i+1],QuantumRange);
            sepia_image=SepiaToneImage(*image,threshold,exception);
            if (sepia_image == (Image *) NULL)
              break;
            InheritException(&sepia_image->exception,exception);
            *image=DestroyImage(*image);
            *image=sepia_image;
            break;
          }
        if (LocaleCompare("segment",option+1) == 0)
          {
            /*
              Segment image.
            */
            flags=ParseGeometry(argv[i+1],&geometry_info);
            if ((flags & SigmaValue) == 0)
              geometry_info.sigma=1.0;
            (void) SegmentImage(*image,quantize_info.colorspace,
              image_info->verbose,geometry_info.rho,geometry_info.sigma);
            break;
          }
        if (LocaleCompare("set",option+1) == 0)
          {
            /*
              Segment image.
            */
            (void) DeleteImageAttribute(*image,argv[i+1]);
            if (*option == '-')
              (void) SetImageAttribute(*image,argv[i+1],argv[i+2]);
            break;
          }
        if (LocaleCompare("shade",option+1) == 0)
          {
            Image
              *shade_image;

            /*
              Shade image.
            */
            flags=ParseGeometry(argv[i+1],&geometry_info);
            if ((flags & SigmaValue) == 0)
              geometry_info.sigma=0.0;
            shade_image=ShadeImage(*image,(*option == '-') ? MagickTrue :
              MagickFalse,geometry_info.rho,geometry_info.sigma,exception);
            if (shade_image == (Image *) NULL)
              break;
            InheritException(&shade_image->exception,exception);
            *image=DestroyImage(*image);
            *image=shade_image;
            break;
          }
        if (LocaleCompare("shadow",option+1) == 0)
          {
            Image
              *shadow_image;

            /*
              Charcoal image.
            */
            flags=ParseGeometry(argv[i+1],&geometry_info);
            if ((flags & SigmaValue) == 0)
              geometry_info.sigma=1.0;
            if ((flags & XiValue) == 0)
              geometry_info.xi=4.0;
            if ((flags & PsiValue) == 0)
              geometry_info.psi=4.0;
            shadow_image=ShadowImage(*image,geometry_info.rho,
              geometry_info.sigma,(long) (geometry_info.xi+0.5),(long)
              (geometry_info.psi+0.5),exception);
            if (shadow_image == (Image *) NULL)
              break;
            InheritException(&shadow_image->exception,exception);
            *image=DestroyImage(*image);
            *image=shadow_image;
            break;
          }
        if (LocaleCompare("sharpen",option+1) == 0)
          {
            Image
              *sharp_image;

            /*
              Sharpen image.
            */
            flags=ParseGeometry(argv[i+1],&geometry_info);
            if ((flags & SigmaValue) == 0)
              geometry_info.sigma=1.0;
            sharp_image=SharpenImageChannel(*image,image_info->channel,
              geometry_info.rho,geometry_info.sigma,exception);
            if (sharp_image == (Image *) NULL)
              break;
            InheritException(&sharp_image->exception,exception);
            *image=DestroyImage(*image);
            *image=sharp_image;
            break;
          }
        if (LocaleCompare("shave",option+1) == 0)
          {
            Image
              *shave_image;

            /*
              Shave the image edges.
            */
            (void) ParsePageGeometry(*image,argv[i+1],&geometry);
            shave_image=ShaveImage(*image,&geometry,exception);
            if (shave_image == (Image *) NULL)
              break;
            InheritException(&shave_image->exception,exception);
            *image=DestroyImage(*image);
            *image=shave_image;
            break;
          }
        if (LocaleCompare("shear",option+1) == 0)
          {
            Image
              *shear_image;

            /*
              Shear image.
            */
            flags=ParseGeometry(argv[i+1],&geometry_info);
            if ((flags & SigmaValue) == 0)
              geometry_info.sigma=geometry_info.rho;
            shear_image=ShearImage(*image,geometry_info.rho,geometry_info.sigma,
              exception);
            if (shear_image == (Image *) NULL)
              break;
            InheritException(&shear_image->exception,exception);
            *image=DestroyImage(*image);
            *image=shear_image;
            break;
          }
        if (LocaleCompare("sigmoidal-contrast",option+1) == 0)
          {
            /*
              Sigmoidal non-linearity contrast control.
            */
            flags=ParseGeometry(argv[i+1],&geometry_info);
            if ((flags & SigmaValue) == 0)
              geometry_info.sigma=QuantumRange/2.0;
            if ((flags & PercentValue) != 0)
              geometry_info.sigma=QuantumRange*geometry_info.sigma/100.0;
            (void) SigmoidalContrastImageChannel(*image,image_info->channel,
              (*option == '-') ? MagickTrue : MagickFalse,geometry_info.rho,
              geometry_info.sigma);
            break;
          }
        if (LocaleCompare("solarize",option+1) == 0)
          {
            double
              threshold;

            threshold=StringToDouble(argv[i+1],QuantumRange);
            (void) SolarizeImage(*image,threshold);
            break;
          }
        if (LocaleCompare("splice",option+1) == 0)
          {
            Image
              *splice_image;

            /*
              Splice a solid color into the image.
            */
            (void) ParseGravityGeometry(*image,argv[i+1],&geometry);
            switch ((*image)->gravity)
            {
              default:
              case UndefinedGravity:
              case NorthWestGravity:
                break;
              case NorthGravity:
              {
                geometry.x+=geometry.width/2;
                break;
              }
              case NorthEastGravity:
              {
                geometry.x+=geometry.width;
                break;
              }
              case WestGravity:
              {
                geometry.x+=geometry.width/2;
                break;
              }
              case StaticGravity:
              case CenterGravity:
              {
                geometry.x+=geometry.width/2;
                geometry.y+=geometry.height/2;
                break;
              }
              case EastGravity:
              {
                geometry.x+=geometry.width;
                geometry.y+=geometry.height/2;
                break;
              }
              case SouthWestGravity:
              {
                geometry.y+=geometry.height/2;
                break;
              }
              case SouthGravity:
              {
                geometry.x+=geometry.width/2;
                geometry.y+=geometry.height;
                break;
              }
              case SouthEastGravity:
              {
                geometry.x+=geometry.width;
                geometry.y+=geometry.height;
                break;
              }
            }
            splice_image=SpliceImage(*image,&geometry,exception);
            if (splice_image == (Image *) NULL)
              break;
            InheritException(&splice_image->exception,exception);
            *image=DestroyImage(*image);
            *image=splice_image;
            break;
          }
        if (LocaleCompare("spread",option+1) == 0)
          {
            Image
              *spread_image;

            /*
              Spread an image.
            */
            (void) ParseGeometry(argv[i+1],&geometry_info);
            spread_image=SpreadImage(*image,geometry_info.rho,exception);
            if (spread_image == (Image *) NULL)
              break;
            InheritException(&spread_image->exception,exception);
            *image=DestroyImage(*image);
            *image=spread_image;
            break;
          }
        if (LocaleCompare("stretch",option+1) == 0)
          {
            if (*option == '+')
              {
                draw_info->stretch=UndefinedStretch;
                break;
              }
            draw_info->stretch=(StretchType)
              ParseMagickOption(MagickStretchOptions,MagickFalse,argv[i+1]);
            break;
          }
        if (LocaleCompare("strip",option+1) == 0)
          {
            /*
              Strip image of profiles and comments.
            */
            (void) StripImage(*image);
            break;
          }
        if (LocaleCompare("stroke",option+1) == 0)
          {
            if (*option == '+')
              (void) QueryColorDatabase("none",&draw_info->stroke,exception);
            else
              (void) QueryColorDatabase(argv[i+1],&draw_info->stroke,exception);
            break;
          }
        if (LocaleCompare("strokewidth",option+1) == 0)
          {
            draw_info->stroke_width=atof(argv[i+1]);
            break;
          }
        if (LocaleCompare("style",option+1) == 0)
          {
            if (*option == '+')
              {
                draw_info->style=UndefinedStyle;
                break;
              }
            draw_info->style=(StyleType)
              ParseMagickOption(MagickStyleOptions,MagickFalse,argv[i+1]);
            break;
          }
        if (LocaleCompare("support",option+1) == 0)
          {
            (*image)->blur=1.0;
            if (*option == '+')
              break;
            (*image)->blur=atof(argv[i+1]);
            break;
          }
        if (LocaleCompare("swirl",option+1) == 0)
          {
            Image
              *swirl_image;

            /*
              Swirl image.
            */
            (void) ParseGeometry(argv[i+1],&geometry_info);
            swirl_image=SwirlImage(*image,geometry_info.rho,exception);
            if (swirl_image == (Image *) NULL)
              break;
            InheritException(&swirl_image->exception,exception);
            *image=DestroyImage(*image);
            *image=swirl_image;
            break;
          }
        break;
      }
      case 't':
      {
        if (LocaleCompare("threshold",option+1) == 0)
          {
            double
              threshold;

            /*
              Threshold image.
            */
            threshold=StringToDouble(argv[i+1],QuantumRange);
            (void) BilevelImageChannel(*image,image_info->channel,threshold);
            break;
          }
        if (LocaleCompare("thumbnail",option+1) == 0)
          {
            Image
              *thumbnail_image;

            /*
              Thumbnail image.
            */
            (void) ParseSizeGeometry(*image,argv[i+1],&geometry);
            thumbnail_image=ThumbnailImage(*image,geometry.width,
              geometry.height,exception);
            if (thumbnail_image == (Image *) NULL)
              break;
            InheritException(&thumbnail_image->exception,exception);
            *image=DestroyImage(*image);
            *image=thumbnail_image;
            break;
          }
        if (LocaleCompare("tile",option+1) == 0)
          {
            Image
              *fill_pattern;

            ImageInfo
              *tile_info;

            if (*option == '+')
              {
                draw_info->fill_pattern=DestroyImage(draw_info->fill_pattern);
                break;
              }
            tile_info=CloneImageInfo(image_info);
            (void) CopyMagickString(tile_info->filename,argv[i+1],
              MaxTextExtent);
            fill_pattern=ReadImage(tile_info,exception);
            CatchException(exception);
            tile_info=DestroyImageInfo(tile_info);
            if (fill_pattern == (Image *) NULL)
              break;
            draw_info->fill_pattern=
              CloneImage(fill_pattern,0,0,MagickTrue,exception);
            fill_pattern=DestroyImage(fill_pattern);
            break;
          }
        if (LocaleCompare("tint",option+1) == 0)
          {
            Image
              *tint_image;

            /*
              Tint the image.
            */
            tint_image=TintImage(*image,argv[i+1],draw_info->fill,exception);
            if (tint_image == (Image *) NULL)
              break;
            InheritException(&tint_image->exception,exception);
            *image=DestroyImage(*image);
            *image=tint_image;
            break;
          }
        if (LocaleCompare("transform",option+1) == 0)
          {
            Image
              *transform_image;

            /*
              Affine transform image.
            */
            transform_image=AffineTransformImage(*image,&draw_info->affine,
              exception);
            if (transform_image == (Image *) NULL)
              break;
            InheritException(&transform_image->exception,exception);
            *image=DestroyImage(*image);
            *image=transform_image;
            break;
          }
        if (LocaleCompare("transparent",option+1) == 0)
          {
            MagickPixelPacket
              target;

            (void) QueryMagickColor(argv[i+1],&target,exception);
            (void) PaintTransparentImage(*image,&target,TransparentOpacity);
            break;
          }
        if (LocaleCompare("treedepth",option+1) == 0)
          {
            quantize_info.tree_depth=(unsigned long) atol(argv[i+1]);
            break;
          }
        if (LocaleCompare("trim",option+1) == 0)
          {
            Image
              *trim_image;

            trim_image=TrimImage(*image,exception);
            if (trim_image == (Image *) NULL)
              break;
            InheritException(&trim_image->exception,exception);
            *image=DestroyImage(*image);
            *image=trim_image;
            break;
          }
        if (LocaleCompare("type",option+1) == 0)
          {
            (void) SetImageType(*image,image_info->type);
            break;
          }
        break;
      }
      case 'u':
      {
        if (LocaleCompare("undercolor",option+1) == 0)
          {
            (void) QueryColorDatabase(argv[i+1],&draw_info->undercolor,
              exception);
            break;
          }
        if (LocaleCompare("units",option+1) == 0)
          {
            if ((*image)->units != image_info->units)
              switch ((*image)->units)
              {
                case UndefinedResolution:
                {
                  (*image)->x_resolution=0.0;
                  (*image)->y_resolution=0.0;
                  break;
                }
                case PixelsPerInchResolution:
                {
                  if (image_info->units == PixelsPerCentimeterResolution)
                    {
                      (*image)->x_resolution*=2.54;
                      (*image)->y_resolution*=2.54;
                    }
                  break;
                }
                case PixelsPerCentimeterResolution:
                {
                  if (image_info->units == PixelsPerInchResolution)
                    {
                      (*image)->x_resolution/=2.54;
                      (*image)->y_resolution/=2.54;
                    }
                  break;
                }
              }
            (*image)->units=image_info->units;
            break;
          }
        if (LocaleCompare("unsharp",option+1) == 0)
          {
            Image
              *unsharp_image;

            /*
              Unsharp mask image.
            */
            flags=ParseGeometry(argv[i+1],&geometry_info);
            if ((flags & SigmaValue) == 0)
              geometry_info.sigma=1.0;
            if ((flags & XiValue) == 0)
              geometry_info.xi=1.0;
            if ((flags & PsiValue) == 0)
              geometry_info.psi=0.05;
            unsharp_image=UnsharpMaskImageChannel(*image,image_info->channel,
              geometry_info.rho,geometry_info.sigma,geometry_info.xi,
              geometry_info.psi,exception);
            if (unsharp_image == (Image *) NULL)
              break;
            InheritException(&unsharp_image->exception,exception);
            *image=DestroyImage(*image);
            *image=unsharp_image;
            break;
          }
        break;
      }
      case 'v':
      {
        if (LocaleCompare("verbose",option+1) == 0)
          {
            quantize_info.measure_error=image_info->verbose;
            break;
          }
        if (LocaleCompare("version",option+1) == 0)
          break;
        if (LocaleCompare("virtual-pixel",option+1) == 0)
          {
            if (*option == '+')
              {
                (void) SetImageVirtualPixelMethod(*image,
                  UndefinedVirtualPixelMethod);
                break;
              }
            (void) SetImageVirtualPixelMethod(*image,(VirtualPixelMethod)
              ParseMagickOption(MagickVirtualPixelOptions,MagickFalse,
              argv[i+1]));
            break;
          }
        break;
      }
      case 'w':
      {
        if (LocaleCompare("wave",option+1) == 0)
          {
            Image
              *wave_image;

            /*
              Wave image.
            */
            flags=ParseGeometry(argv[i+1],&geometry_info);
            if ((flags & SigmaValue) == 0)
              geometry_info.sigma=1.0;
            wave_image=WaveImage(*image,geometry_info.rho,geometry_info.sigma,
              exception);
            if (wave_image == (Image *) NULL)
              break;
            InheritException(&wave_image->exception,exception);
            *image=DestroyImage(*image);
            *image=wave_image;
            break;
          }
        if (LocaleCompare("weight",option+1) == 0)
          {
            draw_info->weight=(unsigned long) atol(argv[i+1]);
            if (LocaleCompare(argv[i+1],"all") == 0)
              draw_info->weight=0;
            if (LocaleCompare(argv[i+1],"bold") == 0)
              draw_info->weight=700;
            if (LocaleCompare(argv[i+1],"bolder") == 0)
              if (draw_info->weight <= 800)
                draw_info->weight+=100;
            if (LocaleCompare(argv[i+1],"lighter") == 0)
              if (draw_info->weight >= 100)
                draw_info->weight-=100;
            if (LocaleCompare(argv[i+1],"normal") == 0)
              draw_info->weight=400;
            break;
          }
        if (LocaleCompare("white-point",option+1) == 0)
          {
            /*
              White chromaticity point.
            */
            if (*option == '+')
              {
                (*image)->chromaticity.white_point.x=0.0;
                (*image)->chromaticity.white_point.y=0.0;
                break;
              }
            flags=ParseGeometry(image_info->density,&geometry_info);
            (*image)->chromaticity.white_point.x=geometry_info.rho;
            (*image)->chromaticity.white_point.y=geometry_info.sigma;
            if ((flags & SigmaValue) == 0)
              (*image)->chromaticity.white_point.y=
                (*image)->chromaticity.white_point.x;
            break;
          }
        if (LocaleCompare("white-threshold",option+1) == 0)
          {
            /*
              Threshold white image.
            */
            (void) WhiteThresholdImage(*image,argv[i+1]);
            break;
          }
        break;
      }
      default:
        break;
    }
    i+=count;
  }
  if (region_image != (Image *) NULL)
    {
      /*
        Composite transformed region onto image.
      */
      (void) CompositeImage(region_image, (*image)->matte != MagickFalse ?
        OverCompositeOp : CopyCompositeOp,*image,region_geometry.x,
        region_geometry.y);
      InheritException(&region_image->exception,exception);
      *image=DestroyImage(*image);
      *image=region_image;
    }
  if (quantize_info.number_colors != 0)
    {
      /*
        Reduce the number of colors in the image.
      */
      if (((*image)->storage_class == DirectClass) ||
          (*image)->colors > quantize_info.number_colors)
        (void) QuantizeImage(&quantize_info,*image);
      else
        CompressImageColormap(*image);
    }
  if (map_image != (Image *) NULL)
    {
      (void) MapImage(*image,map_image,quantize_info.dither);
      map_image=DestroyImage(map_image);
    }
  /*
    Free resources.
  */
  draw_info=DestroyDrawInfo(draw_info);
  status=(*image)->exception.severity == UndefinedException ?
    MagickTrue : MagickFalse;
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%    M o g r i f y I m a g e C o m m a n d                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MogrifyImageCommand() transforms an image or a sequence of images. These
%  transforms include image scaling, image rotation, color reduction, and
%  others. The transmogrified image overwrites the original image.
%
%  The format of the MogrifyImageCommand method is:
%
%      MagickBooleanType MogrifyImageCommand(ImageInfo *image_info,int argc,
%        const char **argv,char **metadata,ExceptionInfo *exception)
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

static void MogrifyUsage(void)
{
  static const char
    *options[]=
    {
      "-affine matrix       affine transform matrix",
      "-annotate geometry text",
      "                     annotate the image with text",
      "-antialias           remove pixel-aliasing",
      "-authenticate value  decrypt image with this password",
      "-background color    background color",
      "-bias value          add bias when convolving an image",
      "-black-threshold value",
      "                     forces all pixels below the threshold into black",
      "-blue-primary point  chromaticity blue primary point",
      "-blur geometry       blur the image",
      "-border geometry     surround image with a border of color",
      "-bordercolor color   border color",
      "-channel type        apply option to select image channels",
      "-charcoal radius     simulate a charcoal drawing",
      "-chop geometry       remove pixels from the image interior",
      "-colorize value      colorize the image with the fill color",
      "-colors value        preferred number of colors in the image",
      "-colorspace type     alternate image colorspace",
      "-comment string      annotate image with comment",
      "-compress type       type of pixel compression when writing the image",
      "-contrast            enhance or reduce the image contrast",
      "-convolve coefficients",
      "                     apply a convolution kernel to the image",
      "-crop geometry       preferred size and location of the cropped image",
      "-cycle amount        cycle the image colormap",
      "-debug events        display copious debugging information",
      "-define format:option",
      "                     define one or more image format options",
      "-delay value         display the next image after pausing",
      "-density geometry    horizontal and vertical density of the image",
      "-depth value         image depth",
      "-despeckle           reduce the speckles within an image",
      "-display server      get image or font from this X server",
      "-dispose method      GIF disposal method",
      "-dither              apply Floyd/Steinberg error diffusion to image",
      "-draw string         annotate the image with a graphic primitive",
      "-edge radius         apply a filter to detect edges in the image",
      "-emboss radius       emboss an image",
      "-encoding type       text encoding type",
      "-endian type         endianness (MSB or LSB) of the image",
      "-enhance             apply a digital filter to enhance a noisy image",
      "-equalize            perform histogram equalization to an image",
      "-evaluate operator value",
      "                     evaluate an arithmetic, relational, or logical expression",
      "-extent geometry     set the image size",
      "-extract geometry    extract area from image",
      "-family name         render text with this font family",
      "-fill color          color to use when filling a graphic primitive",
      "-filter type         use this filter when resizing an image",
      "-flip                flip image in the vertical direction",
      "-flop                flop image in the horizontal direction",
      "-floodfill geometry color",
      "                     floodfill the image with color",
      "-font name           render text with this font",
      "-format type         image format type",
      "-frame geometry      surround image with an ornamental border",
      "-fuzz distance       colors within this distance are considered equal",
      "-gamma value         level of gamma correction",
      "-gaussian geometry   gaussian blur an image",
      "-geometry geometry   perferred size or location of the image",
      "-green-primary point chromaticity green primary point",
      "-implode amount      implode image pixels about the center",
      "-intent type         type of rendering intent when managing the image color",
      "-interlace type      type of image interlacing scheme",
      "-help                print program options",
      "-label name          assign a label to an image",
      "-lat geometry        local adaptive thresholding",
      "-level value         adjust the level of image contrast",
      "-limit type value    pixel cache resource limit",
      "-log format          format of debugging information",
      "-loop iterations     add Netscape loop extension to your GIF animation",
      "-map filename        transform image colors to match this set of colors",
      "-mask filename       set the image clip mask",
      "-matte               store matte channel if the image has one",
      "-median radius       apply a median filter to the image",
      "-modulate value      vary the brightness, saturation, and hue",
      "-monitor             monitor progress",
      "-monochrome          transform image to black and white",
      "-negate              replace every pixel with its complementary color ",
      "-noise radius        add or reduce noise in an image.",
      "-normalize           transform image to span the full range of colors",
      "-opaque color        change this color to the fill color",
      "-ordered-dither NxN",
      "                     ordered dither the image",
      "-orient type         image orientation",
      "-page geometry       size and location of an image canvas (setting)",
      "-paint radius        simulate an oil painting",
      "-pointsize value     font point size",
      "-posterize levels    reduce the image to a limited number of color levels",
      "-profile filename    add ICM or IPTC information profile to image",
      "-quality value       JPEG/MIFF/PNG compression level",
      "-quiet               suppress all error or warning messages",
      "-raise value         lighten/darken image edges to create a 3-D effect",
      "-random-threshold low,high",
      "                     random threshold the image",
      "-radial-blur angle   radial blur the image",
      "-red-primary point   chromaticity red primary point",
      "-region geometry     apply options to a portion of the image",
      "-repage geometry     size and location of an image canvas (operator)",
      "-resample geometry   change the resolution of an image",
      "-resize geometry     perferred size or location of the image",
      "-roll geometry       roll an image vertically or horizontally",
      "-rotate degrees      apply Paeth rotation to the image",
      "-sample geometry     scale image with pixel sampling",
      "-sampling-factor geometry",
      "                     horizontal and vertical sampling factor",
      "-scale geometry      scale the image",
      "-sigmodial-contrast geometry",
      "                     lightness rescaling using sigmoidal contrast enhancement",
      "-scene number        image scene number",
      "-seed value          pseudo-random number generator seed value",
      "-segment values      segment an image",
      "-separate            separate an image channel into a grayscale image",
      "-sepia-tone threshold",
      "                     simulate a sepia-toned photo",
      "-set attribute value set an image attribute",
      "-shade degrees       shade the image using a distant light source",
      "-shadow geometry     simulate an image shadow",
      "-sharpen radius      sharpen the image",
      "-shear geometry      slide one edge of the image along the X or Y axis",
      "-size geometry       width and height of image",
      "-solarize threshold  negate all pixels above the threshold level",
      "-spread amount       displace image pixels by a random amount",
      "-stretch type        render text with this font stretch",
      "-strip               strip image of all profiles and comments",
      "-stroke color        graphic primitive stroke color",
      "-strokewidth value   graphic primitive stroke width",
      "-style type          render text with this font style",
      "-support factor      resize support: > 1.0 is blurry, < 1.0 is sharp",
      "-swirl degrees       swirl image pixels about the center",
      "-texture filename    name of texture to tile onto the image background",
      "-threshold value     threshold the image",
      "-thumbnail geometry  create a thumbnail of the image",
      "-tile filename       tile image when filling a graphic primitive",
      "-tint value          tint the image with the fill color",
      "-transform           affine transform image",
      "-transparent color   make this color transparent within the image",
      "-treedepth value     color tree depth",
      "-trim                trim image edges",
      "-type type           image type",
      "-undercolor color    annotation bounding box color",
      "-units type          the units of image resolution",
      "-unsharp geometry    sharpen the image",
      "-verbose             print detailed information about the image",
      "-version             print version information",
      "-view                FlashPix viewing transforms",
      "-virtual-pixel method",
      "                     virtual pixel access method",
      "-wave geometry       alter an image along a sine wave",
      "-weight type         render text with this font weight",
      "-white-point point   chromaticity white point",
      "-white-threshold value",
      "                     forces all pixels above the threshold into white",
      (char *) NULL
    };

  const char
    **p;

  (void) printf("Version: %s\n",GetMagickVersion((unsigned long *) NULL));
  (void) printf("Copyright: %s\n\n",GetMagickCopyright());
  (void) printf("Usage: %s [options ...] file [ [options ...] file ...]\n",
    GetClientName());
  (void) printf("\nWhere options include: \n");
  for (p=options; *p != (char *) NULL; p++)
    (void) printf("  %s\n",*p);
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

MagickExport MagickBooleanType MogrifyImageCommand(ImageInfo *image_info,
  int argc,char **argv,char **magick_unused(metadata),ExceptionInfo *exception)
{
#define DestroyMogrify() \
{ \
  for ( ; k >= 0; k--) \
    image_stack[k]=DestroyImageList(image_stack[k]); \
  for (i=0; i < (long) argc; i++) \
    argv[i]=(char *) RelinquishMagickMemory(argv[i]); \
  argv=(char **) RelinquishMagickMemory(argv); \
}
#define ThrowMogrifyException(asperity,tag,option) \
{ \
  if (exception->severity == UndefinedException) \
    (void) ThrowMagickException(exception,GetMagickModule(),asperity,tag, \
      "`%s'",option); \
  DestroyMogrify(); \
  return(MagickFalse); \
}
#define ThrowMogrifyInvalidArgumentException(option,argument) \
{ \
  (void) ThrowMagickException(exception,GetMagickModule(),OptionError, \
    "InvalidArgument","`%s': %s",argument,option); \
  DestroyMogrify(); \
  return(MagickFalse); \
}

  char
    *format,
    *option;

  Image
    *image_stack[MaxImageStackDepth+1];

  long
    j,
    k;

  register long
    i;

  MagickBooleanType
    global_colormap;

  MagickStatusType
    pend,
    status;

  /*
    Set defaults.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(exception != (ExceptionInfo *) NULL);
  if (argc < 2)
    MogrifyUsage();
  format=(char *) NULL;
  global_colormap=MagickFalse;
  k=0;
  j=1;
  image_stack[k]=NewImageList();
  pend=MagickFalse;
  option=(char *) NULL;
  status=MagickTrue;
  /*
    Parse command line.
  */
  ReadCommandlLine(argc,&argv);
  status=ExpandFilenames(&argc,&argv);
  if (status == MagickFalse)
    ThrowMogrifyException(ResourceLimitError,"MemoryAllocationFailed",
      strerror(errno));
  for (i=1; i < (long) argc; i++)
  {
    option=argv[i];
    if (LocaleCompare(option,"(") == 0)
      {
        if (k == MaxImageStackDepth)
          ThrowMogrifyException(OptionError,"ParenthesisNestedTooDeeply",
            option);
        MogrifyImageStack(image_stack[k],MagickTrue,pend);
        k++;
        image_stack[k]=NewImageList();
        continue;
      }
    if (LocaleCompare(option,")") == 0)
      {
        if (k == 0)
          ThrowMogrifyException(OptionError,"UnableToParseExpression",option);
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

        /*
          Option is a file name: begin by reading image from specified file.
        */
        MogrifyImageStack(image_stack[k],MagickFalse,pend);
        (void) CopyMagickString(image_info->filename,argv[i],MaxTextExtent);
        image=ReadImage(image_info,exception);
        status&=(image != (Image *) NULL) &&
          (exception->severity < ErrorException);
        if (image == (Image *) NULL)
          continue;
        if (format != (char *) NULL)
          AppendImageFormat(format,image->filename);
        AppendImageToList(&image_stack[k],image);
        MogrifyImageStack(image_stack[k],MagickFalse,MagickTrue);
        if (global_colormap != MagickFalse)
          (void) MapImages(image,(Image *) NULL,image_info->dither);
        /*
          Write transmogrified image to disk.
        */
        status&=WriteImages(image_info,image_stack[k],image_stack[k]->filename,
          exception);
        image_stack[k]=DestroyImageList(image_stack[k]);
        continue;
      }
    pend=image_stack[k] != (Image *) NULL ? MagickTrue : MagickFalse;
    switch (*(option+1))
    {
      case 'a':
      {
        if (LocaleCompare("affine",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("annotate",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            i++;
            break;
          }
        if (LocaleCompare("antialias",option+1) == 0)
          break;
        if (LocaleCompare("authenticate",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'b':
      {
        if (LocaleCompare("background",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("bias",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("black-threshold",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("blue-primary",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("blur",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("border",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("bordercolor",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("box",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'c':
      {
        if (LocaleCompare("cache",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("channel",option+1) == 0)
          {
            if (*option == '-')
              {
                long
                  channel;

                i++;
                if (i == (long) (argc-1))
                  ThrowMogrifyException(OptionError,"MissingArgument",option);
                channel=ParseChannelOption(argv[i]);
                if (channel < 0)
                  ThrowMogrifyException(OptionError,"UnrecognizedChannelType",
                    argv[i]);
                break;
              }
            break;
          }
        if (LocaleCompare("charcoal",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("chop",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("colorize",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("colors",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("colorspace",option+1) == 0)
          {
            long
              colorspace;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            colorspace=ParseMagickOption(MagickColorspaceOptions,
              MagickFalse,argv[i]);
            if (colorspace < 0)
              ThrowMogrifyException(OptionError,"UnrecognizedColorspace",
                argv[i]);
            break;
          }
        if (LocaleCompare("comment",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
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
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            compression=ParseMagickOption(MagickCompressionOptions,
              MagickFalse,argv[i]);
            if (compression < 0)
              ThrowMogrifyException(OptionError,
                "UnrecognizedImageCompression",argv[i]);
            break;
          }
        if (LocaleCompare("contrast",option+1) == 0)
          break;
        if (LocaleCompare("convolve",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("crop",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("cycle",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
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
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            event_mask=SetLogEventMask(argv[i]);
            if (event_mask == UndefinedEvents)
              ThrowMogrifyException(OptionError,"UnrecognizedEventType",
                option);
            break;
          }
        if (LocaleCompare("define",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (*option == '+')
              {
                const char
                  *define;

                define=GetImageOption(image_info,argv[i]);
                if (define == (const char *) NULL)
                  ThrowMogrifyException(OptionError,"NoSuchOption",argv[i]);
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
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("density",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("depth",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("despeckle",option+1) == 0)
          break;
        if (LocaleCompare("display",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
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
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            dispose=ParseMagickOption(MagickDisposeOptions,MagickFalse,argv[i]);
            if (dispose < 0)
              ThrowMogrifyException(OptionError,"UnrecognizedDisposeMethod",
                argv[i]);
            break;
          }
        if (LocaleCompare("dither",option+1) == 0)
          break;
        if (LocaleCompare("draw",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'e':
      {
        if (LocaleCompare("edge",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("emboss",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("encoding",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
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
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            endian=ParseMagickOption(MagickEndianOptions,MagickFalse,argv[i]);
            if (endian < 0)
              ThrowMogrifyException(OptionError,"UnrecognizedEndianType",
                argv[i]);
            break;
          }
        if (LocaleCompare("enhance",option+1) == 0)
          break;
        if (LocaleCompare("equalize",option+1) == 0)
          break;
        if (LocaleCompare("evaluate",option+1) == 0)
          {
            long
              op;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            op=ParseMagickOption(MagickEvaluateOptions,MagickFalse,argv[i]);
            if (op < 0)
              ThrowMogrifyException(OptionError,
                "UnrecognizedEvaluateOperator",argv[i]);
            i++;
            if (i == (long) (argc-1))
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("extent",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("extract",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'f':
      {
        if (LocaleCompare("family",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("fill",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
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
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            filter=ParseMagickOption(MagickFilterOptions,MagickFalse,
              argv[i]);
            if (filter < 0)
              ThrowMogrifyException(OptionError,"UnrecognizedImageFilter",
                argv[i]);
            break;
          }
        if (LocaleCompare("flip",option+1) == 0)
          break;
        if (LocaleCompare("flop",option+1) == 0)
          break;
        if (LocaleCompare("floodfill",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            i++;
            break;
          }
        if (LocaleCompare("font",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("format",option+1) == 0)
          {
            (void) strcpy(argv[i]+1,"{1}");
            (void) CloneString(&format,(char *) NULL);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            (void) CloneString(&format,argv[i]);
            (void) CopyMagickString(image_info->filename,format,
              MaxTextExtent);
            (void) ConcatenateMagickString(image_info->filename,":",
              MaxTextExtent);
            (void) SetImageInfo(image_info,MagickFalse,exception);
            if (*image_info->magick == '\0')
              ThrowMogrifyException(OptionError,
                "UnrecognizedImageFormat",format);
            break;
          }
        if (LocaleCompare("frame",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("fuzz",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'g':
      {
        if (LocaleCompare("gamma",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("gaussian",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("geometry",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
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
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            gravity=ParseMagickOption(MagickGravityOptions,MagickFalse,
              argv[i]);
            if (gravity < 0)
              ThrowMogrifyException(OptionError,"UnrecognizedGravityType",
                argv[i]);
            break;
          }
        if (LocaleCompare("green-primary",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'h':
      {
        if (LocaleCompare("help",option+1) == 0)
          MogrifyUsage();
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'i':
      {
        if (LocaleCompare("implode",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("intent",option+1) == 0)
          {
            long
              intent;

            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            intent=ParseMagickOption(MagickIntentOptions,MagickFalse,
              argv[i]);
            if (intent < 0)
              ThrowMogrifyException(OptionError,"UnrecognizedIntentType",
                argv[i]);
            break;
          }
        if (LocaleCompare("interlace",option+1) == 0)
          {
            long
              interlace;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            interlace=ParseMagickOption(MagickInterlaceOptions,MagickFalse,
              argv[i]);
            if (interlace < 0)
              ThrowMogrifyException(OptionError,"UnrecognizedInterlaceType",
                argv[i]);
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'l':
      {
        if (LocaleCompare("label",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("lat",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
          }
        if (LocaleCompare("level",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("linewidth",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
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
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            resource=ParseMagickOption(MagickResourceOptions,MagickFalse,
              argv[i]);
            if (resource < 0)
              ThrowMogrifyException(OptionError,"UnrecognizedResourceType",
                argv[i]);
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if ((LocaleCompare("unlimited",argv[i]) != 0) &&
                (IsGeometry(argv[i]) == MagickFalse))
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("list",option+1) == 0)
          {
            long
              list;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            list=ParseMagickOption(MagickListOptions,MagickFalse,argv[i]);
            if (list < 0)
              ThrowMogrifyException(OptionError,"UnrecognizedListType",argv[i]);
            (void) MogrifyImageInfo(image_info,(int) (i-j+1),(const char **)
              argv+j,exception);
            return(MagickTrue);
          }
        if (LocaleCompare("log",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if ((i == (long) argc) ||
                (strchr(argv[i],'%') == (char *) NULL))
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("loop",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'm':
      {
        if (LocaleCompare("map",option+1) == 0)
          {
            global_colormap=(MagickBooleanType) (*option == '+');
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("mask",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("matte",option+1) == 0)
          break;
        if (LocaleCompare("mattecolor",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("modulate",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("median",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("monitor",option+1) == 0)
          break;
        if (LocaleCompare("monochrome",option+1) == 0)
          break;
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'n':
      {
        if (LocaleCompare("negate",option+1) == 0)
          break;
        if (LocaleCompare("noise",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (*option == '+')
              {
                long
                  noise;

                noise=ParseMagickOption(MagickNoiseOptions,MagickFalse,argv[i]);
                if (noise < 0)
                  ThrowMogrifyException(OptionError,"UnrecognizedNoiseType",
                    argv[i]);
                break;
              }
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("noop",option+1) == 0)
          break;
        if (LocaleCompare("normalize",option+1) == 0)
          break;
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'o':
      {
        if (LocaleCompare("opaque",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("ordered-dither",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("orient",option+1) == 0)
          {
            long
              orientation;

            orientation=UndefinedOrientation;
            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            orientation=ParseMagickOption(MagickOrientationOptions,
              MagickFalse,argv[i]);
            if (orientation < 0)
              ThrowMogrifyException(OptionError,
                "UnrecognizedImageOrientation",argv[i]);
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'p':
      {
        if (LocaleCompare("page",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("paint",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("pointsize",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("profile",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'q':
      {
        if (LocaleCompare("quality",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("quiet",option+1) == 0)
          break;
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'r':
      {
        if (LocaleCompare("radial-blur",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("raise",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("random-threshold",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("red-primary",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
          }
        if (LocaleCompare("region",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
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
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("resample",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("resize",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("roll",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("rotate",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 's':
      {
        if (LocaleCompare("sample",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("sampling-factor",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("scale",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("scene",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("seed",option+1) == 0)
          {
            unsigned long
              seed;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            seed=(unsigned long) atol(argv[i]);
            DistillRandomEvent((unsigned char *) &seed,sizeof(seed));
            break;
          }
        if (LocaleCompare("segment",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("separate",option+1) == 0)
          break;
        if (LocaleCompare("sepia-tone",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("set",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("shade",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("shadow",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("sharpen",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("shave",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("shear",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("sigmoidal-contrast",option+1) == 0)
          {
            i++;
            if (i == (long) (argc-1))
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("size",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("solarize",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("spread",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("stretch",option+1) == 0)
          {
            long
              stretch;

            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            stretch=ParseMagickOption(MagickStretchOptions,MagickFalse,
              argv[i]);
            if (stretch < 0)
              ThrowMogrifyException(OptionError,"UnrecognizedStyleType",
                argv[i]);
            break;
          }
        if (LocaleCompare("strip",option+1) == 0)
          break;
        if (LocaleCompare("stroke",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("strokewidth",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("style",option+1) == 0)
          {
            long
              style;

            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            style=ParseMagickOption(MagickStyleOptions,MagickFalse,argv[i]);
            if (style < 0)
              ThrowMogrifyException(OptionError,"UnrecognizedStyleType",
                argv[i]);
            break;
          }
        if (LocaleCompare("support",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("swirl",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 't':
      {
        if (LocaleCompare("texture",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("threshold",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("thumbnail",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("transparent",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("treedepth",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
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
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            type=ParseMagickOption(MagickImageOptions,MagickFalse,argv[i]);
            if (type < 0)
              ThrowMogrifyException(OptionError,"UnrecognizedImageType",
                argv[i]);
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'u':
      {
        if (LocaleCompare("undercolor",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("units",option+1) == 0)
          {
            long
              units;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            units=ParseMagickOption(MagickResolutionOptions,MagickFalse,
              argv[i]);
            if (units < 0)
              ThrowMogrifyException(OptionError,"UnrecognizedUnitsType",
                argv[i]);
            break;
          }
        if (LocaleCompare("unsharp",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'v':
      {
        if (LocaleCompare("verbose",option+1) == 0)
          {
            image_info->verbose=(*option == '-') ? MagickTrue : MagickFalse;
            break;
          }
        if (LocaleCompare("version",option+1) == 0)
          break;
        if (LocaleCompare("view",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("virtual-pixel",option+1) == 0)
          {
            long
              method;

            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            method=ParseMagickOption(MagickVirtualPixelOptions,MagickFalse,
              argv[i]);
            if (method < 0)
              ThrowMogrifyException(OptionError,
                "UnrecognizedVirtualPixelMethod",argv[i]);
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case 'w':
      {
        if (LocaleCompare("wave",option+1) == 0)
          {
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("weight",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) (argc-1))
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            break;
          }
        if (LocaleCompare("white-point",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        if (LocaleCompare("white-threshold",option+1) == 0)
          {
            if (*option == '+')
              break;
            i++;
            if (i == (long) argc)
              ThrowMogrifyException(OptionError,"MissingArgument",option);
            if (IsGeometry(argv[i]) == MagickFalse)
              ThrowMogrifyInvalidArgumentException(option,argv[i]);
            break;
          }
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
      }
      case '?':
        break;
      default:
        ThrowMogrifyException(OptionError,"UnrecognizedOption",option)
    }
    status=(MagickStatusType)
      ParseMagickOption(MagickMogrifyOptions,MagickFalse,option+1);
    if (status == MagickTrue)
      MogrifyImageStack(image_stack[k],MagickTrue,MagickTrue);
  }
  if (k != 0)
    ThrowMogrifyException(OptionError,"UnbalancedParenthesis",argv[i]);
  if (i != argc)
    ThrowMogrifyException(OptionError,"MissingAnImageFilename",argv[i]);
  DestroyMogrify();
  return(status != 0 ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
+     M o g r i f y I m a g e I n f o                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MogrifyImageInfo() applies image processing settings to the image as
%  prescribed by command line options.
%
%  The format of the MogrifyImageInfo method is:
%
%      MagickBooleanType MogrifyImageInfo(ImageInfo *image_info,const int argc,
%        const char **argv,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info..
%
%    o argc: Specifies a pointer to an integer describing the number of
%      elements in the argument vector.
%
%    o argv: Specifies a pointer to a text array containing the command line
%      arguments.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

static MagickBooleanType MonitorProgress(const char *tag,
  const MagickOffsetType quantum,const MagickSizeType span,
  void *magick_unused(client_data))
{
  char
    message[MaxTextExtent];

  const char
    *locale_message;

  if (span < 2)
    return(MagickTrue);
  (void) FormatMagickString(message,MaxTextExtent,"Monitor/%s",tag);
  locale_message=GetLocaleMessage(message);
  if (locale_message == message)
    locale_message=tag;
  (void) fprintf(stdout,"%s: %02ld%%\r",locale_message,(long)
    (100L*quantum/(span-1)));
  if ((MagickSizeType) quantum == (span-1))
    (void) fprintf(stdout,"\n");
  (void) fflush(stdout);
  return(MagickTrue);
}

MagickExport MagickBooleanType MogrifyImageInfo(ImageInfo *image_info,
  const int argc,const char **argv,ExceptionInfo *exception)
{
  const char
    *option;

  GeometryInfo
    geometry_info;

  register long
    i;

  /*
    Initialize method variables.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      image_info->filename);
  if (argc < 0)
    return(MagickTrue);
  for (i=0; i < (long) argc; i++)
    if (strlen(argv[i]) > (MaxTextExtent/2-1))
      ThrowMagickFatalException(OptionFatalError,"OptionLengthExceedsLimit",
        argv[i]);
  /*
    Set the image settings.
  */
  for (i=0; i < (long) argc; i++)
  {
    option=argv[i];
    if (IsMagickOption(option) == MagickFalse)
      continue;
    switch (*(option+1))
    {
      case 'a':
      {
        if (LocaleCompare("adjoin",option+1) == 0)
          {
            image_info->adjoin=(*option == '-') ? MagickTrue : MagickFalse;
            break;
          }
        if (LocaleCompare("antialias",option+1) == 0)
          {
            image_info->antialias=(*option == '-') ? MagickTrue : MagickFalse;
            break;
          }
        if (LocaleCompare("authenticate",option+1) == 0)
          {
            if (*option == '+')
              (void) CloneString(&image_info->authenticate,(char *) NULL);
            else
              (void) CloneString(&image_info->authenticate,argv[i+1]);
            break;
          }
        break;
      }
      case 'b':
      {
        if (LocaleCompare("background",option+1) == 0)
          {
            if (*option == '+')
              (void) QueryColorDatabase(BackgroundColor,
                &image_info->background_color,exception);
            else
              (void) QueryColorDatabase(argv[i+1],
                &image_info->background_color,exception);
            break;
          }
        if (LocaleCompare("bordercolor",option+1) == 0)
          {
            if (*option == '+')
              (void) QueryColorDatabase(BorderColor,&image_info->border_color,
                exception);
            else
              (void) QueryColorDatabase(argv[i+1],&image_info->border_color,
                exception);
            break;
          }
        break;
      }
      case 'c':
      {
        if (LocaleCompare("cache",option+1) == 0)
          {
            unsigned long
              limit;

            limit=(~0UL);
            if (LocaleCompare("unlimited",argv[i+1]) != 0)
              limit=(unsigned long) atol(argv[i+1]);
            (void) SetMagickResourceLimit(MemoryResource,limit);
            (void) SetMagickResourceLimit(MapResource,2*limit);
            break;
          }
        if (LocaleCompare("channel",option+1) == 0)
          {
            if (*option == '+')
              {
                image_info->channel=(ChannelType)
                  ((long) AllChannels &~ (long) OpacityChannel);
                break;
              }
            image_info->channel=(ChannelType) ParseChannelOption(argv[i+1]);
            break;
          }
        if (LocaleCompare("colors",option+1) == 0)
          {
            image_info->colors=(unsigned long) atol(argv[i+1]);
            break;
          }
        if (LocaleCompare("colorspace",option+1) == 0)
          {
            if (*option == '+')
              {
                image_info->colorspace=UndefinedColorspace;
                break;
              }
            image_info->colorspace=(ColorspaceType)
              ParseMagickOption(MagickColorspaceOptions,MagickFalse,argv[i+1]);
            break;
          }
        if (LocaleCompare("comment",option+1) == 0)
          {
            if (*option == '+')
              {
                (void) RemoveImageOption(image_info,"Comment");
                break;
              }
            (void) SetImageOption(image_info,"Comment",argv[i+1]);
            break;
          }
        if (LocaleCompare("compress",option+1) == 0)
          {
            if (*option == '+')
              image_info->compression=UndefinedCompression;
            else
              image_info->compression=(CompressionType) ParseMagickOption(
                MagickCompressionOptions,MagickFalse,argv[i+1]);
            break;
          }
        break;
      }
      case 'd':
      {
        if (LocaleCompare("debug",option+1) == 0)
          {
            if (*option == '+')
              (void) SetLogEventMask("none");
            else
              (void) SetLogEventMask(argv[i+1]);
            image_info->debug=IsEventLogging();
            break;
          }
        if (LocaleCompare("define",option+1) == 0)
          {
            if (*option == '+')
              {
                (void) RemoveImageOption(image_info,argv[i+1]);
                break;
              }
            (void) DefineImageOption(image_info,argv[i+1]);
            break;
          }
        if (LocaleCompare("delay",option+1) == 0)
          {
            if (*option == '+')
              {
                (void) RemoveImageOption(image_info,"delay");
                break;
              }
            (void) SetImageOption(image_info,"delay",argv[i+1]);
            break;
          }
        if (LocaleCompare("density",option+1) == 0)
          {
            /*
              Set image density.
            */
            if (*option == '+')
              (void) CloneString(&image_info->density,"");
            else
              (void) CloneString(&image_info->density,argv[i+1]);
            break;
          }
        if (LocaleCompare("depth",option+1) == 0)
          {
            if (*option == '+')
              image_info->depth=QuantumDepth;
            else
              image_info->depth=(unsigned long) atol(argv[i+1]);
            break;
          }
        if (LocaleCompare("display",option+1) == 0)
          {
            if (*option == '+')
              (void) CloneString(&image_info->server_name,"");
            else
              (void) CloneString(&image_info->server_name,argv[i+1]);
            break;
          }
        if (LocaleCompare("dispose",option+1) == 0)
          {
            if (*option == '+')
              {
                (void) RemoveImageOption(image_info,"dispose");
                break;
              }
            (void) SetImageOption(image_info,"dispose",argv[i+1]);
            break;
          }
        if (LocaleCompare("dither",option+1) == 0)
          {
            image_info->dither=(*option == '-') ? MagickTrue : MagickFalse;
            break;
          }
        break;
      }
      case 'e':
      {
        if (LocaleCompare("endian",option+1) == 0)
          {
            if (*option == '+')
              {
                image_info->endian=LSBEndian;
                break;
              }
            image_info->endian=(EndianType)
              ParseMagickOption(MagickEndianOptions,MagickFalse,argv[i+1]);
            break;
          }
        if (LocaleCompare("extract",option+1) == 0)
          {
            /*
              Set image extract geometry.
            */
            if (*option == '+')
              (void) CloneString(&image_info->extract,"");
            else
              (void) CloneString(&image_info->extract,argv[i+1]);
            break;
          }
        break;
      }
      case 'f':
      {
        if (LocaleCompare("fill",option+1) == 0)
          {
            if (*option == '+')
              (void) QueryColorDatabase("none",&image_info->pen,exception);
            else
              (void) QueryColorDatabase(argv[i+1],&image_info->pen,exception);
            break;
          }
        if (LocaleCompare("font",option+1) == 0)
          {
            if (*option == '+')
              (void) CloneString(&image_info->font,"");
            else
              (void) CloneString(&image_info->font,argv[i+1]);
            break;
          }
        if (LocaleCompare("format",option+1) == 0)
          {
            register char
              *q;

            for (q=strchr(argv[i+1],'%'); q != (char *) NULL; q=strchr(q+1,'%'))
              if (strchr("gkrz@#",*(q+1)) != (char *) NULL)
                image_info->ping=MagickFalse;
            (void) SetImageOption(image_info,"format",argv[i+1]);
            break;
          }
        if (LocaleCompare("fuzz",option+1) == 0)
          {
            if (*option == '+')
              image_info->fuzz=0.0;
            else
              image_info->fuzz=StringToDouble(argv[i+1],QuantumRange);
            break;
          }
        break;
      }
      case 'g':
      {
        if (LocaleCompare("gravity",option+1) == 0)
          {
            if (*option == '+')
              {
                (void) RemoveImageOption(image_info,"gravity");
                break;
              }
            (void) SetImageOption(image_info,"gravity",argv[i+1]);
            break;
          }
        break;
      }
      case 'i':
      {
        if (LocaleCompare("interlace",option+1) == 0)
          {
            if (*option == '+')
              image_info->interlace=UndefinedInterlace;
            else
              image_info->interlace=(InterlaceType)
                ParseMagickOption(MagickInterlaceOptions,MagickFalse,argv[i+1]);
            break;
          }
        break;
      }
      case 'l':
      {
        if (LocaleCompare("label",option+1) == 0)
          {
            if (*option == '+')
              {
                (void) RemoveImageOption(image_info,"Label");
                break;
              }
            (void) SetImageOption(image_info,"Label",argv[i+1]);
            break;
          }
        if (LocaleCompare("limit",option+1) == 0)
          {
            ResourceType
              type;

            unsigned long
              limit;

            if (*option == '+')
              break;
            type=(ResourceType) ParseMagickOption(MagickResourceOptions,
              MagickFalse,argv[i+1]);
            limit=(~0UL);
            if (LocaleCompare("unlimited",argv[i+2]) != 0)
              limit=(unsigned long) atol(argv[i+2]);
            (void) SetMagickResourceLimit(type,limit);
            break;
          }
        if (LocaleCompare("list",option+1) == 0)
          {
            /*
              Display configuration list.
            */
            switch (*argv[i+1])
            {
              case 'C':
              case 'c':
              {
                if (LocaleCompare("Coder",argv[i+1]) == 0)
                  {
                    (void) ListCoderInfo((FILE *) NULL,exception);
                    break;
                  }
                if (LocaleCompare("Color",argv[i+1]) == 0)
                  {
                    (void) ListColorInfo((FILE *) NULL,exception);
                    break;
                  }
                if (LocaleCompare("Configure",argv[i+1]) == 0)
                  {
                    (void) ListConfigureInfo((FILE *) NULL,exception);
                    break;
                  }
                break;
              }
              case 'D':
              case 'd':
              {
                if (LocaleCompare("Delegate",argv[i+1]) == 0)
                  {
                    (void) ListDelegateInfo((FILE *) NULL,exception);
                    break;
                  }
                break;
              }
              case 'F':
              case 'f':
              {
                if (LocaleCompare("Format",argv[i+1]) == 0)
                  {
                    (void) ListMagickInfo((FILE *) NULL,exception);
                    break;
                  }
                break;
              }
              case 'L':
              case 'l':
              {
                if (LocaleCompare("Locale",argv[i+1]) == 0)
                  {
                    (void) ListLocaleInfo((FILE *) NULL,exception);
                    break;
                  }
                if (LocaleCompare("Log",argv[i+1]) == 0)
                  {
                    (void) ListLogInfo((FILE *) NULL,exception);
                    break;
                  }
                break;
              }
              case 'M':
              case 'm':
              {
                if (LocaleCompare("Magic",argv[i+1]) == 0)
                  {
                    (void) ListMagicInfo((FILE *) NULL,exception);
                    break;
                  }
#if defined(SupportMagickModules)
                if (LocaleCompare("Module",argv[i+1]) == 0)
                  {
                    (void) ListModuleInfo((FILE *) NULL,exception);
                    break;
                  }
                break;
#endif
              }
              case 'R':
              case 'r':
              {
                if (LocaleCompare("Resource",argv[i+1]) == 0)
                  {
                    (void) ListMagickResourceInfo((FILE *) NULL,exception);
                    break;
                  }
                break;
              }
              case 'T':
              case 't':
              {
                if (LocaleCompare("Type",argv[i+1]) == 0)
                  {
                    (void) ListTypeInfo((FILE *) NULL,exception);
                    break;
                  }
                break;
              }
              default:
                break;
            }
            break;
          }
        if (LocaleCompare("log",option+1) == 0)
          {
            if (*option == '+')
              break;
            (void) SetLogFormat(argv[i+1]);
            break;
          }
        break;
      }
      case 'm':
      {
        if (LocaleCompare("mattecolor",option+1) == 0)
          {
            if (*option == '+')
              (void) QueryColorDatabase(MatteColor,&image_info->matte_color,
                exception);
            else
              (void) QueryColorDatabase(argv[i+1],&image_info->matte_color,
                exception);
            break;
          }
        if (LocaleCompare("monitor",option+1) == 0)
          {
            (void) SetImageInfoProgressMonitor(image_info,MonitorProgress,
              (void *) NULL);
            break;
          }
        if (LocaleCompare("monochrome",option+1) == 0)
          {
            image_info->monochrome=(*option == '-') ? MagickTrue : MagickFalse;
            break;
          }
        break;
      }
      case 'o':
      {
        if (LocaleCompare("orient",option+1) == 0)
          {
            if (*option == '+')
              image_info->orientation=UndefinedOrientation;
            else
              image_info->orientation=(OrientationType) ParseMagickOption(
                MagickOrientationOptions,MagickFalse,argv[i+1]);
            break;
          }
        break;
      }
      case 'p':
      {
        if (LocaleCompare("page",option+1) == 0)
          {
            if (*option == '+')
              {
                (void) RemoveImageOption(image_info,"page");
                (void) CloneString(&image_info->page,(char *) NULL);
                break;
              }
            (void) SetImageOption(image_info,"page",argv[i+1]);
            image_info->page=GetPageGeometry(argv[i+1]);
            break;
          }
        if (LocaleCompare("pen",option+1) == 0)
          {
            if (*option == '+')
              (void) QueryColorDatabase("none",&image_info->pen,exception);
            else
              (void) QueryColorDatabase(argv[i+1],&image_info->pen,exception);
            break;
          }
        if (LocaleCompare("ping",option+1) == 0)
          {
            image_info->ping=(*option == '-') ? MagickTrue : MagickFalse;
            break;
          }
        if (LocaleCompare("pointsize",option+1) == 0)
          {
            if (*option == '+')
              (void) ParseGeometry("12",&geometry_info);
            else
              (void) ParseGeometry(argv[i+1],&geometry_info);
            image_info->pointsize=geometry_info.rho;
            break;
          }
        if (LocaleCompare("preview",option+1) == 0)
          {
            /*
              Preview image.
            */
            if (*option == '+')
              image_info->preview_type=UndefinedPreview;
            else
              image_info->preview_type=(PreviewType)
                ParseMagickOption(MagickPreviewOptions,MagickFalse,argv[i+1]);
            break;
          }
        break;
      }
      case 'q':
      {
        if (LocaleCompare("quality",option+1) == 0)
          {
            /*
              Set image compression quality.
            */
            if (*option == '+')
              image_info->quality=UndefinedCompressionQuality;
            else
              image_info->quality=(unsigned long) atol(argv[i+1]);
            break;
          }
        break;
      }
      case 's':
      {
        if (LocaleCompare("sampling-factor",option+1) == 0)
          {
            /*
              Set image sampling factor.
            */
            if (*option == '+')
              (void) CloneString(&image_info->sampling_factor,"");
            else
              (void) CloneString(&image_info->sampling_factor,argv[i+1]);
            break;
          }
        if (LocaleCompare("size",option+1) == 0)
          {
            if (*option == '+')
              (void) CloneString(&image_info->size,"");
            else
              (void) CloneString(&image_info->size,argv[i+1]);
            break;
          }
        break;
      }
      case 't':
      {
        if (LocaleCompare("texture",option+1) == 0)
          {
            if (*option == '+')
              (void) CloneString(&image_info->texture,"");
            else
              (void) CloneString(&image_info->texture,argv[i+1]);
            break;
          }
        if (LocaleCompare("type",option+1) == 0)
          {
            if (*option == '+')
              {
                image_info->type=UndefinedType;
                break;
              }
            image_info->type=(ImageType)
              ParseMagickOption(MagickImageOptions,MagickFalse,argv[i+1]);
            break;
          }
        break;
      }
      case 'u':
      {
        if (LocaleCompare("units",option+1) == 0)
          {
            if (*option == '+')
              image_info->units=UndefinedResolution;
            else
              image_info->units=(ResolutionType) ParseMagickOption(
                MagickResolutionOptions,MagickFalse,argv[i+1]);
            break;
          }
        break;
      }
      case 'v':
      {
        if (LocaleCompare("verbose",option+1) == 0)
          {
            image_info->verbose=(*option == '-') ? MagickTrue : MagickFalse;
            break;
          }
        if (LocaleCompare("view",option+1) == 0)
          {
            if (*option == '+')
              (void) CloneString(&image_info->view,"");
            else
              (void) CloneString(&image_info->view,argv[i+1]);
            break;
          }
        break;
      }
      default:
        break;
    }
    i+=Max(ParseMagickOption(MagickCommandOptions,MagickFalse,option),0);
  }
  return(exception->severity == UndefinedException ?  MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+     M o g r i f y I m a g e L i s t                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MogrifyImageList() applies any command line options that might affect the
%  entire image list (e.g. -append, -coalesce, etc.).
%
%  The format of the MogrifyImage method is:
%
%      MagickBooleanType MogrifyImageList(ImageInfo *image_info,const int argc,
%        const char **argv,Image **images,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info..
%
%    o argc: Specifies a pointer to an integer describing the number of
%      elements in the argument vector.
%
%    o argv: Specifies a pointer to a text array containing the command line
%      arguments.
%
%    o images: The images.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
MagickExport MagickBooleanType MogrifyImageList(ImageInfo *image_info,
  const int argc,const char **argv,Image **images,ExceptionInfo *exception)
{
  const char
    *option;

  long
    count,
    index;

  MagickStatusType
    status;

  register long
    i;

  /*
    Apply options to the image list.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(images != (Image **) NULL);
  assert((*images)->signature == MagickSignature);
  if ((*images)->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      (*images)->filename);
  if ((argc <= 0) || (*argv == (char *) NULL))
    return(MagickTrue);
  status=MagickTrue;
  for (i=0; i < (long) argc; i++)
  {
    option=argv[i];
    if (IsMagickOption(option) == MagickFalse)
      continue;
    count=Max(ParseMagickOption(MagickCommandOptions,MagickFalse,option),0);
    if ((i+count) >= argc)
      break;
    (void) MogrifyImageInfo(image_info,(int) count+1,argv+i,exception);
    switch (*(option+1))
    {
      case 'a':
      {
        if (LocaleCompare("append",option+1) == 0)
          {
            Image
              *append_image;

            (*images)->background_color=image_info->background_color;
            append_image=AppendImages(*images,
              *option == '-' ? MagickTrue : MagickFalse,exception);
            if (append_image == (Image *) NULL)
              break;
            InheritException(&append_image->exception,exception);
            *images=DestroyImageList(*images);
            *images=append_image;
            break;
          }
        if (LocaleCompare("average",option+1) == 0)
          {
            Image
              *average_image;

            average_image=AverageImages(*images,exception);
            if (average_image == (Image *) NULL)
              break;
            InheritException(&average_image->exception,exception);
            *images=DestroyImageList(*images);
            *images=average_image;
            break;
          }
        break;
      }
      case 'c':
      {
        if (LocaleCompare("coalesce",option+1) == 0)
          {
            Image
              *coalesce_image;

            coalesce_image=CoalesceImages(*images,exception);
            if (coalesce_image == (Image *) NULL)
              break;
            InheritException(&coalesce_image->exception,exception);
            *images=DestroyImageList(*images);
            *images=coalesce_image;
            break;
          }
        if (LocaleCompare("combine",option+1) == 0)
          {
            Image
              *combine_image;

            combine_image=CombineImages(*images,image_info->channel,exception);
            if (combine_image == (Image *) NULL)
              break;
            InheritException(&combine_image->exception,exception);
            *images=DestroyImageList(*images);
            *images=combine_image;
            break;
          }
        if (LocaleCompare("composite",option+1) == 0)
          {
            char
              composite_geometry[MaxTextExtent];

            Image
              *composite_image,
              *image,
              *mask_image;

            RectangleInfo
              geometry;

            image=(*images);
            composite_image=GetNextImageInList(*images);
            if (composite_image == (Image *) NULL)
              break;
            mask_image=GetNextImageInList(composite_image);
            image=CloneImage(image,0,0,MagickTrue,exception);
            if (image == (Image *) NULL)
              break;
            SetGeometry(composite_image,&geometry);
            (void) ParseAbsoluteGeometry(image->geometry,&geometry);
            (void) FormatMagickString(composite_geometry,MaxTextExtent,
              "%lux%lu%+ld%+ld",composite_image->columns,composite_image->rows,
              geometry.x,geometry.y);
            (void) ParseGravityGeometry(image,composite_geometry,&geometry);
            if (mask_image != (Image *) NULL)
              {
                (void) SetImageType(composite_image,TrueColorMatteType);
                if (composite_image->matte == MagickFalse)
                  SetImageOpacity(composite_image,OpaqueOpacity);
                status&=CompositeImage(composite_image,CopyOpacityCompositeOp,
                  mask_image,0,0);
              }
            (void) CompositeImage(image,image->compose,composite_image,
              geometry.x,geometry.y);
            InheritException(&image->exception,exception);
            *images=DestroyImageList(*images);
            *images=image;
            break;
          }
        if (LocaleCompare("crop",option+1) == 0)
          {
            (void) TransformImages(images,argv[i+1],(char *) NULL);
            break;
          }
        break;
      }
      case 'd':
      {
        if (LocaleCompare("deconstruct",option+1) == 0)
          {
            Image
              *deconstruct_image;

            deconstruct_image=DeconstructImages(*images,exception);
            if (deconstruct_image == (Image *) NULL)
              break;
            InheritException(&deconstruct_image->exception,exception);
            *images=DestroyImageList(*images);
            *images=deconstruct_image;
            break;
          }
        if (LocaleCompare("delete",option+1) == 0)
          {
            Image
              *delete_image;

            index=(-1);
            if (*option != '+')
              index=atol(argv[i+1]);
            delete_image=GetImageFromList(*images,index);
            if (delete_image == (Image *) NULL)
              {
                (void) ThrowMagickException(exception,GetMagickModule(),
                  OptionError,"NoSuchImage","`%s'",argv[i+1]);
                break;
              }
            DeleteImageFromList(&delete_image);
            *images=GetFirstImageInList(delete_image);
            break;
          }
        break;
      }
      case 'f':
      {
        if (LocaleCompare("flatten",option+1) == 0)
          {
            Image
              *flatten_image;

            flatten_image=FlattenImages(*images,exception);
            if (flatten_image == (Image *) NULL)
              break;
            InheritException(&flatten_image->exception,exception);
            *images=DestroyImageList(*images);
            *images=flatten_image;
            break;
          }
        if (LocaleCompare("fx",option+1) == 0)
          {
            Image
              *fx_image;

            fx_image=FxImageChannel(*images,image_info->channel,argv[i+1],
              exception);
            if (fx_image == (Image *) NULL)
              break;
            InheritException(&fx_image->exception,exception);
            *images=DestroyImageList(*images);
            *images=fx_image;
            break;
          }
        break;
      }
      case 'i':
      {
        if (LocaleCompare("insert",option+1) == 0)
          {
            Image
              *p,
              *q;

            index=0;
            if (*option != '+')
              index=atol(argv[i+1]);
            p=RemoveLastImageFromList(images);
            if (p == (Image *) NULL)
              {
                (void) ThrowMagickException(exception,GetMagickModule(),
                  OptionError,"NoSuchImage","`%s'",argv[i+1]);
                break;
              }
            q=p;
            if (index == 0)
              PrependImageToList(images,q);
            else
              {
                 q=GetImageFromList(*images,index-1);
                 if (q == (Image *) NULL)
                   {
                     (void) ThrowMagickException(exception,GetMagickModule(),
                       OptionError,"NoSuchImage","`%s'",argv[i+1]);
                     break;
                   }
                InsertImageInList(&q,p);
              }
            *images=GetFirstImageInList(q);
            break;
          }
        break;
      }
      case 'm':
      {
        if (LocaleCompare("map",option+1) == 0)
          {
             if (*option == '+')
               {
                 (void) MapImages(*images,(Image *) NULL,image_info->dither);
                 break;
               }
             i++;
             break;
          }
        if (LocaleCompare("morph",option+1) == 0)
          {
            Image
              *morph_image;

            morph_image=MorphImages(*images,(unsigned long) atol(argv[i+1]),
              exception);
            if (morph_image == (Image *) NULL)
              break;
            InheritException(&morph_image->exception,exception);
            *images=DestroyImageList(*images);
            *images=morph_image;
            break;
          }
        if (LocaleCompare("mosaic",option+1) == 0)
          {
            Image
              *mosaic_image;

            mosaic_image=MosaicImages(*images,exception);
            if (mosaic_image == (Image *) NULL)
              break;
            InheritException(&mosaic_image->exception,exception);
            *images=DestroyImageList(*images);
            *images=mosaic_image;
            break;
          }
        break;
      }
      case 'p':
      {
        if (LocaleCompare("process",option+1) == 0)
          {
            char
              *arguments,
              breaker,
              quote,
              *token;

            int
              next,
              status;

            size_t
              length;

            TokenInfo
              token_info;

            length=strlen(argv[i+1]);
            token=(char *) AcquireMagickMemory(length+MaxTextExtent);
            if (token == (char *) NULL)
              break;
            next=0;
            arguments=(char *) argv[i+1];
            status=Tokenizer(&token_info,0,token,length,arguments,"","=","\"",
              '\0',&breaker,&next,&quote);
            if (status == 0)
              {
                char
                  *argv;

                argv=(char *) &(arguments[next]);
                (void) ExecuteModuleProcess(token,&(*images),1,&argv);
              }
            token=(char *) RelinquishMagickMemory(token);
            break;
          }
        break;
      }
      case 's':
      {
        if (LocaleCompare("scene",option+1) == 0)
          {
            Image
              *p;

            long
              scene;

            p=GetFirstImageInList(*images);
            for (scene=atol(argv[i+1]); p != (Image *) NULL; scene++)
            {
              p->scene=(unsigned long) scene;
              p=GetNextImageInList(p);
            }
            break;
          }
        if (LocaleCompare("swap",option+1) == 0)
          {
            Image
              *p,
              *q,
              *swap;

            long
              swap_index;

            index=(-1);
            swap_index=(-2);
            if (*option != '+')
              {
                GeometryInfo
                  geometry_info;

                MagickStatusType
                  flags;

                flags=ParseGeometry(argv[i+1],&geometry_info);
                index=(long) geometry_info.rho;
                if ((flags & SigmaValue) != 0)
                  swap_index=(long) geometry_info.sigma;
              }
            p=GetImageFromList(*images,index);
            q=GetImageFromList(*images,swap_index);
            if ((p == (Image *) NULL) || (q == (Image *) NULL))
              {
                (void) ThrowMagickException(exception,GetMagickModule(),
                  OptionError,"NoSuchImage","`%s'",(*images)->filename);
                break;
              }
            swap=CloneImage(p,0,0,MagickTrue,exception);
            ReplaceImageInList(&p,CloneImage(q,0,0,MagickTrue,exception));
            ReplaceImageInList(&q,swap);
            *images=GetFirstImageInList(q);
            break;
          }
        break;
      }
      case 'w':
      {
        if (LocaleCompare("write",option+1) == 0)
          {
            Image
              *write_images;

            ImageInfo
              *write_info;

            write_images=(*images);
            if (*option == '+')
              write_images=CloneImageList(*images,exception);
            write_info=CloneImageInfo(image_info);
            status&=WriteImages(write_info,write_images,argv[i+1],exception);
            write_info=DestroyImageInfo(write_info);
            if (*option == '+')
              write_images=DestroyImageList(write_images);
            break;
          }
        break;
      }
      default:
        break;
    }
    i+=count;
  }
  return(status != 0 ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
+     M o g r i f y I m a g e s                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MogrifyImages() applies image processing options to a sequence of images as
%  prescribed by command line options.
%
%  The format of the MogrifyImage method is:
%
%      MagickBooleanType MogrifyImages(ImageInfo *image_info,const int argc,
%        const char **argv,Image **images,Exceptioninfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info..
%
%    o argc: Specifies a pointer to an integer describing the number of
%      elements in the argument vector.
%
%    o argv: Specifies a pointer to a text array containing the command line
%      arguments.
%
%    o images: The images.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType MogrifyImages(ImageInfo *image_info,
  const int argc,const char **argv,Image **images,ExceptionInfo *exception)
{
#define MogrifyImageTag  "Mogrify/Image"

  const char
    *option;

  Image
    *image,
    *mogrify_images;

  long
    count;

  MagickStatusType
    status;

  register long
    i;

  unsigned long
    number_images,
    scene;

  /*
    Apply options to individual images in the list.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (images == (Image **) NULL)
    return(MogrifyImage(image_info,argc,argv,images,exception));
  assert((*images)->signature == MagickSignature);
  if ((*images)->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",
      (*images)->filename);
  if ((argc <= 0) || (*argv == (char *) NULL))
    return(MagickTrue);
  scene=MagickFalse;
  for (i=0; i < (long) argc; i++)
  {
    option=argv[i];
    if (IsMagickOption(option) == MagickFalse)
      continue;
    count=Max(ParseMagickOption(MagickCommandOptions,MagickFalse,option),0);
    if ((i+count) >= argc)
      break;
    switch (*(option+1))
    {
      case 's':
      {
        if (LocaleCompare("scene",option+1) == 0)
          scene=MagickTrue;
        break;
      }
      default:
        break;
    }
    i+=count;
  }
  (void) SetImageInfoProgressMonitor(image_info,(MagickProgressMonitor) NULL,
    (void *) NULL);
  status=MagickTrue;
  mogrify_images=NewImageList();
  number_images=GetImageListLength(*images);
  for (i=0; i < (long) number_images; i++)
  {
    image=RemoveFirstImageFromList(images);
    status&=MogrifyImage(image_info,argc,argv,&image,exception);
    if (scene != MagickFalse)
      image->scene=(unsigned long) i;
    if (image_info->verbose != MagickFalse)
      (void) IdentifyImage(image,stdout,MagickFalse);
    AppendImageToList(&mogrify_images,image);
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(i,number_images) != MagickFalse))
      {
        status=image->progress_monitor(MogrifyImageTag,i,number_images,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  status&=MogrifyImageList(image_info,argc,argv,&mogrify_images,exception);
  *images=mogrify_images;
  return(status != 0 ? MagickTrue : MagickFalse);
}
