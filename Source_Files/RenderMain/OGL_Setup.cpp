/*

	Copyright (C) 1991-2001 and beyond by Bungie Studios, Inc.
	and the "Aleph One" developers.
 
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	This license is contained in the file "COPYING",
	which is included with this source code; it is available online at
	http://www.gnu.org/licenses/gpl.html
	
	OpenGL Renderer,
	by Loren Petrich,
	March 12, 2000

	This contains implementations of functions intended for finding out OpenGL's presence
	in the host system, for setting parameters for OpenGL rendering,
	and for deciding whether to use OpenGL for rendering.
	
	June 11, 2000
	
	Had added XML parsing before that; most recently, added "opac_shift".
	
	Made semitransparency optional if the void is on one side of the texture

Oct 13, 2000 (Loren Petrich)
	Converted the OpenGL-addition accounting into Standard Template Library vectors

Nov 12, 2000 (Loren Petrich):
	Implemented texture substitution, also moved pixel-opacity editing into here;
	the code is carefully constructed to assume RGBA byte order whether integers are
	big- or little-endian.

Nov 18, 2000 (Loren Petrich):
	Added support for glow mapping; constrained it to only be present
	when a normal texture is present, and to have the same size

Nov 26, 2000 (Loren Petrich):
	Added system for reloading textures only when their filenames change.

Dec 17, 2000 (Loren Petrich):
	Eliminated fog parameters from the preferences;
	there is still a "fog present" switch, which is used to indicate
	whether fog will not be suppressed.

Apr 27, 2001 (Loren Petrich):
	Modified the OpenGL fog support so as to enable below-liquid fogs

Jul 8, 2001 (Loren Petrich):
	Made it possible to read in silhouette bitmaps; one can now use the silhouette index
	as a MML color-table index

Aug 21, 2001 (Loren Petrich):
	Adding support for 3D-model inhabitant objects

Jan 25, 2002 (Br'fin (Jeremy Parsons)):
	Added TARGET_API_MAC_CARBON for OpenGL.h, AGL.h
	Removed QuickDraw3D support from Carbon

Feb 5, 2002 (Br'fin (Jeremy Parsons)):
	Refined OGL default preferences for Carbon
*/

#include <vector>
#include <string.h>
#include <math.h>
#include "cseries.h"

#ifdef HAVE_OPENGL

#ifdef __MVCPP__
#include <windows.h>
#endif

#if defined(mac)
# if defined(EXPLICIT_CARBON_HEADER)
#  include <OpenGL/gl.h>
#  include <AGL/agl.h>
# else
#  include <agl.h>
# endif
#elif defined(SDL)
# if defined (__APPLE__) && defined (__MACH__)
#  include <OpenGL/gl.h>
# else
#  include <GL/gl.h>
# endif
#endif

#endif

#include "shape_descriptors.h"
#include "OGL_Setup.h"
#include "ColorParser.h"

#ifdef __WIN32__
#include "OGL_Win32.h"
#endif

// Whether or not OpenGL is present and usable
static bool _OGL_IsPresent = false;


// Initializer
bool OGL_Initialize()
{
#ifdef HAVE_OPENGL
#if defined(mac)
	// Cribbed from Apple's DrawSprocket documentation;
	// look for OpenGL function
	return (_OGL_IsPresent = ((Ptr)aglChoosePixelFormat != (Ptr)kUnresolvedCFragSymbolAddress));
	// return (_OGL_IsPresent = ((Ptr)glBegin != (Ptr)kUnresolvedCFragSymbolAddress));
#elif defined(SDL)
	// nothing to do
#if defined(__WIN32__)
	setup_gl_extensions();
#endif	
	return _OGL_IsPresent = true;
#else
#error OGL_Initialize() not implemented for this platform
#endif
#else
	return false;
#endif
}

// Test for presence
bool OGL_IsPresent() {return _OGL_IsPresent;}


