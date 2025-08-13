#include <glad.h>
#include <glfw3.h>

#include "stack.h"
#include "text_render.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <math.h>
#include <string.h>
#include <time.h>

#define SEGMENTS 50
#define CHIP_RADIUS 0.1f
#define SUGGEST_RADIUS 0.02f

#define SIZE 8

#define PLAYER_MOVE 0
#define WAIT_BEFORE_BOT 1
#define BOT_MOVE 2

#define PLAYER_HUMAN 0 // Черные (пользователь)
#define PLAYER_BOT 1   // Белые (бот)

int state = PLAYER_MOVE;
double bot_wait_start_time = 0;

bool new_game = true;
bool move_back = false;

float chip_center[2] = {0, 0};
bool clicked = false;

Difficulty g_current_difficulty = DIFFICULTY_MEDIUM;

const char *chipVertexShaderPath =
    "../include/shaders/ChipVertexShaderSource.glsl";
const char *vertexShaderPath =
    "../include/shaders/NormalVertexShaderSource.glsl";
const char *fragmentShaderPath =
    "../include/shaders/NormalFragmentShaderSource.glsl";
const char *font_file = "../include/fonts/static/OpenSans-Bold.ttf";

float field_color[] = {221 / 255.0, 196 / 255.0, 168 / 255.0};
float background_color[] = {55 / 255.0, 106 / 255.0, 64 / 255.0};
float grid_color[] = {0.0f, 0.0f, 0.0f};
float user_color[] = {247 / 255.0, 245 / 255.0, 240 / 255.0};
float bot_color[] = {33 / 255.0, 33 / 255.0, 31 / 255.0};
float suggestion_color[] = {8 / 255.0, 161 / 255.0, 236 / 255.0};
float *chip_colors[2] = {bot_color, user_color};

typedef enum { vertex = 1, fragment = 2 } ShaderType;

typedef enum { black = 0, white = 1, clear = -1 } CellCondition;

typedef struct
{
   int row;
   int col;
} Move;

CellCondition players[2] = {black, white};

char *read_shader(const char *path)
{
   FILE *file = fopen(path, "r");

   if (file == NULL)
      puts("Error!");

   fseek(file, 0, SEEK_END);
   long fileSize = ftell(file);
   fseek(file, 0, SEEK_SET);

   char *content = (char *)malloc(fileSize + 1);

   int i = 0;
   char symbol;
   while ((symbol = fgetc(file)) != EOF)
      content[i++] = symbol;
   content[i] = '\0';

   return content;
}

GLuint create_shader(const char *source_code, ShaderType type)
{
   GLuint shader;

   if (type == vertex)
      shader = glCreateShader(GL_VERTEX_SHADER);

   else if (type == fragment)
      shader = glCreateShader(GL_FRAGMENT_SHADER);

   glShaderSource(shader, 1, &source_code, NULL);
   glCompileShader(shader);

   return shader;
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
   if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
   {
      double xpos, ypos;
      glfwGetCursorPos(window, &xpos, &ypos);

      int width, height;
      glfwGetWindowSize(window, &width, &height);

      
      int x1 = (width - height) / 2 - 250;
      int y1 = height / 20;
      int x2 = height;
      int y2 = height - 2 * y1;

      
      xpos -= x1;
      ypos -= y1;

      if (xpos >= 0 && xpos < x2 && ypos >= 0 && ypos < y2)
      {
         
         float normalizedX = xpos / x2;
         float normalizedY = ypos / y2;

         int col = (int)(normalizedX * 8);
         int row = (int)(normalizedY * 8);

         
         chip_center[0] = (float)col;
         chip_center[1] = (float)row;

         clicked = true;
      }
   }
}

void escape_button_callback(GLFWwindow *window, int key, int scancode,
                            int action, int mods)
{
   if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);
}

