// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

varying vec3 varyingEyepos;
varying vec3 varyingNormal;
varying vec3 varyingVertex;

uniform Scene scene;
uniform Material material;
uniform float shieldStrength;
uniform float shieldCooldown;

#define MAX_SHIELD_HITS 5
// hitPos entries should be in object local space
uniform vec3 hitPos[MAX_SHIELD_HITS];
uniform float radii[MAX_SHIELD_HITS];
uniform int numHits;

const vec4 red = vec4(1.0, 0.5, 0.5, 0.5);
const vec4 blue = vec4(0.5, 0.5, 1.0, 1.0);
const vec4 hitColour = vec4(1.0, 0.5, 0.5, 1.0);

float calcIntensity(const in vec3 current_position, const in float life)
{
	//vec3 current_position = hitPos[shieldIndex];
	//float life = radii[shieldIndex];
	float radius = 50.0 * life;
	vec3 dif = varyingVertex - current_position;
	
	float sqrDist = dot(dif,dif);

	return clamp(1.0/sqrDist*radius, 0.0, 0.9) * (1.0 - life);
}

void main(void)
{
	//vec4 color = material.diffuse;
	vec4 color = mix(red, blue, shieldStrength);
	vec4 fillColour = color * 0.15;
	
	vec3 eyenorm = normalize(-varyingEyepos);

	float fresnel = 1.0 - abs(dot(eyenorm, varyingNormal)); // Calculate fresnel.
	fresnel = pow(fresnel, 10.0);
	fresnel += 0.05 * (1.0 - fresnel);
	
	float sumIntensity = 0.0;
/*	for ( int hit=0; hit<numHits; hit++ )
	{
		sumIntensity += calcIntensity(hit);
	}
*/
	
	if (numHits >= 0) 
	{
		vec3 current_position = hitPos[0];
		float life = radii[0];
		sumIntensity += calcIntensity(hitPos[0], radii[0]);
	}
	if (numHits >= 1) 
	{
		vec3 current_position = hitPos[1];
		float life = radii[1];
		sumIntensity += calcIntensity(hitPos[1], radii[1]);
	}
	if (numHits >= 2) 
	{
		vec3 current_position = hitPos[2];
		float life = radii[2];
		sumIntensity += calcIntensity(hitPos[2], radii[2]);
	}
	if (numHits >= 3) 
	{
		vec3 current_position = hitPos[3];
		float life = radii[3];
		sumIntensity += calcIntensity(hitPos[3], radii[3]);
	}
	if (numHits >= 4) 
	{
		vec3 current_position = hitPos[4];
		float life = radii[4];
		sumIntensity += calcIntensity(hitPos[4], radii[4]);
	}
	
	/*if (numHits > 0) sumIntensity += calcIntensity1();
	if (numHits > 1) sumIntensity += calcIntensity2();
	if (numHits > 2) sumIntensity += calcIntensity3();
	if (numHits > 3) sumIntensity += calcIntensity4();
	*/
	float clampedInt = clamp(sumIntensity, 0.0, 1.0);
	
	// combine a base colour with the (clamped) fresnel value and fade it out according to the cooldown period.
	color.a = (fillColour.a + clamp(fresnel * 0.5, 0.0, 1.0)) * shieldCooldown;
	// add on our hit effect colour
	color = color + (hitColour * clampedInt);
	
	gl_FragColor = color;

	SetFragDepth();
}
