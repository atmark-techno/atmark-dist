/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                             M   M  SSSSS  L                                 %
%                             MM MM  SS     L                                 %
%                             M M M   SSS   L                                 %
%                             M   M     SS  L                                 %
%                             M   M  SSSSS  LLLLL                             %
%                                                                             %
%                                                                             %
%                    Execute Magick Scripting Language Scripts.               %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                             Leonard Rosenthol                               %
%                             William Radcliffe                               %
%                               December 2001                                 %
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
#include "magick/blob-private.h"
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/composite.h"
#include "magick/constitute.h"
#include "magick/decorate.h"
#include "magick/draw.h"
#include "magick/effect.h"
#include "magick/enhance.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/fx.h"
#include "magick/geometry.h"
#include "magick/image.h"
#include "magick/image-private.h"
#include "magick/list.h"
#include "magick/log.h"
#include "magick/magick.h"
#include "magick/memory_.h"
#include "magick/option.h"
#include "magick/paint.h"
#include "magick/resize.h"
#include "magick/shear.h"
#include "magick/static.h"
#include "magick/string_.h"
#include "magick/transform.h"
#include "magick/utility.h"
#if defined(HasXML)
#  if defined(__WINDOWS__)
#    if defined(__MINGW32__)
#      define _MSC_VER
#    endif
#    include <win32config.h>
#  endif
#  include <libxml/parser.h>
#  include <libxml/xmlmemory.h>
#  include <libxml/parserInternals.h>
#  include <libxml/xmlerror.h>
#endif

/*
  Define Declatations.
*/
#define ThrowMSLException(severity,tag,reason) \
  (void) ThrowMagickException(msl_info->exception,GetMagickModule(),severity, \
    tag,"`%s'",reason);

/*
  Typedef declaractions.
*/
typedef struct _MSLGroupInfo
{
  unsigned long
    numImages;  /* how many images are in this group */
} MSLGroupInfo;

typedef struct _MSLInfo
{
  ExceptionInfo
    *exception;

  long
    n,

  nGroups;

  ImageInfo
    **image_info;

  DrawInfo
   **draw_info;

  Image
    **attributes,
    **image;

  MSLGroupInfo
    *group_info;

#if defined(HasXML)
  xmlParserCtxtPtr
    parser;

  xmlDocPtr
    document;
#endif
} MSLInfo;

/*
  Forward declarations.
*/
#if defined(HasXML)
static MagickBooleanType
  WriteMSLImage(const ImageInfo *,Image *);
#endif

#if defined(HasXML)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e a d M S L I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ReadMSLImage() reads a Magick Scripting Language file and returns it.
%  It allocates the memory necessary for the new Image structure and returns a
%  pointer to the new image.
%
%  The format of the ReadMSLImage method is:
%
%      Image *ReadMSLImage(const ImageInfo *image_info,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image_info: The image info.
%
%    o exception: return any errors or warnings in this structure.
%
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int MSLIsStandalone(void *context)
{
  MSLInfo
    *msl_info;

  /*
    Is this document tagged standalone?
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),"  SAX.MSLIsStandalone()");
  msl_info=(MSLInfo *) context;
  return(msl_info->document->standalone == 1);
}

static int MSLHasInternalSubset(void *context)
{
  MSLInfo
    *msl_info;

  /*
    Does this document has an internal subset?
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.MSLHasInternalSubset()");
  msl_info=(MSLInfo *) context;
  return(msl_info->document->intSubset != NULL);
}

static int MSLHasExternalSubset(void *context)
{
  MSLInfo
    *msl_info;

  /*
    Does this document has an external subset?
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.MSLHasExternalSubset()");
  msl_info=(MSLInfo *) context;
  return(msl_info->document->extSubset != NULL);
}

static void MSLInternalSubset(void *context,const xmlChar *name,
  const xmlChar *external_id,const xmlChar *system_id)
{
  MSLInfo
    *msl_info;

  /*
    Does this document has an internal subset?
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.internalSubset(%s %s %s)",name,
    (external_id != (const xmlChar *) NULL ? (char *) external_id : " "),
    (system_id != (const xmlChar *) NULL ? (char *) system_id : " "));
  msl_info=(MSLInfo *) context;
  (void) xmlCreateIntSubset(msl_info->document,name,external_id,system_id);
}

static xmlParserInputPtr MSLResolveEntity(void *context,
  const xmlChar *public_id,const xmlChar *system_id)
{
  MSLInfo
    *msl_info;

  xmlParserInputPtr
    stream;

  /*
    Special entity resolver, better left to the parser, it has more
    context than the application layer.  The default behaviour is to
    not resolve the entities, in that case the ENTITY_REF nodes are
    built in the structure (and the parameter values).
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.resolveEntity(%s, %s)",
    (public_id != (const xmlChar *) NULL ? (char *) public_id : "none"),
    (system_id != (const xmlChar *) NULL ? (char *) system_id : "none"));
  msl_info=(MSLInfo *) context;
  stream=xmlLoadExternalEntity((const char *) system_id,(const char *)
    public_id,msl_info->parser);
  return(stream);
}

static xmlEntityPtr MSLGetEntity(void *context,const xmlChar *name)
{
  MSLInfo
    *msl_info;

  /*
    Get an entity by name.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.MSLGetEntity(%s)",(char *) name);
  msl_info=(MSLInfo *) context;
  return(xmlGetDocEntity(msl_info->document,name));
}

static xmlEntityPtr MSLGetParameterEntity(void *context,const xmlChar *name)
{
  MSLInfo
    *msl_info;

  /*
    Get a parameter entity by name.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.getParameterEntity(%s)",(char *) name);
  msl_info=(MSLInfo *) context;
  return(xmlGetParameterEntity(msl_info->document,name));
}

static void MSLEntityDeclaration(void *context,const xmlChar *name,int type,
  const xmlChar *public_id,const xmlChar *system_id,xmlChar *content)
{
  MSLInfo
    *msl_info;

  /*
    An entity definition has been parsed.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.entityDecl(%s, %d, %s, %s, %s)",name,type,
    public_id != (const xmlChar *) NULL ? (char *) public_id : "none",
    system_id != (const xmlChar *) NULL ? (char *) system_id : "none",content);
  msl_info=(MSLInfo *) context;
  if (msl_info->parser->inSubset == 1)
    (void) xmlAddDocEntity(msl_info->document,name,type,public_id,system_id,
      content);
  else
    if (msl_info->parser->inSubset == 2)
      (void) xmlAddDtdEntity(msl_info->document,name,type,public_id,system_id,
        content);
}

static void MSLAttributeDeclaration(void *context,const xmlChar *element,
  const xmlChar *name,int type,int value,const xmlChar *default_value,
  xmlEnumerationPtr tree)
{
  MSLInfo
    *msl_info;

  xmlChar
    *fullname,
    *prefix;

  xmlParserCtxtPtr
    parser;

  /*
    An attribute definition has been parsed.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.attributeDecl(%s, %s, %d, %d, %s, ...)\n",element,name,type,value,
    default_value);
  msl_info=(MSLInfo *) context;
  fullname=(xmlChar *) NULL;
  prefix=(xmlChar *) NULL;
  parser=msl_info->parser;
  fullname=(xmlChar *) xmlSplitQName(parser,name,&prefix);
  if (parser->inSubset == 1)
    (void) xmlAddAttributeDecl(&parser->vctxt,msl_info->document->intSubset,
      element,fullname,prefix,(xmlAttributeType) type,
      (xmlAttributeDefault) value,default_value,tree);
  else
    if (parser->inSubset == 2)
      (void) xmlAddAttributeDecl(&parser->vctxt,msl_info->document->extSubset,
        element,fullname,prefix,(xmlAttributeType) type,
        (xmlAttributeDefault) value,default_value,tree);
  if (prefix != (xmlChar *) NULL)
    xmlFree(prefix);
  if (fullname != (xmlChar *) NULL)
    xmlFree(fullname);
}

static void MSLElementDeclaration(void *context,const xmlChar *name,int type,
  xmlElementContentPtr content)
{
  MSLInfo
    *msl_info;

  xmlParserCtxtPtr
    parser;

  /*
    An element definition has been parsed.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.elementDecl(%s, %d, ...)",name,type);
  msl_info=(MSLInfo *) context;
  parser=msl_info->parser;
  if (parser->inSubset == 1)
    (void) xmlAddElementDecl(&parser->vctxt,msl_info->document->intSubset,
      name,(xmlElementTypeVal) type,content);
  else
    if (parser->inSubset == 2)
      (void) xmlAddElementDecl(&parser->vctxt,msl_info->document->extSubset,
        name,(xmlElementTypeVal) type,content);
}

static void MSLNotationDeclaration(void *context,const xmlChar *name,
  const xmlChar *public_id,const xmlChar *system_id)
{
  MSLInfo
    *msl_info;

  xmlParserCtxtPtr
    parser;

  /*
    What to do when a notation declaration has been parsed.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.notationDecl(%s, %s, %s)",name,
    public_id != (const xmlChar *) NULL ? (char *) public_id : "none",
    system_id != (const xmlChar *) NULL ? (char *) system_id : "none");
  msl_info=(MSLInfo *) context;
  parser=msl_info->parser;
  if (parser->inSubset == 1)
    (void) xmlAddNotationDecl(&parser->vctxt,msl_info->document->intSubset,
      name,public_id,system_id);
  else
    if (parser->inSubset == 2)
      (void) xmlAddNotationDecl(&parser->vctxt,msl_info->document->intSubset,
        name,public_id,system_id);
}

static void MSLUnparsedEntityDeclaration(void *context,const xmlChar *name,
  const xmlChar *public_id,const xmlChar *system_id,const xmlChar *notation)
{
  MSLInfo
    *msl_info;

  /*
    What to do when an unparsed entity declaration is parsed.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.unparsedEntityDecl(%s, %s, %s, %s)",name,
    public_id != (const xmlChar *) NULL ? (char *) public_id : "none",
    system_id != (const xmlChar *) NULL ? (char *) system_id : "none",notation);
  msl_info=(MSLInfo *) context;
  (void) xmlAddDocEntity(msl_info->document,name,
    XML_EXTERNAL_GENERAL_UNPARSED_ENTITY,public_id,system_id,notation);

}

static void MSLSetDocumentLocator(void *context,xmlSAXLocatorPtr location)
{
  MSLInfo
    *msl_info;

  /*
    Receive the document locator at startup, actually xmlDefaultSAXLocator.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.setDocumentLocator()\n");
  msl_info=(MSLInfo *) context;
}

static void MSLStartDocument(void *context)
{
  MSLInfo
    *msl_info;

  xmlParserCtxtPtr
    parser;

  /*
    Called when the document start being processed.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.startDocument()");
  msl_info=(MSLInfo *) context;
  parser=msl_info->parser;
  msl_info->document=xmlNewDoc(parser->version);
  if (msl_info->document == (xmlDocPtr) NULL)
    return;
  if (parser->encoding == NULL)
    msl_info->document->encoding=NULL;
  else
    msl_info->document->encoding=xmlStrdup(parser->encoding);
  msl_info->document->standalone=parser->standalone;
}

static void MSLEndDocument(void *context)
{
  MSLInfo
    *msl_info;

  /*
    Called when the document end has been detected.
  */
  msl_info=(MSLInfo *) context;
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),"  SAX.endDocument()");
}

