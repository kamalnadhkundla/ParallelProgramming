#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define INF 1e9

// CSR Graph structure
typedef struct {
    int num_nodes;
    int num_edges;
    int* row_ptr;
    int* col_idx;
    int* weights;
} CSRGraph;

// Create CSR graph from file
CSRGraph* create_csr_graph(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    CSRGraph* graph = malloc(sizeof(CSRGraph));
    fscanf(file, "%d %d", &graph->num_nodes, &graph->num_edges);

    // Temporary arrays for edge list
    int* edges_per_vertex = calloc(graph->num_nodes, sizeof(int));
    int* temp_edges = malloc(2 * graph->num_edges * sizeof(int));
    int* temp_weights = malloc(2 * graph->num_edges * sizeof(int));
    int edge_count = 0;

    // Read edges and count edges per vertex
    for (int i = 0; i < graph->num_edges; i++) {
        int src, dest, weight;
        fscanf(file, "%d %d %d", &src, &dest, &weight);
        
        temp_edges[edge_count] = src;
        temp_edges[edge_count + 1] = dest;
        temp_weights[edge_count / 2] = weight;
        edges_per_vertex[src]++;
        edges_per_vertex[dest]++;
        edge_count += 2;
    }
    fclose(file);

    // Allocate CSR arrays
    graph->row_ptr = malloc((graph->num_nodes + 1) * sizeof(int));
    graph->col_idx = malloc(edge_count * sizeof(int));
    graph->weights = malloc(edge_count * sizeof(int));
    
    // Set up row pointers
    graph->row_ptr[0] = 0;
    for (int i = 0; i < graph->num_nodes; i++) {
        graph->row_ptr[i + 1] = graph->row_ptr[i] + edges_per_vertex[i];
        edges_per_vertex[i] = 0;  // Reset for use as current count
    }

    // Fill in column indices and weights
    for (int i = 0; i < edge_count; i += 2) {
        int src = temp_edges[i];
        int dest = temp_edges[i + 1];
        int weight = temp_weights[i / 2];
        
        // Add edge src -> dest
        int pos = graph->row_ptr[src] + edges_per_vertex[src];
        graph->col_idx[pos] = dest;
        graph->weights[pos] = weight;
        edges_per_vertex[src]++;
        
        // Add edge dest -> src (undirected graph)
        pos = graph->row_ptr[dest] + edges_per_vertex[dest];
        graph->col_idx[pos] = src;
        graph->weights[pos] = weight;
        edges_per_vertex[dest]++;
    }

    free(edges_per_vertex);
    free(temp_edges);
    free(temp_weights);
    return graph;
}

// Sequential Dijkstra
void dijkstra_sequential(CSRGraph* graph, int source, int* distances) {
    char* visited = calloc(graph->num_nodes, sizeof(char));
    
    for (int i = 0; i < graph->num_nodes; i++) {
        distances[i] = INF;
    }
    distances[source] = 0;

    for (int i = 0; i < graph->num_nodes; i++) {
        int min_dist = INF;
        int min_vertex = -1;
        
        for (int j = 0; j < graph->num_nodes; j++) {
            if (!visited[j] && distances[j] < min_dist) {
                min_dist = distances[j];
                min_vertex = j;
            }
        }
        
        if (min_vertex == -1) break;
        visited[min_vertex] = 1;

        for (int j = graph->row_ptr[min_vertex]; j < graph->row_ptr[min_vertex + 1]; j++) {
            int adj_vertex = graph->col_idx[j];
            if (!visited[adj_vertex]) {
                int new_dist = distances[min_vertex] + graph->weights[j];
                if (new_dist < distances[adj_vertex]) {
                    distances[adj_vertex] = new_dist;
                }
            }
        }
    }
    
    free(visited);
}

// Parallel Dijkstra optimized for large graphs
void dijkstra_parallel(CSRGraph* graph, int source, int* distances) {
    char* visited = calloc(graph->num_nodes, sizeof(char));
    
    #pragma omp parallel for
    for (int i = 0; i < graph->num_nodes; i++) {
        distances[i] = INF;
    }
    distances[source] = 0;

    for (int count = 0; count < graph->num_nodes; count++) {
        int min_index = -1;
        {
            int min_distance = INF;
            #pragma omp parallel
            {
                int local_min_distance = INF;
                int local_min_index = -1;

                #pragma omp for schedule(static, 1024)
                for (int i = 0; i < graph->num_nodes; i++) {
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
        }

        if (min_index == -1) break;
        visited[min_index] = 1;

        #pragma omp parallel for schedule(dynamic, 512)
        for (int i = graph->row_ptr[min_index]; i < graph->row_ptr[min_index + 1]; i++) {
            int adj_vertex = graph->col_idx[i];
            if (!visited[adj_vertex]) {
                int new_dist = distances[min_index] + graph->weights[i];
                if (new_dist < distances[adj_vertex]) {
                    #pragma omp atomic write
                    distances[adj_vertex] = new_dist;
                }
            }
        }
    }

    free(visited);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file> <source_node>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];
    int source = atoi(argv[2]);

    // Set optimal number of threads based on system
    int num_threads = omp_get_max_threads();
    num_threads = num_threads > 16 ? 16 : num_threads;  // Limit to 16 threads
    omp_set_num_threads(num_threads);

    // Create graph
    CSRGraph* graph = create_csr_graph(filename);
    printf("Graph loaded: %d nodes, %d edges\n", graph->num_nodes, graph->num_edges);
    printf("Using %d threads for parallel execution\n", num_threads);

    // Allocate distance arrays
    int* sequential_distances = malloc(graph->num_nodes * sizeof(int));
    int* parallel_distances = malloc(graph->num_nodes * sizeof(int));

    // Run sequential version
    double start_time = omp_get_wtime();
    dijkstra_sequential(graph, source, sequential_distances);
    double sequential_time = omp_get_wtime() - start_time;

    // Run parallel version
    start_time = omp_get_wtime();
    dijkstra_parallel(graph, source, parallel_distances);
    double parallel_time = omp_get_wtime() - start_time;

    // Print results
    printf("\nSequential execution time: %.6f seconds\n", sequential_time);
    printf("Parallel kamalexecution time: %.6f seconds\n", parallel_time);
    printf("Speedup: %.2fx\n", sequential_time / parallel_time);

    // Verify results
    int correct = 1;
    for (int i = 0; i < graph->num_nodes; i++) {
        if (sequential_distances[i] != parallel_distances[i]) {
            printf("Mismatch at node %d: Sequential=%d, Parallel=%d\n",
                   i, sequential_distances[i], parallel_distances[i]);
            correct = 0;
            break;
        }
    }
    printf("Results verification: %s\n", correct ? "PASSED" : "FAILED");

    // Print first few distances if requested
    if (graph->num_nodes <= 20) {
        printf("\nShortest distances from node %d:\n", source);
        for (int i = 0; i < graph->num_nodes; i++) {
            printf("Node %d: %d\n", i, parallel_distances[i]);
        }
    }

    // Cleanup
    free(graph->row_ptr);
    free(graph->col_idx);
    free(graph->weights);
    free(graph);
    free(sequential_distances);
    free(parallel_distances);

    return 0;
}
