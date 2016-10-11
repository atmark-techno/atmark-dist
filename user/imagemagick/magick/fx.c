/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                                 FFFFF  X   X                                %
%                                 F       X X                                 %
%                                 FFF      X                                  %
%                                 F       X X                                 %
%                                 F      X   X                                %
%                                                                             %
%                                                                             %
%                  ImageMagick Image Special Effects Methods                  %
%                                                                             %
%                               Software Design                               %
%                                 John Cristy                                 %
%                                 October 1996                                %
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
#include "magick/cache.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/decorate.h"
#include "magick/draw.h"
#include "magick/effect.h"
#include "magick/enhance.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/fx.h"
#include "magick/gem.h"
#include "magick/geometry.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/pixel-private.h"
#include "magick/random_.h"
#include "magick/resize.h"
#include "magick/splay-tree.h"
#include "magick/string_.h"
#include "magick/utility.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     C h a r c o a l I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  CharcoalImage() creates a new image that is a copy of an existing one with
%  the edge highlighted.  It allocates the memory necessary for the new Image
%  structure and returns a pointer to the new image.
%
%  The format of the CharcoalImage method is:
%
%      Image *CharcoalImage(const Image *image,const double radius,
%        const double sigma,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o radius: the radius of the pixel neighborhood.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *CharcoalImage(const Image *image,const double radius,
  const double sigma,ExceptionInfo *exception)
{
  Image
    *charcoal_image,
    *clone_image,
    *edge_image;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  clone_image=CloneImage(image,0,0,MagickTrue,exception);
  if (clone_image == (Image *) NULL)
    return((Image *) NULL);
  (void) SetImageType(clone_image,GrayscaleType);
  edge_image=EdgeImage(clone_image,radius,exception);
  if (edge_image == (Image *) NULL)
    return((Image *) NULL);
  clone_image=DestroyImage(clone_image);
  charcoal_image=BlurImage(edge_image,radius,sigma,exception);
  if (charcoal_image == (Image *) NULL)
    return((Image *) NULL);
  edge_image=DestroyImage(edge_image);
  (void) NormalizeImage(charcoal_image);
  (void) NegateImage(charcoal_image,MagickFalse);
  (void) SetImageType(charcoal_image,GrayscaleType);
  return(charcoal_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     C o l o r i z e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ColorizeImage() blends the fill color with each pixel in the image.
%  A percentage blend is specified with opacity.  Control the application
%  of different color components by specifying a different percentage for
%  each component (e.g. 90/100/10 is 90% red, 100% green, and 10% blue).
%
%  The format of the ColorizeImage method is:
%
%      Image *ColorizeImage(const Image *image,const char *opacity,
%        const PixelPacket colorize,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o opacity:  A character string indicating the level of opacity as a
%      percentage.
%
%    o colorize: A color value.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *ColorizeImage(const Image *image,const char *opacity,
  const PixelPacket colorize,ExceptionInfo *exception)
{
#define ColorizeImageTag  "Colorize/Image"

  GeometryInfo
    geometry_info;

  Image
    *colorize_image;

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MagickStatusType
    flags;

  register const PixelPacket
    *p;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Allocate colorized image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  colorize_image=CloneImage(image,0,0,MagickTrue,exception);
  if (colorize_image == (Image *) NULL)
    return((Image *) NULL);
  colorize_image->storage_class=DirectClass;
  if (opacity == (const char *) NULL)
    return(colorize_image);
  /*
    Determine RGB values of the pen color.
  */
  flags=ParseGeometry(opacity,&geometry_info);
  pixel.red=geometry_info.rho;
  if ((flags & SigmaValue) != 0)
    pixel.green=geometry_info.sigma;
  else
    pixel.green=pixel.red;
  if ((flags & XiValue) != 0)
    pixel.blue=geometry_info.xi;
  else
    pixel.blue=pixel.red;
  if ((flags & PsiValue) != 0)
    pixel.opacity=geometry_info.psi;
  else
    pixel.opacity=(MagickRealType) OpaqueOpacity;
  /*
    Colorize DirectClass image.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    q=GetImagePixels(colorize_image,0,y,colorize_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      q->red=(Quantum) ((p->red*(100.0-pixel.red)+
        colorize.red*pixel.red)/100.0);
      q->green=(Quantum) ((p->green*(100.0-pixel.green)+
        colorize.green*pixel.green)/100.0);
      q->blue=(Quantum) ((p->blue*(100.0-pixel.blue)+
        colorize.blue*pixel.blue)/100.0);
      if (image->matte != MagickFalse)
        q->opacity=(Quantum) ((p->opacity*(100.0-pixel.opacity)+
          colorize.opacity*pixel.opacity)/100.0);
      p++;
      q++;
    }
    if (SyncImagePixels(colorize_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(ColorizeImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(colorize_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     C o n v o l v e I m a g e C h a n n e l                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ConvolveImageChannel() applies a custom convolution kernel to the image.
%
%  The format of the ConvolveImageChannel method is:
%
%      Image *ConvolveImageChannel(const Image *image,const ChannelType channel,
%        const unsigned long order,const double *kernel,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel type.
%
%    o order: The number of columns and rows in the filter kernel.
%
%    o kernel: An array of double representing the convolution kernel.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

MagickExport Image *ConvolveImage(const Image *image,const unsigned long order,
  const double *kernel,ExceptionInfo *exception)
{
  Image
    *convolve_image;

  convolve_image=ConvolveImageChannel(image,(ChannelType) ((long) AllChannels &~
    (long) OpacityChannel),order,kernel,exception);
  return(convolve_image);
}

MagickExport Image *ConvolveImageChannel(const Image *image,
  const ChannelType channel,const unsigned long order,const double *kernel,
  ExceptionInfo *exception)
{
#define ConvolveImageTag  "Convolve/Image"

  double
    *normal_kernel;

  Image
    *convolve_image;

  IndexPacket
    *convolve_indexes,
    *indexes;

  long
    u,
    v,
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MagickRealType
    alpha,
    gamma,
    normalize;

  register const double
    *k;

  register const PixelPacket
    *p,
    *pixels;

  register long
    i,
    x;

  register PixelPacket
    *q;

  unsigned long
    width;

  /*
    Initialize convolve image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  width=order;
  if ((width % 2) == 0)
    ThrowImageException(OptionError,"KernelWidthMustBeAnOddNumber");
  convolve_image=CloneImage(image,0,0,MagickTrue,exception);
  if (convolve_image == (Image *) NULL)
    return((Image *) NULL);
  convolve_image->storage_class=DirectClass;
  /*
    Convolve image.
  */
  normal_kernel=(double *)
    AcquireMagickMemory((size_t) (width*width)*sizeof(*normal_kernel));
  if (normal_kernel == (double *) NULL)
    {
      convolve_image=DestroyImage(convolve_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  normalize=0.0;
  for (i=0; i < (long) (width*width); i++)
    normalize+=kernel[i];
  if (AbsoluteValue(normalize) <= MagickEpsilon)
    normalize=1.0;
  normalize=1.0/normalize;
  for (i=0; i < (long) (width*width); i++)
    normal_kernel[i]=(double) (normalize*kernel[i]);
  if (image->debug != MagickFalse)
    {
      char
        format[MaxTextExtent],
        *message;

      (void) LogMagickEvent(TransformEvent,GetMagickModule(),
        "  ConvolveImage with %ldx%ld kernel:",width,width);
      message=AcquireString("");
      k=normal_kernel;
      for (v=0; v < (long) width; v++)
      {
        *message='\0';
        (void) FormatMagickString(format,MaxTextExtent,"%ld: ",v);
        (void) ConcatenateString(&message,format);
        for (u=0; u < (long) width; u++)
        {
          (void) FormatMagickString(format,MaxTextExtent,"%g ",*k++);
          (void) ConcatenateString(&message,format);
        }
        (void) LogMagickEvent(TransformEvent,GetMagickModule(),"%s",message);
      }
    }
  for (y=0; y < (long) convolve_image->rows; y++)
  {
    p=AcquireImagePixels(image,-((long) width/2L),y-(long) (width/2L),
      image->columns+width,width,exception);
    q=GetImagePixels(convolve_image,0,y,convolve_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    indexes=GetIndexes(image);
    convolve_indexes=GetIndexes(convolve_image);
    for (x=0; x < (long) convolve_image->columns; x++)
    {
      GetMagickPixelPacket(image,&pixel);
      gamma=0.0;
      pixels=p;
      k=normal_kernel;
      for (v=0; v < (long) width; v++)
      {
        for (u=0; u < (long) width; u++)
        {
          alpha=1.0;
          if (((channel & OpacityChannel) != 0) &&
              (image->matte != MagickFalse))
            {
              alpha=((MagickRealType) QuantumRange-pixels[u].opacity)/
                QuantumRange;
              pixel.opacity+=(*k)*pixels[u].opacity;
            }
          if ((channel & RedChannel) != 0)
            pixel.red+=(*k)*alpha*pixels[u].red;
          if ((channel & GreenChannel) != 0)
            pixel.green+=(*k)*alpha*pixels[u].green;
          if ((channel & BlueChannel) != 0)
            pixel.blue+=(*k)*alpha*pixels[u].blue;
          if (((channel & IndexChannel) != 0) &&
              (image->colorspace == CMYKColorspace))
            pixel.index+=(*k)*alpha*indexes[x+(pixels-p)+u];
          gamma+=(*k)*alpha;
          k++;
        }
        pixels+=image->columns+width;
      }
      gamma=1.0/(fabs((double) gamma) <= MagickEpsilon ? 1.0 : gamma);
      if ((channel & RedChannel) != 0)
        q->red=RoundToQuantum(gamma*pixel.red+image->bias);
      if ((channel & GreenChannel) != 0)
        q->green=RoundToQuantum(gamma*pixel.green+image->bias);
      if ((channel & BlueChannel) != 0)
        q->blue=RoundToQuantum(gamma*pixel.blue+image->bias);
      if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
        q->opacity=RoundToQuantum(pixel.opacity+image->bias);
      if (((channel & IndexChannel) != 0) &&
          (image->colorspace == CMYKColorspace))
        convolve_indexes[x]=RoundToQuantum(gamma*pixel.index+image->bias);
      p++;
      q++;
    }
    if (SyncImagePixels(convolve_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(ConvolveImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  normal_kernel=(double *) RelinquishMagickMemory(normal_kernel);
  return(convolve_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     E v a l u a t e I m a g e C h a n n e l                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  EvaluateImageChannel() applies a value to the image with an arithmetic,
%  relational, or logical operator to an image. Use these operations to
%  lighten or darken an image, to increase or decrease contrast in an image, or
%  to produce the "negative" of an image.
%
%  The format of the EvaluateImageChannel method is:
%
%      MagickBooleanType EvaluateImageChannel(Image *image,
%        const ChannelType channel,const MagickEvaluateOperator op,
%        const double value,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel.
%
%    o op: A channel op.
%
%    o value: A value value.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

static Quantum ApplyEvaluateOperator(const Quantum pixel,
  const MagickEvaluateOperator op,const MagickRealType value)
{
  MagickRealType
    result;

  result=0.0;
  switch(op)
  {
    case UndefinedEvaluateOperator:
      break;
    case AddEvaluateOperator:
      result=(MagickRealType) (pixel+value); break;
    case AndEvaluateOperator:
      result=(MagickRealType) (pixel & (unsigned long) (value+0.5)); break;
    case DivideEvaluateOperator:
      result=(MagickRealType) (pixel/(value == 0.0 ? 1.0 : value)); break;
    case LeftShiftEvaluateOperator:
      result=(MagickRealType) (pixel << (unsigned long) (value+0.5)); break;
    case MaxEvaluateOperator:
      result=(MagickRealType) Max((MagickRealType) pixel,value); break;
    case MinEvaluateOperator:
      result=(MagickRealType) Min((MagickRealType) pixel,value); break;
    case MultiplyEvaluateOperator:
      result=(MagickRealType) (pixel*value); break;
    case OrEvaluateOperator:
      result=(MagickRealType) (pixel | (unsigned long) (value+0.5)); break;
    case RightShiftEvaluateOperator:
      result=(MagickRealType) (pixel >> (unsigned long) (value+0.5)); break;
    case SetEvaluateOperator:
      result=(MagickRealType) (value); break;
    case SubtractEvaluateOperator:
      result=(MagickRealType) (pixel-value); break;
    case XorEvaluateOperator:
      result=(MagickRealType) (pixel ^ (unsigned long) (value+0.5)); break;
  }
  return(RoundToQuantum(result));
}

MagickExport MagickBooleanType EvaluateImage(Image *image,
  const MagickEvaluateOperator op,const double value,ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  status=EvaluateImageChannel(image,AllChannels,op,value,exception);
  return(status);
}

MagickExport MagickBooleanType EvaluateImageChannel(Image *image,
  const ChannelType channel,const MagickEvaluateOperator op,const double value,
  ExceptionInfo *exception)
{
#define EvaluateImageTag  "Constant/Image "

  IndexPacket
    *indexes;

  long
    y;

  MagickBooleanType
    status;

  register long
    x;

  register PixelPacket
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  image->storage_class=DirectClass;
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
    {
      if ((channel & RedChannel) != 0)
        q->red=ApplyEvaluateOperator(q->red,op,value);
      if ((channel & GreenChannel) != 0)
        q->green=ApplyEvaluateOperator(q->green,op,value);
      if ((channel & BlueChannel) != 0)
        q->blue=ApplyEvaluateOperator(q->blue,op,value);
      if (((channel & OpacityChannel) != 0) && (image->matte != MagickFalse))
        q->opacity=QuantumRange-
          ApplyEvaluateOperator(QuantumRange-q->opacity,op,value);
      if (((channel & IndexChannel) != 0) && (indexes != (IndexPacket *) NULL))
        indexes[x]=(IndexPacket) ApplyEvaluateOperator(indexes[x],op,value);
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(EvaluateImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return((MagickBooleanType) (y == (long) image->rows));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     F x I m a g e C h a n n e l                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FxImageChannel() applies a mathematical expression to the specified image
%  channel(s).
%
%  The format of the FxImageChannel method is:
%
%      Image *FxImageChannel(const Image *image,const ChannelType channel,
%        const char *expression,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o channel: The channel.
%
%    o expression: A mathematical expression.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

typedef enum
{
  UndefinedPrecedence = 0,
  NullPrecedence = 1,
  ExponentPrecedence = 2,
  MultiplyPrecedence = 3,
  DividePrecedence = 3,
  ModulusPrecedence = 4,
  AddPrecedence = 5,
  SubtractPrecedence = 5,
  EqualPrecedence = 6,
  LeftShiftPrecedence = 6,
  RightShiftPrecedence = 6,
  BinaryAndPrecedence = 7,
  BinaryXorPrecedence = 8,
  BinaryOrPrecedence = 9,
  MethodPrecedence = 10
} FxPrecedence;

typedef struct _FxInfo
{
  const Image
    *images;

  ChannelType
    channel;

  long
    x,
    y;

  SplayTreeInfo
    *colors;

  ExceptionInfo
    *exception,
    sans;
} FxInfo;

/*
  Forwared declarations.
*/
static const char
  *FxOperatorPrecedence(FxInfo *,const char *),
  *FxSubexpression(FxInfo *,const char *);

static MagickRealType
  FxEvaluateExpression(FxInfo *,const char *,MagickRealType *),
  FxGetSymbol(FxInfo *,const char *);

static inline MagickPixelPacket *CloneMagickPixelPacket(
  const MagickPixelPacket *pixel)
{
  MagickPixelPacket
    *clone_pixel;

  clone_pixel=(MagickPixelPacket *) AcquireMagickMemory(sizeof(*clone_pixel));
  if (clone_pixel == (MagickPixelPacket *) NULL)
    ThrowMagickFatalException(ResourceLimitFatalError,"MemoryAllocationFailed",
      strerror(errno));
  *clone_pixel=(*pixel);
  return(clone_pixel);
}

static inline MagickRealType FxMax(FxInfo *fx,const char *expression)
{
  MagickRealType
    alpha,
    beta;

  alpha=FxEvaluateExpression(fx,expression,&beta);
  return(Max(alpha,beta));
}

static inline MagickRealType FxMin(FxInfo *fx,const char *expression)
{
  MagickRealType
    alpha,
    beta;

  alpha=FxEvaluateExpression(fx,expression,&beta);
  return(Min(alpha,beta));
}

static inline const char *FxSubexpression(FxInfo *fx,const char *expression)
{
  register long
    level;

  level=0;
  while ((*expression != '\0') &&
         ((level != 1) || (strchr(")",(int) *expression) == (char *) NULL)))
  {
    if (strchr("(",(int) *expression) != (char *) NULL)
      level++;
    else
      if (strchr(")",(int) *expression) != (char *) NULL)
        level--;
    expression++;
  }
  if (*expression == '\0')
    (void) ThrowMagickException(fx->exception,GetMagickModule(),OptionError,
      "UnbalancedParenthesis","`%s'",expression);
  return(expression);
}

static MagickRealType FxEvaluateExpression(FxInfo *fx,const char *expression,
  MagickRealType *beta)
{
  char
    *q,
    subexpression[MaxTextExtent];

  MagickRealType
    alpha;

  register const char
    *p;

  *beta=0.0;
  if (fx->exception->severity != UndefinedException)
    return(0.0);
  while (isspace((int) ((unsigned char) *expression)) != 0)
    expression++;
  if (*expression == '\0')
    {
      (void) ThrowMagickException(fx->exception,GetMagickModule(),OptionError,
        "MissingExpression","`%s'",expression);
      return(0.0);
    }
  p=FxOperatorPrecedence(fx,expression);
  if (p != (const char *) NULL)
    {
      (void) CopyMagickString(subexpression,expression,(size_t)
        (p-expression+1));
      alpha=FxEvaluateExpression(fx,subexpression,beta);
      switch (*p)
      {
        case ',':
        {
          *beta=FxEvaluateExpression(fx,++p,beta);
          return(alpha);
        }
        case '+':
        {
          *beta=FxEvaluateExpression(fx,++p,beta);
          return(alpha+(*beta));
        }
        case '-':
        {
          *beta=FxEvaluateExpression(fx,++p,beta);
          return(alpha-(*beta));
        }
        case '/':
        {
          *beta=FxEvaluateExpression(fx,++p,beta);
          if (*beta == 0.0)
            {
              if (fx->exception->severity == UndefinedException)
                (void) ThrowMagickException(fx->exception,GetMagickModule(),
                  OptionError,"DivideByZero","`%s'",expression);
              return(0.0);
            }
          return(alpha/(*beta));
        }
        case '*':
        {
          *beta=FxEvaluateExpression(fx,++p,beta);
          return(alpha*(*beta));
        }
        case '%':
        {
          *beta=FxEvaluateExpression(fx,++p,beta);
          *beta=fabs(floor(((double) *beta)+0.5));
          if (*beta == 0.0)
            {
              (void) ThrowMagickException(fx->exception,GetMagickModule(),
                OptionError,"DivideByZero","`%s'",expression);
              return(0.0);
            }
          return(fmod((double) alpha,(double) *beta));
        }
        case '^':
        {
          *beta=pow((double) alpha,(double) FxEvaluateExpression(fx,++p,beta));
          return(*beta);
        }
        case '&':
        {
          *beta=(MagickRealType) (QuantumScale*((unsigned long)
            (QuantumRange*alpha) & (unsigned long) (QuantumRange*
            FxEvaluateExpression(fx,++p,beta))));
          return(*beta);
        }
        case ':':
        {
          *beta=(MagickRealType) (QuantumScale*((unsigned long)
            (QuantumRange*alpha) ^ (unsigned long) (QuantumRange*
            FxEvaluateExpression(fx,++p,beta))));
          return(*beta);
        }
        case '|':
        {
          *beta=(MagickRealType) (QuantumScale*((unsigned long)
            (QuantumRange*alpha) | (unsigned long) (QuantumRange*
            FxEvaluateExpression(fx,++p,beta))));
          return(*beta);
        }
        case '=':
        {
          *beta=FxEvaluateExpression(fx,++p,beta);
          return(alpha == *beta ? 1.0 : 0.0);
        }
        case '<':
        {
          *beta=FxEvaluateExpression(fx,++p,beta);
          return(alpha < *beta ? 1.0 : 0.0);
        }
        case '>':
        {
          *beta=FxEvaluateExpression(fx,++p,beta);
          return(alpha > *beta ? 1.0 : 0.0);
        }
        default:
          return(alpha*FxEvaluateExpression(fx,p,beta));
      }
    }
  if (strchr("(",(int) *expression) != (char *) NULL)
    {
      (void) CopyMagickString(subexpression,expression+1,MaxTextExtent);
      subexpression[strlen(subexpression)-1]='\0';
      return(FxEvaluateExpression(fx,subexpression,beta));
    }
  if (*expression == '+')
    return(FxEvaluateExpression(fx,expression+1,beta));
  if (*expression == '-')
    return(-1.0*FxEvaluateExpression(fx,expression+1,beta));
  if (*expression == '~')
    {
      unsigned long
        operand;

      operand=(unsigned long) FxEvaluateExpression(fx,expression+1,beta);
      operand=(~operand);
      alpha=(MagickRealType) operand;
      return(alpha);
    }
  switch (*expression)
  {
    case 'A':
    case 'a':
    {
      if (LocaleCompare(expression,"a") == 0)
        return(FxGetSymbol(fx,expression));
      if (LocaleNCompare(expression,"abs",3) == 0)
        {
          alpha=FxEvaluateExpression(fx,expression+3,beta);
          return(fabs((double) alpha));
        }
      if (LocaleNCompare(expression,"acos",4) == 0)
        {
          alpha=FxEvaluateExpression(fx,expression+4,beta);
          return(acos((double) alpha));
        }
      if (LocaleNCompare(expression,"asin",4) == 0)
        {
          alpha=FxEvaluateExpression(fx,expression+4,beta);
          return(asin((double) alpha));
        }
      if (LocaleNCompare(expression,"atan",4) == 0)
        {
          alpha=FxEvaluateExpression(fx,expression+4,beta);
          return(atan((double) alpha));
        }
      break;
    }
    case 'B':
    case 'b':
    {
      if (LocaleCompare(expression,"b") == 0)
        return(FxGetSymbol(fx,expression));
      break;
    }
    case 'C':
    case 'c':
    {
      if (LocaleCompare(expression,"c") == 0)
        return(FxGetSymbol(fx,expression));
      if (LocaleNCompare(expression,"ceil",4) == 0)
        {
          alpha=FxEvaluateExpression(fx,expression+4,beta);
          return(ceil((double) alpha));
        }
      if (LocaleNCompare(expression,"cos",3) == 0)
        {
          alpha=FxEvaluateExpression(fx,expression+3,beta);
          return(cos((double) alpha));
        }
      break;
    }
    case 'E':
    case 'e':
    {
      if (LocaleNCompare(expression,"exp",3) == 0)
        {
          alpha=FxEvaluateExpression(fx,expression+3,beta);
          return(exp((double) alpha));
        }
      break;
    }
    case 'F':
    case 'f':
    {
      if (LocaleNCompare(expression,"floor",5) == 0)
        {
          alpha=FxEvaluateExpression(fx,expression+5,beta);
          return(floor((double) alpha));
        }
      break;
    }
    case 'G':
    case 'g':
    {
      if (LocaleCompare(expression,"g") == 0)
        return(FxGetSymbol(fx,expression));
      break;
    }
    case 'H':
    case 'h':
    {
      if (LocaleCompare(expression,"h") == 0)
        return(FxGetSymbol(fx,expression));
      break;
    }
    case 'K':
    case 'k':
    {
      if (LocaleCompare(expression,"k") == 0)
        return(FxGetSymbol(fx,expression));
      break;
    }
    case 'I':
    case 'i':
    {
      if (LocaleCompare(expression,"i") == 0)
        return(FxGetSymbol(fx,expression));
      if (LocaleCompare(expression,"intensity") == 0)
        return(FxGetSymbol(fx,expression));
      break;
    }
    case 'J':
    case 'j':
    {
      if (LocaleCompare(expression,"j") == 0)
        return(FxGetSymbol(fx,expression));
      break;
    }
    case 'L':
    case 'l':
    {
      if (LocaleNCompare(expression,"log",3) == 0)
        {
          alpha=FxEvaluateExpression(fx,expression+3,beta);
          return(log10((double) alpha));
        }
      if (LocaleNCompare(expression,"ln",2) == 0)
        {
          alpha=FxEvaluateExpression(fx,expression+2,beta);
          return(log((double) alpha));
        }
      if (LocaleCompare(expression,"luminosity") == 0)
        return(FxGetSymbol(fx,expression));
      break;
    }
    case 'M':
    case 'm':
    {
      if (LocaleCompare(expression,"m") == 0)
        return(FxGetSymbol(fx,expression));
      if (LocaleCompare(expression,"QuantumRange") == 0)
        return((MagickRealType) QuantumRange);
      if (LocaleNCompare(expression,"max",3) == 0)
        return(FxMax(fx,expression+3));
      if (LocaleNCompare(expression,"min",3) == 0)
        return(FxMin(fx,expression+3));
      break;
    }
    case 'O':
    case 'o':
    {
      if (LocaleCompare(expression,"Opaque") == 0)
        return(1.0);
      break;
    }
    case 'P':
    case 'p':
    {
      if (LocaleCompare(expression,"pi") == 0)
        return((MagickRealType) MagickPI);
      if (*expression == 'p')
        return(FxGetSymbol(fx,expression));
      break;
    }
    case 'R':
    case 'r':
    {
      if (LocaleNCompare(expression,"rand",4) == 0)
        return(GetRandomValue());
      if (LocaleCompare(expression,"r") == 0)
        return(FxGetSymbol(fx,expression));
      break;
    }
    case 'S':
    case 's':
    {
      if (LocaleNCompare(expression,"sign",4) == 0)
        {
          alpha=FxEvaluateExpression(fx,expression+4,beta);
          return(alpha < 0.0 ? -1.0 : 1.0);
        }
      if (LocaleNCompare(expression,"sin",3) == 0)
        {
          alpha=FxEvaluateExpression(fx,expression+3,beta);
          return(sin((double) alpha));
        }
      if (LocaleNCompare(expression,"sqrt",4) == 0)
        {
          alpha=FxEvaluateExpression(fx,expression+4,beta);
          return(sqrt((double) alpha));
        }
      break;
    }
    case 'T':
    case 't':
    {
      if (LocaleNCompare(expression,"tan",3) == 0)
        {
          alpha=FxEvaluateExpression(fx,expression+3,beta);
          return(tan((double) alpha));
        }
      if (LocaleCompare(expression,"Transparent") == 0)
        return(0.0);
      break;
    }
    case 'U':
    case 'u':
    {
      if (*expression == 'u')
        return(FxGetSymbol(fx,expression));
      break;
    }
    case 'V':
    case 'v':
    {
      if (*expression == 'v')
        return(FxGetSymbol(fx,expression));
      break;
    }
    case 'W':
    case 'w':
    {
      if (LocaleCompare(expression,"w") == 0)
        return(FxGetSymbol(fx,expression));
      break;
    }
    case 'Y':
    case 'y':
    {
      if (LocaleCompare(expression,"y") == 0)
        return(FxGetSymbol(fx,expression));
      break;
    }
    default:
      break;
  }
  q=(char *) expression;
  alpha=strtod(expression,&q);
  if (q == expression)
    return(FxGetSymbol(fx,expression));
  return(alpha);
}

static MagickRealType FxGetSymbol(FxInfo *fx,const char *expression)
{
  char
    *q,
    subexpression[MaxTextExtent],
    symbol[MaxTextExtent];

  Image
    *image;

  long
    x,
    y;

  MagickPixelPacket
    color;

  MagickRealType
    alpha,
    beta;

  register long
    i;

  PixelPacket
    pixel;

  unsigned long
    level;

  i=0;
  level=0;
  x=fx->x;
  y=fx->y;
  if (isalpha((int) (unsigned char) *(expression+1)) == 0)
    {
      if ((*expression == 'u') || (*expression == 'v'))
        {
          if (*expression == 'v')
            i=1;
          expression++;
          if (*expression == '[')
            {
              level++;
              q=subexpression;
              for (expression++; *expression != '\0'; )
              {
                if (*expression == '[')
                  level++;
                else
                  if (*expression == ']')
                    {
                      level--;
                      if (level == 0)
                        break;
                    }
                *q++=(*expression++);
              }
              *q='\0';
              alpha=FxEvaluateExpression(fx,subexpression,&beta);
              i=(long) (alpha+0.5);
              expression++;
            }
          if (*expression == '.')
            expression++;
        }
      if (*expression == 'p')
        {
          expression++;
          if (*expression == '{')
            {
              level++;
              q=subexpression;
              for (expression++; *expression != '\0'; )
              {
                if (*expression == '{')
                  level++;
                else
                  if (*expression == '}')
                    {
                      level--;
                      if (level == 0)
                        break;
                    }
                *q++=(*expression++);
              }
              *q='\0';
              alpha=FxEvaluateExpression(fx,subexpression,&beta);
              x=(long) (alpha+0.5);
              y=(long) (beta+0.5);
              expression++;
            }
          else
            if (*expression == '[')
              {
                level++;
                q=subexpression;
                for (expression++; *expression != '\0'; )
                {
                  if (*expression == '[')
                    level++;
                  else
                    if (*expression == ']')
                      {
                        level--;
                        if (level == 0)
                          break;
                      }
                  *q++=(*expression++);
                }
                *q='\0';
                alpha=FxEvaluateExpression(fx,subexpression,&beta);
                x=(long) (x+alpha+0.5);
                y=(long) (y+beta+0.5);
                expression++;
              }
          if (*expression == '.')
            expression++;
        }
    }
  image=GetImageFromList(fx->images,i);
  if (image == (Image *) NULL)
    {
      (void) ThrowMagickException(fx->exception,GetMagickModule(),OptionError,
        "NoSuchImage","`%s'",expression);
      return(0.0);
    }
  pixel=AcquireOnePixel(image,x,y,fx->exception);
  color.red=(MagickRealType) pixel.red;
  color.green=(MagickRealType) pixel.green;
  color.blue=(MagickRealType) pixel.blue;
  color.opacity=(MagickRealType) pixel.opacity;
  color.index=0.0;
  if (image->colorspace == CMYKColorspace)
    {
      IndexPacket
        *indexes;

      indexes=GetIndexes(image);
      color.index=(MagickRealType) *indexes;
    }
  if ((strlen(expression) > 2) &&
      (LocaleCompare(expression,"intensity") != 0) &&
      (LocaleCompare(expression,"luminosity") != 0))
    {
      char
        name[MaxTextExtent];

      GetPathComponent(expression,BasePath,name);
      if (strlen(name) > 2)
        {
          MagickPixelPacket
            *pixel;

          pixel=(MagickPixelPacket *) GetValueFromSplayTree(fx->colors,name);
          if (pixel != (MagickPixelPacket *) NULL)
            {
              color=(*pixel);
              expression+=strlen(name);
            }
          else
            if (QueryMagickColor(name,&color,&fx->sans) != MagickFalse)
              {
                (void) AddValueToSplayTree(fx->colors,
                  ConstantString(AcquireString(name)),
                  CloneMagickPixelPacket(&color));
                expression+=strlen(name);
              }
        }
    }
  (void) CopyMagickString(symbol,expression,MaxTextExtent);
  StripString(symbol);
  if (*symbol == '\0')
    {
      switch (fx->channel)
      {
        case RedChannel: return(QuantumScale*color.red);
        case GreenChannel: return(QuantumScale*color.green);
        case BlueChannel: return(QuantumScale*color.blue);
        case OpacityChannel: return(QuantumScale*(QuantumRange-color.opacity));
        case IndexChannel:
        {
          if (image->colorspace != CMYKColorspace)
            {
              (void) ThrowMagickException(fx->exception,GetMagickModule(),
                OptionError,"ColorSeparatedImageRequired","`%s'",
                image->filename);
              return(0.0);
            }
          return(QuantumScale*color.index);
        }
        default:
          break;
      }
      (void) ThrowMagickException(fx->exception,GetMagickModule(),OptionError,
        "UnableToParseExpression","`%s'",expression);
      return(0.0);
    }
  switch (*symbol)
  {
    case 'A':
    case 'a':
    {
      if (LocaleCompare(symbol,"a") == 0)
        {
          if (image->matte == MagickFalse)
            return(0.0);
          return(QuantumScale*(QuantumRange-color.opacity));
        }
      break;
    }
    case 'B':
    case 'b':
    {
      if (LocaleCompare(symbol,"b") == 0)
        return(QuantumScale*color.blue);
      break;
    }
    case 'C':
    case 'c':
    {
      if (LocaleCompare(symbol,"c") == 0)
        return(QuantumScale*color.red);
      break;
    }
    case 'G':
    case 'g':
    {
      if (LocaleCompare(symbol,"g") == 0)
        return(QuantumScale*color.green);
      break;
    }
    case 'K':
    case 'k':
    {
      if (LocaleCompare(symbol,"k") == 0)
        {
          if (image->colorspace != CMYKColorspace)
            {
              (void) ThrowMagickException(fx->exception,GetMagickModule(),
                OptionError,"ColorSeparatedImageRequired","`%s'",
                image->filename);
              return(0.0);
            }
          return(QuantumScale*color.index);
        }
      break;
    }
    case 'H':
    case 'h':
    {
      if (LocaleCompare(symbol,"h") == 0)
        return((MagickRealType) image->rows);
      break;
    }
    case 'I':
    case 'i':
    {
      if (LocaleCompare(symbol,"i") == 0)
        return((MagickRealType) fx->x);
      if (LocaleCompare(symbol,"intensity") == 0)
        return(QuantumScale*PixelIntensity(&color));
      break;
    }
    case 'J':
    case 'j':
    {
      if (LocaleCompare(symbol,"j") == 0)
        return((MagickRealType) fx->y);
      break;
    }
    case 'L':
    case 'l':
    {
      if (LocaleCompare(symbol,"luminosity") == 0)
        {
          double
            hue,
            luminosity,
            saturation;

          pixel.red=RoundToQuantum(color.red);
          pixel.green=RoundToQuantum(color.green);
          pixel.blue=RoundToQuantum(color.blue);
          TransformHSB(pixel.red,pixel.green,pixel.blue,&hue,&saturation,
            &luminosity);
          return(luminosity);
        }
      break;
    }
    case 'M':
    case 'm':
    {
      if (LocaleCompare(symbol,"m") == 0)
        return(QuantumScale*color.blue);
      break;
    }
    case 'R':
    case 'r':
    {
      if (LocaleCompare(symbol,"r") == 0)
        return(QuantumScale*color.red);
      break;
    }
    case 'W':
    case 'w':
    {
      if (LocaleCompare(symbol,"w") == 0)
        return((MagickRealType) image->columns);
      break;
    }
    case 'Y':
    case 'y':
    {
      if (LocaleCompare(symbol,"y") == 0)
        return(QuantumScale*color.green);
      break;
    }
    default:
      break;
  }
  (void) ThrowMagickException(fx->exception,GetMagickModule(),OptionError,
    "UnableToParseExpression","`%s'",symbol);
  return(0.0);
}

static const char *FxOperatorPrecedence(FxInfo *fx,const char *expression)
{
  FxPrecedence
    precedence,
    target;

  register const char
    *subexpression;

  register int
    c;

  unsigned long
    level;

  c=0;
  level=0;
  subexpression=(const char *) NULL;
  target=NullPrecedence;
  while (*expression != '\0')
  {
    precedence=UndefinedPrecedence;
    if ((isspace((int) ((unsigned char) *expression)) != 0) || (c == (int) '@'))
      {
        expression++;
        continue;
      }
    if ((c == (int) '{') || (c == (int) '['))
      level++;
    else
      if ((c == (int) '}') || (c == (int) ']'))
        level--;
    if (level == 0)
      switch (*expression)
      {
        case '+':
        {
          if ((strchr("(+-/*%:&^|<>~,",c) == (char *) NULL) ||
              (isalpha(c) != 0))
            precedence=AddPrecedence;
          break;
        }
        case '-':
        {
          if ((strchr("(+-/*%:&^|<>~,",c) == (char *) NULL) ||
              (isalpha(c) != 0))
            precedence=SubtractPrecedence;
          break;
        }
        case '/':
        {
          precedence=DividePrecedence;
          break;
        }
        case '*':
        {
          precedence=MultiplyPrecedence;
          break;
        }
        case '%':
        {
          precedence=ModulusPrecedence;
          break;
        }
        case ':':
        {
          precedence=BinaryXorPrecedence;
          break;
        }
        case '&':
        {
          precedence=BinaryAndPrecedence;
          break;
        }
        case '^':
        {
          precedence=ExponentPrecedence;
          break;
        }
        case '|':
        {
          precedence=BinaryOrPrecedence;
          break;
        }
        case '=':
        {
          precedence=EqualPrecedence;
          break;
        }
        case '<':
        {
          precedence=LeftShiftPrecedence;
          break;
        }
        case '>':
        {
          precedence=RightShiftPrecedence;
          break;
        }
        case ',':
        {
          if ((strchr(expression,(int) '}') == (char *) NULL) &&
              (strchr(expression,(int) ']') == (char *) NULL))
            precedence=MethodPrecedence;
          break;
        }
        default:
        {
          if (((c != 0) && ((isdigit((int) ((unsigned char) c)) != 0) ||
               (strchr(")",c) != (char *) NULL))) &&
              (((islower((int) ((unsigned char) *expression)) != 0) ||
               (strchr("(",(int) *expression) != (char *) NULL)) ||
               ((isdigit((int) ((unsigned char) c)) == 0) &&
               (isdigit((int) ((unsigned char) *expression)) != 0))) &&
              (strchr("xy",(int) *expression) == (char *) NULL))
            precedence=MultiplyPrecedence;
          break;
        }
      }
    if (precedence >= target)
      {
        target=precedence;
        subexpression=expression;
      }
    if (strchr("(",(int) *expression) != (char *) NULL)
      expression=FxSubexpression(fx,expression);
    c=(int) (*expression++);
  }
  return(subexpression);
}

MagickExport Image *FxImage(const Image *image,const char *expression,
  ExceptionInfo *exception)
{
  Image
    *fx_image;

  fx_image=FxImageChannel(image,(ChannelType) ((long) AllChannels &~
    (long) OpacityChannel),expression,exception);
  return(fx_image);
}

MagickExport Image *FxImageChannel(const Image *image,const ChannelType channel,
  const char *expression,ExceptionInfo *exception)
{
#define FxImageTag  "Fx/Image"

  FxInfo
    fx;

  Image
    *fx_image;

  IndexPacket
    *indexes;

  long
    y;

  MagickBooleanType
    status;

  MagickRealType
    alpha,
    beta;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Fx image.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  fx_image=CloneImage(image,0,0,MagickTrue,exception);
  if (fx_image == (Image *) NULL)
    return((Image *) NULL);
  fx_image->storage_class=DirectClass;
  if ((channel & OpacityChannel) != 0)
    fx_image->matte=MagickTrue;
  (void) ResetMagickMemory(&fx,0,sizeof(fx));
  fx.images=image;
  fx.colors=NewSplayTree(CompareSplayTreeString,RelinquishMagickMemory,
    RelinquishMagickMemory);
  fx.channel=RedChannel;
  fx.exception=exception;
  GetExceptionInfo(&fx.sans);
  alpha=FxEvaluateExpression(&fx,expression,&beta);
  if (fx.exception->severity != UndefinedException)
     {
       fx.colors=DestroySplayTree(fx.colors);
       DestroyExceptionInfo(&fx.sans);
       return((Image *) NULL);
     }
  for (y=0; y < (long) fx_image->rows; y++)
  {
    q=GetImagePixels(fx_image,0,y,fx_image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(fx_image);
    fx.y=y;
    for (x=0; x < (long) fx_image->columns; x++)
    {
      fx.x=x;
      if ((channel & RedChannel) != 0)
        {
          fx.channel=RedChannel;
          alpha=FxEvaluateExpression(&fx,expression,&beta);
          q->red=RoundToQuantum(QuantumRange*alpha);
        }
      if ((channel & GreenChannel) != 0)
        {
          fx.channel=GreenChannel;
          alpha=FxEvaluateExpression(&fx,expression,&beta);
          q->green=RoundToQuantum(QuantumRange*alpha);
        }
      if ((channel & BlueChannel) != 0)
        {
          fx.channel=BlueChannel;
          alpha=FxEvaluateExpression(&fx,expression,&beta);
          q->blue=RoundToQuantum(QuantumRange*alpha);
        }
      if ((channel & OpacityChannel) != 0)
        {
          fx.channel=OpacityChannel;
          alpha=FxEvaluateExpression(&fx,expression,&beta);
          q->opacity=RoundToQuantum(QuantumRange-QuantumRange*alpha);
        }
      if (((channel & IndexChannel) != 0) && (indexes != (IndexPacket *) NULL))
        {
          fx.channel=IndexChannel;
          alpha=FxEvaluateExpression(&fx,expression,&beta);
          indexes[x]=(IndexPacket) RoundToQuantum(QuantumRange*alpha);
        }
      q++;
    }
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(FxImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  fx.colors=DestroySplayTree(fx.colors);
  DestroyExceptionInfo(&fx.sans);
  return(fx_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     I m p l o d e I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ImplodeImage() creates a new image that is a copy of an existing
%  one with the image pixels "implode" by the specified percentage.  It
%  allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ImplodeImage method is:
%
%      Image *ImplodeImage(const Image *image,const double amount,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o implode_image: Method ImplodeImage returns a pointer to the image
%      after it is implode.  A null image is returned if there is a memory
%      shortage.
%
%    o image: The image.
%
%    o amount:  Define the extent of the implosion.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *ImplodeImage(const Image *image,const double amount,
  ExceptionInfo *exception)
{
#define ImplodeImageTag  "Implode/Image"

  Image
    *implode_image;

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MagickRealType
    distance,
    radius;

  PointInfo
    center,
    delta,
    scale;

  register const PixelPacket
    *p;

  register IndexPacket
    *implode_indexes,
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Initialize implode image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  implode_image=CloneImage(image,0,0,MagickTrue,exception);
  if (implode_image == (Image *) NULL)
    return((Image *) NULL);
  implode_image->storage_class=DirectClass;
  if (implode_image->background_color.opacity != OpaqueOpacity)
    implode_image->matte=MagickTrue;
  /*
    Compute scaling factor.
  */
  scale.x=1.0;
  scale.y=1.0;
  center.x=0.5*image->columns;
  center.y=0.5*image->rows;
  radius=center.x;
  if (image->columns > image->rows)
    scale.y=(MagickRealType) image->columns/(MagickRealType) image->rows;
  else
    if (image->columns < image->rows)
      {
        scale.x=(MagickRealType) image->rows/(MagickRealType) image->columns;
        radius=center.y;
      }
  /*
    Implode each row.
  */
  GetMagickPixelPacket(image,&pixel);
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(implode_image,0,y,implode_image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    implode_indexes=GetIndexes(implode_image);
    delta.y=scale.y*(MagickRealType) (y-center.y);
    for (x=0; x < (long) image->columns; x++)
    {
      /*
        Determine if the pixel is within an ellipse.
      */
      delta.x=scale.x*(MagickRealType) (x-center.x);
      distance=delta.x*delta.x+delta.y*delta.y;
      if (distance >= (radius*radius))
        {
          p=AcquireImagePixels(image,x,y,1,1,exception);
          if (p == (const PixelPacket *) NULL)
            break;
          indexes=GetIndexes(image);
          SetMagickPixelPacket(p,indexes,&pixel);
        }
      else
        {
          MagickRealType
            factor;

          /*
            Implode the pixel.
          */
          factor=1.0;
          if (distance > 0.0)
            factor=pow(sin((double) (MagickPI*sqrt((double) distance)/
              radius/2)),-amount);
          pixel=InterpolateColor(image,(double) (factor*delta.x/scale.x+
            center.x+0.5),(double) (factor*delta.y/scale.y+center.y+0.5),
            exception);
        }
      SetPixelPacket(&pixel,q,implode_indexes+x);
      q++;
    }
    if (SyncImagePixels(implode_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(ImplodeImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(implode_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     M o r p h I m a g e s                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  The MorphImages() method requires a minimum of two images.  The first
%  image is transformed into the second by a number of intervening images
%  as specified by frames.
%
%  The format of the MorphImage method is:
%
%      Image *MorphImages(const Image *image,const unsigned long number_frames,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o number_frames:  Define the number of in-between image to generate.
%      The more in-between frames, the smoother the morph.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *MorphImages(const Image *image,
  const unsigned long number_frames,ExceptionInfo *exception)
{
#define MorphImageTag  "Morph/Image"

  Image
    *morph_image,
    *morph_images;

  long
    y;

  MagickBooleanType
    status;

  MagickOffsetType
    scene;

  MagickRealType
    alpha,
    beta;

  register const Image
    *next;

  register const PixelPacket
    *p;

  register long
    i,
    x;

  register PixelPacket
    *q;

  /*
    Clone first frame in sequence.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  morph_images=CloneImage(image,0,0,MagickTrue,exception);
  if (morph_images == (Image *) NULL)
    return((Image *) NULL);
  if (GetNextImageInList(image) == (Image *) NULL)
    {
      /*
        Morph single image.
      */
      for (i=1; i < (long) number_frames; i++)
      {
        morph_image=CloneImage(image,0,0,MagickTrue,exception);
        if (morph_image == (Image *) NULL)
          {
            morph_images=DestroyImageList(morph_images);
            return((Image *) NULL);
          }
        AppendImageToList(&morph_images,morph_image);
        if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
            (QuantumTick(i,number_frames) != MagickFalse))
          {
            status=image->progress_monitor(MorphImageTag,i,number_frames,
              image->client_data);
            if (status == MagickFalse)
              break;
          }
      }
      return(GetFirstImageInList(morph_images));
    }
  /*
    Morph image sequence.
  */
  scene=0;
  next=image;
  for ( ; GetNextImageInList(next) != (Image *) NULL; next=GetNextImageInList(next))
  {
    for (i=0; i < (long) number_frames; i++)
    {
      beta=(MagickRealType) (i+1.0)/(MagickRealType) (number_frames+1.0);
      alpha=1.0-beta;
      morph_image=ZoomImage(next,(unsigned long) (alpha*next->columns+beta*
        GetNextImageInList(next)->columns+0.5),(unsigned long)
        (alpha*next->rows+beta*GetNextImageInList(next)->rows+0.5),exception);
      if (morph_image == (Image *) NULL)
        {
          morph_images=DestroyImageList(morph_images);
          return((Image *) NULL);
        }
      AppendImageToList(&morph_images,morph_image);
      morph_images=GetLastImageInList(morph_images);
      morph_image=ZoomImage(GetNextImageInList(next),morph_images->columns,
        morph_images->rows,exception);
      if (morph_image == (Image *) NULL)
        {
          morph_images=DestroyImageList(morph_images);
          return((Image *) NULL);
        }
      morph_images->storage_class=DirectClass;
      for (y=0; y < (long) morph_images->rows; y++)
      {
        p=AcquireImagePixels(morph_image,0,y,morph_image->columns,1,exception);
        q=GetImagePixels(morph_images,0,y,morph_images->columns,1);
        if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
          break;
        for (x=0; x < (long) morph_images->columns; x++)
        {
          q->red=RoundToQuantum(alpha*q->red+beta*p->red);
          q->green=RoundToQuantum(alpha*q->green+beta*p->green);
          q->blue=RoundToQuantum(alpha*q->blue+beta*p->blue);
          q->opacity=RoundToQuantum(alpha*q->opacity+beta*p->opacity);
          p++;
          q++;
        }
        if (SyncImagePixels(morph_images) == MagickFalse)
          break;
      }
      morph_image=DestroyImage(morph_image);
    }
    if (i < (long) number_frames)
      break;
    /*
      Clone last frame in sequence.
    */
    morph_image=CloneImage(GetNextImageInList(next),0,0,MagickTrue,exception);
    if (morph_image == (Image *) NULL)
      {
        morph_images=DestroyImageList(morph_images);
        return((Image *) NULL);
      }
    AppendImageToList(&morph_images,morph_image);
    morph_images=GetLastImageInList(morph_images);
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(scene,GetImageListLength(image)) != MagickFalse))
      {
        status=image->progress_monitor(MorphImageTag,scene,
          GetImageListLength(image),image->client_data);
        if (status == MagickFalse)
          break;
      }
    scene++;
  }
  if (GetNextImageInList(next) != (Image *) NULL)
    {
      morph_images=DestroyImageList(morph_images);
      return((Image *) NULL);
    }
  return(GetFirstImageInList(morph_images));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     O i l P a i n t I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  OilPaintImage() applies a special effect filter that simulates an oil
%  painting.  Each pixel is replaced by the most frequent color occurring
%  in a circular region defined by radius.
%
%  The format of the OilPaintImage method is:
%
%      Image *OilPaintImage(const Image *image,const double radius,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o radius: The radius of the circular neighborhood.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *OilPaintImage(const Image *image,const double radius,
  ExceptionInfo *exception)
{
#define OilPaintImageTag  "OilPaint/Image"

  const PixelPacket
    *s;

  Image
    *paint_image;

  long
    k,
    y;

  MagickBooleanType
    status;

  register const PixelPacket
    *p,
    *r;

  register long
    u,
    v,
    x;

  register PixelPacket
    *q;

  unsigned long
    count,
    *histogram,
    width;

  /*
    Initialize painted image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  width=GetOptimalKernelWidth2D(radius,0.5);
  if ((image->columns < width) || (image->rows < width))
    ThrowImageException(OptionError,"ImageSmallerThanRadius");
  paint_image=CloneImage(image,0,0,MagickTrue,exception);
  if (paint_image == (Image *) NULL)
    return((Image *) NULL);
  paint_image->storage_class=DirectClass;
  /*
    Allocate histogram and scanline.
  */
  histogram=(unsigned long *) AcquireMagickMemory(256*sizeof(*histogram));
  if (histogram == (unsigned long *) NULL)
    {
      paint_image=DestroyImage(paint_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  /*
    Paint each row of the image.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,-((long) width/2L),y-(long) (width/2L),
      image->columns+width,width,exception);
    q=GetImagePixels(paint_image,0,y,paint_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    for (x=(long) image->columns-1; x >= 0; x--)
    {
      /*
        Determine most frequent color.
      */
      count=0;
      (void) ResetMagickMemory(histogram,0,256*sizeof(*histogram));
      r=p++;
      s=r;
      for (v=0; v < (long) width; v++)
      {
        for (u=0; u < (long) width; u++)
        {
          k=(long) ScaleQuantumToChar(PixelIntensityToQuantum(r+u));
          histogram[k]++;
          if (histogram[k] > count)
            {
              s=r+u;
              count=histogram[k];
            }
        }
        r+=image->columns+width;
      }
      *q++=(*s);
    }
    if (SyncImagePixels(paint_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(OilPaintImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  histogram=(unsigned long *) RelinquishMagickMemory(histogram);
  return(paint_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     S e p i a T o n e I m a g e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MagickSepiaToneImage() applies a special effect to the image, similar to the
%  effect achieved in a photo darkroom by sepia toning.  Threshold ranges from
%  0 to QuantumRange and is a measure of the extent of the sepia toning.  A
%  threshold of 80% is a good starting point for a reasonable tone.
%
%  The format of the SepiaToneImage method is:
%
%      Image *SepiaToneImage(const Image *image,const double threshold,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o threshold: The tone threshold.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *SepiaToneImage(const Image *image,const double threshold,
  ExceptionInfo *exception)
{
#define SepiaToneImageTag  "SepiaTone/Image"

  Image
    *sepia_image;

  long
    y;

  MagickBooleanType
    status;

  MagickRealType
    intensity,
    tone;

  register const PixelPacket
    *p;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Initialize sepia-toned image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  sepia_image=CloneImage(image,image->columns,image->rows,MagickTrue,exception);
  if (sepia_image == (Image *) NULL)
    return((Image *) NULL);
  sepia_image->storage_class=DirectClass;
  /*
    Tone each row of the image.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    q=SetImagePixels(sepia_image,0,y,sepia_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    for (x=(long) image->columns-1; x >= 0; x--)
    {
      intensity=(MagickRealType) PixelIntensityToQuantum(p);
      tone=intensity > threshold ? (MagickRealType) QuantumRange :
        intensity+QuantumRange-threshold;
      q->red=RoundToQuantum(tone);
      tone=intensity > (7.0*threshold/6.0) ? (MagickRealType) QuantumRange :
        intensity+QuantumRange-7.0*threshold/6.0;
      q->green=RoundToQuantum(tone);
      tone=intensity < (threshold/6.0) ? 0 : intensity-threshold/6.0;
      q->blue=RoundToQuantum(tone);
      tone=threshold/7.0;
      if ((MagickRealType) q->green < tone)
        q->green=RoundToQuantum(tone);
      if ((MagickRealType) q->blue < tone)
        q->blue=RoundToQuantum(tone);
      p++;
      q++;
    }
    if (SyncImagePixels(sepia_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(SepiaToneImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  (void) NormalizeImage(sepia_image);
  (void) ContrastImage(sepia_image,MagickTrue);
  return(sepia_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     S h a d o w I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ShadowImage() simulates a shadow from the specified image and retuns it.
%
%  The format of the ShadowImage method is:
%
%      Image *ShadowImage(const Image *image,const double opacity,
%        const double sigma,const long x_offset,const long y_offset,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o opacity: percentage transparency.
%
%    o sigma: The standard deviation of the Gaussian, in pixels.
%
%    o x_offset: the shadow x-offset.
%
%    o y_offset: the shadow y-offset.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *ShadowImage(const Image *image,const double opacity,
  const double sigma,const long x_offset,const long y_offset,
  ExceptionInfo *exception)
{
  Image
    *border_image,
    *clone_image,
    *shadow_image;

  long
    x;

  RectangleInfo
    border_info;

  register long
    y;

  register PixelPacket
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  clone_image=CloneImage(image,0,0,MagickTrue,exception);
  if (clone_image == (Image *) NULL)
    return((Image *) NULL);
  border_info.width=(unsigned long) (2.0*sigma+0.5);
  border_info.height=(unsigned long) (2.0*sigma+0.5);
  border_info.x=0;
  border_info.y=0;
  (void) QueryColorDatabase("none",&clone_image->border_color,exception);
  border_image=BorderImage(clone_image,&border_info,exception);
  clone_image=DestroyImage(clone_image);
  if (border_image == (Image *) NULL)
    return((Image *) NULL);
  for (y=0; y < (long) border_image->rows; y++)
  {
    q=GetImagePixels(border_image,0,y,border_image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    for (x=0; x < (long) border_image->columns; x++)
    {
      q->red=border_image->background_color.red;
      q->green=border_image->background_color.green;
      q->blue=border_image->background_color.blue;
      q->opacity=(Quantum) (QuantumRange-
        (QuantumRange-q->opacity)*opacity/100.0+0.5);
      q++;
    }
    if (SyncImagePixels(border_image) == MagickFalse)
      break;
  }
  shadow_image=BlurImageChannel(border_image,AlphaChannel,0.0,sigma,exception);
  border_image=DestroyImage(border_image);
  if (shadow_image != (Image *) NULL)
    {
      shadow_image->page.x+=x_offset-(long) border_info.width;
      shadow_image->page.y+=y_offset-(long) border_info.height;
    }
  return(shadow_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     S o l a r i z e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SolarizeImage() applies a special effect to the image, similar to the effect
%  achieved in a photo darkroom by selectively exposing areas of photo
%  sensitive paper to light.  Threshold ranges from 0 to QuantumRange and is a
%  measure of the extent of the solarization.
%
%  The format of the SolarizeImage method is:
%
%      MagickBooleanType SolarizeImage(Image *image,const double threshold)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o threshold:  Define the extent of the solarization.
%
%
*/
MagickExport MagickBooleanType SolarizeImage(Image *image,
  const double threshold)
{
#define SolarizeImageTag  "Solarize/Image"

  long
    y;

  MagickBooleanType
    status;

  register long
    i,
    x;

  register PixelPacket
    *q;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->storage_class == PseudoClass)
    {
      /*
        Solarize colormap.
      */
      for (i=0; i < (long) image->colors; i++)
      {
        image->colormap[i].red=(Quantum) ((MagickRealType)
          image->colormap[i].red > threshold ? QuantumRange-
          image->colormap[i].red : image->colormap[i].red);
        image->colormap[i].green=(Quantum) ((MagickRealType)
          image->colormap[i].green > threshold ? QuantumRange-
          image->colormap[i].green : image->colormap[i].green);
        image->colormap[i].blue=(Quantum) ((MagickRealType)
          image->colormap[i].blue > threshold ? QuantumRange-
          image->colormap[i].blue : image->colormap[i].blue);
      }
    }
  /*
    Solarize image.
  */
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(image,0,y,image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      q->red=(Quantum) ((MagickRealType) q->red > threshold ?
        QuantumRange-q->red : q->red);
      q->green=(Quantum) ((MagickRealType) q->green > threshold ?
        QuantumRange-q->green : q->green);
      q->blue=(Quantum) ((MagickRealType) q->blue > threshold ?
        QuantumRange-q->blue : q->blue);
      q++;
    }
    if (SyncImagePixels(image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(SolarizeImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
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
%   S t e g a n o I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SteganoImage() hides a digital watermark within the image.  Recover
%  the hidden watermark later to prove that the authenticity of an image.
%  Offset defines the start position within the image to hide the watermark.
%
%  The format of the SteganoImage method is:
%
%      Image *SteganoImage(const Image *image,Image *watermark,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o watermark: The watermark image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *SteganoImage(const Image *image,const Image *watermark,
  ExceptionInfo *exception)
{
#define GetBit(a,i) ((((a) >> (unsigned long) (i)) & 0x01) != 0)
#define SetBit(a,i,set) a=(Quantum) ((set) ? (a) | \
  (1UL << (unsigned long) (i)) : (a) & ~(1UL << (unsigned long) (i)))
#define SteganoImageTag  "Stegano/Image"

  Image
    *stegano_image;

  int
    c;

  long
    i,
    j,
    k,
    y;

  MagickBooleanType
    status;

  PixelPacket
    pixel;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Initialize steganographic image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(watermark != (const Image *) NULL);
  assert(watermark->signature == MagickSignature);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  stegano_image=CloneImage(image,0,0,MagickTrue,exception);
  if (stegano_image == (Image *) NULL)
    return((Image *) NULL);
  stegano_image->storage_class=DirectClass;
  stegano_image->depth=QuantumDepth;
  /*
    Hide watermark in low-order bits of image.
  */
  c=0;
  i=0;
  j=0;
  k=image->offset;
  for (i=QuantumDepth-1; (i >= 0) && (j < QuantumDepth); i--)
  {
    for (y=0; (y < (long) watermark->rows) && (j < QuantumDepth); y++)
    {
      for (x=0; (x < (long) watermark->columns) && (j < QuantumDepth); x++)
      {
        pixel=AcquireOnePixel(watermark,x,y,exception);
        q=GetImagePixels(stegano_image,k % (long) stegano_image->columns,
          k/(long) stegano_image->columns,1,1);
        if (q == (PixelPacket *) NULL)
          break;
        switch (c)
        {
          case 0:
          {
            SetBit(q->red,j,GetBit(PixelIntensityToQuantum(&pixel),i));
            break;
          }
          case 1:
          {
            SetBit(q->green,j,GetBit(PixelIntensityToQuantum(&pixel),i));
            break;
          }
          case 2:
          {
            SetBit(q->blue,j,GetBit(PixelIntensityToQuantum(&pixel),i));
            break;
          }
        }
        (void) SyncImage(stegano_image);
        c++;
        if (c == 3)
          c=0;
        k++;
        if (k == (long) (stegano_image->columns*stegano_image->columns))
          k=0;
        if (k == image->offset)
          j++;
      }
    }
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(QuantumDepth-i,QuantumDepth) != MagickFalse))
      {
        status=image->progress_monitor(SteganoImageTag,QuantumDepth-i,
          QuantumDepth,image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  if (stegano_image->storage_class == PseudoClass)
    (void) SyncImage(stegano_image);
  return(stegano_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S t e r e o I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  StereoImage() combines two images and produces a single image that is the
%  composite of a left and right image of a stereo pair.  Special red-green
%  stereo glasses are required to view this effect.
%
%  The format of the StereoImage method is:
%
%      Image *StereoImage(const Image *image,const Image *offset_image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o offset_image: Another image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *StereoImage(const Image *image,const Image *offset_image,
  ExceptionInfo *exception)
{
#define StereoImageTag  "Stereo/Image"

  Image
    *stereo_image;

  long
    y;

  MagickBooleanType
    status;

  register const PixelPacket
    *p,
    *q;

  register long
    x;

  register PixelPacket
    *r;

  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  assert(offset_image != (const Image *) NULL);
  if ((image->columns != offset_image->columns) ||
      (image->rows != offset_image->rows))
    ThrowImageException(ImageError,"LeftAndRightImageSizesDiffer");
  /*
    Initialize stereo image attributes.
  */
  stereo_image=CloneImage(image,0,0,MagickTrue,exception);
  if (stereo_image == (Image *) NULL)
    return((Image *) NULL);
  stereo_image->storage_class=DirectClass;
  /*
    Copy left image to red channel and right image to blue channel.
  */
  for (y=0; y < (long) stereo_image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    q=AcquireImagePixels(offset_image,0,y,offset_image->columns,1,exception);
    r=GetImagePixels(stereo_image,0,y,stereo_image->columns,1);
    if ((p == (PixelPacket *) NULL) || (q == (PixelPacket *) NULL) ||
        (r == (PixelPacket *) NULL))
      break;
    for (x=0; x < (long) stereo_image->columns; x++)
    {
      r->red=p->red;
      r->green=q->green;
      r->blue=q->blue;
      r->opacity=(Quantum) ((p->opacity+q->opacity)/2);
      p++;
      q++;
      r++;
    }
    if (SyncImagePixels(stereo_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(StereoImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(stereo_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     S w i r l I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SwirlImage() swirls the pixels about the center of the image, where
%  degrees indicates the sweep of the arc through which each pixel is moved.
%  You get a more dramatic effect as the degrees move from 1 to 360.
%
%  The format of the SwirlImage method is:
%
%      Image *SwirlImage(const Image *image,double degrees,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o degrees: Define the tightness of the swirling effect.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *SwirlImage(const Image *image,double degrees,
  ExceptionInfo *exception)
{
#define SwirlImageTag  "Swirl/Image"

  long
    y;

  Image
    *swirl_image;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MagickRealType
    cosine,
    distance,
    factor,
    radius,
    sine;

  PointInfo
    center,
    delta,
    scale;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes,
    *swirl_indexes;

  register PixelPacket
    *q;

  register long
    x;

  /*
    Initialize swirl image attributes.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  swirl_image=CloneImage(image,0,0,MagickTrue,exception);
  if (swirl_image == (Image *) NULL)
    return((Image *) NULL);
  swirl_image->storage_class=DirectClass;
  if (swirl_image->background_color.opacity != OpaqueOpacity)
    swirl_image->matte=MagickTrue;
  /*
    Compute scaling factor.
  */
  center.x=(MagickRealType) image->columns/2.0;
  center.y=(MagickRealType) image->rows/2.0;
  radius=Max(center.x,center.y);
  scale.x=1.0;
  scale.y=1.0;
  if (image->columns > image->rows)
    scale.y=(MagickRealType) image->columns/(MagickRealType) image->rows;
  else
    if (image->columns < image->rows)
      scale.x=(MagickRealType) image->rows/(MagickRealType) image->columns;
  degrees=DegreesToRadians(degrees);
  /*
    Swirl each row.
  */
  GetMagickPixelPacket(image,&pixel);
  for (y=0; y < (long) image->rows; y++)
  {
    q=GetImagePixels(swirl_image,0,y,swirl_image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    swirl_indexes=GetIndexes(swirl_image);
    delta.y=scale.y*(MagickRealType) (y-center.y);
    for (x=0; x < (long) image->columns; x++)
    {
      /*
        Determine if the pixel is within an ellipse.
      */
      delta.x=scale.x*(MagickRealType) (x-center.x);
      distance=delta.x*delta.x+delta.y*delta.y;
      if (distance >= (radius*radius))
        {
          p=AcquireImagePixels(image,x,y,1,1,exception);
          if (p == (const PixelPacket *) NULL)
            break;
          indexes=GetIndexes(image);
          SetMagickPixelPacket(p,indexes,&pixel);
        }
      else
        {
          /*
            Swirl the pixel.
          */
          factor=1.0-sqrt((double) distance)/radius;
          sine=sin((double) (degrees*factor*factor));
          cosine=cos((double) (degrees*factor*factor));
          pixel=InterpolateColor(image,
            (double) ((cosine*delta.x-sine*delta.y)/scale.x+center.x+0.5),
            (double) ((sine*delta.x+cosine*delta.y)/scale.y+center.y+0.5),
            exception);
        }
      SetPixelPacket(&pixel,q,swirl_indexes+x);
      q++;
    }
    if (SyncImagePixels(swirl_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(SwirlImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(swirl_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     T i n t I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TintImage() applies a color vector to each pixel in the image.  The length
%  of the vector is 0 for black and white and at its maximum for the midtones.
%  The vector weighting function is f(x)=(1-(4.0*((x-0.5)*(x-0.5))))
%
%  The format of the TintImage method is:
%
%      Image *TintImage(const Image *image,const char *opacity,
%        const PixelPacket tint,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o opacity: A color value used for tinting.
%
%    o tint: A color value used for tinting.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *TintImage(const Image *image,const char *opacity,
  const PixelPacket tint,ExceptionInfo *exception)
{
#define TintImageTag  "Tint/Image"

  GeometryInfo
    geometry_info;

  Image
    *tint_image;

  long
    y;

  MagickBooleanType
    status;

  MagickStatusType
    flags;

  MagickPixelPacket
    color_vector,
    pixel;

  MagickRealType
    weight;

  register const PixelPacket
    *p;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Allocate tint image.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  tint_image=CloneImage(image,0,0,MagickTrue,exception);
  if (tint_image == (Image *) NULL)
    return((Image *) NULL);
  tint_image->storage_class=DirectClass;
  if (opacity == (const char *) NULL)
    return(tint_image);
  /*
    Determine RGB values of the color.
  */
  flags=ParseGeometry(opacity,&geometry_info);
  pixel.red=geometry_info.rho;
  if ((flags & SigmaValue) != 0)
    pixel.green=geometry_info.sigma;
  else
    pixel.green=pixel.red;
  if ((flags & XiValue) != 0)
    pixel.blue=geometry_info.xi;
  else
    pixel.blue=pixel.red;
  if ((flags & PsiValue) != 0)
    pixel.opacity=geometry_info.psi;
  else
    pixel.opacity=(MagickRealType) OpaqueOpacity;
  color_vector.red=(MagickRealType) (pixel.red*
    tint.red/100.0-PixelIntensity(&tint));
  color_vector.green=(MagickRealType) (pixel.green*
    tint.green/100.0-PixelIntensity(&tint));
  color_vector.blue=(MagickRealType) (pixel.blue*
    tint.blue/100.0-PixelIntensity(&tint));
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    q=GetImagePixels(tint_image,0,y,tint_image->columns,1);
    if ((p == (const PixelPacket *) NULL) || (q == (PixelPacket *) NULL))
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      weight=(MagickRealType) p->red/QuantumRange-0.5;
      pixel.red=(MagickRealType)
        p->red+color_vector.red*(1.0-(4.0*(weight*weight)));
      q->red=RoundToQuantum(pixel.red);
      weight=(MagickRealType) p->green/QuantumRange-0.5;
      pixel.green=(MagickRealType) p->green+color_vector.green*
        (1.0-(4.0*(weight*weight)));
      q->green=RoundToQuantum(pixel.green);
      weight=(MagickRealType) p->blue/QuantumRange-0.5;
      pixel.blue=(MagickRealType) p->blue+color_vector.blue*(1.0-
        (4.0*(weight*weight)));
      q->blue=RoundToQuantum(pixel.blue);
      q->opacity=p->opacity;
      p++;
      q++;
    }
    if (SyncImagePixels(tint_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(TintImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(tint_image);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     W a v e I m a g e                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WaveImage() creates a "ripple" effect in the image by shifting the pixels
%  vertically along a sine wave whose amplitude and wavelength is specified
%  by the given parameters.
%
%  The format of the WaveImage method is:
%
%      Image *WaveImage(const Image *image,const double amplitude,
%        const double wave_length,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o amplitude, wave_length:  Define the amplitude and wave length of the
%      sine wave.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport Image *WaveImage(const Image *image,const double amplitude,
  const double wave_length,ExceptionInfo *exception)
{
#define WaveImageTag  "Wave/Image"

  Image
    *clone_image,
    *wave_image;

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    pixel;

  MagickRealType
    *sine_map;

  register IndexPacket
    *indexes;

  register long
    x;

  register PixelPacket
    *q;

  /*
    Initialize wave image attributes.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  clone_image=CloneImage(image,0,0,MagickTrue,exception);
  if (clone_image == (Image *) NULL)
    return((Image *) NULL);
  (void) SetImageVirtualPixelMethod(clone_image,ConstantVirtualPixelMethod);
  clone_image->storage_class=DirectClass;
  if (clone_image->background_color.opacity != OpaqueOpacity)
    clone_image->matte=MagickTrue;
  wave_image=CloneImage(clone_image,clone_image->columns,(unsigned long)
    (clone_image->rows+2.0*fabs(amplitude)),MagickTrue,exception);
  if (wave_image == (Image *) NULL)
    {
      (void) DestroyImage(clone_image);
      return((Image *) NULL);
    }
  /*
    Allocate sine map.
  */
  sine_map=(MagickRealType *)
    AcquireMagickMemory((size_t) wave_image->columns*sizeof(*sine_map));
  if (sine_map == (MagickRealType *) NULL)
    {
      clone_image=DestroyImage(clone_image);
      wave_image=DestroyImage(wave_image);
      ThrowImageException(ResourceLimitError,"MemoryAllocationFailed");
    }
  for (x=0; x < (long) wave_image->columns; x++)
    sine_map[x]=fabs(amplitude)+amplitude*sin((2*MagickPI*x)/wave_length);
  /*
    Wave image.
  */
  GetMagickPixelPacket(clone_image,&pixel);
  for (y=0; y < (long) wave_image->rows; y++)
  {
    q=SetImagePixels(wave_image,0,y,wave_image->columns,1);
    if (q == (PixelPacket *) NULL)
      break;
    indexes=GetIndexes(wave_image);
    for (x=0; x < (long) wave_image->columns; x++)
    {
      pixel=InterpolateColor(clone_image,(double) (x+0.5),(double) (y-
        sine_map[x]+0.5),exception);
      SetPixelPacket(&pixel,q,indexes+x);
      q++;
    }
    if (SyncImagePixels(wave_image) == MagickFalse)
      break;
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(WaveImageTag,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  clone_image=DestroyImage(clone_image);
  sine_map=(MagickRealType *) RelinquishMagickMemory(sine_map);
  return(wave_image);
}
