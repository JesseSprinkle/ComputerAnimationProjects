#version 120

attribute vec4 aPos;
attribute vec3 aNor;
attribute vec2 aTex;
attribute vec4 w0;
attribute vec4 w1;
attribute vec4 w2;
attribute vec4 b0;
attribute vec4 b1;
attribute vec4 b2;
attribute float numInfl;

uniform mat4 P;
uniform mat4 MV;
uniform mat3 T;
uniform mat4 M[82];

varying vec3 vPos;
varying vec3 vNor;
varying vec2 vTex;

void main()
{
	vec4 iNor = vec4(aNor, 0.0);
	vec4 resultPos = vec4(0.0, 0.0, 0.0, 0.0);
	vec4 resultNor = vec4(0.0, 0.0, 0.0, 0.0);
	for(int i = 0; i < int(numInfl); i++)
	{
		if(i > 7)
		{
			resultPos += w2[i - 8] * M[int(b2[i - 8])] * aPos;
			resultNor += w2[i - 8] * M[int(b2[i - 8])] * iNor;
		}
		else if(i > 3)
		{
			resultPos += w1[i - 4] * M[int(b1[i - 4])] * aPos;
			resultNor += w1[i - 4] * M[int(b1[i - 4])] * iNor;
		}
		else
		{
			resultPos += w0[i] * M[int(b0[i])] * aPos;
			resultNor += w0[i] * M[int(b0[i])] * iNor;
		}
	}
	vec4 posCam = MV * resultPos;
	vec3 norCam = (MV * resultNor).xyz;
	gl_Position = P * posCam;
	vPos = posCam.xyz;
	vNor = norCam;
	vTex = vec2(T * vec3(aTex, 1.0));
}
