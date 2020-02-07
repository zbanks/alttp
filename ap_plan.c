#include "ap_plan.h"
#include "ap_item.h"
#include "ap_map.h"
#include "ap_snes.h"
#include <limits.h>

#define GOAL_SCORE_UNSATISFIABLE    (INT_MAX-1)
#define GOAL_SCORE_GT_LIMIT         (INT_MAX)
#define GOAL_SCORE_COMPLETE         (-2)
static int ap_goal_score(struct ap_goal * goal, int max_cost);

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

static int ap_goal_count[_GOAL_TYPE_MAX];
static int ap_goal_completed[_GOAL_TYPE_MAX];

void
ap_print_goals(bool no_limit)
{
    FILE * f = fopen("goals.txt", "w+");
    assert(f != NULL);
    fprintf(f, "Goal list: (current index=%#x)\n", XYMAPSCREEN(ap_link_xy()));
    int max_score = GOAL_SCORE_GT_LIMIT;
    for (struct ap_goal * goal = ap_goal_list->next; goal != ap_goal_list; goal = goal->next) {
        int score = ap_goal_score(goal, max_score);
        goal->last_score = score;
        char score_str[16];
        if (score == GOAL_SCORE_COMPLETE) {
            snprintf(score_str, sizeof(score_str), TERM_GREEN("compl"));
        } else if (score == GOAL_SCORE_UNSATISFIABLE) {
            snprintf(score_str, sizeof(score_str), TERM_RED("unsat"));
        } else if (score == GOAL_SCORE_GT_LIMIT) {
            snprintf(score_str, sizeof(score_str), TERM_BLUE("limit"));
        } else {
            if (!no_limit) {
                max_score = MIN(score, max_score);
            }
            snprintf(score_str, sizeof(score_str), TERM_BLUE("%5d"), score);
        }
        fprintf(f, "    " TERM_BOLD("*") " %s " PRIGOAL, score_str, PRIGOALF(goal));
        if (goal->type == GOAL_EXPLORE) {
            if (goal->node != NULL && goal->node->adjacent_node != NULL) {
                fprintf(f, " to %s %p", goal->node->adjacent_node->name, goal->node);
            } else {
                fprintf(f, " to umapped %p", goal->node);
            }
        } else {
            if (goal->node != NULL && goal->node->screen != NULL) {
                fprintf(f, " on %s", goal->node->screen->name);
            }
        }
        fprintf(f, " Needs: %s", ap_req_print(&goal->req));
        fprintf(f, "\n");
    }
    fprintf(f, "\n");
    fprintf(f, "Stats: ");
    for (int i = 0; i < _GOAL_TYPE_MAX; i++) {
        fprintf(f, "%s %d/%d", ap_goal_type_names[i], ap_goal_completed[i], ap_goal_count[i]);
        if (i != _GOAL_TYPE_MAX - 1) {
            fprintf(f, ", ");
        }
    }

    fprintf(f, "\n");
    fflush(f);
    rewind(f);
    static char buf[4096];
    size_t rc;
    while ((rc = fread(buf, 1, sizeof(buf), f)) > 0) {
        fwrite(buf, 1, rc, stdout);
    }
    fclose(f);
}

