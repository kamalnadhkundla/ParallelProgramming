#include <stdio.h>
#include <stdlib.h>

#define INF 1e9  // Infinity value

// Function to create an adjacency list
typedef struct Node {
    int vertex;
    struct Node *next;
} Node;

typedef struct Graph {
    int num_nodes;
    Node **adj_list;
} Graph;

Graph *create_graph(int num_nodes) {
    Graph *graph = malloc(sizeof(Graph));
    graph->num_nodes = num_nodes;
    graph->adj_list = malloc(num_nodes * sizeof(Node *));
    for (int i = 0; i < num_nodes; i++) {
        graph->adj_list[i] = NULL;
    }
    return graph;
}

void add_edge(Graph *graph, int src, int dest) {
    Node *new_node = malloc(sizeof(Node));
    new_node->vertex = dest;
    new_node->next = graph->adj_list[src];
    graph->adj_list[src] = new_node;

    new_node = malloc(sizeof(Node));
    new_node->vertex = src;
    new_node->next = graph->adj_list[dest];
    graph->adj_list[dest] = new_node;
}

Graph *read_graph(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    int num_nodes, num_edges;
    fscanf(file, "%d %d", &num_nodes, &num_edges);

    Graph *graph = create_graph(num_nodes);

    for (int i = 0; i < num_edges; i++) {
        int u, v;
        fscanf(file, "%d %d", &u, &v);
        add_edge(graph, u, v);
    }

    fclose(file);
    return graph;
}

// Depth-First Search (DFS)
void dfs(Graph *graph, int vertex, int *visited) {
    visited[vertex] = 1;

    Node *adj = graph->adj_list[vertex];
    while (adj != NULL) {
        if (!visited[adj->vertex]) {
            dfs(graph, adj->vertex, visited);
        }
        adj = adj->next;
    }
}

// Check if the graph is fully connected
int is_connected(Graph *graph) {
    int *visited = calloc(graph->num_nodes, sizeof(int));

    // Start DFS from node 0
    dfs(graph, 0, visited);

    // Check if all nodes are visited
    for (int i = 0; i < graph->num_nodes; i++) {
        if (!visited[i]) {
            free(visited);
            return 0;  // Graph is not fully connected
        }
    }

    free(visited);
    return 1;  // Graph is fully connected
}

void free_graph(Graph *graph) {
    for (int i = 0; i < graph->num_nodes; i++) {
        Node *adj = graph->adj_list[i];
        while (adj != NULL) {
            Node *temp = adj;
            adj = adj->next;
            free(temp);
        }
    }
    free(graph->adj_list);
    free(graph);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];
    Graph *graph = read_graph(filename);

    if (is_connected(graph)) {
        printf("The graph is fully connected.\n");
    } else {
        printf("The graph is not fully connected.\n");
    }

    free_graph(graph);

    return 0;
}

