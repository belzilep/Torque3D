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
#ifndef _SHADERGEN_HLSL_SHADERFEATUREHLSL_H_
#define _SHADERGEN_HLSL_SHADERFEATUREHLSL_H_

#ifndef _SHADERFEATURE_H_
#include "shaderGen/shaderFeature.h"
#endif

struct LangElement;
struct MaterialFeatureData;
struct RenderPassData;


class ShaderFeatureHLSL : public ShaderFeature
{
public:
   ShaderFeatureHLSL();

   ///
   Var* getOutTexCoord( const char *name,
                        const char *type,
                        bool mapsToSampler,
                        bool useTexAnim,
                        MultiLine *meta,
                        Vector<ShaderComponent*> &componentList );

   /// Returns an input texture coord by name adding it
   /// to the input connector if it doesn't exist.
   static Var* getInTexCoord( const char *name,
                              const char *type,
                              bool mapsToSampler,
                              Vector<ShaderComponent*> &componentList );

   static Var* getInColor( const char *name,
                           const char *type,
                           Vector<ShaderComponent*> &componentList );

   ///
   static Var* addOutVpos( MultiLine *meta,
                           Vector<ShaderComponent*> &componentList );

   /// Returns the VPOS input register for the pixel shader.
   static Var* getInVpos(  MultiLine *meta,
                           Vector<ShaderComponent*> &componentList );

   /// Returns the "objToTangentSpace" transform or creates one if this
   /// is the first feature to need it.
   Var* getOutObjToTangentSpace( Vector<ShaderComponent*> &componentList,
                                 MultiLine *meta,
                                 const MaterialFeatureData &fd );

   /// Returns the existing output "outWorldToTangent" transform or 
   /// creates one if this is the first feature to need it.
   Var* getOutWorldToTangent( Vector<ShaderComponent*> &componentList,
                              MultiLine *meta,
                              const MaterialFeatureData &fd );

   /// Returns the input "worldToTanget" space transform 
   /// adding it to the input connector if it doesn't exist.
   static Var* getInWorldToTangent( Vector<ShaderComponent*> &componentList );

   /// Returns the existing output "outViewToTangent" transform or 
   /// creates one if this is the first feature to need it.
   Var* getOutViewToTangent(  Vector<ShaderComponent*> &componentList,
                              MultiLine *meta,
                              const MaterialFeatureData &fd );

   /// Returns the input "viewToTangent" space transform 
   /// adding it to the input connector if it doesn't exist.
   static Var* getInViewToTangent( Vector<ShaderComponent*> &componentList );

   /// Calculates the world space position in the vertex shader and 
   /// assigns it to the passed language element.  It does not pass 
   /// it across the connector to the pixel shader.
   /// @see addOutWsPosition
   void getWsPosition(  Vector<ShaderComponent*> &componentList,                                       
                        bool useInstancing,
                        MultiLine *meta,
                        LangElement *wsPosition );

   /// Adds the "wsPosition" to the input connector if it doesn't exist.
   Var* addOutWsPosition(  Vector<ShaderComponent*> &componentList,                                       
                           bool useInstancing,
                           MultiLine *meta );

   /// Returns the input world space position from the connector.
   static Var* getInWsPosition( Vector<ShaderComponent*> &componentList );

   /// Returns the world space view vector from the wsPosition.
   static Var* getWsView( Var *wsPosition, MultiLine *meta );

   /// Returns the input normal map texture.
   static Var* getNormalMapTex();

   ///
   Var* addOutDetailTexCoord( Vector<ShaderComponent*> &componentList, 
                              MultiLine *meta,
                              bool useTexAnim );

   ///
   Var* getObjTrans( Vector<ShaderComponent*> &componentList,                                       
                     bool useInstancing,
                     MultiLine *meta );

   ///
   Var* getModelView(   Vector<ShaderComponent*> &componentList,                                       
                        bool useInstancing,
                        MultiLine *meta );

   ///
   Var* getWorldView(   Vector<ShaderComponent*> &componentList,                                       
                        bool useInstancing,
                        MultiLine *meta );

   ///
   Var* getInvWorldView(   Vector<ShaderComponent*> &componentList,                                       
                           bool useInstancing,
                           MultiLine *meta );

