#version 120 

attribute vec3 inVertex;
attribute vec3 inNormal;

uniform mat4 modelingMat;
uniform mat4 modelingMatInvTr;
uniform mat4 perspectiveMat;




void main(void)
{	
    gl_Position = perspectiveMat * modelingMat * vec4(inVertex, 1);
}

