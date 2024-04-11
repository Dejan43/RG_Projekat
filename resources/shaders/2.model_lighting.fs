#version 330 core
out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

struct PointLight {
    vec3 position;
    vec3 color;

    vec3 specular;
    vec3 diffuse;
    vec3 ambient;

    float constant;
    float linear;
    float quadratic;
};

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;

    float shininess;
};
in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

struct DirLight{
    vec3 direction;
    vec3 specular;
    vec3 diffuse;
    vec3 ambient;

};
struct SpotLight {
    bool turnOn;
    vec3 position;
    vec3 direction;
    vec3 color;

    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform PointLight pointLight;
uniform Material material;
uniform DirLight dirLight;
uniform SpotLight spotLight;

uniform vec3 viewPosition;
// calculates the color when using a point light.

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);

    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);

    // specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);

    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords));
    ambient *= attenuation;
    diffuse *= attenuation * diff;
    specular *= attenuation;
    return (ambient + diffuse + specular) * light.color;
}

// calculates the color when using a direct light with blinn.

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{

    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);
    // combine results
    vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords));
    return (ambient +diffuse + specular );
}
// calculates the color when using a spot light.
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    if (light.turnOn){
        vec3 lightDir = normalize(light.position - fragPos);
        // diffuse shading
        float diff = max(dot(normal, lightDir), 0.0);
        // specular shading
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        // attenuation
        float distance = length(light.position - fragPos);
        float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
        // spotlight intensity
        float theta = dot(lightDir, normalize(-light.direction));
        float epsilon = light.cutOff - light.outerCutOff;
        float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
        // combine results

        vec3 ambient = light.ambient * vec3(texture(material.texture_diffuse1, TexCoords));
        vec3 diffuse = light.diffuse * diff * vec3(texture(material.texture_diffuse1, TexCoords));
        vec3 specular = light.specular * spec * vec3(texture(material.texture_specular1, TexCoords));
        ambient *= attenuation * intensity;
        diffuse *= attenuation * intensity;
        specular *= attenuation * intensity;
        return (ambient + diffuse + specular) * light.color;
    }
    else
        return vec3(0);
}
void main()
{
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(viewPosition - FragPos);
    vec3 result = vec3(0, 0, 0);
    result += CalcDirLight(dirLight, normal, FragPos, viewDir);
    result += CalcPointLight(pointLight, normal, FragPos, viewDir);
    result += CalcSpotLight(spotLight, normal, FragPos, viewDir);

    FragColor = vec4(result, 1.0);
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
        if(brightness > 1.0)
            BrightColor = vec4(FragColor.rgb, 1.0);
        else
        	BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}