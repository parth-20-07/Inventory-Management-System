# **Dash Dot Robotics - eInventory Management System**

This repository is created to store the data on the Central Reciever for the Automatic Inventorial Management System. 



## **What is the aim of the project?**

The project aims at creating an IoT Connected system which is used to manage the inventory of the user.Our product is connected to Amazon Web Services which stores the data on server and the data can be used to store the historical inventory, daily updates and future prediction for inventory restocking.

---

## **How the product works?**
Our product is a two part circuit:
1. ### **Slave Box Circuits**
    Slave Boxes are the smaller nodes installed inside inventory holding crates, these node circuits contains following peripherals:
    1. Microcontroller
    2. Load Cell
    3. Temperature Sensor
    4. Battery Pack
    5. NRF23L01

    During the calibration cycle of the box, the weight of individual component is measured of the components contained inside the box.
    
    During regular functioning, the device remains in sleep mode. When it recieved the signal from NRF24, it wakes up and Load Cell is used to measure the total weight inside the inventory box. Number of components is calculated by the following formula:

    $\ Number of components = \frac{Total Weight Measured by Load Cell}{Weight of individual component}$
    
    The number of components and temperature from temperature sensor is sent to the master reciever when requested. After data is sent, the device goes back to sleep.

2. ### **Master Reciever Circuit**
    The master reciever is a central node which connects to multiple boxes in order to collect data from it. The Master reciever requires following peripherals:
    1. Main Microcontroller
    2. Real Time Clock Module
    3. Power Supply Module
    4. WiFi Connectivity
    5. MicroSD Module
    6. NRF24L01 Module
    7. Rotary Encoder
    8. Li-Ion Cell
    9. Li-Ion Cell Charging Unit
    10. OLED Display
    11. NeoPixel RGB LED Ring
   
   The device always functions in active state. The device is capable of performing following operations:
   - Polling Device for Regular Updates:
     - As per a decided time period, the Reciever starts polling the slave boxes 