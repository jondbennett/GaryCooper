# GaryCooper
An arduino project to automatically open and close a chicken coop door based on sunrise and sunset times.

Basic hardware requirements:
Arduino (I used a Mega 2560)
GPS receiver.
Relay to control the coop door opener.

Advanced hardware requirements:
Beeper for notification.
Radio telemetry link (for drones) to send status info back to the house.
Additional relay to control the coop light.

My wife and I got tired of getting up early to let the chickens out. Like any good geek, I said I could solve that problem with a computer and some additional hardware. The biggest part of the additional hardware is the stuff to physically open and close the coop door. I am using an old garage door opener. Once that part of was installed, software development commenced. The problem is more difficult than it first appears.

As it turns out, chickens don't have watches and they don't observe the USA's stupid daylight saving time. See, they're not as dump as we first though. Their wake/sleep cycle depens on both the length of the days, and their circadian rhythms. They wake up each morning, even if it is very overcast and the light level is quite low. This means that a simple light level sensor will have problems getting in sync with the birds. Sunrise and sunset times are a better choice.

To make things more complicated, sunrise and sunset times depend on the date and the coop's location on Earth. In the northern hemisphere, days are longer during summer and shorter during the winter. The opposite is true in the southern hemisphere.

This project uses a GPS receiver to determine both date and location. From there, it uses some code to calculate sunrise and sunet times. I found some public domain code to do those calculations online at:
http://www.stjarnhimlen.se/

Did you know that chickens adjust their egg output based on day length? Longer days yield more eggs per week. Shorter days, fewer eggs. However, by turning on additiona light in their coop in the morning and evening we can fool their bodies into thinking the day is longer.

Basic Operation:
The arduino reads the data from the GPS and passes it to the GPS parser. If there is valid data, the data is passed to a sunrise/sunset calculator object which figures the times based on late and location.

A door controller object monitors the current time relative to the sunrise and sunset times and decides when to open and close the door. I call the amount of time the door is open the chicken's day length.

There is another object that controls the coop light. It monitors the chicken's day length and decides if the light should be turned on early and left on late. This object also provides additional light after the door is opened in the morning to help the chickens get their day started, and turns it on before the door closes in the evening to draw them back to the coop.

Finally, status and error information is transmitted back to the house to alert us to gps lock problems, the door being stuck, and such things.

This project depends on my GPSParser, PMS, and SaveController libraries.

