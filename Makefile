.PHONY: test clean icons assets fap launch format lint

CORE := src/core
CORE_SRC := $(CORE)/digiflip_game.c $(CORE)/digiflip_roster.c
CORE_DEPS := $(wildcard $(CORE)/*.h $(CORE)/*.inc)
CFLAGS_TEST := -std=c11 -Wall -Wextra -Werror -pedantic

test: build/test_game build/test_roster build/test_album build/test_log
	./build/test_game
	./build/test_roster
	./build/test_album
	./build/test_log

build/test_game: $(CORE_SRC) $(CORE_DEPS) tests/test_game.c
	mkdir -p build
	$(CC) $(CFLAGS_TEST) $(CORE_SRC) tests/test_game.c -o $@

build/test_roster: $(CORE)/digiflip_roster.c $(CORE_DEPS) tests/test_roster.c
	mkdir -p build
	$(CC) $(CFLAGS_TEST) $(CORE)/digiflip_roster.c tests/test_roster.c -o $@

build/test_album: $(CORE)/digiflip_album.c $(CORE)/digiflip_unlock.c $(CORE)/digiflip_roster.c $(CORE_DEPS) tests/test_album.c
	mkdir -p build
	$(CC) $(CFLAGS_TEST) $(CORE)/digiflip_album.c $(CORE)/digiflip_unlock.c $(CORE)/digiflip_roster.c tests/test_album.c -o $@

build/test_log: $(CORE)/digiflip_log.c $(CORE)/digiflip_roster.c $(CORE_DEPS) tests/test_log.c
	mkdir -p build
	$(CC) $(CFLAGS_TEST) $(CORE)/digiflip_log.c $(CORE)/digiflip_roster.c tests/test_log.c -o $@

clean:
	$(RM) -r build

# The compiled-in icons (images/*.png, digiflip_icon.png) are build output
# from the text grids in assets/icons/.
icons:
	python3 tools/make_icons.py

# Also rebuild the SD-card sprite pack (plus its review sheet).
assets: icons
	python3 tools/build_sprites.py --preview assets/poses/preview.png

# Build / install-and-run the app with uFBT.
fap: icons
	ufbt

launch: icons
	ufbt launch

format:
	ufbt format

lint:
	ufbt lint

