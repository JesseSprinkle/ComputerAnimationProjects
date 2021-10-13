#version 120

attribute vec4 aPos;
attribute vec3 aNor;
attribute vec2 aTex;
attribute vec3 dBlendPos0;
attribute vec3 dBlendPos1;
attribute vec3 dBlendPos2;
attribute vec3 dBlendNor0;
attribute vec3 dBlendNor1;
attribute vec3 dBlendNor2;

uniform mat4 P;
uniform mat4 MV;
uniform float t;
uniform float numBlendShapes;

varying vec3 vPos;
varying vec3 vNor;
varying vec2 vTex;

void main()
{
	vec4 nPos = aPos;
	vec3 nNor = aNor;
	if(numBlendShapes > 0)
	{
		float t0 = sin(t) / 2.0 + 0.5;
		nPos += vec4(t0 * dBlendPos0, 0);
		nNor += t0 * dBlendNor0;
	}
	if(numBlendShapes > 1)
	{
		float t1 = sin(t * 2.3) / 2.0 + 0.5;
		nPos += vec4(t1 * dBlendPos1, 0);
		nNor += t1 * dBlendNor1;
	}
	if(numBlendShapes > 2)
	{
		float t2 = sin(t * 4.7) / 2.0 + 0.5;
		nPos += vec4(t2 * dBlendPos2, 0);
		nNor += t2 * dBlendNor2;
	}
	nNor = normalize(nNor);
	vec4 posCam = MV * nPos;
	vec3 norCam = (MV * vec4(nNor, 0.0)).xyz;
	gl_Position = P * posCam;
	vPos = posCam.xyz;
	vNor = norCam;
	vTex = aTex;
}