   // ShaderFeature
   Var* getVertTexCoord( const String &name );
   LangElement* setupTexSpaceMat(  Vector<ShaderComponent*> &componentList, Var **texSpaceMat );
   LangElement* assignColor( LangElement *elem, Material::BlendOp blend, LangElement *lerpElem = NULL, ShaderFeature::OutputTarget outputTarget = ShaderFeature::DefaultTarget );
   LangElement* expandNormalMap( LangElement *sampleNormalOp, LangElement *normalDecl, LangElement *normalVar, const MaterialFeatureData &fd );
};


class NamedFeatureHLSL : public ShaderFeatureHLSL
{
protected:
   String mName;

public:
   NamedFeatureHLSL( const String &name )
      : mName( name )
   {}

   virtual String getName() { return mName; }
};

class RenderTargetZeroHLSL : public ShaderFeatureHLSL
{
protected:
   ShaderFeature::OutputTarget mOutputTargetMask;
   String mFeatureName;

public:
   RenderTargetZeroHLSL( const ShaderFeature::OutputTarget target )
      : mOutputTargetMask( target )
   {
      char buffer[256];
      dSprintf(buffer, sizeof(buffer), "Render Target Output = 0.0, output mask %04b", mOutputTargetMask);
      mFeatureName = buffer;
   }

   virtual String getName() { return mFeatureName; }

   virtual void processPix( Vector<ShaderComponent*> &componentList, 
      const MaterialFeatureData &fd );
   
   virtual U32 getOutputTargets( const MaterialFeatureData &fd ) const { return mOutputTargetMask; }
};


class HeatMapFeatureHLSL : public ShaderFeatureHLSL
{
protected:
	String mFeatureName;

public:
	HeatMapFeatureHLSL()
	{
		char buffer[256];
		dSprintf(buffer, sizeof(buffer), "Heat Map");
		mFeatureName = buffer;
	}

	virtual String getName() { return mFeatureName; }

	virtual void processVert( Vector<ShaderComponent*> &componentList,
		const MaterialFeatureData &fd );

	virtual void processPix( Vector<ShaderComponent*> &componentList, 
		const MaterialFeatureData &fd );
};


/// Vertex position
class VertPositionHLSL : public ShaderFeatureHLSL
{
private:
	U32 mStage;
public:
   virtual void processVert( Vector<ShaderComponent*> &componentList,
                             const MaterialFeatureData &fd );
                             
   virtual String getName()
   {
      return "Vert Position";
   }

   virtual void determineFeature(   Material *material,
                                    const GFXVertexFormat *vertexFormat,
                                    U32 stageNum,
                                    const FeatureType &type,
                                    const FeatureSet &features,
                                    MaterialFeatureData *outFeatureData );

};


/// Vertex lighting based on the normal and the light 
/// direction passed through the vertex color.
class RTLightingFeatHLSL : public ShaderFeatureHLSL
{
protected:

   ShaderIncludeDependency mDep;

public:

   RTLightingFeatHLSL();

   virtual void processVert( Vector<ShaderComponent*> &componentList,
                             const MaterialFeatureData &fd );

   virtual void processPix( Vector<ShaderComponent*> &componentList, 
                            const MaterialFeatureData &fd );

   virtual Material::BlendOp getBlendOp(){ return Material::None; }
   
   virtual Resources getResources( const MaterialFeatureData &fd );

   virtual String getName()
   {
      return "RT Lighting";
   }
};


/// Base texture
class DiffuseMapFeatHLSL : public ShaderFeatureHLSL
{
public:
   virtual void processVert( Vector<ShaderComponent*> &componentList,
                             const MaterialFeatureData &fd );

   virtual void processPix( Vector<ShaderComponent*> &componentList, 
                            const MaterialFeatureData &fd );

   virtual Material::BlendOp getBlendOp(){ return Material::LerpAlpha; }

   virtual Resources getResources( const MaterialFeatureData &fd );

   // Sets textures and texture flags for current pass
   virtual void setTexData( Material::StageData &stageDat,
                            const MaterialFeatureData &fd,
                            RenderPassData &passData,
                            U32 &texIndex );
                            
   virtual String getName()
   {
      return "Base Texture";
   }
};


/// Overlay texture
class OverlayTexFeatHLSL : public ShaderFeatureHLSL
{
public:
   virtual void processVert( Vector<ShaderComponent*> &componentList,
                             const MaterialFeatureData &fd );

   virtual void processPix( Vector<ShaderComponent*> &componentList, 
                            const MaterialFeatureData &fd );

