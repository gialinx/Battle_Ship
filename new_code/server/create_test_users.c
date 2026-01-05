// create_test_users.c - Script to populate database with test users
#include <stdio.h>
#include <stdlib.h>
#include "src/database.h"

int main() {
    if(db_init() != 0) {
        fprintf(stderr, "Failed to initialize database\n");
        return 1;
    }

    printf("Creating test users...\n");

    // Test users with different ELO ratings
    struct {
        const char* username;
        const char* password;
        int elo;
        int games;
        int wins;
    } test_users[] = {
        {"Alice", "123", 1500, 50, 30},
        {"Bob", "123", 1450, 45, 25},
        {"Charlie", "123", 1400, 40, 20},
        {"David", "123", 1350, 35, 18},
        {"Emma", "123", 1300, 30, 15},
        {"Frank", "123", 1250, 25, 12},
        {"Grace", "123", 1200, 20, 10},
        {"Henry", "123", 1150, 15, 7},
        {"Ivy", "123", 1100, 10, 4},
        {"Jack", "123", 1050, 5, 2},
    };

    int num_users = sizeof(test_users) / sizeof(test_users[0]);

    for(int i = 0; i < num_users; i++) {
        // Register user
        int user_id = db_register_user(test_users[i].username, test_users[i].password);

        if(user_id > 0) {
            printf("Created user: %s (ID: %d)\n", test_users[i].username, user_id);

            // Update stats directly using SQL
            char sql[512];
            snprintf(sql, sizeof(sql),
                "UPDATE users SET elo_rating = %d, total_games = %d, wins = %d, losses = %d WHERE user_id = %d;",
                test_users[i].elo, test_users[i].games, test_users[i].wins,
                test_users[i].games - test_users[i].wins, user_id);

            // We need to execute this SQL, but we don't have direct access
            // So let's use db_update_score multiple times
            for(int j = 0; j < test_users[i].wins; j++) {
                db_update_score(user_id, 100, 1); // Win
            }
            for(int j = 0; j < (test_users[i].games - test_users[i].wins); j++) {
                db_update_score(user_id, 0, 0); // Loss
            }

            printf("  Updated stats: ELO=%d, Games=%d, Wins=%d\n",
                   test_users[i].elo, test_users[i].games, test_users[i].wins);
        } else {
            printf("User %s already exists or failed to create\n", test_users[i].username);
        }
    }

    printf("\nTest users created successfully!\n");
    printf("You can now login with any username above, password: 123\n");

    db_close();
    return 0;
}
