# Build stage
FROM omnetpp/omnetpp:u22.04-6.0 AS builder

# Install dependencies
RUN apt-get update && \
    apt-get install -y \
    python3-dev \
    python3-venv \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Install uv
COPY --from=ghcr.io/astral-sh/uv:latest /uv /uvx /usr/local/bin/

WORKDIR /workspace
COPY pyproject.toml uv.lock ./
COPY scripts ./scripts
COPY omnetpp/simulations ./omnetpp/simulations

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

# Copy only required runtime artifacts
WORKDIR /workspace
COPY --from=builder /workspace/.venv /workspace/.venv
COPY --from=builder /workspace/pyproject.toml /workspace/pyproject.toml
COPY --from=builder /workspace/uv.lock /workspace/uv.lock
COPY --from=builder /workspace/scripts /workspace/scripts
COPY --from=builder /workspace/omnetpp/simulations /workspace/omnetpp/simulations

# Ensure scripts are executable
RUN chmod +x /workspace/scripts/*.sh

# Set working directory
WORKDIR /workspace/omnetpp/simulations

# Default command
CMD ["/workspace/scripts/run_experiments.sh", "--skip-sim-build"]