void create_lines_vertices(float *lines_vertices)
{
   float step = 0.25f;

   int index = 0;

   float up = 1.0f;
   float down = -1.0f;

   float left = -1.0f;
   float right = 1.0f;

   float x = -1.0f;
   for (int i = 0; i < 9; i++)
   {
      lines_vertices[index++] = x + step * i;
      lines_vertices[index++] = up;
      lines_vertices[index++] = 0.0f;

      lines_vertices[index++] = x + step * i;
      lines_vertices[index++] = down;
      lines_vertices[index++] = 0.0f;
   }

   float y = 1.0f;
   for (int i = 0; i < 9; i++)
   {
      lines_vertices[index++] = left;
      lines_vertices[index++] = y - step * i;
      lines_vertices[index++] = 0.0f;

      lines_vertices[index++] = right;
      lines_vertices[index++] = y - step * i;
      lines_vertices[index++] = 0.0f;
   }
}

void create_circle_vertices(GLfloat *vertices, int segments)
{
   float centerX = 0.0f;
   float centerY = 0.0f;

   vertices[0] = centerX;
   vertices[1] = centerY;
   vertices[2] = 0.0f;

   for (int i = 0; i <= segments; i++)
   {
      float angle = 2.0f * M_PI * i / segments;

      float x = cosf(angle);
      float y = sinf(angle);

      vertices[3 * (i + 1) + 0] = x;
      vertices[3 * (i + 1) + 1] = y;
      vertices[3 * (i + 1) + 2] = 0.0f;
   }
}

void create_start_board(int board[][8])
{
   for (int i = 0; i < SIZE; i++)
   {
      for (int j = 0; j < SIZE; j++)
      {
         board[i][j] = clear;

         if (i == 3 && j == 3 || i == 4 && j == 4)
            board[i][j] = white;

         else if (i == 3 && j == 4 || i == 4 && j == 3)
            board[i][j] = black;
      }
   }
}

