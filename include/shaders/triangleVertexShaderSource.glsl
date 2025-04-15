#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in float type;

uniform uint offset;

void main() {
    float step = 0.25;

    // Если линия горизонтальная
    if (type == 0){
        gl_Position = vec4(aPos.x, aPos.y - offset * step, aPos.z, 1.0);
    }

    // Вертикальная
    else if (type == 1){
        gl_Position = vec4(aPos.x + offset * step, aPos.y, aPos.z, 1.0);
    }
};