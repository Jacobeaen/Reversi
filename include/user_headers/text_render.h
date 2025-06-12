#ifndef TEXT_RENDER_H 
#define TEXT_RENDER_H

#include <stdbool.h>
#include <glfw3.h>

typedef struct { 
    float x;
    float y;
    float w;
    float h; 
} PanelRect;


// Инициализация и очистка
bool init_pretty_text_renderer(GLFWwindow* window, const char* font_path, float font_size);
void cleanup_pretty_text_renderer();

// Управление кадром Nuklear
void begin_text_frame();
void render_text_frame();

void draw_info_panel(
    PanelRect  panel_rect, // Где рисовать панель
    int current_turn,          // Чей ход
    int black_score,           // Счет черных
    int white_score,           // Счет белых
    bool is_game_over);        // Игра закончена?

#endif