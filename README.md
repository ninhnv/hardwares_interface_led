# Android HAL Blinking LED with RPI3
This project will straight forward show how to create a new Android HAL blinking LED using the sysfs Interface

# Basic Steps
* The basic steps to use a GPIO pin from the sysfs interface are the following:
1. Export the pin.
2. Set the pin direction (input or output).
3. If an output pin, set the level to low or high.
4. If an input pin, read the pin's level (low or high).
5. When done, unexport the pin.



