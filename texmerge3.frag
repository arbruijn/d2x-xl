uniform sampler2D btmTex, topTex, maskTex;
uniform float grAlpha;
vec4 btmColor, topColor;
float bMask;
void main (void)
{
bMask = texture2D (maskTex, vec2 (gl_TexCoord [1])).a;
if (bMask < 0.5)
	discard;
else {
	topColor = texture2D (topTex, vec2 (gl_TexCoord [1]));
	btmColor = texture2D (btmTex, vec2 (gl_TexCoord [0]));
	if(topColor.a == 0.0)
		gl_FragColor = vec4 (vec3 (btmColor), btmColor.a * grAlpha) * gl_Color;
	else 
		gl_FragColor = vec4 (vec3 (mix (btmColor, topColor, topColor.a)), (btmColor.a + topColor.a) * grAlpha) * gl_Color;
	}
}
