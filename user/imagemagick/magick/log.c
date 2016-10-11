/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                             L       OOO    GGGG                             %
%                             L      O   O  G                                 %
%                             L      O   O  G GG                              %
%                             L      O   O  G   G                             %
%                             LLLLL   OOO    GGG                              %
%                                                                             %
%                                                                             %
%                           Log ImageMagick Events                            %
%                                                                             %
%                               Software Design                               %
%                                 John Cristy                                 %
%                                September 2002                               %
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
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/hashmap.h"
#include "magick/log.h"
#include "magick/memory_.h"
#include "magick/option.h"
#include "magick/semaphore.h"
#include "magick/timer.h"
#include "magick/string_.h"
#include "magick/token.h"
#include "magick/utility.h"
#include "magick/version.h"

/*
  Define declarations.
*/
#define LogFilename  "log.xml"

/*
  Typedef declarations.
*/
typedef struct _EventInfo
{
  char
    *name;

  LogEventType
    event;
} EventInfo;

typedef struct _HandlerInfo
{
  const char
    *name;

  LogHandlerType
    handler;
} HandlerInfo;

/*
  Declare log map.
*/
static const HandlerInfo
  LogHandlers[] =
  {
    { "console", ConsoleHandler },
    { "file", FileHandler },
    { "none", NoHandler },
    { "stdout", StdoutHandler },
    { "stderr", StderrHandler },
    { (char *) NULL, UndefinedHandler }
  };

static const char
  *LogMap = (const char *)
    "<?xml version=\"1.0\"?>"
    "<logmap>"
    "  <log events=\"None\" />"
    "  <log output=\"console\" />"
    "  <log filename=\"Magick-%d.log\" />"
    "  <log generations=\"3\" />"
    "  <log limit=\"2000\" />"
    "  <log format=\"%t %r %u %v %d %c[%p]: %m/%f/%l/%d\n  %e\" />"
    "</logmap>";

/*
  Static declarations.
*/
static LinkedListInfo
  *log_list = (LinkedListInfo *) NULL;

static SemaphoreInfo
  *log_semaphore = (SemaphoreInfo *) NULL;

static volatile MagickBooleanType
  instantiate_log = MagickFalse;

/*
  Forward declarations.
*/
static LogHandlerType
  ParseLogHandlers(const char *);

