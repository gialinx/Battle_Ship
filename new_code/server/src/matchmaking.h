#ifndef MATCHMAKING_H
#define MATCHMAKING_H

#include <time.h>

#define MAX_QUEUE 20

typedef struct {
    int user_id;
    char username[50];
    int elo_rating;
    time_t join_time;  // Thời gian vào queue
} QueueEntry;

typedef struct {
    QueueEntry entries[MAX_QUEUE];
    int count;
} MatchmakingQueue;

extern MatchmakingQueue mm_queue;

void mm_init();
int mm_add_player(int user_id, const char* username, int elo_rating);
int mm_remove_player(int user_id);
int mm_find_match(int user_id, int* matched_user_id);  // Returns 1 if found
int mm_get_elo_range(time_t join_time);  // Calculate ELO range based on wait time

#endif
