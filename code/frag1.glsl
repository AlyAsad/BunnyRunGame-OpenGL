#version 120

uniform bool isOdd;

void main(void)
{
	vec4 c;
	if (isOdd) {
		c = vec4(0.2, 0.2, 0.8, 1);
	}
	else {
    	c = vec4(0.05, 0.05, 0.2, 1);
    }
    
    gl_FragColor = c;
}
