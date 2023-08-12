#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#define FMT_HEADER_ONLY
#include <fmt/core.h> 
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <set>

// window specifics
const uint32_t window_width = 1920;
const uint32_t window_height = 1080;

// vulkan specifics

const std::vector<const char*> enabled_validation_layers = {
	"VK_LAYER_KHRONOS_validation"
};

std::vector<const char*> enabled_extensions=  {};



const std::vector<const char*> enabled_physical_device_extensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME;
};

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

	// setup validation layers
	VkInstanceCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;
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

	assert_with_message(enable_validation_layers && !check_validation_layer_support(enabled_validation_layers), "[vk] validation layers requested but are not available.");
	VkResult result = vkCreateInstance(&create_info, nullptr, &vk_instance);
	assert_with_message(result == VK_SUCCESS, "[vk] vkCreateInstance failed.");


	VkDebugUtilsMessengerEXT debug_messenger{};
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
			assert_with_message(func == nullptr,"[vk] could not find function vkCreateDebugUtilsMessengerEXT");
		}
	}

	VkSurfaceKHR surface{};
	VkWin32SurfaceCreateInfoKHR surface_create_info{};
	{
		surface_create_info.sType =  VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surface_create_info.hwnd = glfwGetWin32Window(main_window);
		surface_create_info.hinstance = GetModuleHandle(nullptr);

		auto result = vkCreateWin32SurfaceKHR(vk_instance, &surface_create_info, nullptr, &surface);
		assert_with_message(result == VK_SUCCESS, "[vk][ Failed to vkCreateWin32SurfaceKHR.");
	}

	// glfw actually create the window surface
	{
		auto result = glfwCreateWindowSurface(vk_instance, main_window, nullptr, &surface);
		assert_with_message(result == VK_SUCCESS, "[glfw] failed to create a window surface.");
	}

	// --> we have an instance with validation layers and a debug messenger.



	// physical device
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	{
		// get all physical devices.
		uint32_t device_count = 0;
		vkEnumeratePhysicalDevices(vk_instance, &device_count, nullptr);
		assert_with_message(device_count > 0, "[vk] failed to find any GPU with vulkan support.");
		std::vector<VkPhysicalDevice> devices(device_count);
		vkEnumeratePhysicalDevices(vk_instance, &device_count, devices.data());


		// do we have any suitable devices?
		//@FIXME(SJM): we do not distinguish between multiple devices here.
		for (const auto& device: devices)
		{
			// do we have discrete gpu and geometry shader support?
			VkPhysicalDeviceProperties device_properties{};
			vkGetPhysicalDeviceProperties(device, &device_properties);
			VkPhysicalDeviceFeatures device_features{};
			vkGetPhysicalDeviceFeatures(device, &device_features);
			bool device_has_discrete_gpu_and_geometry_shader = (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) && device_features.geometryShader;


			//what extensions do we have?
			uint32_t extension_count{};
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
			std::vector<VkExtensionProperties> available_extensions{extension_count};
			vkEnumerateDeviceExtensionProperties(device, nullptr, extension_count, available_extensions.data());

			std::set<std::string> required_physical_device_extensions(enabled_physical_device_extensions.begin(), enabled_physical_device_extensions.end());

			for (const auto& extension: available_extensions)
			{
				required_physical_device_extensions.erase(extension.extensionName);
			}
			bool device_has_required_extensions = required_physical_device_extensions.empty();

			bool device_is_suitable = device_has_discrete_gpu_and_geometry_shader && device_has_required_extensions;
			fmt::print("[vk] found suitable device: {}\n", device_is_suitable);
			if (device_is_suitable) physical_device = device;
		}


	}

	// which queue families are supported?
	// -1 is sentinel value.
	struct queue_family_indices_t {
		uint32_t graphics_family = -1; 
		uint32_t present_family = -1;
	};

	auto queue_family_indices_is_complete = [](queue_family_indices_t& indices) -> bool 
	{
		return (indices.graphics_family != -1  && indices.present_family != -1);
	};

	queue_family_indices_t indices;
	{
		uint32_t queue_family_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

		std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());


		for (size_t idx = 0; idx != queue_family_count; ++idx)
		{
			if (queue_families[idx].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphics_family = idx;
			}

			VkBool32 present_support = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, idx, surface, &present_support);

			if (present_support)
			{
				indices.present_family = idx;
			}

			if (queue_family_indices_is_complete(indices))
			{
				break;
			}

		}
		assert_with_message(indices.graphics_family != -1, "we did not actually find any queues that have the graphics bit set.");

		// also get the present support.
	}
	
	VkDevice device{}; // "logical" device
	{
		VkDeviceCreateInfo device_create_info{};
		device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_create_info.enabledExtensionCount = 0;

		VkPhysicalDeviceFeatures device_features{}; // default 0 for now.
		device_create_info.pEnabledFeatures= &device_features;
		// this is not strictly necessary apparently but we do it anyway(?)
		
		if (enable_validation_layers)
		{
			device_create_info.enabledLayerCount = static_cast<uint32_t>(enabled_validation_layers.size());
			device_create_info.ppEnabledLayerNames = enabled_validation_layers.data();
		} else 
		{
			device_create_info.enabledLayerCount = 0;
		}

		// create the presentation queue and retrieve the VkQueue handle.
		std::vector<VkDeviceQueueCreateInfo> queue_create_infos{};
		std::set<uint32_t> unique_queue_families = {indices.graphics_family, indices.present_family};

		float queue_priority = 1.0f;
		for (auto queue_family: unique_queue_families)
		{
			VkDeviceQueueCreateInfo queue_create_info{};
			queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_create_info.queueFamilyIndex = queue_family;
			queue_create_info.queueCount = 1;
			queue_create_info.pQueuePriorities = &queue_priority;
			queue_create_infos.push_back(queue_create_info);
		}

		device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
		device_create_info.pQueueCreateInfos = queue_create_infos.data();

		auto result = vkCreateDevice(physical_device, &device_create_info, nullptr, &device);
		assert_with_message(result == VK_SUCCESS, "[vk] Failed to create logical device.");
		fmt::print("[vk] created logical device.\n");


	}

	VkQueue graphics_queue{};
	{
		vkGetDeviceQueue(device, indices.graphics_family, 0, &graphics_queue);
	}
	VkQueue present_queue{};
	{
		vkGetDeviceQueue(device, indices.present_family, 0, &present_queue);
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
			assert_with_message(func == nullptr, "[vk] could not find vkDestroyDebugUtilsMessengerEXT.");
		}
	}

	// destroy the surface.
	vkDestroySurfaceKHR(vk_instance, surface, nullptr);

	// destroy instance
	vkDestroyInstance(vk_instance, nullptr);


	glfwDestroyWindow(main_window);
	glfwTerminate();
}
