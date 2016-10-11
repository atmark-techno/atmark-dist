/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                   CCCC   OOO   DDDD    EEEEE  RRRR                          %
%                  C      O   O  D   D   E      R   R                         %
%                  C      O   O  D   D   EEE    RRRR                          %
%                  C      O   O  D   D   E      R R                           %
%                   CCCC   OOO   DDDD    EEEEE  R  R                          %
%                                                                             %
%                                                                             %
%                     ImageMagick Image Coder Methods                         %
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
#include "magick/coder.h"
#include "magick/configure.h"
#include "magick/draw.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/hashmap.h"
#include "magick/log.h"
#include "magick/memory_.h"
#include "magick/option.h"
#include "magick/semaphore.h"
#include "magick/string_.h"
#include "magick/splay-tree.h"
#include "magick/token.h"
#include "magick/utility.h"

/*
  Define declarations.
*/
#define MagickCoderFilename  "coder.xml"

/*
  Declare coder map.
*/
static const char
  *CoderMap = (const char *)
    "<?xml version=\"1.0\"?>"
    "<codermap>"
    "  <coder magick=\"8BIM\" name=\"META\" />"
    "  <coder magick=\"8BIMTEXT\" name=\"META\" />"
    "  <coder magick=\"8BIMWTEXT\" name=\"META\" />"
    "  <coder magick=\"AFM\" name=\"TTF\" />"
    "  <coder magick=\"APP1JPEG\" name=\"META\" />"
    "  <coder magick=\"APP1\" name=\"META\" />"
    "  <coder magick=\"BIE\" name=\"JBIG\" />"
    "  <coder magick=\"BMP2\" name=\"BMP\" />"
    "  <coder magick=\"BMP3\" name=\"BMP\" />"
    "  <coder magick=\"B\" name=\"GRAY\" />"
    "  <coder magick=\"CMYKA\" name=\"CMYK\" />"
    "  <coder magick=\"C\" name=\"GRAY\" />"
    "  <coder magick=\"CUR\" name=\"ICON\" />"
    "  <coder magick=\"DCX\" name=\"PCX\" />"
    "  <coder magick=\"DNG\" name=\"TIFF\" />"
    "  <coder magick=\"EMF\" name=\"EMF\" />"
    "  <coder magick=\"EPDF\" name=\"PDF\" />"
    "  <coder magick=\"EPI\" name=\"PS\" />"
    "  <coder magick=\"EPS2\" name=\"PS2\" />"
    "  <coder magick=\"EPS3\" name=\"PS3\" />"
    "  <coder magick=\"EPSF\" name=\"PS\" />"
    "  <coder magick=\"EPSI\" name=\"PS\" />"
    "  <coder magick=\"EPS\" name=\"PS\" />"
    "  <coder magick=\"FILE\" name=\"URL\" />"
    "  <coder magick=\"FRACTAL\" name=\"PLASMA\" />"
    "  <coder magick=\"FTP\" name=\"URL\" />"
    "  <coder magick=\"G3\" name=\"FAX\" />"
    "  <coder magick=\"GIF87\" name=\"GIF\" />"
    "  <coder magick=\"G\" name=\"GRAY\" />"
    "  <coder magick=\"GRANITE\" name=\"MAGICK\" />"
    "  <coder magick=\"H5\" name=\"HDF\" />"
    "  <coder magick=\"H\" name=\"MAGICK\" />"
    "  <coder magick=\"HTM\" name=\"HTML\" />"
    "  <coder magick=\"HTTP\" name=\"URL\" />"
    "  <coder magick=\"ICB\" name=\"TGA\" />"
    "  <coder magick=\"ICC\" name=\"META\" />"
    "  <coder magick=\"ICM\" name=\"META\" />"
    "  <coder magick=\"ICO\" name=\"ICON\" />"
    "  <coder magick=\"IPTC\" name=\"META\" />"
    "  <coder magick=\"IPTCTEXT\" name=\"META\" />"
    "  <coder magick=\"IPTCWTEXT\" name=\"META\" />"
    "  <coder magick=\"JBG\" name=\"JBIG\" />"
    "  <coder magick=\"JNG\" name=\"PNG\" />"
    "  <coder magick=\"JPC\" name=\"JP2\" />"
    "  <coder magick=\"JPG\" name=\"JPEG\" />"
    "  <coder magick=\"JPX\" name=\"JP2\" />"
    "  <coder magick=\"K\" name=\"GRAY\" />"
    "  <coder magick=\"LOGO\" name=\"MAGICK\" />"
    "  <coder magick=\"M2V\" name=\"MPEG\" />"
    "  <coder magick=\"M\" name=\"GRAY\" />"
    "  <coder magick=\"MNG\" name=\"PNG\" />"
    "  <coder magick=\"MPG\" name=\"MPEG\" />"
    "  <coder magick=\"MPRI\" name=\"MPR\" />"
    "  <coder magick=\"NETSCAPE\" name=\"MAGICK\" />"
    "  <coder magick=\"O\" name=\"GRAY\" />"
    "  <coder magick=\"OTF\" name=\"TTF\" />"
    "  <coder magick=\"P7\" name=\"PNM\" />"
    "  <coder magick=\"PAL\" name=\"UYVY\" />"
    "  <coder magick=\"PBM\" name=\"PNM\" />"
    "  <coder magick=\"PCDS\" name=\"PCD\" />"
    "  <coder magick=\"PCT\" name=\"PICT\" />"
    "  <coder magick=\"PFA\" name=\"TTF\" />"
    "  <coder magick=\"PFB\" name=\"TTF\" />"
    "  <coder magick=\"PFM\" name=\"TTF\" />"
    "  <coder magick=\"PGM\" name=\"PNM\" />"
    "  <coder magick=\"PGX\" name=\"JP2\" />"
    "  <coder magick=\"PICON\" name=\"XPM\" />"
    "  <coder magick=\"PJPEG\" name=\"JPEG\" />"
    "  <coder magick=\"PM\" name=\"XPM\" />"
    "  <coder magick=\"PNG24\" name=\"PNG\" />"
    "  <coder magick=\"PNG32\" name=\"PNG\" />"
    "  <coder magick=\"PNG8\" name=\"PNG\" />"
    "  <coder magick=\"PPM\" name=\"PNM\" />"
    "  <coder magick=\"PSD\" name=\"PSD\" />"
    "  <coder magick=\"PTIF\" name=\"TIFF\" />"
    "  <coder magick=\"RAS\" name=\"SUN\" />"
    "  <coder magick=\"RGBA\" name=\"RGB\" />"
    "  <coder magick=\"R\" name=\"GRAY\" />"
    "  <coder magick=\"ROSE\" name=\"MAGICK\" />"
    "  <coder magick=\"SHTML\" name=\"HTML\" />"
    "  <coder magick=\"SVGZ\" name=\"SVG\" />"
    "  <coder magick=\"TEXT\" name=\"TXT\" />"
    "  <coder magick=\"TIF\" name=\"TIFF\" />"
    "  <coder magick=\"TTC\" name=\"TTF\" />"
    "  <coder magick=\"VDA\" name=\"TGA\" />"
    "  <coder magick=\"VST\" name=\"TGA\" />"
    "  <coder magick=\"WMFWIN32\" name=\"EMF\" />"
    "  <coder magick=\"XTRNARRAY\" name=\"XTRN\" />"
    "  <coder magick=\"XTRNBLOB\" name=\"XTRN\" />"
    "  <coder magick=\"XTRNBSTR\" name=\"XTRN\" />"
    "  <coder magick=\"XTRNFILE\" name=\"XTRN\" />"
    "  <coder magick=\"XTRNIMAGE\" name=\"XTRN\" />"
    "  <coder magick=\"XTRNSTREAM\" name=\"XTRN\" />"
    "  <coder magick=\"XV\" name=\"VIFF\" />"
    "  <coder magick=\"Y\" name=\"GRAY\" />"
    "</codermap>";

