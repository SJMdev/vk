#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define FMT_HEADER_ONLY
#include <fmt/core.h> 
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const int window_width = 1920;
const int window_height = 1080;

int main()
{
	GLFWwindow* main_window = nullptr;
	// glfw stuff
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		main_window = glfwCreateWindow(window_width, window_height, "Vulkan", nullptr, nullptr);

	}

	// vk stuff
	{
		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
		fmt::print("[vk] {} extensions supported.\n", extension_count);
	}


	while (!glfwWindowShouldClose(main_window))
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(main_window);
	glfwTerminate();
}
