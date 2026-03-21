# Build stage
FROM omnetpp/omnetpp:6.0.1-ubuntu20.04 AS builder

# Install dependencies
RUN apt-get update && \
    apt-get install -y \
    python3-dev \
    git \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Install uv
COPY --from=ghcr.io/astral-sh/uv:latest /uv /uvx /usr/local/bin/

# Copy project files
WORKDIR /workspace
COPY . .

# Install Python dependencies via uv
RUN uv sync --frozen --no-dev

# Build OMNeT++ project
RUN cd omnetpp/simulations && \
    opp_makemake -f --deep -o scm-simulations && \
    make -j$(nproc)

# Final stage
FROM omnetpp/omnetpp:6.0.1-ubuntu20.04

# Install uv in runtime stage
COPY --from=ghcr.io/astral-sh/uv:latest /uv /uvx /usr/local/bin/

# Copy built artifacts and venv
COPY --from=builder /workspace /workspace

# Set working directory
WORKDIR /workspace/omnetpp/simulations

# Default command - show help
CMD ["/workspace/scripts/run_experiments.sh"]