static void MSLPushImage(MSLInfo *msl_info,Image *image)
{
  const ImageAttribute
    *attribute;

  long
    n;

  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  assert(msl_info != (MSLInfo *) NULL);
  msl_info->n++;
  n=msl_info->n;
  msl_info->image_info=(ImageInfo **) ResizeMagickMemory(msl_info->image_info,
    (n+1)*sizeof(*msl_info->image_info));
  msl_info->draw_info=(DrawInfo **)
    ResizeMagickMemory(msl_info->draw_info,(n+1)*sizeof(*msl_info->draw_info));
  msl_info->attributes=(Image **) ResizeMagickMemory(msl_info->attributes,
    (n+1)*sizeof(*msl_info->attributes));
  msl_info->image=(Image **)
    ResizeMagickMemory(msl_info->image,(n+1)*sizeof(*msl_info->image));
  if ((msl_info->image_info == (ImageInfo **) NULL) ||
    (msl_info->draw_info == (DrawInfo **) NULL) ||
    (msl_info->attributes == (Image **) NULL) ||
    (msl_info->image == (Image **) NULL))
      ThrowMSLException(ResourceLimitFatalError,"MemoryAllocationFailed","msl");
  msl_info->image_info[n]=CloneImageInfo(msl_info->image_info[n-1]);
  msl_info->draw_info[n]=
    CloneDrawInfo(msl_info->image_info[n-1], msl_info->draw_info[n-1]);
  msl_info->attributes[n]=AllocateImage(msl_info->image_info[n]);
  msl_info->image[n]=(Image *) image;
  if ((msl_info->image_info[n] == (ImageInfo *) NULL) ||
    (msl_info->attributes[n] == (Image *) NULL))
    ThrowMSLException(ResourceLimitFatalError,"MemoryAllocationFailed","msl");
  if ( msl_info->nGroups )
    msl_info->group_info[msl_info->nGroups-1].numImages++;
  if (image != (Image *) NULL)
    {
      ResetImageAttributeIterator(image);
    	attribute=GetNextImageAttribute(image);
      while (attribute != (const ImageAttribute *) NULL)
      {
        (void) SetImageAttribute(msl_info->attributes[n],attribute->key,
          attribute->value);
        attribute=GetNextImageAttribute(image);
      }
    }
}

static void MSLPopImage(MSLInfo *msl_info)
{
  /*
    only dispose of images when they aren't in a group
  */
  if ( msl_info->nGroups == 0 )
  {
    if (msl_info->image[msl_info->n] != (Image *) NULL)
      msl_info->image[msl_info->n]=DestroyImage(msl_info->image[msl_info->n]);
    msl_info->attributes[msl_info->n]=DestroyImage(msl_info->attributes[msl_info->n]);
    msl_info->image_info[msl_info->n]=DestroyImageInfo(msl_info->image_info[msl_info->n]);
    msl_info->n--;
  }
}

