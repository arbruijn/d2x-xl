uniform sampler2D btmTex,topTex,lMapTex;
void main(void){
vec4 topColor=texture2D(topTex,vec2(gl_TexCoord[1]));
if(abs(topColor.a-1.0/255.0)<0.25)discard;
else {vec4 btmColor=texture2D(btmTex,vec2(gl_TexCoord[0]));
vec4 lMapColor=texture2D(lMapTex,vec2(gl_TexCoord[2]))+((gl_Color)-0.5);
float maxC=lMapColor.r;
if(lMapColor.g>maxC)maxC=lMapColor.g;
if(lMapColor.b>maxC)maxC=lMapColor.b;
if(maxC>1.0){lMapColor.r/=maxC;lMapColor.g/=maxC;lMapColor.b/=maxC;}
if(topColor.a==0.0)gl_FragColor = btmColor*lMapColor;
else {lMapColor.a = gl_Color.a;
gl_FragColor = vec4(vec3(mix(btmColor,topColor,topColor.a)),(btmColor.a+topColor.a))*lMapColor;}}}
