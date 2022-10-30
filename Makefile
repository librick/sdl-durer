
default:
	g++ src/*.cpp -g -std=c++17 -lSDL2 -lSDL2_image -Wall `sdl2-config --cflags --libs` -o out 
html:
	mkdir -p html
	emcc --embed-file res -sNO_DISABLE_EXCEPTION_CATCHING  -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' src/main.cpp -o html/durer.html
web:
	mkdir -p dist
	emcc --embed-file res -s ENVIRONMENT='web' -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' src/main.cpp -o dist/durer.mjs
clean:
	rm -rf ./build
	rm -rf ./dist
	rm ./out
run: default
	./out
