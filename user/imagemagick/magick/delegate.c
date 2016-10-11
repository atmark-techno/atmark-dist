/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%           DDDD   EEEEE  L      EEEEE   GGGG   AAA   TTTTT  EEEEE            %
%           D   D  E      L      E      G      A   A    T    E                %
%           D   D  EEE    L      EEE    G  GG  AAAAA    T    EEE              %
%           D   D  E      L      E      G   G  A   A    T    E                %
%           DDDD   EEEEE  LLLLL  EEEEE   GGG   A   A    T    EEEEE            %
%                                                                             %
%                                                                             %
%                   Methods to Read/Write/Invoke Delegates                    %
%                                                                             %
%                             Software Design                                 %
%                               John Cristy                                   %
%                               October 1998                                  %
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
%  The Delegates methods associate a set of commands with a particular
%  image format.  ImageMagick uses delegates for formats it does not handle
%  directly.
%
%  Thanks to Bob Friesenhahn for the initial inspiration and design of the
%  delegates methods.
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
#include "magick/constitute.h"
#include "magick/delegate.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/hashmap.h"
#include "magick/list.h"
#include "magick/memory_.h"
#include "magick/resource_.h"
#include "magick/semaphore.h"
#include "magick/string_.h"
#include "magick/token.h"
#include "magick/utility.h"

/*
  Define declarations.
*/
#define DelegateFilename  "delegates.xml"

