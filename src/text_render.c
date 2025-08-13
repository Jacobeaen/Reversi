

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT

#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glad.h>
#include <glfw3.h>

#include "nuklear.h"
#include "nuklear_glfw_gl3.h"
#include "text_render.h"

#define MAX_VERTEX_BUFFER (512 * 1024)
#define MAX_ELEMENT_BUFFER (128 * 1024)

extern bool new_game;
extern bool move_back;

extern Difficulty g_current_difficulty;

static struct nk_context *g_nk_context = NULL;
static struct nk_glfw g_nk_glfw = {0};
static struct nk_font *g_pretty_font = NULL;

bool init_pretty_text_renderer(GLFWwindow *window, const char *font_path,
                               float font_size) {
  if (!window || !font_path) {
    fprintf(stderr, "window or path to font is not specified\n");
    return false;
  }

  g_nk_context = nk_glfw3_init(&g_nk_glfw, window, NK_GLFW3_DEFAULT);

  if (!g_nk_context) {
    fprintf(stderr, "Nuclear contex was not created\n");
    return false;
  }

  struct nk_font_atlas *atlas;
  nk_glfw3_font_stash_begin(&g_nk_glfw, &atlas);

  g_pretty_font =
      nk_font_atlas_add_from_file(atlas, font_path, font_size, NULL);
  if (!g_pretty_font) {
    nk_glfw3_font_stash_end(&g_nk_glfw);

    fprintf(stderr, "Cant load font: %s (size: %.1f)\n", font_path, font_size);
    fprintf(stderr, "Check file and path! Defalt font will be used.\n");
  } else {
    printf("Font '%s' (%.1f px) loaded!\n", font_path, font_size);
    nk_glfw3_font_stash_end(&g_nk_glfw);

    nk_style_set_font(g_nk_context, &g_pretty_font->handle);
  }

  printf("Render was initialized!\n");
  return true;
}

void begin_text_frame() {
  if (g_nk_context) {
    nk_glfw3_new_frame(&g_nk_glfw);
  }
}

void render_text_frame() {
  if (g_nk_context) {
    nk_glfw3_render(&g_nk_glfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER,
                    MAX_ELEMENT_BUFFER);
  }
}

void cleanup_pretty_text_renderer() {
  if (g_nk_context) {
    nk_glfw3_shutdown(&g_nk_glfw);
    g_nk_context = NULL;
    g_pretty_font = NULL;
    printf("Render cleared!\n");
  }
}

static void render_pretty_text_in_panel(const char *text,
                                        unsigned char text_color[4],
                                        enum nk_text_alignment alignment) {

  if (!g_nk_context || !text)
    return;

  const struct nk_user_font *font = g_nk_context->style.font;
  if (g_pretty_font)
    font = &g_pretty_font->handle;

  if (!font)
    return;

  float row_height = font->height + 4.0f;
  nk_layout_row_dynamic(g_nk_context, row_height, 1);

  struct nk_color color =
      nk_rgba(text_color[0], text_color[1], text_color[2], text_color[3]);

  nk_label_colored(g_nk_context, text, alignment, color);
}

