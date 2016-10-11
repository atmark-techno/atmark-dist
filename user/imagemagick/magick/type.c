/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                        TTTTT  Y   Y  PPPP   EEEEE                           %
%                          T     Y Y   P   P  E                               %
%                          T      Y    PPPP   EEE                             %
%                          T      Y    P      E                               %
%                          T      Y    P      EEEEE                           %
%                                                                             %
%                                                                             %
%                     ImageMagick Image Type Methods                          %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 May 2001                                    %
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
#include "magick/blob.h"
#include "magick/client.h"
#include "magick/configure.h"
#include "magick/draw.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/hashmap.h"
#include "magick/log.h"
#include "magick/memory_.h"
#if defined(__WINDOWS__)
# include "magick/nt-feature.h"
#endif
#include "magick/option.h"
#include "magick/semaphore.h"
#include "magick/splay-tree.h"
#include "magick/string_.h"
#include "magick/type.h"
#include "magick/token.h"
#include "magick/utility.h"

/*
  Define declarations.
*/
#define MagickTypeFilename  "type.xml"

/*
  Declare type map.
*/
static const char
  *TypeMap = (const char *)
    "<?xml version=\"1.0\"?>"
    "<typemap>"
    "  <type stealth=\"True\" />"
    "</typemap>";

/*
  Static declarations.
*/
static SemaphoreInfo
  *type_semaphore = (SemaphoreInfo *) NULL;

static volatile MagickBooleanType
  instantiate_type = MagickFalse;

static SplayTreeInfo
  *type_list = (SplayTreeInfo *) NULL;

