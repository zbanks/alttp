#include "ap_plan.h"
#include "ap_map.h"
#include "ap_snes.h"
#include <limits.h>

const char * const ap_goal_type_names[] = {
#define X(type) [CONCAT(GOAL_, type)] = #type,
AP_GOAL_TYPE_LIST
#undef X
};

const char * const ap_task_type_names[] = {
#define X(type) [CONCAT(TASK_, type)] = #type,
AP_TASK_TYPE_LIST
#undef X
};

static struct ap_goal _ap_goal_list = {.next = &_ap_goal_list, .prev = &_ap_goal_list};
static struct ap_goal * ap_goal_list = &_ap_goal_list;
static struct ap_goal * ap_active_goal = NULL;
static struct ap_task _ap_task_list = {.next = &_ap_task_list, .prev = &_ap_task_list};
static struct ap_task * ap_task_list = &_ap_task_list;
static bool ap_new_goals = true;

void
ap_print_goals()
{
    printf("Goal list:\n");
    for (struct ap_goal * goal = ap_goal_list->next; goal != ap_goal_list; goal = goal->next) {
        printf("    * " PRIGOAL "\n", PRIGOALF(goal));
    }
    printf("\n");
}

struct ap_goal *
ap_goal_append()
{
    struct ap_goal * goal = calloc(1, sizeof *goal);
    assert(goal != NULL);

    LL_PUSH(ap_goal_list, goal);
    ap_new_goals = true;
    return goal;
}

struct ap_goal *
ap_goal_add(enum ap_goal_type type, struct ap_node * node)
{
    for (struct ap_goal * goal = ap_goal_list->next; goal != ap_goal_list; goal = goal->next) {
        if (goal->type == type && goal->node == node)
            return NULL;
    }
    struct ap_goal * goal = ap_goal_append();
    goal->type = type;
    goal->node = node;
    snprintf(goal->name, sizeof goal->name, "-");
    return goal;
}

static struct ap_task *
ap_task_append()
{
    struct ap_task * task = calloc(1, sizeof *task);
    assert(task != NULL);

    LL_PUSH(ap_task_list, task);
    return task;
}

void
ap_print_tasks()
{
    if (ap_active_goal == NULL) {
        printf("Current Goal: " PRIGOAL "\n", PRIGOALF(ap_active_goal));
    }
    printf("Task List:\n");
    for (struct ap_task * task = ap_task_list->next; task != ap_task_list; task = task->next) {
        printf("    > " PRITASK "\n", PRITASKF(task));
    }
    printf("\n");
}

// return -1 for failure, 0 for success, 1 for in-progress
static int
ap_task_evaluate(struct ap_task * task, uint16_t * joypad)
{
    int rc;
    struct ap_node * map_node = ap_update_map_node();
    switch (task->type) {
    case TASK_GOTO_POINT:
        switch (task->state) {
        case 0:
            rc = ap_pathfind_node(task->node);
            if (rc < 0) return RC_FAIL;
            task->timeout = rc + 32;
            task->state++;
        case 1:
        case 2:
            if (task->state == 2) {
                JOYPAD_CLEAR(A);
                task->state = 1;
            } else if (*ap_ram.push_dir_bitmask & 0xF) {
                JOYPAD_SET(A);
                task->state = 2;
            } else if (*ap_ram.carrying_bit7) {
                JOYPAD_SET(A);
                task->state = 2;
            }
            return ap_follow_targets(joypad);
        }
        break;
    case TASK_CHEST:
        switch (task->state) {
        case 0:
            rc = ap_pathfind_node(task->node);
            if (rc < 0) return RC_FAIL;
            task->timeout = rc + 32;
            task->state++;
        case 1:
            rc = ap_follow_targets(joypad);
            if (rc == RC_DONE) task->state++;
            else return rc;
        case 2:
            JOYPAD_SET(UP);
            if (*ap_ram.touching_chest & 0xF) task->state++;
        case 3:
        case 4:
            if (task->state == 3) { JOYPAD_SET(A); task->state = 4; }
            else { JOYPAD_CLEAR(A); task->state = 3; }
            LOG("item_recv_method: %x", *ap_ram.item_recv_method);
            if (*ap_ram.item_recv_method == 1) return RC_DONE;
        }
        break;
    case TASK_POT:
        switch (task->state) {
        case 0:
            rc = ap_pathfind_node(task->node);
            if (rc < 0) return RC_FAIL;
            task->timeout = rc + 32;
            task->state++;
        case 1:
        case 2:
            if (task->state == 1) { JOYPAD_SET(A); task->state = 2; }
            else { JOYPAD_CLEAR(A); task->state = 1; }
            return ap_follow_targets(joypad);
        }
        break;
    case TASK_TRANSITION:
        switch (task->state) {
        case 0:
            task->timeout = 128;
            task->state++;
        case 1:
            if (map_node != task->original_node) {
                task->timeout = 8;
                task->state++;
            } else {
                ap_joypad_setdir(joypad, task->direction);
            }
        case 2:
            if (task->timeout == 1) {
                if (map_node != task->original_node) {
                    LOG("transition: %p %p %p %p", map_node, task->original_node, task->node, task->node->node_parent);
                    LOG("transition: %s; %s; %s", map_node->name, task->original_node->name, task->node->name);
                    if (map_node == task->node || map_node == task->node->node_parent) return RC_DONE;
                }
                return RC_FAIL;
            }
        }
        break;
    case TASK_NONE:
    default:
        LOG("unknown task type %d", task->type);
        return RC_FAIL;
    }
    if (task->timeout-- <= 0) {
        LOG("task timeout: " PRITASK, PRITASKF(task));
        return RC_FAIL;
    }
    return RC_INPR;
}

