

if we look at the avaiable validation layers, it seems that we do not have the validation layer at this moment?

I launched the Vulkan configurator and launched my executable and hijacked the validation layers, and from that point onward,
the other validation layers are available. It seems there is something weird going on with the registry(????????).

I do not know how other people fix this on console (are environment variables available there?)


# setup
- Install vulkan SDK
- point to the correct path in the build.bat script.
- you should be g2g!


# Lessons Learned
 
 - We need to include the vulkan headers, which are part of the SDK.
 - we need to link to the vulkan libraries, which are part of the SDK.
 ( I do not know yet if this is true:)
 - we need to set VK_LAYER_PATH env variable to use validation layers. e.g.
 `set VK_LAYER_PATH=C:\VulkanSDK\1.1.121\Bin`