/*
  Forward declarations.
*/
static MagickBooleanType
  InitializeTypeList(ExceptionInfo *),
  LoadTypeLists(const char *,ExceptionInfo *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y T y p e L i s t                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyTypeList() deallocates memory associated with the font list.
%
%  The format of the DestroyTypeList method is:
%
%      DestroyTypeList(void)
%
%
*/
MagickExport void DestroyTypeList(void)
{
  AcquireSemaphoreInfo(&type_semaphore);
  if (type_list != (SplayTreeInfo *) NULL)
    type_list=DestroySplayTree(type_list);
  instantiate_type=MagickFalse;
  RelinquishSemaphoreInfo(type_semaphore);
  type_semaphore=DestroySemaphoreInfo(type_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t T y p e I n f o                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetTypeInfo searches the type list for the specified name and if found
%  returns attributes for that type.
%
%  The format of the GetTypeInfo method is:
%
%      const TypeInfo *GetTypeInfo(const char *name,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o name: The type name.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport const TypeInfo *GetTypeInfo(const char *name,
  ExceptionInfo *exception)
{
  register const TypeInfo
    *p;

  assert(exception != (ExceptionInfo *) NULL);
  if ((type_list == (SplayTreeInfo *) NULL) ||
      (instantiate_type == MagickFalse))
    if (InitializeTypeList(exception) == MagickFalse)
      return((const TypeInfo *) NULL);
  if ((type_list == (SplayTreeInfo *) NULL) ||
      (GetNumberOfNodesInSplayTree(type_list) == 0))
    return((const TypeInfo *) NULL);
  if ((name == (const char *) NULL) || (LocaleCompare(name,"*") == 0))
    {
      ResetSplayTreeIterator(type_list);
      return((const TypeInfo *) GetNextValueInSplayTree(type_list));
    }
  /*
    Search for requested type.
  */
  AcquireSemaphoreInfo(&type_semaphore);
  ResetSplayTreeIterator(type_list);
  p=(const TypeInfo *) GetNextValueInSplayTree(type_list);
  while (p != (const TypeInfo *) NULL)
  {
    if (LocaleCompare(name,p->name) == 0)
      break;
    p=(const TypeInfo *) GetNextValueInSplayTree(type_list);
  }
  RelinquishSemaphoreInfo(type_semaphore);
  return(p);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t T y p e I n f o B y F a m i l y                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetTypeInfoByFamily() searches the type list for the specified family and if
%  found returns attributes for that type.
%
%  Type substitution and scoring algorithm contributed by Bob Friesenhahn.
%
%  The format of the GetTypeInfoByFamily method is:
%
%      const TypeInfo *GetTypeInfoByFamily(const char *family,
%        const StyleType style,const StretchType stretch,
%        const unsigned long weight,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o family: The type family.
%
%    o style: The type style.
%
%    o stretch: The type stretch.
%
%    o weight: The type weight.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport const TypeInfo *GetTypeInfoByFamily(const char *family,
  const StyleType style,const StretchType stretch,const unsigned long weight,
  ExceptionInfo *exception)
{
  typedef struct _Fontmap
  {
    const char
      *name,
      *substitute;
  } Fontmap;

  const TypeInfo
    *type_info;

  long
    range;

  register const TypeInfo
    *p;

  register long
    i;

  static Fontmap
    fontmap[] =
    {
      { "fixed", "courier" },
      { "modern","courier" },
      { "monotype corsiva", "courier" },
      { "news gothic", "helvetica" },
      { "system", "courier" },
      { "terminal", "courier" },
      { "wingdings", "symbol" },
      { NULL, NULL }
    };

  unsigned long
    max_score,
    score;

  /*
    Check for an exact type match.
  */
  (void) GetTypeInfo("*",exception);
  if (type_list == (SplayTreeInfo *) NULL)
    return((TypeInfo *) NULL);
  AcquireSemaphoreInfo(&type_semaphore);
  ResetSplayTreeIterator(type_list);
  type_info=(const TypeInfo *) NULL;
  p=(const TypeInfo *) GetNextValueInSplayTree(type_list);
  while (p != (const TypeInfo *) NULL)
  {
    if (p->family == (char *) NULL)
      {
        p=(const TypeInfo *) GetNextValueInSplayTree(type_list);
        continue;
      }
    if (family == (const char *) NULL)
      {
        if ((LocaleCompare(p->family,"arial") != 0) &&
            (LocaleCompare(p->family,"helvetica") != 0))
          {
            p=(const TypeInfo *) GetNextValueInSplayTree(type_list);
            continue;
          }
      }
    else
      if (LocaleCompare(p->family,family) != 0)
        {
          p=(const TypeInfo *) GetNextValueInSplayTree(type_list);
          continue;
        }
    if ((style != UndefinedStyle) && (style != AnyStyle) && (p->style != style))
      {
        p=(const TypeInfo *) GetNextValueInSplayTree(type_list);
        continue;
      }
    if ((stretch != UndefinedStretch) && (stretch != AnyStretch) &&
        (p->stretch != stretch))
      {
        p=(const TypeInfo *) GetNextValueInSplayTree(type_list);
        continue;
      }
    if ((weight != 0) && (p->weight != weight))
      {
        p=(const TypeInfo *) GetNextValueInSplayTree(type_list);
        continue;
      }
    type_info=p;
    break;
  }
  RelinquishSemaphoreInfo(type_semaphore);
  if (type_info != (const TypeInfo *) NULL)
    return(type_info);
  /*
    Check for types in the same family.
  */
  max_score=0;
  AcquireSemaphoreInfo(&type_semaphore);
  ResetSplayTreeIterator(type_list);
  p=(const TypeInfo *) GetNextValueInSplayTree(type_list);
  while (p != (const TypeInfo *) NULL)
  {
    if (p->family == (char *) NULL)
      {
        p=(const TypeInfo *) GetNextValueInSplayTree(type_list);
        continue;
      }
    if (family == (const char *) NULL)
      {
        if ((LocaleCompare(p->family,"arial") != 0) &&
            (LocaleCompare(p->family,"helvetica") != 0))
          {
            p=(const TypeInfo *) GetNextValueInSplayTree(type_list);
            continue;
          }
      }
    else
      if (LocaleCompare(p->family,family) != 0)
        {
          p=(const TypeInfo *) GetNextValueInSplayTree(type_list);
          continue;
        }
    score=0;
    if ((style == UndefinedStyle) || (style == AnyStyle) || (p->style == style))
      score+=32;
    else
      if (((style == ItalicStyle) || (style == ObliqueStyle)) &&
          ((p->style == ItalicStyle) || (p->style == ObliqueStyle)))
        score+=25;
    if (weight == 0)
      score+=16;
    else
      score+=(16*(800-((long) Max(Min(weight,900),p->weight)-
        (long) Min(Min(weight,900),p->weight))))/800;
    if ((stretch == UndefinedStretch) || (stretch == AnyStretch))
      score+=8;
    else
      {
        range=(long) UltraExpandedStretch-(long) NormalStretch;
        score+=(8*(range-((long) Max(stretch,p->stretch)-
          (long) Min(stretch,p->stretch))))/range;
      }
    if (score > max_score)
      {
        max_score=score;
        type_info=p;
      }
    p=(const TypeInfo *) GetNextValueInSplayTree(type_list);
  }
  RelinquishSemaphoreInfo(type_semaphore);
  if (type_info != (const TypeInfo *) NULL)
    return(type_info);
  /*
    Check for table-based substitution match.
  */
  for (i=0; fontmap[i].name != (char *) NULL; i++)
  {
    if (family == (const char *) NULL)
      {
        if ((LocaleCompare(fontmap[i].name,"arial") != 0) &&
            (LocaleCompare(fontmap[i].name,"helvetica") != 0))
          continue;
      }
    else
      if (LocaleCompare(fontmap[i].name,family) != 0)
        continue;
    type_info=GetTypeInfoByFamily(fontmap[i].substitute,style,stretch,weight,
      exception);
    break;
  }
  if (type_info != (const TypeInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),TypeError,
        "FontSubstitutionRequired","`%s'",type_info->family);
      return(type_info);
    }
  if (family != (const char *) NULL)
    type_info=GetTypeInfoByFamily((const char *) NULL,style,stretch,weight,
      exception);
  return(type_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t T y p e I n f o L i s t                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetTypeInfoList() returns any fonts that match the specified pattern.
%
%  The format of the GetTypeInfoList function is:
%
%      const TypeInfo **GetTypeInfoList(const char *pattern,
%        unsigned long *number_fonts,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_fonts:  This integer returns the number of types in the list.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int TypeInfoCompare(const void *x,const void *y)
{
  const TypeInfo
    **p,
    **q;

  p=(const TypeInfo **) x,
  q=(const TypeInfo **) y;
  if (LocaleCompare((*p)->path,(*q)->path) == 0)
    return(LocaleCompare((*p)->name,(*q)->name));
  return(LocaleCompare((*p)->path,(*q)->path));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport const TypeInfo **GetTypeInfoList(const char *pattern,
  unsigned long *number_fonts,ExceptionInfo *exception)
{
  const TypeInfo
    **fonts;

  register const TypeInfo
    *p;

  register long
    i;

  /*
    Allocate type list.
  */
  assert(pattern != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  assert(number_fonts != (unsigned long *) NULL);
  *number_fonts=0;
  p=GetTypeInfo("*",exception);
  if (p == (const TypeInfo *) NULL)
    return((const TypeInfo **) NULL);
  fonts=(const TypeInfo **) AcquireMagickMemory((size_t)
    (GetNumberOfNodesInSplayTree(type_list)+1)*sizeof(*fonts));
  if (fonts == (const TypeInfo **) NULL)
    return((const TypeInfo **) NULL);
  /*
    Generate type list.
  */
  AcquireSemaphoreInfo(&type_semaphore);
  ResetSplayTreeIterator(type_list);
  p=(const TypeInfo *) GetNextValueInSplayTree(type_list);
  for (i=0; p != (const TypeInfo *) NULL; )
  {
    if ((p->stealth == MagickFalse) &&
        (GlobExpression(p->name,pattern) != MagickFalse))
      fonts[i++]=p;
    p=(const TypeInfo *) GetNextValueInSplayTree(type_list);
  }
  RelinquishSemaphoreInfo(type_semaphore);
  qsort((void *) fonts,(size_t) i,sizeof(*fonts),TypeInfoCompare);
  fonts[i]=(TypeInfo *) NULL;
  *number_fonts=(unsigned long) i;
  return(fonts);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t T y p e L i s t                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetTypeList() returns any fonts that match the specified pattern.
%
%  The format of the GetTypeList function is:
%
%      char **GetTypeList(const char *pattern,unsigned long *number_fonts,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_fonts:  This integer returns the number of fonts in the list.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int TypeCompare(const void *x,const void *y)
{
  register const char
    **p,
    **q;

  p=(const char **) x;
  q=(const char **) y;
  return(LocaleCompare(*p,*q));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport char **GetTypeList(const char *pattern,unsigned long *number_fonts,
  ExceptionInfo *exception)
{
  char
    **fonts;

  register const TypeInfo
    *p;

  register long
    i;

  /*
    Allocate type list.
  */
  assert(pattern != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  assert(number_fonts != (unsigned long *) NULL);
  *number_fonts=0;
  p=GetTypeInfo("*",exception);
  if (p == (const TypeInfo *) NULL)
    return((char **) NULL);
  fonts=(char **) AcquireMagickMemory((size_t)
    (GetNumberOfNodesInSplayTree(type_list)+1)*sizeof(*fonts));
  if (fonts == (char **) NULL)
    return((char **) NULL);
  /*
    Generate type list.
  */
  AcquireSemaphoreInfo(&type_semaphore);
  ResetSplayTreeIterator(type_list);
  p=(const TypeInfo *) GetNextValueInSplayTree(type_list);
  for (i=0; p != (const TypeInfo *) NULL; )
  {
    if ((p->stealth == MagickFalse) &&
        (GlobExpression(p->name,pattern) != MagickFalse))
      fonts[i++]=ConstantString(AcquireString(p->name));
    p=(const TypeInfo *) GetNextValueInSplayTree(type_list);
  }
  RelinquishSemaphoreInfo(type_semaphore);
  qsort((void *) fonts,(size_t) i,sizeof(*fonts),TypeCompare);
  fonts[i]=(char *) NULL;
  *number_fonts=(unsigned long) i;
  return(fonts);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I n i t i a l i z e T y p e L i s t                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InitializeTypeList() initializes the type list.
%
%  The format of the InitializeTypeList method is:
%
%      MagickBooleanType InitializeTypeList(ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
static MagickBooleanType InitializeTypeList(ExceptionInfo *exception)
{
  if ((type_list == (SplayTreeInfo *) NULL) &&
      (instantiate_type == MagickFalse))
    {
      AcquireSemaphoreInfo(&type_semaphore);
      if ((type_list == (SplayTreeInfo *) NULL) &&
          (instantiate_type == MagickFalse))
        {
          (void) LoadTypeLists(MagickTypeFilename,exception);
#if defined(__WINDOWS__)
          (void) NTLoadTypeLists(type_list,exception);
#endif
          instantiate_type=MagickTrue;
        }
      RelinquishSemaphoreInfo(type_semaphore);
    }
  return((MagickBooleanType) (type_list != (SplayTreeInfo *) NULL));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L i s t T y p e I n f o                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListTypeInfo() lists the fonts to a file.
%
%  The format of the ListTypeInfo method is:
%
%      MagickBooleanType ListTypeInfo(FILE *file,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o file:  An pointer to a FILE.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType ListTypeInfo(FILE *file,ExceptionInfo *exception)
{
  char
    weight[MaxTextExtent];

  const char
    *family,
    *name,
    *path,
    *stretch,
    *style;

  const TypeInfo
    **type_info;

  register long
    i;

  unsigned long
    number_fonts;

  if (file == (FILE *) NULL)
    file=stdout;
  number_fonts=0;
  type_info=GetTypeInfoList("*",&number_fonts,exception);
  if (type_info == (const TypeInfo **) NULL)
    return(MagickFalse);
  *weight='\0';
  path=(const char *) NULL;
  for (i=0; i < (long) number_fonts; i++)
  {
    if (type_info[i]->stealth != MagickFalse)
      continue;
    if ((path == (const char *) NULL) ||
        (LocaleCompare(path,type_info[i]->path) != 0))
      {
        if (type_info[i]->path != (char *) NULL)
          (void) fprintf(file,"\nPath: %s\n\n",type_info[i]->path);
        (void) fprintf(file,"%-32.32s %-23.23s %-7.7s %-8s %-3s\n",
          "Name","Family","Style","Stretch","Weight");
        (void) fprintf(file,"--------------------------------------------------"
          "------------------------------\n");
      }
    path=type_info[i]->path;
    name="unknown";
    if (type_info[i]->name != (char *) NULL)
      name=type_info[i]->name;
    family="unknown";
    if (type_info[i]->family != (char *) NULL)
      family=type_info[i]->family;
    style=MagickOptionToMnemonic(MagickStyleOptions,type_info[i]->style);
    stretch=MagickOptionToMnemonic(MagickStretchOptions,type_info[i]->stretch);
    (void) FormatMagickString(weight,MaxTextExtent,"%lu",type_info[i]->weight);
    (void) fprintf(file,"%-32.32s %-23.23s %-7.7s %-9s %-3s\n",name,family,
      style,stretch,weight);
  }
  (void) fflush(file);
  type_info=(const TypeInfo **) RelinquishMagickMemory((void *) type_info);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   L o a d T y p e L i s t                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LoadTypeList() loads the type configuration file which provides a mapping
%  between type attributes and a type name.
%
%  The format of the LoadTypeList method is:
%
%      MagickBooleanType LoadTypeList(const char *xml,const char *filename,
%        const unsigned long depth,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o xml:  The type list in XML format.
%
%    o filename:  The type list filename.
%
%    o depth: depth of <include /> statements.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

static void *DestroyTypeNode(void *type_info)
{
  register TypeInfo
    *p;

  p=(TypeInfo *) type_info;
  if (p->path != (char *) NULL)
    p->path=(char *) RelinquishMagickMemory(p->path);
  if (p->name != (char *) NULL)
    p->name=(char *) RelinquishMagickMemory(p->name);
  if (p->description != (char *) NULL)
    p->description=(char *) RelinquishMagickMemory(p->description);
  if (p->family != (char *) NULL)
    p->family=(char *) RelinquishMagickMemory(p->family);
  if (p->encoding != (char *) NULL)
    p->encoding=(char *) RelinquishMagickMemory(p->encoding);
  if (p->foundry != (char *) NULL)
    p->foundry=(char *) RelinquishMagickMemory(p->foundry);
  if (p->format != (char *) NULL)
    p->format=(char *) RelinquishMagickMemory(p->format);
  if (p->metrics != (char *) NULL)
    p->metrics=(char *) RelinquishMagickMemory(p->metrics);
  if (p->glyphs != (char *) NULL)
    p->glyphs=(char *) RelinquishMagickMemory(p->glyphs);
  return(RelinquishMagickMemory(p));
}

static MagickBooleanType LoadTypeList(const char *xml,const char *filename,
  const unsigned long depth,ExceptionInfo *exception)
{
  char
#if defined(__WINDOWS__)
    font_path[MaxTextExtent],
#endif
    keyword[MaxTextExtent],
    *q,
    *token;

  MagickStatusType
    status;

  TypeInfo
    *type_info = (TypeInfo *) NULL;

  /*
    Load the type map file.
  */
  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
    "Loading type file \"%s\" ...",filename);
  if (xml == (const char *) NULL)
    return(MagickFalse);
  if (type_list == (SplayTreeInfo *) NULL)
    {
      type_list=NewSplayTree(CompareSplayTreeString,RelinquishMagickMemory,
        DestroyTypeNode);
      if (type_list == (SplayTreeInfo *) NULL)
        {
          ThrowFileException(exception,ResourceLimitError,
            "MemoryAllocationFailed",filename);
          return(MagickFalse);
        }
    }
  status=MagickTrue;
  token=AcquireString(xml);
#if defined(__WINDOWS__)
  /*
    Determine the Ghostscript font path.
  */
  *font_path='\0';
  if (NTGhostscriptFonts(font_path,MaxTextExtent-2))
    (void) ConcatenateMagickString(font_path,DirectorySeparator,MaxTextExtent);
#endif
  for (q=(char *) xml; *q != '\0'; )
  {
    /*
      Interpret XML.
    */
    GetMagickToken(q,&q,token);
    if (*token == '\0')
      break;
    (void) CopyMagickString(keyword,token,MaxTextExtent);
    if (LocaleNCompare(keyword,"<!DOCTYPE",9) == 0)
      {
        /*
          Doctype element.
        */
        while ((LocaleNCompare(q,"]>",2) != 0) && (*q != '\0'))
          GetMagickToken(q,&q,token);
        continue;
      }
    if (LocaleNCompare(keyword,"<!--",4) == 0)
      {
        /*
          Comment element.
        */
        while ((LocaleNCompare(q,"->",2) != 0) && (*q != '\0'))
          GetMagickToken(q,&q,token);
        continue;
      }
    if (LocaleCompare(keyword,"<include") == 0)
      {
        /*
          Include element.
        */
        while (((*token != '/') && (*(token+1) != '>')) && (*q != '\0'))
        {
          (void) CopyMagickString(keyword,token,MaxTextExtent);
          GetMagickToken(q,&q,token);
          if (*token != '=')
            continue;
          GetMagickToken(q,&q,token);
          if (LocaleCompare(keyword,"file") == 0)
            {
              if (depth > 200)
                (void) ThrowMagickException(exception,GetMagickModule(),
                  ConfigureError,"IncludeNodeNestedTooDeeply","`%s'",token);
              else
                {
                  char
                    path[MaxTextExtent],
                    *xml;

                  *path='\0';
                  GetPathComponent(filename,HeadPath,path);
                  if (*path != '\0')
                    (void) ConcatenateMagickString(path,DirectorySeparator,
                      MaxTextExtent);
                  (void) ConcatenateMagickString(path,token,MaxTextExtent);
                  xml=FileToString(path,~0,exception);
                  status|=LoadTypeList(xml,path,depth+1,exception);
                  xml=(char *) RelinquishMagickMemory(xml);
                }
            }
        }
        continue;
      }
    if (LocaleCompare(keyword,"<type") == 0)
      {
        /*
          Allocate memory for the type list.
        */
        type_info=(TypeInfo *) AcquireMagickMemory(sizeof(*type_info));
        if (type_info == (TypeInfo *) NULL)
          ThrowMagickFatalException(ResourceLimitFatalError,
            "MemoryAllocationFailed",filename);
        (void) ResetMagickMemory(type_info,0,sizeof(*type_info));
        type_info->path=ConstantString(AcquireString(filename));
        type_info->signature=MagickSignature;
        continue;
      }
    if (type_info == (TypeInfo *) NULL)
      continue;
    if (LocaleCompare(keyword,"/>") == 0)
      {
        status=AddValueToSplayTree(type_list,
          ConstantString(AcquireString(type_info->name)),type_info);
        if (status == MagickFalse)
          (void) ThrowMagickException(exception,GetMagickModule(),
            ResourceLimitError,"MemoryAllocationFailed","`%s'",type_info->name);
        type_info=(TypeInfo *) NULL;
      }
    GetMagickToken(q,(char **) NULL,token);
    if (*token != '=')
      continue;
    GetMagickToken(q,&q,token);
    GetMagickToken(q,&q,token);
    switch (*keyword)
    {
      case 'E':
      case 'e':
      {
        if (LocaleCompare((char *) keyword,"encoding") == 0)
          {
            type_info->encoding=ConstantString(AcquireString(token));
            break;
          }
        break;
      }
      case 'F':
      case 'f':
      {
        if (LocaleCompare((char *) keyword,"face") == 0)
          {
            type_info->face=(unsigned long) atol(token);
            break;
          }
        if (LocaleCompare((char *) keyword,"family") == 0)
          {
            type_info->family=ConstantString(AcquireString(token));
            break;
          }
        if (LocaleCompare((char *) keyword,"format") == 0)
          {
            type_info->format=ConstantString(AcquireString(token));
            break;
          }
        if (LocaleCompare((char *) keyword,"foundry") == 0)
          {
            type_info->foundry=ConstantString(AcquireString(token));
            break;
          }
        if (LocaleCompare((char *) keyword,"fullname") == 0)
          {
            type_info->description=ConstantString(AcquireString(token));
            break;
          }
        break;
      }
      case 'G':
      case 'g':
      {
        if (LocaleCompare((char *) keyword,"glyphs") == 0)
          {
            char
              *glyphs;

            glyphs=AcquireString(token);
#if defined(__WINDOWS__)
            if (strchr(glyphs,'@') != (char *) NULL)
              SubstituteString(&glyphs,"@ghostscript_font_path@",font_path);
#endif
            type_info->glyphs=ConstantString(glyphs);
            break;
          }
        break;
      }
      case 'M':
      case 'm':
      {
        if (LocaleCompare((char *) keyword,"metrics") == 0)
          {
            char
              *metrics;

            metrics=AcquireString(token);
#if defined(__WINDOWS__)
            if (strchr(metrics,'@') != (char *) NULL)
              SubstituteString(&metrics,"@ghostscript_font_path@",font_path);
#endif
            type_info->metrics=ConstantString(metrics);
            break;
          }
        break;
      }
      case 'N':
      case 'n':
      {
        if (LocaleCompare((char *) keyword,"name") == 0)
          {
            type_info->name=ConstantString(AcquireString(token));
            break;
          }
        break;
      }
      case 'S':
      case 's':
      {
        if (LocaleCompare((char *) keyword,"stealth") == 0)
          {
            type_info->stealth=(MagickBooleanType)
              (LocaleCompare(token,"True") == 0);
            break;
          }
        if (LocaleCompare((char *) keyword,"stretch") == 0)
          {
            type_info->stretch=(StretchType)
              ParseMagickOption(MagickStretchOptions,MagickFalse,token);
            break;
          }
        if (LocaleCompare((char *) keyword,"style") == 0)
          {
            type_info->style=(StyleType)
              ParseMagickOption(MagickStyleOptions,MagickFalse,token);
            break;
          }
        break;
      }
      case 'W':
      case 'w':
      {
        if (LocaleCompare((char *) keyword,"weight") == 0)
          {
            type_info->weight=(unsigned long) atol(token);
            if (LocaleCompare(token,"bold") == 0)
              type_info->weight=700;
            if (LocaleCompare(token,"normal") == 0)
              type_info->weight=400;
            break;
          }
        break;
      }
      default:
        break;
    }
  }
  token=(char *) RelinquishMagickMemory(token);
  if (type_list == (SplayTreeInfo *) NULL)
    return(MagickFalse);
  return(status != 0 ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L o a d T y p e L i s t s                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LoadTypeList() loads one or more type configuration files which provides a
%  mapping between type attributes a type name.
%
%  The format of the LoadTypeLists method is:
%
%      MagickBooleanType LoadTypeLists(const char *filename,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: The font file name.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
static MagickBooleanType LoadTypeLists(const char *filename,
  ExceptionInfo *exception)
{
#if defined(UseEmbeddableMagick)
  return(LoadTypeList(TypeMap,"built-in",0,exception));
#else
  const char
    *p;

  const StringInfo
    *option;

  LinkedListInfo
    *options;

  MagickStatusType
    status;

  status=MagickFalse;
  options=GetConfigureOptions(filename,exception);
  option=(const StringInfo *) GetNextValueInLinkedList(options);
  while (option != (const StringInfo *) NULL)
  {
    status|=LoadTypeList((const char *) option->datum,option->path,0,exception);
    option=(const StringInfo *) GetNextValueInLinkedList(options);
  }
  options=DestroyConfigureOptions(options);
  p=getenv("MAGICK_FONT_PATH");
  if (p != (const char *) NULL)
    {
      char
        path[MaxTextExtent],
        *option;

      /*
        Search MAGICK_FONT_PATH.
      */
      (void) FormatMagickString(path,MaxTextExtent,"%s%s%s",p,
        DirectorySeparator,filename);
      option=FileToString(path,~0,exception);
      if (option != (void *) NULL)
        {
          status|=LoadTypeList(option,path,0,exception);
          option=(char *) RelinquishMagickMemory(option);
        }
    }
  if ((type_list == (SplayTreeInfo *) NULL) || 
      (GetNumberOfNodesInSplayTree(type_list) == 0))
    status|=LoadTypeList(TypeMap,"built-in",0,exception);
  else
    (void) SetExceptionInfo(exception,UndefinedException);
  return(status != 0 ? MagickTrue : MagickFalse);
#endif
}
