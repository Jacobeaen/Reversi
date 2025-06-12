#include <glad.h>
#include <glfw3.h>

#include "text_render.h"
#include "stack.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <math.h>
#include <time.h>

#define SEGMENTS             50
#define CHIP_RADIUS          0.1f
#define SUGGEST_RADIUS       0.02f

#define SIZE                 8

#define PLAYER_MOVE          0
#define WAIT_BEFORE_BOT      1
#define BOT_MOVE             2

int state = PLAYER_MOVE;
double bot_wait_start_time = 0;


bool new_game = false;
bool move_back = false;

float chip_center[2] = {0, 0};
bool clicked = false;

const char *chipVertexShaderPath = "../include/shaders/ChipVertexShaderSource.glsl";
const char *vertexShaderPath = "../include/shaders/NormalVertexShaderSource.glsl";
const char *fragmentShaderPath = "../include/shaders/NormalFragmentShaderSource.glsl";
const char* font_file = "../include/fonts/static/OpenSans-Bold.ttf";

float field_color[] = {221 / 255.0, 196 / 255.0, 168 / 255.0};
float background_color[] = {55 / 255.0, 106 / 255.0, 64 / 255.0};
float grid_color[] = {0.0f, 0.0f, 0.0f};
float user_color[] = {247 / 255.0, 245 / 255.0, 240 / 255.0};
float bot_color[] = {33 / 255.0, 33 / 255.0, 31 / 255.0};
float suggestion_color[] = {8 / 255.0, 161 / 255.0, 236 / 255.0}; 
float *chip_colors[2] = {bot_color, user_color};

typedef enum {
    vertex =    1,
    fragment =  2
} ShaderType;

typedef enum {
    black =  0,
    white =  1,
    clear = -1
} CellCondition;

typedef struct {
    int row;
    int col;
} Move;

CellCondition players[2] = {black, white};

void print_array(moves *);

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

    return content;
}

GLuint create_shader(const char *source_code, ShaderType type){
    GLuint shader;

    if (type == vertex)
        shader = glCreateShader(GL_VERTEX_SHADER);
         
    else if (type == fragment)
        shader = glCreateShader(GL_FRAGMENT_SHADER);
    
    glShaderSource(shader, 1, &source_code, NULL);
    glCompileShader(shader);

    return shader;
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        int width, height;
        glfwGetWindowSize(window, &width, &height);

        // Настройки viewport
        int offsetX = (width - height) / 2 - 250;
        int offsetY = height / 20;
        int viewportWidth = height;
        int viewportHeight = height - 2 * offsetY;

        // Смещаем координаты относительно начала viewport
        xpos -= offsetX;
        ypos -= offsetY;

        if (xpos >= 0 && xpos < viewportWidth && ypos >= 0 && ypos < viewportHeight) {
            // Нормализуем координаты внутри viewport
            float normalizedX = xpos / viewportWidth;
            float normalizedY = ypos / viewportHeight;

            int col = (int)(normalizedX * 8);
            int row = (int)(normalizedY * 8);

            // Глобальная переменная
            chip_center[0] = (float)col;
            chip_center[1] = (float)row;

            clicked = true;

        } 
    }
}

void escape_button_callback(GLFWwindow *window, int key, int scancode, int action, int mods){
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)        
        glfwSetWindowShouldClose(window, true);
}

void create_lines_vertices(float *lines_vertices){
    float step = 0.25f;

    int index = 0;

    float up = 1.0f;
    float down = -1.0f;

    float left = -1.0f;
    float right = 1.0f;

    float x = -1.0f;
    for (int i=0; i < 9; i++){
        lines_vertices[index++] = x + step * i;
        lines_vertices[index++] = up;
        lines_vertices[index++] = 0.0f;
        
        lines_vertices[index++] = x + step * i;
        lines_vertices[index++] = down;
        lines_vertices[index++] = 0.0f;
    }
    
    float y = 1.0f;
    for (int i=0; i < 9; i++){
        lines_vertices[index++] = left;
        lines_vertices[index++] = y - step * i;
        lines_vertices[index++] = 0.0f;
        
        lines_vertices[index++] = right;
        lines_vertices[index++] = y - step * i;
        lines_vertices[index++] = 0.0f;
    }
}

