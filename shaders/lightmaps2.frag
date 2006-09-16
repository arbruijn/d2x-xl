uniform sampler2D btmTex,lMapTex;
float maxC;
vec4  btmColor,topColor,lMapColor;
void main(void)
{
btmColor=texture2D(btmTex,vec2(gl_TexCoord[0]));
lMapColor=texture2D(lMapTex,vec2(gl_TexCoord[2]));
lMapColor+=((gl_Color)-0.5);
maxC=lMapColor.r;
if(lMapColor.g>maxC)maxC=lMapColor.g;
if(lMapColor.b>maxC)maxC=lMapColor.b;
if(maxC>1.0) lMapColor=lMapColor/maxC;
gl_FragColor = btmColor*lMapColor;
}