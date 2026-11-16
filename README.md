# IPC Nexus — Multi-Process Communication and Benchmarking

A Linux C project demonstrating five local IPC mechanisms, a real multi-client TCP chat/file-upload server, and an empirical IPC benchmark with CSV output and graphs.

## Features

1. Anonymous pipes — parent/child request-response.
2. Named FIFOs — named-pipe request-response.
3. System V message queues — typed message exchange.
4. POSIX shared memory synchronized with process-shared semaphores.
5. UNIX-domain stream socketpair — local bidirectional IPC (not TCP).
6. Multi-client TCP chat — threaded server; messages broadcast to other connected clients.
7. Binary file upload — client uploads a file to server `uploads/` using a length-prefixed transfer protocol and 64 KiB chunks.
8. Benchmark — repeated ping-pong round trips for the five IPC mechanisms; exports CSV and generates latency/throughput graphs.

## Requirements

Ubuntu 22.04/24.04, WSL2 Ubuntu, or another Linux distribution with GCC, Make, pthreads, Python 3 and Matplotlib. Native Windows MinGW is not supported because the project uses POSIX/System V IPC APIs.

## Build

```bash
sudo apt update
sudo apt install -y build-essential gcc make python3 python3-venv python3-pip
python3 -m venv .venv
source .venv/bin/activate
pip install matplotlib
make clean
make
```

## Run the IPC demonstrations

```bash
./build/ipc_nexus demo-all
```

Individual demonstrations: `pipe`, `fifo`, `queue`, `shm`, `socket`.

## Automated TCP integration test

With the build complete, run:

```bash
make integration-test
```

This starts a local server, connects three test clients, verifies broadcast delivery, uploads a binary payload, and checks its SHA-256-equivalent byte integrity.

## Run TCP chat and file transfer

Terminal 1 (server):

```bash
./build/tcp_server 5050
```

Terminal 2, 3, 4 (each a separate client):

```bash
./build/tcp_client 127.0.0.1 5050
```

Type a message and press Enter to broadcast it to the other clients. Upload a file with:

```text
/send /absolute/path/to/file.pdf
```

The server saves uploaded files in `uploads/`. Type `/quit` to disconnect. For two different computers, use the server machine's LAN IP instead of `127.0.0.1`, and allow inbound TCP port 5050 in its firewall. Both machines must be on a network that permits direct client-to-server connections.

## Run benchmarks and generate graphs

```bash
make benchmark
make graphs
```

Outputs:
- `results/benchmark.csv`
- `results/latency_comparison.png`
- `results/throughput_comparison.png`

The benchmark uses a 1024-byte payload and 2,000 ping-pong round trips per mechanism. It reports average round-trip time and an estimated throughput derived from the bytes exchanged and elapsed time. Results are machine-, kernel-, load-, and filesystem-dependent; run at least three times for a report and compare medians if you need a more stable result. This is a microbenchmark, not a universal ranking of IPC mechanisms. Shared-memory results include semaphore synchronization overhead; FIFO results include filesystem FIFO behavior. TCP is intentionally not included in this local IPC comparison because TCP is a network transport and should be benchmarked separately under controlled network conditions.

## Important protocol notes / limitations

- The chat server uses one detached pthread per client and supports up to 64 simultaneous connections. This is a classroom demonstration, not a hardened production server.
- Chat broadcasts to all other connected clients; a sender does not receive its own message back.
- File transfer is client-to-server upload only. The server sanitizes filenames to alphanumeric, dot, underscore and hyphen characters. Uploads with the same sanitized filename overwrite an earlier file; use unique filenames.
- File size is capped at 1 GiB. Use only trusted networks and test files.
- The server stores files relative to its current working directory under `uploads/`.
- The five IPC mechanisms are separate demonstrations. The TCP chat/file-transfer feature is an additional network application; it does not imply all five local IPC mechanisms are simultaneously used by the chat server.

## Suggested teammate verification checklist

1. `make clean && make` completes without errors.
2. `./build/ipc_nexus demo-all` prints a PASS result for all five mechanisms.
3. Start server and at least three clients; messages from one client appear in the other clients.
4. Upload a small text file and a binary file; compare original and server copy using `sha256sum`.
5. Run `make benchmark` and `make graphs`; confirm CSV and two PNG files exist.
6. Send the terminal output, screenshots, `uname -a`, `gcc --version`, and `sha256sum` results to the project team.
