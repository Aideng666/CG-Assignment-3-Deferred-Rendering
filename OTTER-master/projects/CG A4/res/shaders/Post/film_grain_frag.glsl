#version 420

layout(location = 0) in vec2 inUV;

out vec4 frag_color;

layout(binding = 0) uniform sampler2D s_screenTex;

uniform float u_Strength = 32.0;
uniform float u_Time;
uniform vec2 u_WindowSize;

void main()
{
	//Create new more pixelated UV
	vec2 uv = gl_FragCoord.xy / u_WindowSize;
	//Load in the source image with new uvs
	vec4 source = texture(s_screenTex, uv);

	//Scrolls the uvs used to create the grain
	float x = (uv.x + 4.0) * (uv.y + 4.0) * (u_Time * 10.0);
	//Creates the grain
	vec4 grain = vec4(mod((mod(x, 13.0) + 1.0) * (mod(x, 123.0) + 1.0), 0.01) - 0.005) * u_Strength;

	//Apply the grain to the image
	grain = 1.0 - grain;

	frag_color.rgb = source.rgb * grain.rgb;
	frag_color.a = 1.0f;
}