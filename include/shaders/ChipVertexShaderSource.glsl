#version 330 core

layout (location = 0) in vec3 aPos;

uniform vec2 center;
uniform float radius;

void main() {
    float offsetX = -1.0 + 0.25 * center.x + 0.125; 
    float offsetY = 1.0 - 0.25 * center.y - 0.125;

    vec2 scaled = aPos.xy * radius;
    vec2 offset = vec2(offsetX, offsetY);

    gl_Position = vec4(scaled + offset, aPos.z, 1.0);
};