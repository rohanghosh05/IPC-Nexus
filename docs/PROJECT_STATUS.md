# Implementation status

| Requirement | Status | Verification |
|---|---|---|
| Anonymous pipes | Implemented | `./build/ipc_nexus demo-all` |
| Named FIFOs | Implemented | `./build/ipc_nexus demo-all` |
| System V message queues | Implemented | `./build/ipc_nexus demo-all` |
| POSIX shared memory + process-shared semaphores | Implemented | `./build/ipc_nexus demo-all` |
| UNIX-domain socket IPC | Implemented | `./build/ipc_nexus demo-all` |
| Multi-client TCP chat | Implemented | `make integration-test` and manual three-client test |
| Binary client-to-server file upload | Implemented | `make integration-test` verifies byte integrity |
| IPC round-trip benchmark | Implemented | `make benchmark` |
| Benchmark CSV | Implemented | `results/benchmark.csv` (regenerate on teammate's machine) |
| Benchmark graphs | Implemented | `make graphs` (regenerate on teammate's machine) |

This is a working academic prototype. It is not a hardened production server. Benchmark results vary by machine and should be regenerated on the machine used for the final report.
