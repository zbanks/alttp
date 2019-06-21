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
struct ap_goal * ap_goal_list = &_ap_goal_list;
static struct ap_goal * ap_active_goal = NULL;
static struct ap_task _ap_task_list = {.next = &_ap_task_list, .prev = &_ap_task_list};
struct ap_task * ap_task_list = &_ap_task_list;
static bool ap_new_goals = true;

static struct ap_goal * item_goal_flippers = NULL;
static struct ap_goal * item_goal_bombs = NULL;

void
ap_print_goals()
{
    printf("Goal list: (current index=%#x)\n", XYMAPSCREEN(ap_link_xy()));
    for (struct ap_goal * goal = ap_goal_list->next; goal != ap_goal_list; goal = goal->next) {
        if (goal->node != NULL && goal->node->adjacent_node != NULL)
            printf("    * " PRIGOAL " to %s\n", PRIGOALF(goal), goal->node->adjacent_node->name);
        else
            printf("    * " PRIGOAL " to umapped\n", PRIGOALF(goal));
        
    }
    printf("\n");
}

static struct ap_goal *
ap_goal_append()
{
    struct ap_goal * goal = calloc(1, sizeof *goal);
    assert(goal != NULL);

    LL_PUSH(ap_goal_list, goal);
    ap_graph_init(&goal->graph);
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
    if (type == GOAL_EXPLORE) {
        LOG("New explore, attr: %#x", node->tile_attr);
    }
    if (type == GOAL_EXPLORE && (ap_tile_attrs[node->tile_attr] & TILE_ATTR_SWIM)) {
        ap_graph_add_prereq(&goal->graph, &item_goal_flippers->graph);
    }
    return goal;
}

static struct ap_task *
ap_task_append()
{
    struct ap_task * task = NONNULL(calloc(1, sizeof *task));
    LL_PUSH(ap_task_list, task);
    return task;
}

static struct ap_task *
ap_task_prepend()
{
    struct ap_task * task = NONNULL(calloc(1, sizeof *task));
    LL_PREPEND(ap_task_list, task);
    return task;
}

