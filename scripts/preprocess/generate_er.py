#!/usr/bin/env python3
import argparse
import random

def generate_er(num_nodes, prob, output_file, seed=None):
    if seed is not None:
        random.seed(seed)
        
    with open(output_file, 'w') as f:
        f.write(f"# Erdős-Rényi graph with {num_nodes} nodes, p={prob}\n")
        for i in range(num_nodes):
            for j in range(i + 1, num_nodes):
                if random.random() < prob:
                    f.write(f"{i} {j}\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate Erdős-Rényi random graph')
    parser.add_argument('--nodes', type=int, required=True, help='Number of nodes')
    parser.add_argument('--prob', type=float, required=True, help='Connection probability')
    parser.add_argument('--output', type=str, default='er_edges.txt',
                       help='Output edge list file')
    parser.add_argument('--seed', type=int, help='Random seed')
    args = parser.parse_args()
    
    generate_er(args.nodes, args.prob, args.output, args.seed)
    print(f"Generated Erdős-Rényi graph (n={args.nodes}, p={args.prob}) in {args.output}")