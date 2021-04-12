#version 420

layout(location = 0) in vec2 inUV;

//struct DirectionalLight
//{
//	//Light direction (defaults to down, to the left, and a little forward)
//	vec4 _lightDirection;
//
//	//Generic Light controls
//	vec4 _lightCol;
//
//	//Ambience controls
//	vec4 _ambientCol;
//	float _ambientPow;
//	
//	//Power controls
//	float _lightAmbientPow;
//	float _lightSpecularPow;
//	
//	float _shadowBias;
//};

struct PointLight
{
	vec4 _lightPos;
	vec4 _lightCol;
	vec4 _ambientCol;

	float _lightLinearFalloff;
	float _lightQuadraticFalloff;
	float _ambientPow;
	float _lightAmbientPow;
	float _lightSpecularPow;
};

layout (std140, binding = 0) uniform u_Lights
{
	PointLight light;
};

layout (binding = 30) uniform sampler2D s_ShadowMap;

layout (binding = 0) uniform sampler2D s_albedoTex;
layout (binding = 1) uniform sampler2D s_normalsTex;
layout (binding = 2) uniform sampler2D s_specularTex;
layout (binding = 3) uniform sampler2D s_positionTex;

layout (binding = 4) uniform sampler2D s_lightAccumTex;

uniform mat4 u_LightSpaceMatrix;
uniform vec3 u_CamPos;
uniform float u_Radius;

out vec4 frag_color;


float ShadowCalculation(vec4 fragPosLightSpace, float bias)
{
	//Perspective division
	vec3 projectionCoordinates = fragPosLightSpace.xyz / fragPosLightSpace.w;
	
	//Transform into a [0,1] range
	projectionCoordinates = projectionCoordinates * 0.5 + 0.5;
	
	//Get the closest depth value from light's perspective (using our 0-1 range)
	float closestDepth = texture(s_ShadowMap, projectionCoordinates.xy).r;

	//Get the current depth according to the light
	float currentDepth = projectionCoordinates.z;

	//Check whether there's a shadow
	float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

	//Return the value
	return shadow;
}

void main() {

	//Albedo
	vec4 textureColor = texture(s_albedoTex, inUV);
	//Normals
	vec3 inNormal = (normalize(texture(s_normalsTex, inUV).rgb) * 2.0) - 1.0;

	//Specular
	float texSpec = texture(s_specularTex, inUV).r;
	//Positions
	vec3 fragPos = texture(s_positionTex, inUV).rgb;
	
	//float dist = length(sun._lightDirection.xyz - fragPos);

	vec3 result;

	for (int i = 0; i < 5; i++)
	{
		vec3 lightDir = normalize(light._lightPos.xyz - fragPos);

		vec3 viewDir  = normalize(u_CamPos - fragPos);
		vec3 h        = normalize(lightDir + viewDir);

		// attenuation
		float dist = length(light._lightPos.xyz - fragPos);
		float attenuation = 1.0 / (1.0 + 0.7 * dist + 1.8 * (dist * dist));

		if(dist < u_Radius)
		{
			// Diffuse
			vec3 N = normalize(inNormal);
			float dif = max(dot(N, lightDir), 0.0);
			vec3 diffuse = dif * light._lightCol.xyz;// add diffuse intensity

			//Specular
			float spec = pow(max(dot(N, h), 0.0), 4.0); // Shininess coefficient (can be a uniform)
			vec3 specular = light._lightSpecularPow * texSpec * spec * light._lightCol.xyz; // Can also use a specular color

			vec4 fragPosLightSpace = u_LightSpaceMatrix * vec4(fragPos, 1.0f);

			float shadow = ShadowCalculation(fragPosLightSpace, 0.005);

			diffuse   *= attenuation;
			specular *= attenuation;

			result = (
				(light._ambientPow * light._ambientCol.xyz * attenuation) + // global ambient light
				(1.0 - shadow) * //Shadow value
				(diffuse + specular));

			if (textureColor.a < 0.31)
			{
				result = vec3(1.0, 1.0, 1.0);
			}
		}
	}

	frag_color = vec4(result, textureColor.a);
}