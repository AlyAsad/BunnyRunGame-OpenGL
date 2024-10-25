#version 330

layout (location = 0) in vec3 inVertex;
layout (location = 1) in vec2 TexCoord;

uniform mat4 modelingMat;
uniform mat4 modelingMatInvTr;
uniform mat4 perspectiveMat;

out vec2 text;


void main(void)
{
    gl_Position = perspectiveMat * modelingMat * vec4(inVertex, 1);
    text = TexCoord;
}

