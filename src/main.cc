#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define FMT_HEADER_ONLY
#include <fmt/core.h> 
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>

// window specifics
const uint32_t window_width = 1920;
const uint32_t window_height = 1080;

// vulkan specifics

const std::vector<const char*> enabled_validation_layers = {
	"VK_LAYER_KHRONOS_validation"
};

std::vector<const char*> enabled_extensions=  {};

#ifdef NDEBUG
	const bool enable_validation_layers= false;
#else
	const bool enable_validation_layers = true;
#endif



static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
	VkDebugUtilsMessageTypeFlagsEXT message_type,
	const VkDebugUtilsMessengerCallbackDataEXT* callback_data_ptr,
	void* user_data_ptr)
{
	fmt::print("[vk] Validation Layer: {}\n", callback_data_ptr->pMessage);

	// we should always return VK_FALSE apparently. sure.
	return VK_FALSE;
}

void on_key_pressed(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
        // Key pressed
        // Handle the key press here
        switch (key) {
            case GLFW_KEY_ESCAPE:
                glfwSetWindowShouldClose(window, GLFW_TRUE); // Close the window when the Escape key is pressed
                break;
            default:
                // Handle other key presses
                break;
        }
    } else if (action == GLFW_RELEASE) {
        // Key released
        // Handle the key release here
    }
}

static bool check_validation_layer_support(const std::vector<const char*>& validation_layers)
{
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count,
		available_layers.data());

	for (const char* layer_name: validation_layers)
	{
		bool layer_found = false;
		for (const auto& layer_properties: available_layers)
		{
			fmt::print("supported layer: {}\n", layer_properties.layerName);
			if (strcmp(layer_name, layer_properties.layerName) == 0)
			{
				layer_found = true;
				break; // huh?
			}
		}
		if (!layer_found)
		{
			return false;
		}
	}

	return false;
}

static void populate_DebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info)
{
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity = 
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType =  VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = debug_callback;
	create_info.pUserData = nullptr;
}



#define assert_with_message(condition, message) \
    do { \
        if (!(condition)) { \
            fmt::print("Assertion failed: {}, {}, line {}\n", #condition, message, __LINE__); \
            std::terminate(); \
        } \
    } while (false)



int main()
{
	GLFWwindow* main_window = nullptr;
	uint32_t glfw_extension_count = 0;
	const char** glfw_required_extensions;
	// glfw stuff
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		main_window = glfwCreateWindow(window_width, window_height, "Vulkan", nullptr, nullptr);
	    // Set the key callback function
    	glfwSetKeyCallback(main_window, on_key_pressed);


		// get glfw extension count and required extensions for vk.
		glfw_required_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
		fmt::print("[glfw] required extension count: {}. Required extensions are: \n", glfw_extension_count);
		for (size_t idx = 0; idx != glfw_extension_count; ++idx)
		{
			fmt::print("\t {}\n", glfw_required_extensions[idx]);
		}
	}

	// enable vk extensions 
	{
		uint32_t extension_count = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
		fmt::print("[vk] {} extensions supported.\n", extension_count);
		std::vector<VkExtensionProperties> extensions(extension_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());
		for (const auto& extension: extensions)
		{
			fmt::print("\t {}\n", extension.extensionName);
		}
	}
	// this is getting messy. We need to add the required glfw extensions _and_ the validation layer extensions together.
	std::vector<const char*> extensions(glfw_required_extensions, glfw_required_extensions + glfw_extension_count);
	if (enable_validation_layers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	enabled_extensions = extensions;


	VkInstance vk_instance{};
	VkApplicationInfo app_info{};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; // reflection?
	app_info.pApplicationName = "Hello Triangle";
	app_info.applicationVersion = VK_MAKE_VERSION(1,0,0);
	app_info.pEngineName = "My Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1,0,0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;
	// setup validation layers
	if (enable_validation_layers) 
	{
		create_info.enabledLayerCount = static_cast<uint32_t>(enabled_validation_layers.size());
		create_info.ppEnabledLayerNames = enabled_validation_layers.data();
	}
	else
	{
		create_info.enabledLayerCount = 0;
	}
	// set up extension count.
	create_info.enabledExtensionCount = static_cast<uint32_t>(enabled_extensions.size());
	create_info.ppEnabledExtensionNames= enabled_extensions.data();


	// set up debug messenger if we have validation layers on.
	VkDebugUtilsMessengerCreateInfoEXT debug_create_info{}; 
	if (enable_validation_layers)
	{
		populate_DebugMessengerCreateInfo(debug_create_info);
		create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
	}

	assert_with_message(enable_validation_layers && !check_validation_layer_support(enabled_validation_layers), "validation layers requested but are not available.");
	VkResult result = vkCreateInstance(&create_info, nullptr, &vk_instance);

	assert_with_message(result == VK_SUCCESS, "vkCreateInstance failed.");

	VkDebugUtilsMessengerEXT debug_messenger;
	if (enable_validation_layers)
	{
		VkDebugUtilsMessengerCreateInfoEXT create_info{};
		populate_DebugMessengerCreateInfo(create_info);

		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)(vkGetInstanceProcAddr(vk_instance, "vkCreateDebugUtilsMessengerEXT")); // does the instance need to exist at this point in time?
		if (func != nullptr)
		{
			VkResult create_debug_result = func(vk_instance, &create_info, nullptr, &debug_messenger);
			assert_with_message(create_debug_result == VK_SUCCESS, "vkCreateDebugUtilsMessengerEXT failed.\n");
		}
		else
		{
			assert_with_message(func == nullptr,"could not find function vkCreateDebugUtilsMessengerEXT");
		}
	}


	while (!glfwWindowShouldClose(main_window))
	{
		glfwPollEvents();
	}


	// destroy debug messenger.
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)(vkGetInstanceProcAddr(vk_instance, "vkDestroyDebugUtilsMessengerEXT")); // does the instance need to exist at this point in time?
		if (func != nullptr)
		{
			func(vk_instance, debug_messenger, nullptr);
		}
		else
		{
			assert_with_message(func == nullptr, "could not find vkDestroyDebugUtilsMessengerEXT.");
		}
	}

	// destroy instance
	vkDestroyInstance(vk_instance, nullptr);


	glfwDestroyWindow(main_window);
	glfwTerminate();
}
