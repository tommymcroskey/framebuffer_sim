# framebuffer_sim
## Description

The purpose of this project is to introduce myself to rendering in C. Because I didn't know anything yet at the time of starting, I opted against using any third-party libraries to ensure I understood what they were doing first. Instead, I wrote direct to /dev/fb0. To keep the project simple, the screen is rendered in a serial fashion, but this process is embarassingly parallel and so could be sped up with some pthreads quite easily. Otherwise, some GPU programming really does make sense for simulations (crazy huh?), so I'll be moving to OpenGL or Vulcan in the future, with all likelihood.