static struct ap_goal *
ap_goal_append()
{
    struct ap_goal * goal = calloc(1, sizeof *goal);
    assert(goal != NULL);

    LL_INIT(goal);
    LL_PUSH(ap_goal_list, goal);
    //ap_graph_init(&goal->graph, goal->name);
    ap_req_init(&goal->req);
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
    snprintf(goal->name, sizeof goal->name, PRIGOAL, PRIGOALF(goal));
    if (type == GOAL_EXPLORE) {
        LOG("New explore, attr: %#x", node->tile_attr);
    } else if (type == GOAL_SCRIPT) {
        if (goal->node->script->start_item == INVENTORY_BOMBS) {
            ap_req_require(&goal->req, 0, REQUIREMENT_BOMBS);
        }
        if (goal->node->script->type == SCRIPT_KILLALL) {
            ap_req_require(&goal->req, 0, REQUIREMENT_SWORD);
        }
    }
    /*
        if (!(type == GOAL_NPC && node->sprite_type == 0x73 && node->sprite_subtype == 0x100)) {
            // Don't do anything but explore until we get sword from uncle
            ap_req_require(&goal->req, 0, REQUIREMENT_SWORD);
        }
        */
    if (type == GOAL_NPC && node->sprite_type == 0x16 && node->sprite_subtype == 0x0000) {
        // Sahashrala needs the green pendant before giving us an item
        ap_req_require(&goal->req, 0, REQUIREMENT_GREEN_PENDANT);
    }
    if (type == GOAL_EXPLORE) {
        uint16_t attrs = ap_tile_attrs[node->tile_attr];
        if (attrs & TILE_ATTR_SWIM) {
            ap_req_require(&goal->req, 0, REQUIREMENT_FLIPPERS);
        }
        if (attrs & TILE_ATTR_BONK) {
            ap_req_require(&goal->req, 0, REQUIREMENT_BOOTS);
        }
        if (node->tile_attr == 0x55) {
            ap_req_require(&goal->req, 0, REQUIREMENT_GLOVES_1);
        }
        if (strcmp(node->name, "door 0x24") == 0) {
            // Castle door to Agahnim
            ap_req_require(&goal->req, 0, REQUIREMENT_MASTER_SWORD);
        }
        if (strcmp(node->name, "door 0x09") == 0) {
            // Desert Palace
            ap_req_require(&goal->req, 0, REQUIREMENT_BOOK);
        }
        if (strcmp(node->name, "door 0x63") == 0) {
            // Big-bomb Fairy
            ap_req_require(&goal->req, 0, REQUIREMENT_FIVESIX_CRYSTALS);
        }
    }
    ap_goal_count[goal->type]++;
    return goal;
}

static struct ap_task *
ap_task_append()
{
    struct ap_task * task = NONNULL(calloc(1, sizeof *task));
    LL_INIT(task);
    LL_PUSH(ap_task_list, task);
    return task;
}

