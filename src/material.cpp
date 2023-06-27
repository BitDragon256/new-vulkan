#include "material.h"

bool Material::operator==(const Material& other)
{
	return
		m_pDiffuseTexture == other.m_pDiffuseTexture &&
		m_pNormalTexture  == other.m_pNormalTexture  &&
		m_pShader		  == other.m_pShader		 &&
		m_diffuseColor	  == other.m_diffuseColor	 &&
		m_ambientColor	  == other.m_ambientColor	 &&
		m_alphaTest		  == other.m_alphaTest
	;
}