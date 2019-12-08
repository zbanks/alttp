#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define AP_GRAPH_MAX_EDGES 64

struct ap_graph;
struct ap_graph_equivs;
struct ap_graph {
    struct ap_graph_equivs {
        struct ap_graph_equivs * next;
        struct ap_graph_equivs * prev;
    } equivs;

    struct ap_graph * next;
    struct ap_graph * prev;

    //bool visited;
    const char * name;
    bool done;
    // int min_cost; TODO

    size_t prereqs_count;
    struct ap_graph * prereqs[AP_GRAPH_MAX_EDGES];

    size_t enables_count;
    struct ap_graph * enables[AP_GRAPH_MAX_EDGES];
};

void
ap_graph_init(struct ap_graph * graph, const char * name);

void
ap_graph_extract(struct ap_graph * graph);

void
ap_graph_add_prereq(struct ap_graph * graph, struct ap_graph * prereq);

void
ap_graph_add_equiv(struct ap_graph * graph, struct ap_graph * equiv);

void
ap_graph_mark_done(struct ap_graph * graph);

bool
ap_graph_is_blocked(struct ap_graph * graph);

void
ap_graph_print();
