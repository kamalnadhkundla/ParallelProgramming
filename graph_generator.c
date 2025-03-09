#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Hash function to encode edges as unique integers
long long encode_edge(int u, int v) {
    return (long long)(u < v ? u : v) * 1000000000LL + (u > v ? u : v);
}

// Function to generate a random weighted graph in edge list format
void generate_random_weighted_graph(int num_nodes, int num_edges, int max_weight, const char *filename) {
    // Check if the number of edges exceeds the maximum possible
    int max_possible_edges = num_nodes * (num_nodes - 1) / 2;
    if (num_edges > max_possible_edges) {
        fprintf(stderr, "Error: Too many edges for the given number of nodes.\n");
        exit(EXIT_FAILURE);
    }

    // Open the output file
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for tracking edges using a hash set
    long long *edge_set = (long long *)malloc(num_edges * sizeof(long long));
    int edge_count = 0;

    // Seed the random number generator
    srand(time(NULL));

    // Write the number of nodes and edges to the file
    fprintf(file, "%d %d\n", num_nodes, num_edges);

    // Generate random edges with weights
    while (edge_count < num_edges) {
        int u = rand() % num_nodes;
        int v = rand() % num_nodes;

        // Ensure no self-loops
        if (u == v) {
            continue;
        }

        // Encode the edge
        long long edge = encode_edge(u, v);

        // Check for duplicates
        int is_duplicate = 0;
        for (int i = 0; i < edge_count; i++) {
            if (edge_set[i] == edge) {
                is_duplicate = 1;
                break;
            }
        }

        if (!is_duplicate) {
            // Add the edge to the hash set
            edge_set[edge_count++] = edge;

            // Generate a random weight
            int weight = (rand() % max_weight) + 1; // Random weight between 1 and max_weight

            // Write the edge to the file
            fprintf(file, "%d %d %d\n", u, v, weight);
        }
    }

    // Free the hash set memory
    free(edge_set);

    // Close the file
    fclose(file);
    printf("Weighted graph with %d nodes and %d edges generated in %s.\n", num_nodes, num_edges, filename);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <num_nodes> <num_edges> <max_weight> <output_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_nodes = atoi(argv[1]);
    int num_edges = atoi(argv[2]);
    int max_weight = atoi(argv[3]);
    const char *filename = argv[4];

    if (num_nodes <= 0 || num_edges <= 0 || max_weight <= 0) {
        fprintf(stderr, "Number of nodes, edges, and max weight must be positive integers.\n");
        return EXIT_FAILURE;
    }

    generate_random_weighted_graph(num_nodes, num_edges, max_weight, filename);

    return EXIT_SUCCESS;
}

