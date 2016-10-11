/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                       CCCC   OOO   L       OOO   RRRR                       %
%                      C      O   O  L      O   O  R   R                      %
%                      C      O   O  L      O   O  RRRR                       %
%                      C      O   O  L      O   O  R R                        %
%                       CCCC   OOO   LLLLL   OOO   R  R                       %
%                                                                             %
%                                                                             %
%                          ImageMagick Color Methods                          %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1992                                   %
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
#include "magick/color.h"
#include "magick/color-private.h"
#include "magick/client.h"
#include "magick/configure.h"
#include "magick/exception.h"
#include "magick/exception-private.h"
#include "magick/geometry.h"
#include "magick/image-private.h"
#include "magick/memory_.h"
#include "magick/monitor.h"
#include "magick/pixel-private.h"
#include "magick/quantize.h"
#include "magick/semaphore.h"
#include "magick/string_.h"
#include "magick/token.h"
#include "magick/utility.h"

/*
  Define declarations.
*/
#define ColorFilename  "colors.xml"
#define MaxTreeDepth  16
#define NodesInAList  1536

/*
  Declare color map.
*/
static const char
  *ColorMap = (const char *)
    "<?xml version=\"1.0\"?>"
    "<colormap>"
    "  <color name=\"black\" red=\"0\" green=\"0\" blue=\"0\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"white\" red=\"255\" green=\"255\" blue=\"255\" compliance=\"SVG, X11\" />"
    "  <color name=\"AliceBlue\" red=\"240\" green=\"248\" blue=\"255\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"AntiqueWhite\" red=\"250\" green=\"235\" blue=\"215\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"aqua\" red=\"0\" green=\"255\" blue=\"255\" compliance=\"SVG\" />"
    "  <color name=\"aquamarine\" red=\"127\" green=\"255\" blue=\"212\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"azure\" red=\"240\" green=\"255\" blue=\"255\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"beige\" red=\"245\" green=\"245\" blue=\"220\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"bisque\" red=\"255\" green=\"228\" blue=\"196\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"BlanchedAlmond\" red=\"255\" green=\"235\" blue=\"205\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"blue\" red=\"0\" green=\"0\" blue=\"255\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"BlueViolet\" red=\"138\" green=\"43\" blue=\"226\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"brown\" red=\"165\" green=\"42\" blue=\"42\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"burlywood\" red=\"222\" green=\"184\" blue=\"135\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"CadetBlue\" red=\"95\" green=\"158\" blue=\"160\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"chartreuse\" red=\"127\" green=\"255\" blue=\"0\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"chocolate\" red=\"210\" green=\"105\" blue=\"30\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"coral\" red=\"255\" green=\"127\" blue=\"80\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"CornflowerBlue\" red=\"100\" green=\"149\" blue=\"237\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"cornsilk\" red=\"255\" green=\"248\" blue=\"220\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"crimson\" red=\"220\" green=\"20\" blue=\"60\" compliance=\"SVG\" />"
    "  <color name=\"cyan\" red=\"0\" green=\"255\" blue=\"255\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"DarkBlue\" red=\"0\" green=\"0\" blue=\"139\" compliance=\"SVG, X11\" />"
    "  <color name=\"DarkCyan\" red=\"0\" green=\"139\" blue=\"139\" compliance=\"SVG, X11\" />"
    "  <color name=\"DarkGoldenrod\" red=\"184\" green=\"134\" blue=\"11\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"DarkGray\" red=\"169\" green=\"169\" blue=\"169\" compliance=\"SVG, X11\" />"
    "  <color name=\"DarkGreen\" red=\"0\" green=\"100\" blue=\"0\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"DarkGrey\" red=\"169\" green=\"169\" blue=\"169\" compliance=\"SVG, X11\" />"
    "  <color name=\"DarkKhaki\" red=\"189\" green=\"183\" blue=\"107\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"DarkMagenta\" red=\"139\" green=\"0\" blue=\"139\" compliance=\"SVG, X11\" />"
    "  <color name=\"DarkOliveGreen\" red=\"85\" green=\"107\" blue=\"47\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"DarkOrange\" red=\"255\" green=\"140\" blue=\"0\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"DarkOrchid\" red=\"153\" green=\"50\" blue=\"204\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"DarkRed\" red=\"139\" green=\"0\" blue=\"0\" compliance=\"SVG, X11\" />"
    "  <color name=\"DarkSalmon\" red=\"233\" green=\"150\" blue=\"122\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"DarkSeaGreen\" red=\"143\" green=\"188\" blue=\"143\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"DarkSlateBlue\" red=\"72\" green=\"61\" blue=\"139\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"DarkSlateGray\" red=\"47\" green=\"79\" blue=\"79\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"DarkSlateGrey\" red=\"47\" green=\"79\" blue=\"79\" compliance=\"SVG, X11\" />"
    "  <color name=\"DarkTurquoise\" red=\"0\" green=\"206\" blue=\"209\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"DarkViolet\" red=\"148\" green=\"0\" blue=\"211\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"DeepPink\" red=\"255\" green=\"20\" blue=\"147\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"DeepSkyBlue\" red=\"0\" green=\"191\" blue=\"255\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"DimGray\" red=\"105\" green=\"105\" blue=\"105\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"DimGrey\" red=\"105\" green=\"105\" blue=\"105\" compliance=\"SVG, X11\" />"
    "  <color name=\"DodgerBlue\" red=\"30\" green=\"144\" blue=\"255\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"firebrick\" red=\"178\" green=\"34\" blue=\"34\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"FloralWhite\" red=\"255\" green=\"250\" blue=\"240\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"ForestGreen\" red=\"34\" green=\"139\" blue=\"34\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"fractal\" red=\"128\" green=\"128\" blue=\"128\" compliance=\"SVG\" />"
    "  <color name=\"fuchsia\" red=\"255\" green=\"0\" blue=\"255\" compliance=\"SVG\" />"
    "  <color name=\"gainsboro\" red=\"220\" green=\"220\" blue=\"220\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"GhostWhite\" red=\"248\" green=\"248\" blue=\"255\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"gold\" red=\"255\" green=\"215\" blue=\"0\" compliance=\"X11, XPM\" />"
    "  <color name=\"goldenrod\" red=\"218\" green=\"165\" blue=\"32\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"gray\" red=\"126\" green=\"126\" blue=\"126\" compliance=\"SVG\" />"
    "  <color name=\"gray74\" red=\"189\" green=\"189\" blue=\"189 \" compliance=\"SVG, X11\" />"
    "  <color name=\"gray100\" red=\"255\" green=\"255\" blue=\"255 \" compliance=\"SVG, X11\" />"
    "  <color name=\"green\" red=\"0\" green=\"128\" blue=\"0\" compliance=\"SVG\" />"
    "  <color name=\"grey\" red=\"190\" green=\"190\" blue=\"190\" compliance=\"SVG, X11\" />"
    "  <color name=\"grey0\" red=\"0\" green=\"0\" blue=\"0\" compliance=\"SVG, X11\" />"
    "  <color name=\"grey1\" red=\"3\" green=\"3\" blue=\"3\" compliance=\"SVG, X11\" />"
    "  <color name=\"grey10\" red=\"26\" green=\"26\" blue=\"26 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey100\" red=\"255\" green=\"255\" blue=\"255 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey11\" red=\"28\" green=\"28\" blue=\"28 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey12\" red=\"31\" green=\"31\" blue=\"31 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey13\" red=\"33\" green=\"33\" blue=\"33 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey14\" red=\"36\" green=\"36\" blue=\"36 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey15\" red=\"38\" green=\"38\" blue=\"38 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey16\" red=\"41\" green=\"41\" blue=\"41 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey17\" red=\"43\" green=\"43\" blue=\"43 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey18\" red=\"46\" green=\"46\" blue=\"46 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey19\" red=\"48\" green=\"48\" blue=\"48 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey2\" red=\"5\" green=\"5\" blue=\"5\" compliance=\"SVG, X11\" />"
    "  <color name=\"grey20\" red=\"51\" green=\"51\" blue=\"51 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey21\" red=\"54\" green=\"54\" blue=\"54 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey22\" red=\"56\" green=\"56\" blue=\"56 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey23\" red=\"59\" green=\"59\" blue=\"59 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey24\" red=\"61\" green=\"61\" blue=\"61 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey25\" red=\"64\" green=\"64\" blue=\"64 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey26\" red=\"66\" green=\"66\" blue=\"66 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey27\" red=\"69\" green=\"69\" blue=\"69 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey28\" red=\"71\" green=\"71\" blue=\"71 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey29\" red=\"74\" green=\"74\" blue=\"74 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey3\" red=\"8\" green=\"8\" blue=\"8\" compliance=\"SVG, X11\" />"
    "  <color name=\"grey30\" red=\"77\" green=\"77\" blue=\"77 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey31\" red=\"79\" green=\"79\" blue=\"79 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey32\" red=\"82\" green=\"82\" blue=\"82 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey33\" red=\"84\" green=\"84\" blue=\"84 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey34\" red=\"87\" green=\"87\" blue=\"87 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey35\" red=\"89\" green=\"89\" blue=\"89 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey36\" red=\"92\" green=\"92\" blue=\"92 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey37\" red=\"94\" green=\"94\" blue=\"94 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey38\" red=\"97\" green=\"97\" blue=\"97 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey39\" red=\"99\" green=\"99\" blue=\"99 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey4\" red=\"10\" green=\"10\" blue=\"10 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey40\" red=\"102\" green=\"102\" blue=\"102 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey41\" red=\"105\" green=\"105\" blue=\"105 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey42\" red=\"107\" green=\"107\" blue=\"107 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey43\" red=\"110\" green=\"110\" blue=\"110 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey44\" red=\"112\" green=\"112\" blue=\"112 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey45\" red=\"115\" green=\"115\" blue=\"115 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey46\" red=\"117\" green=\"117\" blue=\"117 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey47\" red=\"120\" green=\"120\" blue=\"120 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey48\" red=\"122\" green=\"122\" blue=\"122 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey49\" red=\"125\" green=\"125\" blue=\"125 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey5\" red=\"13\" green=\"13\" blue=\"13 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey50\" red=\"127\" green=\"127\" blue=\"127 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey51\" red=\"130\" green=\"130\" blue=\"130 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey52\" red=\"133\" green=\"133\" blue=\"133 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey53\" red=\"135\" green=\"135\" blue=\"135 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey54\" red=\"138\" green=\"138\" blue=\"138 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey55\" red=\"140\" green=\"140\" blue=\"140 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey56\" red=\"143\" green=\"143\" blue=\"143 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey57\" red=\"145\" green=\"145\" blue=\"145 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey58\" red=\"148\" green=\"148\" blue=\"148 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey59\" red=\"150\" green=\"150\" blue=\"150 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey6\" red=\"15\" green=\"15\" blue=\"15 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey60\" red=\"153\" green=\"153\" blue=\"153 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey61\" red=\"156\" green=\"156\" blue=\"156 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey62\" red=\"158\" green=\"158\" blue=\"158 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey63\" red=\"161\" green=\"161\" blue=\"161 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey64\" red=\"163\" green=\"163\" blue=\"163 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey65\" red=\"166\" green=\"166\" blue=\"166 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey66\" red=\"168\" green=\"168\" blue=\"168 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey67\" red=\"171\" green=\"171\" blue=\"171 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey68\" red=\"173\" green=\"173\" blue=\"173 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey69\" red=\"176\" green=\"176\" blue=\"176 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey7\" red=\"18\" green=\"18\" blue=\"18 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey70\" red=\"179\" green=\"179\" blue=\"179 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey71\" red=\"181\" green=\"181\" blue=\"181 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey72\" red=\"184\" green=\"184\" blue=\"184 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey73\" red=\"186\" green=\"186\" blue=\"186 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey74\" red=\"189\" green=\"189\" blue=\"189 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey75\" red=\"191\" green=\"191\" blue=\"191 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey76\" red=\"194\" green=\"194\" blue=\"194 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey77\" red=\"196\" green=\"196\" blue=\"196 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey78\" red=\"199\" green=\"199\" blue=\"199 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey79\" red=\"201\" green=\"201\" blue=\"201 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey8\" red=\"20\" green=\"20\" blue=\"20 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey80\" red=\"204\" green=\"204\" blue=\"204 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey81\" red=\"207\" green=\"207\" blue=\"207 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey82\" red=\"209\" green=\"209\" blue=\"209 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey83\" red=\"212\" green=\"212\" blue=\"212 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey84\" red=\"214\" green=\"214\" blue=\"214 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey85\" red=\"217\" green=\"217\" blue=\"217 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey86\" red=\"219\" green=\"219\" blue=\"219 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey87\" red=\"222\" green=\"222\" blue=\"222 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey88\" red=\"224\" green=\"224\" blue=\"224 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey89\" red=\"227\" green=\"227\" blue=\"227 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey9\" red=\"23\" green=\"23\" blue=\"23 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey90\" red=\"229\" green=\"229\" blue=\"229 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey91\" red=\"232\" green=\"232\" blue=\"232 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey92\" red=\"235\" green=\"235\" blue=\"235 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey93\" red=\"237\" green=\"237\" blue=\"237 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey94\" red=\"240\" green=\"240\" blue=\"240 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey95\" red=\"242\" green=\"242\" blue=\"242 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey96\" red=\"245\" green=\"245\" blue=\"245 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey97\" red=\"247\" green=\"247\" blue=\"247 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey98\" red=\"250\" green=\"250\" blue=\"250 \" compliance=\"SVG, X11\" />"
    "  <color name=\"grey99\" red=\"252\" green=\"252\" blue=\"252 \" compliance=\"SVG, X11\" />"
    "  <color name=\"honeydew\" red=\"240\" green=\"255\" blue=\"240\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"HotPink\" red=\"255\" green=\"105\" blue=\"180\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"IndianRed\" red=\"205\" green=\"92\" blue=\"92\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"indigo\" red=\"75\" green=\"0\" blue=\"130\" compliance=\"SVG\" />"
    "  <color name=\"ivory\" red=\"255\" green=\"255\" blue=\"240\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"khaki\" red=\"240\" green=\"230\" blue=\"140\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"lavender\" red=\"230\" green=\"230\" blue=\"250\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"LavenderBlush\" red=\"255\" green=\"240\" blue=\"245\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"LawnGreen\" red=\"124\" green=\"252\" blue=\"0\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"LemonChiffon\" red=\"255\" green=\"250\" blue=\"205\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"LightBlue\" red=\"173\" green=\"216\" blue=\"230\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"LightCoral\" red=\"240\" green=\"128\" blue=\"128\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"LightCyan\" red=\"224\" green=\"255\" blue=\"255\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"LightGoldenrodYellow\" red=\"250\" green=\"250\" blue=\"210\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"LightGray\" red=\"211\" green=\"211\" blue=\"211\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"LightGreen\" red=\"144\" green=\"238\" blue=\"144\" compliance=\"SVG, X11\" />"
    "  <color name=\"LightGrey\" red=\"211\" green=\"211\" blue=\"211\" compliance=\"SVG, X11\" />"
    "  <color name=\"LightPink\" red=\"255\" green=\"182\" blue=\"193\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"LightSalmon\" red=\"255\" green=\"160\" blue=\"122\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"LightSeaGreen\" red=\"32\" green=\"178\" blue=\"170\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"LightSkyBlue\" red=\"135\" green=\"206\" blue=\"250\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"LightSlateGray\" red=\"119\" green=\"136\" blue=\"153\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"LightSlateGrey\" red=\"119\" green=\"136\" blue=\"153\" compliance=\"SVG, X11\" />"
    "  <color name=\"LightSteelBlue\" red=\"176\" green=\"196\" blue=\"222\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"LightYellow\" red=\"255\" green=\"255\" blue=\"224\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"lime\" red=\"0\" green=\"255\" blue=\"0\" compliance=\"SVG\" />"
    "  <color name=\"LimeGreen\" red=\"50\" green=\"205\" blue=\"50\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"linen\" red=\"250\" green=\"240\" blue=\"230\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"magenta\" red=\"255\" green=\"0\" blue=\"255\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"maroon\" red=\"128\" green=\"0\" blue=\"0\" compliance=\"SVG\" />"
    "  <color name=\"MediumAquamarine\" red=\"102\" green=\"205\" blue=\"170\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"MediumBlue\" red=\"0\" green=\"0\" blue=\"205\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"MediumOrchid\" red=\"186\" green=\"85\" blue=\"211\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"MediumPurple\" red=\"147\" green=\"112\" blue=\"219\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"MediumSeaGreen\" red=\"60\" green=\"179\" blue=\"113\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"MediumSlateBlue\" red=\"123\" green=\"104\" blue=\"238\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"MediumSpringGreen\" red=\"0\" green=\"250\" blue=\"154\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"MediumTurquoise\" red=\"72\" green=\"209\" blue=\"204\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"MediumVioletRed\" red=\"199\" green=\"21\" blue=\"133\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"MidnightBlue\" red=\"25\" green=\"25\" blue=\"112\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"MintCream\" red=\"245\" green=\"255\" blue=\"250\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"MistyRose\" red=\"255\" green=\"228\" blue=\"225\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"moccasin\" red=\"255\" green=\"228\" blue=\"181\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"NavajoWhite\" red=\"255\" green=\"222\" blue=\"173\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"navy\" red=\"0\" green=\"0\" blue=\"128\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"none\" red=\"0\" green=\"0\" blue=\"0\" opacity=\"255\" compliance=\"SVG\" />"
    "  <color name=\"OldLace\" red=\"253\" green=\"245\" blue=\"230\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"olive\" red=\"128\" green=\"128\" blue=\"0\" compliance=\"SVG\" />"
    "  <color name=\"OliveDrab\" red=\"107\" green=\"142\" blue=\"35\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"opaque\" opacity=\"0\" compliance=\"SVG\" />"
    "  <color name=\"orange\" red=\"255\" green=\"165\" blue=\"0\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"OrangeRed\" red=\"255\" green=\"69\" blue=\"0\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"orchid\" red=\"218\" green=\"112\" blue=\"214\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"PaleGoldenrod\" red=\"238\" green=\"232\" blue=\"170\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"PaleGreen\" red=\"152\" green=\"251\" blue=\"152\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"PaleTurquoise\" red=\"175\" green=\"238\" blue=\"238\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"PaleVioletRed\" red=\"219\" green=\"112\" blue=\"147\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"PapayaWhip\" red=\"255\" green=\"239\" blue=\"213\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"PeachPuff\" red=\"255\" green=\"218\" blue=\"185\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"peru\" red=\"205\" green=\"133\" blue=\"63\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"pink\" red=\"255\" green=\"192\" blue=\"203\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"plum\" red=\"221\" green=\"160\" blue=\"221\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"PowderBlue\" red=\"176\" green=\"224\" blue=\"230\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"purple\" red=\"128\" green=\"0\" blue=\"128\" compliance=\"SVG\" />"
    "  <color name=\"red\" red=\"255\" green=\"0\" blue=\"0\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"RosyBrown\" red=\"188\" green=\"143\" blue=\"143\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"RoyalBlue\" red=\"65\" green=\"105\" blue=\"225\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"SaddleBrown\" red=\"139\" green=\"69\" blue=\"19\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"salmon\" red=\"250\" green=\"128\" blue=\"114\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"SandyBrown\" red=\"244\" green=\"164\" blue=\"96\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"SeaGreen\" red=\"46\" green=\"139\" blue=\"87\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"seashell\" red=\"255\" green=\"245\" blue=\"238\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"sienna\" red=\"160\" green=\"82\" blue=\"45\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"silver\" red=\"192\" green=\"192\" blue=\"192\" compliance=\"SVG\" />"
    "  <color name=\"SkyBlue\" red=\"135\" green=\"206\" blue=\"235\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"SlateBlue\" red=\"106\" green=\"90\" blue=\"205\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"SlateGray\" red=\"112\" green=\"128\" blue=\"144\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"SlateGrey\" red=\"112\" green=\"128\" blue=\"144\" compliance=\"SVG, X11\" />"
    "  <color name=\"snow\" red=\"255\" green=\"250\" blue=\"250\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"SpringGreen\" red=\"0\" green=\"255\" blue=\"127\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"SteelBlue\" red=\"70\" green=\"130\" blue=\"180\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"tan\" red=\"210\" green=\"180\" blue=\"140\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"teal\" red=\"0\" green=\"128\" blue=\"128\" compliance=\"SVG\" />"
    "  <color name=\"thistle\" red=\"216\" green=\"191\" blue=\"216\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"tomato\" red=\"255\" green=\"99\" blue=\"71\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"transparent\" opacity=\"255\" compliance=\"SVG\" />"
    "  <color name=\"turquoise\" red=\"64\" green=\"224\" blue=\"208\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"violet\" red=\"238\" green=\"130\" blue=\"238\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"wheat\" red=\"245\" green=\"222\" blue=\"179\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"WhiteSmoke\" red=\"245\" green=\"245\" blue=\"245\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"yellow\" red=\"255\" green=\"255\" blue=\"0\" compliance=\"SVG, X11, XPM\" />"
    "  <color name=\"YellowGreen\" red=\"154\" green=\"205\" blue=\"50\" compliance=\"SVG, X11, XPM\" />"
    "</colormap>";