// Sensible defaults for the fog:
static OGL_FogData FogData[OGL_NUMBER_OF_FOG_TYPES] = 
{
	{{0x8000,0x8000,0x8000},8,false,true},
	{{0x8000,0x8000,0x8000},8,false,true}
};


// For flat landscapes:
const RGBColor DefaultLscpColors[4][2] =
{
	{
		{0xffff, 0xffff, 0x6666},		// Day
		{0x3333, 0x9999, 0xffff},
	},
	{
		{0x1818, 0x1818, 0x1010},		// Night
		{0x0808, 0x0808, 0x1010},
	},
	{
		{0x6666, 0x6666, 0x6666},		// Moon
		{0x0000, 0x0000, 0x0000},
	},
	{
		{0x0000, 0x0000, 0x0000},		// Outer Space
		{0x0000, 0x0000, 0x0000},
	},
};


// Set defaults
void OGL_SetDefaults(OGL_ConfigureData& Data)
{
	for (int k=0; k<OGL_NUMBER_OF_TEXTURE_TYPES; k++)
	{
		OGL_Texture_Configure& TxtrData = Data.TxtrConfigList[k];
		TxtrData.NearFilter = 1;		// GL_LINEAR
		if (k == OGL_Txtr_Wall)
			TxtrData.FarFilter = 5;		// GL_LINEAR_MIPMAP_LINEAR
		else
			TxtrData.FarFilter = 1;		// GL_LINEAR
		TxtrData.Resolution = 0;		// 1x
		TxtrData.ColorFormat = 0;		// 32-bit color
	}
	
#ifdef SDL
	// Reasonable default flags ("static" effect causes massive slowdown, so we turn it off)
	Data.Flags = OGL_Flag_FlatStatic | OGL_Flag_Fader | OGL_Flag_Map |
		OGL_Flag_HUD | OGL_Flag_LiqSeeThru | OGL_Flag_3D_Models;
#elif TARGET_API_MAC_CARBON
	// Reasonable default OS X flags
	Data.Flags = OGL_Flag_Map | OGL_Flag_LiqSeeThru | OGL_Flag_3D_Models |
		OGL_Flag_2DGraphics | OGL_Flag_Fader | OGL_Flag_HUD;
#else
	// Reasonable default flags
	Data.Flags = OGL_Flag_Map | OGL_Flag_LiqSeeThru | OGL_Flag_3D_Models |
		OGL_Flag_2DGraphics;
#endif

        Data.AnisotropyLevel = 0.0; // off
	Data.Multisamples = 0; // off
	
	Data.VoidColor = rgb_black;			// Self-explanatory
	for (int il=0; il<4; il++)
		for (int ie=0; ie<2; ie++)
			Data.LscpColors[il][ie] = DefaultLscpColors[il][ie];
}


inline bool StringPresent(vector<char>& String)
{
	return (String.size() > 1);
}


