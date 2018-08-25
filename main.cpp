#include <iostream>
#include <math.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <thread>
#include <mutex>

#include "ray_tracing.h"
#include "stb_image_write.h"

const char *vertex_shader_src = R"(#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main()
{
	gl_Position = vec4(aPos, 1.0);
	TexCoord = aTexCoord;
})";

const char *fragment_shader_src = R"(#version 450 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D ourTexture;

void main()
{
   FragColor = texture(ourTexture, TexCoord);
})";

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
unsigned char frame_buffer[3 * SCR_WIDTH*SCR_HEIGHT];

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}

	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
	{
		int res = stbi_write_png("R:\\ray_trace.png", SCR_WIDTH, SCR_HEIGHT, 3, frame_buffer, 3*SCR_WIDTH);
		if (res != 1) {
			puts("Writing to PNG failed");
		}
		else {
			puts("Image saved");
		}

	}
}

void check_shader_status(unsigned int shader, unsigned int check)
{
	int  success;
	char infoLog[512];
	glGetShaderiv(shader, check, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
}

void ray_trace_worker(int width, int height, int stride, int *current_row, hitable *world, unsigned char *buffer, std::mutex *lock)
{
	while (true) {
		int row;
		{
			std::lock_guard<std::mutex> guard(*lock);
			if (*current_row >= height) {
				break;
			}
			row = *current_row;
			*current_row += stride;
		}

		ray_trace(width, height, row, row + stride, world, buffer);
	}
}

void ray_trace_job(int width, int height, bool *finished, unsigned char *buffer)
{
	unsigned int n = std::thread::hardware_concurrency();
	std::mutex render_mutex;
	int rows_processed = 0;
	hitable *world = random_scene();
	std::thread *threads = new std::thread[n];

	for (unsigned int i = 0; i < n; i++) {
		new (threads + i) std::thread(ray_trace_worker, width, height, 30, &rows_processed, world, buffer, &render_mutex);
	}

	for (unsigned int i = 0; i < n; i++) {
		threads[i].join();
	}

	*finished = true;
}

int main()
{
	stbi_flip_vertically_on_write(1);

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	printf("OpenGL Vendor: %s\n", glGetString(GL_VENDOR));
	printf("OpenGL Renderer: %s\n", glGetString(GL_RENDERER));
	printf("OpenGL Version: %s\n", glGetString(GL_VERSION));

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// vertex shader
	unsigned int vertexShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertex_shader_src, NULL);
	glCompileShader(vertexShader);

	// fragment shader
	unsigned int fragmentShader;
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragment_shader_src, NULL);
	glCompileShader(fragmentShader);

	check_shader_status(vertexShader, GL_COMPILE_STATUS);
	check_shader_status(fragmentShader, GL_COMPILE_STATUS);

	// create shader program
	unsigned int shaderProgram;
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	check_shader_status(shaderProgram, GL_LINK_STATUS);

	// once you linked shaders into a program, you can delete everything else
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// prepare triangle
	float vertices[] = {
		 1.0f,  1.0f, 0.0f,		1.0f, 1.0f, // top right
		 1.0f, -1.0f, 0.0f,		1.0f, 0.0f, // bottom right
		-1.0f, -1.0f, 0.0f,		0.0f, 0.0f, // bottom left
		-1.0f,  1.0f, 0.0f,		0.0f, 1.0f  // top left 
	};
	unsigned int indices[] = {  // note that we start from 0!
		0, 1, 3,   // first triangle
		1, 2, 3    // second triangle
	};

	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// set up indexing structure into the vertex array
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3*sizeof(float)));
	glEnableVertexAttribArray(1);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	// Prepare a single texture: ray trace the scene
	bool finished_rendering = false;
	bool final_render = false;
	std::thread worker(ray_trace_job, SCR_WIDTH, SCR_HEIGHT, &finished_rendering, frame_buffer);

	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	auto last_render = std::chrono::system_clock::now();
	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		auto now = std::chrono::system_clock::now();

		if (!final_render && now - last_render > std::chrono::milliseconds(1500)) {
			// Yes, I am aware that while the texture is copying, ray tracing is still happening
			// Since this is a read only task, there's really no harm I don't think
			if (finished_rendering) {
				final_render = true;
			}

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, frame_buffer);
			glGenerateMipmap(GL_TEXTURE_2D);
			last_render = now;
		}

		// draw our first triangle
		glUseProgram(shaderProgram);
		glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
		//glDrawArrays(GL_TRIANGLES, 0, 6);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		// glBindVertexArray(0); // no need to unbind it every time 

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}