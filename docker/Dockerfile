# Build stage
FROM omnetpp/omnetpp:6.0.1-ubuntu20.04 AS builder

# Install dependencies
RUN apt-get update && \
    apt-get install -y \
    python3-pip \
    python3-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

# Copy project files
WORKDIR /workspace
COPY . .

# Build OMNeT++ project
RUN cd omnetpp/simulations && \
    opp_makemake -f --deep -o scm-simulations && \
    make -j$(nproc)

# Install Python requirements
RUN pip3 install -r requirements.txt

# Final stage
FROM omnetpp/omnetpp:6.0.1-ubuntu20.04

# Copy built artifacts
COPY --from=builder /workspace /workspace
COPY --from=builder /usr/local/lib/python3.8/dist-packages /usr/local/lib/python3.8/dist-packages

# Set working directory
WORKDIR /workspace/omnetpp/simulations

# Default command - show help
CMD ["/workspace/scripts/run_experiments.sh"]