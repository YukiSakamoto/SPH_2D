#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif


struct Particle {
    
};

// 点の位置
float x_pos = -0.8f; // 初期位置（左側）
float speed = 0.01f; // 移動速度

// 描画関数
void render() {
    glClear(GL_COLOR_BUFFER_BIT);

    glBegin(GL_POINTS);
    glVertex2f(x_pos, 0.0f);  // 点を描画
    glEnd();

    glFlush();
}

// アニメーション更新
void update() {
    x_pos += speed;
    if (x_pos > 0.8f || x_pos < -0.8f) {
        speed = -speed; // 端で跳ね返る
    }
}


GLFWwindow* window;

void main_loop_func() {
    update();
    render();
    glfwSwapBuffers(window);
    glfwPollEvents();
}

int main() {
    // GLFWの初期化
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // ウィンドウを作成
    window = glfwCreateWindow(500, 500, "GLFW Animation", nullptr, nullptr);
    //GLFWwindow* window = glfwCreateWindow(500, 500, "GLFW Animation", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // OpenGLの設定
    glPointSize(10.0f);  // 点のサイズ
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // 背景を黒

    // メインループ
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop_func, 0, 1);
#else
    while (!glfwWindowShouldClose(window)) {
        main_loop_func();
    }
#endif

    // クリーンアップ
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

