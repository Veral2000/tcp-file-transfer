# Performance

## Benchmark plan

Performance numbers will be measured on the target test environment rather than estimated.

Record at least:

- File size
- Chunk size
- Transfer duration
- Effective throughput
- Sender CPU utilization
- Receiver CPU utilization
- Peak memory usage
- Network link speed
- Operating system and CPU

## Baseline

The first benchmark represents the sequential streaming implementation. No application-level pipelining should be claimed until measured.

## Optimization candidate

A later implementation may use a bounded sliding window of in-flight chunks. The benchmark should compare the optimized implementation with the sequential baseline on both low-latency and higher-latency links.

## Results

| Test | File | Chunk | Duration | Throughput | Peak RAM | CPU | Notes |
|---|---:|---:|---:|---:|---:|---:|---|
| TBD | TBD | 4 MiB | TBD | TBD | TBD | TBD | Baseline |
