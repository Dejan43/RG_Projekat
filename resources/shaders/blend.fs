#version 330 core
out vec4 FragColor;

layout (location = 1) out vec4 BrightColor;

in vec2 TexCoord;

// texture samplers
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;
uniform sampler2D texture4;
uniform sampler2D texture5;
uniform int pair;
uniform bool side;
void main()
{

	if(side)
	FragColor = texture(texture1, TexCoord);
	else{
	    if(pair == 1)
	        FragColor = mix(texture(texture1, TexCoord), texture(texture3, TexCoord), 0.7);
	    if(pair == 2)
            FragColor = mix(texture(texture1, TexCoord), texture(texture4, TexCoord), 0.7);
        if(pair == 3)
            FragColor = mix(texture(texture1, TexCoord), texture(texture5, TexCoord), 0.7);
        if(pair == 4)
       	    FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.7);
	}
	float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
        if(brightness > 1.0)
            BrightColor = vec4(FragColor.rgb, 1.0);
        else
        	BrightColor = vec4(0.0, 0.0, 0.0, 1.0);

}