void create_circle_vertices(GLfloat *vertices, int segments) {
    float centerX = 0.0f;
    float centerY = 0.0f;

    vertices[0] = centerX;
    vertices[1] = centerY;
    vertices[2] = 0.0f;

    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;

        float x = cosf(angle);
        float y = sinf(angle);

        vertices[3 * (i + 1) + 0] = x;
        vertices[3 * (i + 1) + 1] = y;
        vertices[3 * (i + 1) + 2] = 0.0f;
    }
}

void create_start_board(int board[][8]){
    for (int i = 0; i < SIZE; i++){
        for (int j = 0; j < SIZE; j++){
            board[i][j] = clear;

            if (i == 3 && j == 3 || i == 4 && j == 4)
                board[i][j] = white;

            else if (i == 3 && j == 4 || i == 4 && j == 3)
                board [i][j] = black;
        }
    }
}

bool is_valid_move(int board[][SIZE], int row, int col, int player){
    if (board[row][col] != clear)
        return false;

    int opponent = 1 - player;

    // Выбираем направление
    int direction[8][2] = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1},
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
    };

    for (int d = 0; d < 8; d++) {
        int dx = direction[d][0];
        int dy = direction[d][1];

        int x = row + dx;
        int y = col + dy;

        bool found_opponent = false;

        
        while (x >= 0 && x < SIZE && y >= 0 && y < SIZE) {
            if (board[x][y] == opponent) 
                found_opponent = true;
            else if (board[x][y] == player) {
                if (found_opponent)
                    return true;
                break;
            }
            else 
                break;
            

            x += dx;
            y += dy;
        }
    }

    return false;
}

void update_board(int board[][SIZE], int row, int col, int player, moves *last_moves){
    board[row][col] = player;

    int opponent = 1 - player;

    int directions[8][2] = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1},  
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1}  
    };


    // dx и dy пробегаются по directions
    for (int d = 0; d < 8; d++) {
        int dx = directions[d][0];
        int dy = directions[d][1];

        int x = row + dx;
        int y = col + dy;

        bool found_opponent = false;

        while (x >= 0 && x < SIZE && y >= 0 && y < SIZE) {
            if (board[x][y] == opponent)
                found_opponent = true; 
            
            else if (board[x][y] == player) {
                if (found_opponent) {

                    int temp_x = row + dx;
                    int temp_y = col + dy;

                    while (temp_x != x || temp_y != y) {
                        board[temp_x][temp_y] = player;

                        last_moves->cell_x[last_moves->size] = temp_x;
                        last_moves->cell_y[last_moves->size] =   temp_y;
                        last_moves->size++;

                        temp_x += dx;
                        temp_y += dy;
                    }
                    break;

                }
                break;
            }

            else 
                break;            

            x += dx;
            y += dy;
        }
    }
}

void get_score(int board[][SIZE], int *black_chips, int *white_chips){
    for (int i=0; i < SIZE; i++){
        for (int j=0; j < SIZE; j++){
            if (board[i][j] == white)
                (*white_chips)++;
            if (board[i][j] == black)
                (*black_chips)++;
        }
    }
}

bool has_valid_moves(int board[SIZE][SIZE], int player) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (is_valid_move(board, i, j, player)) {
                return true;
            }
        }
    }
    return false;
}

bool is_game_over(int board[SIZE][SIZE]) {
    return !has_valid_moves(board, 0) && !has_valid_moves(board, 1);
}

moves* new_array(){
    moves *tmp = (moves *)malloc(sizeof(moves));
    tmp->size = 0;

    return tmp;
}

void go_backward(int board[][SIZE], moves last_moves){
    for (int i=0; i < last_moves.size; i++){
        int x = last_moves.cell_x[i];
        int y = last_moves.cell_y[i];

        CellCondition cell_value = board[x][y];

        if (i == 0)
            board[x][y] = clear;

        else if (cell_value == black)
            board[x][y] = white;

        else if (cell_value == white)   
            board[x][y] = black;
    }
}

