CC      = gcc
CFLAGS  = -Wall -Wextra -g -Iinclude
LDFLAGS = -lpthread -lrt

# Binaries
MONITOR      = monitor
SENSOR       = sensor
ALERT_ENGINE = alert_engine

# Sources
MONITOR_SRCS = src/main.c src/ipc_utils.c src/logger.c src/threads.c
SENSOR_SRCS  = src/sensor.c
ALERT_SRCS   = src/alert_engine.c

.PHONY: all clean run test

all: $(MONITOR) $(SENSOR) $(ALERT_ENGINE)
	@echo "Build complete. Run with: make run"

$(MONITOR): $(MONITOR_SRCS) include/vitals.h include/ipc_utils.h include/logger.h
	$(CC) $(CFLAGS) -o $@ $(MONITOR_SRCS) $(LDFLAGS)

$(SENSOR): $(SENSOR_SRCS) include/vitals.h
	$(CC) $(CFLAGS) -o $@ $(SENSOR_SRCS)

$(ALERT_ENGINE): $(ALERT_SRCS) include/vitals.h
	$(CC) $(CFLAGS) -o $@ $(ALERT_SRCS) $(LDFLAGS)

run: all
	@mkdir -p logs
	./$(MONITOR)

# Deterministic 15-second test run (sends SIGINT after 15s)
test: all
	@mkdir -p logs
	@echo "--- Starting 15-second test run ---"
	./$(MONITOR) & MPID=$$!; sleep 15; kill -INT $$MPID; wait $$MPID
	@echo "--- Test complete. Log output: ---"
	@cat logs/vitals.log

clean:
	rm -f $(MONITOR) $(SENSOR) $(ALERT_ENGINE)
	rm -f logs/vitals.log
	@echo "Cleaned."