static void MSLStartElement(void *context,const xmlChar *name,
  const xmlChar **attributes)
{
  char
    key[MaxTextExtent],
    *value = NULL;

  const char
    *keyword;

  ExceptionInfo
    exception;

  GeometryInfo
    geometry_info;

  long
    n,
    x,
    y;

  MSLInfo
    *msl_info;

  register long
    i, j;

  unsigned long
    height,
    width;

  /*
    Called when an opening tag has been processed.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.startElement(%s",name);
  GetExceptionInfo(&exception);
  msl_info=(MSLInfo *) context;
  n=msl_info->n;
  keyword=(const char *) NULL;
  switch (*name)
  {
    case 'B':
    case 'b':
    {
      if (LocaleCompare((char *) name, "blur") == 0)
      {
        double  radius = 0.0,
            sigma = 1.0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        /*
        NOTE: blur can have no attributes, since we use all the defaults!
        */
        if (attributes != (const xmlChar **) NULL)
        {
          for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
          {
          keyword=(const char *) attributes[i++];
          CloneString(&value,TranslateText(msl_info->image_info[n],
            msl_info->attributes[n],(char *) attributes[i]));
          switch (*keyword)
          {
            case 'R':
            case 'r':
            {
              if (LocaleCompare(keyword, "radius") == 0)
              {
                radius = atof( value );
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            case 'S':
            case 's':
            {
              if (LocaleCompare(keyword,"sigma") == 0)
              {
                sigma = atoi( value );
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            default:
            {
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
          }
          }
        }

        /*
          blur image.
        */
        {
        Image
          *newImage;

        newImage=BlurImage(msl_info->image[n],radius,sigma,&msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        break;
        }
      }
      else if (LocaleCompare((char *) name,"border") == 0)
      {
        /* init the values */
        width = height = 6;  /* this is the value that Magick++ uses */
        x = y = 0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        /*
        NOTE: border can have no attributes, since we use all the defaults!
        */
        if (attributes != (const xmlChar **) NULL)
        {
          for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
          {
          keyword=(const char *) attributes[i++];
          CloneString(&value,TranslateText(msl_info->image_info[n],
            msl_info->attributes[n],(char *) attributes[i]));
          switch (*keyword)
          {
            case 'F':
            case'f':
            {
              if (LocaleCompare(keyword, "fill") == 0)
              {
                (void) QueryColorDatabase(value,
                  &msl_info->image[n]->border_color,&exception);
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            case 'G':
            case 'g':
            {
            if (LocaleCompare(keyword,"geometry") == 0)
              {
              (void) ParseMetaGeometry(value,&x,&y,&width,&height);
              break;
              }
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
            }
            case 'H':
            case 'h':
            {
            if (LocaleCompare(keyword,"height") == 0)
              {
              height = atoi( value );
              break;
              }
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
            }
            case 'W':
            case 'w':
            {
            if (LocaleCompare(keyword,"width") == 0)
              {
              width = atoi( value );
              break;
              }
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
            }
            default:
            {
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
            }
          }
          }
        }

        /*
          border image.
        */
        {
        Image
          *border_image;
        RectangleInfo
          rectInfo;

        rectInfo.height = height;
        rectInfo.width = width;
        rectInfo.x = x;
        rectInfo.y = y;

        border_image=BorderImage(msl_info->image[n],&rectInfo, &msl_info->image[n]->exception);
        if (border_image == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=border_image;
        }

        break;
      }
      ThrowMSLException(OptionError,"UnrecognizedElement",(const char *) name);
    }
  case 'C':
  case 'c':
    {
      if (LocaleCompare((char *) name, "charcoal") == 0)
      {
        double  radius = 0.0,
            sigma = 1.0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        /*
        NOTE: charcoal can have no attributes, since we use all the defaults!
        */
        if (attributes != (const xmlChar **) NULL)
        {
          for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
          {
          keyword=(const char *) attributes[i++];
          CloneString(&value,TranslateText(msl_info->image_info[n],
            msl_info->attributes[n],(char *) attributes[i]));
          switch (*keyword)
          {
            case 'R':
            case 'r':
            {
              if (LocaleCompare(keyword, "radius") == 0)
              {
                radius = atof( value );
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            case 'S':
            case 's':
            {
              if (LocaleCompare(keyword,"sigma") == 0)
              {
                sigma = atoi( value );
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            default:
            {
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
          }
          }
        }

        /*
          charcoal image.
        */
        {
        Image
          *newImage;

        newImage=CharcoalImage(msl_info->image[n],radius,sigma,&msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        break;
        }
      }
      else if (LocaleCompare((char *) name,"chop") == 0)
      {
        /* init the values */
        width=msl_info->image[n]->columns;
        height=msl_info->image[n]->rows;
        x = y = 0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
        break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
        keyword=(const char *) attributes[i++];
        CloneString(&value,TranslateText(msl_info->image_info[n],
          msl_info->attributes[n],(char *) attributes[i]));
        switch (*keyword)
        {
          case 'G':
          case 'g':
          {
          if (LocaleCompare(keyword,"geometry") == 0)
            {
            (void) ParseMetaGeometry(value,&x,&y,&width,&height);
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          case 'H':
          case 'h':
          {
          if (LocaleCompare(keyword,"height") == 0)
            {
            height = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          case 'W':
          case 'w':
          {
          if (LocaleCompare(keyword,"width") == 0)
            {
            width = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          case 'X':
          case 'x':
          {
          if (LocaleCompare(keyword,"x") == 0)
            {
            x = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          case 'Y':
          case 'y':
          {
          if (LocaleCompare(keyword,"y") == 0)
            {
            y = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          default:
          {
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
        }
        }

        /*
          process image.
        */
        {
        Image
          *newImage;
        RectangleInfo
          rectInfo;

        rectInfo.height = height;
        rectInfo.width = width;
        rectInfo.x = x;
        rectInfo.y = y;

        newImage=ChopImage(msl_info->image[n],&rectInfo, &msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        }

        break;
      }
      else if (LocaleCompare((char *) name, "composite") == 0 )
      {
        char
          composite_geometry[MaxTextExtent];

        Image *
          srcImage = (Image*)NULL;
        CompositeOperator
          compositeOp = OverCompositeOp;
        GravityType
          gravity = CenterGravity;

        RectangleInfo
          geometry;

        SetGeometry(msl_info->image[n],&geometry);

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
          break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
          keyword=(const char *) attributes[i++];
          CloneString(&value,TranslateText(msl_info->image_info[n],
            msl_info->attributes[n],(char *) attributes[i]));
          switch (*keyword)
          {
            case 'C':
            case 'c':
            {
              if (LocaleCompare(keyword, "compose") == 0)
              {
                if (LocaleCompare(value, "Over") == 0)
                  compositeOp = OverCompositeOp;
                else if (LocaleCompare(value, "In") == 0)
                  compositeOp = InCompositeOp;
                else if (LocaleCompare(value, "Out") == 0)
                  compositeOp = OutCompositeOp;
                else if (LocaleCompare(value, "Atop") == 0)
                  compositeOp = AtopCompositeOp;
                else if (LocaleCompare(value, "Xor") == 0)
                  compositeOp = XorCompositeOp;
                else if (LocaleCompare(value, "Plus") == 0)
                  compositeOp = PlusCompositeOp;
                else if (LocaleCompare(value, "Minus") == 0)
                  compositeOp = MinusCompositeOp;
                else if (LocaleCompare(value, "Add") == 0)
                  compositeOp = AddCompositeOp;
                else if (LocaleCompare(value, "Subtract") == 0)
                  compositeOp = SubtractCompositeOp;
                else if (LocaleCompare(value, "Difference") == 0)
                  compositeOp = DifferenceCompositeOp;
                else if (LocaleCompare(value, "Multiply") == 0)
                  compositeOp = MultiplyCompositeOp;
                else if (LocaleCompare(value, "Bumpmap") == 0)
                  compositeOp = BumpmapCompositeOp;
                else if (LocaleCompare(value, "Copy") == 0)
                  compositeOp = CopyCompositeOp;
                else if (LocaleCompare(value, "CopyRed") == 0)
                  compositeOp = CopyRedCompositeOp;
                else if (LocaleCompare(value, "CopyCyan") == 0)
                  compositeOp = CopyCyanCompositeOp;
                else if (LocaleCompare(value, "CopyGreen") == 0)
                  compositeOp = CopyGreenCompositeOp;
                else if (LocaleCompare(value, "CopyMagenta") == 0)
                  compositeOp = CopyMagentaCompositeOp;
                else if (LocaleCompare(value, "CopyBlue") == 0)
                  compositeOp = CopyBlueCompositeOp;
                else if (LocaleCompare(value, "CopyYellow") == 0)
                  compositeOp = CopyYellowCompositeOp;
                else if (LocaleCompare(value, "CopyOpacity") == 0)
                  compositeOp = CopyOpacityCompositeOp;
                else if (LocaleCompare(value, "CopyBlack") == 0)
                  compositeOp = CopyBlackCompositeOp;
                else if (LocaleCompare(value, "Dissolve") == 0)
                  compositeOp = DissolveCompositeOp;
                else if (LocaleCompare(value, "Clear") == 0)
                  compositeOp = ClearCompositeOp;
                else if (LocaleCompare(value, "Displace") == 0)
                  compositeOp = DisplaceCompositeOp;
                else if (LocaleCompare(value, "Modulate") == 0)
                  compositeOp = ModulateCompositeOp;
                else if (LocaleCompare(value, "Threshold") == 0)
                  compositeOp = ThresholdCompositeOp;
                else if (LocaleCompare(value, "Darken") == 0)
                  compositeOp = DarkenCompositeOp;
                else if (LocaleCompare(value, "Lighten") == 0)
                  compositeOp = LightenCompositeOp;
                else if (LocaleCompare(value, "Hue") == 0)
                  compositeOp = HueCompositeOp;
                else if (LocaleCompare(value, "Saturate") == 0)
                  compositeOp = SaturateCompositeOp;
                else if (LocaleCompare(value, "Colorize") == 0)
                  compositeOp = ColorizeCompositeOp;
                else if (LocaleCompare(value, "Luminize") == 0)
                  compositeOp = LuminizeCompositeOp;
                else if (LocaleCompare(value, "Screen") == 0)
                  compositeOp = ScreenCompositeOp;
                else if (LocaleCompare(value, "Overlay") == 0)
                  compositeOp = OverlayCompositeOp;
                else
                  ThrowMSLException(OptionError,"UnrecognizedComposeOperator",
                    value);
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            case 'G':
            case 'g':
            {
              if (LocaleCompare(keyword,"geometry") == 0)
              {
                (void) ParseGravityGeometry(msl_info->image[n],value,&geometry);
                break;
              }
              else if (LocaleCompare(keyword,"gravity") == 0)
              {
                long
                  i;

                i = ParseMagickOption(MagickGravityOptions,MagickFalse,value);
                if (i < 0)
                  ThrowMSLException(OptionError,"UnrecognizedComposeOperator",
                    value);
                gravity = (GravityType) i;
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            case 'I':
            case 'i':
            {
              if (LocaleCompare(keyword,"image") == 0)
              {
                for (j=0; j<msl_info->n;j++)
                {
                  const ImageAttribute *
                    theAttr = GetImageAttribute(msl_info->attributes[j], "id");
                  if (theAttr && LocaleCompare(theAttr->value, value) == 0)
                  {
                    srcImage = msl_info->image[j];
                    break;
                  }
                }
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            case 'X':
            case 'x':
            {
              if (LocaleCompare(keyword,"x") == 0)
              {
                geometry.x = atoi( value );
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            case 'Y':
            case 'y':
            {
              if (LocaleCompare(keyword,"y") == 0)
              {
                geometry.y = atoi( value );
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            default:
            {
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
          }
        }

        /*
          process image.
        */
        if (srcImage != (Image*)NULL)
        {
          unsigned int
            result;

          (void) FormatMagickString(composite_geometry,MaxTextExtent,
            "%lux%lu%+ld%+ld",msl_info->image[n]->columns,
            msl_info->image[n]->rows,geometry.x,geometry.y);
          (void) ParseGravityGeometry(msl_info->image[n],composite_geometry,
            &geometry);
          result = CompositeImage(msl_info->image[n], compositeOp, srcImage,                geometry.x, geometry.y);
          break;
        } else
          ThrowMSLException(OptionError,"no composite image defined",
            (char *) name);

        break;
      }
    else if (LocaleCompare((char *) name, "contrast") == 0)
    {
      MagickBooleanType
        sharpen = MagickFalse;

      if (msl_info->image[n] == (Image *) NULL)
      {
        ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
        break;
      }
      if (attributes == (const xmlChar **) NULL)
        break;
      for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
      {
        keyword=(const char *) attributes[i++];
        CloneString(&value,(char *) attributes[i]);
        (void) CopyMagickString(key,value,MaxTextExtent);
        switch (*keyword)
        {
          case 'S':
          case 's':
          {
            if (LocaleCompare(keyword,"sharpen") == 0)
            {
              sharpen = (MagickBooleanType) atoi( value );
              break;
            }
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
          }
          default:
          {
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
          }
        }
      }

      /* process image */
      {
        ContrastImage ( msl_info->image[n], sharpen );
  break;
      }
    }
      else if (LocaleCompare((char *) name,"crop") == 0)
      {
        /* init the values */
        width=msl_info->image[n]->columns;
        height=msl_info->image[n]->rows;
        x = y = 0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
        break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
        keyword=(const char *) attributes[i++];
        CloneString(&value,TranslateText(msl_info->image_info[n],
          msl_info->attributes[n],(char *) attributes[i]));
        switch (*keyword)
        {
          case 'G':
          case 'g':
          {
          if (LocaleCompare(keyword,"geometry") == 0)
            {
            (void) ParseMetaGeometry(value,&x,&y,&width,&height);
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          case 'H':
          case 'h':
          {
          if (LocaleCompare(keyword,"height") == 0)
            {
            height = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          case 'W':
          case 'w':
          {
          if (LocaleCompare(keyword,"width") == 0)
            {
            width = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          case 'X':
          case 'x':
          {
          if (LocaleCompare(keyword,"x") == 0)
            {
            x = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          case 'Y':
          case 'y':
          {
          if (LocaleCompare(keyword,"y") == 0)
            {
            y = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          default:
          {
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
        }
        }

        /*
          process image.
        */
        {
        Image
          *newImage;
        RectangleInfo
          rectInfo;

        rectInfo.height = height;
        rectInfo.width = width;
        rectInfo.x = x;
        rectInfo.y = y;

        newImage=CropImage(msl_info->image[n],&rectInfo, &msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        }

        break;
      }
      ThrowMSLException(OptionError,"UnrecognizedElement",(const char *) name);
    }
  case 'D':
  case 'd':
    {
      if (LocaleCompare((char *) name, "despeckle") == 0)
      {
        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }

        /* no attributes here */

        /* process the image */
        {
          Image
            *newImage;

          newImage=DespeckleImage(msl_info->image[n],&msl_info->image[n]->exception);
          if (newImage == (Image *) NULL)
            break;
          msl_info->image[n]=DestroyImage(msl_info->image[n]);
          msl_info->image[n]=newImage;
          break;
        }
      }
      ThrowMSLException(OptionError,"UnrecognizedElement",(const char *) name);
    }
  case 'E':
  case 'e':
    {
      if (LocaleCompare((char *) name, "edge") == 0)
      {
        double  radius = 0.0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        /*
        NOTE: edge can have no attributes, since we use all the defaults!
        */
        if (attributes != (const xmlChar **) NULL)
        {
          for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
          {
          keyword=(const char *) attributes[i++];
          CloneString(&value,TranslateText(msl_info->image_info[n],
            msl_info->attributes[n],(char *) attributes[i]));
          switch (*keyword)
          {
            case 'R':
            case 'r':
            {
              if (LocaleCompare(keyword, "radius") == 0)
              {
                radius = atof( value );
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            default:
            {
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
          }
          }
        }

        /*
          edge image.
        */
        {
        Image
          *newImage;

        newImage=EdgeImage(msl_info->image[n],radius,&msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        break;
        }
      }
      else if (LocaleCompare((char *) name, "emboss") == 0)
      {
        double  radius = 0.0,
            sigma = 1.0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        /*
        NOTE: emboss can have no attributes, since we use all the defaults!
        */
        if (attributes != (const xmlChar **) NULL)
        {
          for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
          {
          keyword=(const char *) attributes[i++];
          CloneString(&value,TranslateText(msl_info->image_info[n],
            msl_info->attributes[n],(char *) attributes[i]));
          switch (*keyword)
          {
            case 'R':
            case 'r':
            {
              if (LocaleCompare(keyword, "radius") == 0)
              {
                radius = atof( value );
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            case 'S':
            case 's':
            {
              if (LocaleCompare(keyword,"sigma") == 0)
              {
                sigma = atoi( value );
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            default:
            {
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
          }
          }
        }

        /*
          emboss image.
        */
        {
        Image
          *newImage;

        newImage=EmbossImage(msl_info->image[n],radius,sigma,&msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        break;
        }
      }
      else if (LocaleCompare((char *) name, "enhance") == 0)
      {
        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }

        /* no attributes here */

        /* process the image */
        {
          Image
            *newImage;

          newImage=EnhanceImage(msl_info->image[n],&msl_info->image[n]->exception);
          if (newImage == (Image *) NULL)
            break;
          msl_info->image[n]=DestroyImage(msl_info->image[n]);
          msl_info->image[n]=newImage;
          break;
        }
      }
      else if (LocaleCompare((char *) name, "equalize") == 0)
      {
        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }

        /* no attributes here */

        /* process the image */
        {
          EqualizeImage(msl_info->image[n]);  /* why doesn't this take an exception or return the changed image */
          break;
        }
      }
      ThrowMSLException(OptionError,"UnrecognizedElement",(const char *) name);
    }
  case 'F':
  case 'f':
    {
      if (LocaleCompare((char *) name, "flatten") == 0)
      {
        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }

        /* no attributes here */

        /* process the image */
        {
          Image
            *newImage;

          newImage=FlattenImages(msl_info->image[n],&msl_info->image[n]->exception);
          if (newImage == (Image *) NULL)
            break;
          msl_info->image[n]=DestroyImage(msl_info->image[n]);
          msl_info->image[n]=newImage;
          break;
        }
      }
      else if (LocaleCompare((char *) name, "flip") == 0)
      {
        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }

        /* no attributes here */

        /* process the image */
        {
          Image
            *newImage;

          newImage=FlipImage(msl_info->image[n],&msl_info->image[n]->exception);
          if (newImage == (Image *) NULL)
            break;
          msl_info->image[n]=DestroyImage(msl_info->image[n]);
          msl_info->image[n]=newImage;
          break;
        }
      }
      else if (LocaleCompare((char *) name, "flop") == 0)
      {
        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }

        /* no attributes here */

        /* process the image */
        {
          Image
            *newImage;

          newImage=FlopImage(msl_info->image[n],&msl_info->image[n]->exception);
          if (newImage == (Image *) NULL)
            break;
          msl_info->image[n]=DestroyImage(msl_info->image[n]);
          msl_info->image[n]=newImage;
          break;
        }
      }
      else if (LocaleCompare((char *) name,"frame") == 0)
      {
        /* init the values */
        width = height = 25;  /* these are the values that Magick++ uses */
        x = y = 6;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        /*
        NOTE: frame can have no attributes, since we use all the defaults!
        */
        if (attributes != (const xmlChar **) NULL)
        {
          for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
          {
          keyword=(const char *) attributes[i++];
          CloneString(&value,TranslateText(msl_info->image_info[n],
            msl_info->attributes[n],(char *) attributes[i]));
          switch (*keyword)
          {
            case 'F':
            case 'f':
            {
              if (LocaleCompare(keyword, "fill") == 0)
              {
                (void) QueryColorDatabase(value,
                  &msl_info->image[n]->matte_color,&exception);
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            }
            case 'G':
            case 'g':
            {
            if (LocaleCompare(keyword,"geometry") == 0)
              {
              (void) ParseMetaGeometry(value,&x,&y,&width,&height);
              break;
              }
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
            }
            case 'H':
            case 'h':
            {
            if (LocaleCompare(keyword,"height") == 0)
              {
              height = atoi( value );
              break;
              }
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
            }
            case 'I':
            case 'i':
            {
            if (LocaleCompare(keyword,"inner") == 0)
              {
              y = atoi( value );
              break;
              }
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
            }
            case 'O':
            case 'o':
            {
            if (LocaleCompare(keyword,"outer") == 0)
              {
              x = atoi( value );
              break;
              }
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
            }
            case 'W':
            case 'w':
            {
            if (LocaleCompare(keyword,"width") == 0)
              {
              width = atoi( value );
              break;
              }
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
            }
            case 'X':
            case 'x':
            {
            if (LocaleCompare(keyword,"x") == 0)
              {
              x = atoi( value );
              break;
              }
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
            }
            case 'Y':
            case 'y':
            {
            if (LocaleCompare(keyword,"y") == 0)
              {
              y = atoi( value );
              break;
              }
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
            }
            default:
            {
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
            }
          }
          }
        }

        /*
          frame image.
        */
        {
        Image
          *newImage;
        FrameInfo
          frameInfo;

        frameInfo.x = (long) width;
        frameInfo.y = (long) height;
        frameInfo.width = msl_info->image[n]->columns + ( frameInfo.x << 1 );
        frameInfo.height = msl_info->image[n]->rows + ( frameInfo.y << 1 );
        frameInfo.outer_bevel = x;
        frameInfo.inner_bevel = y;

        newImage=FrameImage(msl_info->image[n],&frameInfo, &msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        }

        break;
      }
      ThrowMSLException(OptionError,"UnrecognizedElement",(const char *) name);
    }
    case 'G':
    case 'g':
    {
    if (LocaleCompare((char *) name, "gamma") == 0)
    {
      double
        gammaRed = 1.0, gammaGreen = 1.0, gammaBlue = 1.0;

      if (msl_info->image[n] == (Image *) NULL)
      {
        ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
        break;
      }
      if (attributes == (const xmlChar **) NULL)
        break;
      for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
      {
        keyword=(const char *) attributes[i++];
        CloneString(&value,(char *) attributes[i]);
        (void) CopyMagickString(key,value,MaxTextExtent);
        switch (*keyword)
        {
          case 'B':
          case 'b':
          {
            if (LocaleCompare(keyword,"blue") == 0)
            {
              gammaBlue = atof( value );
              break;
            }
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
          }
          case 'G':
          case 'g':
          {
            if (LocaleCompare(keyword,"green") == 0)
            {
              gammaGreen = atof( value );
              break;
            }
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
          }
          case 'R':
          case 'r':
          {
            if (LocaleCompare(keyword,"red") == 0)
            {
              gammaRed = atof( value );
              break;
            }
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
          }
          default:
          {
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
          }
        }
      }

      /* process image */
      {
        char gamma[MaxTextExtent + 1];
        (void) FormatMagickString(gamma,MaxTextExtent,"%3.6f/%3.6f/%3.6f/",
          gammaRed,gammaGreen,gammaBlue);
        GammaImage ( msl_info->image[n], gamma );
      }
      break;
    }
      else if (LocaleCompare((char *) name,"get") == 0)
        {
          if (msl_info->image[n] == (Image *) NULL)
            {
              ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
              break;
            }
          if (attributes == (const xmlChar **) NULL)
            break;
          for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
          {
            keyword=(const char *) attributes[i++];
            CloneString(&value,(char *) attributes[i]);
            (void) CopyMagickString(key,value,MaxTextExtent);
            switch (*keyword)
            {
              case 'H':
              case 'h':
              {
                if (LocaleCompare(keyword,"height") == 0)
                  {
                    (void) FormatMagickString(value,MaxTextExtent,"%ld",
                      msl_info->image[n]->rows);
                    (void) SetImageAttribute(msl_info->attributes[n],key,value);
                    break;
                  }
                ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              }
              case 'W':
              case 'w':
              {
                if (LocaleCompare(keyword,"width") == 0)
                  {
                    (void) FormatMagickString(value,MaxTextExtent,"%ld",
                      msl_info->image[n]->columns);
                    (void) SetImageAttribute(msl_info->attributes[n],key,value);
                    break;
                  }
                ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              }
              default:
              {
                ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
                break;
              }
            }
          }
          break;
        }
    else if (LocaleCompare((char *) name, "group") == 0)
    {
      msl_info->nGroups++;
      msl_info->group_info=(MSLGroupInfo *) ResizeMagickMemory(
        msl_info->group_info,(msl_info->nGroups+1)*
        sizeof(*msl_info->group_info));
      break;
    }
      ThrowMSLException(OptionError,"UnrecognizedElement",(const char *) name);
    }
    case 'I':
    case 'i':
    {
      if (LocaleCompare((char *) name,"image") == 0)
        {
          long
            n;

          MSLPushImage(msl_info,(Image *) NULL);
          n=msl_info->n;
          if (attributes == (const xmlChar **) NULL)
            break;
          for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
          {
            keyword=(const char *) attributes[i++];
            CloneString(&value,TranslateText(msl_info->image_info[n],
              msl_info->attributes[n],(char *) attributes[i]));
            switch (*keyword)
            {
      case 'B':
      case 'b':
        {
          if (LocaleCompare(keyword,"background") == 0)
          {
            (void) QueryColorDatabase(value,
              &msl_info->image_info[n]->background_color,&exception);
            break;
          }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
        }

      case 'C':
      case 'c':
        {
          if (LocaleCompare(keyword,"color") == 0)
          {
            Image
              *next_image;

            (void) strcpy(msl_info->image_info[n]->filename,"xc:");
            (void) ConcatenateMagickString(msl_info->image_info[n]->filename,
              value,MaxTextExtent);
            next_image=ReadImage(msl_info->image_info[n],&exception);
            CatchException(&exception);
            if (next_image == (Image *) NULL)
              continue;
            if (msl_info->image[n] == (Image *) NULL)
              msl_info->image[n]=next_image;
            else
              {
              register Image
                *p;

              /*
                Link image into image list.
              */
              p=msl_info->image[n];
              for ( ; p->next != (Image *) NULL; p=GetNextImageInList(p));
              next_image->previous=p;
              p->next=next_image;
              }
            break;
          }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
        }

      case 'I':
      case 'i':
        {
          if (LocaleCompare(keyword,"id") == 0)
          {
            (void) SetImageAttribute(msl_info->attributes[n],keyword,NULL);  /* make sure to clear it! */
            (void) SetImageAttribute(msl_info->attributes[n],keyword,value);
            break;
          }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
        }
              case 'S':
              case 's':
              {
                if (LocaleCompare(keyword,"size") == 0)
                {
          CloneString(&msl_info->image_info[n]->size,value);
          break;
        }
                ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
                break;
              }
              default:
              {
                ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
                break;
              }
            }
          }
          break;
        }
      else if (LocaleCompare((char *) name,"implode") == 0)
      {
        /* init the values */
        double  amount = 0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
        break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
        keyword=(const char *) attributes[i++];
        CloneString(&value,TranslateText(msl_info->image_info[n],
          msl_info->attributes[n],(char *) attributes[i]));
        switch (*keyword)
        {
          case 'A':
          case 'a':
          {
          if (LocaleCompare(keyword,"amount") == 0)
            {
            amount = atof( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          default:
          {
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
        }
        }

        /*
          process image.
        */
        {
        Image
          *newImage;

        newImage=ImplodeImage(msl_info->image[n],amount,&msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        break;
        }
      }
      ThrowMSLException(OptionError,"UnrecognizedElement",(const char *) name);
    }
    case 'L':
    case 'l':
    {
      if (LocaleCompare((char *) name, "level") == 0)
      {
        double
          levelBlack = 0, levelGamma = 1, levelWhite = QuantumRange;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
          break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
          keyword=(const char *) attributes[i++];
          CloneString(&value,(char *) attributes[i]);
          (void) CopyMagickString(key,value,MaxTextExtent);
          switch (*keyword)
          {
            case 'B':
            case 'b':
            {
              if (LocaleCompare(keyword,"black") == 0)
              {
                levelBlack = atof( value );
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            case 'G':
            case 'g':
            {
              if (LocaleCompare(keyword,"gamma") == 0)
              {
                levelGamma = atof( value );
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            case 'W':
            case 'w':
            {
              if (LocaleCompare(keyword,"white") == 0)
              {
                levelWhite = atof( value );
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            default:
            {
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
          }
        }

        /* process image */
        {
          char level[MaxTextExtent + 1];
          (void) FormatMagickString(level,MaxTextExtent,"%3.6f/%3.6f/%3.6f/",
            levelBlack,levelGamma,levelWhite);
          LevelImage ( msl_info->image[n], level );
          break;
        }
      }
    }
  case 'M':
  case 'm':
    {
      if (LocaleCompare((char *) name, "magnify") == 0)
      {
        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }

        /* no attributes here */

        /* process the image */
        {
          Image
            *newImage;

          newImage=MagnifyImage(msl_info->image[n],&msl_info->image[n]->exception);
          if (newImage == (Image *) NULL)
            break;
          msl_info->image[n]=DestroyImage(msl_info->image[n]);
          msl_info->image[n]=newImage;
          break;
        }
      }
      else if (LocaleCompare((char *) name,"medianfilter") == 0)
      {
        /* init the values */
        unsigned int  radius = 0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
        break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
        keyword=(const char *) attributes[i++];
        CloneString(&value,TranslateText(msl_info->image_info[n],
          msl_info->attributes[n],(char *) attributes[i]));
        switch (*keyword)
        {
          case 'R':
          case 'r':
          {
          if (LocaleCompare(keyword,"radius") == 0)
            {
            radius = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          default:
          {
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
        }
        }

        /*
          process image.
        */
        {
        Image
          *newImage;

        newImage=MedianFilterImage(msl_info->image[n], radius, &msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        break;
        }
      }
      else if (LocaleCompare((char *) name, "minify") == 0)
      {
        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }

        /* no attributes here */

        /* process the image */
        {
          Image
            *newImage;

          newImage=MinifyImage(msl_info->image[n],&msl_info->image[n]->exception);
          if (newImage == (Image *) NULL)
            break;
          msl_info->image[n]=DestroyImage(msl_info->image[n]);
          msl_info->image[n]=newImage;
          break;
        }
      }
      else if (LocaleCompare((char *) name, "msl") == 0 )
      {
        /*
          This is our base element.
            at the moment we don't do anything special
            but someday we might!
        */
        break;
      }
      else if (LocaleCompare((char *) name, "modulate") == 0)
      {
       double modBrightness=100;
       char modHue[64+1]={0}, modSaturation[64+1]={0};

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
          break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
          keyword=(const char *) attributes[i++];
          CloneString(&value,(char *) attributes[i]);
          (void) CopyMagickString(key,value,MaxTextExtent);
          switch (*keyword)
          {
            case 'B':
            case 'b':
            {
              if (LocaleCompare(keyword,"brightness") == 0)
              {
                modBrightness = atof(value);
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            case 'H':
            case 'h':
            {
              if (LocaleCompare(keyword,"hue") == 0)
              {
                CopyMagickString(modHue,value,64);
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            case 'S':
            case 's':
            {
              if (LocaleCompare(keyword,"saturation") == 0)
              {
                CopyMagickString(modSaturation, value, 64);
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            default:
            {
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
          }
        }

        /* process image */
        {
          char modulate[MaxTextExtent + 1];
          (void) FormatMagickString(modulate,MaxTextExtent,"%3.6f/%s/%s/",
            modBrightness,modSaturation,modHue);
          ModulateImage ( msl_info->image[n], modulate );
          break;
        }
      }
    ThrowMSLException(OptionError,"UnrecognizedElement",(const char *) name);
    }


  case 'N':
  case 'n':
    {
      if (LocaleCompare((char *) name, "normalize") == 0)
      {
        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }

        /* no attributes here */

        /* process the image */
        {
          NormalizeImage(msl_info->image[n]);
          break;
        }
      }
      ThrowMSLException(OptionError,"UnrecognizedElement",(const char *) name);
    }
  case 'O':
  case 'o':
    {
      if (LocaleCompare((char *) name, "oilpaint") == 0)
      {
        /* init the values */
        unsigned int  radius = 3;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
        break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
        keyword=(const char *) attributes[i++];
        CloneString(&value,TranslateText(msl_info->image_info[n],
          msl_info->attributes[n],(char *) attributes[i]));
        switch (*keyword)
        {
          case 'R':
          case 'r':
          {
          if (LocaleCompare(keyword,"radius") == 0)
            {
            radius = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          default:
          {
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
        }
        }

        /*
          process image.
        */
        {
        Image
          *newImage;

        newImage=OilPaintImage(msl_info->image[n], radius, &msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        break;
        }
      }
      ThrowMSLException(OptionError,"UnrecognizedElement",(const char *) name);
    }
    case 'P':
    case 'p':
    {
      if (LocaleCompare((char *) name,"print") == 0)
        {
      /* print does not require an image to be present! */

          if (attributes == (const xmlChar **) NULL)
            break;
          for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
          {
            keyword=(const char *) attributes[i++];
            CloneString(&value,TranslateText(msl_info->image_info[n],
              msl_info->attributes[n],(char *) attributes[i]));
            switch (*keyword)
            {
              case 'O':
              case 'o':
              {
                if (LocaleCompare(keyword,"output") == 0)
                {
                  (void) fprintf(stdout,"%s",value);
                break;
              }
                ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
        break;
              }
              default:
              {
                ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
                break;
              }
            }
          }
          break;
        }
      ThrowMSLException(OptionError,"UnrecognizedElement",(const char *) name);
    }
    case 'R':
    case 'r':
    {
      if (LocaleCompare((char *) name,"read") == 0)
        {
          if (attributes == (const xmlChar **) NULL)
            break;
          for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
          {
            keyword=(const char *) attributes[i++];
            CloneString(&value,TranslateText(msl_info->image_info[n],
              msl_info->attributes[n],(char *) attributes[i]));
            switch (*keyword)
            {
              case 'F':
              case 'f':
              {
                if (LocaleCompare(keyword,"filename") == 0)
                  {
                    Image
                      *next_image;

                    (void) strcpy(msl_info->image_info[n]->filename,value);
                    next_image=ReadImage(msl_info->image_info[n],&exception);
                    CatchException(&exception);
                    if (next_image == (Image *) NULL)
                      continue;
                    if (msl_info->image[n] == (Image *) NULL)
                      msl_info->image[n]=next_image;
                    else
                      {
                        register Image
                          *p;

                        /*
                          Link image into image list.
                        */
                        p=msl_info->image[n];
                        for ( ; p->next != (Image *) NULL; p=GetNextImageInList(p));
                        next_image->previous=p;
                        p->next=next_image;
                      }
                    break;
                  }
                ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
                break;
              }
              case 'S':
              case 's':
              {
                if (LocaleCompare(keyword,"size") == 0)
                  {
                    msl_info->image_info[n]->size=AcquireString(value);
                    break;
                  }
                ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
                break;
              }
              default:
              {
                ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
                break;
              }
            }
          }
          break;
        }
    else if (LocaleCompare((char *) name, "reducenoise") == 0 )
    {
        /* init the values */
        unsigned int  radius = 0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
        break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
        keyword=(const char *) attributes[i++];
        CloneString(&value,TranslateText(msl_info->image_info[n],
          msl_info->attributes[n],(char *) attributes[i]));
        switch (*keyword)
        {
          case 'R':
          case 'r':
          {
          if (LocaleCompare(keyword,"radius") == 0)
            {
            radius = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          default:
          {
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
        }
        }

        /*
          process image.
        */
        {
        Image
          *newImage;

        newImage=ReduceNoiseImage(msl_info->image[n], radius, &msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        break;
        }
    }
    else if (LocaleCompare((char *) name,"resample") == 0)
    {
      double
        x_resolution,
        y_resolution;

      if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
      if (attributes == (const xmlChar **) NULL)
        break;
      x_resolution=DefaultResolution;
      y_resolution=DefaultResolution;
      for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
      {
        keyword=(const char *) attributes[i++];
        CloneString(&value,TranslateText(msl_info->image_info[n],
          msl_info->attributes[n],(char *) attributes[i]));
        switch (*keyword)
        {
          case 'b':
          {
            if (LocaleCompare(keyword,"blur") == 0)
              {
                msl_info->image[n]->blur=atof(value);
                break;
              }
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
          }
          case 'G':
          case 'g':
          {
            if (LocaleCompare(keyword,"geometry") == 0)
              {
                long
                  flags;

                flags=ParseGeometry(value,&geometry_info);
                if ((flags & SigmaValue) == 0)
                  geometry_info.sigma*=geometry_info.rho;
                x_resolution=geometry_info.rho;
                y_resolution=geometry_info.sigma;
                break;
              }
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
          }
          case 'X':
          case 'x':
          {
            if (LocaleCompare(keyword,"x-resolution") == 0)
              {
                x_resolution=atof(value);
                break;
              }
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
          }
          case 'Y':
          case 'y':
          {
            if (LocaleCompare(keyword,"y-resolution") == 0)
              {
                y_resolution=atof(value);
                break;
              }
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
          }
          default:
          {
            ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
          }
        }
      }
      /*
        Resample image.
      */
      {
        double
          factor;

        Image
          *resample_image;

        factor=1.0;
        if (msl_info->image[n]->units == PixelsPerCentimeterResolution)
          factor=2.54;
        width=(unsigned long) (x_resolution*msl_info->image[n]->columns/
          (factor*(msl_info->image[n]->x_resolution == 0.0 ? DefaultResolution :
          msl_info->image[n]->x_resolution))+0.5);
        height=(unsigned long) (y_resolution*msl_info->image[n]->rows/
          (factor*(msl_info->image[n]->y_resolution == 0.0 ? DefaultResolution :
          msl_info->image[n]->y_resolution))+0.5);
        resample_image=ResizeImage(msl_info->image[n],width,height,
          msl_info->image[n]->filter,msl_info->image[n]->blur,
          &msl_info->image[n]->exception);
        if (resample_image == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=resample_image;
      }
      break;
    }
    else if (LocaleCompare((char *) name,"resize") == 0)
    {
      /* init the values */
      width=msl_info->image[n]->columns;
      height=msl_info->image[n]->rows;
      x=0;
      y=0;

          if (msl_info->image[n] == (Image *) NULL)
            {
              ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
              break;
            }
          if (attributes == (const xmlChar **) NULL)
            break;
          for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
          {
            keyword=(const char *) attributes[i++];
            CloneString(&value,TranslateText(msl_info->image_info[n],
              msl_info->attributes[n],(char *) attributes[i]));
            switch (*keyword)
            {
        case 'B':
        case 'b':
          {
          if (LocaleCompare(keyword,"blur") == 0)
            {
            msl_info->image[n]->blur = atof( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
        case 'F':
        case 'f':
        {
          if (LocaleCompare(keyword, "filter") == 0)
            {
              long
                filter;

              filter=ParseMagickOption(MagickFilterOptions,MagickFalse,value);
              if (filter < 0)
                ThrowMSLException(OptionError,"UnrecognizedImageFilter",value);
              msl_info->image[n]->filter=(FilterTypes) filter;
              break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
        }
              case 'G':
              case 'g':
              {
                if (LocaleCompare(keyword,"geometry") == 0)
                  {
                    (void) ParseMetaGeometry(value,&x,&y,&width,&height);
                    break;
                  }
                ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
                break;
              }
              case 'H':
              case 'h':
              {
                if (LocaleCompare(keyword,"height") == 0)
                  {
          height = atoi( value );
          break;
                  }
                ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
                break;
              }
              case 'W':
              case 'w':
              {
                if (LocaleCompare(keyword,"width") == 0)
                  {
          width = atoi( value );
          break;
                  }
                ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
                break;
              }
              default:
              {
                ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
                break;
              }
            }
          }

            /*
              Resize image.
            */
          {
            Image
              *resize_image;

            if ((width == msl_info->image[n]->columns) &&
                (height == msl_info->image[n]->rows))
              break;
            resize_image=ZoomImage(msl_info->image[n],width,height,
              &msl_info->image[n]->exception);
            if (resize_image == (Image *) NULL)
              break;
            msl_info->image[n]=DestroyImage(msl_info->image[n]);
            msl_info->image[n]=resize_image;
          }

          break;
        }
      else if (LocaleCompare((char *) name,"roll") == 0)
      {
        /* init the values */
        width=msl_info->image[n]->columns;
        height=msl_info->image[n]->rows;
        x = y = 0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
        break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
        keyword=(const char *) attributes[i++];
        CloneString(&value,TranslateText(msl_info->image_info[n],
          msl_info->attributes[n],(char *) attributes[i]));
        switch (*keyword)
        {
          case 'G':
          case 'g':
          {
          if (LocaleCompare(keyword,"geometry") == 0)
            {
            (void) ParseMetaGeometry(value,&x,&y,&width,&height);
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          case 'X':
          case 'x':
          {
          if (LocaleCompare(keyword,"x") == 0)
            {
            x = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          case 'Y':
          case 'y':
          {
          if (LocaleCompare(keyword,"y") == 0)
            {
            y = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          default:
          {
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
        }
        }

        /*
          process image.
        */
        {
        Image
          *newImage;

        newImage=RollImage(msl_info->image[n], x, y, &msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        }

        break;
      }
      else if (LocaleCompare((char *) name,"rotate") == 0)
      {
        /* init the values */
        double  degrees = 0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
        break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
        keyword=(const char *) attributes[i++];
        CloneString(&value,TranslateText(msl_info->image_info[n],
          msl_info->attributes[n],(char *) attributes[i]));
        switch (*keyword)
        {
          case 'D':
          case 'd':
          {
          if (LocaleCompare(keyword,"degrees") == 0)
            {
            degrees = atof( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          default:
          {
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
        }
        }

        /*
          process image.
        */
        {
        Image
          *newImage;

        newImage=RotateImage(msl_info->image[n], degrees, &msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        }

        break;
      }
      ThrowMSLException(OptionError,"UnrecognizedElement",(const char *) name);
    }
  case 'S':
  case 's':
    {
      if (LocaleCompare((char *) name,"sample") == 0)
      {
        /* init the values */
        width=msl_info->image[n]->columns;
        height=msl_info->image[n]->rows;
        x = y = 0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
        break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
        keyword=(const char *) attributes[i++];
        CloneString(&value,TranslateText(msl_info->image_info[n],
          msl_info->attributes[n],(char *) attributes[i]));
        switch (*keyword)
        {
          case 'G':
          case 'g':
          {
          if (LocaleCompare(keyword,"geometry") == 0)
            {
            (void) ParseMetaGeometry(value,&x,&y,&width,&height);
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          case 'H':
          case 'h':
          {
          if (LocaleCompare(keyword,"height") == 0)
            {
            height = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          case 'W':
          case 'w':
          {
          if (LocaleCompare(keyword,"width") == 0)
            {
            width = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          default:
          {
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
        }
        }

        /*
          process image.
        */
        {
        Image
          *newImage;

        newImage=SampleImage(msl_info->image[n], width, height, &msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        }

        break;
      }
      else if (LocaleCompare((char *) name,"scale") == 0)
      {
        /* init the values */
        width=msl_info->image[n]->columns;
        height=msl_info->image[n]->rows;
        x = y = 0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
        break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
        keyword=(const char *) attributes[i++];
        CloneString(&value,TranslateText(msl_info->image_info[n],
          msl_info->attributes[n],(char *) attributes[i]));
        switch (*keyword)
        {
          case 'G':
          case 'g':
          {
          if (LocaleCompare(keyword,"geometry") == 0)
            {
            (void) ParseMetaGeometry(value,&x,&y,&width,&height);
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          case 'H':
          case 'h':
          {
          if (LocaleCompare(keyword,"height") == 0)
            {
            height = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          case 'W':
          case 'w':
          {
          if (LocaleCompare(keyword,"width") == 0)
            {
            width = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          default:
          {
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
        }
        }

        /*
          process image.
        */
        {
        Image
          *newImage;

        newImage=ScaleImage(msl_info->image[n], width, height, &msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        }

        break;
      }
      else if (LocaleCompare((char *) name, "set") == 0)
      {
        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }

        if (attributes == (const xmlChar **) NULL)
          break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
          keyword=(const char *) attributes[i++];
          CloneString(&value,TranslateText(msl_info->image_info[n],
                msl_info->attributes[n],(char *) attributes[i]));
          switch (*keyword)
          {
            case 'B':
            case 'b':
            {
              if (LocaleCompare(keyword,"background") == 0)
              {
                (void) QueryColorDatabase(value,
                  &msl_info->image_info[n]->background_color,&exception);
                break;
              }
              else if (LocaleCompare(keyword,"bordercolor") == 0)
              {
                (void) QueryColorDatabase(value,
                  &msl_info->image_info[n]->border_color,&exception);
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            case 'C':
            case 'c':
            {
              if (LocaleCompare(keyword,"clip-mask") == 0)
              {
                for (j=0; j<msl_info->n;j++)
                {
                  const ImageAttribute *
                    theAttr = GetImageAttribute(msl_info->attributes[j], "id");
                  if (theAttr && LocaleCompare(theAttr->value, value) == 0)
                  {
                    SetImageClipMask( msl_info->image[n], msl_info->image[j] );
                    break;
                  }
                }
                break;
              }
              else if (LocaleCompare(keyword, "colorspace") == 0)
              {
                if (LocaleCompare(value, "CMYK") == 0)
                  SetImageType( msl_info->image[n], ColorSeparationType );
                else if (LocaleCompare(value, "Gray") == 0)
                  SetImageType( msl_info->image[n], GrayscaleType );
                else if (LocaleCompare(value, "RGB") == 0)
                  SetImageType( msl_info->image[n], TrueColorType );
                else
                  ThrowMSLException(OptionError,"Unrecognized colorspace",keyword);
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            case 'D':
            case 'd':
            {
              if (LocaleCompare(keyword, "density") == 0)
              {
                GeometryInfo
                  geometry_info;

                MagickStatusType
                  flags;

                (void) CloneString(&msl_info->image_info[n]->density,
                  (char *) NULL);
                (void) CloneString(&msl_info->image_info[n]->density,value);
                (void) CloneString(&msl_info->draw_info[n]->density,
                  msl_info->image_info[n]->density);
                flags=ParseGeometry(msl_info->image_info[n]->density,
                  &geometry_info);
                msl_info->image[n]->x_resolution=geometry_info.rho;
                msl_info->image[n]->y_resolution=geometry_info.sigma;
                if ((flags & SigmaValue) == 0)
                  msl_info->image[n]->y_resolution=
                    msl_info->image[n]->x_resolution;
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            case 'M':
            case 'm':
            {
              if (LocaleCompare(keyword, "magick") == 0)
              {
                (void) strcpy( msl_info->image_info[n]->magick, value );
                break;
              }
              else if (LocaleCompare(keyword,"mattecolor") == 0)
              {
                (void) QueryColorDatabase(value,
                  &msl_info->image_info[n]->matte_color,&exception);
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            case 'O':
            case 'o':
            {
              if (LocaleCompare(keyword, "opacity") == 0)
              {
                int  opac = OpaqueOpacity,
                  len = strlen( value );

                if (value[len-1] == '%') {
                  char  tmp[100];
                  (void) CopyMagickString(tmp,value,len);
                  opac = atoi( tmp );
                  opac = (int)(QuantumRange * ((float)opac/100));
                } else
                  opac = atoi( value );
                SetImageOpacity( msl_info->image[n], opac );
                break;
              }

              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            default:
            {
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
          }
        }
        break;
      }
      else if (LocaleCompare((char *) name, "sharpen") == 0)
      {
        double  radius = 0.0,
            sigma = 1.0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        /*
        NOTE: sharpen can have no attributes, since we use all the defaults!
        */
        if (attributes != (const xmlChar **) NULL)
        {
          for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
          {
          keyword=(const char *) attributes[i++];
          CloneString(&value,TranslateText(msl_info->image_info[n],
            msl_info->attributes[n],(char *) attributes[i]));
          switch (*keyword)
          {
            case 'R':
            case 'r':
            {
              if (LocaleCompare(keyword, "radius") == 0)
              {
                radius = atof( value );
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            case 'S':
            case 's':
            {
              if (LocaleCompare(keyword,"sigma") == 0)
              {
                sigma = atoi( value );
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            default:
            {
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
          }
          }
        }

        /*
          sharpen image.
        */
        {
        Image
          *newImage;

        newImage=SharpenImage(msl_info->image[n],radius,sigma,&msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        break;
        }
      }
      else if (LocaleCompare((char *) name,"shave") == 0)
      {
        /* init the values */
        width = height = 0;
        x = y = 0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
        break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
        keyword=(const char *) attributes[i++];
        CloneString(&value,TranslateText(msl_info->image_info[n],
          msl_info->attributes[n],(char *) attributes[i]));
        switch (*keyword)
        {
          case 'G':
          case 'g':
          {
          if (LocaleCompare(keyword,"geometry") == 0)
            {
            (void) ParseMetaGeometry(value,&x,&y,&width,&height);
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          case 'H':
          case 'h':
          {
          if (LocaleCompare(keyword,"height") == 0)
            {
            height = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          case 'W':
          case 'w':
          {
          if (LocaleCompare(keyword,"width") == 0)
            {
            width = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          default:
          {
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
        }
        }

        /*
          process image.
        */
        {
        Image
          *newImage;
        RectangleInfo
          rectInfo;

        rectInfo.height = height;
        rectInfo.width = width;
        rectInfo.x = x;
        rectInfo.y = y;


        newImage=ShaveImage(msl_info->image[n], &rectInfo, &msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        }

        break;
      }
      else if (LocaleCompare((char *) name,"shear") == 0)
      {
        /* init the values */
        width=msl_info->image[n]->columns;
        height=msl_info->image[n]->rows;
        x = y = 0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
        break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
        keyword=(const char *) attributes[i++];
        CloneString(&value,TranslateText(msl_info->image_info[n],
          msl_info->attributes[n],(char *) attributes[i]));
        switch (*keyword)
        {
          case 'X':
          case 'x':
          {
          if (LocaleCompare(keyword,"x") == 0)
            {
            x = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          case 'Y':
          case 'y':
          {
          if (LocaleCompare(keyword,"y") == 0)
            {
            y = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          default:
          {
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
        }
        }

        /*
          process image.
        */
        {
        Image
          *newImage;

        newImage=ShearImage(msl_info->image[n], x, y, &msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        }

        break;
      }
      else if (LocaleCompare((char *) name,"solarize") == 0)
      {
        /* init the values */
        double  threshold = 0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
        break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
        keyword=(const char *) attributes[i++];
        CloneString(&value,TranslateText(msl_info->image_info[n],
          msl_info->attributes[n],(char *) attributes[i]));
        switch (*keyword)
        {
          case 'T':
          case 't':
          {
          if (LocaleCompare(keyword,"threshold") == 0)
            {
            threshold = atof( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          default:
          {
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
        }
        }

        /*
          process image.
        */
        {
        SolarizeImage(msl_info->image[n], threshold);
        break;
        }
      }
      else if (LocaleCompare((char *) name,"spread") == 0)
      {
        /* init the values */
        unsigned int  radius = 0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
        break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
        keyword=(const char *) attributes[i++];
        CloneString(&value,TranslateText(msl_info->image_info[n],
          msl_info->attributes[n],(char *) attributes[i]));
        switch (*keyword)
        {
          case 'R':
          case 'r':
          {
          if (LocaleCompare(keyword,"radius") == 0)
            {
            radius = atoi( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          default:
          {
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
        }
        }

        /*
          process image.
        */
        {
        Image
          *newImage;

        newImage=SpreadImage(msl_info->image[n], radius, &msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        break;
        }
      }
      else if (LocaleCompare((char *) name,"stegano") == 0)
      {
        Image *
          watermark = (Image*)NULL;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
        break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
        keyword=(const char *) attributes[i++];
        CloneString(&value,TranslateText(msl_info->image_info[n],
          msl_info->attributes[n],(char *) attributes[i]));
        switch (*keyword)
        {
          case 'I':
          case 'i':
          {
          if (LocaleCompare(keyword,"image") == 0)
            {
            for (j=0; j<msl_info->n;j++)
            {
              const ImageAttribute *
                theAttr = GetImageAttribute(msl_info->attributes[j], "id");
              if (theAttr && LocaleCompare(theAttr->value, value) == 0)
              {
                watermark = msl_info->image[j];
                break;
              }
            }
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          default:
          {
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
        }
        }

        /*
          process image.
        */
        if ( watermark != (Image*) NULL )
        {
        Image
          *newImage;

        newImage=SteganoImage(msl_info->image[n], watermark, &msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        break;
        } else
          ThrowMSLException(OptionError,"Missing watermark image",keyword);
      }
      else if (LocaleCompare((char *) name,"stereo") == 0)
      {
        Image *
          stereoImage = (Image*)NULL;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
        break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
        keyword=(const char *) attributes[i++];
        CloneString(&value,TranslateText(msl_info->image_info[n],
          msl_info->attributes[n],(char *) attributes[i]));
        switch (*keyword)
        {
          case 'I':
          case 'i':
          {
          if (LocaleCompare(keyword,"image") == 0)
            {
            for (j=0; j<msl_info->n;j++)
            {
              const ImageAttribute *
                theAttr = GetImageAttribute(msl_info->attributes[j], "id");
              if (theAttr && LocaleCompare(theAttr->value, value) == 0)
              {
                stereoImage = msl_info->image[j];
                break;
              }
            }
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          default:
          {
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
        }
        }

        /*
          process image.
        */
        if ( stereoImage != (Image*) NULL )
        {
        Image
          *newImage;

        newImage=StereoImage(msl_info->image[n], stereoImage, &msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        break;
        } else
          ThrowMSLException(OptionError,"Missing stereo image",keyword);
      }
      else if (LocaleCompare((char *) name,"swirl") == 0)
      {
        /* init the values */
        double  degrees = 0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
        break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
        keyword=(const char *) attributes[i++];
        CloneString(&value,TranslateText(msl_info->image_info[n],
          msl_info->attributes[n],(char *) attributes[i]));
        switch (*keyword)
        {
          case 'D':
          case 'd':
          {
          if (LocaleCompare(keyword,"degrees") == 0)
            {
            degrees = atof( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          default:
          {
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
        }
        }

        /*
          process image.
        */
        {
        Image
          *newImage;

        newImage=SwirlImage(msl_info->image[n], degrees, &msl_info->image[n]->exception);
        if (newImage == (Image *) NULL)
          break;
        msl_info->image[n]=DestroyImage(msl_info->image[n]);
        msl_info->image[n]=newImage;
        }

        break;
      }
      ThrowMSLException(OptionError,"UnrecognizedElement",(const char *) name);
    }
  case 'T':
  case 't':
    {
      if (LocaleCompare((char *) name,"texture") == 0)
      {
        Image *
          textureImage = (Image*)NULL;

        /*
        if the image hasn't been initted yet, then clear it to white
        */
        if (msl_info->image[n] == (Image *) NULL)
        {
          Image
            *next_image;

          (void) strcpy(msl_info->image_info[n]->filename,"xc:white");
          next_image=ReadImage(msl_info->image_info[n],&exception);
          CatchException(&exception);
          if (next_image == (Image *) NULL)
            break;
          if (msl_info->image[n] == (Image *) NULL)
            msl_info->image[n]=next_image;
          else
            {
            register Image
              *p;

            /*
              Link image into image list.
            */
            p=msl_info->image[n];
            for ( ; p->next != (Image *) NULL; p=GetNextImageInList(p));
            next_image->previous=p;
            p->next=next_image;
            }
        }
        if (attributes == (const xmlChar **) NULL)
        break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
        keyword=(const char *) attributes[i++];
        CloneString(&value,TranslateText(msl_info->image_info[n],
          msl_info->attributes[n],(char *) attributes[i]));
        switch (*keyword)
        {
          case 'I':
          case 'i':
          {
          if (LocaleCompare(keyword,"image") == 0)
            {
            for (j=0; j<msl_info->n;j++)
            {
              const ImageAttribute *
                theAttr = GetImageAttribute(msl_info->attributes[j], "id");
              if (theAttr && LocaleCompare(theAttr->value, value) == 0)
              {
                textureImage = msl_info->image[j];
                break;
              }
            }
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          default:
          {
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
        }
        }

        /*
          process image.
        */
        if ( textureImage != (Image*) NULL )
        {
        TextureImage(msl_info->image[n], textureImage);
        break;
        } else
          ThrowMSLException(OptionError,"Missing texture image",keyword);
      }
      else if (LocaleCompare((char *) name,"threshold") == 0)
      {
        /* init the values */
        double  threshold = 0;

        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
        break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
        keyword=(const char *) attributes[i++];
        CloneString(&value,TranslateText(msl_info->image_info[n],
          msl_info->attributes[n],(char *) attributes[i]));
        switch (*keyword)
        {
          case 'T':
          case 't':
          {
          if (LocaleCompare(keyword,"threshold") == 0)
            {
            threshold = atof( value );
            break;
            }
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
          default:
          {
          ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
          break;
          }
        }
        }

        /*
          process image.
        */
        {
        BilevelImageChannel(msl_info->image[n],
          (ChannelType) ((long) (AllChannels &~ (long) OpacityChannel)),
          threshold);
        break;
        }
      }
      else if (LocaleCompare((char *) name, "transparent") == 0)
      {
        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }
        if (attributes == (const xmlChar **) NULL)
          break;
        for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
        {
          keyword=(const char *) attributes[i++];
          CloneString(&value,TranslateText(msl_info->image_info[n],
                msl_info->attributes[n],(char *) attributes[i]));
          switch (*keyword)
          {
            case 'C':
            case 'c':
            {
              if (LocaleCompare(keyword,"color") == 0)
              {
                MagickPixelPacket
                  target;

                (void) QueryMagickColor(value,&target,&exception);
                (void) PaintTransparentImage(msl_info->image[n],&target,
                  TransparentOpacity);
                break;
              }
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              break;
            }
            default:
            {
              ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
            break;
            }
          }
        }
        break;
      }
      else if (LocaleCompare((char *) name, "trim") == 0)
      {
        if (msl_info->image[n] == (Image *) NULL)
        {
          ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
          break;
        }

        /* no attributes here */

        /* process the image */
        {
          Image
            *newImage;
          RectangleInfo
            rectInfo;

          /* all zeros on a crop == trim edges! */
          rectInfo.height = rectInfo.width = 0;
          rectInfo.x =  rectInfo.y = 0;

          newImage=CropImage(msl_info->image[n],&rectInfo, &msl_info->image[n]->exception);
          if (newImage == (Image *) NULL)
            break;
          msl_info->image[n]=DestroyImage(msl_info->image[n]);
          msl_info->image[n]=newImage;
          break;
        }
      }
      ThrowMSLException(OptionError,"UnrecognizedElement",(const char *) name);
    }
    case 'W':
    case 'w':
    {
      if (LocaleCompare((char *) name,"write") == 0)
        {
          if (msl_info->image[n] == (Image *) NULL)
            {
              ThrowMSLException(OptionError,"NoImagesDefined",(char *) name);
              break;
            }
          if (attributes == (const xmlChar **) NULL)
            break;
          for (i=0; (attributes[i] != (const xmlChar *) NULL); i++)
          {
            keyword=(const char *) attributes[i++];
            CloneString(&value,TranslateText(msl_info->image_info[n],
              msl_info->attributes[n],(char *) attributes[i]));
            switch (*keyword)
            {
              case 'F':
              case 'f':
              {
                if (LocaleCompare(keyword,"filename") == 0)
                  {
                    (void) CopyMagickString(msl_info->image[n]->filename,value,
                      MaxTextExtent);
                    break;
                  }
                ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              }
              case 'Q':
              case 'q':
              {
                if (LocaleCompare(keyword,"quality") == 0)
                  {
                    msl_info->image_info[n]->quality=atol(value);
                    msl_info->image[n]->quality=
                      msl_info->image_info[n]->quality;
                    break;
                  }
                ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
              }
              default:
              {
                ThrowMSLException(OptionError,"UnrecognizedAttribute",keyword);
                break;
              }
            }
          }

          /* process */
          {
            (void) WriteImage(msl_info->image_info[n], msl_info->image[n]);
            break;
          }
        }
      ThrowMSLException(OptionError,"UnrecognizedElement",(const char *) name);
    }
    default:
    {
      ThrowMSLException(OptionError,"UnrecognizedElement",(const char *) name);
      break;
    }
  }
  if ( value != NULL )
  value=(char *) RelinquishMagickMemory(value);
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),"  )");
}

static void MSLEndElement(void *context,const xmlChar *name)
{
  MSLInfo
    *msl_info;

  /*
    Called when the end of an element has been detected.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.endElement(%s)",name);
  msl_info=(MSLInfo *) context;
  switch (*name)
  {
    case 'G':
    case 'g':
    {
      if (LocaleCompare((char *) name, "group") == 0 )
      {
        if (msl_info->group_info[msl_info->nGroups-1].numImages > 0 )
        {
          long  i = (long)
            (msl_info->group_info[msl_info->nGroups-1].numImages);
          while ( i-- )
          {
            if (msl_info->image[msl_info->n] != (Image *) NULL)
              msl_info->image[msl_info->n]=DestroyImage(msl_info->image[msl_info->n]);
            msl_info->attributes[msl_info->n]=DestroyImage(msl_info->attributes[msl_info->n]);
            msl_info->image_info[msl_info->n]=DestroyImageInfo(msl_info->image_info[msl_info->n]);
            msl_info->n--;
          }
        }
        msl_info->nGroups--;
      }
    }
    break;

    case 'I':
    case 'i':
    {
      if (LocaleCompare((char *) name, "image") == 0)
        MSLPopImage(msl_info);
    }
    break;

    case 'M':
    case 'm':
    {
      if (LocaleCompare((char *) name, "msl") == 0 )
      {
        /*
          This is our base element.
            at the moment we don't do anything special
            but someday we might!
        */
      }
    }
    break;

    default:
    break;
  }
}

static void MSLCharacters(void *context,const xmlChar *c,int length)
{
  MSLInfo
    *msl_info;

  /*
    Receiving some characters from the parser.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.characters(%s,%d)",c,length);
  msl_info=(MSLInfo *) context;
}

static void MSLReference(void *context,const xmlChar *name)
{
  MSLInfo
    *msl_info;

  xmlParserCtxtPtr
    parser;

  /*
    Called when an entity reference is detected.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.reference(%s)",name);
  msl_info=(MSLInfo *) context;
  parser=msl_info->parser;
  if (*name == '#')
    (void) xmlAddChild(parser->node,xmlNewCharRef(msl_info->document,name));
  else
    (void) xmlAddChild(parser->node,xmlNewReference(msl_info->document,name));
}

static void MSLIgnorableWhitespace(void *context,const xmlChar *c,int length)
{
  MSLInfo
    *msl_info;

  /*
    Receiving some ignorable whitespaces from the parser.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.ignorableWhitespace(%.30s, %d)",c,length);
  msl_info=(MSLInfo *) context;
}

static void MSLProcessingInstructions(void *context,const xmlChar *target,
  const xmlChar *data)
{
  MSLInfo
    *msl_info;

  /*
    A processing instruction has been parsed.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.processingInstruction(%s, %s)",
    target,data);
  msl_info=(MSLInfo *) context;
}

static void MSLComment(void *context,const xmlChar *value)
{
  MSLInfo
    *msl_info;

  /*
    A comment has been parsed.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.comment(%s)",value);
  msl_info=(MSLInfo *) context;
}

static void MSLWarning(void *context,const char *format,...)
{
  char
    reason[MaxTextExtent];

  MSLInfo
    *msl_info;

  va_list
    operands;

  /**
    Display and format a warning messages, gives file, line, position and
    extra parameters.
  */
  va_start(operands,format);
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),"  SAX.warning: ");
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),format,operands);
  msl_info=(MSLInfo *) context;
#if !defined(HAVE_VSNPRINTF)
  (void) vsprintf(reason,format,operands);
#else
  (void) vsnprintf(reason,MaxTextExtent,format,operands);
#endif
  ThrowMSLException(CoderError,reason,strerror(errno));
  va_end(operands);
}

static void MSLError(void *context,const char *format,...)
{
  char
    reason[MaxTextExtent];

  MSLInfo
    *msl_info;

  va_list
    operands;

  /*
    Display and format a error formats, gives file, line, position and
    extra parameters.
  */
  va_start(operands,format);
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),"  SAX.error: ");
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),format,operands);
  msl_info=(MSLInfo *) context;
#if !defined(HAVE_VSNPRINTF)
  (void) vsprintf(reason,format,operands);
#else
  (void) vsnprintf(reason,MaxTextExtent,format,operands);
#endif
  ThrowMSLException(DelegateFatalError,reason,"some text");
  va_end(operands);
}

static void MSLCDataBlock(void *context,const xmlChar *value,int length)
{
  MSLInfo
    *msl_info;

   xmlNodePtr
     child;

  xmlParserCtxtPtr
    parser;

  /*
    Called when a pcdata block has been parsed.
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.pcdata(%s, %d)",value,length);
  msl_info=(MSLInfo *) context;
  parser=msl_info->parser;
  child=xmlGetLastChild(parser->node);
  if ((child != (xmlNodePtr) NULL) && (child->type == XML_CDATA_SECTION_NODE))
    {
      xmlTextConcat(child,value,length);
      return;
    }
  (void) xmlAddChild(parser->node,xmlNewCDataBlock(parser->myDoc,value,length));
}

static void MSLExternalSubset(void *context,const xmlChar *name,
  const xmlChar *external_id,const xmlChar *system_id)
{
  MSLInfo
    *msl_info;

  xmlParserCtxt
    parser_context;

  xmlParserCtxtPtr
    parser;

  xmlParserInputPtr
    input;

  /*
    Does this document has an external subset?
  */
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),
    "  SAX.externalSubset(%s %s %s)",name,
    (external_id != (const xmlChar *) NULL ? (char *) external_id : " "),
    (system_id != (const xmlChar *) NULL ? (char *) system_id : " "));
  msl_info=(MSLInfo *) context;
  parser=msl_info->parser;
  if (((external_id == NULL) && (system_id == NULL)) ||
      ((parser->validate == 0) || (parser->wellFormed == 0) ||
      (msl_info->document == 0)))
    return;
  input=MSLResolveEntity(context,external_id,system_id);
  if (input == NULL)
    return;
  (void) xmlNewDtd(msl_info->document,name,external_id,system_id);
  parser_context=(*parser);
  parser->inputTab=(xmlParserInputPtr *) xmlMalloc(5*sizeof(*parser->inputTab));
  if (parser->inputTab == (xmlParserInputPtr *) NULL)
    {
      parser->errNo=XML_ERR_NO_MEMORY;
      parser->input=parser_context.input;
      parser->inputNr=parser_context.inputNr;
      parser->inputMax=parser_context.inputMax;
      parser->inputTab=parser_context.inputTab;
      return;
  }
  parser->inputNr=0;
  parser->inputMax=5;
  parser->input=NULL;
  xmlPushInput(parser,input);
  (void) xmlSwitchEncoding(parser,xmlDetectCharEncoding(parser->input->cur,4));
  if (input->filename == (char *) NULL)
    input->filename=(char *) xmlStrdup(system_id);
  input->line=1;
  input->col=1;
  input->base=parser->input->cur;
  input->cur=parser->input->cur;
  input->free=NULL;
  xmlParseExternalSubset(parser,external_id,system_id);
  while (parser->inputNr > 1)
    (void) xmlPopInput(parser);
  xmlFreeInputStream(parser->input);
  xmlFree(parser->inputTab);
  parser->input=parser_context.input;
  parser->inputNr=parser_context.inputNr;
  parser->inputMax=parser_context.inputMax;
  parser->inputTab=parser_context.inputTab;
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

static MagickBooleanType ProcessMSLScript(const ImageInfo *image_info,Image **image,
  ExceptionInfo *exception)
{
  xmlSAXHandler
    SAXModules =
    {
      MSLInternalSubset,
      MSLIsStandalone,
      MSLHasInternalSubset,
      MSLHasExternalSubset,
      MSLResolveEntity,
      MSLGetEntity,
      MSLEntityDeclaration,
      MSLNotationDeclaration,
      MSLAttributeDeclaration,
      MSLElementDeclaration,
      MSLUnparsedEntityDeclaration,
      MSLSetDocumentLocator,
      MSLStartDocument,
      MSLEndDocument,
      MSLStartElement,
      MSLEndElement,
      MSLReference,
      MSLCharacters,
      MSLIgnorableWhitespace,
      MSLProcessingInstructions,
      MSLComment,
      MSLWarning,
      MSLError,
      MSLError,
      MSLGetParameterEntity,
      MSLCDataBlock,
      MSLExternalSubset
    };

  char
    message[MaxTextExtent];

  Image
    *msl_image;

  int
    status;

  long
    n;

  MSLInfo
    msl_info;

  xmlSAXHandlerPtr
    SAXHandler;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image_info->filename);
  assert(image != (Image **) NULL);
  msl_image=AllocateImage(image_info);
  status=OpenBlob(image_info,msl_image,ReadBinaryBlobMode,exception);
  if (status == MagickFalse)
    {
      ThrowFileException(exception,FileOpenError,"UnableToOpenFile",
        msl_image->filename);
      DestroyImageList(msl_image);
      return(MagickFalse);
    }

  /*
    Parse MSL file.
  */
  (void) ResetMagickMemory(&msl_info,0,sizeof(msl_info));
  msl_info.exception=exception;
  msl_info.image_info=(ImageInfo **)
    AcquireMagickMemory(sizeof(*msl_info.image_info));
  msl_info.draw_info=(DrawInfo **)
    AcquireMagickMemory(sizeof(*msl_info.draw_info));
  /* top of the stack is the MSL file itself */
  msl_info.image=(Image **) AcquireMagickMemory(sizeof(*msl_info.image));
  msl_info.attributes=(Image **)
    AcquireMagickMemory(sizeof(*msl_info.attributes));
  msl_info.group_info=(MSLGroupInfo *)
    AcquireMagickMemory(sizeof(*msl_info.group_info));
  if ((msl_info.image_info == (ImageInfo **) NULL) ||
      (msl_info.image == (Image **) NULL) ||
      (msl_info.attributes == (Image **) NULL) ||
      (msl_info.group_info == (MSLGroupInfo *) NULL))
    ThrowMagickFatalException(ResourceLimitFatalError,
      "UnableToInterpretMSLImage",(*image)->filename);
  *msl_info.image_info=CloneImageInfo(image_info);
  *msl_info.draw_info=CloneDrawInfo(image_info,(DrawInfo *) NULL);
  *msl_info.attributes=CloneImage(image_info->attributes,0,0,MagickTrue,exception);
  msl_info.group_info[0].numImages=0;
  /* the first slot is used to point to the MSL file image */
  *msl_info.image=msl_image;
  if (*image != (Image *) NULL)
    MSLPushImage(&msl_info,*image);
  (void) xmlSubstituteEntitiesDefault(1);
  SAXHandler=(&SAXModules);
  msl_info.parser=xmlCreatePushParserCtxt(SAXHandler,&msl_info,(char *) NULL,0,
    msl_image->filename);
  while (ReadBlobString(msl_image,message) != (char *) NULL)
  {
    n=(long) strlen(message);
    if (n == 0)
      continue;
    status=xmlParseChunk(msl_info.parser,message,(int) n,MagickFalse);
    if (status != 0)
      break;
    (void) xmlParseChunk(msl_info.parser," ",1,MagickFalse);
    if (msl_info.exception->severity >= ErrorException)
      break;
  }
  if (msl_info.exception->severity == UndefinedException)
    (void) xmlParseChunk(msl_info.parser," ",1,MagickTrue);
  xmlFreeParserCtxt(msl_info.parser);
  (void) LogMagickEvent(CoderEvent,GetMagickModule(),"end SAX");
  xmlCleanupParser();
  msl_info.group_info=(MSLGroupInfo *)
    RelinquishMagickMemory(msl_info.group_info);
  if (*image == (Image *) NULL)
    *image=(*msl_info.image);
  return((MagickBooleanType) ((*msl_info.image)->exception.severity == UndefinedException));
}

static Image *ReadMSLImage(const ImageInfo *image_info,ExceptionInfo *exception)
{
  Image *
    image;

  /*
    Open image file.
  */
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  if (image_info->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image_info->filename);
  assert(exception != (ExceptionInfo *) NULL);
  assert(exception->signature == MagickSignature);
  image=(Image *) NULL;
  (void) ProcessMSLScript(image_info,&image,exception);
  return(GetFirstImageInList(image));
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r M S L I m a g e                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterMSLImage() adds attributes for the MSL image format to
%  the list of supported formats.  The attributes include the image format
%  tag, a method to read and/or write the format, whether the format
%  supports the saving of more than one frame to the same file or blob,
%  whether the format supports native in-memory I/O, and a brief
%  description of the format.
%
%  The format of the RegisterMSLImage method is:
%
%      RegisterMSLImage(void)
%
*/
ModuleExport void RegisterMSLImage(void)
{
  MagickInfo
    *entry;

  entry=SetMagickInfo("MSL");
#if defined(HasXML)
  entry->decoder=(DecoderHandler *) ReadMSLImage;
  entry->encoder=(EncoderHandler *) WriteMSLImage;
#endif
  entry->description=AcquireString("Magick Scripting Language");
  entry->module=AcquireString("MSL");
  (void) RegisterMagickInfo(entry);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r M S L I m a g e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterMSLImage() removes format registrations made by the
%  MSL module from the list of supported formats.
%
%  The format of the UnregisterMSLImage method is:
%
%      UnregisterMSLImage(void)
%
*/
ModuleExport void UnregisterMSLImage(void)
{
  (void) UnregisterMagickInfo("MSL");
}

#if defined(HasXML)
/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   W r i t e M S L I m a g e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  WriteMSLImage() writes an image to a file in MVG image format.
%
%  The format of the WriteMSLImage method is:
%
%      MagickBooleanType WriteMSLImage(const ImageInfo *image_info,Image *image)
%
%  A description of each parameter follows.
%
%    o image_info: The image info.
%
%    o image:  The image.
%
%
*/
static MagickBooleanType WriteMSLImage(const ImageInfo *image_info,Image *image)
{
  assert(image_info != (const ImageInfo *) NULL);
  assert(image_info->signature == MagickSignature);
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  (void) ReferenceImage(image);
  (void) ProcessMSLScript(image_info,&image,&image->exception);
  return(MagickTrue);
}
#endif
