uniform sampler2D btmTex,topTex,lMapTex;
float maxC,r,g,b;
vec4 btmColor,topColor,lMapColor;
void main(void)
{
btmColor=texture2D(btmTex,vec2(gl_TexCoord[0]));
topColor=texture2D(topTex,vec2(gl_TexCoord[1]));
lMapColor=texture2D(lMapTex,vec2(gl_TexCoord[2]))+((gl_Color)-0.5);
maxC=lMapColor.r;
if(lMapColor.g>maxC)
	maxC=lMapColor.g;
if(lMapColor.b>maxC)
	maxC=lMapColor.b;
if(maxC>1.0) {
	lMapColor.r/=maxC;
	lMapColor.g/=maxC;
	lMapColor.b/=maxC;
	}
lMapColor.a = gl_Color.a;
gl_FragColor = vec4(vec3(mix(btmColor,topColor,topColor.a)),(btmColor.a+topColor.a))*lMapColor;
}
