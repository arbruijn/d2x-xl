uniform sampler2D btmTex,topTex,lMapTex;
float maxC,r,g,b;
vec4 btmColor,topColor,lMapColor;
void main(void)
{
topColor=texture2D(topTex,vec2(gl_TexCoord[1]));
if (topColor.a == 1.0/255.0)
	discard;
else {
	btmColor=texture2D(btmTex,vec2(gl_TexCoord[0]));
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
	if(topColor.a==0.0) {
		if((abs(topColor.r*255.0-120.0)<1.0)&&
			(abs(topColor.g*255.0-88.0)<1.0)&&
			(abs(topColor.b*255.0-128.0)<1.0))
			discard;
		else
			gl_FragColor = btmColor*lMapColor;
		}
	else {
		lMapColor.a = gl_Color.a;
		gl_FragColor = vec4(vec3(mix(btmColor,topColor,topColor.a)),(btmColor.a+topColor.a))*lMapColor;
		}
	}
}
