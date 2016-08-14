# Micrium-Alert-System
A proximity alert system must be designed and implemented using the following elements:
• The HC-SR04 Ultrasonic Ranging Module;
• The FRDM-K64F board;
• The Micrium μC/OS.
The FRDM-K64F shall read the distance of any object located in front of the HC-SR04 sensor,
and then it shall operate the RGB led as follows:
• If the distance of the nearest object is equal or greater than 2 meters the GREEN led
shall blink with a period of 2.000 ms;
• If the distance of the nearest object is between 1 and 2 meters the BLUE led shall blink
with a period inversely proportional to the distance, according to the following table:
Distance [m]    [1.0 – 1.2)   [1.2-1.4)   [1.4-1.6)   [1.6-1.8)   [1.8-2)
Period [ms]     200           400         600         800         1.000
• If the distance of the nearest object is smaller than 1 meter the RED led shall blink
with a period inversely proportional to the distance, according to the following table:
Distance [cm]   < 10   [10-25)   [25-50)   [50-75)  [75-100)
Period [ms]     ∞      400       600       800      1.000