// Простая оценочная функция: разница фишек бота и игрока
int evaluate(int board[SIZE][SIZE]) {
    int score = 0;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == white)
                score++;
            else if (board[i][j] == black)
                score--;
        }
    }

    return score;
}

int minimax(int board[SIZE][SIZE], int depth, bool maximizingPlayer) {
    if (depth == 0 || is_game_over(board)) {
        return evaluate(board);
    }

    int bestScore = maximizingPlayer ? INT_MIN : INT_MAX;
    int player = maximizingPlayer ? 1 : 0;

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (is_valid_move(board, i, j, player)) {
                int temp_board[SIZE][SIZE];
                for (int x = 0; x < SIZE; x++)
                    for (int y = 0; y < SIZE; y++)
                        temp_board[x][y] = board[x][y];

                moves dummy_moves = { .size = 0 };
                update_board(temp_board, i, j, player, &dummy_moves);

                int score = minimax(temp_board, depth - 1, !maximizingPlayer);

                if (maximizingPlayer) {
                    if (score > bestScore) bestScore = score;
                } else {
                    if (score < bestScore) bestScore = score;
                }
            }
        }
    }
    return bestScore;
}

Move find_best_move(int board[SIZE][SIZE], int depth) {
    int bestScore = INT_MIN;
    Move bestMove = {-1, -1};

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (is_valid_move(board, i, j, 1)) { // бот — белый (1)
                int temp_board[SIZE][SIZE];
                for (int x = 0; x < SIZE; x++)
                    for (int y = 0; y < SIZE; y++)
                        temp_board[x][y] = board[x][y];

                moves dummy_moves = { .size = 0 };
                update_board(temp_board, i, j, 1, &dummy_moves);

                int score = minimax(temp_board, depth - 1, false);

                if (score > bestScore) {
                    bestScore = score;
                    bestMove.row = i;
                    bestMove.col = j;
                }
            }
        }
    }
    return bestMove;
}


void print_array(moves *last_moves){
    puts("Hello");
    for (int i=0; i < last_moves->size; i++)
        printf("cell: %d %d\n", last_moves->cell_x[i], last_moves->cell_y[i]);
    
    puts("");
}