/*
  Static declarations.
*/
static SemaphoreInfo
  *coder_semaphore = (SemaphoreInfo *) NULL;

static SplayTreeInfo
  *coder_list = (SplayTreeInfo *) NULL;

static volatile MagickBooleanType
  instantiate_coder = MagickFalse;

/*
  Forward declarations.
*/
static MagickBooleanType
  InitializeCoderList(ExceptionInfo *),
  LoadCoderLists(const char *,ExceptionInfo *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y C o d e r L i s t                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyCoderList() deallocates memory associated with the font list.
%
%  The format of the DestroyCoderList method is:
%
%      DestroyCoderList(void)
%
%
*/
MagickExport void DestroyCoderList(void)
{
  AcquireSemaphoreInfo(&coder_semaphore);
  if (coder_list != (SplayTreeInfo *) NULL)
    coder_list=DestroySplayTree(coder_list);
  instantiate_coder=MagickFalse;
  RelinquishSemaphoreInfo(coder_semaphore);
  coder_semaphore=DestroySemaphoreInfo(coder_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t C o d e r I n f o                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetCoderInfo searches the coder list for the specified name and if found
%  returns attributes for that coder.
%
%  The format of the GetCoderInfo method is:
%
%      const CoderInfo *GetCoderInfo(const char *name,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o name: The coder name.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport const CoderInfo *GetCoderInfo(const char *name,
  ExceptionInfo *exception)
{
  register const CoderInfo
    *p;

  assert(exception != (ExceptionInfo *) NULL);
  if ((coder_list == (SplayTreeInfo *) NULL) ||
      (instantiate_coder == MagickFalse))
    if (InitializeCoderList(exception) == MagickFalse)
      return((const CoderInfo *) NULL);
  if ((coder_list == (SplayTreeInfo *) NULL) ||
      (GetNumberOfNodesInSplayTree(coder_list) == 0))
    return((const CoderInfo *) NULL);
  if ((name == (const char *) NULL) || (LocaleCompare(name,"*") == 0))
    {
      ResetSplayTreeIterator(coder_list);
      return((const CoderInfo *) GetNextValueInSplayTree(coder_list));
    }
  /*
    Search for requested coder.
  */
  AcquireSemaphoreInfo(&coder_semaphore);
  ResetSplayTreeIterator(coder_list);
  p=(const CoderInfo *) GetNextValueInSplayTree(coder_list);
  while (p != (const CoderInfo *) NULL)
  {
    if (LocaleCompare(name,p->magick) == 0)
      break;
    p=(const CoderInfo *) GetNextValueInSplayTree(coder_list);
  }
  RelinquishSemaphoreInfo(coder_semaphore);
  return(p);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t C o d e r I n f o L i s t                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetCoderInfoList() returns any coders that match the specified pattern.
%
%  The format of the GetCoderInfoList function is:
%
%      const CoderInfo **GetCoderInfoList(const char *pattern,
%        unsigned long *number_coders,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_coders:  This integer returns the number of coders in the list.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

static int CoderInfoCompare(const void *x,const void *y)
{
  const CoderInfo
    **p,
    **q;

  p=(const CoderInfo **) x,
  q=(const CoderInfo **) y;
  if (LocaleCompare((*p)->path,(*q)->path) == 0)
    return(LocaleCompare((*p)->name,(*q)->name));
  return(LocaleCompare((*p)->path,(*q)->path));
}

MagickExport const CoderInfo **GetCoderInfoList(const char *pattern,
  unsigned long *number_coders,ExceptionInfo *exception)
{
  const CoderInfo
    **coders;

  register const CoderInfo
    *p;

  register long
    i;

  /*
    Allocate coder list.
  */
  assert(pattern != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  assert(number_coders != (unsigned long *) NULL);
  *number_coders=0;
  p=GetCoderInfo("*",exception);
  if (p == (const CoderInfo *) NULL)
    return((const CoderInfo **) NULL);
  coders=(const CoderInfo **) AcquireMagickMemory((size_t)
    (GetNumberOfNodesInSplayTree(coder_list)+1)*sizeof(*coders));
  if (coders == (const CoderInfo **) NULL)
    return((const CoderInfo **) NULL);
  /*
    Generate coder list.
  */
  AcquireSemaphoreInfo(&coder_semaphore);
  ResetSplayTreeIterator(coder_list);
  p=(const CoderInfo *) GetNextValueInSplayTree(coder_list);
  for (i=0; p != (const CoderInfo *) NULL; )
  {
    if ((p->stealth == MagickFalse) &&
        (GlobExpression(p->name,pattern) != MagickFalse))
      coders[i++]=p;
    p=(const CoderInfo *) GetNextValueInSplayTree(coder_list);
  }
  RelinquishSemaphoreInfo(coder_semaphore);
  qsort((void *) coders,(size_t) i,sizeof(*coders),CoderInfoCompare);
  coders[i]=(CoderInfo *) NULL;
  *number_coders=(unsigned long) i;
  return(coders);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t C o d e r L i s t                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetCoderList() returns any coders that match the specified pattern.
%
%  The format of the GetCoderList function is:
%
%      char **GetCoderList(const char *pattern,unsigned long *number_coders,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_coders:  This integer returns the number of coders in the list.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

static int CoderCompare(const void *x,const void *y)
{
  register const char
    **p,
    **q;

  p=(const char **) x;
  q=(const char **) y;
  return(LocaleCompare(*p,*q));
}

MagickExport char **GetCoderList(const char *pattern,
  unsigned long *number_coders,ExceptionInfo *exception)
{
  char
    **coders;

  register const CoderInfo
    *p;

  register long
    i;

  /*
    Allocate coder list.
  */
  assert(pattern != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  assert(number_coders != (unsigned long *) NULL);
  *number_coders=0;
  p=GetCoderInfo("*",exception);
  if (p == (const CoderInfo *) NULL)
    return((char **) NULL);
  coders=(char **) AcquireMagickMemory((size_t)
    (GetNumberOfNodesInSplayTree(coder_list)+1)*sizeof(*coders));
  if (coders == (char **) NULL)
    return((char **) NULL);
  /*
    Generate coder list.
  */
  AcquireSemaphoreInfo(&coder_semaphore);
  ResetSplayTreeIterator(coder_list);
  p=(const CoderInfo *) GetNextValueInSplayTree(coder_list);
  for (i=0; p != (const CoderInfo *) NULL; )
  {
    if ((p->stealth == MagickFalse) &&
        (GlobExpression(p->name,pattern) != MagickFalse))
      coders[i++]=ConstantString(AcquireString(p->name));
    p=(const CoderInfo *) GetNextValueInSplayTree(coder_list);
  }
  RelinquishSemaphoreInfo(coder_semaphore);
  qsort((void *) coders,(size_t) i,sizeof(*coders),CoderCompare);
  coders[i]=(char *) NULL;
  *number_coders=(unsigned long) i;
  return(coders);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I n i t i a l i z e C o d e r L i s t                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InitializeCoderList() initializes the coder list.
%
%  The format of the InitializeCoderList method is:
%
%      MagickBooleanType InitializeCoderList(ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
static MagickBooleanType InitializeCoderList(ExceptionInfo *exception)
{
  if ((coder_list == (SplayTreeInfo *) NULL) &&
      (instantiate_coder == MagickFalse))
    {
      AcquireSemaphoreInfo(&coder_semaphore);
      if ((coder_list == (SplayTreeInfo *) NULL) &&
          (instantiate_coder == MagickFalse))
        {
          (void) LoadCoderLists(MagickCoderFilename,exception);
          instantiate_coder=MagickTrue;
        }
      RelinquishSemaphoreInfo(coder_semaphore);
    }
  return(coder_list != (SplayTreeInfo *) NULL ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L i s t C o d e r I n f o                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListCoderInfo() lists the coder info to a file.
%
%  The format of the ListCoderInfo coder is:
%
%      MagickBooleanType ListCoderInfo(FILE *file,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o file:  An pointer to a FILE.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType ListCoderInfo(FILE *file,
  ExceptionInfo *exception)
{
  const char
    *path;

  const CoderInfo
    **coder_info;

  long
    j;

  register long
    i;

  unsigned long
    number_coders;

  if (file == (const FILE *) NULL)
    file=stdout;
  coder_info=GetCoderInfoList("*",&number_coders,exception);
  if (coder_info == (const CoderInfo **) NULL)
    return(MagickFalse);
  path=(const char *) NULL;
  for (i=0; i < (long) number_coders; i++)
  {
    if (coder_info[i]->stealth != MagickFalse)
      continue;
    if ((path == (const char *) NULL) ||
        (LocaleCompare(path,coder_info[i]->path) != 0))
      {
        if (coder_info[i]->path != (char *) NULL)
          (void) fprintf(file,"\nPath: %s\n\n",coder_info[i]->path);
        (void) fprintf(file,"Magick      Coder\n");
        (void) fprintf(file,"-------------------------------------------------"
          "------------------------------\n");
      }
    path=coder_info[i]->path;
    (void) fprintf(file,"%s",coder_info[i]->magick);
    for (j=(long) strlen(coder_info[i]->magick); j <= 11; j++)
      (void) fprintf(file," ");
    if (coder_info[i]->name != (char *) NULL)
      (void) fprintf(file,"%s",coder_info[i]->name);
    (void) fprintf(file,"\n");
  }
  coder_info=(const CoderInfo **) RelinquishMagickMemory((void *) coder_info);
  (void) fflush(file);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   L o a d C o d e r L i s t                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LoadCoderList() loads the coder configuration file which provides a
%  mapping between coder attributes and a coder name.
%
%  The format of the LoadCoderList coder is:
%
%      MagickBooleanType LoadCoderList(const char *xml,const char *filename,
%        const unsigned long depth,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o xml:  The coder list in XML format.
%
%    o filename:  The coder list filename.
%
%    o depth: depth of <include /> statements.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

static void *DestroyCoderNode(void *coder_info)
{
  register CoderInfo
    *p;

  p=(CoderInfo *) coder_info;
  if (p->path != (char *) NULL)
    p->path=(char *) RelinquishMagickMemory(p->path);
  if (p->name != (char *) NULL)
    p->name=(char *) RelinquishMagickMemory(p->name);
  if (p->magick != (char *) NULL)
    p->magick=(char *) RelinquishMagickMemory(p->magick);
  return(RelinquishMagickMemory(p));
}

static MagickBooleanType LoadCoderList(const char *xml,const char *filename,
  const unsigned long depth,ExceptionInfo *exception)
{
  char
    keyword[MaxTextExtent],
    *q,
    *token;

  CoderInfo
    *coder_info = (CoderInfo *) NULL;

  MagickBooleanType
    status;

  /*
    Load the coder map file.
  */
  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
    "Loading coder file \"%s\" ...",filename);
  if (xml == (const char *) NULL)
    return(MagickFalse);
  if (coder_list == (SplayTreeInfo *) NULL)
    {
      coder_list=NewSplayTree(CompareSplayTreeString,RelinquishMagickMemory,
        DestroyCoderNode);
      if (coder_list == (SplayTreeInfo *) NULL)
        {
          ThrowFileException(exception,ResourceLimitError,
            "MemoryAllocationFailed",filename);
          return(MagickFalse);
        }
    }
  status=MagickTrue;
  token=AcquireString(xml);
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

                  GetPathComponent(filename,HeadPath,path);
                  if (*path != '\0')
                    (void) ConcatenateMagickString(path,DirectorySeparator,
                      MaxTextExtent);
                  (void) ConcatenateMagickString(path,token,MaxTextExtent);
                  xml=FileToString(path,~0,exception);
                  if (LoadCoderList(xml,path,depth+1,exception) == MagickFalse)
                    status=MagickFalse;
                  xml=(char *) RelinquishMagickMemory(xml);
                }
            }
        }
        continue;
      }
    if (LocaleCompare(keyword,"<coder") == 0)
      {
        /*
          Allocate memory for the coder list.
        */
        coder_info=(CoderInfo *) AcquireMagickMemory(sizeof(*coder_info));
        if (coder_info == (CoderInfo *) NULL)
          ThrowMagickFatalException(ResourceLimitFatalError,
            "MemoryAllocationFailed",filename);
        (void) ResetMagickMemory(coder_info,0,sizeof(*coder_info));
        coder_info->path=ConstantString(AcquireString(filename));
        coder_info->signature=MagickSignature;
        continue;
      }
    if (coder_info == (CoderInfo *) NULL)
      continue;
    if (LocaleCompare(keyword,"/>") == 0)
      {
        status=AddValueToSplayTree(coder_list,
          ConstantString(AcquireString(coder_info->magick)),coder_info);
        if (status == MagickFalse)
          (void) ThrowMagickException(exception,GetMagickModule(),
            ResourceLimitError,"MemoryAllocationFailed","`%s'",
            coder_info->magick);
        coder_info=(CoderInfo *) NULL;
      }
    GetMagickToken(q,(char **) NULL,token);
    if (*token != '=')
      continue;
    GetMagickToken(q,&q,token);
    GetMagickToken(q,&q,token);
    switch (*keyword)
    {
      case 'M':
      case 'm':
      {
        if (LocaleCompare((char *) keyword,"magick") == 0)
          {
            coder_info->magick=ConstantString(AcquireString(token));
            break;
          }
        break;
      }
      case 'N':
      case 'n':
      {
        if (LocaleCompare((char *) keyword,"name") == 0)
          {
            coder_info->name=ConstantString(AcquireString(token));
            break;
          }
        break;
      }
      case 'S':
      case 's':
      {
        if (LocaleCompare((char *) keyword,"stealth") == 0)
          {
            coder_info->stealth=(MagickBooleanType)
              (LocaleCompare(token,"True") == 0);
            break;
          }
        break;
      }
      default:
        break;
    }
  }
  token=(char *) RelinquishMagickMemory(token);
  if (coder_list == (SplayTreeInfo *) NULL)
    return(MagickFalse);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L o a d C o d e r L i s t s                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LoadCoderLists() loads one or more coder configuration file which
%  provides a mapping between coder attributes and a coder name.
%
%  The format of the LoadCoderLists coder is:
%
%      MagickBooleanType LoadCoderLists(const char *filename,
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
static MagickBooleanType LoadCoderLists(const char *filename,
  ExceptionInfo *exception)
{
#if defined(UseEmbeddableMagick)
  return(LoadCoderList(CoderMap,"built-in",0,exception));
#else
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
    status|=LoadCoderList((const char *) option->datum,option->path,0,
      exception);
    option=(const StringInfo *) GetNextValueInLinkedList(options);
  }
  options=DestroyConfigureOptions(options);
  if ((coder_list == (SplayTreeInfo *) NULL) || 
      (GetNumberOfNodesInSplayTree(coder_list) == 0))
    status|=LoadCoderList(CoderMap,"built-in",0,exception);
  else
    (void) SetExceptionInfo(exception,UndefinedException);
  return(status != 0 ? MagickTrue : MagickFalse);
#endif
}