static MagickBooleanType
  InitializeLogList(ExceptionInfo *),
  LoadLogLists(const char *,ExceptionInfo *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y L o g L i s t                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyLogList() deallocates memory associated with the log list.
%
%  The format of the DestroyLogList method is:
%
%      DestroyLogList(void)
%
%
*/

static void *DestroyLogElement(void *log_info)
{
  register LogInfo
    *p;

  p=(LogInfo *) log_info;
  if (p->file != (FILE *) NULL)
    if (LocaleCompare(p->format,"xml") == 0)
      {
        (void) fprintf(p->file,"</log>\n");
        (void) fclose(p->file);
      }
  if (p->filename != (char *) NULL)
    p->filename=(char *) RelinquishMagickMemory(p->filename);
  if (p->path != (char *) NULL)
    p->path=(char *) RelinquishMagickMemory(p->path);
  if (p->format != (char *) NULL)
    p->format=(char *) RelinquishMagickMemory(p->format);
  p=(LogInfo *) RelinquishMagickMemory(p);
  return((void *) NULL);
}

MagickExport void DestroyLogList(void)
{
  AcquireSemaphoreInfo(&log_semaphore);
  if (log_list != (LinkedListInfo *) NULL)
    log_list=DestroyLinkedList(log_list,DestroyLogElement);
  instantiate_log=MagickFalse;
  RelinquishSemaphoreInfo(log_semaphore);
  log_semaphore=DestroySemaphoreInfo(log_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t L o g I n f o                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetLogInfo() searches the log list for the specified name and if found
%  returns attributes for that log.
%
%  The format of the GetLogInfo method is:
%
%      const LogInfo *GetLogInfo(const char *name,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o name: The log name.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport const LogInfo *GetLogInfo(const char *name,
  ExceptionInfo *exception)
{
  register const LogInfo
    *p;

  assert(exception != (ExceptionInfo *) NULL);
  if ((log_list == (LinkedListInfo *) NULL) || (instantiate_log == MagickFalse))
    if (InitializeLogList(exception) == MagickFalse)
      return((const LogInfo *) NULL);
  if ((log_list == (LinkedListInfo *) NULL) ||
      (IsLinkedListEmpty(log_list) != MagickFalse))
    return((const LogInfo *) NULL);
  if ((name == (const char *) NULL) || (LocaleCompare(name,"*") == 0))
    return((const LogInfo *) GetValueFromLinkedList(log_list,0));
  /*
    Search for named log.
  */
  AcquireSemaphoreInfo(&log_semaphore);
  ResetLinkedListIterator(log_list);
  p=(const LogInfo *) GetNextValueInLinkedList(log_list);
  while (p != (const LogInfo *) NULL)
  {
    if (LocaleCompare(name,p->name) == 0)
      break;
    p=(const LogInfo *) GetNextValueInLinkedList(log_list);
  }
  if (p == (LogInfo *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
      "NoSuchElement","`%s'",name);
  RelinquishSemaphoreInfo(log_semaphore);
  return(p);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t L o g I n f o L i s t                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetLogInfoList() returns any logs that match the specified pattern.
%
%  The format of the GetLogInfoList function is:
%
%      const LogInfo **GetLogInfoList(const char *pattern,
%        unsigned long *number_preferences,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_preferences:  This integer returns the number of logs in the list.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int LogInfoCompare(const void *x,const void *y)
{
  const LogInfo
    **p,
    **q;

  p=(const LogInfo **) x,
  q=(const LogInfo **) y;
  if (LocaleCompare((*p)->path,(*q)->path) == 0)
    return(LocaleCompare((*p)->name,(*q)->name));
  return(LocaleCompare((*p)->path,(*q)->path));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport const LogInfo **GetLogInfoList(const char *pattern,
  unsigned long *number_preferences,ExceptionInfo *exception)
{
  const LogInfo
    **preferences;

  register const LogInfo
    *p;

  register long
    i;

  /*
    Allocate log list.
  */
  assert(pattern != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  assert(number_preferences != (unsigned long *) NULL);
  *number_preferences=0;
  p=GetLogInfo("*",exception);
  if (p == (const LogInfo *) NULL)
    return((const LogInfo **) NULL);
  preferences=(const LogInfo **) AcquireMagickMemory((size_t)
    (GetNumberOfElementsInLinkedList(log_list)+1)*sizeof(*preferences));
  if (preferences == (const LogInfo **) NULL)
    return((const LogInfo **) NULL);
  /*
    Generate log list.
  */
  AcquireSemaphoreInfo(&log_semaphore);
  ResetLinkedListIterator(log_list);
  p=(const LogInfo *) GetNextValueInLinkedList(log_list);
  for (i=0; p != (const LogInfo *) NULL; )
  {
    if ((p->stealth == MagickFalse) &&
        (GlobExpression(p->name,pattern) != MagickFalse))
      preferences[i++]=p;
    p=(const LogInfo *) GetNextValueInLinkedList(log_list);
  }
  RelinquishSemaphoreInfo(log_semaphore);
  qsort((void *) preferences,(size_t) i,sizeof(*preferences),LogInfoCompare);
  preferences[i]=(LogInfo *) NULL;
  *number_preferences=(unsigned long) i;
  return(preferences);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t L o g L i s t                                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetLogList() returns any logs that match the specified pattern.
%
%  The format of the GetLogList function is:
%
%      char **GetLogList(const char *pattern,unsigned long *number_preferences,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_preferences:  This integer returns the number of logs in the list.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int LogCompare(const void *x,const void *y)
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

MagickExport char **GetLogList(const char *pattern,
  unsigned long *number_preferences,ExceptionInfo *exception)
{
  char
    **preferences;

  register const LogInfo
    *p;

  register long
    i;

  /*
    Allocate log list.
  */
  assert(pattern != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  assert(number_preferences != (unsigned long *) NULL);
  *number_preferences=0;
  p=GetLogInfo("*",exception);
  if (p == (const LogInfo *) NULL)
    return((char **) NULL);
  preferences=(char **) AcquireMagickMemory((size_t)
    (GetNumberOfElementsInLinkedList(log_list)+1)*sizeof(*preferences));
  if (preferences == (char **) NULL)
    return((char **) NULL);
  /*
    Generate log list.
  */
  AcquireSemaphoreInfo(&log_semaphore);
  ResetLinkedListIterator(log_list);
  p=(const LogInfo *) GetNextValueInLinkedList(log_list);
  for (i=0; p != (const LogInfo *) NULL; )
  {
    if ((p->stealth == MagickFalse) &&
        (GlobExpression(p->name,pattern) != MagickFalse))
      preferences[i++]=ConstantString(AcquireString(p->name));
    p=(const LogInfo *) GetNextValueInLinkedList(log_list);
  }
  RelinquishSemaphoreInfo(log_semaphore);
  qsort((void *) preferences,(size_t) i,sizeof(*preferences),LogCompare);
  preferences[i]=(char *) NULL;
  *number_preferences=(unsigned long) i;
  return(preferences);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I n i t i a l i z e L o g L i s t                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InitializeLogList() initialize the log list.
%
%  The format of the InitializeLogList method is:
%
%      MagickBooleanType InitializeLogList(ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
static MagickBooleanType InitializeLogList(ExceptionInfo *exception)
{
  if ((log_list == (LinkedListInfo *) NULL) && (instantiate_log == MagickFalse))
    {
      AcquireSemaphoreInfo(&log_semaphore);
      if ((log_list == (LinkedListInfo *) NULL) &&
          (instantiate_log == MagickFalse))
        {
          (void) LoadLogLists(LogFilename,exception);
          instantiate_log=MagickTrue;
        }
      RelinquishSemaphoreInfo(log_semaphore);
    }
  return(log_list != (LinkedListInfo *) NULL ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  I s E v e n t L o g g i n g                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsEventLogging() returns MagickTrue if debug of events is enabled otherwise
%  MagickFalse.
%
%  The format of the IsEventLogging method is:
%
%      MagickBooleanType IsEventLogging(void)
%
%
*/
MagickExport MagickBooleanType IsEventLogging(void)
{
  const LogInfo
    *log_info;

  ExceptionInfo
    exception;

  if ((log_list == (LinkedListInfo *) NULL) ||
      (IsLinkedListEmpty(log_list) != MagickFalse))
    return(MagickFalse);
  GetExceptionInfo(&exception);
  log_info=GetLogInfo("*",&exception);
  DestroyExceptionInfo(&exception);
  return((MagickBooleanType) (log_info->event_mask != NoEvents));
}
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L i s t L o g I n f o                                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListLogInfo() lists the log info to a file.
%
%  The format of the ListLogInfo method is:
%
%      MagickBooleanType ListLogInfo(FILE *file,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o file:  An pointer to a FILE.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType ListLogInfo(FILE *file,ExceptionInfo *exception)
{
  const char
    *path;

  const LogInfo
    **log_info;

  long
    j;

  register long
    i;

  unsigned long
    number_aliases;

  if (file == (const FILE *) NULL)
    file=stdout;
  log_info=GetLogInfoList("*",&number_aliases,exception);
  if (log_info == (const LogInfo **) NULL)
    return(MagickFalse);
  j=0;
  path=(const char *) NULL;
  for (i=0; i < (long) number_aliases; i++)
  {
    if (log_info[i]->stealth != MagickFalse)
      continue;
    if ((path == (const char *) NULL) ||
        (LocaleCompare(path,log_info[i]->path) != 0))
      {
        if (log_info[i]->path != (char *) NULL)
          (void) fprintf(file,"\nPath: %s\n\n",log_info[i]->path);
        (void) fprintf(file,"Filename         Generations  Limit  Format\n");
        (void) fprintf(file,"-------------------------------------------------"
          "------------------------------\n");
      }
    path=log_info[i]->path;
    if (log_info[i]->filename != (char *) NULL)
      {
        (void) fprintf(file,"%s",log_info[i]->filename);
        for (j=(long) strlen(log_info[i]->filename); j <= 16; j++)
          (void) fprintf(file," ");
      }
    (void) fprintf(file,"%11lu",log_info[i]->generations);
    (void) fprintf(file,"%7lu  ",log_info[i]->limit);
    if (log_info[i]->format != (char *) NULL)
      (void) fprintf(file,"%s",log_info[i]->format);
    (void) fprintf(file,"\n");
  }
  (void) fflush(file);
  log_info=(const LogInfo **) RelinquishMagickMemory((void *) log_info);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   L o g M a g i c k E v e n t                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LogMagickEvent() logs an event as determined by the log configuration file.
%  If an error occurs, MagickFalse is returned otherwise MagickTrue.
%
%  The format of the LogMagickEvent method is:
%
%      MagickBooleanType LogMagickEvent(const LogEventType type,
%        const char *module,const char *function,const unsigned long line,
%        const char *format,...)
%
%  A description of each parameter follows:
%
%    o type: The event type.
%
%    o filename: The source module filename.
%
%    o function: The function name.
%
%    o line: The line number of the source module.
%
%    o format: The output format.
%
%
*/

static char *TranslateEvent(const LogEventType type,const char *module,
  const char *function,const unsigned long line,const char *domain,
  const char *event)
{
  char
    *text;

  double
    elapsed_time,
    user_time;

  LogInfo
    *log_info;

  ExceptionInfo
    exception;

  register const char
    *p;

  register char
    *q;

  size_t
    length;

  struct tm
    *time_meridian;

  time_t
    seconds;

  GetExceptionInfo(&exception);
  log_info=(LogInfo *) GetLogInfo("*",&exception);
  DestroyExceptionInfo(&exception);
  seconds=time((time_t *) NULL);
  time_meridian=localtime(&seconds);
  elapsed_time=GetElapsedTime(&log_info->timer);
  user_time=GetUserTime(&log_info->timer);
  text=AcquireString((char *) NULL);
  length=MaxTextExtent;
  if (log_info->format == (char *) NULL)
    return(text);
  if (LocaleCompare(log_info->format,"xml") == 0)
    {
      char
        timestamp[MaxTextExtent];

      /*
        Translate event in "XML" format.
      */
      (void) FormatMagickString(timestamp,MaxTextExtent,
        "%04d%02d%02d%02d%02d%02d",time_meridian->tm_year+1900,
        time_meridian->tm_mon+1,time_meridian->tm_mday,time_meridian->tm_hour,
        time_meridian->tm_min,time_meridian->tm_sec);
      (void) FormatMagickString(text,length,
        "<entry>\n"
        "  <timestamp>%s</timestamp>\n"
        "  <elapsed-time>%ld:%02ld</elapsed-time>\n"
        "  <user-time>%0.3f</user-time>\n"
        "  <pid>%ld</pid>\n"
        "  <module>%s</module>\n"
        "  <function>%s</function>\n"
        "  <line>%lu</line>\n"
        "  <domain>%s</domain>\n"
        "  <event>%s</event>\n"
        "</entry>",timestamp,(long) (elapsed_time/60.0),
        (long) ceil(fmod(elapsed_time,60.0)),user_time,(long) getpid(),module,
        function,line,domain,event);
      return(text);
    }
  /*
    Translate event in "human readable" format.
  */
  q=text;
  for (p=log_info->format; *p != '\0'; p++)
  {
    *q='\0';
    if ((size_t) (q-text+MaxTextExtent) >= length)
      {
        length<<=1;
        text=(char *) ResizeMagickMemory(text,(length+MaxTextExtent)*
          sizeof(*text));
        if (text == (char *) NULL)
          return((char *) NULL);
        q=text+strlen(text);
      }
    /*
      The format of the log is defined by embedding special format characters:

        %c   client name
        %d   domain
        %e   event
        %f   function
        %l   line
        %m   module
        %p   process id
        %r   real CPU time
        %t   wall clock time
        %u   user CPU time
        %v   version
        %%   percent sign
        \n   newline
        \r   carriage return
    */
    if ((*p == '\\') && (*(p+1) == 'r'))
      {
        *q++='\r';
        p++;
        continue;
      }
    if ((*p == '\\') && (*(p+1) == 'n'))
      {
        *q++='\n';
        p++;
        continue;
      }
    if (*p != '%')
      {
        *q++=(*p);
        continue;
      }
    p++;
    switch (*p)
    {
      case 'c':
      {
        (void) CopyMagickString(q,GetClientName(),MaxTextExtent);
        q+=strlen(GetClientName());
        break;
      }
      case 'd':
      {
        (void) CopyMagickString(q,domain,MaxTextExtent);
        q+=strlen(domain);
        break;
      }
      case 'e':
      {
        (void) CopyMagickString(q,event,MaxTextExtent);
        q+=strlen(event);
        break;
      }
      case 'f':
      {
        (void) CopyMagickString(q,function,MaxTextExtent);
        q+=strlen(function);
        break;
      }
      case 'l':
      {
        (void) FormatMagickString(q,MaxTextExtent,"%lu",line);
        while (*q != '\0')
          q++;
        break;
      }
      case 'm':
      {
        register const char
          *p;

        for (p=module+strlen(module)-1; p > module; p--)
          if (*p == *DirectorySeparator)
            {
              p++;
              break;
            }
        (void) CopyMagickString(q,p,MaxTextExtent);
        q+=strlen(p);
        break;
      }
      case 'p':
      {
        (void) FormatMagickString(q,MaxTextExtent,"%ld",(long) getpid());
        while (*q != '\0')
          q++;
        break;
      }
      case 'r':
      {
        (void) FormatMagickString(q,MaxTextExtent,"%ld:%02ld",
          (long) (elapsed_time/60.0),(long) ceil(fmod(elapsed_time,60.0)));
        while (*q != '\0')
          q++;
        break;
      }
      case 't':
      {
        (void) FormatMagickString(q,MaxTextExtent,"%02d:%02d:%02d",
          time_meridian->tm_hour,time_meridian->tm_min,time_meridian->tm_sec);
        while (*q != '\0')
          q++;
        break;
      }
      case 'u':
      {
        (void) FormatMagickString(q,MaxTextExtent,"%0.3fu",user_time);
        while (*q != '\0')
          q++;
        break;
      }
      case 'v':
      {
        (void) CopyMagickString(q,MagickLibVersionText,MaxTextExtent);
        q+=strlen(MagickLibVersionText);
        break;
      }
      default:
      {
        *q++=(*p);
        break;
      }
    }
  }
  *q='\0';
  return(text);
}

MagickBooleanType LogMagickEventList(const LogEventType type,const char *module,
  const char *function,const unsigned long line,const char *format,
  va_list operands)
{
  char
    event[MaxTextExtent],
    *text;

  const char
    *domain;

  ExceptionInfo
    exception;

  int
    n;

  LogInfo
    *log_info;

  if (IsEventLogging() == MagickFalse)
    return(MagickFalse);
  GetExceptionInfo(&exception);
  log_info=(LogInfo *) GetLogInfo("*",&exception);
  DestroyExceptionInfo(&exception);
  AcquireSemaphoreInfo(&log_semaphore);
  if ((log_info->event_mask & type) == 0)
    {
      RelinquishSemaphoreInfo(log_semaphore);
      return(MagickTrue);
    }
  domain=MagickOptionToMnemonic(MagickLogEventOptions,type);
#if defined(HAVE_VSNPRINTF)
  n=vsnprintf(event,MaxTextExtent,format,operands);
#else
  n=vsprintf(event,format,operands);
#endif
  if  (n < 0)
    event[MaxTextExtent-1]='\0';
  text=TranslateEvent(type,module,function,line,domain,event);
  if (text == (char *) NULL)
    {
      (void) ContinueTimer((TimerInfo *) &log_info->timer);
      RelinquishSemaphoreInfo(log_semaphore);
      return(MagickFalse);
    }
  if ((log_info->handler_mask & ConsoleHandler) != 0)
    {
      (void) fprintf(stdout,"%s\n",text);
      (void) fflush(stdout);
#if defined(__WINDOWS__)
      OutputDebugString(text);
#endif
    }
  if ((log_info->handler_mask & StdoutHandler) != 0)
    {
      (void) fprintf(stdout,"%s\n",text);
      (void) fflush(stdout);
    }
  if ((log_info->handler_mask & StderrHandler) != 0)
    {
      (void) fprintf(stderr,"%s\n",text);
      (void) fflush(stderr);
    }
  if ((log_info->handler_mask & FileHandler) != 0)
    {
      log_info->count++;
      if (log_info->count == log_info->limit)
        {
          (void) fprintf(log_info->file,"</log>\n");
          (void) fclose(log_info->file);
          log_info->file=(FILE *) NULL;
        }
      if (log_info->file == (FILE *) NULL)
        {
          char
            filename[MaxTextExtent];

          (void) FormatMagickString(filename,MaxTextExtent,log_info->filename,
            log_info->generation % log_info->generations);
          log_info->file=fopen(filename,"w");
          if (log_info->file == (FILE *) NULL)
            {
              RelinquishSemaphoreInfo(log_semaphore);
              return(MagickFalse);
            }
          log_info->generation++;
          (void) fprintf(log_info->file,
            "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n");
          (void) fprintf(log_info->file,"<log>\n");
        }
      (void) fprintf(log_info->file,"%s\n",text);
      (void) fflush(log_info->file);
    }
  text=(char  *) RelinquishMagickMemory(text);
  (void)  ContinueTimer((TimerInfo *) &log_info->timer);
  RelinquishSemaphoreInfo(log_semaphore);
  return(MagickTrue);
}

MagickBooleanType LogMagickEvent(const LogEventType type,const char *module,
  const char *function,const unsigned long line,const char *format,...)
{
  va_list
    operands;

  MagickBooleanType
    status;
   
  va_start(operands,format);
  status=LogMagickEventList(type,module,function,line,format,operands);
  va_end(operands);
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   L o a d L o g L i s t                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LoadLogList() loads the log configuration file which provides a
%  mapping between log attributes and log name.
%
%  The format of the LoadLogList method is:
%
%      MagickBooleanType LoadLogList(const char *xml,const char *filename,
%        const unsigned long depth,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o xml:  The log list in XML format.
%
%    o filename:  The log list filename.
%
%    o depth: depth of <include /> statements.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
static MagickBooleanType LoadLogList(const char *xml,const char *filename,
  const unsigned long depth,ExceptionInfo *exception)
{
  char
    keyword[MaxTextExtent],
    *q,
    *token;

  LogInfo
    *log_info = (LogInfo *) NULL;

  MagickStatusType
    status;

  /*
    Load the log map file.
  */
  if (xml == (const char *) NULL)
    return(MagickFalse);
  if (log_list == (LinkedListInfo *) NULL)
    {
      log_list=NewLinkedList(0);
      if (log_list == (LinkedListInfo *) NULL)
        {
          ThrowFileException(exception,ResourceLimitError,
            "MemoryAllocationFailed",filename);
          return(MagickFalse);
        }
    }
  status=MagickTrue;
  token=AcquireString((char *) xml);
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
                  status|=LoadLogList(xml,path,depth+1,exception);
                  xml=(char *) RelinquishMagickMemory(xml);
                }
            }
        }
        continue;
      }
    if (LocaleCompare(keyword,"<logmap>") == 0)
      {
        /*
          Allocate memory for the log list.
        */
        log_info=(LogInfo *) AcquireMagickMemory(sizeof(*log_info));
        if (log_info == (LogInfo *) NULL)
          ThrowMagickFatalException(ResourceLimitFatalError,
            "MemoryAllocationFailed",filename);
        (void) ResetMagickMemory(log_info,0,sizeof(*log_info));
        log_info->path=ConstantString(AcquireString(filename));
        GetTimerInfo((TimerInfo *) &log_info->timer);
        log_info->signature=MagickSignature;
        continue;
      }
    if (log_info == (LogInfo *) NULL)
      continue;
    if (LocaleCompare(keyword,"</logmap>") == 0)
      {
        status=AppendValueToLinkedList(log_list,log_info);
        if (status == MagickFalse)
          (void) ThrowMagickException(exception,GetMagickModule(),
            ResourceLimitError,"MemoryAllocationFailed","`%s'",filename);
        log_info=(LogInfo *) NULL;
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
        if (LocaleCompare((char *) keyword,"events") == 0)
          {
            log_info->event_mask=(LogEventType) (log_info->event_mask |
              ParseMagickOption(MagickLogEventOptions,MagickTrue,token));
            break;
          }
        break;
      }
      case 'F':
      case 'f':
      {
        if (LocaleCompare((char *) keyword,"filename") == 0)
          {
            if (log_info->filename != (char *) NULL)
              log_info->filename=(char *)
                RelinquishMagickMemory(log_info->filename);
            log_info->filename=ConstantString(AcquireString(token));
            break;
          }
        if (LocaleCompare((char *) keyword,"format") == 0)
          {
            if (log_info->format != (char *) NULL)
              log_info->format=(char *)
                RelinquishMagickMemory(log_info->format);
            log_info->format=ConstantString(AcquireString(token));
            break;
          }
        break;
      }
      case 'G':
      case 'g':
      {
        if (LocaleCompare((char *) keyword,"generations") == 0)
          {
            log_info->generations=(unsigned long) atol(token);
            break;
          }
        break;
      }
      case 'L':
      case 'l':
      {
        if (LocaleCompare((char *) keyword,"limit") == 0)
          {
            log_info->limit=(unsigned long) atol(token);
            break;
          }
        break;
      }
      case 'O':
      case 'o':
      {
        if (LocaleCompare((char *) keyword,"output") == 0)
          {
            log_info->handler_mask=(LogHandlerType)
              (log_info->handler_mask | ParseLogHandlers(token));
            break;
          }
        break;
      }
      default:
        break;
    }
  }
  token=(char *) RelinquishMagickMemory(token);
  if (log_list == (LinkedListInfo *) NULL)
    return(MagickFalse);
  return(status != 0 ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L o a d L o g L i s t s                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LoadLogLists() loads one or more log configuration file which provides a
%  mapping between log attributes and log name.
%
%  The format of the LoadLogLists method is:
%
%      MagickBooleanType LoadLogLists(const char *filename,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o filename: The log configuration filename.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
static MagickBooleanType LoadLogLists(const char *filename,
  ExceptionInfo *exception)
{
#if defined(UseEmbeddableMagick)
  return(LoadLogList(LogMap,"built-in",0,exception));
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
    status|=LoadLogList((const char *) option->datum,option->path,0,exception);
    option=(const StringInfo *) GetNextValueInLinkedList(options);
  }
  options=DestroyConfigureOptions(options);
  if ((log_list == (LinkedListInfo *) NULL) || 
      (IsLinkedListEmpty(log_list) != MagickFalse))
    status|=LoadLogList(LogMap,"built-in",0,exception);
  else
    (void) SetExceptionInfo(exception,UndefinedException);
  return(status != 0 ? MagickTrue : MagickFalse);
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   P a r s e L o g H a n d l e r s                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ParseLogHandlers() parses a string defining which handlers takes a log
%  message and exports them.
%
%  The format of the ParseLogHandlers method is:
%
%      LogHandlerType ParseLogHandlers(const char *handlers)
%
%  A description of each parameter follows:
%
%    o handlers: one or more handlers separated by commas.
%
%
*/
static LogHandlerType ParseLogHandlers(const char *handlers)
{
  LogHandlerType
    handler_mask;

  register const char
    *p;

  register long
    i;

  size_t
    length;

  handler_mask=NoHandler;
  for (p=handlers; p != (char *) NULL; p=strchr(p,','))
  {
    while ((*p != '\0') && ((isspace((int) ((unsigned char) *p)) != 0) ||
           (*p == ',')))
      p++;
    for (i=0; LogHandlers[i].name != (char *) NULL; i++)
    {
      length=strlen(LogHandlers[i].name);
      if (LocaleNCompare(p,LogHandlers[i].name,length) == 0)
        {
          handler_mask=(LogHandlerType) (handler_mask | LogHandlers[i].handler);
          break;
        }
    }
    if (LogHandlers[i].name == (char *) NULL)
      return(UndefinedHandler);
  }
  return(handler_mask);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t L o g E v e n t M a s k                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetLogEventMask() accepts a list that determines which events to log.  All
%  other events are ignored.  By default, no debug is enabled.  This method
%  returns the previous log event mask.
%
%  The format of the SetLogEventMask method is:
%
%      LogEventType SetLogEventMask(const char *events)
%
%  A description of each parameter follows:
%
%    o events: log these events.
%
%
*/
MagickExport LogEventType SetLogEventMask(const char *events)
{
  ExceptionInfo
    exception;

  LogInfo
    *log_info;

  long
    option;

  GetExceptionInfo(&exception);
  log_info=(LogInfo *) GetLogInfo("*",&exception);
  DestroyExceptionInfo(&exception);
  option=ParseMagickOption(MagickLogEventOptions,MagickTrue,events);
  AcquireSemaphoreInfo(&log_semaphore);
  log_info=(LogInfo *) GetValueFromLinkedList(log_list,0);
  log_info->event_mask=(LogEventType) option;
  if (option == -1)
    log_info->event_mask=UndefinedEvents;
  RelinquishSemaphoreInfo(log_semaphore);
  return(log_info->event_mask);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S e t L o g F o r m a t                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SetLogFormat() sets the format for the "human readable" log record.
%
%  The format of the LogMagickFormat method is:
%
%      SetLogFormat(const char *format)
%
%  A description of each parameter follows:
%
%    o format: The log record format.
%
%
*/
MagickExport void SetLogFormat(const char *format)
{
  ExceptionInfo
    exception;

  LogInfo
    *log_info;

  GetExceptionInfo(&exception);
  log_info=(LogInfo *) GetLogInfo("*",&exception);
  DestroyExceptionInfo(&exception);
  AcquireSemaphoreInfo(&log_semaphore);
  if (log_info->format != (char *) NULL)
    log_info->format=(char *) RelinquishMagickMemory(log_info->format);
  log_info->format=ConstantString(AcquireString(format));
  RelinquishSemaphoreInfo(log_semaphore);
}