void OGL_TextureOptionsBase::Load()
{
	FileSpecifier File;
	
	// Load the normal image with alpha channel
	try
	{
		// Check to see if loading needs to be done;
		// it does not need to be if an image is present.
		if (NormalImg.IsPresent()) throw 0;
		NormalImg.Clear();
		
		// Load the normal image if it has a filename specified for it
		if (!StringPresent(NormalColors)) throw 0;
		if (!File.SetNameWithPath(&NormalColors[0])) throw 0;
		if (!LoadImageFromFile(NormalImg,File,ImageLoader_Colors)) throw 0;
	}
	catch(...)
	{
		// A texture must have a normal colored part
		return;
	}
	
	try
	{
		// Load the normal mask if it has a filename specified for it
		if (!StringPresent(NormalMask)) throw 0;
		if (!File.SetNameWithPath(&NormalMask[0])) throw 0;
		if (!LoadImageFromFile(NormalImg,File,ImageLoader_Opacity)) throw 0;
	}
	catch(...)
	{}
	
	// Load the glow image with alpha channel
	try
	{
		// Check to see if loading needs to be done;
		// it does not need to be if an image is present.
		if (GlowImg.IsPresent()) throw 0;
		GlowImg.Clear();
		
		// Load the glow image if it has a filename specified for it
		if (!StringPresent(GlowColors)) throw 0;
		if (!File.SetNameWithPath(&GlowColors[0])) throw 0;
		if (!LoadImageFromFile(GlowImg,File,ImageLoader_Colors)) throw 0;
		
		// Load the glow mask if it has a filename specified for it;
		// only loaded if an image has been loaded for it
		if (!StringPresent(GlowMask)) throw 0;
		if (!File.SetNameWithPath(&GlowMask[0])) throw 0;
		if (!LoadImageFromFile(GlowImg,File,ImageLoader_Opacity)) throw 0;
	}
	catch(...)
	{}
	
	// The rest of the code is made simpler by these constraints:
	// that the glow texture only be present if the normal texture is also present,
	// and that the normal and glow textures have the same dimensions
	if (NormalImg.IsPresent())
	{
		int W0 = NormalImg.GetWidth();
		int W1 = GlowImg.GetWidth();
		int H0 = NormalImg.GetHeight();
		int H1 = GlowImg.GetHeight();
		if ((W1 != W0) || (H1 != H0)) GlowImg.Clear();
	}
	else
	{
		GlowImg.Clear();
	}
}

void OGL_TextureOptionsBase::Unload()
{
	NormalImg.Clear();
	GlowImg.Clear();
}


// Does this for a set of several pixel values or color-table values;
// the pixels are assumed to be in OpenGL-friendly byte-by-byte RGBA format.
void SetPixelOpacities(OGL_TextureOptions& Options, int NumPixels, uint32 *Pixels)
{
	for (int k=0; k<NumPixels; k++)
	{
		uint8 *PxlPtr = (uint8 *)(Pixels + k);
		
		// This won't be scaled to (0,1), but will be left at (0,255) here
		float Opacity;
		switch(Options.OpacityType)
		{
		// Two versions of the Tomb Raider texture-opacity hack
		case OGL_OpacType_Avg:
			{
				uint32 Red = uint32(PxlPtr[0]);
				uint32 Green = uint32(PxlPtr[1]);
				uint32 Blue = uint32(PxlPtr[2]);
				Opacity = (Red + Green + Blue)/3.0F;
			}
			break;
			
		case OGL_OpacType_Max:
			{
				uint32 Red = uint32(PxlPtr[0]);
				uint32 Green = uint32(PxlPtr[1]);
				uint32 Blue = uint32(PxlPtr[2]);
				Opacity = (float)MAX(MAX(Red,Green),Blue);
			}
			break;
		
		// Use pre-existing alpha value; useful if the opacity was loaded from a mask image
		default:
			Opacity = PxlPtr[3];
			break;
		}
		
		// Scale, shift, and put back the edited opacity;
		// round off and pin to the appropriate range.
		// The shift has to be scaled to the color-channel range (1 -> 255).
		PxlPtr[3] = PIN(int32(Options.OpacityScale*Opacity + 255*Options.OpacityShift + 0.5),0,255);
	}
}


#ifdef HAVE_OPENGL

// for managing the model and image loading and unloading
void OGL_LoadModelsImages(short Collection)
{
	assert(Collection >= 0 && Collection < MAXIMUM_COLLECTIONS);
	
	// For wall/sprite images
	OGL_LoadTextures(Collection);
	
	// For models, skins
	bool UseModels = TEST_FLAG(Get_OGL_ConfigureData().Flags,OGL_Flag_3D_Models) ? true : false;
	if (UseModels)
		OGL_LoadModels(Collection);
	else
		OGL_UnloadModels(Collection);
}

