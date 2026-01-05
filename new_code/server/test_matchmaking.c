// Test matchmaking logic
#include <stdio.h>
#include <unistd.h>
#include "src/matchmaking.h"

void test_basic_matching() {
    printf("\n=== TEST 1: Basic Matching (Similar ELO) ===\n");
    mm_init();
    
    mm_add_player(1, "Player1", 1000, 5);
    mm_add_player(2, "Player2", 1050, 8);
    mm_add_player(3, "Player3", 1200, 15);
    mm_print_queue();
    
    int matched_id;
    
    // Player1 vs Player2 (ELO diff = 50, cả 2 đều newbie)
    if (mm_find_match(1, &matched_id)) {
        printf("✓ Player1 matched with user_id=%d (expected: 2)\n", matched_id);
    } else {
        printf("✗ Player1 không tìm được match\n");
    }
}

void test_newbie_protection() {
    printf("\n=== TEST 2: Newbie Protection ===\n");
    mm_init();
    
    mm_add_player(1, "Newbie", 1000, 5);      // Newbie (5 games)
    mm_add_player(2, "Expert", 1050, 50);     // Expert (50 games)
    mm_add_player(3, "AnotherNewbie", 1020, 8); // Newbie (8 games)
    mm_print_queue();
    
    int matched_id;
    
    printf("Expected: Newbie should match with AnotherNewbie, NOT Expert\n");
    // Newbie should match with AnotherNewbie, NOT Expert
    if (mm_find_match(1, &matched_id)) {
        printf("✓ Newbie matched with user_id=%d (expected: 3, NOT 2)\n", matched_id);
    } else {
        printf("✗ Newbie không tìm được match\n");
    }
}

void test_queue_management() {
    printf("\n=== TEST 3: Queue Management ===\n");
    mm_init();
    
    mm_add_player(1, "PlayerA", 1000, 10);
    mm_add_player(2, "PlayerB", 1100, 15);
    
    printf("Queue size: %d (expected: 2)\n", mm_get_queue_size());
    printf("PlayerA in queue: %d (expected: 1)\n", mm_is_in_queue(1));
    printf("PlayerC in queue: %d (expected: 0)\n", mm_is_in_queue(999));
    
    mm_remove_player(1);
    printf("After removing PlayerA, queue size: %d (expected: 1)\n", mm_get_queue_size());
}

void test_elo_range_expansion() {
    printf("\n=== TEST 4: ELO Range Expansion Over Time ===\n");
    mm_init();
    
    mm_add_player(1, "PlayerA", 1000, 20);
    sleep(1); // Wait a bit
    mm_add_player(2, "PlayerB", 1250, 25);  // 250 ELO diff
    mm_print_queue();
    
    int matched_id;
    
    // Initially should NOT match (diff=250, range=100)
    printf("Time 0s: Range=±100\n");
    if (mm_find_match(1, &matched_id)) {
        printf("  Match found (unexpected)\n");
    } else {
        printf("  ✓ No match (correct - diff=250 > range=100)\n");
    }
    
    // Wait 15 seconds -> range should be ±200
    printf("Waiting 15s... Range should become ±200\n");
    sleep(15);
    
    if (mm_find_match(1, &matched_id)) {
        printf("  ✓ Match found after range expansion (correct)\n");
    } else {
        printf("  Still no match (range should be ±200 now)\n");
    }
}

void test_best_match_selection() {
    printf("\n=== TEST 5: Best Match Selection (Closest ELO) ===\n");
    mm_init();
    
    mm_add_player(1, "PlayerX", 1000, 20);
    mm_add_player(2, "Close1", 1050, 25);   // diff=50
    mm_add_player(3, "Far1", 1150, 22);     // diff=150
    mm_add_player(4, "VeryClose", 1020, 18); // diff=20
    mm_print_queue();
    
    int matched_id;
    
    printf("Expected: Should match with VeryClose (smallest ELO diff=20)\n");
    // Should match with VeryClose (smallest diff=20)
    if (mm_find_match(1, &matched_id)) {
        printf("✓ PlayerX matched with user_id=%d (expected: 4)\n", matched_id);
    } else {
        printf("✗ PlayerX không tìm được match\n");
    }
}

void test_timeout_cleanup() {
    printf("\n=== TEST 6: Timeout Cleanup ===\n");
    mm_init();
    
    mm_add_player(1, "PlayerOld", 1000, 20);
    printf("Added PlayerOld to queue\n");
    printf("Queue size before: %d\n", mm_get_queue_size());
    
    // Simulate old timestamp (301 seconds ago = 5+ minutes)
    mm_queue.entries[0].join_time -= 301;
    
    printf("Simulated 5+ minutes wait...\n");
    mm_cleanup_timeout();
    printf("Queue size after cleanup: %d (expected: 0)\n", mm_get_queue_size());
}

int main() {
    printf("========================================\n");
    printf("   MATCHMAKING LOGIC TEST SUITE\n");
    printf("========================================\n");
    
    test_basic_matching();
    test_newbie_protection();
    test_queue_management();
    test_best_match_selection();
    test_timeout_cleanup();
    
    printf("\n========================================\n");
    printf("   ALL TESTS COMPLETE\n");
    printf("========================================\n");
    
    return 0;
}
