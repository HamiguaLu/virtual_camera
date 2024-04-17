# A virtual USB camera on Windows 10 using the Windows Driver Framework (WDF)

Device Descriptor: This describes the overall characteristics of the device, including its vendor ID, product ID, and device class/subclass.

Configuration Descriptor: This describes the device's configuration, including the number and types of interfaces supported.

UVC Specifications: Understanding the UVC specifications is crucial, as it defines the standard for USB video devices. This includes formats for video and still images, control commands, and more.

UDE (User-Mode Driver Framework): You may need to develop a user-mode driver using the UDE framework to manage the virtual camera device and handle communication with applications like Zoom.

USB Interface Association Descriptor: This descriptor is used to associate multiple interfaces belonging to a function.

Control Commands (PTZ): To allow Zoom to control the virtual camera's Pan-Tilt-Zoom (PTZ) functionality, you'll need to implement the necessary UVC control commands.

It's important to thoroughly understand these concepts and refer to the UVC specifications and Windows Driver Framework documentation for detailed guidance. Additionally, you may find the links you provided and other resources helpful for implementing the virtual USB camera


If you only need basic camera functionality, there is no need to virtualize USB. You can consider solutions like DirectShow or AVstream. The project requires transferring audio and video data from a network NDI camera to Zoom and controlling the camera (PTZ) through Zoom. From what I understand, Zoom can only control UVC (USB video camera) cameras, so virtual USB cameras are the only option. 

Here, I will introduce the implementation based on Microsoft's UDE framework. If you also want to implement similar functionality, it is recommended to familiarize yourself with the following content to avoid taking detours:

UDE documentation

UVC 1.5 Class Specification

USB Interface Association Descriptor

UDEMbimClientSample GitHub repository

Device Descriptor: Refer to section 3.1 of the UVC documentation. Essentially, we only need to pay attention to the device descriptor and configuration descriptor. Here is the descriptor information for the Logitech Webcam C170 for reference."
The translation may not be perfect due to technical terminology and context, so please let me know if you need further clarification or if there are specific parts you would like to focus on
