# Build stage
FROM omnetpp/omnetpp:u22.04-6.0 AS builder

# Install dependencies
RUN apt-get update && \
    apt-get install -y \
    python3-dev \
    python3-venv \
    git \
    curl \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Install uv
COPY --from=ghcr.io/astral-sh/uv:latest /uv /uvx /usr/local/bin/

# Copy project files
WORKDIR /workspace
COPY . .

# Install Python dependencies using system Python
RUN uv venv --python python3 .venv && \
    uv sync --frozen --no-dev

# Build OMNeT++ project (source setenv to get opp_* tools on PATH)
SHELL ["/bin/bash", "-c"]
RUN source /root/omnetpp/setenv && \
    cd omnetpp/simulations && \
    opp_makemake -f --deep -o scm-simulations -I/usr/include -L/usr/lib/x86_64-linux-gnu -lssl -lcrypto && \
    make -j$(nproc)

# Final stage — same base image so system Python path matches
FROM omnetpp/omnetpp:u22.04-6.0

# Runtime dependencies
RUN apt-get update && \
    apt-get install -y python3-venv libssl3 && \
    rm -rf /var/lib/apt/lists/*

# Install uv in runtime stage
COPY --from=ghcr.io/astral-sh/uv:latest /uv /uvx /usr/local/bin/

# Copy built artifacts and venv
COPY --from=builder /workspace /workspace

# Ensure scripts are executable
RUN chmod +x /workspace/scripts/*.sh /workspace/docker/entrypoint.sh

# Set working directory
WORKDIR /workspace/omnetpp/simulations

# Default command
CMD ["/workspace/scripts/run_experiments.sh"]
