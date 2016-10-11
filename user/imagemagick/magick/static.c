/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%                  SSSSS  TTTTT   AAA   TTTTT  IIIII   CCCC                   %
%                  SS       T    A   A    T      I    C                       %
%                   SSS     T    AAAAA    T      I    C                       %
%                     SS    T    A   A    T      I    C                       %
%                  SSSSS    T    A   A    T    IIIII   CCCC                   %
%                                                                             %
%                                                                             %
%                         ImageMagick Static Methods                          %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 March 2000                                  %
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
#include "magick/exception-private.h"
#include "magick/module.h"
#include "magick/static.h"
#include "magick/string_.h"

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   E x e c u t e S t a t i c M o d u l e P r o c e s s                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ExecuteStaticModuleProcess() is just a template method.
%
%  The format of the ExecuteStaticModuleProcess method is:
%
%      MagickBooleanType ExecuteStaticModuleProcess(const char *tag,
%        Image **image,const int argc,char **argv)
%
%  A description of each parameter follows:
%
%    o tag: The module tag.
%
%    o image: The image.
%
%    o argc: The number of elements in the argument vector.
%
%    o argv: A text array containing the command line arguments.
%
*/
#if defined(SupportMagickModules)
MagickExport MagickBooleanType ExecuteStaticModuleProcess(const char *tag,
  Image **image,const int argc,char **argv)
{
  MagickBooleanType
    status;

  assert(image != (Image **) NULL);
  assert((*image)->signature == MagickSignature);
  if ((*image)->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",(*image)->filename);
  status=MagickFalse;
#if !defined(BuildMagickModules)
  {
    MagickBooleanType
      (*module)(Image **,const int,char **);

    module=(MagickBooleanType (*)(Image **,const int,char **)) NULL;
    if (LocaleCompare("analyze",tag) == 0)
      module=AnalyzeImage;
    if (module != (MagickBooleanType (*)(Image **,const int,char **)) NULL)
      {
        if ((*image)->debug != MagickFalse)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),
            "Invoking \"%s\" static filter module",tag);
        status=(*module)(image,argc,argv);
        if ((*image)->debug != MagickFalse)
          (void) LogMagickEvent(CoderEvent,GetMagickModule(),"\"%s\" completes",
            tag);
      }
  }