static struct ap_task *
ap_task_prepend()
{
    struct ap_task * task = NONNULL(calloc(1, sizeof *task));
    LL_INIT(task);
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
    case TASK_TALK_NPC:
        buf += sprintf(buf, " [node=" PRINODE "]", PRINODEF(task->node));
        break;
    case TASK_SCRIPT_SEQUENCE:
    case TASK_SCRIPT_KILLALL:
    case TASK_SCRIPT_KILLDROPS:
        if (task->node != NULL) {
            buf += sprintf(buf, " [script=%s start_tl=" PRIXYV "]", task->node->script->name, PRIXYVF(task->node->script->start_tl));
        }
        break;
    case TASK_SET_INVENTORY:
        buf += sprintf(buf, " [item=%#x]", task->item);
        break;
    case TASK_BOMB:
        buf += sprintf(buf, " [bomb=%s]", task->node->name);
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

static int
ap_task_follow_targets(struct ap_task * task, uint16_t * joypad)
{
    enum ap_inventory equip = *ap_ram.current_item;
    int rc = ap_follow_targets(joypad, &equip);
    if (rc != RC_INPR) return rc;
    if (equip != *ap_ram.current_item) {
        struct ap_task * new_task = ap_task_prepend(); 
        new_task->type = TASK_SET_INVENTORY;
        new_task->item = equip;
        snprintf(new_task->name, sizeof new_task->name, "equip to follow_targets");

        task->state = 0;
        task->timeout = 1;
    }
    return rc;
}

// return -1 for failure, 0 for success, 1 for in-progress
static int
ap_task_evaluate(struct ap_task * task, uint16_t * joypad)
{
    int rc;
    struct ap_screen * screen = ap_update_map_screen(false);
    struct xy link = ap_link_xy();
    bool unlockable = false;
    const struct ap_room_tag * unlock_tag = NULL;
    if (task->node != NULL && ap_node_islocked(task->node, &unlockable, &unlock_tag)) {
        if (!unlockable) {
            LOG("Cannot unlock door");
            return RC_FAIL;
        }

        if (task->node->type == NODE_TRANSITION && (ap_door_attrs[task->node->door_type] & (DOOR_ATTR_SKEY | DOOR_ATTR_BKEY))) {
            // pass
        } else if (task->node->type == NODE_TRANSITION && (ap_door_attrs[task->node->door_type] & DOOR_ATTR_BOMB) && task->type != TASK_TRANSITION) {
            // pass
        } else {
            if (task->node->type == NODE_TRANSITION &&
                ((ap_door_attrs[task->node->door_type] & DOOR_ATTR_BOMB) ||
                 (task->node->lock_node != NULL && task->node->lock_node->type == NODE_OVERLAY && task->node->lock_node->overlay_index != 0x5B))) {
                // Try bombing it
                struct ap_node * bomb_node = task->node->lock_node;
                if (bomb_node == NULL) {
                    bomb_node = task->node;
                }

                struct ap_task * new_task = ap_task_prepend(); 
                new_task->type = TASK_GOTO_POINT;
                new_task->node = task->node;
                snprintf(new_task->name, sizeof new_task->name, "goto bombed %s", task->node->name);

                new_task = ap_task_prepend(); 
                new_task->type = TASK_BOMB;
                new_task->node = bomb_node;
                snprintf(new_task->name, sizeof new_task->name, "bomb open");

                new_task = ap_task_prepend(); 
                new_task->type = TASK_SET_INVENTORY;
                new_task->item = INVENTORY_BOMBS;
                snprintf(new_task->name, sizeof new_task->name, "set bombs");

                if (!XYIN(link, bomb_node->tl, bomb_node->br)) {
                    new_task = ap_task_prepend(); 
                    new_task->type = TASK_GOTO_POINT;
                    new_task->node = bomb_node;
                    snprintf(new_task->name, sizeof new_task->name, "goto bomb %s", task->node->name);
                }
            } else if (unlock_tag != NULL && unlock_tag->action == ROOM_ACTION_SWITCH_TOGGLE) {
                struct ap_task * new_task = ap_task_prepend(); 
                if (task->type != TASK_GOTO_POINT) {
                    new_task->type = TASK_GOTO_POINT;
                    new_task->node = task->node;
                    snprintf(new_task->name, sizeof new_task->name, "goto %s", task->node->name);
                    new_task = ap_task_prepend(); 
                }
                // Prepend an unlock task
                new_task->type = TASK_GOTO_POINT;
                new_task->node = task->node->lock_node;
                snprintf(new_task->name, sizeof new_task->name, "unlock %s", task->node->name);

                // Maybe we need to step off first
                new_task = ap_task_prepend(); 
                new_task->type = TASK_STEP_OFF_SWITCH;
                new_task->node = task->node->lock_node;
                snprintf(new_task->name, sizeof new_task->name, "step off");
            } else if (unlock_tag != NULL &&
                    (unlock_tag->action == ROOM_ACTION_KILL_ENEMY || unlock_tag->action == ROOM_ACTION_CLEAR_LEVEL)) {
                // XXX CLEAR_LEVEL is separate from KILL_ENEMY
                struct ap_task * new_task = ap_task_prepend(); 
                if (task->type != TASK_GOTO_POINT) {
                    new_task->type = TASK_GOTO_POINT;
                    new_task->node = task->node;
                    snprintf(new_task->name, sizeof new_task->name, "goto %s", task->node->name);
                    new_task = ap_task_prepend(); 
                }

                // Prepend an killall task
                new_task->type = TASK_SCRIPT_KILLALL;
                snprintf(new_task->name, sizeof new_task->name, "killall");

                // Spawn the stalfos
                if (task->node->screen->id == 0x1580) {
                    for (struct ap_node * node = screen->node_list->next; node != screen->node_list; node = node->next) {
                        if (node->type == NODE_ITEM && node->tile_attr == 0x74) {
                            new_task = ap_task_prepend(); 
                            new_task->type = TASK_LIFT_POT;
                            new_task->node = node;
                            snprintf(new_task->name, sizeof new_task->name, "spawn");
                            break;
                        }
                    }
                }
            } else {
                LOG("Don't know how to unlock door (%s)", ap_room_tag_print(unlock_tag));
                return RC_FAIL;
            }

            LOGB("Locked/bombable door; prepending open steps");
            *joypad = 0;
            return RC_INPR; // XXX should there be RC_RTRY?
        }
    }

    uint8_t module_index = *ap_ram.module_index;
    uint8_t submodule_index = *ap_ram.submodule_index;
    // Dialog, Textbox, Inventory Screen
    if (module_index == 0x0E) {
        if (submodule_index == 0x01) {
            // Inventory
            if (task->type != TASK_SET_INVENTORY) {
                JOYPAD_MASH(START, 0x01);
            }
        } else {
            // Dialog?
            LOGB("Mashing dialog; submodule = %#x", submodule_index);
            JOYPAD_MASH(A, 0x01);
        }
    }

    switch (task->type) {
    case TASK_STEP_OFF_SWITCH:
        if (!*ap_ram.link_on_switch && task->state < 5) {
            task->state = 5;
            task->timeout = 16;
        }
        switch (task->state) {
        case 0:
            task->timeout = 32;
            task->state++;
            task->state += rand() % 4;
            break;
            // FIXME
            // Going in a random direction means it usually works if we retry a few times
        case 1: JOYPAD_SET(LEFT); return RC_INPR;
        case 2: JOYPAD_SET(RIGHT); return RC_INPR;
        case 3: JOYPAD_SET(UP); return RC_INPR;
        case 4: JOYPAD_SET(DOWN); return RC_INPR;
        case 5:
            if (task->timeout == 1)
                return RC_DONE;
        }
        break;
    case TASK_GOTO_POINT:
        switch (task->state) {
        case 0:
            rc = ap_pathfind_node(task->node, true, 0);
            if (rc < 0) {
                LOG("ap_pathfind_node failed");
                return RC_FAIL;
            }
            task->timeout = rc + 32;
            task->state++;
        case 1:
        case 2:
            /*
            if (task->state == 2) {
                JOYPAD_CLEAR(A);
                task->state = 1;
            } else if (*ap_ram.push_timer != 0x20) {
                // TODO: Need a way of peaking ahead here to see if we need to use hammer
                // then we can insert a TASK_SET_INVENTORY
                JOYPAD_SET(A);
                task->state = 2;
            } else if (*ap_ram.carrying_bit7) {
                JOYPAD_SET(A);
                task->state = 2;
            }
            */
            rc = ap_task_follow_targets(task, joypad);
            if (rc != RC_INPR) return rc;
        }
        break;
    case TASK_OPEN_CHEST:
        LOG("timeout: %d; state: %d; touching: %x; push: %x; item_recv_method: %x; recving_item: %#x", task->timeout, task->state, *ap_ram.touching_chest, *ap_ram.push_timer, *ap_ram.item_recv_method, *ap_ram.recving_item);
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
            if (*ap_ram.item_recv_method == 1) {
                ap_item_loc_set_raw(task->node->item_loc, *ap_ram.recving_item);
                task->timeout = 64;
                task->state = 4;
            }
            break;
        case 4:
            if (*ap_ram.item_recv_method != 1) return RC_DONE;
        }
        break;
    case TASK_TALK_NPC:
        LOG("timeout: %d; state: %d; recving: %x; push: %x; item_recv_method: %x; link_state: %x; recving_item: %#x", task->timeout, task->state, *ap_ram.recving_item, *ap_ram.push_timer, *ap_ram.item_recv_method, *ap_ram.link_state, *ap_ram.recving_item);
        switch (task->state) {
        case 0:
            task->timeout = 64;
            task->state++;
        case 1:
            JOYPAD_SET(UP);
            JOYPAD_MASH(A, 1);
            if (*ap_ram.link_state == LINK_STATE_RECVING_ITEM || *ap_ram.link_state == LINK_STATE_RECVING_ITEM2 || *ap_ram.recving_item) {
                // Holding item (uncle)
                LOGB("holding item");
                ap_item_loc_set_raw(task->node->item_loc, *ap_ram.recving_item);
                task->timeout = 128;
                task->state++;
            }
            if (task->node->sprite_type == 0x73) {
                // Return success early from the uncle
                return RC_DONE;
            }
            break;
        case 2:
            JOYPAD_CLEAR(UP);
            if (*ap_ram.link_state != LINK_STATE_RECVING_ITEM && *ap_ram.link_state != LINK_STATE_RECVING_ITEM2) {
                return RC_DONE;
            }
            break;
        }
        break;
    case TASK_LIFT_POT:
        switch (task->state) {
        case 0:
            rc = ap_pathfind_node(task->node, true, 0);
            if (rc < 0) {
                LOG("ap_pathfind_node failed");
                return RC_FAIL;
            }
            task->timeout = rc + 32;
            task->state++;
        case 1:
        case 2:
            if (task->state == 1) { JOYPAD_SET(A); task->state = 2; }
            else { JOYPAD_CLEAR(A); task->state = 1; }
            rc = ap_task_follow_targets(task, joypad);
            if (rc != RC_INPR) return rc;
        }
        break;
    case TASK_TRANSITION:
        INFO("cross: st=%d timeout=%d dir=%s", task->state, task->timeout, dir_names[task->direction]);
        switch (task->state) {
        case 0:
            task->timeout = 200;
            task->state++;
        case 1:
        case 2:
            if (screen != task->node->screen) {
                task->timeout = 8;
                task->state = 3;
            } else {
                if (task->node->tile_attr == 0x55 && *ap_ram.push_timer != 0x20) {
                    JOYPAD_SET(A);
                    if (task->state == 1) {
                        task->timeout += 500;
                        task->state = 2;
                    }
                }
                ap_joypad_setdir(joypad, task->direction);
                break;
            }
        case 3:
        case 4:
            if (submodule_index == 0x10) {
                task->timeout++;
                break;
            }
            if (task->node->tile_attr == 0x55 && *ap_ram.push_timer != 0x20) {
                LOG("Push timer in transition!");
                JOYPAD_SET(A);
                if (task->state == 3) {
                    task->timeout += 500;
                    task->state = 4;
                }
            }
            if (task->timeout == 1) {
                if (screen != task->node->screen) {
                    ap_map_record_transition_from(task->node);
                    return RC_DONE;
                    //LOG("transition: %p %p %p", screen, task->node->screen, task->node->adjacent_screen[0]);
                    //LOG("transition: %s; %s; -", screen->name, task->node->screen->name);
                    //if (screen == task->node->adjacent_screen[0]) return RC_DONE;
                }
                LOG("Failed to transition in time");
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
    case TASK_BOMB:;
        bool active_bomb = false;
        for (size_t i = 0; i < N_ANCILLIA; i++) {
            if (ap_ancillia[i].type == 0x07) {
                active_bomb = true;
                break;
            }
        }
        switch (task->state) {
        case 0:
            if (*ap_ram.current_item != INVENTORY_BOMBS) {
                LOG("Bombs not equipped");
                return RC_FAIL;
            }
            if (*ap_ram.inventory_bombs == 0) {
                LOG("No bombs in inventory");
                return RC_FAIL;
            }
            task->timeout = 200;
            task->state++;
            if (task->node != NULL && task->node->adjacent_direction) {
                ap_joypad_setdir(joypad, task->node->adjacent_direction);
            }
            break;
        case 1:
            JOYPAD_SET(Y);
            task->state++;
            break;
        case 2:
            JOYPAD_CLEAR(Y);
            if (active_bomb) {
                task->state++;
            }
            break;
        case 3:
            // TODO: Dodge the bomb
            if (!active_bomb) {
                return RC_DONE;
            }
            break;
        }
        break;
    case TASK_SCRIPT_SEQUENCE:
        switch (task->state) {
        case 0:
            rc = ap_set_script(task->node->script);
            if (rc < 0) {
                LOG("ap_set_script failed: %s", task->node->script->sequence);
                return RC_FAIL;
            }
            task->timeout = rc + 32;
            task->state++;
        case 1:
            rc = ap_task_follow_targets(task, joypad);
            if (rc != RC_INPR) return rc;
        }
        break;
    case TASK_SCRIPT_KILLALL:
    case TASK_SCRIPT_KILLDROPS:
        if (*ap_ram.inventory_sword == 0) {
            ap_req_require(&ap_active_goal->req, 0, REQUIREMENT_SWORD);
            return RC_FAIL;
        }
        if (task->state == -100) {
            if (task->timeout == 1) {
                ap_update_map_screen(true);
                return RC_DONE;
            }
            break;
        }
        if (ap_ram.overlord_types[7] == 0x19) {
            if (*ap_ram.room_chest_state & 0x80) {
                return RC_DONE;
            }
            // Wait for armos knights to spawn
            //task->timeout = 2;
            //if (!ap_sprites[0].active) {
            //    break;
            //}
        }
        bool no_sword = false;
        if (task->state == 0 || task->timeout == 1) {
            size_t target = -1;
            for (size_t i = 0; i < 16; i++) {
                if (!ap_sprites[i].active)
                    continue;
                if (!XYIN(ap_sprites[i].tl, screen->tl, screen->br))
                    continue;
                if (task->type == TASK_SCRIPT_KILLDROPS && ap_sprites[i].drop == 0)
                    continue;
                target = i;
                LOGB("Targeting sprite #%zu", target);
                break;
            }
            if (target == (size_t)-1) {
                // Almost done; wait ~120 frames for loot to drop
                LOG("Done killing; waiting 128 frames for loot to drop");
                task->state = -100;
                task->timeout = 128;
                break;
            }
            if (ap_sprites[target].attrs & SPRITE_ATTR_VBOW) {
                if (*ap_ram.current_item != INVENTORY_BOW) {
                    struct ap_task * new_task = ap_task_prepend(); 
                    new_task->type = TASK_SET_INVENTORY;
                    new_task->item = INVENTORY_BOW;
                    snprintf(new_task->name, sizeof new_task->name, "set bow to kill %zu", target);

                    task->state = 0;
                    task->timeout = 0;
                    return RC_INPR;
                }
                if (ap_sprites[target].type == 0x84) {
                    if (ap_sprites[target].interaction == 0x07) {
                        JOYPAD_MASH(Y, 0x04);
                    }
                } else {
                    JOYPAD_MASH(Y, 0x08);
                }

                no_sword = true;
            }
            rc = ap_pathfind_sprite(target);
            if (rc == RC_FAIL) {
                LOG("ap_pathfind_sprite failed");
                return RC_FAIL;
            }
            task->timeout = MIN(20, MAX(8, rc));
            task->state++;
        } 
        if (task->state == 8000) { // Actual timeout
            LOG("timeout");
            return RC_FAIL;
        }
        rc = ap_task_follow_targets(task, joypad);
        if (rc == RC_FAIL) {
            LOG("ap_follow_targets failed");
            return RC_FAIL;
        }
        if (no_sword) {
            JOYPAD_CLEAR(B);
        }
        break;
    case TASK_NONE:
    default:
        LOG("unknown task type %d", task->type);
        assert_bp(false);
        return RC_FAIL;
    }
    if (task->timeout-- <= 0) {
        LOG("task timeout: " PRITASK, PRITASKF(task));
        return RC_FAIL;
    }
    return RC_INPR;
}

static int
ap_goal_score(struct ap_goal * goal, int max_score)
{
    assert(goal != NULL);

    struct ap_screen * screen = ap_update_map_screen(false);

    /*
    bool is_accessible = false;
    for (struct ap_node * node = screen->node_list->next; node != screen->node_list; node = node->next) {
        if (node->type == NODE_TRANSITION && node->adjacent_node != NULL) {
            is_accessible = true;
            break;
        }
    }
    if (!is_accessible) {
        return GOAL_SCORE_UNSATISFIABLE;
    }
    */
    if (!ap_req_is_satisfied(&goal->req)) {
        return GOAL_SCORE_UNSATISFIABLE;
    }

    int score = 0;

    if (*ap_ram.inventory_sword == 0 && goal->type != GOAL_NPC) {
        score += 10000;
    }
    /*
    if (*ap_ram.inventory_sword == 0 && !(goal->node == NULL || goal->node->sprite_type == 0x73 || goal->node->sprite_subtype == 0x100)) {
        //score += 1000000;
        return GOAL_SCORE_UNSATISFIABLE;
    }
    */

    //if (goal->type != GOAL_SCRIPT && goal->type != GOAL_NPC) {
    //    score += 1000000;
    //}
    score += goal->attempts * 100; //1000000;
    switch (goal->type) {
    case GOAL_PICKUP:
        score += 0;
        if (goal->node->screen != screen) {
            return GOAL_SCORE_UNSATISFIABLE;
        }
        if (ap_map_attr(goal->node->tl) == 0x27) {
            return GOAL_SCORE_COMPLETE;
        }
        break;
    case GOAL_CHEST:;
        uint8_t attr = goal->node->tile_attr;
        assert(attr >= 0x58 && attr <= 0x5D);
        assert(goal->node->screen->dungeon_room != (uint16_t) -1);
        uint16_t room_state = ap_ram.sram_room_state[goal->node->screen->dungeon_room];
        if (room_state & (1 << (attr - 0x58 + 4))) {
            return GOAL_SCORE_COMPLETE;
        }
        break;
    case GOAL_NPC:
        //score += 1000;
        break;
    case GOAL_SCRIPT:
        break;
    case GOAL_EXPLORE:
        //if (goal->node->screen->name[0] != 'H') {
        //    score += 100000;
        //}
        //if (goal->node->type != NODE_SWITCH)
            //score += 1000;
        if (goal->node->adjacent_node != NULL) {
            return GOAL_SCORE_COMPLETE;
        }
        break;
    case GOAL_ITEM:
        if (ap_ram.inventory_base[goal->item]) {
            return GOAL_SCORE_COMPLETE;
        } else {
            return GOAL_SCORE_UNSATISFIABLE;
        }
        break;
    default:
        return GOAL_SCORE_UNSATISFIABLE;
        break;
    }

    /*
    if (ap_graph_is_blocked(&goal->graph)) {
        return GOAL_SCORE_UNSATISFIABLE;
    }
    */

    if (goal->node != NULL) {
        //struct xy link = ap_link_xy();
        //int heuristic = ap_path_heuristic(link, goal->node->tl, goal->node->br);
        int max_distance = max_score - score;
        if (max_distance <= 0) {
            return GOAL_SCORE_GT_LIMIT;
        }
        int distance = ap_pathfind_node(goal->node, false, max_distance);
        if (distance < 0) {
            return GOAL_SCORE_UNSATISFIABLE;
        } else if (distance > max_distance) {
            return GOAL_SCORE_GT_LIMIT;
        }
        score += distance;
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
    case GOAL_NPC:
    case GOAL_SCRIPT:
        //LL_EXTRACT(goal->node->node_parent, goal->node);
        //free(goal->node);
        break;
    default:
        break;
    }
    ap_goal_completed[goal->type]++;

    LL_EXTRACT(goal);
    //ap_graph_mark_done(&goal->graph);
    //free(goal);
    if (ap_active_goal == goal)
        ap_active_goal = NULL;
}

static void
ap_goal_fail(struct ap_goal * goal)
{
    assert(goal != NULL);
    assert_bp(false);
    LOG(TERM_BOLD("Failed goal: ") TERM_RED(PRIGOAL), PRIGOALF(goal));
    goal->attempts++;
    if (goal->attempts > 3) {
        LOG(TERM_BOLD("Permafailing goal: ") TERM_RED(TERM_BOLD(PRIGOAL)), PRIGOALF(goal));
        LL_EXTRACT(goal);
        //ap_graph_extract(&goal->graph);
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
    ap_print_goals(false);

retry_new_goal:;
    int min_score = INT_MAX;
    struct ap_goal * min_goal = NULL;
    for (struct ap_goal * goal = ap_goal_list->next; goal != ap_goal_list; goal = goal->next) {
        int score = goal->last_score; // ap_goal_score(goal);
        if (score == GOAL_SCORE_COMPLETE) {
            struct ap_goal * g = goal;
            goal = goal->prev;
            ap_goal_complete(g);
            continue;
        }
        if (score == GOAL_SCORE_UNSATISFIABLE || score == GOAL_SCORE_GT_LIMIT) {
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
        //ap_new_goals = false;
        ap_manual_mode = true;
        ap_print_map_full();
        LOGB("No goals available; falling back to manual mode");
        //assert_bp(false);
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
    case GOAL_NPC:
    case GOAL_SCRIPT:
    case GOAL_EXPLORE:;
        // For debugging: set inventory
        //task = ap_task_prepend(); 
        //task->type = TASK_SET_INVENTORY;
        //task->item = 0x4;

        struct ap_node * node = min_goal->node;
        int rc = ap_pathfind_node(node, true, 0);
        if (rc < 0) {
            ap_goal_fail(min_goal);
            ap_active_goal = NULL;
            goto retry_new_goal;
        }

        uint64_t iter = node->pgsearch.iter;
        while (node->pgsearch.from != NULL) {
            assert_bp(node->pgsearch.iter == iter);
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
    case GOAL_NPC:
        task = ap_task_append();
        task->type = TASK_TALK_NPC;
        task->node = min_goal->node;
        snprintf(task->name, sizeof task->name, "npc");
        break;
    case GOAL_SCRIPT:;
        int item = min_goal->node->script->start_item;
        if (item > 0) {
            assert(item <= 0xff && ap_inventory_names[item] != NULL);
            task = ap_task_append();
            task->type = TASK_SET_INVENTORY;
            task->item = item;
            snprintf(task->name, sizeof task->name, "start item: %s", ap_inventory_names[item]);
        }
        task = ap_task_append();
        task->node = min_goal->node;
        enum script_type script_type = task->node->script->type;
        switch (task->node->script->type) {
        case SCRIPT_SEQUENCE: task->type = TASK_SCRIPT_SEQUENCE; break;
        case SCRIPT_KILLALL: task->type = TASK_SCRIPT_KILLALL; break;
        case SCRIPT_KILLDROPS: task->type = TASK_SCRIPT_KILLDROPS; break;
        default: assert(0);
        }
        snprintf(task->name, sizeof task->name, "script");
        break;
    default:
        LOG("Invalid goal type: %d", min_goal->type);
        assert_bp(false);
        break;
    }
}

void
ap_plan_evaluate(uint16_t * joypad)
{
    while (true) {
        if (ap_active_goal == NULL) {
            ap_goal_evaluate();
            if (ap_active_goal == NULL) goto done;
            ap_print_tasks();
        }

        struct ap_task * task = LL_PEEK(ap_task_list);
        if (task == NULL) {
            ap_goal_complete(ap_active_goal);
            task = LL_PEEK(ap_task_list);
            if (task == NULL) goto done;
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
        case RC_INPR:;
            goto done;
        }
    }

done:;
    // Debug
    const char *goal_name = "(no goal)";
    if (ap_active_goal != NULL) {
        goal_name = ap_goal_type_names[ap_active_goal->type];
    }
    const char *task_name = "(no task)";
    int task_timeout = 0;
    if (LL_PEEK(ap_task_list) != NULL) {
        task_name = ap_task_type_names[LL_PEEK(ap_task_list)->type];
        task_timeout = LL_PEEK(ap_task_list)->timeout;
    }
    struct ap_screen * screen = ap_update_map_screen(NULL);
    INFO("Plan: %s via %s (%d)", goal_name, task_name, task_timeout);
}

void
ap_plan_init()
{
}