bool is_valid_move(int board[][SIZE], int row, int col, int player)
{
   if (board[row][col] != clear)
      return false;

   int opponent = 1 - player;

   
   int direction[8][2] = {{-1, 0},  {1, 0},  {0, -1}, {0, 1},
                          {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};

   for (int d = 0; d < 8; d++)
   {
      int dx = direction[d][0];
      int dy = direction[d][1];

      int x = row + dx;
      int y = col + dy;

      bool found_opponent = false;

      while (x >= 0 && x < SIZE && y >= 0 && y < SIZE)
      {
         if (board[x][y] == opponent)
            found_opponent = true;
         else if (board[x][y] == player)
         {
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

void update_board(int board[][SIZE], int row, int col, int player,
                  moves *last_moves)
{
   board[row][col] = player;

   int opponent = 1 - player;

   int directions[8][2] = {{-1, 0},  {1, 0},  {0, -1}, {0, 1},
                           {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};

   
   for (int d = 0; d < 8; d++)
   {
      int dx = directions[d][0];
      int dy = directions[d][1];

      int x = row + dx;
      int y = col + dy;

      bool found_opponent = false;

      while (x >= 0 && x < SIZE && y >= 0 && y < SIZE)
      {
         if (board[x][y] == opponent)
            found_opponent = true;

         else if (board[x][y] == player)
         {
            if (found_opponent)
            {
               int temp_x = row + dx;
               int temp_y = col + dy;

               while (temp_x != x || temp_y != y)
               {
                  board[temp_x][temp_y] = player;

                  if (last_moves != NULL)
                  {
                     last_moves->cell_x[last_moves->size] = temp_x;
                     last_moves->cell_y[last_moves->size] = temp_y;
                     last_moves->size++;
                  }
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

void get_score(int board[][SIZE], int *black_chips, int *white_chips)
{
   for (int i = 0; i < SIZE; i++)
   {
      for (int j = 0; j < SIZE; j++)
      {
         if (board[i][j] == white)
            (*white_chips)++;
         if (board[i][j] == black)
            (*black_chips)++;
      }
   }
}

bool has_valid_moves(int board[SIZE][SIZE], int player)
{
   for (int i = 0; i < SIZE; i++)
   {
      for (int j = 0; j < SIZE; j++)
      {
         if (is_valid_move(board, i, j, player))
         {
            return true;
         }
      }
   }
   return false;
}

bool is_game_over(int board[SIZE][SIZE])
{
   return !has_valid_moves(board, 0) && !has_valid_moves(board, 1);
}

moves *new_array()
{
   moves *tmp = (moves *)malloc(sizeof(moves));
   tmp->size = 0;

   return tmp;
}

void go_backward(int board[][SIZE], moves last_moves)
{
   for (int i = 0; i < last_moves.size; i++)
   {
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

int evaluate_board(int board[SIZE][SIZE])
{
   const int weights[8][8] = {{120, -20, 20, 5, 5, 20, -20, 120},
                              {-20, -40, -5, -5, -5, -5, -40, -20},
                              {20, -5, 15, 3, 3, 15, -5, 20},
                              {5, -5, 3, 3, 3, 3, -5, 5},
                              {5, -5, 3, 3, 3, 3, -5, 5},
                              {20, -5, 15, 3, 3, 15, -5, 20},
                              {-20, -40, -5, -5, -5, -5, -40, -20},
                              {120, -20, 20, 5, 5, 20, -20, 120}};

   int bot_score = 0;
   int human_score = 0;

   for (int i = 0; i < SIZE; i++)
   {
      for (int j = 0; j < SIZE; j++)
      {
         if (board[i][j] == PLAYER_BOT)
         {
            bot_score += weights[i][j];
         }
         else if (board[i][j] == PLAYER_HUMAN)
         {
            human_score += weights[i][j];
         }
      }
   }
   return bot_score - human_score;
}

int minimax_ab(int board[SIZE][SIZE], int depth, int alpha, int beta,
               bool is_maximizing_player)
{
   if (depth == 0 || is_game_over(board))
   {
      return evaluate_board(board);
   }

   int player = is_maximizing_player ? PLAYER_BOT : PLAYER_HUMAN;

   if (!has_valid_moves(board, player))
   {
      return minimax_ab(board, depth - 1, alpha, beta, !is_maximizing_player);
   }

   if (is_maximizing_player)
   {
      int max_eval = INT_MIN;
      for (int i = 0; i < SIZE; i++)
      {
         for (int j = 0; j < SIZE; j++)
         {
            if (is_valid_move(board, i, j, player))
            {
               int temp_board[SIZE][SIZE];
               memcpy(temp_board, board, sizeof(temp_board));
               update_board(temp_board, i, j, player, NULL);

               int eval = minimax_ab(temp_board, depth - 1, alpha, beta, false);
               if (eval > max_eval)
               {
                  max_eval = eval;
               }
               if (eval > alpha)
               {
                  alpha = eval;
               }
               if (beta <= alpha)
               {
                  return max_eval; 
               }
            }
         }
      }
      return max_eval;
   }
   else
   { 
      int min_eval = INT_MAX;
      for (int i = 0; i < SIZE; i++)
      {
         for (int j = 0; j < SIZE; j++)
         {
            if (is_valid_move(board, i, j, player))
            {
               int temp_board[SIZE][SIZE];
               memcpy(temp_board, board, sizeof(temp_board));
               update_board(temp_board, i, j, player, NULL);

               int eval = minimax_ab(temp_board, depth - 1, alpha, beta, true);
               if (eval < min_eval)
               {
                  min_eval = eval;
               }
               if (eval < beta)
               {
                  beta = eval;
               }
               if (beta <= alpha)
               {
                  return min_eval; 
               }
            }
         }
      }
      return min_eval;
   }
}

Move find_best_move(int board[SIZE][SIZE], int depth)
{
   int bestScore = INT_MIN;
   Move bestMove = {-1, -1};

   for (int i = 0; i < SIZE; i++)
   {
      for (int j = 0; j < SIZE; j++)
      {
         if (is_valid_move(board, i, j, PLAYER_BOT))
         {
            int temp_board[SIZE][SIZE];
            memcpy(temp_board, board, sizeof(temp_board));

            update_board(temp_board, i, j, PLAYER_BOT, NULL);

            int score =
                minimax_ab(temp_board, depth - 1, INT_MIN, INT_MAX, false);

            if (score > bestScore)
            {
               bestScore = score;
               bestMove.row = i;
               bestMove.col = j;
            }
         }
      }
   }
   return bestMove;
}

void print_array(moves *last_moves)
{
   puts("Hello");
   for (int i = 0; i < last_moves->size; i++)
      printf("cell: %d %d\n", last_moves->cell_x[i], last_moves->cell_y[i]);

   puts("");
}

void print_board(int board[][SIZE], char *string)
{
   printf("board %s:\n");
   for (int i = 0; i < SIZE; i++)
   {
      for (int j = 0; j < SIZE; j++)
      {
         printf("%d\t", board[i][j]);
      }
      puts("");
   }
   printf("\n\n");
}

void draw_field(GLuint program, GLuint color, GLuint VAO)
{
   glUseProgram(program);
   glUniform3fv(color, 1, field_color);
   glBindVertexArray(VAO);
   glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void draw_lines(GLuint program, GLuint color, GLuint VAO, float lines_thick)
{
   glUseProgram(program);
   glBindVertexArray(VAO);
   glUniform3fv(color, 1, grid_color);

   glLineWidth(lines_thick);
   glDrawArrays(GL_LINES, 0, 54 * 2);
}

void draw_chip(GLuint program, GLuint colorLoc, float *color, GLuint radiusLoc,
               float radius, GLuint centerLoc, float *center, GLuint VAO)
{
   glUseProgram(program);
   glBindVertexArray(VAO);
   glUniform1f(radiusLoc, radius);
   glUniform2fv(centerLoc, 1, center);
   glUniform3fv(colorLoc, 1, color);
   glDrawArrays(GL_TRIANGLE_FAN, 0, SEGMENTS + 2);
}

void draw_panel(int x1, int y1, int x2, int y2, int turn, int board[][SIZE])
{
   begin_text_frame();
   PanelRect info_panel_data = {x1, y1, x2, y2};

   int blacks = 0, whites = 0;
   get_score(board, &blacks, &whites);
   draw_info_panel(info_panel_data, turn, blacks, whites, is_game_over(board),
                   g_current_difficulty);
}

void game_start(int board[][SIZE], Stack **stack_moves, int *turn,
                int *taken_moves)
{
   create_start_board(board);
   *stack_moves = clear_stack(*stack_moves);
   *turn = 0;
   *taken_moves = 0;
   state = PLAYER_MOVE;
   new_game = false; 
   printf("Game started/restarted with difficulty (depth): %d\n",
          g_current_difficulty);
}

int main()
{
   glfwInit();
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

   GLFWmonitor *monitor = glfwGetPrimaryMonitor();
   const GLFWvidmode *mode = glfwGetVideoMode(monitor);

   GLFWwindow *window = glfwCreateWindow(mode->width, mode->height, "Reversi", monitor, NULL);
   if (!window)
   {
      printf("Error with opening the window!\n");
      glfwTerminate();
      return -1;
   }

   glfwSetMouseButtonCallback(window, mouse_button_callback);
   glfwSetKeyCallback(window, escape_button_callback);

   glfwMakeContextCurrent(window);
   gladLoadGL();

   float font_pixel_size = 32.0f;
   if (!init_pretty_text_renderer(window, font_file, font_pixel_size))
   {
      fprintf(stderr, "ПИЗДЕЦ! ERROR ERROR. Не удалось инициализировать "
                      "рендерер текста!\n");
      glfwDestroyWindow(window);
      glfwTerminate();
      return 1;
   }

   int width = mode->width;
   int height = mode->height;

   int x1 = (width - height) / 2 - 250;
   int y1 = height / 20;
   int x2 = height;
   int y2 = height - 2 * y1;

   int marginLeft = x1 + x2 + 50;
   int marginRight = 50;
   int rightBorder = width - marginLeft - marginRight;

   glViewport(x1, y1, x2, y2);

   
   char *vertexShaderSourceNormal = read_shader(vertexShaderPath);
   GLuint vertexShaderNormal = create_shader(vertexShaderSourceNormal, vertex);

   char *fragmentShaderSource = read_shader(fragmentShaderPath);
   GLuint fragmentShader = create_shader(fragmentShaderSource, fragment);

   char *vertexShaderSourceChip = read_shader(chipVertexShaderPath);
   GLuint vertexShaderChip = create_shader(vertexShaderSourceChip, vertex);

   GLuint shaderProgramNormal = glCreateProgram();
   GLuint shaderProgramChip = glCreateProgram();

   
   glAttachShader(shaderProgramNormal, vertexShaderNormal);
   glAttachShader(shaderProgramNormal, fragmentShader);
   glLinkProgram(shaderProgramNormal);

   
   glAttachShader(shaderProgramChip, vertexShaderChip);
   glAttachShader(shaderProgramChip, fragmentShader);
   glLinkProgram(shaderProgramChip);

   glDeleteShader(vertexShaderNormal);
   glDeleteShader(vertexShaderChip);
   glDeleteShader(fragmentShader);

   free(vertexShaderSourceNormal);
   free(vertexShaderSourceChip);
   free(fragmentShaderSource);

   
   GLfloat circle_vertices[(SEGMENTS + 2) * 3];
   create_circle_vertices(circle_vertices, SEGMENTS);

   GLuint VAO_CI, VBO_CI;
   glGenVertexArrays(1, &VAO_CI);
   glGenBuffers(1, &VBO_CI);

   glBindVertexArray(VAO_CI);
   glBindBuffer(GL_ARRAY_BUFFER, VBO_CI);
   glBufferData(GL_ARRAY_BUFFER, sizeof(circle_vertices), circle_vertices,
                GL_STATIC_DRAW);

   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                         (void *)0);
   glEnableVertexAttribArray(0);
   

   
   GLfloat lines_vertices[9 * 6 * 2];
   create_lines_vertices(lines_vertices);

   GLuint VAO_LI, VBO_LI;
   glGenVertexArrays(1, &VAO_LI);
   glGenBuffers(1, &VBO_LI);

   glBindVertexArray(VAO_LI);
   glBindBuffer(GL_ARRAY_BUFFER, VBO_LI);
   glBufferData(GL_ARRAY_BUFFER, sizeof(lines_vertices), lines_vertices,
                GL_STATIC_DRAW);

   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                         (void *)0);
   glEnableVertexAttribArray(0);

   glBindVertexArray(0);
   

   
   GLfloat rectangle_vertices[] = {
       -1.0f, 1.0f,  0.0f, -1.0f, -1.0f, 0.0f,
       1.0f,  -1.0f, 0.0f, 1.0f,  1.0f,  0.0f,
   };

   GLuint rectangle_indexes[] = {0, 1, 2, 0, 3, 2};

   GLuint VAO_RE, VBO_RE, EBO_RE;
   glGenVertexArrays(1, &VAO_RE);
   glGenBuffers(1, &VBO_RE);
   glGenBuffers(1, &EBO_RE);

   glBindVertexArray(VAO_RE);
   glBindBuffer(GL_ARRAY_BUFFER, VBO_RE);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_RE);

   glBufferData(GL_ARRAY_BUFFER, sizeof(rectangle_vertices), rectangle_vertices,
                GL_STATIC_DRAW);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(rectangle_indexes),
                rectangle_indexes, GL_STATIC_DRAW);

   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                         (void *)0);
   glEnableVertexAttribArray(0);

   glBindVertexArray(0);
   

   GLuint colorLocNormal =
       glGetUniformLocation(shaderProgramNormal, "sq_color");

   GLuint colorLocChip = glGetUniformLocation(shaderProgramChip, "sq_color");
   GLuint centerLoc = glGetUniformLocation(shaderProgramChip, "center");
   GLuint radiusLoc = glGetUniformLocation(shaderProgramChip, "radius");

   Stack *stack_moves = NULL;

   int board[SIZE][SIZE];
   create_start_board(board);

   int turn = 0;
   int taken_moves = 0;
   while (!glfwWindowShouldClose(window))
   {
      glfwPollEvents();

      if (new_game)
      {
         game_start(board, &stack_moves, &turn, &taken_moves);
      }

      
      glViewport(0, 0, width, height);
      draw_panel((float)marginLeft, (float)y1 + y2 / 4, rightBorder, y2 / 2,
                 turn, board);

      
      glClearColor(background_color[0], background_color[1],
                   background_color[2], 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);

      
      glViewport(x1, y1, x2, y2);

      
      if (new_game)
      {
         create_start_board(board);

         stack_moves = clear_stack(stack_moves);
         turn = 0;
         taken_moves = 0;
         state = PLAYER_MOVE;
         new_game = false;
      }

      if (move_back && taken_moves > 0)
      {
         for (int i = 0; i < 2 && taken_moves > 0; i++)
         {
            moves last_move;
            stack_moves = pop(stack_moves, &last_move);
            if (stack_moves != NULL || taken_moves == 1)
            {
               go_backward(board, last_move);
               taken_moves--;
               turn--;
            }
         }
         state = PLAYER_MOVE;

         clicked = false;
         move_back = false;

         printf("Move undone. Current turn: %d. Player to move.\n", turn);
      }

      
      draw_field(shaderProgramNormal, colorLocNormal, VAO_RE);

      
      draw_lines(shaderProgramNormal, colorLocNormal, VAO_LI, 5.0);

      
      for (int i = 0; i < SIZE; i++)
      {
         for (int j = 0; j < SIZE; j++)
         {
            float current_chip_center[] = {j, i};
            if (board[i][j] != clear)
               draw_chip(shaderProgramChip, colorLocChip,
                         chip_colors[board[i][j]], radiusLoc, CHIP_RADIUS,
                         centerLoc, current_chip_center, VAO_CI);

            if (state == PLAYER_MOVE && is_valid_move(board, i, j, turn % 2))
               draw_chip(shaderProgramChip, colorLocChip, suggestion_color,
                         radiusLoc, SUGGEST_RADIUS, centerLoc,
                         current_chip_center, VAO_CI);
         }
      }

      int current_player = turn % 2;

      if (state == PLAYER_MOVE && current_player == PLAYER_HUMAN)
      {
         if (clicked)
         {
            clicked = false;
            if (is_valid_move(board, (int)chip_center[1], (int)chip_center[0],
                              current_player))
            {
               moves *last_move = new_array();
               last_move->cell_x[last_move->size] = (int)chip_center[1];
               last_move->cell_y[last_move->size] = (int)chip_center[0];
               last_move->size++;

               update_board(board, (int)chip_center[1], (int)chip_center[0],
                            current_player, last_move);

               stack_moves = push(stack_moves, last_move);
               taken_moves++;
               turn++;

               state = WAIT_BEFORE_BOT;
               bot_wait_start_time = glfwGetTime();
            }
         }
      }
      
      else if (state == WAIT_BEFORE_BOT)
      {
         if (glfwGetTime() - bot_wait_start_time >= 0.5)
         {
            state = BOT_MOVE;
         }
      }
      
      else if (state == BOT_MOVE && current_player == PLAYER_BOT)
      {
         
         Move best_bot_move = find_best_move(board, g_current_difficulty);

         if (best_bot_move.row != -1)
         {
            moves *last_move = new_array();
            last_move->cell_x[last_move->size] = best_bot_move.row;
            last_move->cell_y[last_move->size] = best_bot_move.col;
            last_move->size++;

            update_board(board, best_bot_move.row, best_bot_move.col,
                         current_player, last_move);

            stack_moves = push(stack_moves, last_move);
            taken_moves++;
            turn++;
         }
         else
         {
            turn++;
         }
         state = PLAYER_MOVE;
      }

      
      if (!is_game_over(board) && !has_valid_moves(board, turn % 2))
      {
         printf("Player %s skips turn!\n",
                (turn % 2 == PLAYER_HUMAN) ? "Human" : "Bot");
         turn++;
         if (turn % 2 == PLAYER_BOT)
         {
            state = WAIT_BEFORE_BOT;
            bot_wait_start_time = glfwGetTime();
         }
         else
         {
            state = PLAYER_MOVE;
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
