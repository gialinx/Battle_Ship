#ifndef MATCHMAKING_H
#define MATCHMAKING_H

#include <time.h>

#define MAX_QUEUE 20
#define MM_TIMEOUT_SECONDS 300  // 5 phút timeout auto-remove

typedef struct {
    int user_id;
    char username[50];
    int elo_rating;
    int total_games;   // Số trận đã chơi (để bảo vệ newbie)
    time_t join_time;  // Thời gian vào queue
} QueueEntry;

typedef struct {
    QueueEntry entries[MAX_QUEUE];
    int count;
} MatchmakingQueue;

extern MatchmakingQueue mm_queue;

void mm_init();
int mm_add_player(int user_id, const char* username, int elo_rating, int total_games);
int mm_remove_player(int user_id);
int mm_find_match(int user_id, int* matched_user_id);  // Returns 1 if found
int mm_find_any_match(int* player1_id, int* player2_id);  // Find match for anyone in queue
int mm_get_elo_range(time_t join_time);  // Calculate ELO range based on wait time
int mm_is_in_queue(int user_id);  // Kiểm tra user có trong queue không
void mm_cleanup_timeout();  // Remove players đã timeout (>5 phút)
int mm_get_queue_size();  // Số người đang chờ
void mm_print_queue();  // Debug: in ra queue hiện tại

#endif
