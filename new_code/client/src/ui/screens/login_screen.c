// login_screen.c - Màn hình Đăng nhập/Đăng ký
#include "login_screen.h"
#include "../../ui/renderer.h"
#include "../../ui/colors.h"
#include <string.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#define SCREEN_WIDTH 1000   // Chiều rộng màn hình
#define SCREEN_HEIGHT 700   // Chiều cao màn hình

// ==================== VẼ Ô NHẬP LIỆU ====================
/**
 * Hàm vẽ một ô nhập text (input field)
 * @param renderer - Renderer SDL để vẽ
 * @param font - Font chữ sử dụng
 * @param field - Con trỏ tới InputField cần vẽ
 * @param label - Nhãn hiển thị phía trên ô nhập (VD: "Username:")
 */
static void render_input_field(SDL_Renderer* renderer, TTF_Font* font, InputField* field, const char* label) {
    SDL_Color white = {255, 255, 255, 255};  // Màu trắng cho text

    // Vẽ nhãn (label) phía trên ô nhập
    render_text(renderer, font, label, field->rect.x, field->rect.y - 30, white);

    // Vẽ nền của ô nhập
    // Nếu ô đang active (được chọn) thì màu sáng hơn (100), không thì tối hơn (60)
    SDL_SetRenderDrawColor(renderer,
        field->is_active ? 100 : 60,   // R - Red
        field->is_active ? 100 : 60,   // G - Green
        field->is_active ? 100 : 60,   // B - Blue
        255);                           // A - Alpha (độ trong suốt)
    SDL_RenderFillRect(renderer, &field->rect);  // Tô màu nền

    // Vẽ viền của ô nhập
    // Nếu ô đang active thì viền trắng (255), không thì xám (150)
    SDL_SetRenderDrawColor(renderer,
        field->is_active ? 255 : 150,
        field->is_active ? 255 : 150,
        field->is_active ? 255 : 150,
        255);
    SDL_RenderDrawRect(renderer, &field->rect);  // Vẽ viền

    // Vẽ text bên trong ô nhập
    if(strlen(field->text) > 0) {  // Chỉ vẽ khi có text
        char display[50];

        // Nếu là ô password thì thay text bằng dấu '*'
        if(field->is_password) {
            for(int i=0; i<strlen(field->text); i++)
                display[i] = '*';  // Mỗi ký tự thành dấu *
            display[strlen(field->text)] = '\0';  // Kết thúc chuỗi
        } else {
            // Nếu không phải password thì hiển thị bình thường
            strcpy(display, field->text);
        }

        // Vẽ text với khoảng cách 10px từ mép trái, 8px từ mép trên
        render_text(renderer, font, display, field->rect.x + 10, field->rect.y + 8, white);
    }
}

// ==================== VẼ MÀN HÌNH ĐĂNG NHẬP ====================
/**
 * Hàm vẽ toàn bộ màn hình đăng nhập
 * @param renderer - Renderer SDL
 * @param game - Con trỏ tới GameData chứa toàn bộ thông tin game
 */
void login_screen_render(SDL_Renderer* renderer, GameData* game) {
    // Tô nền màn hình màu xanh đen (20, 30, 50)
    SDL_SetRenderDrawColor(renderer, 20, 30, 50, 255);
    SDL_RenderClear(renderer);

    // Vẽ ảnh nền battle_ship_1.png (toàn màn hình 1000x700)
    assets_render_image(renderer, &game->assets, "battle_ship_1.png", 0, 0, 1000, 700);

    // Định nghĩa các màu sử dụng
    SDL_Color cyan = {0, 200, 255, 255};      // Màu xanh lơ - cho tiêu đề
    SDL_Color red = {255, 50, 50, 255};       // Màu đỏ - cho thông báo lỗi
    SDL_Color green = {0, 150, 0, 255};       // Màu xanh lá - cho nút Login
    SDL_Color blue = {0, 100, 200, 255};      // Màu xanh dương - cho nút chuyển đổi

    // Vẽ tiêu đề
    // Hiển thị "SIGN UP" nếu đang ở chế độ đăng ký, "LOG IN" nếu đang đăng nhập
    render_text(renderer, game->font,
                game->is_register_mode ? "SIGN UP" : "LOG IN",
                350, 100, cyan);

    // Vẽ 2 ô nhập liệu
    render_input_field(renderer, game->font_small, &game->username_field, "Username:");
    render_input_field(renderer, game->font_small, &game->password_field, "Password:");

    // Lấy vị trí chuột để kiểm tra hover
    int mx, my;
    SDL_GetMouseState(&mx, &my);

    // Kiểm tra xem chuột có đang hover trên nút Login/Signup không
    int login_hover = (mx >= 350 && mx <= 500 && my >= 400 && my <= 450);

    // Kiểm tra xem chuột có đang hover trên nút chuyển đổi không
    int toggle_hover = (mx >= 520 && mx <= 700 && my >= 400 && my <= 450);

    // Kiểm tra xem chuột có đang hover trên nút Quit không
    int quit_hover = (mx >= 850 && mx <= 950 && my >= 630 && my <= 680);

    // Vẽ nút Login/Signup
    // Text thay đổi dựa vào chế độ hiện tại
    render_button(renderer, game->font_small,
                  game->is_register_mode ? "SIGN UP" : "LOG IN",
                  350, 400,      // x, y
                  150, 50,       // width, height
                  green,         // màu nút
                  login_hover,   // có đang hover không
                  1);            // enabled (luôn luôn bật)

    // Vẽ nút chuyển đổi giữa Login và Register
    render_button(renderer, game->font_small,
                  game->is_register_mode ? "Have an account?" : "Create an account",
                  520, 400,      // x, y
                  180, 50,       // width, height
                  blue,          // màu nút
                  toggle_hover,  // có đang hover không
                  1);            // enabled

    // Hiển thị thông báo (nếu có)
    if(strlen(game->login_message) > 0) {
        // Nếu message chứa "OK" thì màu xanh (thành công), không thì màu đỏ (lỗi)
        SDL_Color msg_color = strstr(game->login_message, "OK") ? cyan : red;
        render_text(renderer, game->font_small, game->login_message, 300, 480, msg_color);
    }

    // Vẽ nút Quit ở góc dưới bên phải
    SDL_Color quit_color = {150, 0, 0, 255};  // Màu đỏ đậm
    render_button(renderer, game->font_small,
                  "Quit",
                  860, 630,      // x, y (góc dưới bên phải)
                  100, 50,       // width, height
                  quit_color,    // màu đỏ
                  quit_hover,    // có đang hover không
                  1);            // enabled
}

