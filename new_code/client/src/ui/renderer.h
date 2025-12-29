// renderer.h - SDL Rendering functions
#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "colors.h"
#include "../core/game_data.h"  // MAP_SIZE

#define CELL_SIZE 30

// Core rendering functions
void render_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, 
                 int x, int y, SDL_Color color);

int render_button(SDL_Renderer* renderer, TTF_Font* font, const char* text,
                  int x, int y, int w, int h, SDL_Color bg, int hover, int enabled);

void render_map(SDL_Renderer* renderer, TTF_Font* font, TTF_Font* small_font,
                char map[MAP_SIZE][MAP_SIZE], int start_x, int start_y, 
                int cell_size, int is_own_map);

void render_ship_preview(SDL_Renderer* renderer, int start_x, int start_y,
                         int cell_size, int x, int y, int length, 
                         int horizontal, int valid, char own_map[MAP_SIZE][MAP_SIZE]);

#endif


