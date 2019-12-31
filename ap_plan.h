#pragma once
#include "ap_macro.h"
#include "ap_req.h"

#define AP_GOAL_TYPE_LIST \
    X(NONE) \
    X(PICKUP) \
    X(CHEST) \
    X(EXPLORE) \
    X(ITEM) \
    X(NPC) \
    X(SCRIPT) \

enum ap_goal_type {
#define X(type) CONCAT(GOAL_, type),
AP_GOAL_TYPE_LIST
#undef X
    _GOAL_TYPE_MAX
};

extern const char * const ap_goal_type_names[];
struct ap_goal;
struct ap_goal {
    struct ap_goal * next;
    struct ap_goal * prev;

    enum ap_goal_type type;
    char name[32];

    struct ap_node * node;
    uint8_t item;
    int attempts;
    int last_score;

    //struct ap_graph graph;
    struct ap_req req;
};
extern struct ap_goal * ap_goal_list;

/*
static struct ap_goal *
ap_goal_from_graph(struct ap_graph * graph) {
    uintptr_t p = (uintptr_t) graph;
    p -= (uintptr_t) &((struct ap_goal *) 0)->graph;
    return (struct ap_goal *) p;
}
*/

#define PRIGOAL "%s [node=" PRINODE "]"
#define PRIGOALF(g) ap_goal_type_names[(g)->type], PRINODEF((g)->node)

void
ap_print_goals(bool no_limit);

struct ap_goal *
ap_goal_add(enum ap_goal_type type, struct ap_node * node);

#define AP_TASK_TYPE_LIST \
    X(NONE) \
    X(GOTO_POINT) \
    X(TRANSITION) \
    X(OPEN_CHEST) \
    X(LIFT_POT) \
    X(SET_INVENTORY) \
    X(STEP_OFF_SWITCH) \
    X(TALK_NPC) \
    X(SCRIPT_SEQUENCE) \
    X(SCRIPT_KILLALL) \
    X(SCRIPT_KILLDROPS) \
    X(BOMB) \

enum ap_task_type {
#define X(type) CONCAT(TASK_, type),
AP_TASK_TYPE_LIST
#undef X
};
extern const char * const ap_task_type_names[];

struct ap_task;
struct ap_task {
    struct ap_task * next;
    struct ap_task * prev;

    enum ap_task_type type;
    char name[32];

    int timeout;
    int state;

    struct ap_node * node;
    uint8_t direction;
    uint8_t item;
};
extern struct ap_task * ap_task_list;

const char *
ap_print_task(const struct ap_task * task);

#define PRITASK "%s"
#define PRITASKF(t) ap_print_task(t)

void
ap_print_tasks();

void
ap_plan_evaluate(uint16_t * joypad);

void
ap_plan_init();
