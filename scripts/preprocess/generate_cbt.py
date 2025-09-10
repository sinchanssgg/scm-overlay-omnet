#!/usr/bin/env python3
import argparse
import math

def generate_cbt(depth, output_file):
    num_nodes = 2**(depth + 1) - 1
    with open(output_file, 'w') as f:
        f.write(f"# Complete Binary Tree with depth {depth} ({num_nodes} nodes)\n")
        for i in range(1, num_nodes):
            parent = (i - 1) // 2
            f.write(f"{parent} {i}\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate Complete Binary Tree edge list')
    parser.add_argument('--depth', type=int, required=True, help='Depth of the tree')
    parser.add_argument('--output', type=str, default='cbt_edges.txt', 
                       help='Output edge list file')
    args = parser.parse_args()
    
    generate_cbt(args.depth, args.output)
    print(f"Generated Complete Binary Tree (depth={args.depth}) in {args.output}")