/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%             U   U  TTTTT  IIIII  L      IIIII  TTTTT  Y   Y                 %
%             U   U    T      I    L        I      T     Y Y                  %
%             U   U    T      I    L        I      T      Y                   %
%             U   U    T      I    L        I      T      Y                   %
%              UUU     T    IIIII  LLLLL  IIIII    T      Y                   %
%                                                                             %
%                                                                             %
%                       ImageMagick Utility Methods                           %
%                                                                             %
%                             Software Design                                 %
%                               John Cristy                                   %
%                              January 1993                                   %
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
#include "magick/attribute.h"
#include "magick/blob.h"
#include "magick/color.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/memory_.h"
#include "magick/option.h"
#include "magick/resource_.h"
#include "magick/semaphore.h"
#include "magick/signature.h"
#include "magick/statistic.h"
#include "magick/string_.h"
#include "magick/token.h"
#include "magick/utility.h"
/*
  Static declarations.
*/
static const char
  Base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
  Forward declaration.
*/
static int
  IsDirectory(const char *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   A c q u i r e U n i q u e F i l e n a m e                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AcquireUniqueFilename() replaces the contents of path by a unique path name.
%
%  The format of the AcquireUniqueFilename method is:
%
%      MagickBooleanType AcquireUniqueFilename(char *path)
%
%  A description of each parameter follows.
%
%   o  path:  Specifies a pointer to an array of characters.  The unique path
%      name is returned in this array.
%
*/
MagickExport MagickBooleanType AcquireUniqueFilename(char *path)
{
  int
    file;

  file=AcquireUniqueFileResource(path);
  if (file == -1)
    return(MagickFalse);
  file=close(file)-1;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  A p p e n d I m a g e F o r m a t                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  AppendImageFormat() appends the image format type to the filename.  If an
%  extension to the file already exists, it is first removed.
%
%  The format of the AppendImageFormat method is:
%
%      void AppendImageFormat(const char *format,char *filename)
%
%  A description of each parameter follows.
%
%   o  format:  Specifies a pointer to an array of characters.  This is the
%      format of the image.
%
%   o  filename:  Specifies a pointer to an array of characters.  The unique
%      file name is returned in this array.
%
%
*/
MagickExport void AppendImageFormat(const char *format,char *filename)
{
  char
    root[MaxTextExtent];

  assert(format != (char *) NULL);
  assert(filename != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",filename);
  if ((*format == '\0') || (*filename == '\0'))
    return;
  if (LocaleCompare(filename,"-") == 0)
    {
      char
        message[MaxTextExtent];

      (void) FormatMagickString(message,MaxTextExtent,"%s:%s",format,filename);
      (void) CopyMagickString(filename,message,MaxTextExtent);
      return;
    }
  GetPathComponent(filename,RootPath,root);
  (void) FormatMagickString(filename,MaxTextExtent,"%s.%s",root,format);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   B a s e 6 4 D e c o d e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Base64Decode() decodes Base64-encoded text and returns its binary
%  equivalent.  NULL is returned if the text is not valid Base64 data, or a
%  memory allocation failure occurs.
%
%  Contributed by Bob Friesenhahn.
%
%  The format of the Base64Decode method is:
%
%      unsigned char *Base64Decode(const char *source,length_t *length)
%
%  A description of each parameter follows:
%
%    o source:  A pointer to a Base64-encoded string.
%
%    o length: The number of bytes decoded.
%
*/
MagickExport unsigned char *Base64Decode(const char *source,size_t *length)
{
  int
    state;

  register const char
    *p,
    *q;

  register size_t
    i;

  size_t
    max_length;

  unsigned char
    *decode;

  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(source != (char *) NULL);
  assert(length != (size_t *) NULL);
  *length=0;
  max_length=3*strlen(source)/4+1;
  decode=(unsigned char *) AcquireMagickMemory(max_length);
  if (decode == (unsigned char *) NULL)
    return((unsigned char *) NULL);
  i=0;
  state=0;
  for (p=source; *p != '\0'; p++)
  {
    if (isspace((int) ((unsigned char) *p)) != 0)
      continue;
    if (*p == '=')
      break;
    q=strchr(Base64,*p);
    if (q == (char *) NULL)
      {
        decode=(unsigned char *) RelinquishMagickMemory(decode);
        return((unsigned char *) NULL);  /* non-Base64 character */
      }
    switch (state)
    {
      case 0:
      {
        decode[i]=(q-Base64) << 2;
        state++;
        break;
      }
      case 1:
      {
        decode[i++]|=(q-Base64) >> 4;
        decode[i]=((q-Base64) & 0x0f) << 4;
        state++;
        break;
      }
      case 2:
      {
        decode[i++]|=(q-Base64) >> 2;
        decode[i]=((q-Base64) & 0x03) << 6;
        state++;
        break;
      }
      case 3:
      {
        decode[i++]|=(q-Base64);
        state=0;
        break;
      }
    }
  }
  /*
    Verify Base-64 string has proper terminal characters.
  */
  if (*p != '=')
    {
      if (state != 0)
        {
          decode=(unsigned char *) RelinquishMagickMemory(decode);
          return((unsigned char *) NULL);
        }
    }
  else
    {
      p++;
      switch (state)
      {
        case 0:
        case 1:
        {
          /*
            Unrecognized '=' character.
          */
          decode=(unsigned char *) RelinquishMagickMemory(decode);
          return((unsigned char *) NULL);
        }
        case 2:
        {
          for ( ; *p != '\0'; p++)
            if (isspace((int) ((unsigned char) *p)) == 0)
              break;
          if (*p != '=')
            {
              decode=(unsigned char *) RelinquishMagickMemory(decode);
              return((unsigned char *) NULL);
            }
          p++;
        }
        case 3:
        {
          for ( ; *p != '\0'; p++)
            if (isspace((int) ((unsigned char) *p)) == 0)
              {
                decode=(unsigned char *) RelinquishMagickMemory(decode);
                return((unsigned char *) NULL);
              }
          if ((int) decode[i] != 0)
            {
              decode=(unsigned char *) RelinquishMagickMemory(decode);
              return((unsigned char *) NULL);
            }
        }
      }
    }
  *length=i;
  assert(*length < max_length);
  return(decode);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   B a s e 6 4 E n c o d e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  Base64Encode() encodes arbitrary binary data to Base64 encoded format as
%  described by the "Base64 Content-Transfer-Encoding" section of RFC 2045 and
%  returns the result as a null-terminated ASCII string.  NULL is returned if
%  a memory allocation failure occurs.
%
%  The format of the Base64Encode method is:
%
%      char *Base64Encode(const unsigned char *blob,const size_t blob_length,
%        size_t *encode_length)
%
%  A description of each parameter follows:
%
%    o blob:  A pointer to binary data to encode.
%
%    o blob_length: The number of bytes to encode.
%
%    o encode_length:  The number of bytes encoded.
%
*/
MagickExport char *Base64Encode(const unsigned char *blob,
  const size_t blob_length,size_t *encode_length)
{
  char
    *encode;

  register const unsigned char
    *p;

  register size_t
    i;

  size_t
    max_length,
    remainder;

  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(blob != (const unsigned char *) NULL);
  assert(blob_length != 0);
  assert(encode_length != (size_t *) NULL);
  *encode_length=0;
  max_length=4*blob_length/3+4;
  encode=(char *) AcquireMagickMemory(max_length);
  if (encode == (char *) NULL)
    return((char *) NULL);
  i=0;
  for (p=blob; p < (blob+blob_length-2); p+=3)
  {
    encode[i++]=Base64[(int) (*p >> 2)];
    encode[i++]=Base64[(int) (((*p & 0x03) << 4)+(*(p+1) >> 4))];
    encode[i++]=Base64[(int) (((*(p+1) & 0x0f) << 2)+(*(p+2) >> 6))];
    encode[i++]=Base64[(int) (*(p+2) & 0x3f)];
  }
  remainder=blob_length % 3;
  if (remainder != 0)
    {
      long
        j;

      unsigned char
        code[3];

      code[0]='\0';
      code[1]='\0';
      code[2]='\0';
      for (j=0; j < (long) remainder; j++)
        code[j]=(*p++);
      encode[i++]=Base64[(int) (code[0] >> 2)];
      encode[i++]=Base64[(int) (((code[0] & 0x03) << 4)+(code[1] >> 4))];
      if (remainder == 1)
        encode[i++]='=';
      else
        encode[i++]=Base64[(int) (((code[1] & 0x0f) << 2)+(code[2] >> 6))];
      encode[i++]='=';
    }
  *encode_length=i;
  encode[i++]='\0';
  assert(i <= max_length);
  return(encode);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   C h o p P a t h C o m p o n e n t s                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ChopPathComponents() removes the number of specified file components from a
%  path.
%
%  The format of the ChopPathComponents method is:
%
%      ChopPathComponents(char *path,unsigned long components)
%
%  A description of each parameter follows:
%
%    o path:  The path.
%
%    o components:  The number of components to chop.
%
*/
MagickExport void ChopPathComponents(char *path,const unsigned long components)
{
  register long
    i;

  for (i=0; i < (long) components; i++)
    GetPathComponent(path,HeadPath,path);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x p a n d F i l e n a m e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExpandFilename() expands '~' in a path.
%
%  The format of the ExpandFilename function is:
%
%      ExpandFilename(char *path)
%
%  A description of each parameter follows:
%
%    o path: Specifies a pointer to a character array that contains the
%      path.
%
%
*/
MagickExport void ExpandFilename(char *path)
{
  char
    expanded_path[MaxTextExtent];

  if (path == (char *) NULL)
    return;
  if (*path != '~')
    return;
  (void) CopyMagickString(expanded_path,path,MaxTextExtent);
  if ((*(path+1) == *DirectorySeparator) || (*(path+1) == '\0'))
    {
      register const char
        *p;

      /*
        Substitute ~ with $HOME.
      */
      p=getenv("HOME");
      if (p == (const char *) NULL)
        p=".";
      (void) CopyMagickString(expanded_path,p,MaxTextExtent);
      (void) ConcatenateMagickString(expanded_path,path+1,MaxTextExtent);
    }
  else
    {
#if defined(POSIX)
      char
        username[MaxTextExtent];

      register char
        *p;

      struct passwd
        *entry;

      /*
        Substitute ~ with home directory from password file.
      */
      (void) CopyMagickString(username,path+1,MaxTextExtent);
      p=strchr(username,'/');
      if (p != (char *) NULL)
        *p='\0';
      entry=getpwnam(username);
      if (entry == (struct passwd *) NULL)
        return;
      (void) CopyMagickString(expanded_path,entry->pw_dir,MaxTextExtent);
      if (p != (char *) NULL)
        {
          (void) ConcatenateMagickString(expanded_path,"/",MaxTextExtent);
          (void) ConcatenateMagickString(expanded_path,p+1,MaxTextExtent);
        }
#endif
    }
  (void) CopyMagickString(path,expanded_path,MaxTextExtent);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x p a n d F i l e n a m e s                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExpandFilenames() checks each argument of the command line vector and
%  expands it if they have a wildcard character.  For example, *.jpg might
%  expand to:  bird.jpg rose.jpg tiki.jpg.
%
%  The format of the ExpandFilenames function is:
%
%      status=ExpandFilenames(int *argc,char ***argv)
%
%  A description of each parameter follows:
%
%    o argc: Specifies a pointer to an integer describing the number of
%      elements in the argument vector.
%
%    o argv: Specifies a pointer to a text array containing the command line
%      arguments.
%
%
*/
MagickExport MagickBooleanType ExpandFilenames(int *argc,char ***argv)
{
  char
    **filelist,
    filename[MaxTextExtent],
    home_directory[MaxTextExtent],
    magick[MaxTextExtent],
    *option,
    path[MaxTextExtent],
    subimage[MaxTextExtent],
    **vector;

  long
    count,
    parameters;

  register long
    i,
    j;

  unsigned long
    number_files;

  /*
    Allocate argument vector.
  */
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(argc != (int *) NULL);
  assert(argv != (char ***) NULL);
  for (i=1; i < (long) *argc; i++)
    if (strlen((*argv)[i]) > (MaxTextExtent/2-1))
      ThrowMagickFatalException(ResourceLimitFatalError,
        "TokenLengthExceedsLimit",(*argv)[i]);
  vector=(char **) AcquireMagickMemory((*argc+1)*sizeof(*vector));
  if (vector == (char **) NULL)
    return(MagickFalse);
  /*
    Expand any wildcard filenames.
  */
  (void) getcwd(home_directory,MaxTextExtent);
  count=0;
  for (i=0; i < (long) *argc; i++)
  {
    option=(*argv)[i];
    vector[count++]=AcquireString(option);
    parameters=ParseMagickOption(MagickCommandOptions,MagickFalse,option);
    if (parameters > 0)
      {
        /*
          Do not expand command option parameters.
        */
        for (j=0; j < parameters; j++)
        {
          i++;
          if (i == (long) *argc)
            break;
          option=(*argv)[i];
          vector[count++]=AcquireString(option);
        }
        continue;
      }
    if ((*option == '"') || (*option == '\''))
      continue;
    GetPathComponent(option,TailPath,filename);
    if (IsGlob(filename) == MagickFalse)
      continue;
    GetPathComponent(option,MagickPath,magick);
    if (LocaleCompare(magick,"VID") == 0)
      continue;
    GetPathComponent(option,HeadPath,path);
    GetPathComponent(option,SubimagePath,subimage);
    ExpandFilename(path);
    filelist=ListFiles(*path == '\0' ? home_directory : path,filename,
      &number_files);
    if (filelist == (char **) NULL)
      continue;
    for (j=0; j < (long) number_files; j++)
      if (IsDirectory(filelist[j]) <= 0)
        break;
    if (j == (long) number_files)
      {
        for (j=0; j < (long) number_files; j++)
          filelist[j]=(char *) RelinquishMagickMemory(filelist[j]);
        filelist=(char **) RelinquishMagickMemory(filelist);
        continue;
      }
    /*
      Transfer file list to argument vector.
    */
    vector=(char **) ResizeMagickMemory(vector,(size_t) (*argc+count+
      number_files+1)*sizeof(*vector));
    if (vector == (char **) NULL)
      return(MagickFalse);
    count--;
    for (j=0; j < (long) number_files; j++)
    {
      (void) CopyMagickString(filename,path,MaxTextExtent);
      if (*path != '\0')
        (void) ConcatenateMagickString(filename,DirectorySeparator,
          MaxTextExtent);
      (void) ConcatenateMagickString(filename,filelist[j],MaxTextExtent);
      filelist[j]=(char *) RelinquishMagickMemory(filelist[j]);
      if (IsAccessible(filename) != MagickFalse)
        {
          char
            path[MaxTextExtent];

          *path='\0';
          if (*magick != '\0')
            {
              (void) ConcatenateMagickString(path,magick,MaxTextExtent);
              (void) ConcatenateMagickString(path,":",MaxTextExtent);
            }
          (void) ConcatenateMagickString(path,filename,MaxTextExtent);
          if (*subimage != '\0')
            {
              (void) ConcatenateMagickString(path,"[",MaxTextExtent);
              (void) ConcatenateMagickString(path,subimage,MaxTextExtent);
              (void) ConcatenateMagickString(path,"]",MaxTextExtent);
            }
          vector[count++]=AcquireString(path);
        }
    }
    filelist=(char **) RelinquishMagickMemory(filelist);
  }
  vector[count]=(char *) NULL;
  *argc=(int) count;
  *argv=vector;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  F o r m a t S i z e                                                        %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FormatSize() converts a size to a human readable format, for example,
%  14kb, 234mb, 2.7gb, or 3.0tb.  Scaling is done by repetitively dividing by
%  1024.
%
%  The format of the FormatSize method is:
%
%      void FormatSize(const MagickSizeType size,char *format)
%
%  A description of each parameter follows:
%
%    o size:  convert this size to a human readable format.
%
%    o format:  human readable format.
%
%
*/
MagickExport void FormatSize(const MagickSizeType size,char *format)
{
  double
    length;

  register long
    i;

  length=(double) ((MagickOffsetType) size);
  for (i=0; length > 1024.0; i++)
    length/=1024.0;
  switch (i)
  {
    case 0:
    {
      (void) FormatMagickString(format,MaxTextExtent,"%g",length);
      break;
    }
    case 1:
    {
      (void) FormatMagickString(format,MaxTextExtent,"%.0fkb",length);
      break;
    }
    case 2:
    {
      (void) FormatMagickString(format,MaxTextExtent,"%.1fmb",length);
      break;
    }
    case 3:
    {
      (void) FormatMagickString(format,MaxTextExtent,"%.2fgb",length);
      break;
    }
    case 4:
    {
      (void) FormatMagickString(format,MaxTextExtent,"%.2ftb",length);
      break;
    }
    case 5:
    {
      (void) FormatMagickString(format,MaxTextExtent,"%.2fpb",length);
      break;
    }
    default:
    {
      (void) FormatMagickString(format,MaxTextExtent,"%.3feb",length);
      break;
    }
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t E x e c u t i o n P a t h                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetExecutionPath() returns the pathname of the executable that started
%  the process.  On success MagickTrue is returned, otherwise MagickFalse.
%
%  The format of the GetExecutionPath method is:
%
%      MagickBooleanType GetExecutionPath(char *path)
%
%  A description of each parameter follows:
%
%    o path: The pathname of the executable that started the process.
%
*/
MagickExport MagickBooleanType GetExecutionPath(char *path)
{
  *path='\0';
  (void) getcwd(path,MaxTextExtent);
#if defined(__WINDOWS__)
  return(NTGetExecutionPath(path));
#endif
#if defined(HAVE_GETEXECNAME)
  {
    const char
      *execution_path;

    execution_path=(const char *) getexecname();
    if (execution_path != (const char *) NULL)
      {
        if (*execution_path != *DirectorySeparator)
          (void) ConcatenateMagickString(path,"/",MaxTextExtent);
        (void) ConcatenateMagickString(path,execution_path,MaxTextExtent);
      }
  }
#endif
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t P a t h C o m p o n e n t                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetPathComponent() returns the parent directory name, filename, basename, or
%  extension of a file path.
%
%  The format of the GetPathComponent function is:
%
%      GetPathComponent(const char *path,PathType type,char *component)
%
%  A description of each parameter follows:
%
%    o path: Specifies a pointer to a character array that contains the
%      file path.
%
%    o type: Specififies which file path component to return.
%
%    o component: The selected file path component is returned here.
%
*/
MagickExport void GetPathComponent(const char *path,PathType type,
  char *component)
{
  char
    magick[MaxTextExtent],
    *q,
    subimage[MaxTextExtent];

  register char
    *p;

  assert(path != (const char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",path);
  assert(component != (char *) NULL);
  if (*path == '\0')
    {
      *component='\0';
      return;
    }
  (void) CopyMagickString(component,path,MaxTextExtent);
  *magick='\0';
  if ((IsDirectory(path) < 0) && (IsAccessible(path) == MagickFalse))
    for (p=component; *p != '\0'; p++)
      if (*p == ':')
        {
          /*
            Look for image format specification (e.g. ps3:image).
          */
          (void) CopyMagickString(magick,component,(size_t) (p-component+1));
          if (IsMagickConflict(magick) != MagickFalse)
            *magick='\0';
          else
            for (q=component; *q != '\0'; q++)
              *q=(*++p);
          break;
        }
  *subimage='\0';
  p=component;
  if (*p != '\0')
    p=component+strlen(component)-1;
  if ((*p == ']') && (strchr(component,'[') != (char *) NULL))
    {
      /*
        Look for scene specification (e.g. img0001.pcd[4]).
      */
      for (q=p-1; q > component; q--)
        if (*q == '[')
          break;
      if (*q == '[')
        {
          (void) CopyMagickString(subimage,q+1,MaxTextExtent);
          subimage[p-q-1]='\0';
          if ((IsSceneGeometry(subimage,MagickFalse) == MagickFalse) &&
              (IsGeometry(subimage) == MagickFalse))
            *subimage='\0';
          else
            *q='\0';
        }
    }
  p=component;
  if (*p != '\0')
    for (p=component+strlen(component)-1; p > component; p--)
      if (IsBasenameSeparator(*p))
        break;
  switch (type)
  {
    case MagickPath:
    {
      (void) CopyMagickString(component,magick,MaxTextExtent);
      break;
    }
    case RootPath:
    {
      for (p=component+(strlen(component)-1); p > component; p--)
      {
        if (IsBasenameSeparator(*p))
          break;
        if (*p == '.')
          break;
      }
      if (*p == '.')
        *p='\0';
      break;
    }
    case HeadPath:
    {
      *p='\0';
      break;
    }
    case TailPath:
    {
      if (IsBasenameSeparator(*p))
        (void) CopyMagickMemory((unsigned char *) component,
          (const unsigned char *) (p+1),strlen(p+1)+1);
      break;
    }
    case BasePath:
    {
      if (IsBasenameSeparator(*p))
        (void) CopyMagickString(component,p+1,MaxTextExtent);
      for (p=component+(strlen(component)-1); p > component; p--)
        if (*p == '.')
          {
            *p='\0';
            break;
          }
      break;
    }
    case ExtensionPath:
    {
      if (IsBasenameSeparator(*p))
        (void) CopyMagickString(component,p+1,MaxTextExtent);
      p=component;
      if (*p != '\0')
        for (p=component+strlen(component)-1; p > component; p--)
          if (*p == '.')
            break;
      *component='\0';
      if (*p == '.')
        (void) CopyMagickString(component,p+1,MaxTextExtent);
      break;
    }
    case SubimagePath:
    {
      (void) CopyMagickString(component,subimage,MaxTextExtent);
      break;
    }
    case CanonicalPath:
    default:
      break;
  }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  I s A c c e s s i b l e                                                    %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsAccessible() returns MagickTrue if the file as defined by the path is
%  accessible.
%
%  The format of the IsAccessible method is:
%
%      MagickBooleanType IsAccessible(const char *filename)
%
%  A description of each parameter follows.
%
%    o path:  Specifies a path to a file.
%
%
*/
MagickExport MagickBooleanType IsAccessible(const char *path)
{
  int
    status;

  struct stat
    file_info;

  if ((path == (const char *) NULL) || (*path == '\0'))
    return(MagickFalse);
  status=stat(path,&file_info);
  if (status != 0)
    return(MagickFalse);
  if (S_ISREG(file_info.st_mode) == 0)
    return(MagickFalse);
  if (access(path,F_OK) != 0)
    return(MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  I s D i r e c t o r y                                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsDirectory() returns -1 if the directory does not exist,  1 is returned
%  if the path represents a directory otherwise 0.
%
%  The format of the IsAccessible method is:
%
%      int IsDirectory(const char *path)
%
%  A description of each parameter follows.
%
%   o  path:  The directory path.
%
%
*/
static int IsDirectory(const char *path)
{
#if !defined(X_OK)
#define X_OK  1
#endif

  int
    status;

  struct stat
    file_info;

  if ((path == (const char *) NULL) || (*path == '\0'))
    return(MagickFalse);
  status=stat(path,&file_info);
  if (status != 0)
    return(-1);
  if (S_ISDIR(file_info.st_mode) == 0)
    return(0);
  if (access(path,X_OK) != 0)
    return(0);
  return(1);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   L i s t F i l e s                                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListFiles() reads the directory specified and returns a list of filenames
%  contained in the directory sorted in ascending alphabetic order.
%
%  The format of the ListFiles function is:
%
%      char **ListFiles(const char *directory,const char *pattern,
%        long *number_entries)
%
%  A description of each parameter follows:
%
%    o filelist: Method ListFiles returns a list of filenames contained
%      in the directory.  If the directory specified cannot be read or it is
%      a file a NULL list is returned.
%
%    o directory: Specifies a pointer to a text string containing a directory
%      name.
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_entries:  This integer returns the number of filenames in the
%      list.
%
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int FileCompare(const void *x,const void *y)
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

MagickExport char **ListFiles(const char *directory,const char *pattern,
  unsigned long *number_entries)
{
  char
    **filelist;

  DIR
    *current_directory;

  struct dirent
    *entry;

  unsigned long
    max_entries;

  /*
    Open directory.
  */
  assert(directory != (const char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",directory);
  assert(pattern != (const char *) NULL);
  assert(number_entries != (unsigned long *) NULL);
  *number_entries=0;
  current_directory=opendir(directory);
  if (current_directory == (DIR *) NULL)
    return((char **) NULL);
  /*
    Allocate filelist.
  */
  max_entries=2048;
  filelist=(char **) AcquireMagickMemory((size_t)
    max_entries*sizeof(*filelist));
  if (filelist == (char **) NULL)
    {
      (void) closedir(current_directory);
      return((char **) NULL);
    }
  /*
    Save the current and change to the new directory.
  */
  entry=readdir(current_directory);
  while (entry != (struct dirent *) NULL)
  {
    if (*entry->d_name == '.')
      {
        entry=readdir(current_directory);
        continue;
      }
    if ((IsDirectory(entry->d_name) > 0) ||
        (GlobExpression(entry->d_name,pattern) != MagickFalse))
      {
        if (*number_entries >= max_entries)
          {
            /*
              Extend the file list.
            */
            max_entries<<=1;
            filelist=(char **) ResizeMagickMemory(filelist,(size_t)
              max_entries*sizeof(*filelist));
            if (filelist == (char **) NULL)
              {
                (void) closedir(current_directory);
                return((char **) NULL);
              }
          }
#if defined(vms)
        {
          register char
            *p;

          p=strchr(entry->d_name,';');
          if (p)
            *p='\0';
          if (*number_entries > 0)
            if (LocaleCompare(entry->d_name,filelist[*number_entries-1]) == 0)
              {
                entry=readdir(current_directory);
                continue;
              }
        }
#endif
        filelist[*number_entries]=(char *) AcquireString(entry->d_name);
        if (IsDirectory(entry->d_name) > 0)
          (void) ConcatenateMagickString(filelist[*number_entries],
            DirectorySeparator,MaxTextExtent);
        (*number_entries)++;
      }
    entry=readdir(current_directory);
  }
  (void) closedir(current_directory);
  /*
    Sort filelist in ascending order.
  */
  qsort((void *) filelist,(size_t) *number_entries,sizeof(*filelist),
    FileCompare);
  return(filelist);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  M u l t i l i n e C e n s u s                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  MultilineCensus() returns the number of lines within a label.  A line is
%  represented by a \n character.
%
%  The format of the MultilineCenus method is:
%
%      unsigned long MultilineCensus(const char *label)
%
%  A description of each parameter follows.
%
%   o  label:  This character string is the label.
%
%
*/
MagickExport unsigned long MultilineCensus(const char *label)
{
  unsigned long
    number_lines;

  /*
    Determine the number of lines within this label.
  */
  if (label == (char *) NULL)
    return(0);
  for (number_lines=1; *label != '\0'; label++)
    if (*label == '\n')
      number_lines++;
  return(number_lines);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   S y s t e m C o m m a n d                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  SystemCommand() executes the specified command and waits until it
%  terminates.  The returned value is the exit status of the command.
%
%  The format of the SystemCommand method is:
%
%      int SystemCommand(const MagickBooleanType verbose,const char *command)
%
%  A description of each parameter follows:
%
%    o verbose: An MagickBooleanType other than 0 prints the executed command
%      before it is invoked.
%
%    o command: This string is the command to execute.
%
%
*/
MagickExport int SystemCommand(const MagickBooleanType verbose,
  const char *command)
{
  int
    status;

  if (verbose != MagickFalse)
    (void) fputs(command,stdout);
#if defined(POSIX)
  status=system(command);
#elif defined(__WINDOWS__)
  status=NTSystemCommand(command);
#elif defined(macintosh)
  status=MACSystemCommand(command);
#elif defined(vms)
  status=system(command);
#else
#  error No suitable system() method.
#endif
  if (status < 0)
    {
      ExceptionInfo
        exception;

      GetExceptionInfo(&exception);
      (void) ThrowMagickException(&exception,GetMagickModule(),DelegateError,
        "`%s': %s",command,strerror(errno));
      CatchException(&exception);
      DestroyExceptionInfo(&exception);
    }
  return(status);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   T r a n s l a t e T e x t                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  TranslateText() replaces any embedded formatting characters with the
%  appropriate image attribute and returns the translated text.
%
%  The format of the TranslateText method is:
%
%      char *TranslateText(const ImageInfo *image_info,Image *image,
%        const char *embed_text)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o image: The image.
%
%    o embed_text: The address of a character string containing the embedded
%      formatting characters.
%
%
*/
MagickExport char *TranslateText(const ImageInfo *image_info,Image *image,
  const char *embed_text)
{
  char
    filename[MaxTextExtent],
    *text,
    *translate_text;

  const ImageAttribute
    *attribute;

  ImageInfo
    *text_info;

  register char
    *p,
    *q;

  register long
    i;

  size_t
    length;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((embed_text == (const char *) NULL) || (*embed_text == '\0'))
    return((char *) NULL);
  text=(char *) embed_text;
  if ((*text == '@') &&
      ((*(text+1) == '-') || (IsAccessible(text+1) != MagickFalse)))
    {
      text=FileToString(text+1,~0,&image->exception);
      if (text == (char *) NULL)
        return((char *) NULL);
    }
  /*
    Translate any embedded format characters.
  */
  text_info=CloneImageInfo(image_info);
  translate_text=AcquireString(text);
  length=strlen(text)+MaxTextExtent;
  p=text;
  for (q=translate_text; *p != '\0'; p++)
  {
    *q='\0';
    if ((size_t) (q-translate_text+MaxTextExtent) >= length)
      {
        length<<=1;
        translate_text=(char *) ResizeMagickMemory(translate_text,
          (length+MaxTextExtent)*sizeof(*translate_text));
        if (translate_text == (char *) NULL)
          break;
        q=translate_text+strlen(translate_text);
      }
    /*
      Process formatting characters in text.
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
    if (*p == '\\')
      {
        p++;
        *q++=(*p++);
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
      case 'O':
      {
        (void) FormatMagickString(q,MaxTextExtent,"%+ld%+ld",image->page.x,
          image->page.y);
        while (*q != '\0')
          q++;
        break;
      }
      case 'P':
      {
        (void) FormatMagickString(q,MaxTextExtent,"%lux%lu",image->page.width,
          image->page.height);
        while (*q != '\0')
          q++;
        break;
      }
      case 'b':
      {
        char
          format[MaxTextExtent];

        MagickSizeType
          length;

        length=GetBlobSize(image);
        (void) FormatMagickString(format,MaxTextExtent,"%lu",
          (unsigned long) length);
        if (length != (MagickSizeType) ((size_t) length))
          FormatSize(length,format);
        (void) ConcatenateMagickString(q,format,MaxTextExtent);
        while (*q != '\0')
          q++;
        break;
      }
      case 'c':
      {
        attribute=GetImageAttribute(image,"Comment");
        if (attribute == (const ImageAttribute *) NULL)
          break;
        (void) CopyMagickString(q,attribute->value,MaxTextExtent);
        q+=strlen(attribute->value);
        break;
      }
      case 'd':
      case 'e':
      case 'f':
      case 't':
      {
        /*
          Label segment is the base of the filename.
        */
        if (strlen(image->magick_filename) == 0)
          break;
        switch (*p)
        {
          case 'd':
          {
            GetPathComponent(image->magick_filename,HeadPath,filename);
            (void) CopyMagickString(q,filename,MaxTextExtent);
            q+=strlen(filename);
            break;
          }
          case 'e':
          {
            GetPathComponent(image->magick_filename,ExtensionPath,filename);
            (void) CopyMagickString(q,filename,MaxTextExtent);
            q+=strlen(filename);
            break;
          }
          case 'f':
          {
            GetPathComponent(image->magick_filename,TailPath,filename);
            (void) CopyMagickString(q,filename,MaxTextExtent);
            q+=strlen(filename);
            break;
          }
          case 't':
          {
            GetPathComponent(image->magick_filename,BasePath,filename);
            (void) CopyMagickString(q,filename,MaxTextExtent);
            q+=strlen(filename);
            break;
          }
        }
        break;
      }
      case 'g':
      {
        (void) FormatMagickString(q,MaxTextExtent,"%lux%lu%+ld%+ld",
          image->page.width,image->page.height,image->page.x,image->page.y);
        while (*q != '\0')
          q++;
        break;
      }
      case 'h':
      {
        (void) FormatMagickString(q,MaxTextExtent,"%lu",
          image->rows != 0 ? image->rows : image->magick_rows);
        while (*q != '\0')
          q++;
        break;
      }
      case 'i':
      {
        (void) CopyMagickString(q,image->filename,MaxTextExtent);
        q+=strlen(image->filename);
        break;
      }
      case 'k':
      {
        (void) FormatMagickString(q,MaxTextExtent,"%lu",
          GetNumberColors(image,(FILE *) NULL,&image->exception));
        while (*q != '\0')
          q++;
        break;
      }
      case 'l':
      {
        attribute=GetImageAttribute(image,"label");
        if (attribute == (const ImageAttribute *) NULL)
          break;
        (void) CopyMagickString(q,attribute->value,MaxTextExtent);
        q+=strlen(attribute->value);
        break;
      }
      case 'm':
      {
        (void) CopyMagickString(q,image->magick,MaxTextExtent);
        q+=strlen(image->magick);
        break;
      }
      case 'n':
      {
        (void) FormatMagickString(q,MaxTextExtent,"%lu",(unsigned long)
          GetImageListLength(image));
        while (*q != '\0')
          q++;
        break;
      }
      case 'o':
      {
        (void) CopyMagickString(q,text_info->filename,MaxTextExtent);
        q+=strlen(text_info->filename);
        break;
      }
      case 'p':
      {
        register const Image
          *p;

        unsigned long
          page;

        p=image;
        for (page=1; GetPreviousImageInList(p) != (Image *) NULL; page++)
          p=GetPreviousImageInList(p);
        (void) FormatMagickString(q,MaxTextExtent,"%lu",page);
        while (*q != '\0')
          q++;
        break;
      }
      case 'q':
      {
        (void) FormatMagickString(q,MaxTextExtent,"%lu",image->depth);
        while (*q != '\0')
          q++;
        break;
      }
      case 'r':
      {
        ColorspaceType
          colorspace;

        colorspace=image->colorspace;
        if (IsGrayImage(image,&image->exception) != MagickFalse)
          colorspace=GRAYColorspace;
        (void) FormatMagickString(q,MaxTextExtent,"%s%s%s",
          MagickOptionToMnemonic(MagickClassOptions,(long) image->storage_class),
          MagickOptionToMnemonic(MagickColorspaceOptions,(long) colorspace),
          image->matte != MagickFalse ? "Matte" : "");
        while (*q != '\0')
          q++;
        break;
      }
      case 's':
      {
        (void) FormatMagickString(q,MaxTextExtent,"%lu",image->scene);
        if (text_info->number_scenes != 0)
          (void) FormatMagickString(q,MaxTextExtent,"%lu",text_info->scene);
        while (*q != '\0')
          q++;
        break;
      }
      case 'u':
      {
        (void) CopyMagickString(filename,text_info->unique,MaxTextExtent);
        (void) CopyMagickString(q,filename,MaxTextExtent);
        q+=strlen(filename);
        break;
      }
      case 'w':
      {
        (void) FormatMagickString(q,MaxTextExtent,"%lu",
          image->columns != 0 ? image->columns : image->magick_columns);
        while (*q != '\0')
          q++;
        break;
      }
      case 'x':
      {
        (void) FormatMagickString(q,MaxTextExtent,"%g %s",image->x_resolution,
          MagickOptionToMnemonic(MagickResolutionOptions,(long) image->units));
        while (*q != '\0')
          q++;
        break;
      }
      case 'y':
      {
        (void) FormatMagickString(q,MaxTextExtent,"%g %s",image->y_resolution,
          MagickOptionToMnemonic(MagickResolutionOptions,(long) image->units));
        while (*q != '\0')
          q++;
        break;
      }
      case 'z':
      {
        (void) FormatMagickString(q,MaxTextExtent,"%lu",
          GetImageDepth(image,&image->exception));
        while (*q != '\0')
          q++;
        break;
      }
      case '[':
      {
        char
          key[MaxTextExtent];

        MagickOffsetType
          offset;

        if (strchr(p,']') == (char *) NULL)
          break;
        p++;
        for (i=0; (i < (long) MaxTextExtent) && (*p != ']'); i++)
          key[i]=(*p++);
        key[i]='\0';
        attribute=GetImageAttribute(image,key);
        if (attribute != (const ImageAttribute *) NULL)
          {
            offset=(MagickOffsetType) strlen(attribute->value);
            if ((size_t) (q-translate_text+offset+1) >= length)
              {
                length+=offset;
                translate_text=(char *) ResizeMagickMemory(translate_text,
                  (length+MaxTextExtent)*sizeof(*translate_text));
                if (translate_text == (char *) NULL)
                  break;
                q=translate_text+strlen(translate_text);
              }
            (void) strcpy(q,attribute->value);
            q+=offset;
            break;
          }
        attribute=GetImageInfoAttribute(text_info,image,key);
        if (attribute == (const ImageAttribute *) NULL)
          break;
        offset=(MagickOffsetType) strlen(attribute->value);
        if ((size_t) (q-translate_text+offset+1) >= length)
          {
            length+=offset;
            translate_text=(char *) ResizeMagickMemory(translate_text,
              (length+MaxTextExtent)*sizeof(*translate_text));
            if (translate_text == (char *) NULL)
              break;
            q=translate_text+strlen(translate_text);
          }
        (void) strcpy(q,attribute->value);
        q+=offset;
        break;
      }
      case '@':
      {
        RectangleInfo
          page;

        page=GetImageBoundingBox(image,&image->exception);
        (void) FormatMagickString(q,MaxTextExtent,"%lux%lu%+ld%+ld",
          page.width,page.height,page.x,page.y);
        while (*q != '\0')
          q++;
        break;
      }
      case '#':
      {
        (void) SignatureImage(image);
        attribute=GetImageAttribute(image,"Signature");
        if (attribute == (const ImageAttribute *) NULL)
          break;
        (void) CopyMagickString(q,attribute->value,MaxTextExtent);
        q+=strlen(attribute->value);
        break;
      }
      case '%':
      {
        *q++=(*p);
        break;
      }
      default:
      {
        *q++='%';
        *q++=(*p);
        break;
      }
    }
  }
  *q='\0';
  text_info=DestroyImageInfo(text_info);
  if (text != (char *) embed_text)
    text=(char *) RelinquishMagickMemory(text);
  (void) SubstituteString(&translate_text,"&lt;","<");
  (void) SubstituteString(&translate_text,"&gt;",">");
  return(translate_text);
}
