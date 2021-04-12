#version 420

layout(location = 0) in vec2 inUV;

out vec4 frag_color;
  
uniform sampler2D hdrBuffer;
uniform float u_exposure;

void main()
{             
    const float gamma = 2.2;
    vec3 hdrColor = texture(hdrBuffer, inUV).rgb;

        // exposure
        vec3 result = vec3(1.0) - exp(-hdrColor * u_exposure);
        // also gamma correct while we're at it       
        result = pow(result, vec3(1.0 / gamma));
        frag_color = vec4(result, 1.0);

}