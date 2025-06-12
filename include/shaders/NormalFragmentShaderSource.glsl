#version 330 core

out vec4 FragColor;
uniform vec3 sq_color;

void main() {
   FragColor = vec4(sq_color, 1.0);
};