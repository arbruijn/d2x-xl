uniform int tmTypeVS;

void TexMergeVS0 (void)
{	
gl_TexCoord [0] = gl_MultiTexCoord0;
gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
gl_FrontColor = gl_Color;
}


void TexMergeVS12 (void)
{	
gl_TexCoord [0] = gl_MultiTexCoord0;
gl_TexCoord [1] = gl_MultiTexCoord1;
gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
gl_FrontColor = gl_Color;
}


void TexMergeVS3 (void)
{	
gl_TexCoord [0] = gl_MultiTexCoord0;
gl_TexCoord [1] = gl_MultiTexCoord1;
gl_TexCoord [2] = gl_MultiTexCoord2;
gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
gl_FrontColor = gl_Color;
}


void TexMergeVS (void)
{
if (tmTypeVS == 0)
	TexMergeVS0 ();
else if ((tmTypeVS == 1) || (tmTypeVS == 2))
	TexMergeVS12 ();	
else if (tmTypeVS == 3)
	TexMergeVS3 ();	
}
