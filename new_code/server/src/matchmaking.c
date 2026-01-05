#include "matchmaking.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

MatchmakingQueue mm_queue = {.count = 0};

void mm_init() {
    mm_queue.count = 0;
}

int mm_add_player(int user_id, const char* username, int elo_rating, int total_games) {
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
    entry->total_games = total_games;
    entry->join_time = time(NULL);

    mm_queue.count++;
    printf("[MM] Player %s (ELO:%d, Games:%d) joined queue\n", username, elo_rating, total_games);
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

// Tính ELO range dựa theo thời gian chờ (cải tiến)
// 0-10s: ±100, 10-30s: ±200, 30-60s: ±300, 60-120s: ±500, >120s: không giới hạn
int mm_get_elo_range(time_t join_time) {
    time_t now = time(NULL);
    int wait_seconds = (int)(now - join_time);

    if (wait_seconds < 10) {
        return 100;   // Tìm đối thủ cùng level
    } else if (wait_seconds < 30) {
        return 200;   // Mở rộng ra
    } else if (wait_seconds < 60) {
        return 300;   // Mở rộng hơn nữa
    } else if (wait_seconds < 120) {
        return 500;   // Chấp nhận mọi đối thủ gần
    } else {
        return 9999;  // Match với bất kỳ ai (không giới hạn)
    }
}

// Tìm match cho user (cải tiến: ưu tiên ELO gần nhất + bảo vệ newbie)
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
    
    // Biến để track đối thủ tốt nhất
    int best_opponent_idx = -1;
    int smallest_elo_diff = 9999;

    // Tìm đối thủ phù hợp (ưu tiên người có ELO gần nhất)
    for (int i = 0; i < mm_queue.count; i++) {
        if (i == my_index) continue;  // Skip chính mình

        QueueEntry* opponent = &mm_queue.entries[i];
        int elo_diff = abs(me->elo_rating - opponent->elo_rating);

        // Kiểm tra điều kiện match
        if (elo_diff > my_range) continue;  // Quá xa về ELO
        
        // Bảo vệ người mới: Người < 10 trận chỉ match với người < 30 trận
        if (me->total_games < 10 && opponent->total_games >= 30) {
            continue;  // Tránh newbie gặp cao thủ
        }
        if (opponent->total_games < 10 && me->total_games >= 30) {
            continue;  // Tránh cao thủ bắt nạt newbie
        }
        
        // Ưu tiên đối thủ có ELO gần nhất
        if (elo_diff < smallest_elo_diff) {
            smallest_elo_diff = elo_diff;
            best_opponent_idx = i;
        }
    }

    // Nếu tìm được đối thủ phù hợp
    if (best_opponent_idx != -1) {
        QueueEntry* opponent = &mm_queue.entries[best_opponent_idx];
        *matched_user_id = opponent->user_id;
        printf("[MM] Match found: %s (ELO:%d, Games:%d) vs %s (ELO:%d, Games:%d) [diff:%d]\n",
               me->username, me->elo_rating, me->total_games,
               opponent->username, opponent->elo_rating, opponent->total_games,
               smallest_elo_diff);
        return 1;
    }

    return 0;  // Chưa tìm được
}

// Kiểm tra user có trong queue không
int mm_is_in_queue(int user_id) {
    for (int i = 0; i < mm_queue.count; i++) {
        if (mm_queue.entries[i].user_id == user_id) {
            return 1;  // Có trong queue
        }
    }
    return 0;  // Không có
}

// Auto-remove players đã chờ quá lâu (>5 phút)
void mm_cleanup_timeout() {
    time_t now = time(NULL);
    int removed_count = 0;
    
    for (int i = 0; i < mm_queue.count; i++) {
        int wait_time = (int)(now - mm_queue.entries[i].join_time);
        
        if (wait_time > MM_TIMEOUT_SECONDS) {
            printf("[MM] Timeout: Removing %s (waited %d seconds)\n",
                   mm_queue.entries[i].username, wait_time);
            
            // Shift các entry phía sau lên
            for (int j = i; j < mm_queue.count - 1; j++) {
                mm_queue.entries[j] = mm_queue.entries[j + 1];
            }
            mm_queue.count--;
            i--;  // Giảm i vì đã shift
            removed_count++;
        }
    }
    
    if (removed_count > 0) {
        printf("[MM] Cleanup: Removed %d timeout players\n", removed_count);
    }
}

// Lấy số người đang chờ
int mm_get_queue_size() {
    return mm_queue.count;
}

// Debug: In ra queue hiện tại
void mm_print_queue() {
    printf("[MM] Queue status (%d players):\n", mm_queue.count);
    time_t now = time(NULL);
    
    for (int i = 0; i < mm_queue.count; i++) {
        QueueEntry* entry = &mm_queue.entries[i];
        int wait_time = (int)(now - entry->join_time);
        int elo_range = mm_get_elo_range(entry->join_time);
        
        printf("  [%d] %s - ELO:%d Games:%d Wait:%ds Range:±%d\n",
               i + 1, entry->username, entry->elo_rating,
               entry->total_games, wait_time, elo_range);
    }
}

// Tìm match cho bất kỳ 2 người trong queue (dùng cho background thread)
int mm_find_any_match(int* player1_id, int* player2_id) {
    if (mm_queue.count < 2) {
        return -1;  // Không đủ người
    }
    
    // Duyệt từng người trong queue
    for (int i = 0; i < mm_queue.count; i++) {
        QueueEntry* player1 = &mm_queue.entries[i];
        int range1 = mm_get_elo_range(player1->join_time);
        
        // Tìm đối thủ phù hợp cho player1
        int best_opponent_idx = -1;
        int smallest_elo_diff = 999999;
        
        for (int j = 0; j < mm_queue.count; j++) {
            if (i == j) continue;  // Không match với chính mình
            
            QueueEntry* player2 = &mm_queue.entries[j];
            int range2 = mm_get_elo_range(player2->join_time);
            int elo_diff = abs(player1->elo_rating - player2->elo_rating);
            
            // Kiểm tra điều kiện match
            if (elo_diff > range1 && elo_diff > range2) continue;  // Quá xa về ELO
            
            // Bảo vệ người mới
            if (player1->total_games < 10 && player2->total_games >= 30) continue;
            if (player2->total_games < 10 && player1->total_games >= 30) continue;
            
            // Ưu tiên đối thủ có ELO gần nhất
            if (elo_diff < smallest_elo_diff) {
                smallest_elo_diff = elo_diff;
                best_opponent_idx = j;
            }
        }
        
        // Nếu tìm được match
        if (best_opponent_idx != -1) {
            QueueEntry* player2 = &mm_queue.entries[best_opponent_idx];
            
            *player1_id = player1->user_id;
            *player2_id = player2->user_id;
            
            printf("[MM] Match found: %s (ELO:%d) vs %s (ELO:%d) [diff:%d]\n",
                   player1->username, player1->elo_rating,
                   player2->username, player2->elo_rating,
                   smallest_elo_diff);
            
            // Xóa cả 2 người khỏi queue
            mm_remove_player(player1->user_id);
            mm_remove_player(player2->user_id);
            
            return 0;  // Success
        }
    }
    
    return -1;  // Không tìm được match nào
}
