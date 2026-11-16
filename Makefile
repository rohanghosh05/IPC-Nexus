CC := gcc
CFLAGS := -std=c11 -O2 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L
LDLIBS := -pthread -lrt
BIN := build
.PHONY: all clean test integration-test benchmark graphs
all: $(BIN)/ipc_nexus $(BIN)/tcp_server $(BIN)/tcp_client $(BIN)/ipc_bench
$(BIN):
	mkdir -p $(BIN)
$(BIN)/ipc_nexus: src/ipc_demos.c | $(BIN)
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)
$(BIN)/tcp_server: src/tcp_server.c | $(BIN)
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)
$(BIN)/tcp_client: src/tcp_client.c | $(BIN)
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)
$(BIN)/ipc_bench: src/ipc_bench.c | $(BIN)
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)
test: all
	./$(BIN)/ipc_nexus demo-all
integration-test: all
	python3 scripts/integration_test.py
benchmark: all
	mkdir -p results
	./$(BIN)/ipc_bench results/benchmark.csv

graphs:
	python3 scripts/plot_benchmarks.py results/benchmark.csv
clean:
	rm -rf $(BIN) results/benchmark.csv results/*.png
