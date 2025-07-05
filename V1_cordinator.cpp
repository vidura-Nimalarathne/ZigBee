#include "mbed.h"

// Analog input
AnalogIn pot(A0);

// Serial
BufferedSerial pc(USBTX, USBRX, 115200);

// Servo wrapper
class Servo {
public:
    Servo(PinName pin) : pwm(pin) {
        pwm.period(0.02f); // 20ms = 50Hz
    }

    void write_angle(int angle) {
        angle = angle < 0 ? 0 : (angle > 180 ? 180 : angle);
        float pulse = 0.0005f + (angle / 180.0f) * 0.002f;
        pwm.pulsewidth(pulse);
    }

private:
    PwmOut pwm;
};

// Create servo on D9
Servo servo(D9);

// Mapping function
int map_value(float input, int out_min, int out_max) {
    return (int)(out_min + input * (out_max - out_min));
}

int main() {
  

    while (true) {
        float analog_value = pot.read(); // 0.0 to 1.0
        int angle = map_value(analog_value, 0, 180);
        servo.write_angle(angle);
        printf("Servo Angle: %d\n", angle);
        thread_sleep_for(100);
    }
}
