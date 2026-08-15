# Engineering Decision Log

## ADR-001: C++17 + CMake

**Decision:** Use C++17 with CMake.

**Rationale:** C++ provides direct systems/networking APIs, RAII for resource ownership, strong filesystem support, and mature cross-platform toolchains. CMake provides a common build definition for Linux and Windows.

## ADR-002: Streaming instead of whole-file buffering

**Decision:** Read and write bounded chunks.

**Rationale:** The assignment requires files up to 16 GB. Memory consumption must not scale with file size.

## ADR-003: Application-layer framing

**Decision:** Prefix every message with a fixed protocol header.

**Rationale:** TCP is a byte stream and does not preserve application message boundaries. Explicit framing allows safe parsing, validation, and protocol evolution.

## ADR-004: 64-bit file metadata

**Decision:** File size, offset, and chunk index use unsigned 64-bit values.

**Rationale:** 32-bit file metadata is insufficient for the assignment's 16 GB requirement and future growth.

## ADR-005: Sequential baseline before optimization

**Decision:** Implement and benchmark a correct sequential transfer before adding pipelining.

**Rationale:** The assignment lists bandwidth utilization as a bonus. Optimization should be driven by measured bottlenecks rather than added complexity without evidence.
