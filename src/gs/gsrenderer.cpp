#include <gs/gsrenderer.h>
#include <glad/glad.h>
#include <common/emulator.h>

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 ourColor; // output a color to the fragment shader

void main()
{
    gl_Position = vec4(aPos, 1.0);
    ourColor = aColor; // set ourColor to the input color we got from the vertex data
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec3 ourColor;

void main()
{
   FragColor = vec4(ourColor, 1.0);
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
            common::Emulator::terminate("[GS][OpenGL] Vertex shader compilation failed\n");
        }

        uint32_t fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragment);
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragment, 512, NULL, infoLog);
            common::Emulator::terminate("[GS][OpenGL] Fragment shader compilation failed\n");
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
            common::Emulator::terminate("[GS][OpenGL] Shader linking failed\n");
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

        glBufferData(GL_ARRAY_BUFFER, sizeof(GSVertex) * 1024 * 512, nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GSVertex), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GSVertex), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        /* Enable depth testing */
        glEnable(GL_DEPTH_TEST);
	}
    
    void GSRenderer::render()
    {
        /* Add data to the VBO */
        glBufferSubData(GL_ARRAY_BUFFER, 0, draw_data.size() * sizeof(GSVertex), draw_data.data());

        /* Draw vertices */
        glDrawArrays(GL_TRIANGLES, 0, draw_data.size());
        
        draw_data.clear();
    }
    
    void GSRenderer::submit_vertex(GSVertex v1)
    {
        draw_data.push_back(v1);
    }

    void GSRenderer::submit_sprite(GSVertex v1, GSVertex v2)
    {
        draw_data.push_back({ .x = v2.x, .y = v1.y });
        draw_data.push_back({ .x = v2.x, .y = v2.y });
        draw_data.push_back({ .x = v1.x, .y = v1.y });
        draw_data.push_back({ .x = v2.x, .y = v2.y });
        draw_data.push_back({ .x = v1.x, .y = v2.y });
        draw_data.push_back({ .x = v1.x, .y = v1.y });
    }
}
