# Self-stabilizing Multicast Overlay (SCM) Implementation

OMNeT++ implementation of the self-stabilizing multicast overlay algorithm from:

> "On Self-stabilizing Sharing of Multicast Transmission in Overlays"  
> [Full Paper Reference]

## Features

- Implementation of the SCM algorithm with all 5 rules
- Support for multiple network topologies:
  - Complete Binary Trees (CBT)
  - Erdős-Rényi (ER) graphs
  - Twitch social network
- Fault injection mechanisms
- Docker support for reproducible experiments

## Getting Started

### Prerequisites

- OMNeT++ 6.x
- Python 3.8+
- Docker 

### Installation

```bash
git clone https://github.com/yourusername/scm-overlay-omnet.git
cd scm-overlay-omnet/omnetpp/simulations
make