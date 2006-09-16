uniform sampler2D btmTex, topTex;
uniform float grAlpha;
vec4 btmColor, topColor;
void main(void) {
topColor=texture2D(topTex,vec2(gl_TexCoord[1]));
btmColor=texture2D(btmTex,vec2(gl_TexCoord[0]));
if(topColor.a==0.0){
	if((abs(topColor.r*255.0-120.0)<1.0)&&
	   (abs(topColor.g*255.0-88.0)<1.0)&&
	   (abs(topColor.b*255.0-128.0)<1.0))
		discard;
	else 
		gl_FragColor=vec4(vec3(btmColor),btmColor.a*grAlpha)*gl_Color;
	}
else{
	gl_FragColor=vec4(vec3(mix(btmColor,topColor,topColor.a)),(btmColor.a+topColor.a)*grAlpha)*gl_Color;
	}
}