#endif
  return(status);
}
#endif

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e g i s t e r S t a t i c M o d u l e s                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  RegisterStaticModules() statically registers all the available module
%  handlers.
%
%  The format of the RegisterStaticModules method is:
%
%      RegisterStaticModules(void)
%
%
*/
MagickExport void RegisterStaticModules(void)
{
#if !defined(BuildMagickModules)
  RegisterARTImage();
  RegisterAVIImage();
  RegisterAVSImage();
  RegisterBMPImage();
  RegisterCAPTIONImage();
  RegisterCINImage();
  RegisterCIPImage();
  RegisterCLIPImage();
#if defined(HasWINGDI32)
  RegisterCLIPBOARDImage();
#endif
  RegisterCMYKImage();
  RegisterCUTImage();
  RegisterDCMImage();
  RegisterDIBImage();
  RegisterDPSImage();
  RegisterDPXImage();
#if defined(HasWINGDI32)
  RegisterEMFImage();
#endif
#if defined(HasTIFF)
  RegisterEPTImage();
#endif
  RegisterFAXImage();
  RegisterFITSImage();
#if defined(HasFPX)
  RegisterFPXImage();
#endif
  RegisterGIFImage();
  RegisterGRAYImage();
  RegisterGRADIENTImage();
  RegisterHISTOGRAMImage();
  RegisterHTMLImage();
  RegisterICONImage();
  RegisterINFOImage();
#if defined(HasJBIG)
  RegisterJBIGImage();
#endif
#if defined(HasJPEG)
  RegisterJPEGImage();
#endif
#if defined(HasJP2)
  RegisterJP2Image();
#endif
  RegisterLABELImage();
  RegisterMAGICKImage();
  RegisterMAPImage();
  RegisterMATImage();
  RegisterMATTEImage();
  RegisterMETAImage();
  RegisterMIFFImage();
  RegisterMONOImage();
  RegisterMPCImage();
  RegisterMPEGImage();
  RegisterMPRImage();
  RegisterMSLImage();
  RegisterMTVImage();
  RegisterMVGImage();
  RegisterNULLImage();
  RegisterOTBImage();
  RegisterPALMImage();
  RegisterPATTERNImage();
  RegisterPCDImage();
  RegisterPCLImage();
  RegisterPCXImage();
  RegisterPDBImage();
  RegisterPDFImage();
  RegisterPICTImage();
  RegisterPIXImage();
  RegisterPLASMAImage();
#if defined(HasPNG)
  RegisterPNGImage();
#endif
  RegisterPNMImage();
  RegisterPREVIEWImage();
  RegisterPSImage();
  RegisterPS2Image();
  RegisterPS3Image();
  RegisterPSDImage();
  RegisterPWPImage();
  RegisterRAWImage();
  RegisterRGBImage();
  RegisterRLAImage();
  RegisterRLEImage();
  RegisterSCRImage();
  RegisterSCTImage();
  RegisterSFWImage();
  RegisterSGIImage();
  RegisterSTEGANOImage();
  RegisterSUNImage();
  RegisterSVGImage();
  RegisterTGAImage();
#if defined(HasTIFF)
  RegisterTIFFImage();
#endif
  RegisterTILEImage();
  RegisterTIMImage();
  RegisterTTFImage();
  RegisterTXTImage();
  RegisterUILImage();
  RegisterURLImage();
  RegisterUYVYImage();
  RegisterVICARImage();
  RegisterVIDImage();
  RegisterVIFFImage();
  RegisterWBMPImage();
  RegisterWMFImage();
  RegisterWPGImage();
#if defined(HasX11)
  RegisterXImage();
#endif
  RegisterXBMImage();
  RegisterXCImage();
  RegisterXCFImage();
  RegisterXPMImage();
#if defined(_VISUALC_)
  RegisterXTRNImage();
#endif
#if defined(HasX11)
  RegisterXWDImage();
#endif
  RegisterYCBCRImage();
  RegisterYUVImage();
#endif
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   U n r e g i s t e r S t a t i c M o d u l e s                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  UnregisterStaticModules() statically unregisters all the available module
%  handlers.
%
%  The format of the UnregisterStaticModules method is:
%
%      UnregisterStaticModules(void)
%
%
*/
MagickExport void UnregisterStaticModules(void)
{
#if !defined(BuildMagickModules)
  UnregisterARTImage();
  UnregisterAVIImage();
  UnregisterAVSImage();
  UnregisterBMPImage();
  UnregisterCAPTIONImage();
  UnregisterCINImage();
  UnregisterCIPImage();
  UnregisterCLIPImage();
#if defined(HasWINGDI32)
  UnregisterCLIPBOARDImage();
#endif
  UnregisterCMYKImage();
  UnregisterCUTImage();
  UnregisterDCMImage();
  UnregisterDIBImage();
  UnregisterDPSImage();
  UnregisterDPXImage();
#if defined(HasWINGDI32)
  UnregisterEMFImage();
#endif
#if defined(HasTIFF)
  UnregisterEPTImage();
#endif
  UnregisterFAXImage();
  UnregisterFITSImage();
#if defined(HasFPX)
  UnregisterFPXImage();
#endif
  UnregisterGIFImage();
  UnregisterGRAYImage();
  UnregisterGRADIENTImage();
  UnregisterHISTOGRAMImage();
  UnregisterHTMLImage();
  UnregisterICONImage();
  UnregisterINFOImage();
#if defined(HasJBIG)
  UnregisterJBIGImage();
#endif
#if defined(HasJPEG)
  UnregisterJPEGImage();
#endif
#if defined(HasJP2)
  UnregisterJP2Image();
#endif
  UnregisterLABELImage();
  UnregisterMAGICKImage();
  UnregisterMAPImage();
  UnregisterMATImage();
  UnregisterMATTEImage();
  UnregisterMETAImage();
  UnregisterMIFFImage();
  UnregisterMONOImage();
  UnregisterMPCImage();
  UnregisterMPEGImage();
  UnregisterMPRImage();
  UnregisterMSLImage();
  UnregisterMTVImage();
  UnregisterMVGImage();
  UnregisterNULLImage();
  UnregisterOTBImage();
  UnregisterPALMImage();
  UnregisterPATTERNImage();
  UnregisterPCDImage();
  UnregisterPCLImage();
  UnregisterPCXImage();
  UnregisterPDBImage();
  UnregisterPDFImage();
  UnregisterPICTImage();
  UnregisterPIXImage();
  UnregisterPLASMAImage();
#if defined(HasPNG)
  UnregisterPNGImage();
#endif
  UnregisterPNMImage();
  UnregisterPREVIEWImage();
  UnregisterPSImage();
  UnregisterPS2Image();
  UnregisterPS3Image();
  UnregisterPSDImage();
  UnregisterPWPImage();
  UnregisterRAWImage();
  UnregisterRGBImage();
  UnregisterRLAImage();
  UnregisterRLEImage();
  UnregisterSCRImage();
  UnregisterSCTImage();
  UnregisterSFWImage();
  UnregisterSGIImage();
  UnregisterSTEGANOImage();
  UnregisterSUNImage();
  UnregisterSVGImage();
  UnregisterTGAImage();
#if defined(HasTIFF)
  UnregisterTIFFImage();
#endif
  UnregisterTILEImage();
  UnregisterTIMImage();
  UnregisterTTFImage();
  UnregisterTXTImage();
  UnregisterUILImage();
  UnregisterURLImage();
  UnregisterUYVYImage();
  UnregisterVICARImage();
  UnregisterVIDImage();
  UnregisterVIFFImage();
  UnregisterWBMPImage();
  UnregisterWMFImage();
  UnregisterWPGImage();
#if defined(HasX11)
  UnregisterXImage();
#endif
  UnregisterXBMImage();
  UnregisterXCImage();
  UnregisterXCFImage();
  UnregisterXPMImage();
#if defined(_VISUALC_)
  UnregisterXTRNImage();
#endif
#if defined(HasX11)
  UnregisterXWDImage();
#endif
  UnregisterYCBCRImage();
  UnregisterYUVImage();
#endif
}
