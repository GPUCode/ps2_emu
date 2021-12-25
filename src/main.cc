#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <common/emulator.h>
#include <thread>

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "PS2 Emulator", NULL, NULL);
    if (window == NULL)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* window, int width, int height)
	{
		glViewport(0, 0, width, height);
	});

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return -1;

    common::Emulator emulator;
    std::thread thread([&]() { while (!emulator.stop_thread) { emulator.tick(); } });

    while (!glfwWindowShouldClose(window))
    {
		if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        	glfwSetWindowShouldClose(window, true);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    emulator.stop_thread = true;
    thread.join();

    glfwTerminate();
    return 0;
}