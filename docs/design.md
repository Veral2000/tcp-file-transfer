# Design Notes

## Chunking

The transfer is streamed in bounded chunks. The default chunk size is 4 MiB and the protocol currently caps an individual chunk at 16 MiB. This keeps memory usage independent of total file size while avoiding excessive per-chunk protocol overhead.

## Large files

File sizes, offsets, and chunk indexes use 64-bit unsigned integers. The application enforces the assignment limit of 16 GiB before transfer starts.

## Error handling

Failures are surfaced as exceptions inside the library and converted into clear CLI errors at the application boundary. Protocol parsing rejects invalid magic values, unsupported versions, unknown message types, oversized payloads, invalid metadata, and malformed chunks.

## Resource utilization

The sender and receiver operate on bounded buffers. The sender never loads the complete source file into memory, and the receiver writes chunks directly to disk.

## Compression

Compression is intentionally not part of the first implementation. A future `auto` policy can use file type and/or sampled compression ratio to avoid wasting CPU on already-compressed data.

## TCP reliability vs application reliability

TCP already provides ordered, reliable byte-stream delivery. The application protocol therefore does not duplicate TCP's retransmission algorithm. Application-level acknowledgements will instead be used for transfer state, integrity confirmation, and resumability.

## Security

The foundation protects the destination filesystem from simple path traversal and validates untrusted protocol lengths. It is not yet a secure network transport. TLS 1.3 with certificate validation is planned for the security milestone.

## Performance strategy

The baseline implementation deliberately uses sequential streaming first. Performance will be measured before adding a sliding window. The benchmark will record file size, transfer duration, throughput, peak memory, and CPU utilization.

## Trade-offs

| Decision | Benefit | Cost |
|---|---|---|
| 4 MiB default chunk | Bounded memory and efficient I/O | Larger retransmission unit later |
| TCP | Reliable transport already implemented by OS | Head-of-line blocking inherent to TCP |
| Application framing | Explicit protocol state and validation | Additional protocol overhead |
| Sequential baseline | Simple and easy to validate | May underutilize high-BDP links |
| Optional compression | Avoids wasting CPU on all files | Additional implementation complexity |