/*
  Declare delegate map.
*/
static const char
  *DelegateMap = (const char *)
    "<?xml version=\"1.0\"?>"
    "<delegatemap>"
    "  <delegate decode=\"autotrace\" stealth=\"True\" command='\"autotrace\" -output-format svg -output-file \"%o\" \"%i\"' />"
    "  <delegate decode=\"browse\" stealth=\"True\" spawn=\"True\" command='\"htmlview\" http://www.imagemagick.org/'  />"
    "  <delegate decode=\"cgm\" command='\"ralcgm\" -d ps -oC < \"%i\" > \"%o\" 2>/dev/null' />"
    "  <delegate decode=\"crw\" command='\"dcraw\" -3 -w -c \"%i\" > \"%o\"' />"
    "  <delegate decode=\"dcr\" command='\"dcraw\" -3 -w -c \"%i\" > \"%o\"' />"
    "  <delegate decode=\"mrw\" command='\"dcraw\" -3 -w -c \"%i\" > \"%o\"' />"
    "  <delegate decode=\"nef\" command='\"dcraw\" -3 -w -c \"%i\" > \"%o\"' />"
    "  <delegate decode=\"orf\" command='\"dcraw\" -3 -w -c \"%i\" > \"%o\"' />"
    "  <delegate decode=\"raf\" command='\"dcraw\" -3 -w -c \"%i\" > \"%o\"' />"
    "  <delegate decode=\"x3f\" command='\"dcraw\" -3 -w -c \"%i\" > \"%o\"' />"
    "  <delegate decode=\"dvi\" command='\"dvips\" -q -o \"%o\" \"%i\"' />"
    "  <delegate decode=\"edit\" stealth=\"True\" command='\"xterm\" -title \"Edit Image Comment\" -e vi \"%o\"' />"
    "  <delegate decode=\"emf\" command='\"wmf2eps\" -o \"%o\" \"%i\"' />"
    "  <delegate decode=\"eps\" encode=\"pdf\" mode=\"bi\" command='\"gs\" -q -dBATCH -dSAFER -dMaxBitmap=500000000 -dNOPAUSE -dAlignToPixels=0 -sDEVICE=\"pdfwrite\" -sOutputFile=\"%o\" -f\"%i\"' />"
    "  <delegate decode=\"eps\" encode=\"ps\" mode=\"bi\" command='\"gs\" -q -dBATCH -dSAFER -dMaxBitmap=500000000 -dNOPAUSE -dAlignToPixels=0 -sDEVICE=\"pswrite\" -sOutputFile=\"%o\" -f\"%i\"' />"
    "  <delegate decode=\"fig\" command='\"fig2dev\" -L ps \"%i\" \"%o\"' />"
    "  <delegate decode=\"gplt\" command='\"echo\" \"set size 1.25,0.62"
    "    set terminal postscript portrait color solid; set output \"%o\"; load \"%i\"\" > \"%u\";\"gnuplot\" \"%u\"' />"
    "  <delegate decode=\"gs-color\" stealth=\"True\" command='\"gs\" -q -dBATCH -dSAFER -dMaxBitmap=500000000 -dNOPAUSE -dAlignToPixels=0 \"-sDEVICE=pnmraw\" -dTextAlphaBits=%u -dGraphicsAlphaBits=%u \"-g%s\" \"-r%s\" %s \"-sOutputFile=%s\" \"-f%s\" \"-f%s\"' />"
    "  <delegate decode=\"gs-cmyk\" stealth=\"True\" command='\"gs\" -q -dBATCH -dSAFER -dMaxBitmap=500000000 -dNOPAUSE -dAlignToPixels=0 \"-sDEVICE=bmpsep8\" -dTextAlphaBits=%u -dGraphicsAlphaBits=%u \"-g%s\" \"-r%s\" %s \"-sOutputFile=%s\" \"-f%s\" \"-f%s\"' />"
    "  <delegate decode=\"gs-mono\" stealth=\"True\" command='\"gs\" -q -dBATCH -dSAFER -dMaxBitmap=500000000 -dNOPAUSE -dAlignToPixels=0 \"-sDEVICE=pbmraw\" -dTextAlphaBits=%u -dGraphicsAlphaBits=%u \"-g%s\" \"-r%s\" %s \"-sOutputFile=%s\" \"-f%s\" \"-f%s\"' />"
    "  <delegate decode=\"hpg\" command='\"hp2xx\" -q -m eps -f `basename \"%o\"` \"%i\""
    "    mv -f `basename \"%o\"` \"%o\"' />"
    "  <delegate decode=\"hpgl\" command='if [ -e hp2xx -o -e /usr/bin/hp2xx ]; then"
    "    hp2xx -q -m eps -f `basename \"%o\"` \"%i\""
    "    mv -f `basename \"%o\"` \"%o"
    "  else"
    "    echo \"You need to install hp2xx to use HPGL files with ImageMagick.\""
    "    exit 1"
    "  fi' />"
    "  <delegate decode=\"htm\" command='\"html2ps\" -U -o \"%o\" \"%i\"' />"
    "  <delegate decode=\"html\" command='\"html2ps\" -U -o \"%o\" \"%i\"' />"
    "  <delegate decode=\"https\" command='\"@WWWDecodeDelegateDefault@\" -q -O \"%o\" \"https:%i\"' />"
    "  <delegate decode=\"ilbm\" command='\"ilbmtoppm\" \"%i\" > \"%o\"' />"
    "  <delegate decode=\"man\" command='\"groff\" -man -Tps \"%i\" > \"%o\"' />"
    "  <delegate decode=\"mpeg-decode\" stealth=\"True\" command='\"mpeg2decode\" -q -b \"%i\" -f -r -o3 \"%u%%d\"' />"
    "  <delegate encode=\"mpeg-encode\" stealth=\"True\" command='\"mpeg2encode\" \"%i\" \"%o\"' />"
    "  <delegate decode=\"pcl-color\" stealth=\"True\" command='\"pcl6\" -dBATCH -dSAFER -dMaxBitmap=500000000 -dNOPAUSE -dAlignToPixels=0 \"-sDEVICE=ppmraw\" -dTextAlphaBits=%u -dGraphicsAlphaBits=%u \"-g%s\" \"-r%s\" %s \"-sOutputFile=%s\" \"%s\"' />"
    "  <delegate decode=\"pcl-cmyk\" stealth=\"True\" command='\"pcl6\" -dBATCH -dSAFER -dMaxBitmap=500000000 -dNOPAUSE -dAlignToPixels=0 \"-sDEVICE=@PCLCMYKDevice@\" -dTextAlphaBits=%u -dGraphicsAlphaBits=%u \"-g%s\" \"-r%s\" %s \"-sOutputFile=%s\" \"%s\"' />"
    "  <delegate decode=\"pcl-mono\" stealth=\"True\" command='\"pcl6\" -dBATCH -dSAFER -dMaxBitmap=500000000 -dNOPAUSE -dAlignToPixels=0 \"-sDEVICE=pbmraw\" -dTextAlphaBits=%u -dGraphicsAlphaBits=%u \"-g%s\" \"-r%s\" %s \"-sOutputFile=%s\" \"%s\"' />"
    "  <delegate decode=\"pdf\" encode=\"eps\" mode=\"bi\" command='\"gs\" -q -dBATCH -dSAFER -dMaxBitmap=500000000 -dNOPAUSE -dAlignToPixels=0 -sDEVICE=\"epswrite\" -sOutputFile=\"%o\" -f\"%i\"' />"
    "  <delegate decode=\"pdf\" encode=\"ps\" mode=\"bi\" command='\"gs\" -q -dBATCH -dSAFER -dMaxBitmap=500000000 -dNOPAUSE -dAlignToPixels=0 -sDEVICE=\"pswrite\" -sOutputFile=\"%o\" -f\"%i\"' />"
    "  <delegate decode=\"pnm\" encode=\"ilbm\" mode=\"encode\" command='\"ppmtoilbm\" -24if \"%i\" > \"%o\"' />"
    "  <delegate decode=\"pnm\" encode=\"launch\" mode=\"encode\" command='\"gimp\" \"%i\"' />"
    "  <delegate decode=\"miff\" encode=\"win\" mode=\"encode\" command='\"display\" -immutable \"%i\"' />"
    "  <delegate decode=\"pov\" command='\"povray\" \"+i\"%i\"\" +o\"%o\" +fn%q +w%w +h%h +a -q9 -kfi\"%s\" -kff\"%n\""
    "    \"convert\" -concatenate \"%o*.png\" \"%o\"' />"
    "  <delegate decode=\"ps\" encode=\"eps\" mode=\"bi\" command='\"gs\" -q -dBATCH -dSAFER -dMaxBitmap=500000000 -dNOPAUSE -dAlignToPixels=0 -sDEVICE=\"epswrite\" -sOutputFile=\"%o\" -f\"%i\"' />"
    "  <delegate decode=\"ps\" encode=\"pdf\" mode=\"bi\" command='\"gs\" -q -dBATCH -dSAFER -dMaxBitmap=500000000 -dNOPAUSE -dAlignToPixels=0 -sDEVICE=\"pdfwrite\" -sOutputFile=\"%o\" -f\"%i\"' />"
    "  <delegate decode=\"ps\" encode=\"print\" mode=\"encode\" command='lpr \"%i\"' />"
    "  <delegate decode=\"rad\" command='\"ra_ppm\" -g 1.0 \"%i\" \"%o\"' />"
    "  <delegate decode=\"rgba\" encode=\"rle\" mode=\"encode\" command='\"modify\" -flip -size %wx%h \"rgba:%i\""
    "    \"rawtorle\" -w %w -h %h -n 4 -o \"%o\" \"%i\"' />"
    "  <delegate decode=\"scan\" command='\"scanimage\" -d \"%i\" > \"%o\"' />"
    "  <delegate decode=\"shtml\" command='\"html2ps\" -U -o \"%o\" \"%i\"' />"
    "  <delegate decode=\"txt\" encode=\"ps\" mode=\"bi\" command='\"enscript\" -o \"%o\" \"%i\"' />"
    "  <delegate decode=\"wmf\" command='\"wmf2eps\" -o \"%o\" \"%i\"' />"
    "  <delegate encode=\"show\" stealth=\"True\" spawn=\"True\" command='\"display\" -immutable -delay 0 -window_group %g -title \"%l of %f\" \"tmp:%i\"' />"
    "</delegatemap>";