void OGL_UnloadModelsImages(short Collection)
{
	assert(Collection >= 0 && Collection < MAXIMUM_COLLECTIONS);
	
	// For wall/sprite images
	OGL_UnloadTextures(Collection);
	
	// For models, skins
	OGL_UnloadModels(Collection);
}

#else

void OGL_LoadModelsImages(int Collection)
{
}

void OGL_UnloadModelsImages(int Collection)
{
}

#endif // def HAVE_OPENGL


OGL_FogData *OGL_GetFogData(int Type)
{
	return GetMemberWithBounds(FogData,Type,OGL_NUMBER_OF_FOG_TYPES);
}


// XML-parsing stuff
OGL_FogData *OriginalFogData = NULL;
class XML_FogParser: public XML_ElementParser
{
	bool IsPresent[3];
	bool FogPresent, AffectsLandscapes;
	float Depth;
	short Type;
	
public:
	bool Start();
	bool HandleAttribute(const char *Tag, const char *Value);
	bool AttributesDone();
	bool ResetValues();

	XML_FogParser(): XML_ElementParser("fog") {}
};

bool XML_FogParser::Start()
{
	// back up old values first
	if (!OriginalFogData) {
		OriginalFogData = (OGL_FogData *) malloc(sizeof(OGL_FogData) * OGL_NUMBER_OF_FOG_TYPES);
		assert(OriginalFogData);
		for (unsigned i = 0; i < OGL_NUMBER_OF_FOG_TYPES; i++)
			OriginalFogData[i] = FogData[i];
	}

	IsPresent[0] = IsPresent[1] = IsPresent[2] = false;
	Type = 0;
	return true;
}

bool XML_FogParser::HandleAttribute(const char *Tag, const char *Value)
{
	if (StringsEqual(Tag,"on"))
	{
		if (ReadBooleanValueAsBool(Value,FogPresent))
		{
			IsPresent[0] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"depth"))
	{
		if (ReadFloatValue(Value,Depth))
		{
			IsPresent[1] = true;
			return true;
		}
		else return false;
	}
	if (StringsEqual(Tag,"landscapes"))
	{
		if (ReadBooleanValueAsBool(Value,AffectsLandscapes))
		{
			IsPresent[2] = true;
			return true;
		}
		else return false;
	}
	else if (StringsEqual(Tag,"type"))
	{
		return ReadBoundedInt16Value(Value,Type,0,OGL_NUMBER_OF_FOG_TYPES-1);
	}
	UnrecognizedTag();
	return false;
}

bool XML_FogParser::AttributesDone()
{
	OGL_FogData& Data = FogData[Type];
	if (IsPresent[0]) Data.IsPresent = FogPresent;
	if (IsPresent[1]) Data.Depth = Depth;
	if (IsPresent[2]) Data.AffectsLandscapes = AffectsLandscapes;
	Color_SetArray(&Data.Color);	
	return true;
}

bool XML_FogParser::ResetValues()
{
	if (OriginalFogData) {
		for (unsigned i = 0; i < OGL_NUMBER_OF_FOG_TYPES; i++)
			FogData[i] = OriginalFogData[i];
		free(OriginalFogData);
		OriginalFogData = NULL;
	}
	return true;
}

static XML_FogParser FogParser;

static XML_ElementParser OpenGL_Parser("opengl");


// XML-parser support:
XML_ElementParser *OpenGL_GetParser()
{
#ifdef HAVE_OPENGL
	OpenGL_Parser.AddChild(TextureOptions_GetParser());
	OpenGL_Parser.AddChild(TO_Clear_GetParser());
	
	OpenGL_Parser.AddChild(ModelData_GetParser());
	OpenGL_Parser.AddChild(Mdl_Clear_GetParser());
#endif
	
	FogParser.AddChild(Color_GetParser());
	OpenGL_Parser.AddChild(&FogParser);
	
	return &OpenGL_Parser;
}
