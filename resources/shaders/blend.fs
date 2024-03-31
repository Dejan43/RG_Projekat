#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

// texture samplers
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform sampler2D texture3;
uniform sampler2D texture4;
uniform sampler2D texture5;
uniform sampler2D texture6;
uniform int pair;
uniform bool side;
void main()
{
	// linearly interpolate between both textures (80% container, 20% awesomeface)
	if(side)
	FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
	else{
	    if(pair == 1)
	        FragColor = mix(texture(texture1, TexCoord), texture(texture3, TexCoord), 0.5);
	    if(pair == 2)
            FragColor = mix(texture(texture1, TexCoord), texture(texture4, TexCoord), 0.5);
        if(pair == 3)
            FragColor = mix(texture(texture1, TexCoord), texture(texture5, TexCoord), 0.5);
        if(pair == 4)
       	    FragColor = mix(texture(texture1, TexCoord), texture(texture6, TexCoord), 0.5);
	}
}