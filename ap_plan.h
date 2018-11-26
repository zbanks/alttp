#pragma once
#include "ap_macro.h"

#define AP_GOAL_TYPE_LIST \
    X(NONE) \
    X(ITEM) \
    X(EXPLORE) \

extern const char * const ap_goal_type_names[];
struct ap_goal;
struct ap_goal {
    struct ap_goal * next;
    struct ap_goal * prev;

    enum ap_goal_type {
#define X(type) CONCAT(GOAL_, type),
AP_GOAL_TYPE_LIST
#undef X
    } type;
    char name[32];

    struct ap_node * node;
    int attempts;
};
extern struct ap_goal * ap_goal_list;

#define GOAL_SCORE_IMPOSSIBLE INT_MAX


#define PRIGOAL "%s %s [node=%s]"
#define PRIGOALF(g) ap_goal_type_names[(g)->type], (g)->name, ((g)->node ? (g)->node->name : "(null)")

void
ap_print_goals();

struct ap_goal *
ap_goal_append();

struct ap_goal *
ap_goal_add(enum ap_goal_type type, struct ap_node * node);

#define AP_TASK_TYPE_LIST \
    X(NONE) \
    X(GOTO_POINT) \
    X(TRANSITION) \
    X(CHEST) \
    X(POT) \

extern const char * const ap_task_type_names[];
struct ap_task;
struct ap_task {
    struct ap_task * next;
    struct ap_task * prev;

    enum ap_task_type {
#define X(type) CONCAT(TASK_, type),
AP_TASK_TYPE_LIST
#undef X
    } type;
    char name[32];

    int timeout;
    int state;

    struct ap_node * node;
    uint8_t direction;
};
extern struct ap_task * ap_task_list;

#define PRITASK "%s %s [node=%s]"
#define PRITASKF(t) ap_task_type_names[(t)->type], (t)->name, ((t)->node ? (t)->node->name : "(null)")

void
ap_print_tasks();

void
ap_plan_evaluate(uint16_t * joypad);
