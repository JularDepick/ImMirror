precision mediump float; 							
//varying vec4 v_Color; 


varying vec3 v_FragPosition;
varying vec3 v_Normal;
varying vec2 v_TextureCoord; //���������ױ����  ��λ

uniform sampler2D u_Texture; //���������,����һ������        	   								
uniform vec3 u_LightLocation; 
  
void main()                    		
{    
    vec3 normal = normalize(v_Normal); 
    vec3 lightDir = normalize(u_LightLocation-v_FragPosition);
    float factor = max(0.0, dot(normal, lightDir));
    vec4 diffuse = factor * vec4(1.0,1.0,1.0,1.0);                         	
    //gl_FragColor = v_Color; 
    //gl_FragColor = texture2D(u_Texture, v_TextureCoord);   
    //gl_FragColor = (diffuse + vec4(0.6,0.6,0.6,1))*texture2D(u_Texture, vec2(v_TextureCoord.s,v_TextureCoord.t));
    gl_FragColor = texture2D(u_Texture, vec2(v_TextureCoord.s,v_TextureCoord.t));
}
