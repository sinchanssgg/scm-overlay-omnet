#!/usr/bin/env python3
import argparse
import pandas as pd

def process_twitch(input_file, output_file, sample_frac=None, seed=None):
    # Read the large file in chunks if needed
    df = pd.read_csv(input_file, header=None, names=['source', 'target'])
    
    if sample_frac is not None:
        df = df.sample(frac=sample_frac, random_state=seed)
    
    # Save processed edges
    df.to_csv(output_file, index=False, header=False)
    print(f"Processed Twitch dataset. Output: {output_file}")
    print(f"  Total edges: {len(df)}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process Twitch dataset')
    parser.add_argument('--input', type=str, required=True,
                       help='Path to large_twitch_edges.csv')
    parser.add_argument('--output', type=str, default='twitch_processed.txt',
                       help='Output edge list file')
    parser.add_argument('--sample', type=float,
                       help='Fraction of edges to sample (0-1)')
    parser.add_argument('--seed', type=int, help='Random seed for sampling')
    args = parser.parse_args()
    
    process_twitch(args.input, args.output, args.sample, args.seed)