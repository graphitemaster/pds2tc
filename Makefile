CC ?= clang
CFLAGS = -std=c99 -Wall -Wextra -pedantic -O2
LDFLAGS =
SOURCES = s2tc.c
OBJECTS = s2tc.o
S2TC = pds2tc.a

all: $(S2TC)

$(S2TC): $(OBJECTS)
	ar rcs $@ $^

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(S2TC)
