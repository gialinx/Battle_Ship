#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <time.h>
#include <math.h>

#define DB_NAME "battleship.db"

typedef struct {
    int user_id;
    char username[50];
    int total_games;
    int wins;
    int losses;
    int total_score;
    int elo_rating;
    char status[20];
} UserProfile;

typedef struct {
    int match_id;
    int player1_id;
    int player2_id;
    int winner_id;
    int player1_score;
    int player2_score;
    int player1_elo_before;
    int player2_elo_before;
    int player1_elo_gain;
    int player2_elo_gain;
    int player1_elo_after;
    int player2_elo_after;
    int player1_hit_diff;
    int player2_hit_diff;
    float player1_accuracy;
    float player2_accuracy;
    char match_data[4096];
    time_t played_at;
} MatchHistory;

int db_init();
int db_register_user(const char* username, const char* password);
int db_login_user(const char* username, const char* password, UserProfile* profile);
int db_logout_user(int user_id);
int db_update_user_status(int user_id, const char* status);
int db_get_user_profile(int user_id, UserProfile* profile);
int db_save_match(MatchHistory* match);
int db_get_match_history(int user_id, MatchHistory** matches, int* count);
int db_get_match_for_rewatch(int match_id, MatchHistory* match);
int db_update_score(int user_id, int score_change, int is_win);
int calculate_elo_change(int player_elo, int opponent_elo, int total_games,
                         float hit_diff, float accuracy, int is_winner);
void db_update_elo_after_match(MatchHistory* match);

// New functions for lobby redesign
int db_get_leaderboard(char* output, int max_size);  // Returns formatted string
int db_get_user_match_history(int user_id, char* output, int max_size);  // Returns formatted string

void db_close();

#endif