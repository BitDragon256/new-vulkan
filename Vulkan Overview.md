# Initialization
1. Instance
2. Physical Device
	- enumerate for all devices
	- pick one depending on supported *properties*, *features* and *queue families*
		- check for queue families:
			- graphics
			- presentation
1. Logical Device & Queues
	- specify needed queues
	- creation of the logical device
	- retrieve the created queues
2. Window Surface
	- create surface with glfw
3. Swap Chain
4. Image Views
5. Pipeline

# Drawing
1. Framebuffer
2. Command Buffer

# Clean-Up
- Physical Device