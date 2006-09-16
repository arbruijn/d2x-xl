uniform sampler2D btmTex, topTex;
uniform float grAlpha;
vec4 btmColor, topColor;
void main(void) 
{
topColor=texture2D(topTex,vec2(gl_TexCoord[1]));
btmColor=texture2D(btmTex,vec2(gl_TexCoord[0]));
gl_FragColor=vec4(vec3(mix(btmColor,topColor,topColor.a)),(btmColor.a+topColor.a)*grAlpha)*gl_Color;
}
