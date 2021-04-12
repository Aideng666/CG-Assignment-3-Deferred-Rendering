#version 420

layout(location = 0) in vec2 inUV;

out vec4 frag_color;

layout(binding = 0) uniform sampler2D s_ScreenTex;

uniform float u_PixelSize = 8.0;
uniform vec2 u_WindowSize;

void main()
{
	//Create new more pixelated UV 
	vec2 uv = floor(gl_FragCoord.xy / u_PixelSize + 0.5) * u_PixelSize;
	//Load in the source image with new uvs
	vec4 source = texture(s_ScreenTex, uv / u_WindowSize);

	//Output the newly pixelated image
	frag_color.rgb = source.rgb;
	frag_color.a = 1.0f;
}