#include "matchmaking.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

MatchmakingQueue mm_queue = {.count = 0};

void mm_init() {
    mm_queue.count = 0;
}

int mm_add_player(int user_id, const char* username, int elo_rating) {
    // Kiểm tra đã có trong queue chưa
    for (int i = 0; i < mm_queue.count; i++) {
        if (mm_queue.entries[i].user_id == user_id) {
            return -1;  // Đã trong queue rồi
        }
    }

    // Kiểm tra queue đầy chưa
    if (mm_queue.count >= MAX_QUEUE) {
        return -1;
    }

    // Thêm vào queue
    QueueEntry* entry = &mm_queue.entries[mm_queue.count];
    entry->user_id = user_id;
    strncpy(entry->username, username, sizeof(entry->username) - 1);
    entry->elo_rating = elo_rating;
    entry->join_time = time(NULL);

    mm_queue.count++;
    printf("[MM] Player %s (ELO:%d) joined queue\n", username, elo_rating);
    return 0;
}

int mm_remove_player(int user_id) {
    for (int i = 0; i < mm_queue.count; i++) {
        if (mm_queue.entries[i].user_id == user_id) {
            // Shift các entry phía sau lên
            for (int j = i; j < mm_queue.count - 1; j++) {
                mm_queue.entries[j] = mm_queue.entries[j + 1];
            }
            mm_queue.count--;
            printf("[MM] Player removed from queue\n");
            return 0;
        }
    }
    return -1;
}

// Tính ELO range dựa theo thời gian chờ
// Ban đầu: ±200, mỗi 20s tăng thêm 100
int mm_get_elo_range(time_t join_time) {
    time_t now = time(NULL);
    int wait_seconds = (int)(now - join_time);

    int base_range = 200;
    int extra_range = (wait_seconds / 20) * 100;  // Mỗi 20s tăng 100

    return base_range + extra_range;
}

// Tìm match cho user
int mm_find_match(int user_id, int* matched_user_id) {
    int my_index = -1;

    // Tìm user trong queue
    for (int i = 0; i < mm_queue.count; i++) {
        if (mm_queue.entries[i].user_id == user_id) {
            my_index = i;
            break;
        }
    }

    if (my_index == -1) {
        return 0;  // Không có trong queue
    }

    QueueEntry* me = &mm_queue.entries[my_index];
    int my_range = mm_get_elo_range(me->join_time);

    // Tìm đối thủ phù hợp
    for (int i = 0; i < mm_queue.count; i++) {
        if (i == my_index) continue;  // Skip chính mình

        QueueEntry* opponent = &mm_queue.entries[i];
        int elo_diff = abs(me->elo_rating - opponent->elo_rating);

        if (elo_diff <= my_range) {
            // Tìm được match!
            *matched_user_id = opponent->user_id;
            printf("[MM] Match found: %s (ELO:%d) vs %s (ELO:%d)\n",
                   me->username, me->elo_rating,
                   opponent->username, opponent->elo_rating);
            return 1;
        }
    }

    return 0;  // Chưa tìm được
}