// ==================== XỬ LÝ CLICK CHUỘT ====================
/**
 * Hàm xử lý khi người dùng click chuột trên màn hình đăng nhập
 * @param game - Con trỏ tới GameData
 * @param x - Tọa độ x của click
 * @param y - Tọa độ y của click
 */
void login_screen_handle_click(GameData* game, int x, int y) {
    // Kiểm tra click vào ô Username
    if(x >= game->username_field.rect.x && x <= game->username_field.rect.x + game->username_field.rect.w &&
       y >= game->username_field.rect.y && y <= game->username_field.rect.y + game->username_field.rect.h) {
        // Kích hoạt ô username, tắt ô password
        game->username_field.is_active = 1;
        game->password_field.is_active = 0;
        return;
    }

    // Kiểm tra click vào ô Password
    if(x >= game->password_field.rect.x && x <= game->password_field.rect.x + game->password_field.rect.w &&
       y >= game->password_field.rect.y && y <= game->password_field.rect.y + game->password_field.rect.h) {
        // Kích hoạt ô password, tắt ô username
        game->username_field.is_active = 0;
        game->password_field.is_active = 1;
        return;
    }

    // Kiểm tra click vào nút Login/Register
    if(x >= 350 && x <= 500 && y >= 400 && y <= 450) {
        // Validate: phải nhập đủ cả username và password
        if(strlen(game->username_field.text) == 0 || strlen(game->password_field.text) == 0) {
            strcpy(game->login_message, "Please enter username and password!");
            return;
        }
        // Lệnh đăng nhập/đăng ký sẽ được gửi từ main client
        // (xem trong client_gui_complete.c)
    }

    // Kiểm tra click vào nút chuyển đổi mode (Login <-> Register)
    if(x >= 520 && x <= 700 && y >= 400 && y <= 450) {
        // Đảo trạng thái: Login -> Register hoặc ngược lại
        game->is_register_mode = !game->is_register_mode;
        // Xóa thông báo cũ
        strcpy(game->login_message, "");
    }

    // Kiểm tra click vào nút Quit
    if(x >= 850 && x <= 950 && y >= 630 && y <= 680) {
        // Thoát game
        game->running = 0;
    }
}

// ==================== XỬ LÝ NHẬP TEXT ====================
/**
 * Hàm xử lý khi người dùng gõ ký tự vào ô input
 * @param game - Con trỏ tới GameData
 * @param text - Chuỗi ký tự vừa gõ
 */
void login_screen_handle_text(GameData* game, const char* text) {
    // Xác định ô input nào đang active
    InputField* active = game->username_field.is_active ? &game->username_field : &game->password_field;

    // Thêm text vào ô input (giới hạn 49 ký tự, để dành 1 cho '\0')
    if(strlen(active->text) < 49) {
        strcat(active->text, text);  // Nối chuỗi
    }
}

// ==================== XỬ LÝ PHÍM BẤM ====================
/**
 * Hàm xử lý các phím đặc biệt (Backspace, Tab, Enter)
 * @param game - Con trỏ tới GameData
 * @param key - Mã phím được bấm
 */
void login_screen_handle_key(GameData* game, SDL_Keycode key) {
    // Xác định ô input nào đang active
    InputField* active = game->username_field.is_active ? &game->username_field : &game->password_field;

    if(key == SDLK_BACKSPACE && strlen(active->text) > 0) {
        // Phím Backspace: Xóa ký tự cuối cùng
        active->text[strlen(active->text) - 1] = '\0';

    } else if(key == SDLK_TAB) {
        // Phím Tab: Chuyển đổi giữa 2 ô input
        game->username_field.is_active = !game->username_field.is_active;
        game->password_field.is_active = !game->password_field.is_active;

    } else if(key == SDLK_RETURN) {
        // Phím Enter: Kích hoạt nút Login/Register
        login_screen_handle_click(game, 350, 425); // Giả lập click vào nút
    }
}
