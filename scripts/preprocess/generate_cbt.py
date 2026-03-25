#!/usr/bin/env python3
"""Generate Complete Binary Tree edge-list files.

Author: Sinchan Sengupta <sinchan.sengupta@univ-nantes.fr>
"""
import argparse


def generate_cbt(num_nodes, output_file):
    if num_nodes < 2:
        raise ValueError(f"num_nodes must be >= 2 (got {num_nodes})")
    with open(output_file, 'w') as f:
        f.write(f"# Complete Binary Tree with {num_nodes} nodes\n")
        for i in range(1, num_nodes):
            parent = (i - 1) // 2
            f.write(f"{parent} {i}\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Generate Complete Binary Tree edge list')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--depth', type=int, help='Depth of the tree')
    group.add_argument('--nodes', type=int, help='Number of nodes')
    parser.add_argument('--output', type=str, default='cbt_edges.txt',
                        help='Output edge list file')
    args = parser.parse_args()

    if args.depth is not None:
        if args.depth < 0:
            raise ValueError(f"depth must be >= 0 (got {args.depth})")
        num_nodes = 2 ** (args.depth + 1) - 1
    else:
        num_nodes = int(args.nodes)

    generate_cbt(num_nodes, args.output)
    print(f"Generated Complete Binary Tree (n={num_nodes}) in {args.output}")
