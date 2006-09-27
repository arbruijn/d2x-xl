uniform sampler2D btmTex, topTex, maskTex;
uniform float grAlpha;
uniform int tmTypeFS;

void TexMergeFS0 (void) 
{
gl_FragColor = texture2D (btmTex, vec2 (gl_TexCoord [0]));
}


void TexMergeFS1 (void) 
{
vec4 btmColor = texture2D (btmTex, vec2 (gl_TexCoord [0]));
vec4 topColor = texture2D (topTex, vec2 (gl_TexCoord [1]));
if (topColor.a == 0.0)
	gl_FragColor = vec4 (vec3 (btmColor), btmColor.a * grAlpha) * gl_Color;
else
	gl_FragColor = vec4 (vec3 (mix (btmColor, topColor, topColor.a)), (btmColor.a + topColor.a) * grAlpha) * gl_Color;
}


void TexMergeFS2 (void) 
{
vec4 topColor = texture2D (topTex, vec2 (gl_TexCoord [1]));
if (abs (topColor.a * 255.0 - 1.0) < 0.5)
	discard;
else {
	vec4 btmColor = texture2D (btmTex, vec2 (gl_TexCoord [0]));
	if (topColor.a == 0.0)
		gl_FragColor = vec4 (vec3 (btmColor), btmColor.a * grAlpha) * gl_Color;
	else
		gl_FragColor = vec4 (vec3 (mix (btmColor, topColor, topColor.a)), (btmColor.a + topColor.a) * grAlpha) * gl_Color;
	}
}


void TexMergeFS3 (void)
{
float bMask = texture2D (maskTex, vec2 (gl_TexCoord [1])).a;
if (bMask < 0.5)
	discard;
else {
	vec4 btmColor = texture2D (btmTex, vec2 (gl_TexCoord [0]));
	vec4 topColor = texture2D (topTex, vec2 (gl_TexCoord [1]));
	if (topColor.a == 0.0)
		gl_FragColor = vec4 (vec3 (btmColor), btmColor.a * grAlpha) * gl_Color;
	else 
		gl_FragColor = vec4 (vec3 (mix (btmColor, topColor, topColor.a)), (btmColor.a + topColor.a) * grAlpha) * gl_Color;
	}
}


void TexMergeFS (void)
{
if (tmTypeFS == 0)
	TexMergeFS0 ();
else if (tmTypeFS == 1)
	TexMergeFS1 ();	
else if (tmTypeFS == 2)
	TexMergeFS2 ();	
else if (tmTypeFS == 3)
	TexMergeFS3 ();	
}
	