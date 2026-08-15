# Protocol v1

## Header

Every message starts with a 16-byte header:

| Field | Size | Encoding |
|---|---:|---|
| Magic | 4 bytes | uint32, big-endian |
| Version | 2 bytes | uint16, big-endian |
| Message type | 1 byte | uint8 |
| Reserved | 1 byte | zero |
| Payload size | 8 bytes | uint64, big-endian |

Magic: `0x54435046` (`TCPF`)

Version: `1`

Maximum payload accepted by the implementation: 32 MiB.

## Message types

| Value | Message | Payload |
|---:|---|---|
| 1 | HELLO | empty |
| 2 | FILE_INFO | file size + filename length + filename |
| 3 | CHUNK | chunk index + file offset + data length + data |
| 4 | TRANSFER_COMPLETE | empty |
| 5 | ERROR | UTF-8 error message |

## FILE_INFO payload

```text
uint64  file_size
uint16  filename_length
bytes   filename
```

The filename is limited to 4096 bytes and is validated by the receiver before filesystem access.

## CHUNK payload

```text
uint64  chunk_index
uint64  file_offset
uint32  data_length
bytes   data
```

A chunk is limited to 16 MiB. The current sender uses 4 MiB chunks.

## State machine

```text
CONNECTED
   |
   v
EXPECT_HELLO
   |
   v
EXPECT_FILE_INFO
   |
   v
RECEIVING_CHUNKS <-----+
   |                   |
   | CHUNK             |
   +-------------------+
   |
   | TRANSFER_COMPLETE
   v
COMPLETE
```

Unexpected message types are treated as protocol errors.

## Future protocol extensions

The next revision is expected to introduce chunk acknowledgements, chunk hashes, transfer identifiers, and resume negotiation. These will be added as new message types or a protocol version rather than silently changing the meaning of existing messages.
