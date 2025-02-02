#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <Eigen/Dense>
#include <cstdio>

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

GLFWwindow* window;

struct Particle
{
    Particle(float x, float y): r(x,y), v(0.0, 0.0), f(0.0, 0.0), rho(0.0), p(0.0){;}
    Eigen::Vector2d r;
    Eigen::Vector2d v;
    Eigen::Vector2d f;
    float rho, p;
};
std::vector<Particle> particles;

const static int WINDOW_WIDTH = 800;
const static int WINDOW_HEIGHT = 600;
const static float VIEW_WIDTH = 1.5 * float(WINDOW_WIDTH);
const static float VIEW_HEIGHT= 1.5 * float(WINDOW_HEIGHT);

const static Eigen::Vector2d G(0.f, -10.f);
const static float REST_DENS = 300.f;
const static float GAS_CONST = 2000.f;  // const for eq of state
//const static float H = 16.f;
const static float H = 8.f;
const static float HSQ = H*H;       // radius^2 for optimization
const static float MASS = 2.5f;     //assume all particles have the same mass
const static float VISC = 200.f;    // viscosity constant
const static float DT = 0.0007f;
const static float EPS = H;

const static float POLY6 = 4.f / (M_PI * pow(H, 8.f));
const static float SPIKY_GRAD = -10.f / (M_PI * pow(H, 5.f));
const static float VISC_LAP = 40.f / (M_PI * pow(H, 5.f));
const static float BOUND_DAMPING = -0.5f;

void ComputeDensityPressure(void)
{
    //for(auto &pi : particles) {
    #pragma omp parallel for
    for(size_t i = 0; i < particles.size(); i++) {
        Particle &pi = particles[i];
        pi.rho = 0.0;
        for(auto &pj : particles) {
            Eigen::Vector2d rij = pj.r - pi.r;
            float r2 = rij.squaredNorm();

            if (r2 < HSQ) {
                pi.rho += MASS * POLY6 * pow(HSQ-r2, 3.f);
            }
        }
        pi.p = GAS_CONST * (pi.rho - REST_DENS);
    }
}

void ComputeForces(void)
{
    //for(auto &pi : particles) {
    #pragma omp parallel for
    for(size_t i = 0; i < particles.size(); i++) {
        Particle &pi = particles[i];
        Eigen::Vector2d fpress(0.f, 0.f);
        Eigen::Vector2d fvisc(0.f, 0.f);
        for(auto &pj : particles) {
            if (&pi == &pj){
                continue;
            }
            Eigen::Vector2d rij = pj.r - pi.r;
            float r = rij.norm();
            if (r < H) {
                fpress += -rij.normalized() * MASS * (pi.p + pj.p) / (2.f * pj.rho) * SPIKY_GRAD * pow(H - r, 3.f);
                fvisc += VISC * MASS * (pj.v - pi.v) / pj.rho * VISC_LAP * (H-r);
            }
        }
        Eigen::Vector2d fgrav = G*MASS / pi.rho;
        pi.f = fpress + fvisc + fgrav;
    }
}

void Integrate(void)
{
    for(auto &p : particles) {
        p.v += DT * p.f / p.rho;
        p.r += DT * p.v;
        if (p.r(0) - EPS < 0.f){
            p.v(0) *= BOUND_DAMPING;
            p.r(0) = EPS;
        }
        if (p.r(0) + EPS > VIEW_WIDTH) {
            p.v(0) *= BOUND_DAMPING;
            p.r(0) = VIEW_WIDTH - EPS;
        }
        if (p.r(1) - EPS < 0.f){
            p.v(1) *= BOUND_DAMPING;
            p.r(1) = EPS;
        }
        if (p.r(1) + EPS > VIEW_HEIGHT) {
            p.v(1) *= BOUND_DAMPING;
            p.r(1) = VIEW_HEIGHT - EPS;
        }
    }
}

void InitSPH(void)
{
    const size_t num_particles_limit = 3000;
    for(float y = EPS; y < VIEW_HEIGHT - EPS * 2.0; y += H) {
        for(float x = VIEW_WIDTH/ 4; x <= VIEW_WIDTH / 2; x += H) {
            if (particles.size() < num_particles_limit) {
                //float jitter = static_cast<float>(arc4random())/static_cast<float>(RAND_MAX);
                float jitter = static_cast<float>(rand())/static_cast<float>(RAND_MAX);
                particles.push_back(Particle(x+jitter, y));
            } else {
                return;
            }
        }
    }
    std::cout << particles.size() << std::endl;
}

void hsvToRgb(float h, float s, float v, float& r, float& g, float& b) {
    float c = v * s;
    float x = c * (1 - fabs(fmod(h / 60.0, 2) - 1));
    float m = v - c;

    if (h >= 0 && h < 60) {
        r = c, g = x, b = 0;
    } else if (h >= 60 && h < 120) {
        r = x, g = c, b = 0;
    } else if (h >= 120 && h < 180) {
        r = 0, g = c, b = x;
    } else if (h >= 180 && h < 240) {
        r = 0, g = x, b = c;
    } else if (h >= 240 && h < 300) {
        r = x, g = 0, b = c;
    } else {
        r = c, g = 0, b = x;
    }

    r += m;
    g += m;
    b += m;
}

// 描画関数（点を描画）
void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();
    glOrtho(0, VIEW_WIDTH, 0, VIEW_HEIGHT, 0, 1);

    glBegin(GL_POINTS);
    //glVertex2f(0.0f, 0.0f);  // 原点に点を描画
    for(auto &p: particles) {
        float h = 210.0 - p.rho * 210.0;
        float r, g, b;
        hsvToRgb(h, 1.0, 1.0, r, g, b);
        glColor4f(r, g, b, 1.0);
        glVertex2f(p.r[0], p.r[1]);
    }
    glEnd();

    glfwSwapBuffers(window);
    glfwPollEvents();
}

void update()
{
    ComputeDensityPressure();
    ComputeForces();
    Integrate();
}

// Emscripten 用のメインループ
void main_loop_func() { 
    update();
    render(); 
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_Q) {
            std::cout << "QUIT!" << std::endl;
#ifdef __EMSCRIPTEN__
            emscripten_cancel_main_loop();
#else
            glfwSetWindowShouldClose(window, true);
#endif
        }
        else if (key == GLFW_KEY_R) {
            particles.clear();
            std::cerr << "Clear and Rerun!" << std::endl;
            InitSPH();
        }
    }
}

void InitGL(void)
{
    glClearColor(0.9f, 0.9f, 0.9f, 1);
    glEnable(GL_POINT_SMOOTH);
    glPointSize(H / 2.f);
    glMatrixMode(GL_PROJECTION);
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

#ifdef _OPENMP
    std::printf("Run on %d threads\n", omp_get_num_threads());
#endif

    // macOS の場合は OpenGL 3.2 Core Profile を指定
#ifndef __EMSCRIPTEN__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2); // OpenGL 2.1 互換
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#endif

    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "GLFW OpenGL", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);

    InitGL();
    InitSPH();

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop_func, 0, 1);
#else
    while (!glfwWindowShouldClose(window)) {
        main_loop_func();
    }
#endif

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

