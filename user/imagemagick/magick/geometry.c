/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%           GGGG   EEEEE   OOO   M   M  EEEEE  TTTTT  RRRR   Y   Y            %
%           G      E      O   O  MM MM  E        T    R   R   Y Y             %
%           G  GG  EEE    O   O  M M M  EEE      T    RRRR     Y              %
%           G   G  E      O   O  M   M  E        T    R R      Y              %
%            GGGG  EEEEE   OOO   M   M  EEEEE    T    R  R     Y              %
%                                                                             %
%                                                                             %
%                       ImageMagick Geometry Methods                          %
%                                                                             %
%                             Software Design                                 %
%                               John Cristy                                   %
%                              January 2003                                   %
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
#include "magick/constitute.h"
#include "magick/draw.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/memory_.h"
#include "magick/string_.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t G e o m e t r y                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetGeometry() parses a geometry specification and returns the width,
%  height, x, and y values.  It also returns flags that indicates which
%  of the four values (width, height, x, y) were located in the string, and
%  whether the x or y values are negative.  In addition, there are flags to
%  report any meta characters (%, !, <, or >).
%
%  The format of the GetGeometry method is:
%
%      MagickStatusType GetGeometry(const char *geometry,long *x,long *y,
%        unsigned long *width,unsigned long *height)
%
%  A description of each parameter follows:
%
%    o geometry:  The geometry.
%
%    o x,y:  The x and y offset as determined by the geometry specification.
%
%    o width,height:  The width and height as determined by the geometry
%      specification.
%
%
*/
MagickExport MagickStatusType GetGeometry(const char *geometry,long *x,long *y,
  unsigned long *width,unsigned long *height)
{
  char
    *p,
    pedantic_geometry[MaxTextExtent],
    *q;

  MagickStatusType
    flags;

  /*
    Remove whitespace and meta characters from geometry specification.
  */
  flags=NoValue;
  if ((geometry == (char *) NULL) || (*geometry == '\0'))
    return(flags);
  if (strlen(geometry) >= MaxTextExtent)
    return(flags);
  (void) CopyMagickString(pedantic_geometry,geometry,MaxTextExtent);
  for (p=pedantic_geometry; *p != '\0'; )
  {
    if (isspace((int) ((unsigned char) *p)) != 0)
      {
        (void) CopyMagickString(p,p+1,MaxTextExtent);
        continue;
      }
    switch (*p)
    {
      case '%':
      {
        flags|=PercentValue;
        (void) CopyMagickString(p,p+1,MaxTextExtent);
        break;
      }
      case '!':
      {
        flags|=AspectValue;
        (void) CopyMagickString(p,p+1,MaxTextExtent);
        break;
      }
      case '<':
      {
        flags|=LessValue;
        (void) CopyMagickString(p,p+1,MaxTextExtent);
        break;
      }
      case '>':
      {
        flags|=GreaterValue;
        (void) CopyMagickString(p,p+1,MaxTextExtent);
        break;
      }
      case '@':
      {
        flags|=AreaValue;
        (void) CopyMagickString(p,p+1,MaxTextExtent);
        break;
      }
      case '(':
      case ')':
      {
        (void) CopyMagickString(p,p+1,MaxTextExtent);
        break;
      }
      case '-':
      case '.':
      case '+':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case 'x':
      case 'X':
      {
        p++;
        break;
      }
      default:
        return(flags);
    }
  }
  /*
    Parse width, height, x, and y.
  */
  p=pedantic_geometry;
  if (*p == '\0')
    return(flags);
  q=p;
  (void) strtod(p,&q);
  if (LocaleNCompare(p,"0x",2) == 0)
    (void) strtol(p,&q,10);
  if ((*q == 'x') || (*q == 'X') || (*q == '\0'))
    {
      /*
        Parse width.
      */
      q=p;
      if (LocaleNCompare(p,"0x",2) == 0)
        *width=(unsigned long) strtol(p,&p,10);
      else
        *width=(unsigned long) floor(strtod(p,&p)+0.5);
      if (p != q)
        flags|=WidthValue;
    }
  if ((*p == 'x') || (*p == 'X'))
    {
      p++;
      if ((*p != '+') && (*p != '-'))
        {
          /*
            Parse height.
          */
          q=p;
          *height=(unsigned long) floor(strtod(p,&p)+0.5);
          if (p != q)
            flags|=HeightValue;
        }
    }
  if ((*p == '+') || (*p == '-'))
    {
      /*
        Parse x value.
      */
      if (*p == '-')
        flags|=XNegative;
      q=p;
      *x=(long) ceil(strtod(p,&p)-0.5);
      if (p != q)
        flags|=XValue;
      if ((*p == '+') || (*p == '-'))
        {
          /*
            Parse y value.
          */
          if (*p == '-')
            flags|=YNegative;
          q=p;
          *y=(long) ceil(strtod(p,&p)-0.5);
          if (p != q)
            flags|=YValue;
        }
    }
  return(flags);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  G e t P a g e G e o m e t r y                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPageGeometry() replaces any page mneumonic with the equivalent size in
%  picas.
%
%  The format of the GetPageGeometry method is:
%
%      char *GetPageGeometry(const char *page_geometry)
%
%  A description of each parameter follows.
%
%   o  page_geometry:  Specifies a pointer to an array of characters.
%      The string is either a Postscript page name (e.g. A4) or a postscript
%      page geometry (e.g. 612x792+36+36).
%
%
*/
MagickExport char *GetPageGeometry(const char *page_geometry)
{
  static const char
    *PageSizes[][2]=
    {
      { "4x6",  "288x432" },
      { "5x7",  "360x504" },
      { "7x9",  "504x648" },
      { "8x10", "576x720" },
      { "9x11",  "648x792" },
      { "9x12",  "648x864" },
      { "10x13",  "720x936" },
      { "10x14",  "720x1008" },
      { "11x17",  "792x1224" },
      { "A0",  "2384x3370" },
      { "A1",  "1684x2384" },
      { "A10", "73x105" },
      { "A2",  "1191x1684" },
      { "A3",  "842x1191" },
      { "A4",  "595x842" },
      { "A4SMALL", "595x842" },
      { "A5",  "420x595" },
      { "A6",  "297x420" },
      { "A7",  "210x297" },
      { "A8",  "148x210" },
      { "A9",  "105x148" },
      { "ARCHA", "648x864" },
      { "ARCHB", "864x1296" },
      { "ARCHC", "1296x1728" },
      { "ARCHD", "1728x2592" },
      { "ARCHE", "2592x3456" },
      { "B0",  "2920x4127" },
      { "B1",  "2064x2920" },
      { "B10", "91x127" },
      { "B2",  "1460x2064" },
      { "B3",  "1032x1460" },
      { "B4",  "729x1032" },
      { "B5",  "516x729" },
      { "B6",  "363x516" },
      { "B7",  "258x363" },
      { "B8",  "181x258" },
      { "B9",  "127x181" },
      { "C0",  "2599x3676" },
      { "C1",  "1837x2599" },
      { "C2",  "1298x1837" },
      { "C3",  "918x1296" },
      { "C4",  "649x918" },
      { "C5",  "459x649" },
      { "C6",  "323x459" },
      { "C7",  "230x323" },
      { "EXECUTIVE", "540x720" },
      { "FLSA", "612x936" },
      { "FLSE", "612x936" },
      { "FOLIO",  "612x936" },
      { "HALFLETTER", "396x612" },
      { "ISOB0", "2835x4008" },
      { "ISOB1", "2004x2835" },
      { "ISOB10", "88x125" },
      { "ISOB2", "1417x2004" },
      { "ISOB3", "1001x1417" },
      { "ISOB4", "709x1001" },
      { "ISOB5", "499x709" },
      { "ISOB6", "354x499" },
      { "ISOB7", "249x354" },
      { "ISOB8", "176x249" },
      { "ISOB9", "125x176" },
      { "LEDGER",  "1224x792" },
      { "LEGAL",  "612x1008" },
      { "LETTER", "612x792" },
      { "LETTERSMALL",  "612x792" },
      { "QUARTO",  "610x780" },
      { "STATEMENT",  "396x612" },
      { "TABLOID",  "792x1224" },
      { (char *) NULL, (char *) NULL }
    };

  char
    *page;

  register long
    i;

  assert(page_geometry != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",page_geometry);
  page=AcquireString(page_geometry);
  for (i=0; *PageSizes[i] != (char *) NULL; i++)
    if (LocaleNCompare(PageSizes[i][0],page,strlen(PageSizes[i][0])) == 0)
      {
        RectangleInfo
          geometry;

        MagickStatusType
          flags;

        /*
          Replace mneumonic with the equivalent size in dots-per-inch.
        */
        (void) CopyMagickString(page,PageSizes[i][1],MaxTextExtent);
        (void) ConcatenateMagickString(page,page_geometry+
          strlen(PageSizes[i][0]),MaxTextExtent);
        flags=GetGeometry(page,&geometry.x,&geometry.y,&geometry.width,
          &geometry.height);
        if ((flags & GreaterValue) == 0)
          (void) ConcatenateMagickString(page,">",MaxTextExtent);
        break;
      }
  return(page);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
+     I s G e o m e t r y                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsGeometry() returns MagickTrue if the geometry specification is valid.
%  Examples are 100, 100x200, x200, 100x200+10+20, +10+20, 200%, 200x200!, etc.
%
%  The format of the IsGeometry method is:
%
%      MagickBooleanType IsGeometry(const char *geometry)
%
%  A description of each parameter follows:
%
%    o geometry: This string is the geometry specification.
%
%
*/
MagickExport MagickBooleanType IsGeometry(const char *geometry)
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  if (geometry == (const char *) NULL)
    return(MagickFalse);
  flags=ParseGeometry(geometry,&geometry_info);
  return((MagickBooleanType) (flags != NoValue));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
+     I s S c e n e G e o m e t r y                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsSceneGeometry() returns MagickTrue if the geometry is a valid scene
%  specification (e.g. [1], [1-9], [1,7,4]).
%
%  The format of the IsSceneGeometry method is:
%
%      MagickBooleanType IsSceneGeometry(const char *geometry,
%        const MagickBooleanType pedantic)
%
%  A description of each parameter follows:
%
%    o geometry: This string is the geometry specification.
%
%    o pedantic: A value other than 0 invokes a more restrictive set of
%      conditions for a valid specification (e.g. [1], [1-4], [4-1]).
%
%
*/
MagickExport MagickBooleanType IsSceneGeometry(const char *geometry,
  const MagickBooleanType pedantic)
{
  char
    *p;

  if (geometry == (const char *) NULL)
    return(MagickFalse);
  p=(char *) geometry;
  (void) strtod(geometry,&p);
  if (p == geometry)
    return(MagickFalse);
  if ((strchr(geometry,'x') != (char *) NULL) ||
      (strchr(geometry,'X') != (char *) NULL))
    return(MagickFalse);
  if ((pedantic != MagickFalse) && (strchr(geometry,',') != (char *) NULL))
    return(MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a r s e A b s o l u t e G e o m e t r y                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParseAbsoluteGeometry() returns a region as defined by the geometry string.
%
%  The format of the ParseAbsoluteGeometry method is:
%
%      MagickStatusType ParseAbsoluteGeometry(const char *geometry,
%        RectangeInfo *region_info)
%
%  A description of each parameter follows:
%
%    o geometry:  The geometry (e.g. 100x100+10+10).
%
%    o region_info: The region as defined by the geometry string.
%
%
*/
MagickExport MagickStatusType ParseAbsoluteGeometry(const char *geometry,
  RectangleInfo *region_info)
{
  MagickStatusType
    flags;

  flags=GetGeometry(geometry,&region_info->x,&region_info->y,
    &region_info->width,&region_info->height);
  return(flags);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a r s e G e o m e t r y                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParseGeometry() parses a geometry specification and returns the sigma,
%  rho, xi, and psi values.  It also returns flags that indicates which
%  of the four values (sigma, rho, xi, psi) were located in the string, and
%  whether the xi or pi values are negative.  In addition, there are flags to
%  report any meta characters (%, !, <, or >).
%
%  The format of the ParseGeometry method is:
%
%      MagickStatusType ParseGeometry(const char *geometry,
%        GeometryInfo *geometry_info)
%
%  A description of each parameter follows:
%
%    o geometry:  The geometry.
%
%    o geometry_info:  returns the parsed width/height/x/y in this structure.
%
%
*/
MagickExport MagickStatusType ParseGeometry(const char *geometry,
  GeometryInfo *geometry_info)
{
  char
    *p,
    pedantic_geometry[MaxTextExtent],
    *q;

  double
    value;

  MagickStatusType
    flags;

  /*
    Remove whitespaces meta characters from geometry specification.
  */
  assert(geometry_info != (GeometryInfo *) NULL);
  flags=NoValue;
  if ((geometry == (char *) NULL) || (*geometry == '\0'))
    return(flags);
  if (strlen(geometry) >= MaxTextExtent)
    return(flags);
  (void) CopyMagickString(pedantic_geometry,geometry,MaxTextExtent);
  for (p=pedantic_geometry; *p != '\0'; )
  {
    if (isspace((int) ((unsigned char) *p)) != 0)
      {
        (void) CopyMagickString(p,p+1,MaxTextExtent);
        continue;
      }
    switch (*p)
    {
      case '%':
      {
        flags|=PercentValue;
        (void) CopyMagickString(p,p+1,MaxTextExtent);
        break;
      }
      case '!':
      {
        flags|=AspectValue;
        (void) CopyMagickString(p,p+1,MaxTextExtent);
        break;
      }
      case '<':
      {
        flags|=LessValue;
        (void) CopyMagickString(p,p+1,MaxTextExtent);
        break;
      }
      case '>':
      {
        flags|=GreaterValue;
        (void) CopyMagickString(p,p+1,MaxTextExtent);
        break;
      }
      case '@':
      {
        flags|=AreaValue;
        (void) CopyMagickString(p,p+1,MaxTextExtent);
        break;
      }
      case '(':
      case ')':
      {
        (void) CopyMagickString(p,p+1,MaxTextExtent);
        break;
      }
      case '-':
      case '.':
      case '+':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case 'x':
      case 'X':
      case ',':
      case '/':
      {
        p++;
        break;
      }
      default:
        return(flags);
    }
  }
  /*
    Parse rho, sigma, xi, psi, and optionally chi.
  */
  p=pedantic_geometry;
  if (*p == '\0')
    return(flags);
  q=p;
  (void) strtod(p,&q);
  if (LocaleNCompare(p,"0x",2) == 0)
    (void) strtol(p,&q,10);
  if ((*q == 'x') || (*q == 'X') || (*q == '/') || (*q == ',') || (*q =='\0'))
    {
      /*
        Parse rho.
      */
      q=p;
      if (LocaleNCompare(p,"0x",2) == 0)
        value=(double) strtol(p,&p,10);
      else
        value=strtod(p,&p);
      if (p != q)
        {
          flags|=RhoValue;
          geometry_info->rho=value;
        }
    }
  q=p;
  if ((*p == 'x') || (*p == 'X') || (*p == ',') || (*p == '/'))
    {
      /*
        Parse sigma.
      */
      p++;
      while (isspace((int) ((unsigned char) *p)) != 0)
        p++;
      if (((*q != 'x') && (*q != 'X')) || ((*p != '+') && (*p != '-')))
        {
          q=p;
          value=strtod(p,&p);
          if (p != q)
            {
              flags|=SigmaValue;
              geometry_info->sigma=value;
            }
        }
    }
  while (isspace((int) ((unsigned char) *p)) != 0)
    p++;
  if ((*p == '+') || (*p == '-') || (*p == ',') || (*p == '/'))
    {
      /*
        Parse xi value.
      */
      if ((*p == ',') || (*p == '/'))
        p++;
      q=p;
      value=strtod(p,&p);
      if (p != q)
        {
          flags|=XiValue;
          if (*q == '-')
            flags|=XiNegative;
          geometry_info->xi=value;
        }
      while (isspace((int) ((unsigned char) *p)) != 0)
        p++;
      if ((*p == '+') || (*p == '-') || (*p == ',') || (*p == '/'))
        {
          /*
            Parse psi value.
          */
          if ((*p == ',') || (*p == '/'))
            p++;
          q=p;
          value=strtod(p,&p);
          if (p != q)
            {
              flags|=PsiValue;
              if (*q == '-')
                flags|=PsiNegative;
              geometry_info->psi=value;
            }
        }
      while (isspace((int) ((unsigned char) *p)) != 0)
        p++;
      if ((*p == '+') || (*p == '-') || (*p == ',') || (*p == '/'))
        {
          /*
            Parse chi value.
          */
          if ((*p == ',') || (*p == '/'))
            p++;
          q=p;
          value=strtod(p,&p);
          if (p != q)
            {
              flags|=ChiValue;
              if (*q == '-')
                flags|=ChiNegative;
              geometry_info->chi=value;
            }
        }
    }
  return(flags);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a r s e G r a v i t y G e o m e t r y                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParseGravityGeometry() returns a region as defined by the geometry string
%  with respect to the image dimensions and its gravity.
%
%  The format of the ParseGravityGeometry method is:
%
%      MagickStatusType ParseGravityGeometry(Image *image,const char *geometry,
%        RectangeInfo *region_info)
%
%  A description of each parameter follows:
%
%    o geometry:  The geometry (e.g. 100x100+10+10).
%
%    o region_info: The region as defined by the geometry string with
%      respect to the image dimensions and its gravity.
%
*/
MagickExport MagickStatusType ParseGravityGeometry(Image *image,
  const char *geometry,RectangleInfo *region_info)
{
  MagickStatusType
    flags;

  unsigned long
    height,
    width;

  SetGeometry(image,region_info);
  flags=ParseAbsoluteGeometry(geometry,region_info);
  if (flags == NoValue)
    {
      (void) ThrowMagickException(&image->exception,GetMagickModule(),
        OptionError,"InvalidGeometry","`%s'",geometry);
      return(flags);
    }
  if ((flags & PercentValue) != 0)
    {
      GeometryInfo
        geometry_info;

      MagickStatusType
        flags;

      PointInfo
        scale;

      /*
        Geometry is a percentage of the image size.
      */
      flags=ParseGeometry(geometry,&geometry_info);
      scale.x=geometry_info.rho;
      if ((flags & RhoValue) == 0)
        scale.x=100.0;
      scale.y=geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        scale.y=scale.x;
      region_info->width=(unsigned long) ((scale.x*image->columns/100.0)+0.5);
      region_info->height=(unsigned long) ((scale.y*image->rows/100.0)+0.5);
    }
  width=region_info->width;
  height=region_info->height;
  if (width == 0)
    {
      if (image->page.width != 0)
        width=image->page.width;
      else
        width=image->columns;
    }
  if (height == 0)
    {
      if (image->page.height != 0)
        height=image->page.height;
      else
        height=image->rows;
    }
  switch (image->gravity)
  {
    case ForgetGravity:
    case NorthWestGravity:
      break;
    case NorthGravity:
    {
      region_info->x+=(long) (image->columns/2-width/2);
      break;
    }
    case NorthEastGravity:
    {
      region_info->x=(long) (image->columns-width-region_info->x);
      break;
    }
    case WestGravity:
    {
      region_info->y+=(long) (image->rows/2-height/2);
      break;
    }
    case StaticGravity:
    case CenterGravity:
    default:
    {
      region_info->x+=(long) (image->columns/2-width/2);
      region_info->y+=(long) (image->rows/2-height/2);
      break;
    }
    case EastGravity:
    {
      region_info->x=(long) (image->columns-width-region_info->x);
      region_info->y+=(long) (image->rows/2-height/2);
      break;
    }
    case SouthWestGravity:
    {
      region_info->y=(long) (image->rows-height-region_info->y);
      break;
    }
    case SouthGravity:
    {
      region_info->x+=(long) (image->columns/2-width/2);
      region_info->y=(long) (image->rows-height-region_info->y);
      break;
    }
    case SouthEastGravity:
    {
      region_info->x=(long) (image->columns-width-region_info->x);
      region_info->y=(long) (image->rows-height-region_info->y);
      break;
    }
  }
  return(flags);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   P a r s e M e t a G e o m e t r y                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParseMetaGeometry() is similar to GetGeometry() except the returned
%  geometry is modified as determined by the meta characters:  %, !, <, >,
%  and ~.
%
%  The format of the ParseMetaGeometry method is:
%
%      MagickStatusType ParseMetaGeometry(const char *geometry,long *x,long *y,
%        unsigned long *width,unsigned long *height)
%
%  A description of each parameter follows:
%
%    o geometry:  The geometry.
%
%    o x,y:  The x and y offset as determined by the geometry specification.
%
%    o width,height:  The width and height as determined by the geometry
%      specification.
%
*/
MagickExport MagickStatusType ParseMetaGeometry(const char *geometry,long *x,
  long *y,unsigned long *width,unsigned long *height)
{
  GeometryInfo
    geometry_info;

  MagickStatusType
    flags;

  unsigned long
    former_height,
    former_width;

  /*
    Ensure the image geometry is valid.
  */
  assert(x != (long *) NULL);
  assert(y != (long *) NULL);
  assert(width != (unsigned long *) NULL);
  assert(height != (unsigned long *) NULL);
  if ((geometry == (char *) NULL) || (*geometry == '\0'))
    return(NoValue);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",geometry);
  /*
    Parse geometry using GetGeometry.
  */
  SetGeometryInfo(&geometry_info);
  former_width=(*width);
  former_height=(*height);
  flags=GetGeometry(geometry,x,y,width,height);
  if ((flags & PercentValue) != 0)
    {
      MagickStatusType
        flags;

      PointInfo
        scale;

      /*
        Geometry is a percentage of the image size.
      */
      flags=ParseGeometry(geometry,&geometry_info);
      scale.x=geometry_info.rho;
      if ((flags & RhoValue) == 0)
        scale.x=100.0;
      scale.y=geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        scale.y=scale.x;
      *width=(unsigned long) ((scale.x*former_width/100.0)+0.5);
      *height=(unsigned long) ((scale.y*former_height/100.0)+0.5);
      former_width=(*width);
      former_height=(*height);
    }
  if (((flags & AspectValue) == 0) &&
      ((*width != former_width) || (*height != former_height)))
    {
      MagickRealType
        scale_factor;

      /*
        Respect aspect ratio of the image.
      */
      if ((former_width == 0) || (former_height == 0))
        scale_factor=1.0;
      else
        if (((flags & RhoValue) != 0) && (flags & SigmaValue) != 0)
          {
            scale_factor=(MagickRealType) *width/(MagickRealType) former_width;
            if (scale_factor > ((MagickRealType) *height/
                (MagickRealType) former_height))
              scale_factor=(MagickRealType)
                *height/(MagickRealType) former_height;
          }
        else
          if ((flags & RhoValue) != 0)
            scale_factor=(MagickRealType) *width/(MagickRealType) former_width;
          else
            scale_factor=(MagickRealType)
              *height/(MagickRealType) former_height;
    *width=(unsigned long) (scale_factor*former_width+0.5);
    *height=(unsigned long) (scale_factor*former_height+0.5);
  }
  if ((flags & GreaterValue) != 0)
    {
      if (former_width < *width)
        *width=former_width;
      if (former_height < *height)
        *height=former_height;
    }
  if ((flags & LessValue) != 0)
    {
      if (former_width > *width)
        *width=former_width;
      if (former_height > *height)
        *height=former_height;
    }
  if ((flags & AreaValue) != 0)
    {
      MagickRealType
        area,
        distance;

      PointInfo
        scale;

      /*
        Geometry is a maximum area in pixels.
      */
      (void) ParseGeometry(geometry,&geometry_info);
      area=geometry_info.rho;
      distance=sqrt((double) former_width*former_height);
      scale.x=(MagickRealType) former_width/(distance/sqrt((double) area));
      scale.y=(MagickRealType) former_height/(distance/sqrt((double) area));
      if ((scale.x < (MagickRealType) *width) ||
          (scale.y < (MagickRealType) *height))
        {
          *width=(unsigned long) (former_width/(distance/sqrt((double) area)));
          *height=(unsigned long) (former_height/(distance/
            sqrt((double) area)));
        }
      former_width=(*width);
      former_height=(*height);
    }
  return(flags);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a r s e P a g e G e o m e t r y                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParsePageGeometry() returns a region as defined by the geometry string with
%  respect to the image dimensions.
%
%  The format of the ParsePageGeometry method is:
%
%      MagickStatusType ParsePageGeometry(Image *image,const char *geometry,
%        RectangeInfo *region_info)
%
%  A description of each parameter follows:
%
%    o geometry:  The geometry (e.g. 100x100+10+10).
%
%    o region_info: The region as defined by the geometry string with
%      respect to the image and its gravity.
%
*/
MagickExport MagickStatusType ParsePageGeometry(Image *image,
  const char *geometry,RectangleInfo *region_info)
{
  MagickStatusType
    flags;

  SetGeometry(image,region_info);
  flags=ParseAbsoluteGeometry(geometry,region_info);
  if (flags == NoValue)
    {
      (void) ThrowMagickException(&image->exception,GetMagickModule(),
        OptionError,"InvalidGeometry","`%s'",geometry);
      return(flags);
    }
  flags=ParseMetaGeometry(geometry,&region_info->x,&region_info->y,
    &region_info->width,&region_info->height);
  return(flags);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   P a r s e S i z e G e o m e t r y                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParseSizeGeometry() returns a region as defined by the geometry string with
%  respect to the image dimensions and aspect ratio.
%
%  The format of the ParseSizeGeometry method is:
%
%      MagickStatusType ParseSizeGeometry(Image *image,const char *geometry,
%        RectangeInfo *region_info)
%
%  A description of each parameter follows:
%
%    o geometry:  The geometry (e.g. 100x100+10+10).
%
%    o region_info: The region as defined by the geometry string.
%
%
*/
MagickExport MagickStatusType ParseSizeGeometry(Image *image,
  const char *geometry,RectangleInfo *region_info)
{
  MagickStatusType
    flags;

  SetGeometry(image,region_info);
  flags=ParseMetaGeometry(geometry,&region_info->x,&region_info->y,
    &region_info->width,&region_info->height);
  if (flags == NoValue)
    (void) ThrowMagickException(&image->exception,GetMagickModule(),OptionError,
      "InvalidGeometry","`%s'",geometry);
  return(flags);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t G e o m e t r y                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetGeometry sets the geometry to its default values.
%
%  The format of the SetGeometry method is:
%
%      SetGeometry(const Image *image,RectangleInfo *geometry)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o geometry: The geometry.
%
%
*/
MagickExport void SetGeometry(const Image *image,RectangleInfo *geometry)
{
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(geometry != (RectangleInfo *) NULL);
  (void) ResetMagickMemory(geometry,0,sizeof(*geometry));
  geometry->width=image->columns;
  if (image->page.width != 0)
    geometry->width=image->page.width;
  geometry->height=image->rows;
  if (image->page.height != 0)
    geometry->height=image->page.height;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t G e o m e t r y I n f o                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetGeometryInfo sets the GeometryInfo structure to its default values.
%
%  The format of the SetGeometryInfo method is:
%
%      SetGeometryInfo(GeometryInfo *geometry_info)
%
%  A description of each parameter follows:
%
%    o geometry_info: The geometry info structure.
%
%
*/
MagickExport void SetGeometryInfo(GeometryInfo *geometry_info)
{
  assert(geometry_info != (GeometryInfo *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  (void) ResetMagickMemory(geometry_info,0,sizeof(*geometry_info));
}
