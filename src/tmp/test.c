#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

#include <math.h>

// Vertex Shader source code
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"}\0";
//Fragment Shader source code
const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(0.8f, 0.3f, 0.02f, 1.0f);\n"
"}\n\0";

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Создание полноэкранного окна
    GLFWwindow* window = glfwCreateWindow(800, 800, "Reversi", NULL, NULL);
    if (!window) {
        printf("Error with opening the window!\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    gladLoadGL();
    
    glViewport(0, 0, 800, 800);

    

    // Шейдер вершин. (Трансофрмирует нормализованную запись в координаты на экране)
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader); 

    // Шейдер фрагментов. (Дает цвет пикселям)
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Объединяем шейдеры в программу
    GLuint shaderprogram = glCreateProgram();

    glAttachShader(shaderprogram, vertexShader);
    glAttachShader(shaderprogram, fragmentShader);
    glLinkProgram(shaderprogram);

    // Шейдеры уже в программе, они нам не нужны
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);


    GLfloat triangle_vertices[] = {
        0.0f,  0.5f, 0.0f, 
       -0.5f, -0.5f, 0.0f,  
        0.5f, -0.5f, 0.0f   
   };

   GLuint VAO, VBO;

	
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);


	glBindVertexArray(VAO);


	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_vertices), triangle_vertices, GL_STATIC_DRAW);

	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


    while (!glfwWindowShouldClose(window)) {
        
        glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(shaderprogram);
        glBindVertexArray(VAO);
        
        glDrawArrays(GL_TRIANGLES, 0, 3);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderprogram); 

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