/*
  Typedef declarations.
*/
typedef struct _NodeInfo
{
  struct _NodeInfo
    *child[MaxTreeDepth];

  ColorPacket
    *list;

  MagickSizeType
    number_unique;

  unsigned long
    level;
} NodeInfo;

typedef struct _Nodes
{
  NodeInfo
    nodes[NodesInAList];

  struct _Nodes
    *next;
} Nodes;

typedef struct _CubeInfo
{
  NodeInfo
    *root;

  long
    progress;

  unsigned long
    colors,
    free_nodes;

  NodeInfo
    *node_info;

  Nodes
    *node_queue;
} CubeInfo;

/*
  Static declarations.
*/
static LinkedListInfo
  *color_list = (LinkedListInfo *) NULL;

static SemaphoreInfo
  *color_semaphore = (SemaphoreInfo *) NULL;

static volatile MagickBooleanType
  instantiate_color = MagickFalse;

/*
  Forward declarations.
*/
static CubeInfo
  *GetCubeInfo(void);

static NodeInfo
  *GetNodeInfo(CubeInfo *,const unsigned long);

static MagickBooleanType
  InitializeColorList(ExceptionInfo *),
  LoadColorLists(const char *,ExceptionInfo *);

static void
  DestroyColorCube(NodeInfo *),
  HistogramToFile(const Image *,CubeInfo *,const NodeInfo *,FILE *,
    ExceptionInfo *);

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   C l a s s i f y I m a g e C o l o r s                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ClassifyImageColors() builds a populated CubeInfo tree for the specified
%  image.  The returned tree should be deallocated using DestroyCubeInfo()
%  once it is no longer needed.
%
%  The format of the ClassifyImageColors() method is:
%
%      CubeInfo *ClassifyImageColors(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: Specifies a pointer to an Image structure;  returned from
%      ReadImage.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

static inline unsigned long ColorToNodeId(const PixelPacket *pixel,
  unsigned long index,const MagickBooleanType matte)
{
  unsigned long
    id;

  id=(unsigned long) (((ScaleQuantumToChar(pixel->red) >> index) & 0x01) << 3 |
    ((ScaleQuantumToChar(pixel->green) >> index) & 0x01) << 2 |
    ((ScaleQuantumToChar(pixel->blue) >> index) & 0x01) << 1);
  if (matte != MagickFalse)
    id|=(unsigned long) ((ScaleQuantumToChar(pixel->opacity) >> index) & 0x01);
  return(id);
}

static CubeInfo *ClassifyImageColors(const Image *image,
  ExceptionInfo *exception)
{
#define EvaluateImageColorsText  "  Compute image colors...  "

  CubeInfo
    *cube_info;

  long
    y;

  MagickBooleanType
    status;

  MagickPixelPacket
    target,
    pixel;

  NodeInfo
    *node_info;

  register const PixelPacket
    *p;

  register IndexPacket
    *indexes;

  register long
    i,
    x;

  register unsigned long
    id,
    index,
    level;

  /*
    Initialize color description tree.
  */
  assert(image != (const Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  cube_info=GetCubeInfo();
  if (cube_info == (CubeInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      return(cube_info);
    }
  GetMagickPixelPacket(image,&pixel);
  GetMagickPixelPacket(image,&target);
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    if (p == (const PixelPacket *) NULL)
      break;
    indexes=GetIndexes(image);
    for (x=0; x < (long) image->columns; x++)
    {
      /*
        Start at the root and proceed level by level.
      */
      node_info=cube_info->root;
      index=MaxTreeDepth-1;
      for (level=1; level < MaxTreeDepth; level++)
      {
        id=ColorToNodeId(p,index,image->matte);
        if (node_info->child[id] == (NodeInfo *) NULL)
          {
            node_info->child[id]=GetNodeInfo(cube_info,level);
            if (node_info->child[id] == (NodeInfo *) NULL)
              {
                (void) ThrowMagickException(exception,GetMagickModule(),
                  ResourceLimitError,"MemoryAllocationFailed","`%s'",
                  image->filename);
                return(0);
              }
          }
        node_info=node_info->child[id];
        index--;
      }
      SetMagickPixelPacket(p,indexes+x,&pixel);
      for (i=0; i < (long) node_info->number_unique; i++)
      {
        SetMagickPixelPacket(&node_info->list[i].pixel,
          &node_info->list[i].index,&target);
        if (IsMagickColorEqual(&pixel,&target) != MagickFalse)
          break;
      }
      if (i < (long) node_info->number_unique)
        node_info->list[i].count++;
      else
        {
          if (node_info->number_unique == 0)
            node_info->list=(ColorPacket *)
              AcquireMagickMemory(sizeof(*node_info->list));
          else
            node_info->list=(ColorPacket *) ResizeMagickMemory(node_info->list,
              (size_t) (i+1)*sizeof(*node_info->list));
          if (node_info->list == (ColorPacket *) NULL)
            {
              (void) ThrowMagickException(exception,GetMagickModule(),
                ResourceLimitError,"MemoryAllocationFailed","`%s'",
                image->filename);
              return(0);
            }
          node_info->list[i].pixel=(*p);
          if (image->colorspace == CMYKColorspace)
            node_info->list[i].index=indexes[x];
          node_info->list[i].count=1;
          node_info->number_unique++;
          cube_info->colors++;
        }
      p++;
    }
    if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
        (QuantumTick(y,image->rows) != MagickFalse))
      {
        status=image->progress_monitor(EvaluateImageColorsText,y,image->rows,
          image->client_data);
        if (status == MagickFalse)
          break;
      }
  }
  return(cube_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e f i n e I m a g e H i s t o g r a m                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DefineImageHistogram() traverses the color cube tree and notes each colormap
%  entry.  A colormap entry is any node in the color cube tree where the
%  of unique colors is not zero.
%
%  The format of the DefineImageHistogram method is:
%
%      DefineImageHistogram(NodeInfo *node_info,ColorPacket **unique_colors)
%
%  A description of each parameter follows.
%
%    o node_info: The address of a structure of type NodeInfo which points to a
%      node in the color cube tree that is to be pruned.
%
%    o histogram: The image histogram.
%
%
*/
static void DefineImageHistogram(NodeInfo *node_info,ColorPacket **histogram)
{
  register unsigned long
    id;

  /*
    Traverse any children.
  */
  for (id=0; id < MaxTreeDepth; id++)
    if (node_info->child[id] != (NodeInfo *) NULL)
      DefineImageHistogram(node_info->child[id],histogram);
  if (node_info->level == (MaxTreeDepth-1))
    {
      register ColorPacket
        *p;

      register long
        i;

      p=node_info->list;
      for (i=0; i < (long) node_info->number_unique; i++)
      {
        (*histogram)->pixel=p->pixel;
        (*histogram)->index=p->index;
        (*histogram)->count=p->count;
        (*histogram)++;
        p++;
      }
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y C o l o r L i s t                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyColorList() deallocates memory associated with the color list.
%
%  The format of the DestroyColorList method is:
%
%      DestroyColorList(void)
%
%
*/

static void *DestroyColorElement(void *color_info)
{
  register ColorInfo
    *p;

  p=(ColorInfo *) color_info;
  if (p->path != (char *) NULL)
    p->path=(char *) RelinquishMagickMemory(p->path);
  if (p->name != (char *) NULL)
    p->name=(char *) RelinquishMagickMemory(p->name);
  p=(ColorInfo *) RelinquishMagickMemory(p);
  return((void *) NULL);
}

MagickExport void DestroyColorList(void)
{
  AcquireSemaphoreInfo(&color_semaphore);
  if (color_list != (LinkedListInfo *) NULL)
    color_list=DestroyLinkedList(color_list,DestroyColorElement);
  instantiate_color=MagickFalse;
  RelinquishSemaphoreInfo(color_semaphore);
  color_semaphore=DestroySemaphoreInfo(color_semaphore);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   D e s t r o y C u b e I n f o                                             %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyCubeInfo() deallocates memory associated with a CubeInfo structure.
%
%  The format of the DestroyCubeInfo method is:
%
%      DestroyCubeInfo(CubeInfo *cube_info)
%
%  A description of each parameter follows:
%
%    o cube_info: The address of a structure of type CubeInfo.
%
%
*/
static CubeInfo *DestroyCubeInfo(CubeInfo *cube_info)
{
  register Nodes
    *nodes;

  /*
    Release color cube tree storage.
  */
  DestroyColorCube(cube_info->root);
  do
  {
    nodes=cube_info->node_queue->next;
    cube_info->node_queue=(Nodes *)
      RelinquishMagickMemory(cube_info->node_queue);
    cube_info->node_queue=nodes;
  } while (cube_info->node_queue != (Nodes *) NULL);
  return((CubeInfo *) RelinquishMagickMemory(cube_info));
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  D e s t r o y C o l o r C u b e                                            %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  DestroyColorCube() traverses the color cube tree and frees the list of
%  unique colors.
%
%  The format of the DestroyColorCube method is:
%
%      void DestroyColorCube(const NodeInfo *node_info)
%
%  A description of each parameter follows.
%
%    o node_info: The address of a structure of type NodeInfo which points to a
%      node in the color cube tree that is to be pruned.
%
%
*/
static void DestroyColorCube(NodeInfo *node_info)
{
  register long
    id;

  /*
    Traverse any children.
  */
  for (id=0; id < MaxTreeDepth; id++)
    if (node_info->child[id] != (NodeInfo *) NULL)
      DestroyColorCube(node_info->child[id]);
  if (node_info->list != (ColorPacket *) NULL)
    node_info->list=(ColorPacket *) RelinquishMagickMemory(node_info->list);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   F u z z y C o l o r C o m p a r e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FuzzyColorCompare() returns MagickTrue if the distance between two colors is
%  less than the specified distance in a linear three dimensional color space.
%  This method is used by ColorFloodFill() and other algorithms which
%  compare two colors.
%
%  The format of the FuzzyColorCompare method is:
%
%      void FuzzyColorCompare(const Image *image,const PixelPacket *p,
%        const PixelPacket *q)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o p: Pixel p.
%
%    o q: Pixel q.
%
%
*/
MagickExport MagickBooleanType FuzzyColorCompare(const Image *image,
  const PixelPacket *p,const PixelPacket *q)
{
  MagickRealType
    fuzz,
    pixel;

  register MagickRealType
    distance;

  if (image->matte == MagickFalse)
    {
      if ((p->red == q->red) && (p->green == q->green) && (p->blue == q->blue))
        return(MagickTrue);
      fuzz=3.0*image->fuzz*image->fuzz;
    }
  else
    {
      if ((p->red == q->red) && (p->green == q->green) &&
          (p->blue == q->blue) && (p->opacity == q->opacity))
        return(MagickTrue);
      fuzz=4.0*image->fuzz*image->fuzz;
    }
  pixel=p->red-(MagickRealType) q->red;
  distance=pixel*pixel;
  if (distance > fuzz)
    return(MagickFalse);
  pixel=p->green-(MagickRealType) q->green;
  distance+=pixel*pixel;
  if (distance > fuzz)
    return(MagickFalse);
  pixel=p->blue-(MagickRealType) q->blue;
  distance+=pixel*pixel;
  if (distance > fuzz)
    return(MagickFalse);
  if (image->matte != MagickFalse)
    {
      pixel=p->opacity-(MagickRealType) q->opacity;
      distance+=pixel*pixel;
      if (distance > fuzz)
        return(MagickFalse);
    }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   F u z z y O p a c i t y C o m p a r e                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  FuzzyOpacityCompare() returns true if the distance between two opacity
%  values is less than the specified distance in a linear color space.  This
%  method is used by MatteFloodFill() and other algorithms which compare
%  two opacity values.
%
%  The format of the FuzzyOpacityCompare method is:
%
%      void FuzzyOpacityCompare(const Image *image,const PixelPacket *p,
%        const PixelPacket *q)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o p: Pixel p.
%
%    o q: Pixel q.
%
%
*/
MagickExport MagickBooleanType FuzzyOpacityCompare(const Image *image,
  const PixelPacket *p,const PixelPacket *q)
{
  MagickRealType
    fuzz,
    pixel;

  register MagickRealType
    distance;

  if (p->opacity == q->opacity)
    return(MagickTrue);
  fuzz=image->fuzz*image->fuzz;
  pixel=p->opacity-(MagickRealType) q->opacity;
  distance=pixel*pixel;
  if (distance > fuzz)
    return(MagickFalse);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t C o l o r I n f o                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetColorInfo() searches the color list for the specified name and if found
%  returns attributes for that color.
%
%  The format of the GetColorInfo method is:
%
%      const PixelPacket *GetColorInfo(const char *name,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o color_info: Method GetColorInfo searches the color list for the
%      specified name and if found returns attributes for that color.
%
%    o name: The color name.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport const ColorInfo *GetColorInfo(const char *name,
  ExceptionInfo *exception)
{
  char
    colorname[MaxTextExtent];

  register const ColorInfo
    *p;

  register char
    *q;

  assert(exception != (ExceptionInfo *) NULL);
  if ((color_list == (LinkedListInfo *) NULL) ||
      (instantiate_color == MagickFalse))
    if (InitializeColorList(exception) == MagickFalse)
      return((const ColorInfo *) NULL);
  if ((color_list == (LinkedListInfo *) NULL) ||
      (IsLinkedListEmpty(color_list) != MagickFalse))
    return((const ColorInfo *) NULL);
  if ((name == (const char *) NULL) || (LocaleCompare(name,"*") == 0))
    return((const ColorInfo *) GetValueFromLinkedList(color_list,0));
  /*
    Strip names of whitespace.
  */
  (void) CopyMagickString(colorname,name,MaxTextExtent);
  for (q=colorname; *q != '\0'; q++)
  {
    if (isspace((int) ((unsigned char) *q)) == 0)
      continue;
    (void) CopyMagickString(q,q+1,MaxTextExtent);
    q--;
  }
  /*
    Search for named color.
  */
  AcquireSemaphoreInfo(&color_semaphore);
  ResetLinkedListIterator(color_list);
  p=(const ColorInfo *) GetNextValueInLinkedList(color_list);
  while (p != (const ColorInfo *) NULL)
  {
    if (LocaleCompare(colorname,p->name) == 0)
      break;
    p=(const ColorInfo *) GetNextValueInLinkedList(color_list);
  }
  if (p == (ColorInfo *) NULL)
    (void) ThrowMagickException(exception,GetMagickModule(),OptionWarning,
      "UnrecognizedColor","`%s'",name);
  RelinquishSemaphoreInfo(color_semaphore);
  return(p);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t C o l o r I n f o L i s t                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetColorInfoList() returns any colors that match the specified pattern.
%
%  The format of the GetColorInfoList function is:
%
%      const ColorInfo **GetColorInfoList(const char *pattern,
%        unsigned long *number_colors,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_colors:  This integer returns the number of colors in the list.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int ColorInfoCompare(const void *x,const void *y)
{
  const ColorInfo
    **p,
    **q;

  p=(const ColorInfo **) x,
  q=(const ColorInfo **) y;
  if (LocaleCompare((*p)->path,(*q)->path) == 0)
    return(LocaleCompare((*p)->name,(*q)->name));
  return(LocaleCompare((*p)->path,(*q)->path));
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

MagickExport const ColorInfo **GetColorInfoList(const char *pattern,
  unsigned long *number_colors,ExceptionInfo *exception)
{
  const ColorInfo
    **colors;

  register const ColorInfo
    *p;

  register long
    i;

  /*
    Allocate color list.
  */
  assert(pattern != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  assert(number_colors != (unsigned long *) NULL);
  *number_colors=0;
  p=GetColorInfo("*",exception);
  if (p == (const ColorInfo *) NULL)
    return((const ColorInfo **) NULL);
  colors=(const ColorInfo **) AcquireMagickMemory((size_t)
    (GetNumberOfElementsInLinkedList(color_list)+1)*sizeof(*colors));
  if (colors == (const ColorInfo **) NULL)
    return((const ColorInfo **) NULL);
  /*
    Generate color list.
  */
  AcquireSemaphoreInfo(&color_semaphore);
  ResetLinkedListIterator(color_list);
  p=(const ColorInfo *) GetNextValueInLinkedList(color_list);
  for (i=0; p != (const ColorInfo *) NULL; )
  {
    if ((p->stealth == MagickFalse) &&
        (GlobExpression(p->name,pattern) != MagickFalse))
      colors[i++]=p;
    p=(const ColorInfo *) GetNextValueInLinkedList(color_list);
  }
  RelinquishSemaphoreInfo(color_semaphore);
  qsort((void *) colors,(size_t) i,sizeof(*colors),ColorInfoCompare);
  colors[i]=(ColorInfo *) NULL;
  *number_colors=(unsigned long) i;
  return(colors);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t C o l o r L i s t                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetColorList() returns any colors that match the specified pattern.
%
%  The format of the GetColorList function is:
%
%      char **GetColorList(const char *pattern,unsigned long *number_colors,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o pattern: Specifies a pointer to a text string containing a pattern.
%
%    o number_colors:  This integer returns the number of colors in the list.
%
%    o exception: Return any errors or warnings in this structure.
%
*/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

static int ColorCompare(const void *x,const void *y)
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

MagickExport char **GetColorList(const char *pattern,
  unsigned long *number_colors,ExceptionInfo *exception)
{
  char
    **colors;

  register const ColorInfo
    *p;

  register long
    i;

  /*
    Allocate color list.
  */
  assert(pattern != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",pattern);
  assert(number_colors != (unsigned long *) NULL);
  *number_colors=0;
  p=GetColorInfo("*",exception);
  if (p == (const ColorInfo *) NULL)
    return((char **) NULL);
  colors=(char **) AcquireMagickMemory((size_t)
    (GetNumberOfElementsInLinkedList(color_list)+1)*sizeof(*colors));
  if (colors == (char **) NULL)
    return((char **) NULL);
  /*
    Generate color list.
  */
  AcquireSemaphoreInfo(&color_semaphore);
  ResetLinkedListIterator(color_list);
  p=(const ColorInfo *) GetNextValueInLinkedList(color_list);
  for (i=0; p != (const ColorInfo *) NULL; )
  {
    if ((p->stealth == MagickFalse) &&
        (GlobExpression(p->name,pattern) != MagickFalse))
      colors[i++]=ConstantString(AcquireString(p->name));
    p=(const ColorInfo *) GetNextValueInLinkedList(color_list);
  }
  RelinquishSemaphoreInfo(color_semaphore);
  qsort((void *) colors,(size_t) i,sizeof(*colors),ColorCompare);
  colors[i]=(char *) NULL;
  *number_colors=(unsigned long) i;
  return(colors);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t C o l o r T u p l e                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetColorTuple() returns a color as a color tuple string.
%
%  The format of the GetColorTuple method is:
%
%      GetColorTuple(const MagickPixelPacket *pixel,const MagickBooleanType hex,
%        char *tuple)
%
%  A description of each parameter follows.
%
%    o pixel: The pixel.
%
%    o hex: A value other than zero returns the tuple in a hexidecimal format.
%
%    o tuple: Return the color tuple as this string.
%
%
*/
MagickExport void GetColorTuple(const MagickPixelPacket *pixel,
  const MagickBooleanType hex,char *tuple)
{
  assert(pixel != (const MagickPixelPacket *) NULL);
  assert(tuple != (char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",tuple);
  if (pixel->matte != MagickFalse)
    {
      if (pixel->depth <= 8)
        {
          if (pixel->colorspace != CMYKColorspace)
            {
              (void) FormatMagickString(tuple,MaxTextExtent,hex != MagickFalse ?
                "#%02X%02X%02X%02X" : "(%3u,%3u,%3u,%3u)",
                ScaleQuantumToChar(pixel->red),ScaleQuantumToChar(pixel->green),
                ScaleQuantumToChar(pixel->blue),
                ScaleQuantumToChar(pixel->opacity));
              return;
            }
          (void) FormatMagickString(tuple,MaxTextExtent,hex != MagickFalse ?
            "#%02X%02X%02X%02X%02X" : "(%3u,%3u,%3u,%3u,%3u)",
            ScaleQuantumToChar(pixel->red),ScaleQuantumToChar(pixel->green),
            ScaleQuantumToChar(pixel->blue),ScaleQuantumToChar(pixel->index),
            ScaleQuantumToChar(pixel->opacity));
          return;
        }
      if (pixel->depth <= 16)
        {
          if (pixel->colorspace != CMYKColorspace)
            {
              (void) FormatMagickString(tuple,MaxTextExtent,hex != MagickFalse ?
                "#%04X%04X%04X%04X" : "(%5u,%5u,%5u,%5u)",
                ScaleQuantumToShort(pixel->red),
                ScaleQuantumToShort(pixel->green),
                ScaleQuantumToShort(pixel->blue),
                ScaleQuantumToShort(pixel->opacity));
              return;
            }
          (void) FormatMagickString(tuple,MaxTextExtent,hex != MagickFalse ?
            "#%04X%04X%04X%04X%04X" : "(%5u,%5u,%5u,%5u,%5u)",
            ScaleQuantumToShort(pixel->red),ScaleQuantumToShort(pixel->green),
            ScaleQuantumToShort(pixel->blue),ScaleQuantumToChar(pixel->index),
            ScaleQuantumToShort(pixel->opacity));
          return;
        }
      if (pixel->colorspace != CMYKColorspace)
        {
          (void) FormatMagickString(tuple,MaxTextExtent,hex != MagickFalse ?
            "#%08lX%08lX%08lX%08lX" : "(%10lu,%10lu,%10lu,%10lu)",
            ScaleQuantumToLong(pixel->red),ScaleQuantumToLong(pixel->green),
            ScaleQuantumToLong(pixel->blue),ScaleQuantumToLong(pixel->opacity));
          return;
        }
      (void) FormatMagickString(tuple,MaxTextExtent,hex != MagickFalse ?
        "#%08lX%08lX%08lX%08lX%08lX" : "(%10lu,%10lu,%10lu,%10lu,%10lu)",
        ScaleQuantumToLong(pixel->red),ScaleQuantumToLong(pixel->green),
        ScaleQuantumToLong(pixel->blue),ScaleQuantumToLong(pixel->index),
        ScaleQuantumToLong(pixel->opacity));
      return;
    }
  if (pixel->depth <= 8)
    {
      if (pixel->colorspace != CMYKColorspace)
        {
          (void) FormatMagickString(tuple,MaxTextExtent,hex != MagickFalse ?
            "#%02X%02X%02X" : "(%3u,%3u,%3u)",
            ScaleQuantumToChar(pixel->red),ScaleQuantumToChar(pixel->green),
            ScaleQuantumToChar(pixel->blue));
          return;
        }
      (void) FormatMagickString(tuple,MaxTextExtent,hex != MagickFalse ?
        "#%02X%02X%02X%02X" : "(%3u,%3u,%3u,%3u)",
        ScaleQuantumToChar(pixel->red),ScaleQuantumToChar(pixel->green),
        ScaleQuantumToChar(pixel->blue),ScaleQuantumToChar(pixel->index));
      return;
    }
  if (pixel->depth <= 16)
    {
      if (pixel->colorspace != CMYKColorspace)
        {
          (void) FormatMagickString(tuple,MaxTextExtent,hex != MagickFalse ?
            "#%04X%04X%04X" : "(%5u,%5u,%5u)",
            ScaleQuantumToShort(pixel->red),ScaleQuantumToShort(pixel->green),
            ScaleQuantumToShort(pixel->blue));
          return;
        }
      (void) FormatMagickString(tuple,MaxTextExtent,hex != MagickFalse ?
        "#%04X%04X%04X%04X" : "(%5u,%5u,%5u,%5u)",
        ScaleQuantumToShort(pixel->red),ScaleQuantumToShort(pixel->green),
        ScaleQuantumToShort(pixel->blue),ScaleQuantumToChar(pixel->index));
      return;
    }
  if (pixel->colorspace != CMYKColorspace)
    {
      (void) FormatMagickString(tuple,MaxTextExtent,hex != MagickFalse ?
        "#%08lX%08lX%08lX" : "(%10lu,%10lu,%10lu)",
        ScaleQuantumToLong(pixel->red),ScaleQuantumToLong(pixel->green),
        ScaleQuantumToLong(pixel->blue));
      return;
    }
  (void) FormatMagickString(tuple,MaxTextExtent,hex != MagickFalse ?
    "#%08lX%08lX%08lX%08lX" : "(%10lu,%10lu,%10lu,%10lu)",
    ScaleQuantumToLong(pixel->red),ScaleQuantumToLong(pixel->green),
    ScaleQuantumToLong(pixel->blue),ScaleQuantumToLong(pixel->index));
  return;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   G e t C u b e I n f o                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetCubeInfo() initializes the CubeInfo data structure.
%
%  The format of the GetCubeInfo method is:
%
%      cube_info=GetCubeInfo()
%
%  A description of each parameter follows.
%
%    o cube_info: A pointer to the Cube structure.
%
%
*/
static CubeInfo *GetCubeInfo(void)
{
  CubeInfo
    *cube_info;

  /*
    Initialize tree to describe color cube.
  */
  cube_info=(CubeInfo *) AcquireMagickMemory(sizeof(*cube_info));
  if (cube_info == (CubeInfo *) NULL)
    return((CubeInfo *) NULL);
  (void) ResetMagickMemory(cube_info,0,sizeof(*cube_info));
  /*
    Initialize root node.
  */
  cube_info->root=GetNodeInfo(cube_info,0);
  if (cube_info->root == (NodeInfo *) NULL)
    return((CubeInfo *) NULL);
  return(cube_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  G e t I m a g e H i s t o g r a m                                          %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetImageHistogram() returns the unique colors in an image.
%
%  The format of the GetImageHistogram method is:
%
%      unsigned long GetImageHistogram(const Image *image,
%        unsigned long *number_colors,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: The image.
%
%    o file:  Write a histogram of the color distribution to this file handle.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport ColorPacket *GetImageHistogram(const Image *image,
  unsigned long *number_colors,ExceptionInfo *exception)
{
  ColorPacket
    *histogram;

  CubeInfo
    *cube_info;

  *number_colors=0;
  histogram=(ColorPacket *) NULL;
  cube_info=ClassifyImageColors(image,exception);
  if (cube_info != (CubeInfo *) NULL)
    {
      histogram=(ColorPacket *)
        AcquireMagickMemory((size_t) cube_info->colors*sizeof(*histogram));
      if (histogram == (ColorPacket *) NULL)
        (void) ThrowMagickException(exception,GetMagickModule(),
          ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      else
        {
          ColorPacket
            *root;

          *number_colors=cube_info->colors;
          root=histogram;
          DefineImageHistogram(cube_info->root,&root);
        }
    }
  cube_info=DestroyCubeInfo(cube_info);
  return(histogram);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   G e t M a g i c k P i x e l P a c k e t                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetMagickPixelPacket() initializes the MagickPixelPacket structure.
%
%  The format of the GetMagickPixelPacket method is:
%
%      GetMagickPixelPacket(const Image *image,MagickPixelPacket *pixel)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o pixel: Specifies a pointer to a PixelPacket structure.
%
%
*/
MagickExport void GetMagickPixelPacket(const Image *image,
  MagickPixelPacket *pixel)
{
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"...");
  assert(pixel != (MagickPixelPacket *) NULL);
  (void) ResetMagickMemory(pixel,0,sizeof(*pixel));
  pixel->colorspace=RGBColorspace;
  pixel->depth=QuantumDepth;
  pixel->opacity=(MagickRealType) OpaqueOpacity;
  if (image == (const Image *) NULL)
    return;
  pixel->colorspace=image->colorspace;
  pixel->matte=image->matte;
  pixel->depth=image->depth;
  pixel->fuzz=image->fuzz;
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  G e t N o d e I n f o                                                      %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetNodeInfo() allocates memory for a new node in the color cube tree and
%  presets all fields to zero.
%
%  The format of the GetNodeInfo method is:
%
%      NodeInfo *GetNodeInfo(CubeInfo *cube_info,const unsigned long level)
%
%  A description of each parameter follows.
%
%    o cube_info: A pointer to the CubeInfo structure.
%
%    o level: Specifies the level in the storage_class the node resides.
%
%
*/
static NodeInfo *GetNodeInfo(CubeInfo *cube_info,const unsigned long level)
{
  NodeInfo
    *node_info;

  if (cube_info->free_nodes == 0)
    {
      Nodes
        *nodes;

      /*
        Allocate a new nodes of nodes.
      */
      nodes=(Nodes *) AcquireMagickMemory(sizeof(*nodes));
      if (nodes == (Nodes *) NULL)
        return((NodeInfo *) NULL);
      nodes->next=cube_info->node_queue;
      cube_info->node_queue=nodes;
      cube_info->node_info=nodes->nodes;
      cube_info->free_nodes=NodesInAList;
    }
  cube_info->free_nodes--;
  node_info=cube_info->node_info++;
  (void) ResetMagickMemory(node_info,0,sizeof(*node_info));
  node_info->level=level;
  return(node_info);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  G e t N u m b e r C o l o r s                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  GetNumberColors() returns the number of unique colors in an image.
%
%  The format of the GetNumberColors method is:
%
%      unsigned long GetNumberColors(const Image *image,FILE *file,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: The image.
%
%    o file:  Write a histogram of the color distribution to this file handle.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport unsigned long GetNumberColors(const Image *image,FILE *file,
  ExceptionInfo *exception)
{
  CubeInfo
    *cube_info;

  unsigned long
    number_colors;

  number_colors=0;
  cube_info=ClassifyImageColors(image,exception);
  if (cube_info != (CubeInfo *) NULL)
    {
      if (file != (FILE *) NULL)
        {
          HistogramToFile(image,cube_info,cube_info->root,file,exception);
          (void) fflush(file);
        }
      number_colors=cube_info->colors;
    }
  cube_info=DestroyCubeInfo(cube_info);
  return(number_colors);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+  H i s t o g r a m T o F i l e                                              %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  HistogramToFile() traverses the color cube tree and produces a list of
%  unique pixel field values and the number of times each occurs in the image.
%
%  The format of the Histogram method is:
%
%      void Histogram(const Image *image,CubeInfo *cube_info,
%        const NodeInfo *node_info,FILE *file,ExceptionInfo *exception
%
%  A description of each parameter follows.
%
%    o cube_info: A pointer to the CubeInfo structure.
%
%    o node_info: The address of a structure of type NodeInfo which points to a
%      node in the color cube tree that is to be pruned.
%
%
*/
static void HistogramToFile(const Image *image,CubeInfo *cube_info,
  const NodeInfo *node_info,FILE *file,ExceptionInfo *exception)
{
#define HistogramImageTag  "Histogram/Image"

  register long
    id;

  /*
    Traverse any children.
  */
  for (id=0; id < MaxTreeDepth; id++)
    if (node_info->child[id] != (NodeInfo *) NULL)
      HistogramToFile(image,cube_info,node_info->child[id],file,exception);
  if (node_info->level == (MaxTreeDepth-1))
    {
      char
        name[MaxTextExtent],
        tuple[MaxTextExtent];

      MagickPixelPacket
        pixel;

      register ColorPacket
        *p;

      register long
        i;

      GetMagickPixelPacket(image,&pixel);
      p=node_info->list;
      for (i=0; i < (long) node_info->number_unique; i++)
      {
        pixel.red=(MagickRealType) p->pixel.red;
        pixel.green=(MagickRealType) p->pixel.green;
        pixel.blue=(MagickRealType) p->pixel.blue;
        pixel.opacity=(MagickRealType) p->pixel.opacity;
        pixel.index=(MagickRealType) p->index;
        GetColorTuple(&pixel,MagickFalse,tuple);
        (void) QueryColorname(image,&p->pixel,SVGCompliance,name,exception);
        (void) fprintf(file,MagickSizeFormat ": %s\t%s\n",p->count,tuple,name);
        p++;
      }
      if ((image->progress_monitor != (MagickProgressMonitor) NULL) &&
          (QuantumTick(cube_info->progress,cube_info->colors) != MagickFalse))
        (void) image->progress_monitor(HistogramImageTag,cube_info->progress,
          cube_info->colors,image->client_data);
      cube_info->progress++;
    }
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   I n i t i a l i z e C o l o r L i s t                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  InitializeColorList() initializes the color list.
%
%  The format of the InitializeColorList method is:
%
%      MagickBooleanType InitializeColorList(ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o exception: Return any errors or warnings in this structure.
%
*/
static MagickBooleanType InitializeColorList(ExceptionInfo *exception)
{
  if ((color_list == (LinkedListInfo *) NULL) &&
      (instantiate_color == MagickFalse))
    {
      AcquireSemaphoreInfo(&color_semaphore);
      if ((color_list == (LinkedListInfo *) NULL) &&
          (instantiate_color == MagickFalse))
        {
          (void) LoadColorLists(ColorFilename,exception);
          instantiate_color=MagickTrue;
        }
      RelinquishSemaphoreInfo(color_semaphore);
    }
  return(color_list != (LinkedListInfo *) NULL ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     I s G r a y I m a g e                                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsGrayImage() returns MagickTrue if all the pixels in the image have the same
%  red, green, and blue intensities.
%
%  The format of the IsGrayImage method is:
%
%      MagickBooleanType IsGrayImage(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType IsGrayImage(const Image *image,
  ExceptionInfo *exception)
{
  long
    y;

  register const PixelPacket
    *p;

  register long
    i,
    x;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->colorspace == CMYKColorspace)
    return(MagickFalse);
  switch (image->storage_class)
  {
    case DirectClass:
    case UndefinedClass:
    {
      for (y=0; y < (long) image->rows; y++)
      {
        p=AcquireImagePixels(image,0,y,image->columns,1,exception);
        if (p == (const PixelPacket *) NULL)
          return(MagickFalse);
        for (x=(long) image->columns-1; x >= 0; x--)
        {
          if ((p->red != p->green) || (p->green != p->blue))
            return(MagickFalse);
          p++;
        }
      }
      break;
    }
    case PseudoClass:
    {
      p=image->colormap;
      for (i=0; i < (long) image->colors; i++)
      {
        if ((p->red != p->green) || (p->green != p->blue))
          return(MagickFalse);
        p++;
      }
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
+   I s M a g i c k C o l o r S i m i l a r                                   %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsMagickColorSimilar() returns true if the distance between two colors is
%  less than the specified distance in a linear three dimensional color space.
%  This method is used by ColorFloodFill() and other algorithms which
%  compare two colors.
%
%  The format of the IsMagickColorSimilar method is:
%
%      void IsMagickColorSimilar(const MagickPixelPacket *p,
%        const MagickPixelPacket *q)
%
%  A description of each parameter follows:
%
%    o p: Pixel p.
%
%    o q: Pixel q.
%
%
*/
MagickExport MagickBooleanType IsMagickColorSimilar(const MagickPixelPacket *p,
  const MagickPixelPacket *q)
{
  MagickRealType
    fuzz,
    pixel;

  register MagickRealType
    distance;

  if (p->fuzz == 0.0)
    return(IsMagickColorEqual(p,q));
  fuzz=3.0*p->fuzz*p->fuzz;
  if (p->matte != MagickFalse)
    fuzz+=p->fuzz*p->fuzz;
  if (p->colorspace == CMYKColorspace)
    fuzz+=p->fuzz*p->fuzz;
  pixel=p->red-q->red;
  distance=pixel*pixel;
  if (distance > fuzz)
    return(MagickFalse);
  pixel=p->green-p->green;
  distance+=pixel*pixel;
  if (distance > fuzz)
    return(MagickFalse);
  pixel=p->blue-p->blue;
  distance+=pixel*pixel;
  if (distance > fuzz)
    return(MagickFalse);
  if (p->matte != MagickFalse)
    {
      pixel=p->opacity-p->opacity;
      distance+=pixel*pixel;
      if (distance > fuzz)
        return(MagickFalse);
    }
  if (p->colorspace == CMYKColorspace)
    {
      pixel=p->index-p->index;
      distance+=pixel*pixel;
      if (distance > fuzz)
        return(MagickFalse);
    }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%   I s M o n o c h r o m e I m a g e                                         %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsMonochromeImage() returns MagickTrue if all the pixels in the image have
%  the same red, green, and blue intensities and the intensity is either
%  0 or QuantumRange.
%
%  The format of the IsMonochromeImage method is:
%
%      MagickBooleanType IsMonochromeImage(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType IsMonochromeImage(const Image *image,
  ExceptionInfo *exception)
{
  long
    y;

  register const PixelPacket
    *p;

  register long
    i,
    x;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->colorspace == CMYKColorspace)
    return(MagickFalse);
  switch (image->storage_class)
  {
    case DirectClass:
    case UndefinedClass:
    {
      for (y=0; y < (long) image->rows; y++)
      {
        p=AcquireImagePixels(image,0,y,image->columns,1,exception);
        if (p == (const PixelPacket *) NULL)
          return(MagickFalse);
        for (x=(long) image->columns-1; x >= 0; x--)
        {
          if ((p->red != p->green) || (p->green != p->blue) ||
              ((p->red != 0) && (p->red != QuantumRange)))
            return(MagickFalse);
          p++;
        }
      }
      break;
    }
    case PseudoClass:
    {
      p=image->colormap;
      for (i=0; i < (long) image->colors; i++)
      {
        if ((p->red != p->green) || (p->green != p->blue) ||
            ((p->red != 0) && (p->red != QuantumRange)))
          return(MagickFalse);
        p++;
      }
      break;
    }
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%     I s O p a q u e I m a g e                                               %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsOpaqueImage() returns MagickTrue if none of the pixels in the image have an
%  opacity value other than opaque (0).
%
%  The format of the IsOpaqueImage method is:
%
%      MagickBooleanType IsOpaqueImage(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType IsOpaqueImage(const Image *image,
  ExceptionInfo *exception)
{
  long
    y;

  register const PixelPacket
    *p;

  register long
    x;

  /*
    Determine if image is grayscale.
  */
  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if (image->matte == MagickFalse)
    return(MagickTrue);
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    if (p == (const PixelPacket *) NULL)
      return(MagickFalse);
    for (x=0; x < (long) image->columns; x++)
    {
      if (p->opacity != OpaqueOpacity)
        return(MagickFalse);
      p++;
    }
  }
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  I s P a l e t t e I m a g e                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  IsPaletteImage() returns MagickTrue if the image is PseudoClass and has 256
%  unique colors or less.
%
%  The format of the IsPaletteImage method is:
%
%      MagickBooleanType IsPaletteImage(const Image *image,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: The image.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType IsPaletteImage(const Image *image,
  ExceptionInfo *exception)
{
  CubeInfo
    *cube_info;

  long
    y;

  register const PixelPacket
    *p;

  register long
    x;

  register NodeInfo
    *node_info;

  register long
    i;

  unsigned long
    id,
    index,
    level;

  assert(image != (Image *) NULL);
  assert(image->signature == MagickSignature);
  if (image->debug != MagickFalse)
    (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",image->filename);
  if ((image->storage_class == PseudoClass) && (image->colors <= 256))
    return(MagickTrue);
  if (image->storage_class == PseudoClass)
    return(MagickFalse);
  /*
    Initialize color description tree.
  */
  cube_info=GetCubeInfo();
  if (cube_info == (CubeInfo *) NULL)
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        ResourceLimitError,"MemoryAllocationFailed","`%s'",image->filename);
      return(MagickFalse);
    }
  for (y=0; y < (long) image->rows; y++)
  {
    p=AcquireImagePixels(image,0,y,image->columns,1,exception);
    if (p == (const PixelPacket *) NULL)
      break;
    for (x=0; x < (long) image->columns; x++)
    {
      /*
        Start at the root and proceed level by level.
      */
      node_info=cube_info->root;
      index=MaxTreeDepth-1;
      for (level=1; level < MaxTreeDepth; level++)
      {
        id=ColorToNodeId(p,index,MagickFalse);
        if (node_info->child[id] == (NodeInfo *) NULL)
          {
            node_info->child[id]=GetNodeInfo(cube_info,level);
            if (node_info->child[id] == (NodeInfo *) NULL)
              {
                (void) ThrowMagickException(exception,GetMagickModule(),
                  ResourceLimitError,"MemoryAllocationFailed","`%s'",
                  image->filename);
                break;
              }
          }
        node_info=node_info->child[id];
        index--;
      }
      if (level < MaxTreeDepth)
        break;
      for (i=0; i < (long) node_info->number_unique; i++)
        if (ColorMatch(p,&node_info->list[i].pixel) != MagickFalse)
          break;
      if (i == (long) node_info->number_unique)
        {
          /*
            Add this unique color to the color list.
          */
          if (node_info->number_unique == 0)
            node_info->list=(ColorPacket *)
              AcquireMagickMemory(sizeof(*node_info->list));
          else
            node_info->list=(ColorPacket *) ResizeMagickMemory(node_info->list,
              (size_t) (i+1)*sizeof(*node_info->list));
          if (node_info->list == (ColorPacket *) NULL)
            {
              (void) ThrowMagickException(exception,GetMagickModule(),
                ResourceLimitError,"MemoryAllocationFailed","`%s'",
                image->filename);
              break;
            }
          node_info->list[i].pixel=(*p);
          node_info->list[i].index=(IndexPacket) cube_info->colors++;
          node_info->number_unique++;
          if (cube_info->colors > 256)
            break;
        }
      p++;
    }
    if (x < (long) image->columns)
      break;
  }
  cube_info=DestroyCubeInfo(cube_info);
  return(y < (long) image->rows ? MagickFalse : MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L i s t C o l o r I n f o                                                  %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ListColorInfo() lists color names to the specified file.  Color names
%  are a convenience.  Rather than defining a color by its red, green, and
%  blue intensities just use a color name such as white, blue, or yellow.
%
%  The format of the ListColorInfo method is:
%
%      MagickBooleanType ListColorInfo(FILE *file,ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o file:  List color names to this file handle.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType ListColorInfo(FILE *file,
  ExceptionInfo *exception)
{
  const char
    *path;

  const ColorInfo
    **color_info;

  long
    j;

  register long
    i;

  unsigned long
    number_colors;

  /*
    List name and attributes of each color in the list.
  */
  if (file == (const FILE *) NULL)
    file=stdout;
  color_info=GetColorInfoList("*",&number_colors,exception);
  if (color_info == (const ColorInfo **) NULL)
    return(MagickFalse);
  path=(const char *) NULL;
  for (i=0; i < (long) number_colors; i++)
  {
    if (color_info[i]->stealth != MagickFalse)
      continue;
    if ((path == (const char *) NULL) ||
        (LocaleCompare(path,color_info[i]->path) != 0))
      {
        if (color_info[i]->path != (char *) NULL)
          (void) fprintf(file,"\nPath: %s\n\n",color_info[i]->path);
        (void) fprintf(file,
          "Name                   Color                          Compliance\n");
        (void) fprintf(file,"-------------------------------------------------"
          "------------------------------\n");
      }
    path=color_info[i]->path;
    (void) fprintf(file,"%s",color_info[i]->name);
    for (j=(long) strlen(color_info[i]->name); j <= 22; j++)
      (void) fprintf(file," ");
    if (color_info[i]->color.opacity == OpaqueOpacity)
      (void) fprintf(file,"rgb(%5u,%5u,%5u)          ",(unsigned int)
        color_info[i]->color.red,(unsigned int) color_info[i]->color.green,
        (unsigned int) color_info[i]->color.blue);
    else
      (void) fprintf(file,"rgba(%5u,%5u,%5u,%5u)  ",(unsigned int)
        color_info[i]->color.red,(unsigned int) color_info[i]->color.green,
        (unsigned int) color_info[i]->color.blue,(unsigned int)
        color_info[i]->color.opacity);
    if ((color_info[i]->compliance & SVGCompliance) != 0)
      (void) fprintf(file,"SVG ");
    if ((color_info[i]->compliance & X11Compliance) != 0)
      (void) fprintf(file,"X11 ");
    if ((color_info[i]->compliance & XPMCompliance) != 0)
      (void) fprintf(file,"XPM ");
    (void) fprintf(file,"\n");
  }
  color_info=(const ColorInfo **) RelinquishMagickMemory((void *) color_info);
  (void) fflush(file);
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
+   L o a d C o l o r L i s t                                                 %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LoadColorList() loads the color configuration file which provides a mapping
%  between color attributes and a color name.
%
%  The format of the LoadColorList method is:
%
%      MagickBooleanType LoadColorList(const char *xml,const char *filename,
%        const unsigned long depth,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o xml:  The color list in XML format.
%
%    o filename:  The color list filename.
%
%    o depth: depth of <include /> statements.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
static MagickBooleanType LoadColorList(const char *xml,const char *filename,
  const unsigned long depth,ExceptionInfo *exception)
{
  char
    keyword[MaxTextExtent],
    *q,
    *token;

  ColorInfo
    *color_info = (ColorInfo *) NULL;

  MagickStatusType
    status;

  /*
    Load the color map file.
  */
  (void) LogMagickEvent(ConfigureEvent,GetMagickModule(),
    "Loading color file \"%s\" ...",filename);
  if (xml == (char *) NULL)
    return(MagickFalse);
  if (color_list == (LinkedListInfo *) NULL)
    {
      color_list=NewLinkedList(0);
      if (color_list == (LinkedListInfo *) NULL)
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
                  status|=LoadColorList(xml,path,depth+1,exception);
                  xml=(char *) RelinquishMagickMemory(xml);
                }
            }
        }
        continue;
      }
    if (LocaleCompare(keyword,"<color") == 0)
      {
        /*
          Allocate memory for the color list.
        */
        color_info=(ColorInfo *) AcquireMagickMemory(sizeof(*color_info));
        if (color_info == (ColorInfo *) NULL)
          ThrowMagickFatalException(ResourceLimitFatalError,
            "MemoryAllocationFailed",filename);
        (void) ResetMagickMemory(color_info,0,sizeof(*color_info));
        color_info->path=ConstantString(AcquireString(filename));
        color_info->signature=MagickSignature;
        continue;
      }
    if (color_info == (ColorInfo *) NULL)
      continue;
    if (LocaleCompare(keyword,"/>") == 0)
      {
        status=AppendValueToLinkedList(color_list,color_info);
        if (status == MagickFalse)
          (void) ThrowMagickException(exception,GetMagickModule(),
            ResourceLimitError,"MemoryAllocationFailed","`%s'",
            color_info->name);
        color_info=(ColorInfo *) NULL;
      }
    GetMagickToken(q,(char **) NULL,token);
    if (*token != '=')
      continue;
    GetMagickToken(q,&q,token);
    GetMagickToken(q,&q,token);
    switch (*keyword)
    {
      case 'B':
      case 'b':
      {
        if (LocaleCompare((char *) keyword,"blue") == 0)
          {
            color_info->color.blue=ScaleCharToQuantum(atol(token));
            break;
          }
        break;
      }
      case 'C':
      case 'c':
      {
        if (LocaleCompare((char *) keyword,"compliance") == 0)
          {
            long
              compliance;

            compliance=color_info->compliance;
            if (GlobExpression(token,"*[Ss][Vv][Gg]*") != MagickFalse)
              compliance|=SVGCompliance;
            if (GlobExpression(token,"*[Xx]11*") != MagickFalse)
              compliance|=X11Compliance;
            if (GlobExpression(token,"*[Xx][Pp][Mm]*") != MagickFalse)
              compliance|=XPMCompliance;
            color_info->compliance=(ComplianceType) compliance;
            break;
          }
        break;
      }
      case 'G':
      case 'g':
      {
        if (LocaleCompare((char *) keyword,"green") == 0)
          {
            color_info->color.green=ScaleCharToQuantum(atol(token));
            break;
          }
        break;
      }
      case 'N':
      case 'n':
      {
        if (LocaleCompare((char *) keyword,"name") == 0)
          {
            color_info->name=ConstantString(AcquireString(token));
            break;
          }
        break;
      }
      case 'O':
      case 'o':
      {
        if (LocaleCompare((char *) keyword,"opacity") == 0)
          {
            color_info->color.opacity=ScaleCharToQuantum(atol(token));
            break;
          }
        break;
      }
      case 'R':
      case 'r':
      {
        if (LocaleCompare((char *) keyword,"red") == 0)
          {
            color_info->color.red=ScaleCharToQuantum(atol(token));
            break;
          }
        break;
      }
      case 'S':
      case 's':
      {
        if (LocaleCompare((char *) keyword,"stealth") == 0)
          {
            color_info->stealth=(MagickBooleanType)
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
  if (color_list == (LinkedListInfo *) NULL)
    return(MagickFalse);
  return(status != 0 ? MagickTrue : MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  L o a d C o l o r L i s t s                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  LoadColorList() loads one or more color configuration file which provides a
%  mapping between color attributes and a color name.
%
%  The format of the LoadColorLists method is:
%
%      MagickBooleanType LoadColorLists(const char *filename,
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
static MagickBooleanType LoadColorLists(const char *filename,
  ExceptionInfo *exception)
{
#if defined(UseEmbeddableMagick)
  return(LoadColorList(ColorMap,"built-in",0,exception));
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
    status|=LoadColorList((const char *) option->datum,option->path,0,
      exception);
    option=(const StringInfo *) GetNextValueInLinkedList(options);
  }
  options=DestroyConfigureOptions(options);
  if ((color_list == (LinkedListInfo *) NULL) ||
      (IsLinkedListEmpty(color_list) != MagickFalse))
    status|=LoadColorList(ColorMap,"built-in",0,exception);
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
%   Q u e r y C o l o r D a t a b a s e                                       %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  QueryColorDatabase() returns the red, green, blue, and opacity intensities
%  for a given color name.
%
%  The format of the QueryColorDatabase method is:
%
%      MagickBooleanType QueryColorDatabase(const char *name,PixelPacket *color,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o name: The color name (e.g. white, blue, yellow).
%
%    o color: The red, green, blue, and opacity intensities values of the
%      named color in this structure.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType QueryColorDatabase(const char *name,
  PixelPacket *color,ExceptionInfo *exception)
{
  GeometryInfo
    geometry_info;

  MagickPixelPacket
    pixel;

  MagickRealType
    scale;

  MagickStatusType
    flags;

  register const ColorInfo
    *p;

  register long
    i;

  /*
    Initialize color return value.
  */
  assert(name != (const char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",name);
  assert(color != (PixelPacket *) NULL);
  (void) ResetMagickMemory(color,0,sizeof(*color));
  color->opacity=OpaqueOpacity;
  if ((name == (char *) NULL) || (*name == '\0'))
    name=BackgroundColor;
  while (isspace((int) ((unsigned char) *name)) != 0)
    name++;
  if (*name == '#')
    {
      char
        c;

      LongPixelPacket
        pixel;

      unsigned long
        n,
        scale;

      (void) ResetMagickMemory(&pixel,0,sizeof(pixel));
      name++;
      for (n=0; isxdigit((int) ((unsigned char) name[n])) != MagickFalse; n++);
      if ((n == 3) || (n == 6) || (n == 9) || (n == 12) || (n == 24))
        {
          /*
            Parse RGB specification.
          */
          n/=3;
          do
          {
            pixel.red=pixel.green;
            pixel.green=pixel.blue;
            pixel.blue=0;
            for (i=(long) n-1; i >= 0; i--)
            {
              c=(*name++);
              pixel.blue<<=4;
              if ((c >= '0') && (c <= '9'))
                pixel.blue|=(int) (c-'0');
              else
                if ((c >= 'A') && (c <= 'F'))
                  pixel.blue|=(int) c-((int) 'A'-10);
                else
                  if ((c >= 'a') && (c <= 'f'))
                    pixel.blue|=(int) c-((int) 'a'-10);
                  else
                    return(MagickFalse);
            }
          } while (isxdigit((int) ((unsigned char) *name)) != 0);
        }
      else
        if ((n != 4) && (n != 8) && (n != 16) && (n != 32))
          {
            (void) ThrowMagickException(exception,GetMagickModule(),
              OptionWarning,"UnrecognizedColor","`%s'",name);
            return(MagickFalse);
          }
        else
          {
            /*
              Parse RGBA specification.
            */
            n/=4;
            do
            {
              pixel.red=pixel.green;
              pixel.green=pixel.blue;
              pixel.blue=pixel.opacity;
              pixel.opacity=0;
              for (i=(long) n-1; i >= 0; i--)
              {
                c=(*name++);
                pixel.opacity<<=4;
                if ((c >= '0') && (c <= '9'))
                  pixel.opacity|=(int) (c-'0');
                else
                  if ((c >= 'A') && (c <= 'F'))
                    pixel.opacity|=(int) c-((int) 'A'-10);
                  else
                    if ((c >= 'a') && (c <= 'f'))
                      pixel.opacity|=(int) c-((int) 'a'-10);
                    else
                      return(MagickFalse);
              }
            } while (isxdigit((int) ((unsigned char) *name)) != MagickFalse);
          }
      n<<=2;
      scale=(1UL << n)-1;
      if (n == 32)
        scale=4294967295UL;
      color->red=ScaleAnyToQuantum(pixel.red,scale);
      color->green=ScaleAnyToQuantum(pixel.green,scale);
      color->blue=ScaleAnyToQuantum(pixel.blue,scale);
      color->opacity=OpaqueOpacity;
      if ((n != 3) && (n != 6) && (n != 9) && (n != 12) && (n != 24))
        color->opacity=ScaleAnyToQuantum(pixel.opacity,scale);
      return(MagickTrue);
    }
  if (LocaleNCompare(name,"rgb(",4) == 0)
    {
      flags=ParseGeometry(name+3,&geometry_info);
      pixel.red=geometry_info.rho;
      pixel.green=geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        pixel.green=pixel.red;
      pixel.blue=geometry_info.xi;
      if ((flags & XiValue) == 0)
        pixel.blue=pixel.red;
      scale=(MagickRealType) ((flags & PercentValue) != 0 ?
        (MagickRealType) ScaleQuantumToChar(QuantumRange)/100 : 1.0);
      color->red=ScaleCharToQuantum(scale*pixel.red);
      color->green=ScaleCharToQuantum(scale*pixel.green);
      color->blue=ScaleCharToQuantum(scale*pixel.blue);
      color->opacity=OpaqueOpacity;
      return(MagickTrue);
    }
  if (LocaleNCompare(name,"rgba(",5) == 0)
    {
      flags=ParseGeometry(name+4,&geometry_info);
      pixel.red=geometry_info.rho;
      pixel.green=geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        pixel.green=pixel.red;
      pixel.blue=geometry_info.xi;
      if ((flags & XiValue) == 0)
        pixel.blue=pixel.red;
      pixel.opacity=geometry_info.psi;
      if ((flags & XiValue) == 0)
        pixel.opacity=(MagickRealType) OpaqueOpacity;
      scale=(MagickRealType) ((flags & PercentValue) != 0 ?
        (MagickRealType) ScaleQuantumToChar(QuantumRange)/100 : 1.0);
      color->red=ScaleCharToQuantum(scale*pixel.red);
      color->green=ScaleCharToQuantum(scale*pixel.green);
      color->blue=ScaleCharToQuantum(scale*pixel.blue);
      color->opacity=ScaleCharToQuantum(scale*pixel.opacity);
      return(MagickTrue);
    }
  p=GetColorInfo(name,exception);
  if (p == (const ColorInfo *) NULL)
    return(MagickFalse);
  *color=p->color;
  return(MagickTrue);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%  Q u e r y C o l o r n a m e                                                %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  QueryColorname() returns a named color for the given color intensity.  If
%  an exact match is not found, a hex value is return instead.  For example
%  an intensity of rgb:(0,0,0) returns black whereas rgb:(223,223,223)
%  returns #dfdfdf.
%
%  The format of the QueryColorname method is:
%
%      MagickBooleanType QueryColorname(const Image *image,
%        const PixelPacket *color,const ComplianceType compliance,char *name,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows.
%
%    o image: The image.
%
%    o color: The color intensities.
%
%    o Compliance: Adhere to this color standard: SVG, X11, or XPM.
%
%    o name: Return the color name or hex value.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType QueryColorname(const Image *image,
  const PixelPacket *color,const ComplianceType compliance,char *name,
  ExceptionInfo *exception)
{
  Quantum
    opacity;

  register const ColorInfo
    *p;

  MagickPixelPacket
    pixel;

  *name='\0';
  (void) GetColorInfo("*",exception);
  ResetLinkedListIterator(color_list);
  opacity=image->matte != MagickFalse ? color->opacity : OpaqueOpacity;
  p=(const ColorInfo *) GetNextValueInLinkedList(color_list);
  while (p != (const ColorInfo *) NULL)
  {
    if (((p->compliance & compliance) != 0) &&
        ((p->color.red == color->red) && (p->color.green == color->green) &&
         (p->color.blue == color->blue) && (p->color.opacity == opacity)))
      {
        (void) CopyMagickString(name,p->name,MaxTextExtent);
        return(MagickTrue);
      }
    p=(const ColorInfo *) GetNextValueInLinkedList(color_list);
  }
  GetMagickPixelPacket(image,&pixel);
  pixel.colorspace=RGBColorspace;
  pixel.matte=compliance != XPMCompliance ? image->matte : MagickFalse;
  pixel.depth=compliance != XPMCompliance ? image->depth : Min(image->depth,16);
  SetMagickPixelPacket(color,(IndexPacket *) NULL,&pixel);
  GetColorTuple(&pixel,MagickTrue,name);
  return(MagickFalse);
}

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   Q u e r y M a g i c k C o l o r                                           %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  QueryMagickColor() returns the red, green, blue, and opacity intensities
%  for a given color name.
%
%  The format of the QueryMagickColor method is:
%
%      MagickBooleanType QueryMagickColor(const char *name,
%        MagickPixelPacket *color,ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o name: The color name (e.g. white, blue, yellow).
%
%    o color: The red, green, blue, and opacity intensities values of the
%      named color in this structure.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/
MagickExport MagickBooleanType QueryMagickColor(const char *name,
  MagickPixelPacket *color,ExceptionInfo *exception)
{
  GeometryInfo
    geometry_info;

  MagickPixelPacket
    pixel;

  MagickRealType
    scale;

  MagickStatusType
    flags;

  register const ColorInfo
    *p;

  register long
    i;

  /*
    Initialize color return value.
  */
  assert(name != (const char *) NULL);
  (void) LogMagickEvent(TraceEvent,GetMagickModule(),"%s",name);
  assert(color != (MagickPixelPacket *) NULL);
  if ((name == (char *) NULL) || (*name == '\0'))
    name=BackgroundColor;
  while (isspace((int) ((unsigned char) *name)) != 0)
    name++;
  if (*name == '#')
    {
      char
        c;

      LongPixelPacket
        pixel;

      unsigned long
        n,
        scale;

      (void) ResetMagickMemory(&pixel,0,sizeof(pixel));
      name++;
      for (n=0; isxdigit((int) ((unsigned char) name[n])) != MagickFalse; n++);
      if ((n == 3) || (n == 6) || (n == 9) || (n == 12) || (n == 24))
        {
          /*
            Parse RGB specification.
          */
          n/=3;
          do
          {
            pixel.red=pixel.green;
            pixel.green=pixel.blue;
            pixel.blue=0;
            for (i=(long) n-1; i >= 0; i--)
            {
              c=(*name++);
              pixel.blue<<=4;
              if ((c >= '0') && (c <= '9'))
                pixel.blue|=(int) (c-'0');
              else
                if ((c >= 'A') && (c <= 'F'))
                  pixel.blue|=(int) c-((int) 'A'-10);
                else
                  if ((c >= 'a') && (c <= 'f'))
                    pixel.blue|=(int) c-((int) 'a'-10);
                  else
                    return(MagickFalse);
            }
          } while (isxdigit((int) ((unsigned char) *name)) != MagickFalse);
        }
      else
        if ((n != 4) && (n != 8) && (n != 16) && (n != 32))
          {
            (void) ThrowMagickException(exception,GetMagickModule(),
              OptionWarning,"UnrecognizedColor","`%s'",name);
            return(MagickFalse);
          }
        else
          {
            /*
              Parse RGBA specification.
            */
            n/=4;
            do
            {
              pixel.red=pixel.green;
              pixel.green=pixel.blue;
              pixel.blue=pixel.opacity;
              pixel.opacity=0;
              for (i=(long) n-1; i >= 0; i--)
              {
                c=(*name++);
                pixel.opacity<<=4;
                if ((c >= '0') && (c <= '9'))
                  pixel.opacity|=(int) (c-'0');
                else
                  if ((c >= 'A') && (c <= 'F'))
                    pixel.opacity|=(int) c-((int) 'A'-10);
                  else
                    if ((c >= 'a') && (c <= 'f'))
                      pixel.opacity|=(int) c-((int) 'a'-10);
                    else
                      return(MagickFalse);
              }
            } while (isxdigit((int) ((unsigned char) *name)) != MagickFalse);
          }
      n<<=2;
      scale=(1UL << n)-1;
      if (n == 32)
        scale=4294967295UL;
      color->colorspace=RGBColorspace;
      color->matte=MagickFalse;
      color->red=(MagickRealType) ScaleAnyToQuantum(pixel.red,scale);
      color->green=(MagickRealType) ScaleAnyToQuantum(pixel.green,scale);
      color->blue=(MagickRealType) ScaleAnyToQuantum(pixel.blue,scale);
      color->opacity=(MagickRealType) OpaqueOpacity;
      if ((n != 3) && (n != 6) && (n != 9) && (n != 12) && (n != 24))
        {
          color->matte=MagickTrue;
          color->opacity=(MagickRealType)
            ScaleAnyToQuantum(pixel.opacity,scale);
        }
      color->index=0.0;
      return(MagickTrue);
    }
  if (LocaleNCompare(name,"cmyk(",5) == 0)
    {
      flags=ParseGeometry(name+4,&geometry_info);
      pixel.red=geometry_info.rho;
      pixel.green=geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        pixel.green=pixel.red;
      pixel.blue=geometry_info.xi;
      if ((flags & XiValue) == 0)
        pixel.blue=pixel.red;
      pixel.index=geometry_info.psi;
      if ((flags & PsiValue) == 0)
        pixel.index=0.0;
      scale=(MagickRealType) ((flags & PercentValue) != 0 ?
        (MagickRealType) ScaleQuantumToChar(QuantumRange)/100 : 1.0);
      color->colorspace=CMYKColorspace;
      color->matte=MagickFalse;
      color->red=(MagickRealType) ScaleCharToQuantum(scale*pixel.red);
      color->green=(MagickRealType) ScaleCharToQuantum(scale*pixel.green);
      color->blue=(MagickRealType) ScaleCharToQuantum(scale*pixel.blue);
      color->opacity=(MagickRealType) OpaqueOpacity;
      color->index=(MagickRealType) ScaleCharToQuantum(scale*pixel.index);
      return(MagickTrue);
    }
  if (LocaleNCompare(name,"cmyka(",6) == 0)
    {
      flags=ParseGeometry(name+4,&geometry_info);
      pixel.red=geometry_info.rho;
      pixel.green=geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        pixel.green=pixel.red;
      pixel.blue=geometry_info.xi;
      if ((flags & XiValue) == 0)
        pixel.blue=pixel.red;
      pixel.index=geometry_info.psi;
      if ((flags & PsiValue) == 0)
        pixel.index=0.0;
      pixel.opacity=geometry_info.chi;
      if ((flags & ChiValue) == 0)
        pixel.opacity=0.0;
      scale=(MagickRealType) ((flags & PercentValue) != 0 ?
        (MagickRealType) ScaleQuantumToChar(QuantumRange)/100 : 1.0);
      color->colorspace=CMYKColorspace;
      color->matte=MagickTrue;
      color->red=(MagickRealType) ScaleCharToQuantum(scale*pixel.red);
      color->green=(MagickRealType) ScaleCharToQuantum(scale*pixel.green);
      color->blue=(MagickRealType) ScaleCharToQuantum(scale*pixel.blue);
      color->opacity=(MagickRealType) ScaleCharToQuantum(scale*pixel.opacity);
      color->index=(MagickRealType) ScaleCharToQuantum(scale*pixel.index);
      return(MagickTrue);
    }
  if (LocaleNCompare(name,"rgb(",4) == 0)
    {
      flags=ParseGeometry(name+3,&geometry_info);
      pixel.red=geometry_info.rho;
      pixel.green=geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        pixel.green=pixel.red;
      pixel.blue=geometry_info.xi;
      if ((flags & XiValue) == 0)
        pixel.blue=pixel.red;
      scale=(MagickRealType) ((flags & PercentValue) != 0 ?
        (MagickRealType) ScaleQuantumToChar(QuantumRange)/100 : 1.0);
      color->colorspace=RGBColorspace;
      color->matte=MagickFalse;
      color->red=(MagickRealType) ScaleCharToQuantum(scale*pixel.red);
      color->green=(MagickRealType) ScaleCharToQuantum(scale*pixel.green);
      color->blue=(MagickRealType) ScaleCharToQuantum(scale*pixel.blue);
      color->opacity=(MagickRealType) OpaqueOpacity;
      color->index=0.0;
      return(MagickTrue);
    }
  if (LocaleNCompare(name,"rgba(",5) == 0)
    {
      flags=ParseGeometry(name+4,&geometry_info);
      pixel.red=geometry_info.rho;
      pixel.green=geometry_info.sigma;
      if ((flags & SigmaValue) == 0)
        pixel.green=pixel.red;
      pixel.blue=geometry_info.xi;
      if ((flags & XiValue) == 0)
        pixel.blue=pixel.red;
      pixel.opacity=geometry_info.psi;
      if ((flags & PsiValue) == 0)
        pixel.opacity=(MagickRealType) OpaqueOpacity;
      scale=(MagickRealType) ((flags & PercentValue) != 0 ?
        (MagickRealType) ScaleQuantumToChar(QuantumRange)/100 : 1.0);
      color->colorspace=RGBColorspace;
      color->matte=MagickTrue;
      color->red=(MagickRealType) ScaleCharToQuantum(scale*pixel.red);
      color->green=(MagickRealType) ScaleCharToQuantum(scale*pixel.green);
      color->blue=(MagickRealType) ScaleCharToQuantum(scale*pixel.blue);
      color->opacity=(MagickRealType) ScaleCharToQuantum(scale*pixel.opacity);
      color->index=0.0;
      return(MagickTrue);
    }
  p=GetColorInfo(name,exception);
  if (p == (const ColorInfo *) NULL)
    return(MagickFalse);
  color->colorspace=RGBColorspace;
  color->matte=p->color.opacity != OpaqueOpacity ? MagickTrue : MagickFalse;
  color->red=(MagickRealType) p->color.red;
  color->green=(MagickRealType) p->color.green;
  color->blue=(MagickRealType) p->color.blue;
  color->opacity=(MagickRealType) p->color.opacity;
  color->index=0.0;
  return(MagickTrue);
}
