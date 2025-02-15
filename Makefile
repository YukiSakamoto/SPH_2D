
CC=g++
OMP_FLAG=-Xpreprocessor -fopenmp -lomp 
point:
	$(CC) -framework OpenGL -lglfw -Wno-deprecated point.cpp -o point
em:
	em++ point.cpp -o point.html -s USE_GLFW=3 -s FULL_ES2=1 -s WASM=1 -s LEGACY_GL_EMULATION=1

sph_emscripten: sph.cpp
	em++ sph.cpp -o main.html -s USE_GLFW=3 -s WASM=1 -s LEGACY_GL_EMULATION=1 -O3 -Wno-deprecated
sph: sph.cpp
	$(CC) sph.cpp -o sph -lglfw -framework OpenGL -std=c++17 -Wno-deprecated -O3
sph_omp: sph.cpp
	$(CC) sph.cpp -o sph_omp -lglfw -framework OpenGL -std=c++17 -Wno-deprecated -O3 $(OMP_FLAG) $(OMP_CPPFLAGS) $(OMP_LDFLAGS)
hoge: hoge.cpp
	$(CC) hoge.cpp -o hoge -lglfw -framework OpenGL -std=c++17 -Wno-deprecated -O3 $(OMP_FLAG) $(OMP_CPPFLAGS) $(OMP_LDFLAGS)
