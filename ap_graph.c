#include "ap_graph.h"
#include "ap_map.h"
#include "ap_math.h"
#include "ap_macro.h"
#include "ap_snes.h"
#include "ap_plan.h"
#include "pq.h"

static struct ap_graph _ap_graph_list = {.next = &_ap_graph_list, .prev = &_ap_graph_list};
static struct ap_graph * ap_graph_list = &_ap_graph_list;

void
ap_graph_init(struct ap_graph * graph)
{
    NONNULL(graph);
    memset(graph, 0, sizeof *graph);
    LL_INIT(&graph->equivs);
    LL_INIT(graph);
    LL_PUSH(ap_graph_list, graph);
}

void
ap_graph_extract(struct ap_graph * graph)
{
    NONNULL(graph);

    for (size_t i = 0; i < graph->prereqs_count; i++) {
        struct ap_graph * other = graph->prereqs[i];
        for (size_t j = 0; j < other->enables_count; j++) {
            if (other->enables[j] == graph) {
                other->enables_count--;
                memmove(&other->enables[j], &other->enables[j+1], (other->enables_count - j) * sizeof other->enables[j]);
                break;
            }
        }
    }

    struct ap_graph * equiv = (void *) LL_PEEK(&graph->equivs);
    for (size_t i = 0; i < graph->enables_count; i++) {
        struct ap_graph * other = graph->enables[i];
        for (size_t j = 0; j < other->prereqs_count; j++) {
            if (other->prereqs[j] == graph) {
                other->prereqs_count--;
                memmove(&other->prereqs[j], &other->prereqs[j+1], (other->prereqs_count - j) * sizeof other->prereqs[j]);
                if (equiv != NULL) {
                    ap_graph_add_prereq(other, equiv);
                }
                break;
            }
        }
    }

    LL_EXTRACT(&graph->equivs);
    LL_EXTRACT(graph);
}

void
ap_graph_add_prereq(struct ap_graph * graph, struct ap_graph * prereq)
{
    NONNULL(graph);
    NONNULL(prereq);
    for (size_t i = 0; i < graph->prereqs_count; i++) {
        if (graph->prereqs[i] == prereq)
            return;
    }

    assert(graph->prereqs_count < AP_GRAPH_MAX_EDGES);
    assert(prereq->enables_count< AP_GRAPH_MAX_EDGES);
    for (size_t i = 0; i < prereq->enables_count; i++) {
        assert(prereq->enables[i] != graph);
    }

    graph->prereqs[graph->prereqs_count++] = prereq;
    prereq->enables[prereq->enables_count++] = graph;
}

void
ap_graph_add_equiv(struct ap_graph * graph, struct ap_graph * _equiv)
{
    NONNULL(graph);
    assert(graph != _equiv);
    struct ap_graph_equivs * equiv = (void *) _equiv;
    LL_PUSH(&graph->equivs, equiv);
}

void
ap_graph_mark_done(struct ap_graph * graph)
{
    graph->done = true;
}

bool
ap_graph_is_blocked(struct ap_graph * graph)
{
    if (graph->done) {
        return false;
    }
    for (size_t i = 0; i < graph->prereqs_count; i++) {
        struct ap_graph * prereq = graph->prereqs[i];
        if (prereq->done) {
            goto prereq_met;
        }
        for (struct ap_graph_equivs * _equiv = prereq->equivs.next; _equiv != &prereq->equivs; _equiv = _equiv->next) {
            struct ap_graph * equiv = (void *) _equiv;
            if (equiv->done) {
                goto prereq_met;
            }
        }
        return true;
prereq_met:;
    }
    return false;
}

void
ap_graph_print()
{
    FILE * gf = NONNULL(fopen("graph.dot", "w"));
    fprintf(gf, "\ndigraph g {\n");
    for (struct ap_graph * graph = ap_graph_list->next; graph != ap_graph_list; graph = graph->next) {
        assert_bp(graph->next->prev == graph && graph->prev->next == graph);
        //if (graph->equivs.next == &graph->equivs && graph->prereqs_count == 0 && graph->enables_count == 0) continue;
        fprintf(gf, "   g%p [label=\"" PRIGOAL "\"]\n", graph, PRIGOALF(ap_goal_from_graph(graph)));
        if (graph->done) {
            fprintf(gf, "    g%p [color=blue]\n", graph);
        }
        for (struct ap_graph_equivs * es = graph->equivs.next; es != &graph->equivs; es = es->next) {
            if (es < &graph->equivs)
                continue;
            fprintf(gf, "    g%p -> g%p [dir=both color=green]\n", es, graph);
        }
        for (size_t i = 0; i < graph->prereqs_count; i++) {
            fprintf(gf, "    g%p -> g%p\n", graph->prereqs[i], graph);
        }
    }
    fprintf(gf, "}\n\n");
    fclose(gf);
}