const char *
ap_print_task(const struct ap_task * task)
{
    static char sbuf[2048];
    char * buf = sbuf;
    buf += sprintf(buf, "%s %s", ap_task_type_names[task->type], task->name);
    switch (task->type) {
    case TASK_GOTO_POINT:
    case TASK_OPEN_CHEST:
    case TASK_LIFT_POT:
    case TASK_TRANSITION:
    case TASK_STEP_OFF_SWITCH:
        buf += sprintf(buf, " [node=%s]", (task->node ? task->node->name : "(null)"));
        break;
    case TASK_SET_INVENTORY:
        buf += sprintf(buf, " [item=%#x]", task->item);
        break;
    case TASK_NONE:
        break;
    }
    return sbuf;
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
    struct ap_screen * screen = ap_update_map_screen(false);
    switch (task->type) {
    case TASK_STEP_OFF_SWITCH:
        switch (task->state) {
        case 0:
            task->timeout = 32;
            task->state++;
        case 1:
            if (!*ap_ram.link_on_switch)
                return RC_DONE;
            JOYPAD_SET(LEFT); // FIXME
            return RC_INPR;
        }
        break;
    case TASK_GOTO_POINT:
        if (task->node != NULL && ap_node_islocked(task->node)) {
            if (task->node->lock_node == NULL)
                return RC_FAIL;
            // Prepend an unlock task
            struct ap_task * new_task = ap_task_prepend(); 
            new_task->type = TASK_GOTO_POINT;
            new_task->node = task->node->lock_node;
            snprintf(new_task->name, sizeof new_task->name, "unlock %s", task->node->name);
            // Maybe we need to step off first
            new_task = ap_task_prepend(); 
            new_task->type = TASK_STEP_OFF_SWITCH;
            new_task->node = task->node->lock_node;
            snprintf(new_task->name, sizeof new_task->name, "step off");
            return RC_INPR; // XXX should there be RC_RTRY?
        }
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
            } else if (*ap_ram.push_timer != 0x20) {
                JOYPAD_SET(A);
                task->state = 2;
            } else if (*ap_ram.carrying_bit7) {
                //JOYPAD_SET(A);
                task->state = 2;
            }
            return ap_follow_targets(joypad);
        }
        break;
    case TASK_OPEN_CHEST:
        LOG("timeout: %d; state: %d; touching: %x; push: %x; item_recv_method: %x", task->timeout, task->state, *ap_ram.touching_chest, *ap_ram.push_timer, *ap_ram.item_recv_method);
        switch (task->state) {
        case 0:
            task->timeout = 64;
            task->state++;
        case 1:
            JOYPAD_SET(UP);
            if (!(*ap_ram.touching_chest & 0xF) && (*ap_ram.push_timer == 0x20)) {
                break;
            }
            task->state++;
        case 2:
        case 3:
            JOYPAD_CLEAR(UP);
            if (task->state == 2) { JOYPAD_SET(A); task->state = 3; }
            else { JOYPAD_CLEAR(A); task->state = 2; }
            if (*ap_ram.item_recv_method == 1) return RC_DONE;
        }
        break;
    case TASK_LIFT_POT:
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
        INFO("cross: st=%d timeout=%d dir=%s", task->state, task->timeout, dir_names[task->direction]);
        switch (task->state) {
        case 0:
            task->timeout = 200;
            task->state++;
        case 1:
            if (screen != task->node->screen) {
                task->timeout = 8;
                task->state++;
            } else {
                ap_joypad_setdir(joypad, task->direction);
                break;
            }
        case 2:
            if (task->timeout == 1) {
                if (screen != task->node->screen) {
                    ap_map_record_transition_from(task->node);
                    return RC_DONE;
                    //LOG("transition: %p %p %p", screen, task->node->screen, task->node->adjacent_screen[0]);
                    //LOG("transition: %s; %s; -", screen->name, task->node->screen->name);
                    //if (screen == task->node->adjacent_screen[0]) return RC_DONE;
                }
                return RC_FAIL;
            }
        }
        break;
    case TASK_SET_INVENTORY:
        INFO("inv to %u", task->item);
        if (*ap_ram.module_index != 0x0E && *ap_ram.current_item == task->item) {
            return RC_DONE;
        }
        switch (task->state) {
        case 0:
            task->timeout = 200;
            task->state++;
        case 1:
            if (*ap_ram.module_index != 0x0E) {
                if (task->timeout & 1) {
                    JOYPAD_SET(START);
                }
                break;
            }
            task->state++;
        case 2:
            if (*ap_ram.current_item != task->item) {
                if (task->timeout & 1) {
                    // XXX: Actually navigate to the right item
                    JOYPAD_SET(LEFT);
                }
                break;
            }
            task->state++;
        case 3: 
            if (task->timeout & 1) {
                JOYPAD_SET(START);
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

#define GOAL_SCORE_UNSATISFIABLE    (INT_MAX)
#define GOAL_SCORE_COMPLETE         (-2)
static int
ap_goal_score(struct ap_goal * goal)
{
    assert(goal != NULL);
    if (ap_graph_is_blocked(&goal->graph)) {
        return GOAL_SCORE_UNSATISFIABLE;
    }

    struct ap_screen * screen = ap_update_map_screen(false);

    int score = 0;
    if (goal->node != NULL) {
        struct xy link = ap_link_xy();
        score += ap_path_heuristic(link, goal->node->tl, goal->node->br);
    }
    score += goal->attempts * 100; //1000000;
    switch (goal->type) {
    case GOAL_PICKUP:
    case GOAL_CHEST:
        score += 0;
        if (goal->node->screen != screen)
            score = GOAL_SCORE_UNSATISFIABLE;
        break;
    case GOAL_EXPLORE:
        //if (goal->node->type != NODE_SWITCH)
            score += 1000;
        if (goal->node->adjacent_node != NULL)
            score = GOAL_SCORE_COMPLETE;
        break;
    case GOAL_ITEM:
        if (ap_ram.inventory_base[goal->item]) {
            score = GOAL_SCORE_COMPLETE;
        } else {
            score = GOAL_SCORE_UNSATISFIABLE;
        }
        break;
    default:
        score = GOAL_SCORE_UNSATISFIABLE;
        break;
    }

    return score;
}

static void
ap_goal_complete(struct ap_goal * goal)
{
    assert(goal != NULL);
    LOG(TERM_BOLD("Completed goal: ") TERM_GREEN(PRIGOAL), PRIGOALF(goal));

    switch (goal->type) {
    case GOAL_PICKUP:
    case GOAL_CHEST:
        //LL_EXTRACT(goal->node->node_parent, goal->node);
        //free(goal->node);
        break;
    default:
        break;
    }

    LL_EXTRACT(goal);
    ap_graph_mark_done(&goal->graph);
    //free(goal);
    if (ap_active_goal == goal)
        ap_active_goal = NULL;
}

static void
ap_goal_fail(struct ap_goal * goal)
{
    assert(goal != NULL);
    LOG(TERM_BOLD("Failed goal: ") TERM_RED(PRIGOAL), PRIGOALF(goal));
    goal->attempts++;
    if (goal->attempts > 3) {
        LOG(TERM_BOLD("Permafailing goal: ") TERM_RED(TERM_BOLD(PRIGOAL)), PRIGOALF(goal));
        LL_EXTRACT(goal);
        ap_graph_extract(&goal->graph);
    }
    if (ap_active_goal == goal)
        ap_active_goal = NULL;
}

static void
ap_goal_evaluate()
{
    ap_update_map_screen(true);
    if (ap_active_goal == NULL && !ap_new_goals)
        return;
    ap_print_goals();

retry_new_goal:;
    int min_score = INT_MAX;
    struct ap_goal * min_goal = NULL;
    for (struct ap_goal * goal = ap_goal_list->next; goal != ap_goal_list; goal = goal->next) {
        int score = ap_goal_score(goal);
        goal->last_score = score;
        if (score == GOAL_SCORE_COMPLETE) {
            struct ap_goal * g = goal;
            goal = goal->prev;
            ap_goal_complete(g);
            continue;
        }
        if (score == GOAL_SCORE_UNSATISFIABLE) {
            continue;
        }
        assert(score >= 0);
        if (score < min_score) {
            min_score = score;
            min_goal = goal;
        }
    }
    ap_active_goal = min_goal;
    if (min_goal == NULL) {
        ap_new_goals = false;
        ap_print_map_full();
        assert_bp(false);
        return;
    }

    LOG(TERM_BOLD("Active goal: ") TERM_BLUE(PRIGOAL), PRIGOALF(min_goal));

    while (LL_PEEK(ap_task_list) != NULL) {
        free(LL_POP(ap_task_list));
    }
    assert(ap_task_list->next == ap_task_list);
    assert(ap_task_list->prev == ap_task_list);
    struct ap_task * task = NULL;
    
    // Get to point
    switch (min_goal->type) {
    case GOAL_CHEST:
    case GOAL_PICKUP:
    case GOAL_EXPLORE:;
        // For debugging: set inventory
        //task = ap_task_prepend(); 
        //task->type = TASK_SET_INVENTORY;
        //task->item = 0x4;

        struct ap_node * node = min_goal->node;
        int rc = ap_pathfind_node(node);
        if (rc < 0) {
            ap_goal_fail(min_goal);
            ap_active_goal = NULL;
            goto retry_new_goal;
        }
        if (node->screen == ap_update_map_screen(false)) {
            task = ap_task_prepend(); 
            task->type = TASK_GOTO_POINT;
            task->node = node;
            snprintf(task->name, sizeof task->name, "goto point onscreen");
            //break; may not be reachable from where we started
        }

        uint64_t iter = node->pgsearch.iter;
        while (node->pgsearch.from != NULL) {
            assert(node->pgsearch.iter == iter);
            if (node->screen == node->pgsearch.from->screen) {
                task = ap_task_prepend(); 
                task->type = TASK_GOTO_POINT;
                task->node = node;
                snprintf(task->name, sizeof task->name, "goto point onscreen");
            } else {
                task = ap_task_prepend(); 
                task->type = TASK_TRANSITION;
                task->node = node->pgsearch.from;
                task->direction = node->pgsearch.from->adjacent_direction;
                snprintf(task->name, sizeof task->name, "transition %s", dir_names[task->direction]);
            }
            node = node->pgsearch.from;
        }
    default:;
    }

    switch (min_goal->type) {
    case GOAL_PICKUP:
        task = ap_task_append();
        task->type = TASK_LIFT_POT;
        task->node = min_goal->node;
        snprintf(task->name, sizeof task->name, "item");
        break;
    case GOAL_CHEST:
        task = ap_task_append();
        task->type = TASK_OPEN_CHEST;
        task->node = min_goal->node;
        snprintf(task->name, sizeof task->name, "chest");
        break;
    case GOAL_EXPLORE:
        if (min_goal->node->adjacent_direction == 0)
            break;
        task = ap_task_append(); 
        task->type = TASK_TRANSITION;
        task->node = min_goal->node;
        task->direction = min_goal->node->adjacent_direction;
        snprintf(task->name, sizeof task->name, "final transition %s", dir_names[task->direction]);
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
            ap_print_tasks();
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
            LOG("task complete: " TERM_GREEN(PRITASK), PRITASKF(task));
            free(LL_POP(ap_task_list));
            break;
        case RC_FAIL:
            LOG("task failed: " TERM_RED(PRITASK), PRITASKF(task));
            ap_goal_fail(ap_active_goal);
            break;
        case RC_INPR:
            return;
        }
    }
}

void
ap_plan_init()
{
    struct ap_goal * goal = NULL;

    item_goal_bombs = goal = ap_goal_append();
    goal->type = GOAL_ITEM;
    goal->item = 0x04;
    snprintf(goal->name, sizeof goal->name, "get: bombs");

    item_goal_flippers = goal = ap_goal_append();
    goal->type = GOAL_ITEM;
    goal->item = 0x57;
    snprintf(goal->name, sizeof goal->name, "get: flippers");
}
