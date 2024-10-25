#version 330

in vec2 text;

uniform sampler2D gSampler;

out vec4 FragColor;

void main(void)
{ 
	FragColor = texture2D(gSampler, text);
}