   virtual Material::BlendOp getBlendOp(){ return Material::LerpAlpha; }

   virtual Resources getResources( const MaterialFeatureData &fd );

   // Sets textures and texture flags for current pass
   virtual void setTexData( Material::StageData &stageDat,
                            const MaterialFeatureData &fd,
                            RenderPassData &passData,
                            U32 &texIndex );

   virtual String getName()
   {
      return "Overlay Texture";
   }
};


/// Diffuse color
class DiffuseFeatureHLSL : public ShaderFeatureHLSL
{
public:   
   virtual void processPix(   Vector<ShaderComponent*> &componentList, 
                              const MaterialFeatureData &fd );

   virtual Material::BlendOp getBlendOp(){ return Material::None; }

   virtual String getName()
   {
      return "Diffuse Color";
   }
};

/// Diffuse vertex color
class DiffuseVertColorFeatureHLSL : public ShaderFeatureHLSL
{
public:   

   virtual void processVert(  Vector< ShaderComponent* >& componentList,
                              const MaterialFeatureData& fd );
   virtual void processPix(   Vector< ShaderComponent* >&componentList, 
                              const MaterialFeatureData& fd );

   virtual Material::BlendOp getBlendOp(){ return Material::None; }

   virtual String getName()
   {
      return "Diffuse Vertex Color";
   }
};

/// Lightmap
class LightmapFeatHLSL : public ShaderFeatureHLSL
{
public:
   virtual void processVert(  Vector<ShaderComponent*> &componentList,
                              const MaterialFeatureData &fd );

   virtual void processPix(   Vector<ShaderComponent*> &componentList, 
                              const MaterialFeatureData &fd );

   virtual Material::BlendOp getBlendOp(){ return Material::LerpAlpha; }

   virtual Resources getResources( const MaterialFeatureData &fd );

   // Sets textures and texture flags for current pass
   virtual void setTexData( Material::StageData &stageDat,
                            const MaterialFeatureData &fd,
                            RenderPassData &passData,
                            U32 &texIndex );
                            
   virtual String getName()
   {
      return "Lightmap";
   }

   virtual U32 getOutputTargets( const MaterialFeatureData &fd ) const;
};


/// Tonemap
class TonemapFeatHLSL : public ShaderFeatureHLSL
{
public:
   virtual void processVert(  Vector<ShaderComponent*> &componentList,
                              const MaterialFeatureData &fd );

   virtual void processPix(   Vector<ShaderComponent*> &componentList, 
                              const MaterialFeatureData &fd );

   virtual Material::BlendOp getBlendOp(){ return Material::LerpAlpha; }

   virtual Resources getResources( const MaterialFeatureData &fd );

   // Sets textures and texture flags for current pass
   virtual void setTexData( Material::StageData &stageDat,
                            const MaterialFeatureData &fd,
                            RenderPassData &passData,
                            U32 &texIndex );
                            
   virtual String getName()
   {
      return "Tonemap";
   }

   virtual U32 getOutputTargets( const MaterialFeatureData &fd ) const;
};


/// Baked lighting stored on the vertex color
class VertLitHLSL : public ShaderFeatureHLSL
{
public:
   virtual void processVert( Vector<ShaderComponent*> &componentList,
                             const MaterialFeatureData &fd );

   virtual void processPix( Vector<ShaderComponent*> &componentList, 
                            const MaterialFeatureData &fd );

   virtual Material::BlendOp getBlendOp(){ return Material::None; }
   
   virtual String getName()
   {
      return "Vert Lit";
   }

   virtual U32 getOutputTargets( const MaterialFeatureData &fd ) const;
};


/// Detail map
class DetailFeatHLSL : public ShaderFeatureHLSL
{
public:
   virtual void processVert( Vector<ShaderComponent*> &componentList,
                             const MaterialFeatureData &fd );

   virtual void processPix( Vector<ShaderComponent*> &componentList, 
                            const MaterialFeatureData &fd );

   virtual Resources getResources( const MaterialFeatureData &fd );

   virtual Material::BlendOp getBlendOp(){ return Material::Mul; }

   // Sets textures and texture flags for current pass
   virtual void setTexData( Material::StageData &stageDat,
                            const MaterialFeatureData &fd,
                            RenderPassData &passData,
                            U32 &texIndex );

   virtual String getName()
   {
      return "Detail";
   }
};