/*
  Global declaractions.
*/
static LinkedListInfo
  *delegate_list = (LinkedListInfo *) NULL;

static SemaphoreInfo
  *delegate_semaphore = (SemaphoreInfo *) NULL;

static volatile MagickBooleanType
  instantiate_delegate = MagickFalse;

/*
  Forward declaractions.
*/
static MagickBooleanType
  InitializeDelegateList(ExceptionInfo *),
  LoadDelegateLists(const char *,ExceptionInfo *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   D e s t r o y D e l e g a t e L i s t                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyDelegateList() deallocates memory associated with the delegates list.
%
%  The format of the DestroyDelegateList method is:
%
%      DestroyDelegateList(void)
%
*/

static void *DestroyDelegate(void *delegate_info)
{
  register DelegateInfo
    *p;

  p=(DelegateInfo *) delegate_info;
  if (p->path != (char *) NULL)
    p->path=(char *) RelinquishMagickMemory(p->path);
  if (p->decode != (char *) NULL)
    p->decode=(char *) RelinquishMagickMemory(p->decode);
  if (p->encode != (char *) NULL)
    p->encode=(char *) RelinquishMagickMemory(p->encode);
  if (p->commands != (char *) NULL)
    p->commands=(char *) RelinquishMagickMemory(p->commands);
  p=(DelegateInfo *) RelinquishMagickMemory(p);
  return((void *) NULL);
}


MagickExport void DestroyDelegateList(void)
{
  AcquireSemaphoreInfo(&delegate_semaphore);
  if (delegate_list != (LinkedListInfo *) NULL)
    delegate_list=DestroyLinkedList(delegate_list,DestroyDelegate);
  instantiate_delegate=MagickFalse;
  RelinquishSemaphoreInfo(delegate_semaphore);
  delegate_semaphore=DestroySemaphoreInfo(delegate_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t D e l e g a t e C o m m a n d                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetDelegateCommand() replaces any embedded formatting characters with the
%  appropriate image attribute and returns the resulting command.
%
%  The format of the GetDelegateCommand method is:
%
%      char *GetDelegateCommand(const ImageInfo *image_info,Image *image,
%        const char *decode,const char *encode,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o command: Method GetDelegateCommand returns the command associated
%      with specified delegate tag.
%
%    o image_info: The image info.
%
%    o image: The image.
%
%    o decode: Specifies the decode delegate we are searching for as a
%      character string.
%
%    o encode: Specifies the encode delegate we are searching for as a
%      character string.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport char *GetDelegateCommand(const ImageInfo *image_info,Image *image,
  const char *decode,const char *encode,ExceptionInfo *exception)
{
  char
    *command,
    **commands;

  const DelegateInfo
    *delegate_info;

  register long
    i;

  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  delegate_info=GetDelegateInfo(decode,encode,exception);
  if (delegate_info == (const DelegateInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),DelegateError,
        "NoTagFound","`%s'",decode ? decode : encode);
      return((char *) NULL);
    }
  commands=StringToList(delegate_info->commands);
  if (commands == (char **) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",
        decode ? decode : encode);
      return((char *) NULL);
    }
  command=TranslateText(image_info,image,commands[0]);
  if (command == (char *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),ResourceLimitError,
      "MemoryAllocationFailed","`%s'",commands[0]);
  /*
    Free resources.
  */
  for (i=0; commands[i] != (char *) NULL; i++)
    commands[i]=(char *) RelinquishMagickMemory(commands[i]);
  commands=(char **) RelinquishMagickMemory(commands);
  return(command);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t D e l e g a t e C o m m a n d s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetDelegateCommands() returns the commands associated with a delegate.
%
%  The format of the GetDelegateCommands method is:
%
%      const char *GetDelegateCommands(const DelegateInfo *delegate_info)
%
%  A description of each parameter follows:
%
%    o delegate_info:  The delegate info.
%
*/
MagickExport const char *GetDelegateCommands(const DelegateInfo *delegate_info)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(delegate_info != (DelegateInfo *) NULL);
  assert(delegate_info->signature == MagickSignature);
  return(delegate_info->commands);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t D e l e g a t e I n f o                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetDelegateInfo() returns any delegates associated with the specified tag.
%
%  The format of the GetDelegateInfo method is:
%
%      const DelegateInfo *GetDelegateInfo(const char *decode,
%        const char *encode,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o decode: Specifies the decode delegate we are searching for as a
%      character string.
%
%    o encode: Specifies the encode delegate we are searching for as a
%      character string.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport const DelegateInfo *GetDelegateInfo(const char *decode,
  const char *encode,ExceptionInfo *exception)
{
  register const DelegateInfo
    *p;

  assert(exception != (ExceptionInfo *) NULL);
  if ((delegate_list == (LinkedListInfo *) NULL) ||
      (instantiate_delegate == MagickFalse))
    if (InitializeDelegateList(exception) == MagickFalse)
      return((const DelegateInfo *) NULL);
  if ((delegate_list == (LinkedListInfo *) NULL) ||
      (IsLinkedListEmpty(delegate_list) != MagickFalse))
    return((const DelegateInfo *) NULL);
  if ((LocaleCompare(decode,"*") == 0) && (LocaleCompare(encode,"*") == 0))
    return((const DelegateInfo *) GetValueFromLinkedList(delegate_list,0));
  /*
    Search for named delegate.
  */
  AcquireSemaphoreInfo(&delegate_semaphore);
  ResetLinkedListIterator(delegate_list);
  p=(const DelegateInfo *) GetNextValueInLinkedList(delegate_list);
  while (p != (const DelegateInfo *) NULL)
  {
    if (p->mode > 0)
      {
        if (LocaleCompare(p->decode,decode) == 0)
          break;
        p=(const DelegateInfo *) GetNextValueInLinkedList(delegate_list);
        continue;
      }
    if (p->mode < 0)
      {
        if (LocaleCompare(p->encode,encode) == 0)
          break;
        p=(const DelegateInfo *) GetNextValueInLinkedList(delegate_list);
        continue;
      }
    if (LocaleCompare(decode,p->decode) == 0)
      if (LocaleCompare(encode,p->encode) == 0)
        break;
    if (LocaleCompare(decode,"*") == 0)
      if (LocaleCompare(encode,p->encode) == 0)
        break;
    if (LocaleCompare(decode,p->decode) == 0)
      if (LocaleCompare(encode,"*") == 0)
        break;
    p=(const DelegateInfo *) GetNextValueInLinkedList(delegate_list);
  }
  RelinquishSemaphoreInfo(delegate_semaphore);
  return( p);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t D e l e g a t e I n f o L i s t                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetDelegateInfoList() returns any delegates that match the specified pattern.
%
%  The delegate of the GetDelegateInfoList function is:
%
%      const DelegateInfo **GetDelegateInfoList(const char *pattern,
%        unsigned long *number_delegates,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_delegates:  This integer returns the number of delegates in the
%      list.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int DelegateInfoCompare(const void *x,const void *y)
{
  const DelegateInfo
    **p,
    **q;

  p=(const DelegateInfo **) x,
  q=(const DelegateInfo **) y;
  if (LocaleCompare((*p)->path,(*q)->path) == 0)
    {
      if ((*p)->decode == (char *) NULL)
        if (((*p)->encode != (char *) NULL) &&
            ((*q)->encode != (char *) NULL))
          return(strcmp((*p)->encode,(*q)->encode));
      if (((*p)->decode != (char *) NULL) &&
          ((*q)->decode != (char *) NULL))
        return(strcmp((*p)->decode,(*q)->decode));
    }
  return(LocaleCompare((*p)->path,(*q)->path));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport const DelegateInfo **GetDelegateInfoList(const char *pattern,
  unsigned long *number_delegates,ExceptionInfo *exception)
{
  const DelegateInfo
    **delegates;

  register const DelegateInfo
    *p;

  register long
    i;

  /*
    Allocate delegate list.
  */
  assert(pattern != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  assert(number_delegates != (unsigned long *) NULL);
  *number_delegates=0;
  p=GetDelegateInfo("*","*",exception);
  if (p == (const DelegateInfo *) NULL)
    return((const DelegateInfo **) NULL);
  delegates=(const DelegateInfo **) AcquireMagickMemory((size_t)
    (GetNumberOfElementsInLinkedList(delegate_list)+1)*sizeof(*delegates));
  if (delegates == (const DelegateInfo **) NULL)
    return((const DelegateInfo **) NULL);
  /*
    Generate delegate list.
  */
  AcquireSemaphoreInfo(&delegate_semaphore);
  ResetLinkedListIterator(delegate_list);
  p=(const DelegateInfo *) GetNextValueInLinkedList(delegate_list);
  for (i=0; p != (const DelegateInfo *) NULL; )
  {
    if ((p->stealth == MagickFalse) &&
        ((GlobExpression(p->decode,pattern) != MagickFalse) ||
         (GlobExpression(p->encode,pattern) != MagickFalse)))
      delegates[i++]=p;
    p=(const DelegateInfo *) GetNextValueInLinkedList(delegate_list);
  }
  RelinquishSemaphoreInfo(delegate_semaphore);
  qsort((void *) delegates,(size_t) i,sizeof(*delegates),DelegateInfoCompare);
  delegates[i]=(DelegateInfo *) NULL;
  *number_delegates=(unsigned long) i;
  return(delegates);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t D e l e g a t e L i s t                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetDelegateList() returns any image format delegates that match the
%  specified  pattern.
%
%  The format of the GetDelegateList function is:
%
%      char **GetDelegateList(const char *pattern,
%        unsigned long *number_delegates,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_delegates:  This integer returns the number of delegates
%      in the list.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int DelegateCompare(const void *x,const void *y)
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

MagickExport char **GetDelegateList(const char *pattern,
  unsigned long *number_delegates,ExceptionInfo *exception)
{
  char
    **delegates;

  register const DelegateInfo
    *p;

  register long
    i;

  /*
    Allocate delegate list.
  */
  assert(pattern != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  assert(number_delegates != (unsigned long *) NULL);
  *number_delegates=0;
  p=GetDelegateInfo("*","*",exception);
  if (p == (const DelegateInfo *) NULL)
    return((char **) NULL);
  delegates=(char **) AcquireMagickMemory((size_t)
    (GetNumberOfElementsInLinkedList(delegate_list)+1)*sizeof(*delegates));
  if (delegates == (char **) NULL)
    return((char **) NULL);
  AcquireSemaphoreInfo(&delegate_semaphore);
  ResetLinkedListIterator(delegate_list);
  p=(const DelegateInfo *) GetNextValueInLinkedList(delegate_list);
  for (i=0; p != (const DelegateInfo *) NULL; )
  {
    if ((p->stealth == MagickFalse) &&
        (GlobExpression(p->decode,pattern) != MagickFalse))
      delegates[i++]=ConstantString(AcquireString(p->decode));
    if ((p->stealth == MagickFalse) &&
        (GlobExpression(p->encode,pattern) != MagickFalse))
      delegates[i++]=ConstantString(AcquireString(p->encode));
    p=(const DelegateInfo *) GetNextValueInLinkedList(delegate_list);
  }
  RelinquishSemaphoreInfo(delegate_semaphore);
  qsort((void *) delegates,(size_t) i,sizeof(*delegates),DelegateCompare);
  delegates[i]=(char *) NULL;
  *number_delegates=(unsigned long) i;
  return(delegates);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t D e l e g a t e M o d e                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetDelegateMode() returns the mode of the delegate.
%
%  The format of the GetDelegateMode method is:
%
%      long GetDelegateMode(const DelegateInfo *delegate_info)
%
%  A description of each parameter follows:
%
%    o delegate_info:  The delegate info.
%
*/
MagickExport long GetDelegateMode(const DelegateInfo *delegate_info)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(delegate_info != (DelegateInfo *) NULL);
  assert(delegate_info->signature == MagickSignature);
  return(delegate_info->mode);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I n i t i a l i z e D e l e g a t e L i s t                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InitializeDelegateList() initializes the delegate list.
%
%  The format of the InitializeDelegateList method is:
%
%      MagickBooleanType InitializeDelegateList(ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
static MagickBooleanType InitializeDelegateList(ExceptionInfo *exception)
{
  if ((delegate_list == (LinkedListInfo *) NULL) &&
      (instantiate_delegate == MagickFalse))
    {
      AcquireSemaphoreInfo(&delegate_semaphore);
      if ((delegate_list == (LinkedListInfo *) NULL) &&
          (instantiate_delegate == MagickFalse))
        {
          (void) LoadDelegateLists(DelegateFilename,exception);
          instantiate_delegate=MagickTrue;
        }
      RelinquishSemaphoreInfo(delegate_semaphore);
    }
  return(delegate_list != (LinkedListInfo *) NULL ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n v o k e D e l e g a t e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InvokeDelegate replaces any embedded formatting characters with the
%  appropriate image attribute and executes the resulting command.  MagickFalse
%  is returned if the commands execute with success otherwise MagickTrue.
%
%  The format of the InvokeDelegate method is:
%
%      MagickBooleanType InvokeDelegate(ImageInfo *image_info,Image *image,
%        const char *decode,const char *encode,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The imageInfo.
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType InvokeDelegate(ImageInfo *image_info,
  Image *image,const char *decode,const char *encode,ExceptionInfo *exception)
{
  char
    *command,
    **commands,
    filename[MaxTextExtent];

  const DelegateInfo
    *delegate_info;

  register long
    i;

  MagickBooleanType
    status,
    temporary;

  /*
    Get delegate.
  */
  assert(image_info != (ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  temporary=(MagickBooleanType) (*image->filename == '\0');
  if (temporary != MagickFalse)
    if (AcquireUniqueFilename(image->filename) == MagickFalse)
      {
        ThrowFileException(exception,FileOpenError,
          "UnableToCreateTemporaryFile",image->filename);
        return(MagickFalse);
      }
  (void) CopyMagickString(filename,image->filename,MaxTextExtent);
  delegate_info=GetDelegateInfo(decode,encode,exception);
  if (delegate_info == (DelegateInfo *) NULL)
    {
      if (temporary != MagickFalse)
        (void) RelinquishUniqueFileResource(image->filename);
      (void) ThrowMagickException(exception,GetMagickModule(),DelegateError,
        "NoTagFound","`%s'",decode ? decode : encode);
      return(MagickFalse);
    }
  if (*image_info->filename == '\0')
    {
      if (AcquireUniqueFilename(image_info->filename) == MagickFalse)
        {
          if (temporary != MagickFalse)
            (void) RelinquishUniqueFileResource(image->filename);
          ThrowFileException(exception,FileOpenError,
            "UnableToCreateTemporaryFile",image_info->filename);
          return(MagickFalse);
        }
      image_info->temporary=MagickTrue;
    }
  if (delegate_info->mode != 0)
    if (((decode != (const char *) NULL) &&
         (delegate_info->encode != (char *) NULL)) ||
        ((encode != (const char *) NULL) &&
         (delegate_info->decode != (char *) NULL)))
      {
        char
          filename[MaxTextExtent],
          *magick;

        ImageInfo
          *clone_info;

        register Image
          *p;

        /*
          Delegate requires a particular image format.
        */
        if (AcquireUniqueFilename(image_info->unique) == MagickFalse)
          {
            ThrowFileException(exception,FileOpenError,
              "UnableToCreateTemporaryFile",image_info->unique);
            return(MagickFalse);
          }
        if (AcquireUniqueFilename(image_info->zero) == MagickFalse)
          {
            (void) RelinquishUniqueFileResource(image_info->zero);
            ThrowFileException(exception,FileOpenError,
              "UnableToCreateTemporaryFile",image_info->zero);
            return(MagickFalse);
          }
        magick=TranslateText(image_info,image,decode != (char *) NULL ?
          delegate_info->encode : delegate_info->decode);
        if (magick == (char *) NULL)
          {
            (void) RelinquishUniqueFileResource(image_info->unique);
            (void) RelinquishUniqueFileResource(image_info->zero);
            if (temporary != MagickFalse)
              (void) RelinquishUniqueFileResource(image->filename);
            (void) ThrowMagickException(exception,GetMagickModule(),
              DelegateError,"DelegateFailed","`%s'",decode ? decode : encode);
            return(MagickFalse);
          }
        LocaleUpper(magick);
        clone_info=CloneImageInfo(image_info);
        (void) CopyMagickString((char *) clone_info->magick,magick,
          MaxTextExtent);
        (void) CopyMagickString(image->magick,magick,MaxTextExtent);
        magick=(char *) RelinquishMagickMemory(magick);
        (void) CopyMagickString(filename,image->filename,MaxTextExtent);
        (void) FormatMagickString(clone_info->filename,MaxTextExtent,"%s:",
          delegate_info->decode);
        (void) SetImageInfo(clone_info,MagickTrue,exception);
        (void) CopyMagickString(clone_info->filename,image_info->filename,
          MaxTextExtent);
        for (p=image; p != (Image *) NULL; p=GetNextImageInList(p))
        {
          (void) FormatMagickString(p->filename,MaxTextExtent,"%s:%s",
            delegate_info->decode,filename);
          status=WriteImage(clone_info,p);
          if (status == MagickFalse)
            {
              (void) RelinquishUniqueFileResource(image_info->unique);
              (void) RelinquishUniqueFileResource(image_info->zero);
              if (temporary != MagickFalse)
                (void) RelinquishUniqueFileResource(image->filename);
              clone_info=DestroyImageInfo(clone_info);
              (void) ThrowMagickException(exception,GetMagickModule(),
                DelegateError,"DelegateFailed","`%s'",decode ? decode : encode);
              return(MagickFalse);
            }
          if (clone_info->adjoin != MagickFalse)
            break;
        }
        (void) RelinquishUniqueFileResource(image_info->unique);
        (void) RelinquishUniqueFileResource(image_info->zero);
        clone_info=DestroyImageInfo(clone_info);
      }
  /*
    Invoke delegate.
  */
  (void) CopyMagickString(image->filename,filename,MaxTextExtent);
  commands=StringToList(delegate_info->commands);
  if (commands == (char **) NULL)
    {
      if (temporary != MagickFalse)
        (void) RelinquishUniqueFileResource(image->filename);
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",
        decode ? decode : encode);
      return(MagickFalse);
    }
  command=(char *) NULL;
  status=MagickFalse;
  for (i=0; commands[i] != (char *) NULL; i++)
  {
    status=MagickFalse;
    if (AcquireUniqueFilename(image_info->unique) == MagickFalse)
      {
        ThrowFileException(exception,FileOpenError,
          "UnableToCreateTemporaryFile",image_info->unique);
        return(MagickFalse);
      }
    if (AcquireUniqueFilename(image_info->zero) == MagickFalse)
      {
        (void) RelinquishUniqueFileResource(image_info->unique);
        ThrowFileException(exception,FileOpenError,
          "UnableToCreateTemporaryFile",image_info->zero);
        return(MagickFalse);
      }
    command=TranslateText(image_info,image,commands[i]);
    if (command == (char *) NULL)
      break;
    /*
      Execute delegate.
    */
    if (delegate_info->spawn != MagickFalse)
      (void) ConcatenateString(&command," &");
    status=SystemCommand(image_info->verbose,command) != 0 ?
      MagickTrue : MagickFalse;
    command=(char *) RelinquishMagickMemory(command);
    (void) RelinquishUniqueFileResource(image_info->unique);
    (void) RelinquishUniqueFileResource(image_info->zero);
    if (status != MagickFalse)
      {
        (void) ThrowMagickException(exception,GetMagickModule(),DelegateError,
          "DelegateFailed","`%s'",commands[i]);
        return(MagickFalse);
      }
    commands[i]=(char *) RelinquishMagickMemory(commands[i]);
  }
  /*
    Free resources.
  */
  for ( ; commands[i] != (char *) NULL; i++)
    commands[i]=(char *) RelinquishMagickMemory(commands[i]);
  commands=(char **) RelinquishMagickMemory(commands);
  if (temporary != MagickFalse)
    (void) RelinquishUniqueFileResource(image->filename);
  return((MagickBooleanType) (status == MagickFalse));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   I n v o k e P o s t s r i p t D e l e g a t e                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InvokePostscriptDelegate() executes the postscript interpreter with the
%  specified command.
%
%  The format of the InvokePostscriptDelegate method is:
%
%      MagickBooleanType InvokePostscriptDelegate(
%        const MagickBooleanType verbose,const char *command)
%
%  A description of each parameter follows:
%
%    o verbose: A value other than zero displays the command prior to
%      executing it.
%
%    o command: The address of a character string containing the command to
%      execute.
%
%
*/
MagickExport MagickBooleanType InvokePostscriptDelegate(
  const MagickBooleanType verbose,const char *command)
{
#if defined(HasGS) || defined(__WINDOWS__)
  char
    **argv;

  const GhostscriptVectors
    *gs_func;

  gs_main_instance
    *interpreter;

  int
    argc,
    code,
    status;

  register long
    i;

#if defined(__WINDOWS__)
  gs_func=NTGhostscriptDLLVectors();
#else
  GhostscriptVectors
    gs_func_struct;

  gs_func=(&gs_func_struct);
  gs_func_struct.exit=gsapi_exit;
  gs_func_struct.init_with_args=gsapi_init_with_args;
  gs_func_struct.new_instance=gsapi_new_instance;
  gs_func_struct.run_string=gsapi_run_string;
  gs_func_struct.delete_instance=gsapi_delete_instance;
#endif
  if (gs_func == (GhostscriptVectors *) NULL)
    return(SystemCommand(verbose,command));
  if (verbose != MagickFalse)
    {
      (void) fputs("[ghostscript library]",stdout);
      (void) fputs(strchr(command,' '),stdout);
    }
  status=(gs_func->new_instance)(&interpreter,(void *) NULL);
  if (status < 0)
    return(MagickFalse);
  argv=StringToArgv(command,&argc);
  status=(gs_func->init_with_args)(interpreter,argc-1,argv+1);
  if (status == 0)
    status=(gs_func->run_string)
      (interpreter,"systemdict /start get exec\n",0,&code);
  (gs_func->exit)(interpreter);
  (gs_func->delete_instance)(interpreter);
#if defined(__WINDOWS__)
  NTGhostscriptUnLoadDLL();
#endif
  for (i=0; i < (long) argc; i++)
    argv[i]=(char *) RelinquishMagickMemory(argv[i]);
  argv=(char **) RelinquishMagickMemory(argv);
  if ((status == 0) || (status <= -100))
    {
      (void) LogMagickEvent(CoderEvent,GetMagickModule(),
        "Ghostscript returns status %d, exit code %d",status,code);
      return(MagickFalse);
    }
  return(MagickTrue);
#else
  return(SystemCommand(verbose,command) != 0 ? MagickTrue : MagickFalse);
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L i s t D e l e g a t e I n f o                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListDelegateInfo() lists the image formats to a file.
%
%  The format of the ListDelegateInfo method is:
%
%      MagickBooleanType ListDelegateInfo(FILE *file,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o file:  An pointer to a FILE.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType ListDelegateInfo(FILE *file,
  ExceptionInfo *exception)
{
  const DelegateInfo
    **delegate_info;

  char
    **commands,
    delegate[MaxTextExtent];

  const char
    *path;

  long
    j;

  register long
    i;

  unsigned long
    number_delegates;

  if (file == (const FILE *) NULL)
    file=stdout;
  delegate_info=GetDelegateInfoList("*",&number_delegates,exception);
  if (delegate_info == (const DelegateInfo **) NULL)
    return(MagickFalse);
  path=(const char *) NULL;
  for (i=0; i < (long) number_delegates; i++)
  {
    if (delegate_info[i]->stealth != MagickFalse)
      continue;
    if ((path == (const char *) NULL) ||
        (LocaleCompare(path,delegate_info[i]->path) != 0))
      {
        if (delegate_info[i]->path != (char *) NULL)
          (void) fprintf(file,"\nPath: %s\n\n",delegate_info[i]->path);
        (void) fprintf(file,"Delegate             Command\n");
        (void) fprintf(file,"-------------------------------------------------"
          "------------------------------\n");
      }
    path=delegate_info[i]->path;
    *delegate='\0';
    if (delegate_info[i]->encode != (char *) NULL)
      (void) CopyMagickString(delegate,delegate_info[i]->encode,MaxTextExtent);
    (void) ConcatenateMagickString(delegate,"        ",MaxTextExtent);
    delegate[8]='\0';
    commands=StringToList(delegate_info[i]->commands);
    if (commands == (char **) NULL)
      continue;
    (void) fprintf(file,"%8s%c=%c%s  ",delegate_info[i]->decode ?
      delegate_info[i]->decode : "",delegate_info[i]->mode <= 0 ? '<' : ' ',
      delegate_info[i]->mode >= 0 ? '>' : ' ',delegate);
    StripString(commands[0]);
    (void) fprintf(file,"%s\n",commands[0]);
    for (j=1; commands[j] != (char *) NULL; j++)
    {
      StripString(commands[j]);
      (void) fprintf(file,"                     %s\n",commands[j]);
    }
    for (j=0; commands[j] != (char *) NULL; j++)
      commands[j]=(char *) RelinquishMagickMemory(commands[j]);
    commands=(char **) RelinquishMagickMemory(commands);
  }
  (void) fflush(file);
  delegate_info=(const DelegateInfo **)
    RelinquishMagickMemory((void *) delegate_info);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   L o a d D e l e g a t e L i s t                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LoadDelegateList() loads the delegate configuration file which provides a
%  mapping between delegate attributes and a delegate name.
%
%  The format of the LoadDelegateList method is:
%
%      MagickBooleanType LoadDelegateList(const char *xml,const char *filename,
%        const unsigned long depth,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o xml:  The delegate list in XML format.
%
%    o filename:  The delegate list filename.
%
%    o depth: depth of <include /> statements.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
static MagickBooleanType LoadDelegateList(const char *xml,const char *filename,
  const unsigned long depth,ExceptionInfo *exception)
{
  char
    keyword[MaxTextExtent],
    *q,
    *token;

  DelegateInfo
    *delegate_info = (DelegateInfo *) NULL;

  MagickStatusType
    status;

  /*
    Load the delegate map file.
  */
  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
    "Loading delegate file \"%s\" ...",filename);
  if (xml == (const char *) NULL)
    return(MagickFalse);
  if (delegate_list == (LinkedListInfo *) NULL)
    {
      delegate_list=NewLinkedList(0);
      if (delegate_list == (LinkedListInfo *) NULL)
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
                  ConfigureError,"IncludeElementNestedTooDeeply","`%s'",token);
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
                  status|=LoadDelegateList(xml,path,depth+1,exception);
                  xml=(char *) RelinquishMagickMemory(xml);
                }
            }
        }
        continue;
      }
    if (LocaleCompare(keyword,"<delegate") == 0)
      {
        /*
          Allocate memory for the delegate list.
        */
        delegate_info=(DelegateInfo *)
          AcquireMagickMemory(sizeof(*delegate_info));
        if (delegate_info == (DelegateInfo *) NULL)
          ThrowMagickFatalException(ResourceLimitFatalError,
            "MemoryAllocationFailed",filename);
        (void) ResetMagickMemory(delegate_info,0,sizeof(*delegate_info));
        delegate_info->path=ConstantString(AcquireString(filename));
        delegate_info->signature=MagickSignature;
        continue;
      }
    if (delegate_info == (DelegateInfo *) NULL)
      continue;
    if (LocaleCompare(keyword,"/>") == 0)
      {
        status=AppendValueToLinkedList(delegate_list,delegate_info);
        if (status == MagickFalse)
          (void) ThrowMagickException(exception,GetMagickModule(),
            ResourceLimitError,"MemoryAllocationFailed","`%s'",
            delegate_info->commands);
        delegate_info=(DelegateInfo *) NULL;
      }
    GetMagickToken(q,(char **) NULL,token);
    if (*token != '=')
      continue;
    GetMagickToken(q,&q,token);
    GetMagickToken(q,&q,token);
    switch (*keyword)
    {
      case 'C':
      case 'c':
      {
        if (LocaleCompare((char *) keyword,"command") == 0)
          {
            delegate_info->commands=ConstantString(AcquireString(token));
#if defined(__WINDOWS__)
            if (strchr(delegate_info->commands,'@') != (char *) NULL)
              {
                char
                  path[MaxTextExtent];

                NTGhostscriptEXE(path,MaxTextExtent);
                SubstituteString((char **) &delegate_info->commands,
                  "@PSDelegate@",path);
              }
#endif
            break;
          }
        break;
      }
      case 'D':
      case 'd':
      {
        if (LocaleCompare((char *) keyword,"decode") == 0)
          {
            delegate_info->decode=ConstantString(AcquireString(token));
            delegate_info->mode=1;
            break;
          }
        break;
      }
      case 'E':
      case 'e':
      {
        if (LocaleCompare((char *) keyword,"encode") == 0)
          {
            delegate_info->encode=ConstantString(AcquireString(token));
            delegate_info->mode=(-1);
            break;
          }
        break;
      }
      case 'M':
      case 'm':
      {
        if (LocaleCompare((char *) keyword,"mode") == 0)
          {
            delegate_info->mode=1;
            if (LocaleCompare(token,"bi") == 0)
              delegate_info->mode=0;
            else
              if (LocaleCompare(token,"encode") == 0)
                delegate_info->mode=(-1);
            break;
          }
        break;
      }
      case 'S':
      case 's':
      {
        if (LocaleCompare((char *) keyword,"spawn") == 0)
          {
            delegate_info->spawn=(MagickBooleanType)
              (LocaleCompare(token,"True") == 0);
            break;
          }
        if (LocaleCompare((char *) keyword,"stealth") == 0)
          {
            delegate_info->stealth=(MagickBooleanType)
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
  if (delegate_list == (LinkedListInfo *) NULL)
    return(MagickFalse);
  return(status != 0 ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L o a d D e l e g a t e L i s t s                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LoadDelegateList() loads one or more delegate configuration file which
%  provides a mapping between delegate attributes and a delegate name.
%
%  The format of the LoadDelegateLists method is:
%
%      MagickBooleanType LoadDelegateLists(const char *filename,
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
static MagickBooleanType LoadDelegateLists(const char *filename,
  ExceptionInfo *exception)
{
#if defined(UseEmbeddableMagick)
  return(LoadDelegateList(DelegateMap,"built-in",0,exception));
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
    status|=LoadDelegateList((const char *) option->datum,option->path,0,
      exception);
    option=(const StringInfo *) GetNextValueInLinkedList(options);
  }
  options=DestroyConfigureOptions(options);
  if ((delegate_list == (LinkedListInfo *) NULL) || 
      (IsLinkedListEmpty(delegate_list) != MagickFalse))
    status|=LoadDelegateList(DelegateMap,"built-in",0,exception);
  else
    (void) SetExceptionInfo(exception,UndefinedException);
  return(status != 0 ? MagickTrue : MagickFalse);
#endif
}
