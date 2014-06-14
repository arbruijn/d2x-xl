uniform sampler2D bTexture,tTexture,lTexture;
float maxC;
vec4  bColor,tColor,lColor;
void main(void)
{
bColor=texture2D(bTexture,vec2(gl_TexCoord[0]));
tColor=texture2D(tTexture,vec2(gl_TexCoord[1]));
if ((abs (tColor.r * 255.0 - 120.0) < 1.0) && 
	(abs (tColor.g * 255.0 -  88.0) < 1.0) && 
	(abs (tColor.b * 255.0 - 128.0) < 1.0))
	discard;
else if ((abs (tColor.r * 255.0 - 84.0) < 1.0) && 
		 (abs (tColor.g * 255.0 - 36.0) < 1.0) && 
		 (abs (tColor.b * 255.0 - 24.0) < 1.0))
	discard;
lColor=texture2D(lTexture,vec2(gl_TexCoord[2]));
lColor+=((gl_Color)-0.5);
maxC=lColor.r;
if(lColor.g>maxC)maxC=lColor.g;
if(lColor.b>maxC)maxC=lColor.b;
if(maxC>1.0) lColor=lColor/maxC;
gl_FragColor = vec4(vec3(mix(bColor,tColor,tColor.a)),bColor.a+tColor.a)*lColor;
}