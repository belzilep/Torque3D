//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "platform/platform.h"
#include "materials/materialFeatureTypes.h"


ImplementFeatureType( MFT_UseInstancing, U32(-1), -1, false );

ImplementFeatureType( MFT_VertTransform, MFG_Transform, 0, true );

ImplementFeatureType( MFT_TexAnim, MFG_PreTexture, 1.0f, true );
ImplementFeatureType( MFT_Parallax, MFG_PreTexture, 2.0f, true );
ImplementFeatureType( MFT_DiffuseVertColor, MFG_PreTexture, 3.0f, true );

ImplementFeatureType( MFT_DiffuseMap, MFG_Texture, 2.0f, true );
ImplementFeatureType( MFT_OverlayMap, MFG_Texture, 3.0f, true );
ImplementFeatureType( MFT_DetailMap, MFG_Texture, 4.0f, true );
ImplementFeatureType( MFT_DiffuseColor, MFG_Texture, 5.0f, true );
ImplementFeatureType( MFT_AlphaTest, MFG_Texture, 7.0f, true );
ImplementFeatureType( MFT_SpecularMap, MFG_Texture, 8.0f, true );
ImplementFeatureType( MFT_NormalMap, MFG_Texture, 9.0f, true );
ImplementFeatureType( MFT_DetailNormalMap, MFG_Texture, 10.0f, true );

ImplementFeatureType( MFT_RTLighting, MFG_Lighting, 2.0f, true );
ImplementFeatureType( MFT_SubSurface, MFG_Lighting, 3.0f, true );
ImplementFeatureType( MFT_LightMap, MFG_Lighting, 4.0f, true );
ImplementFeatureType( MFT_ToneMap, MFG_Lighting, 5.0f, true );
ImplementFeatureType( MFT_VertLitTone, MFG_Lighting, 6.0f, false );
ImplementFeatureType( MFT_VertLit, MFG_Lighting, 7.0f, true );
ImplementFeatureType( MFT_EnvMap, MFG_Lighting, 8.0f, true );
ImplementFeatureType( MFT_CubeMap, MFG_Lighting, 9.0f, true );
ImplementFeatureType( MFT_PixSpecular, MFG_Lighting, 10.0f, true );
ImplementFeatureType( MFT_MinnaertShading, MFG_Lighting, 12.0f, true );

ImplementFeatureType( MFT_GlowMask, MFG_PostLighting, 1.0f, true );
ImplementFeatureType( MFT_Visibility, MFG_PostLighting, 2.0f, true );
ImplementFeatureType( MFT_Fog, MFG_PostProcess, 3.0f, true );

ImplementFeatureType( MFT_HeatMap, MFG_PostProcess, 998.0f, true );
ImplementFeatureType( MFT_HDROut, MFG_PostProcess, 999.0f, true );

//  [4/15/2013 belp1710]
ImplementFeatureType( MFT_Fur, MFG_Texture, 11.0f, true );

ImplementFeatureType( MFT_Layer0, MFG_PreTexture, 980.0f, true);
ImplementFeatureType( MFT_Layer1, MFG_PreTexture, 981.0f, true);
ImplementFeatureType( MFT_Layer2, MFG_PreTexture, 982.0f, true);
ImplementFeatureType( MFT_Layer3, MFG_PreTexture, 983.0f, true);
ImplementFeatureType( MFT_Layer4, MFG_PreTexture, 984.0f, true);
ImplementFeatureType( MFT_Layer5, MFG_PreTexture, 985.0f, true);
ImplementFeatureType( MFT_Layer6, MFG_PreTexture, 986.0f, true);
ImplementFeatureType( MFT_Layer7, MFG_PreTexture, 987.0f, true);
ImplementFeatureType( MFT_Layer8, MFG_PreTexture, 988.0f, true);
ImplementFeatureType( MFT_Layer9, MFG_PreTexture, 989.0f, true);
ImplementFeatureType( MFT_Layer10, MFG_PreTexture, 990.0f, true);
ImplementFeatureType( MFT_Layer11, MFG_PreTexture, 991.0f, true);
ImplementFeatureType( MFT_Layer12, MFG_PreTexture, 992.0f, true);
ImplementFeatureType( MFT_Layer13, MFG_PreTexture, 993.0f, true);
ImplementFeatureType( MFT_Layer14, MFG_PreTexture, 994.0f, true);
ImplementFeatureType( MFT_Layer15, MFG_PreTexture, 995.0f, true);

ImplementFeatureType( MFT_IsDXTnm, U32(-1), -1, true );
ImplementFeatureType( MFT_IsTranslucent, U32(-1), -1, true );
ImplementFeatureType( MFT_IsTranslucentZWrite, U32(-1), -1, true );
ImplementFeatureType( MFT_IsEmissive, U32(-1), -1, true );
ImplementFeatureType( MFT_GlossMap, U32(-1), -1, true );
ImplementFeatureType( MFT_DiffuseMapAtlas, U32(-1), -1, true );
ImplementFeatureType( MFT_NormalMapAtlas, U32(-1), -1, true );
ImplementFeatureType( MFT_InterlacedPrePass, U32(-1), -1, true );

ImplementFeatureType( MFT_ParaboloidVertTransform, MFG_Transform, -1, false );
ImplementFeatureType( MFT_IsSinglePassParaboloid, U32(-1), -1, false );
ImplementFeatureType( MFT_EyeSpaceDepthOut, MFG_PostLighting, 2.0f, false );
ImplementFeatureType( MFT_DepthOut, MFG_PostLighting, 3.0f, false );
ImplementFeatureType( MFT_PrePassConditioner, MFG_PostProcess, 1.0f, false );
ImplementFeatureType( MFT_NormalsOut, MFG_PreLighting, 1.0f, false );

ImplementFeatureType( MFT_LightbufferMRT, MFG_PreLighting, 1.0f, false );
ImplementFeatureType( MFT_RenderTarget1_Zero, MFG_PreTexture, 1.0f, false );

ImplementFeatureType( MFT_Foliage, MFG_PreTransform, 1.0f, false );

ImplementFeatureType( MFT_ParticleNormal, MFG_PreTransform, 2.0f, false );

ImplementFeatureType( MFT_ForwardShading, U32(-1), -1, true );

ImplementFeatureType( MFT_ImposterVert, MFG_PreTransform, 1.0, false );