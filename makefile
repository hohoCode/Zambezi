TEST_DIR = src/test
DRIVER_DIR = src/driver

OUT_DIR = out
TEST_OUT_DIR = $(OUT_DIR)/test
DRIVER_OUT_DIR = $(OUT_DIR)/driver

HEADERS = -Isrc/shared/
CC = gcc -lz -lm -O3 -fomit-frame-pointer -pipe $(HEADERS)

TEST_SRC_FILES = $(wildcard $(TEST_DIR)/*.c)
TEST_OUT_FILES = $(patsubst $(TEST_DIR)/%.c,$(TEST_OUT_DIR)/%,$(TEST_SRC_FILES))
DRIVER_SRC_FILES = $(wildcard $(DRIVER_DIR)/*.c)
DRIVER_OUT_FILES = $(patsubst $(DRIVER_DIR)/%.c,$(DRIVER_OUT_DIR)/%,$(DRIVER_SRC_FILES))

$(TEST_OUT_DIR)/%: $(TEST_DIR)/%.c
	$(CC) -o $@ $<

$(DRIVER_OUT_DIR)/%: $(DRIVER_DIR)/%.c
	$(CC) -o $@ $<

all: $(DRIVER_OUT_FILES)

cunit: $(TEST_OUT_FILES)

clean:
	rm -rf $(OUT_DIR)
	mkdir $(OUT_DIR)
	mkdir $(TEST_OUT_DIR)
	mkdir $(DRIVER_OUT_DIR)

