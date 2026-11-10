uniform mat4 u_Matrix;//±ä»»¾ØÕó  
uniform mat4 u_MVPMatrix; //×Ü±ä»»¾ØÕó  


attribute vec3 a_Position;
attribute vec3 a_Normal;
attribute vec2 a_TextureCoord;
//attribute vec4 a_Color;


varying vec3 v_FragPosition;
varying vec3 v_Normal;
varying vec2 v_TextureCoord;
//varying vec4 v_Color;     		

void main()                    
{
	//v_Color = a_Color;
    v_TextureCoord = a_TextureCoord;  
    v_Normal = a_Normal;      
    v_FragPosition = vec3(u_Matrix * vec4(a_Position,1.0)); 

                
    gl_Position = u_MVPMatrix *  vec4(a_Position,1);
    gl_PointSize = 10.0;

}   