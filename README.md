# Micrium-Alert-System
A proximity alert system has been designed and implemented using the following elements:<br />
• The HC-SR04 Ultrasonic Ranging Module;<br />
• The FRDM-K64F board;<br />
• The Micrium μC/OS.<br />
The management of the HC-SR04 has been implemented using the FlexTimer Module (see Freescale “K64 Sub-Family Reference Manual”).<br />
The FRDM-K64F reads the distance of any object located in front of the HC-SR04 sensor,
and then it operates the RGB led as follows:<br />
• If the distance of the nearest object is equal or greater than 2 meters the GREEN led
shall blink with a period of 2.000 ms;<br />
• If the distance of the nearest object is between 1 and 2 meters the BLUE led shall blink
with a period inversely proportional to the distance, according to the following table:

| Distance [m] | [1.0 – 1.2)  | [1.2-1.4) |  [1.4-1.6) | [1.6-1.8) | [1.8-2)
|:-------------:|:---:|:-------:|:-------:|:-------:|:--------:|
| Period [ms] |  200      |     400    |     600      | 800  |       1.000

• If the distance of the nearest object is smaller than 1 meter the RED led shall blink
with a period inversely proportional to the distance, according to the following table:<br />

| Distance [cm] |  <10  | [10-25) | [25-50) |  [50-75) | [75-100) |
|:-------------:|:---:|:-------:|:-------:|:-------:|:--------:|:-----:|
| Period [ms]   |  ∞   |  400   |   600   |    800   |   1.000 |