void draw_info_panel(PanelRect panel_rect, int current_turn, int black_score,
                     int white_score, bool is_game_over,
                     Difficulty current_difficulty) {
  if (!g_nk_context)
    return;

  struct nk_rect info_nk_rect =
      nk_rect(panel_rect.x, panel_rect.y, panel_rect.w, panel_rect.h);

  if (nk_begin(g_nk_context, "INFORMATION", info_nk_rect,
               NK_WINDOW_BORDER | NK_WINDOW_TITLE | NK_WINDOW_NO_SCROLLBAR)) {

    unsigned char color_white[4] = {255, 255, 255, 255};
    unsigned char color_black[4] = {30, 30, 30, 255};
    unsigned char color_yellow[4] = {255, 255, 0, 255};
    unsigned char color_red[4] = {255, 50, 50, 255};

    char buffer[128];

    const char *turn_str =
        (current_turn % 2 == 0) ? "Black move" : "White move";
    render_pretty_text_in_panel(turn_str, color_red, NK_TEXT_LEFT);

    snprintf(buffer, sizeof(buffer), "Score:");
    render_pretty_text_in_panel(buffer, color_white, NK_TEXT_LEFT);
    snprintf(buffer, sizeof(buffer), "  Black: %d", black_score);
    render_pretty_text_in_panel(buffer, color_yellow, NK_TEXT_LEFT);
    snprintf(buffer, sizeof(buffer), "  White: %d", white_score);
    render_pretty_text_in_panel(buffer, color_yellow, NK_TEXT_LEFT);

    render_pretty_text_in_panel(" ", color_white, NK_TEXT_LEFT);

    if (is_game_over) {
      const char *winner_str;

      if (black_score > white_score)
        winner_str = "Black won!";
      else if (white_score > black_score)
        winner_str = "White won!";
      else
        winner_str = "Withdraw!";

      render_pretty_text_in_panel(winner_str, color_red, NK_TEXT_CENTERED);
    } else
      render_pretty_text_in_panel("Status: game in process", color_yellow,
                                  NK_TEXT_CENTERED);

    render_pretty_text_in_panel(" ", color_white, NK_TEXT_LEFT);

    nk_layout_row_dynamic(g_nk_context, 30, 2);

    if (nk_button_label(g_nk_context, "New Game")) {
      printf("New Game button pressed!\n");
      new_game = true;
    }

    if (nk_button_label(g_nk_context, "Cancel move")) {
      printf("Cancel move button pressed!\n");
      move_back = true;
    }

    nk_layout_row_dynamic(g_nk_context, 20, 1);
    nk_layout_row_dynamic(g_nk_context, 20, 1);
    nk_label(g_nk_context, "Difficulty:", NK_TEXT_LEFT);

    nk_layout_row_dynamic(g_nk_context, 35, 4);

    {
      bool is_active = (current_difficulty == DIFFICULTY_EASY);
      if (is_active) {
        nk_style_push_style_item(g_nk_context,
                                 &g_nk_context->style.button.normal,
                                 g_nk_context->style.button.active);
        nk_style_push_style_item(g_nk_context,
                                 &g_nk_context->style.button.hover,
                                 g_nk_context->style.button.active);
      }
      if (nk_button_label(g_nk_context, "Easy")) {
        if (!is_active) {
          g_current_difficulty = DIFFICULTY_EASY;
          new_game = true;
        }
      }
      if (is_active) {
        nk_style_pop_style_item(g_nk_context);
        nk_style_pop_style_item(g_nk_context);
      }
    }

    {
      bool is_active = (current_difficulty == DIFFICULTY_MEDIUM);
      if (is_active) {
        nk_style_push_style_item(g_nk_context,
                                 &g_nk_context->style.button.normal,
                                 g_nk_context->style.button.active);
        nk_style_push_style_item(g_nk_context,
                                 &g_nk_context->style.button.hover,
                                 g_nk_context->style.button.active);
      }
      if (nk_button_label(g_nk_context, "Medium")) {
        if (!is_active) {
          g_current_difficulty = DIFFICULTY_MEDIUM;
          new_game = true;
        }
      }
      if (is_active) {
        nk_style_pop_style_item(g_nk_context);
        nk_style_pop_style_item(g_nk_context);
      }
    }

    {
      bool is_active = (current_difficulty == DIFFICULTY_HARD);
      if (is_active) {
        nk_style_push_style_item(g_nk_context,
                                 &g_nk_context->style.button.normal,
                                 g_nk_context->style.button.active);
        nk_style_push_style_item(g_nk_context,
                                 &g_nk_context->style.button.hover,
                                 g_nk_context->style.button.active);
      }
      if (nk_button_label(g_nk_context, "Hard")) {
        if (!is_active) {
          g_current_difficulty = DIFFICULTY_HARD;
          new_game = true;
        }
      }
      if (is_active) {
        nk_style_pop_style_item(g_nk_context);
        nk_style_pop_style_item(g_nk_context);
      }
    }

    {
      bool is_active = (current_difficulty == DIFFICULTY_EXPERT);
      if (is_active) {
        nk_style_push_style_item(g_nk_context,
                                 &g_nk_context->style.button.normal,
                                 g_nk_context->style.button.active);
        nk_style_push_style_item(g_nk_context,
                                 &g_nk_context->style.button.hover,
                                 g_nk_context->style.button.active);
      }
      if (nk_button_label(g_nk_context, "Expert")) {
        if (!is_active) {
          g_current_difficulty = DIFFICULTY_EXPERT;
          new_game = true;
        }
      }
      if (is_active) {
        nk_style_pop_style_item(g_nk_context);
        nk_style_pop_style_item(g_nk_context);
      }
    }
  }
  nk_end(g_nk_context);
}
