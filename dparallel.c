#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define INF 1e9  // Infinity value

// Function to create a graph as an adjacency matrix
int **create_graph(int num_nodes) {
    int **graph = malloc(num_nodes * sizeof(int *));
    for (int i = 0; i < num_nodes; i++) {
        graph[i] = malloc(num_nodes * sizeof(int));
        for (int j = 0; j < num_nodes; j++) {
            graph[i][j] = (i == j) ? 0 : INF;  // Distance to self is 0, others are INF
        }
    }
    return graph;
}

// Function to read the graph from a file
int **read_graph(const char *filename, int *num_nodes, int *num_edges) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fscanf(file, "%d %d", num_nodes, num_edges);
    int **graph = create_graph(*num_nodes);

    for (int i = 0; i < *num_edges; i++) {
        int u, v, weight;
        fscanf(file, "%d %d %d", &u, &v, &weight);
        graph[u][v] = weight;
        graph[v][u] = weight;  // For undirected graph
    }

    fclose(file);
    return graph;
}

// Parallel Dijkstra's Algorithm
void dijkstra_parallel(int **graph, int num_nodes, int source) {
    int *distances = malloc(num_nodes * sizeof(int));
    int *visited = malloc(num_nodes * sizeof(int));

    #pragma omp parallel for
    for (int i = 0; i < num_nodes; i++) {
        distances[i] = INF;
        visited[i] = 0;
    }
    distances[source] = 0;

    for (int count = 0; count < num_nodes - 1; count++) {
        int min_distance = INF, min_index = -1;

        // Parallel finding of the minimum distance node
        #pragma omp parallel
        {
            int local_min_distance = INF, local_min_index = -1;

            #pragma omp for
            for (int i = 0; i < num_nodes; i++) {
                if (!visited[i] && distances[i] < local_min_distance) {
                    local_min_distance = distances[i];
                    local_min_index = i;
                }
            }

            #pragma omp critical
            {
                if (local_min_distance < min_distance) {
                    min_distance = local_min_distance;
                    min_index = local_min_index;
                }
            }
        }

        if (min_index == -1) break; // No reachable node left
        visited[min_index] = 1;

        // Parallel distance updates for neighbors
        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < num_nodes; i++) {
            if (!visited[i] && graph[min_index][i] != INF && distances[min_index] != INF) {
                int new_distance = distances[min_index] + graph[min_index][i];
                if (new_distance < distances[i]) {
                    distances[i] = new_distance;
                }
            }
        }
    }

    // Print the shortest distances
    printf("\nShortest distances from node %d:\n", source);
    for (int i = 0; i < num_nodes; i++) {
        if (distances[i] == INF) {
            printf("Node %d: INF\n", i);
        } else {
            printf("Node %d: %d\n", i, distances[i]);
        }
    }

    free(distances);
    free(visited);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file> <source_node>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];
    int source = atoi(argv[2]);
    int num_nodes, num_edges;

    int **graph = read_graph(filename, &num_nodes, &num_edges);

    printf("Graph loaded: %d nodes, %d edges\n", num_nodes, num_edges);

    dijkstra_parallel(graph, num_nodes, source);

    for (int i = 0; i < num_nodes; i++) {
        free(graph[i]);
    }
    free(graph);

    return 0;
}

