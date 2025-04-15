#include <glad.h>
#include <glfw3.h>

#include <cglm.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <math.h>
#include <windows.h>

const char *vertexShaderPath = "./include/shaders/triangleVertexShaderSource.glsl";
const char *fragmentShaderPath = "./include/shaders/triangleFragmentShaderSource.glsl";

const float square_color[] = {221.0 / 255, 196.0 / 255, 168.0 / 255};

typedef enum {
    vertex = 1,
    fragment = 2
} ShaderType;

char* read_shader(const char *path){
    FILE *file = fopen(path, "r");

    if (file == NULL)
        puts("Error!");

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* content = (char*)malloc(fileSize + 1);

    int i = 0;
    char symbol;
    while ((symbol = fgetc(file)) != EOF)
        content[i++] = symbol;
    content[i] = '\0';

    // Костыль) Нужен именно const char *
    const char *tmp = content;

    return tmp;
}

GLuint create_shader(const char *source_code, ShaderType type){
    GLuint shader;

    if (type == vertex){
        shader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(shader, 1, &source_code, NULL);
        glCompileShader(shader); 
    }

    else if (type == fragment){
        shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(shader, 1, &source_code, NULL);
        glCompileShader(shader);
    }

    return shader;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        int width, height;
        glfwGetWindowSize(window, &width, &height);

        // Преобразуем в нормализованные координаты (от -1 до 1)
        float norm_x = (xpos / width) * 2.0f - 1.0f;
        float norm_y = 1.0f - (ypos / height) * 2.0f; // перевернутая ось Y

        // Пример: определить клетку 8x8
        int col = (norm_x + 1.0f) / 0.25f; // 0.25 — ширина одной клетки
        int row = (1.0f - norm_y) / 0.25f; // тоже самое по Y

        printf("In NDC: x=%.2f, y=%.2f\n", norm_x, norm_y);
        printf("In coords: x=%d, y=%d\n", xpos, ypos);
        printf("Cell: column = %d, row = %d\n", col, row);
        puts("");
    }
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    // Создание полноэкранного окна
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Reversi", monitor, NULL);
    if (!window) {
        printf("Error with opening the window!\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    //glfwSwapInterval(1);


    gladLoadGL();
    
    glViewport(0, 0, mode->width, mode->height);

    
    char *vertexShaderSource = read_shader(vertexShaderPath);
    GLuint vertexShader = create_shader(vertexShaderSource, vertex);

    char *fragmentShaderSource = read_shader(fragmentShaderPath);
    GLuint fragmentShader = create_shader(fragmentShaderSource, fragment);


    // Объединяем шейдеры в программу
    GLuint shaderprogram = glCreateProgram();

    glAttachShader(shaderprogram, vertexShader);
    glAttachShader(shaderprogram, fragmentShader);
    glLinkProgram(shaderprogram);

    // Шейдеры уже в программе, они нам не нужны
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    
    // С помощью двух треугольников
    GLfloat square_vertices[] = {
        -1.0f, 1.0f, 0.0f,
        -1.0f, 0.75, 0.0f,
        -0.75, 0.75f, 0.0f,
        -0.75f, 1.0f, 0.0f,
    };

    GLfloat line_vertices[] = {
        // x    y     z    type (- = 0, | = 1)
        -1.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 0.0f,

        -1.0f, 1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 1.0f,
    };
    
    GLuint square_indexes[] = {
        0, 1, 2,
        0, 3, 2
    };
    
    GLuint VAO_SQ, VBO_SQ, EBO_SQ;

    // Создаем
    glGenVertexArrays(1, &VAO_SQ);
	glGenBuffers(1, &VBO_SQ);
    glGenBuffers(1, &EBO_SQ);

    // Привязываем, активируем
	glBindVertexArray(VAO_SQ);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_SQ);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_SQ);

    // Заполняем VBO, EBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(square_vertices), square_vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(square_indexes), square_indexes, GL_STATIC_DRAW);

    // Настраиваем VAO, активируем VAO для position = 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Отвязываем VAO для того, чтобы случайно не изменить настройки
    glBindVertexArray(0);


    // Линия x
    GLuint VBO_LI, VAO_LI;

    // Создаем
    glGenVertexArrays(1, &VAO_LI);
	glGenBuffers(1, &VBO_LI);

    // Привязываем, активируем
	glBindVertexArray(VAO_LI);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_LI);

    // Заполняем VBO, EBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(line_vertices), line_vertices, GL_STATIC_DRAW);

    // Настраиваем VAO, активируем VAO для position = 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Отвязываем VAO для того, чтобы случайно не изменить настройки
    glBindVertexArray(0);

    // Ставим callback на левую кнопку мыши
    glfwSetMouseButtonCallback(window, mouse_button_callback);


    unsigned int shift;

    // Uniform переменные
    int colorLoc = glGetUniformLocation(shaderprogram, "sq_color");
    int offsetLoc = glGetUniformLocation(shaderprogram, "offset");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
        // glClear(GL_COLOR_BUFFER_BIT);
        
        glUseProgram(shaderprogram);
        glUniform3fv(colorLoc, 1, square_color);

        glUniform1ui(offsetLoc, shift);
        
        glBindVertexArray(VAO_LI);
        glLineWidth(5.0f);
        glDrawArrays(GL_LINES, 0, 8);
        
        shift++;
        if (shift == 9){
            shift = 0;
        }


        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO_LI);
    glDeleteBuffers(1, &VBO_LI);
   // glDeleteBuffers(1, &EBO_LI);
    glDeleteProgram(shaderprogram); 

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
