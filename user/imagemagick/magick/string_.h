/*
  Copyright 1999-2005 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.
  
  You may not use this file except in compliance with the License.
  obtain a copy of the License at
  
    http://www.imagemagick.org/script/license.php
  
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  ImageMagick string methods.
*/
#ifndef _MAGICK_STRING_H_
#define _MAGICK_STRING_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <stdarg.h>
#include "magick/exception.h"

typedef struct _StringInfo
{
  char
    path[MaxTextExtent];

  unsigned char
    *datum;

  size_t
    length;

  unsigned long
    signature;
} StringInfo;

extern MagickExport char
  *AcquireString(const char *),
  *CloneString(char **,const char *),
  *ConstantString(char *),
  *DestroyString(char *),
  **DestroyStringList(char **),
  *EscapeString(const char *,const char),
  *FileToString(const char *,const size_t,ExceptionInfo *),
  **StringToArgv(const char *,int *),
  **StringToList(const char *),
  *StringInfoToString(const StringInfo *);

extern MagickExport double
  StringToDouble(const char *,const double);

extern MagickExport long
  FormatMagickString(char *,const size_t,const char *,...)
    magick_attribute((format (printf,3,4))),
  FormatMagickStringList(char *,const size_t,const char *,va_list)
    magick_attribute((format (printf,3,0))),
  LocaleCompare(const char *,const char *),
  LocaleNCompare(const char *,const char *,const size_t);

extern MagickExport MagickBooleanType
  ConcatenateString(char **,const char *),
  SubstituteString(char **,const char *,const char *);

extern MagickExport int
  CompareStringInfo(const StringInfo *,const StringInfo *);

extern MagickExport size_t
  ConcatenateMagickString(char *,const char *,const size_t),
  CopyMagickString(char *,const char *,const size_t);

extern MagickExport StringInfo
  *AcquireStringInfo(const size_t),
  *CloneStringInfo(const StringInfo *),
  *ConfigureFileToStringInfo(const char *),
  *DestroyStringInfo(StringInfo *),
  *FileToStringInfo(const char *,const size_t,ExceptionInfo *),
  *SplitStringInfo(StringInfo *,const size_t),
  *StringToStringInfo(const char *);

extern MagickExport void
  ConcatenateStringInfo(StringInfo *,const StringInfo *),
  LocaleLower(char *),
  LocaleUpper(char *),
  PrintStringInfo(FILE *file,const char *,const StringInfo *),
  ResetStringInfo(StringInfo *),
  SetStringInfoDatum(StringInfo *,const unsigned char *),
  SetStringInfo(StringInfo *,const StringInfo *),
  SetStringInfoLength(StringInfo *,const size_t),
  StripString(char *);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
