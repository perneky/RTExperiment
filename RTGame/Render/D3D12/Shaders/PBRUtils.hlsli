#pragma once

static const float3 Fdielectric = 0.04;
static const float  PBREpsilon  = 0.00001;

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float ndfGGX( float cosLh, float roughness )
{
	float alpha = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = ( cosLh * cosLh ) * ( alphaSq - 1.0 ) + 1.0;
	return alphaSq / ( PI * denom * denom );
}

// Single term for separable Schlick-GGX below.
float gaSchlickG1( float cosTheta, float k )
{
	return cosTheta / ( cosTheta * ( 1.0 - k ) + k );
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float gaSchlickGGX( float cosLi, float cosLo, float roughness )
{
	float r = roughness + 1.0;
	float k = ( r * r ) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return gaSchlickG1( cosLi, k ) * gaSchlickG1( cosLo, k );
}

// Shlick's approximation of the Fresnel factor.
float3 fresnelSchlick( float3 F0, float cosTheta )
{
	return F0 + ( 1.0 - F0 ) * pow( 1.0 - cosTheta, 5.0 );
}