/// Reflect Cubemap
class ReflectCubeFeatHLSL : public ShaderFeatureHLSL
{
public:
   virtual void processVert( Vector<ShaderComponent*> &componentList,
                             const MaterialFeatureData &fd );

   virtual void processPix( Vector<ShaderComponent*> &componentList, 
                            const MaterialFeatureData &fd );

   virtual Resources getResources( const MaterialFeatureData &fd );

   // Sets textures and texture flags for current pass
   virtual void setTexData( Material::StageData &stageDat,
                            const MaterialFeatureData &fd,
                            RenderPassData &passData,
                            U32 &texIndex );

   virtual String getName()
   {
      return "Reflect Cube";
   }
};


/// Fog
class FogFeatHLSL : public ShaderFeatureHLSL
{
protected:

   ShaderIncludeDependency mFogDep;

public:
   FogFeatHLSL();

   virtual void processVert( Vector<ShaderComponent*> &componentList,
                             const MaterialFeatureData &fd );

   virtual void processPix( Vector<ShaderComponent*> &componentList, 
                            const MaterialFeatureData &fd );

   virtual Resources getResources( const MaterialFeatureData &fd );

   virtual Material::BlendOp getBlendOp() { return Material::LerpAlpha; }

   virtual String getName()
   {
      return "Fog";
   }
};


/// Tex Anim
class TexAnimHLSL : public ShaderFeatureHLSL
{
public:
   virtual Material::BlendOp getBlendOp() { return Material::None; }

   virtual String getName()
   {
      return "Texture Animation";
   }
};


/// Visibility
class VisibilityFeatHLSL : public ShaderFeatureHLSL
{
protected:

   ShaderIncludeDependency mTorqueDep;

public:

   VisibilityFeatHLSL();

   virtual void processVert( Vector<ShaderComponent*> &componentList,
                             const MaterialFeatureData &fd );

   virtual void processPix( Vector<ShaderComponent*> &componentList, 
                            const MaterialFeatureData &fd );

   virtual Resources getResources( const MaterialFeatureData &fd );

   virtual Material::BlendOp getBlendOp() { return Material::None; }

   virtual String getName()
   {
      return "Visibility";
   }
};


///
class AlphaTestHLSL : public ShaderFeatureHLSL
{
public:
   virtual void processPix(   Vector<ShaderComponent*> &componentList, 
                              const MaterialFeatureData &fd );

   virtual Material::BlendOp getBlendOp() { return Material::None; }

   virtual String getName()
   {
      return "Alpha Test";
   }
};


/// Special feature used to mask out the RGB color for
/// non-glow passes of glow materials.
/// @see RenderGlowMgr
class GlowMaskHLSL : public ShaderFeatureHLSL
{
public:
   virtual void processPix(   Vector<ShaderComponent*> &componentList, 
                              const MaterialFeatureData &fd );

   virtual Material::BlendOp getBlendOp() { return Material::None; }

   virtual String getName()
   {
      return "Glow Mask";
   }
};

/// This should be the final feature on most pixel shaders which
/// encodes the color for the current HDR target format.
/// @see HDRPostFx
/// @see LightManager
/// @see torque.hlsl
class HDROutHLSL : public ShaderFeatureHLSL
{
protected:

   ShaderIncludeDependency mTorqueDep;

public:

   HDROutHLSL();

   virtual void processPix(   Vector<ShaderComponent*> &componentList, 
                              const MaterialFeatureData &fd );

   virtual Material::BlendOp getBlendOp() { return Material::None; }

   virtual String getName() { return "HDR Output"; }
};

///
class FoliageFeatureHLSL : public ShaderFeatureHLSL
{
protected:

   ShaderIncludeDependency mDep;

public:

   FoliageFeatureHLSL();

   virtual void processVert( Vector<ShaderComponent*> &componentList,
                             const MaterialFeatureData &fd );

   virtual void processPix( Vector<ShaderComponent*> &componentList,
                           const MaterialFeatureData &fd );   

   virtual String getName()
   {
      return "Foliage Feature";
   }

   virtual void determineFeature( Material *material, 
                                  const GFXVertexFormat *vertexFormat,
                                  U32 stageNum,
                                  const FeatureType &type,
                                  const FeatureSet &features,
                                  MaterialFeatureData *outFeatureData );

   virtual ShaderFeatureConstHandles* createConstHandles( GFXShader *shader, SimObject *userObject );   
};

