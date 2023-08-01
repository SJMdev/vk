

if we look at the avaiable validation layers, it seems that we do not have the validation layer at this moment?



# Lessons Learned
 
 - We need to include the vulkan headers, which are part of the SDK.
 - we need to link to the vulkan libraries, which are part of the SDK.
 ( I do not know yet if this is true:)
 - we need to set VK_LAYER_PATH env variable to use validation layers. e.g.
 `set VK_LAYER_PATH=C:\VulkanSDK\1.1.121\Bin`