static int
ap_goal_score(struct ap_goal * goal)
{
    assert(goal != NULL);
    struct ap_node * map_node = ap_update_map_node();

    int score = 0;
    struct xy link = ap_link_xy();
    if (goal->node != NULL) {
        score += ap_path_heuristic(link, goal->node->tl, goal->node->br);
    }
    score += goal->attempts * 1000000;
    switch (goal->type) {
    case GOAL_ITEM:
        score += 0;
        if (goal->node->node_parent != map_node)
            score = INT_MAX;
        break;
    case GOAL_EXPLORE:
        score += 1000;
        if (goal->node->node_parent != map_node)
            score = INT_MAX;
        if (goal->node->node_adjacent == NULL)
            score = INT_MAX;
        break;
    default:
        score = INT_MAX;
        break;
    }

    return score;
}

static void
ap_goal_complete(struct ap_goal * goal)
{
    assert(goal != NULL);
    LOG("Completed goal: " PRIGOAL, PRIGOALF(goal));

    switch (goal->type) {
    case GOAL_ITEM:
        LL_EXTRACT(goal->node->node_parent, goal->node);
        free(goal->node);
        break;
    default:
        break;
    }

    LL_EXTRACT(ap_goal_list, goal);
    free(goal);
    if (ap_active_goal == goal)
        ap_active_goal = NULL;
}

static void
ap_goal_fail(struct ap_goal * goal)
{
    assert(goal != NULL);
    LOG("Failed goal: " PRIGOAL, PRIGOALF(goal));
    goal->attempts++;
    if (goal->attempts > 3) {
        LOG("Permafailing goal: " PRIGOAL, PRIGOALF(goal));
        LL_EXTRACT(ap_goal_list, goal);
        free(goal);
    }
    if (ap_active_goal == goal)
        ap_active_goal = NULL;
}

static int
ap_plan_goto_node(struct ap_node * node)
{
    struct ap_node * map_node = ap_update_map_node();
    //TODO
    return -1;
}

static void
ap_goal_evaluate()
{
    ap_new_goals = false;
    ap_print_goals();
    int min_score = INT_MAX;
    struct ap_goal * min_goal = NULL;
    for (struct ap_goal * goal = ap_goal_list->next; goal != ap_goal_list; goal = goal->next) {
        int score = ap_goal_score(goal);
        if (score < min_score) {
            min_score = score;
            min_goal = goal;
        }
    }
    ap_active_goal = min_goal;
    if (min_goal == NULL)
        return;

    LOG("Active goal: " PRIGOAL, PRIGOALF(min_goal));

    while (LL_PEEK(ap_task_list) != NULL) {
        free(LL_POP(ap_task_list));
    }
    assert(ap_task_list->next == ap_task_list);
    assert(ap_task_list->prev == ap_task_list);
    struct ap_task * task = NULL;
    switch (min_goal->type) {
    case GOAL_ITEM:
        if (ap_tile_attrs[min_goal->node->tile_attr] & TILE_ATTR_CHST) {
            task = ap_task_append();
            task->type = TASK_GOTO_POINT;
            task->node = min_goal->node;
            snprintf(task->name, sizeof task->name, "item goto local");

            task = ap_task_append();
            task->type = TASK_CHEST;
            task->node = min_goal->node;
            snprintf(task->name, sizeof task->name, "item chest");
        } else {
            task = ap_task_append();
            task->type = TASK_POT;
            task->node = min_goal->node;
            snprintf(task->name, sizeof task->name, "item pot");
        }
        break;
    case GOAL_EXPLORE:
        task = ap_task_append();
        task->type = TASK_GOTO_POINT;
        task->node = min_goal->node;
        snprintf(task->name, sizeof task->name, "explore point");

        task = ap_task_append();
        task->type = TASK_TRANSITION;
        task->node = min_goal->node->node_adjacent;
        assert(task->node != NULL);
        task->original_node = min_goal->node->node_parent;
        assert(task->original_node != NULL);
        task->direction = min_goal->node->adjacent_direction;
        snprintf(task->name, sizeof task->name, "screen transition");
        break;
    default:
        LOG("Invalid goal type: %d", min_goal->type);
    }
}

void
ap_plan_evaluate(uint16_t * joypad)
{
    while (true) {
        if (ap_active_goal == NULL) {
            ap_goal_evaluate();
            if (ap_active_goal == NULL) return;
        }

        struct ap_task * task = LL_PEEK(ap_task_list);
        if (task == NULL) {
            ap_goal_complete(ap_active_goal);
            task = LL_PEEK(ap_task_list);
            if (task == NULL) return;
        }

        enum rc rc = ap_task_evaluate(task, joypad);
        if (rc != RC_INPR) {
            ap_print_tasks();
        }
        switch (rc) {
        case RC_DONE:
            LOG("task complete: " PRITASK, PRITASKF(task));
            free(LL_POP(ap_task_list));
            break;
        case RC_FAIL:
            LOG("task failed: " PRITASK, PRITASKF(task));
            ap_goal_fail(ap_active_goal);
            break;
        case RC_INPR:
            return;
        }
    }
}
