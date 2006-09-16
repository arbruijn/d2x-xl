uniform sampler2D btmTex, topTex;
vec4  btmColor, topColor;
void main(void)
{
btmColor = texture2D (btmTex, vec2 (gl_TexCoord [0]));
topColor = texture2D (topTex, vec2 (gl_TexCoord [1]));
if ((abs (topColor.r * 255.0 - 120.0) < 1.0) && 
	(abs (topColor.g * 255.0 - 88.0) < 1.0) && 
	(abs (topColor.b * 255.0 - 128.0) < 1.0))
	discard;
else if ((abs (topColor.r * 255.0 - 84.0) < 1.0) && 
		 (abs (topColor.g * 255.0 - 36.0) < 1.0) && 
		 (abs (topColor.b * 255.0 - 24.0) < 1.0))
	discard;
gl_FragColor = vec4 (vec3 (mix (btmColor, topColor, topColor.a)), btmColor.a + topColor.a) * gl_Color;
}