class ParticleNormalFeatureHLSL : public ShaderFeatureHLSL
{
public:

   virtual void processVert( Vector<ShaderComponent*> &componentList,
      const MaterialFeatureData &fd );

   virtual String getName()
   {
      return "Particle Normal Generation Feature";
   }

};


/// Special feature for unpacking imposter verts.
/// @see RenderImposterMgr
class ImposterVertFeatureHLSL : public ShaderFeatureHLSL
{
protected:

   ShaderIncludeDependency mDep;

public:

   ImposterVertFeatureHLSL();

   virtual void processVert(  Vector<ShaderComponent*> &componentList,
                              const MaterialFeatureData &fd );

   virtual void processPix(  Vector<ShaderComponent*> &componentList,
                             const MaterialFeatureData &fd );

   virtual String getName() { return "Imposter Vert"; }

   virtual void determineFeature( Material *material, 
                                  const GFXVertexFormat *vertexFormat,
                                  U32 stageNum,
                                  const FeatureType &type,
                                  const FeatureSet &features,
                                  MaterialFeatureData *outFeatureData );
};


//  [4/15/2013 belp1710]
class FurFeatureHLSL : public ShaderFeatureHLSL
{
public:
	virtual String getName()
	{
		return "Fur";
	}

	virtual Resources getResources( const MaterialFeatureData &fd );

	// Sets textures and texture flags for current pass
	virtual void setTexData( Material::StageData &stageDat,
		const MaterialFeatureData &fd,
		RenderPassData &passData,
		U32 &texIndex );

	virtual void determineFeature(   Material *material,
		const GFXVertexFormat *vertexFormat,
		U32 stageNum,
		const FeatureType &type,
		const FeatureSet &features,
		MaterialFeatureData *outFeatureData );

	virtual void processPix( Vector<ShaderComponent*> &componentList, const MaterialFeatureData &fd );
};

class Layer0FeatHLSL : public ShaderFeatureHLSL
{
public:
	virtual String getName()
	{
		return "Layer 0";
	}
};

class Layer1FeatHLSL : public ShaderFeatureHLSL
{
public:
	virtual String getName()
	{
		return "Layer 1";
	}
};

class Layer2FeatHLSL : public ShaderFeatureHLSL
{
public:
	virtual String getName()
	{
		return "Layer 2";
	}
};

class Layer3FeatHLSL : public ShaderFeatureHLSL
{
public:
	virtual String getName()
	{
		return "Layer 3";
	}
};

class Layer4FeatHLSL : public ShaderFeatureHLSL
{
public:
	virtual String getName()
	{
		return "Layer 4";
	}
};

class Layer5FeatHLSL : public ShaderFeatureHLSL
{
public:
	virtual String getName()
	{
		return "Layer 5";
	}
};

class Layer6FeatHLSL : public ShaderFeatureHLSL
{
public:
	virtual String getName()
	{
		return "Layer 6";
	}
};

class Layer7FeatHLSL : public ShaderFeatureHLSL
{
public:
	virtual String getName()
	{
		return "Layer 7";
	}
};

class Layer8FeatHLSL : public ShaderFeatureHLSL
{
public:
	virtual String getName()
	{
		return "Layer 8";
	}
};

class Layer9FeatHLSL : public ShaderFeatureHLSL
{
public:
	virtual String getName()
	{
		return "Layer 9";
	}
};

class Layer10FeatHLSL : public ShaderFeatureHLSL
{
public:
	virtual String getName()
	{
		return "Layer 10";
	}
};

class Layer11FeatHLSL : public ShaderFeatureHLSL
{
public:
	virtual String getName()
	{
		return "Layer 11";
	}
};

class Layer12FeatHLSL : public ShaderFeatureHLSL
{
public:
	virtual String getName()
	{
		return "Layer 12";
	}
};

class Layer13FeatHLSL : public ShaderFeatureHLSL
{
public:
	virtual String getName()
	{
		return "Layer 13";
	}
};

class Layer14FeatHLSL : public ShaderFeatureHLSL
{
public:
	virtual String getName()
	{
		return "Layer 14";
	}
};

class Layer15FeatHLSL : public ShaderFeatureHLSL
{
public:
	virtual String getName()
	{
		return "Layer 15";
	}
};

#endif // _SHADERGEN_HLSL_SHADERFEATUREHLSL_H_