void print_board(int board[][SIZE], char *string){
    printf("board %s:\n");
    for (int i=0; i < SIZE; i++){
        for (int j=0; j < SIZE; j++){
            printf("%d\t", board[i][j]);
        }
        puts("");
    }
    printf("\n\n");
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Reversi", monitor, NULL);
    if (!window) {
        printf("Error with opening the window!\n");
        glfwTerminate();
        return -1;
    }

    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, escape_button_callback);
    
    glfwMakeContextCurrent(window);
    gladLoadGL();
    
    float font_pixel_size = 32.0f;    
    if (!init_pretty_text_renderer(window, font_file, font_pixel_size)) {
        fprintf(stderr, "ПИЗДЕЦ! ERROR ERROR. Не удалось инициализировать рендерер текста!\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    

    int offsetX = (mode->width - mode->height) / 2 - 250;
    int offsetY = mode->height / 20;
    int viewportWidth = mode->height; 
    int viewportHeight = mode->height - 2 * offsetY;  

    int secondOffsetX = offsetX + viewportWidth + 50;
    int marginRight = 50;
    int secondViewportWidth = mode->width - secondOffsetX - marginRight;
    
    glViewport(offsetX, offsetY, viewportWidth, viewportHeight);


    // Шейдеры и программа
    char *vertexShaderSourceNormal = read_shader(vertexShaderPath);
    GLuint vertexShaderNormal = create_shader(vertexShaderSourceNormal, vertex);

    char *fragmentShaderSource = read_shader(fragmentShaderPath);
    GLuint fragmentShader = create_shader(fragmentShaderSource, fragment);

    char *vertexShaderSourceChip = read_shader(chipVertexShaderPath);
    GLuint vertexShaderChip = create_shader(vertexShaderSourceChip, vertex);

    GLuint shaderProgramNormal = glCreateProgram();
    GLuint shaderProgramChip = glCreateProgram();
    
    // Обычная
    glAttachShader(shaderProgramNormal, vertexShaderNormal);
    glAttachShader(shaderProgramNormal, fragmentShader);
    glLinkProgram(shaderProgramNormal);
    
    // Со смещением фишки
    glAttachShader(shaderProgramChip, vertexShaderChip);
    glAttachShader(shaderProgramChip, fragmentShader);
    glLinkProgram(shaderProgramChip);

    glDeleteShader(vertexShaderNormal);
    glDeleteShader(vertexShaderChip);
    glDeleteShader(fragmentShader);

    free(vertexShaderSourceNormal);
    free(vertexShaderSourceChip);
    free(fragmentShaderSource);

    // Фишки
    GLfloat circle_vertices[(SEGMENTS + 2) * 3];
    create_circle_vertices(circle_vertices, SEGMENTS);

    GLuint VAO_CI, VBO_CI;
    glGenVertexArrays(1, &VAO_CI);
    glGenBuffers(1, &VBO_CI);

    glBindVertexArray(VAO_CI);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_CI);
    glBufferData(GL_ARRAY_BUFFER, sizeof(circle_vertices), circle_vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    //


    // Поле
    GLfloat lines_vertices[9 * 6 * 2];
    create_lines_vertices(lines_vertices);
    
    GLuint VAO_LI, VBO_LI;
    glGenVertexArrays(1, &VAO_LI);
	glGenBuffers(1, &VBO_LI);

	glBindVertexArray(VAO_LI);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_LI);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lines_vertices), lines_vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    //


    // Задний фон поля
    GLfloat rectangle_vertices[] = {
        -1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
    };

    GLuint rectangle_indexes[] = {
        0, 1, 2,
        0, 3, 2
    };

    GLuint VAO_RE, VBO_RE, EBO_RE;
    glGenVertexArrays(1, &VAO_RE);
    glGenBuffers(1, &VBO_RE);
    glGenBuffers(1, &EBO_RE);

    glBindVertexArray(VAO_RE);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_RE);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_RE);

    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangle_vertices), rectangle_vertices, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rectangle_indexes), rectangle_indexes, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    //


    int colorLocNormal = glGetUniformLocation(shaderProgramNormal, "sq_color");

    int colorLocChip = glGetUniformLocation(shaderProgramChip, "sq_color");
    int centerLoc = glGetUniformLocation(shaderProgramChip, "center");
    int radiusLoc = glGetUniformLocation(shaderProgramChip, "radius");
    

    // Для панели Nuklear
    float panel_x = (float)secondOffsetX;
    float panel_y = (float)offsetY;
    float panel_width = (float)secondViewportWidth;
    float panel_height = (float)viewportHeight;
    
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    //
    
    Stack *stack_moves = NULL;

    int board[SIZE][SIZE];
    create_start_board(board);
    
    int turn = 0;
    int taken_moves = 0; 
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();        

        // Отрисовка боковой панели
        glViewport(0, 0, width, height);

        begin_text_frame();
        PanelRect info_panel_data = {panel_x, panel_y + panel_height / 4, panel_width, panel_height - panel_height / 2};
        
        int blacks = 0, whites = 0;
        get_score(board, &blacks, &whites);
        draw_info_panel(info_panel_data, turn, blacks, whites, is_game_over(board));
        //


        // Заливаем экранннннннннннннннннннннннннннннн
        glClearColor(background_color[0], background_color[1], background_color[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        

        // Начало отрисовки поля
        glViewport(offsetX, offsetY, viewportWidth, viewportHeight);

        
        // Кнопка новой игры
        if (new_game){
            create_start_board(board);
            
            
            stack_moves = clear_stack(stack_moves);
            turn = 0;
            taken_moves = 0;
            state = PLAYER_MOVE;
            new_game = false;
        }

        // Кнопка возврата хода
        if (move_back && taken_moves > 0){
            printf("Taken moves: %d\n", taken_moves);
            moves last_move;

            for (int i = 0; i < 2; i++) {
                stack_moves = pop(stack_moves, &last_move);
                go_backward(board, last_move);

                taken_moves--;
            }
 
            move_back = false;
        }


        // Задний фон поля
        glUseProgram(shaderProgramNormal);
        glUniform3fv(colorLocNormal, 1, field_color);
        glBindVertexArray(VAO_RE);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        //


        // Cетка
        glUseProgram(shaderProgramNormal);
        glBindVertexArray(VAO_LI);
        glUniform3fv(colorLocNormal, 1, grid_color);

        glLineWidth(5.0f);
        glDrawArrays(GL_LINES, 0, 54 * 2);
        //


        // Фишки
        glUseProgram(shaderProgramChip);
        glBindVertexArray(VAO_CI);


        // Отрисовка поля на данный ход
        for (int i = 0; i < SIZE; i++){
            for (int j = 0; j < SIZE; j++){

                float current_chip_center[] = {j, i};

                if (board[i][j] != clear){
                    glUniform1f(radiusLoc, CHIP_RADIUS);
                    glUniform2fv(centerLoc, 1, current_chip_center);
                    glUniform3fv(colorLocChip, 1, chip_colors[board[i][j]]);
                    glDrawArrays(GL_TRIANGLE_FAN, 0, SEGMENTS + 2);
                }

                if (state == PLAYER_MOVE){
                    if (is_valid_move(board, i, j, turn % 2)){
                        glUniform1f(radiusLoc, SUGGEST_RADIUS);
                        glUniform3fv(colorLocChip, 1, suggestion_color);
                        glUniform2fv(centerLoc, 1, current_chip_center);
                        glDrawArrays(GL_TRIANGLE_FAN, 0, SEGMENTS + 2);
                    }
                }
            }
        }

        // Для пользователя
        if (state == PLAYER_MOVE && turn % 2 == black){
            if (clicked){
                if (is_valid_move(board, (int)chip_center[1], (int)chip_center[0], turn % 2)){
    
                    moves *last_move = new_array();
                    last_move->cell_x[last_move->size] = (int)chip_center[1];
                    last_move->cell_y[last_move->size] = (int)chip_center[0];
                    last_move->size++;
    
                    update_board(board, (int)chip_center[1], (int)chip_center[0], turn % 2, last_move);
                    state = WAIT_BEFORE_BOT;
                    bot_wait_start_time = glfwGetTime();

                    
                    stack_moves = push(stack_moves, last_move);
                    print_array(last_move);
                    
                    taken_moves++;
                    turn++;
                    clicked = false;
                }
                
                else{
                    render_text_frame();
                    glfwSwapBuffers(window);
                    
                    continue;
                }
            }
        }
        
        else if (state == WAIT_BEFORE_BOT) {
            if (glfwGetTime() - bot_wait_start_time >= 0.5) {
                state = BOT_MOVE;
            }
        }

        else if (state == BOT_MOVE && turn % 2 == white){
            Move best_bot_move = find_best_move(board, 4);
            
            if (best_bot_move.row != -1){
                int x = best_bot_move.row;
                int y = best_bot_move.col;
                
                moves *last_move = new_array();
                last_move->cell_x[last_move->size] = chip_center[1] = x;
                last_move->cell_y[last_move->size] = chip_center[0] = y;
                last_move->size++;
                
                update_board(board, (int)chip_center[1], (int)chip_center[0], turn % 2, last_move);
                state = PLAYER_MOVE;
                
                stack_moves = push(stack_moves, last_move);
                print_array(last_move);
                
                
                printf("Best move: %d %d\n", best_bot_move.row, best_bot_move.col);
                
                turn++;
                taken_moves++;
            }
            else 
                puts("No best moves!");   
        }

        if (is_game_over(board))
            printf("Player %d won!", players[turn % 2]);
        else {
            if (!has_valid_moves(board, turn % 2)) {
                printf("Player %d skips his turn!\n", turn % 2 + 1);
                turn++; 

                if (turn % 2 == black)
                    state = PLAYER_MOVE;
                else
                    state = WAIT_BEFORE_BOT, bot_wait_start_time = glfwGetTime();
            }
        }


    render_text_frame();
    glfwSwapBuffers(window);
    
    }

    glDeleteVertexArrays(1, &VAO_LI);
    glDeleteBuffers(1, &VBO_LI);
    glDeleteBuffers(1, &EBO_RE);

    glDeleteProgram(shaderProgramNormal); 
    glDeleteProgram(shaderProgramChip);



    clear_stack(stack_moves);
    cleanup_pretty_text_renderer();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
