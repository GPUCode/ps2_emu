#include <gs/gsrenderer.h>
#include <glad/glad.h>
#include <fmt/core.h>

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in ivec3 aPos;

void main()
{
    float pos_x = (aPos.x / 320.0f) - 1.0f;
    float pos_y = 1.0f - (aPos.y / 112.0f);
    gl_Position = vec4(pos_y, pos_x, aPos.z, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

void main()
{
   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
}
)";

namespace gs
{
	GSRenderer::GSRenderer()
	{
        int success;
        char infoLog[512];

        /* Compile vertex and fragment shaders */
        uint32_t vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vertexShaderSource, NULL);
        glCompileShader(vertex);

        glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertex, 512, NULL, infoLog);
            fmt::print("[GS][OpenGL] Vertex shader compilation failed\n");
        }

        uint32_t fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragment);
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragment, 512, NULL, infoLog);
            fmt::print("[GS][OpenGL] Fragment shader compilation failed\n");
        }
        
        /* Link shaders */
        uint32_t program = glCreateProgram();
        glAttachShader(program, vertex);
        glAttachShader(program, fragment);
        glLinkProgram(program);
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) 
        {
            glGetProgramInfoLog(program, 512, NULL, infoLog);
            fmt::print("[GS][OpenGL] Shader linking failed\n");
        }

        /* Delete unused objects */
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        glUseProgram(program);

        /* Configure our VBO and VAO */
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
	}
    
    void GSRenderer::render()
    {
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    void GSRenderer::submit_sprite(Vert v1, Vert v2)
    {
        int vertices[] = 
        {
            v2.x, v1.y, 0,
            v2.x, v2.y, 0,
            v1.x, v1.y, 0,
            v2.x, v2.y, 0,
            v1.x, v2.y, 0,
            v1.x, v1.y, 0
        };

        /* Add data to the VBO */
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

        glVertexAttribIPointer(0, 3, GL_INT, 3 * sizeof(int), (void*)0);
        glEnableVertexAttribArray(0);
    }
}