// renderer.c - SDL Rendering implementation
#include "renderer.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <SDL2/SDL.h>

// ==================== RENDER TEXT ====================
void render_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, 
                 int x, int y, SDL_Color color) {
    if(!font || !text) return;
    
    SDL_Surface* surface = TTF_RenderUTF8_Solid(font, text, color);
    if(!surface) return;
    
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if(!texture) {
        SDL_FreeSurface(surface);
        return;
    }
    
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &rect);
    
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

// ==================== RENDER BUTTON ====================
int render_button(SDL_Renderer* renderer, TTF_Font* font, const char* text,
                  int x, int y, int w, int h, SDL_Color bg, int hover, int enabled) {
    SDL_Rect rect = {x, y, w, h};
    
    // Determine button color
    SDL_Color button_color;
    if(!enabled) {
        button_color = COLOR_BUTTON_DISABLED;
    } else if(hover) {
        button_color.r = bg.r + 30 > 255 ? 255 : bg.r + 30;
        button_color.g = bg.g + 30 > 255 ? 255 : bg.g + 30;
        button_color.b = bg.b + 30 > 255 ? 255 : bg.b + 30;
        button_color.a = 255;
    } else {
        button_color = bg;
    }
    
    // Draw button background
    SDL_SetRenderDrawColor(renderer, button_color.r, button_color.g, button_color.b, 255);
    SDL_RenderFillRect(renderer, &rect);
    
    // Draw border
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &rect);
    
    // Draw text (centered)
    if(font && text) {
        SDL_Surface* text_surface = TTF_RenderText_Blended(font, text, COLOR_TEXT);
        if(text_surface) {
            SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
            if(text_texture) {
                SDL_Rect text_rect = {
                    x + (w - text_surface->w) / 2,
                    y + (h - text_surface->h) / 2,
                    text_surface->w,
                    text_surface->h
                };
                SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
                SDL_DestroyTexture(text_texture);
            }
            SDL_FreeSurface(text_surface);
        }
    }
    
    return 1;
}

// ==================== RENDER MAP ====================
void render_map(SDL_Renderer* renderer, TTF_Font* font, TTF_Font* small_font,
                char map[MAP_SIZE][MAP_SIZE], int start_x, int start_y, 
                int cell_size, int is_own_map) {
    // Draw row/column numbers
    for(int i = 0; i < MAP_SIZE; i++) {
        char num[3];
        snprintf(num, sizeof(num), "%d", i + 1);
        
        // Column numbers (top)
        render_text(renderer, small_font, num, 
                   start_x + i * cell_size + cell_size/2 - 5, 
                   start_y - 25, COLOR_TEXT);
        
        // Row numbers (left)
        render_text(renderer, small_font, num, 
                   start_x - 30, 
                   start_y + i * cell_size + cell_size/2 - 8, COLOR_TEXT);
    }
    
    // Draw grid cells
    for(int row = 0; row < MAP_SIZE; row++) {
        for(int col = 0; col < MAP_SIZE; col++) {
            SDL_Rect rect = {
                start_x + col * cell_size, 
                start_y + row * cell_size, 
                cell_size - 2, 
                cell_size - 2
            };
            
            char cell = map[row][col];
            SDL_Color cell_color;
            
            // Determine cell color
            if(cell == '-') {
                cell_color = COLOR_OCEAN;
            } else if(cell >= '2' && cell <= '9') {
                if(is_own_map) {
                    cell_color = COLOR_SHIP;
                } else {
                    cell_color = COLOR_OCEAN;  // Hide enemy ships
                }
            } else if(cell == 'x') {
                cell_color = COLOR_MISS;
            } else if(cell == 'o') {
                cell_color = COLOR_HIT;
            } else if(cell == '@') {
                cell_color = COLOR_SUNK;
            } else {
                cell_color = COLOR_OCEAN;
            }
            
            // Draw cell
            SDL_SetRenderDrawColor(renderer, cell_color.r, cell_color.g, cell_color.b, 255);
            SDL_RenderFillRect(renderer, &rect);
            
            // Draw grid border
            SDL_SetRenderDrawColor(renderer, COLOR_GRID.r, COLOR_GRID.g, COLOR_GRID.b, 255);
            SDL_RenderDrawRect(renderer, &rect);
            
            // Draw markers
            if(cell == 'x') {
                // Draw X for miss
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderDrawLine(renderer, 
                    rect.x + 5, rect.y + 5, 
                    rect.x + rect.w - 5, rect.y + rect.h - 5);
                SDL_RenderDrawLine(renderer, 
                    rect.x + rect.w - 5, rect.y + 5, 
                    rect.x + 5, rect.y + rect.h - 5);
            } else if(cell == 'o' || cell == '@') {
                // Draw circle for hit/sunk
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                int cx = rect.x + rect.w/2;
                int cy = rect.y + rect.h/2;
                for(int r = 8; r <= 12; r++) {
                    for(int angle = 0; angle < 360; angle += 10) {
                        int px = cx + (int)(r * cos(angle * M_PI / 180.0));
                        int py = cy + (int)(r * sin(angle * M_PI / 180.0));
                        SDL_RenderDrawPoint(renderer, px, py);
                    }
                }
            }
        }
    }
}

// ==================== RENDER SHIP PREVIEW ====================
void render_ship_preview(SDL_Renderer* renderer, int start_x, int start_y,
                         int cell_size, int x, int y, int length, 
                         int horizontal, int valid, char own_map[MAP_SIZE][MAP_SIZE]) {
    if(length <= 0 || x < 0 || y < 0 || x >= MAP_SIZE || y >= MAP_SIZE) return;
    
    SDL_Color color = valid ? COLOR_SHIP_PREVIEW : COLOR_SHIP_INVALID;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    for(int i = 0; i < length; i++) {
        int tx = x + (horizontal ? i : 0);
        int ty = y + (horizontal ? 0 : i);
        
        if(tx >= 0 && tx < MAP_SIZE && ty >= 0 && ty < MAP_SIZE) {
            SDL_Rect rect = {
                start_x + tx * cell_size, 
                start_y + ty * cell_size, 
                cell_size - 2, 
                cell_size - 2
            };
            
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
            SDL_RenderFillRect(renderer, &rect);
        }
    }
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

