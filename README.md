# Finalized version uploaded as v1.0.
This ReadMe is not yet up to date and being worked over to provide a proper guidance to what the plugin does. Hang tight!

# Welcome to a Simple Speedometer for GW2 Nexus!
This plugin adds a graphical Speedometer with customizable measurement settings and an accurate Timer with a movement-based trigger and key bind operated pausing to the game. Both widgets have a few customization options stuffed into the configuration window of this plugin in Nexus.

# Currently available features
## The Speedometer
Graphical and numerical display of your current velocity as an overlay.

To adapt the measurements to your needs you may choose between 2D and 3D measuring of your velocity. Some applications favor the use of one over the other. JP speedruns and Beetle racing generally uses 2D measurements, while Griffon racing is mostly a 3D measuring domain.

Furthermore there is a choice to be made regarding the unit of measurement. You have a choice of three: u/s, Feet% and Beetle%.

u/s, or units per second, is the standard measurement unit. Ingame distances and skill ranges are displayed as units.

Feet% is an arbitrary conversion from u/s to a more humanly processable format, where 294 u/s as the standard out of combat movement speed is assumed to be 100%.

Beetle% is similar, but the Beetle's top speed of 1800 u/s is assumed to be 99%. This is in accordance to the Beetle Racing Speedometer.

Because the dial does not have any units on it, the needle just acts as a visual representation of your velocity. You may configure its amplitude of response independantly for each of the three units of measurement. It will play a little animation when exceeding its currently assigned maximum value. Feel the speed!

You may - for the most part, anyway - freely position and scale the Speedometer widget to your liking.

A keybind is available to hide or show the Speedometer on demand.

You may also pop a small chart with all six possible velocity values visible through the options menu, providing a quick glance for comparison.

## The Timer
Numerical display of an accurate Timer in the format of minutes, seconds and milliseconds as 00:00.000, featuring a movement-based trigger and either key bind or click operated pause and reset system.

When enabled, it will display two circles on your screen. The white circle represents the starting location, and the orange on at your feet represents you. Once the orange circle exceeds the white one, timing will start. The timer will run endlessly up to 59:59.999, where it will force-reset itself.

To pause or manually reset the timer and its starting location you have two options: Either you click the rotating stopwatch once to pause and twice for the reset, or you do the same with the customizable keybind. Moving when paused will resume timing. You may also reset the timer by returning into the currently set starting location, but that is a bit finnicky.

You may - for the most part, anyway - freely position and scale the Timer widget to your liking.

A keybind is available to hide or show the Timer on demand, but be advised: the Timer is always active when shown, and will fully reset when hidden!

## Limitations
Some limitations apply from the intended design and due to my coding ineptitude err i mean ImGui quirks and shortcomings of doing calculations ingame. Both? Both. It's both.

The Mumble API updates some stuff on every frame of GW2, but the positional data is only updated once every 40 ms. This means I only get 25 updates per second, resulting in an inherent latency of both the Speedometer and the Timer of up to 40 ms. To prevent having more than one Mumble update between two polls I poll the API 2.85x as fast and throw out the expected amount of zero readings to prevent spikes. Unfortunately because all my calculations can only be done per frame of GW2, this means ultimately I depend on the framerate GW2 is runnint at. If GW2 runs with a framerate lower than 25 fps or gets dips / stutter that bring it below that rate for a period of time, polling frequency will be lower than Mumble's update frequency. In those cases the displayed velocity may spike to double the previous value. I have taken measures to prevent exact doubles, but in phases of acceleration or deceleration some funky values are to be expected.

Scaling of the two widgets is limited because my amateurish patchwork of .png based textures and ImGui windows leads to elements responding differently to a unique scale value. That causes them to slide out of posiition. I mitigated this to the best of my abilities by working with both multipliers and absolute values, but some deformation between min and max scale is still visible.

I did not make this plugin with the intention of replacing anyone else's hard work. I merely want this to be an augmentation of sorts, because as of initial release of my Speedometer, alternatives to my knowledge were limited to external overlay by means of BLishHUD, TACO or unique creations like Killer's Beetle Racing Speedometer. The aim of the game was to provide some easily recordable values ingame without having to record the entire screen / desktop.

Lastly, I designed the Timer with the direct intention of it being used as an ingame indicator to help with run duration estimates without depending on external means, say for SAB Tribulation mode speedruns for example. The user does not need to count frames, add a timer in editing software or keep a smartphone stopwatch running next to them. The Timer is explicitly accurate to the frame as it is shown and it should be accurate within the confines of the time it took to generate that frame, but there is no mechanism in place to stop it when a certain goal is reached for example. Users will have to pick the time from the frame as that goal is reached by themselves, or accept the inaccuracy from stopping the timer themselves after the fact.

I made this plugin for fun using my own ideas. But to translate them into code i had to make heavy use of ChatGPT. As such the code will likely not measure up to certain standards or conjectures, and I am sorry for all the coders that might turn in their graves.

### Thanks a lot for your understanding. I hope you still have a use for this, and fun doing so!
 --Toxxa

