#include "mbed.h"

// Zigbee RX (from Zigbee TX)
BufferedSerial zigbee(PA_11, PA_12, 9600);  // Use appropriate pins

// USB debug output
BufferedSerial pc(USBTX, USBRX, 9600);

// SG90 Servo on D9 (PWM capable)
PwmOut servo(D9);

// Function to convert angle (1–180) to PWM duty cycle for SG90
void set_servo_angle(int angle) {
    float min_duty = 0.025f;  // 0.5ms / 20ms
    float max_duty = 0.125f;  // 2.5ms / 20ms
    float duty = min_duty + (angle / 180.0f) * (max_duty - min_duty);

    servo.write(duty);  // Set duty cycle (0.0–1.0)
    
    pc.write("Servo set to angle: ", 21);
    char buf[6];
    sprintf(buf, "%d\n", angle);
    pc.write(buf, strlen(buf));
}

// Extract payload from Zigbee API frame (assumes frame is complete)
void process_api_frame(uint8_t* frame, int length) {
    const int payload_start = 15;
    if (length <= payload_start) return;

    char payload[10] = {0};
    int payload_len = length - payload_start - 1;  // exclude checksum
    if (payload_len > 9) payload_len = 9;

    for (int i = 0; i < payload_len; i++) {
        payload[i] = frame[payload_start + i];
    }
    payload[payload_len] = '\0';  // Ensure null-terminated string

    // ✅ Fix #3: Print the payload for debug
    pc.write("Received payload: ", 18);
    pc.write(payload, strlen(payload));
    pc.write("\n", 1);

    int angle = atoi(payload);  // Convert payload to int
    if (angle >= 1 && angle <= 180) {
        set_servo_angle(angle);
    } else {
        pc.write("Invalid angle received\n", 23);
    }
}

int main() {
    pc.write("Zigbee Servo Receiver Starting...\n", 34);

    // Setup PWM for SG90
    servo.period(0.02f);  // 20ms period (50Hz)
    set_servo_angle(1);  // Start at center

    uint8_t buffer[100];
    int idx = 0;
    bool receiving = false;
    uint16_t expected_length = 0;

    while (true) {
        if (zigbee.readable()) {
            char byte;
            zigbee.read(&byte, 1);

            if (!receiving) {
                if (byte == 0x7E) {
                    idx = 0;
                    buffer[idx++] = byte;
                    receiving = true;
                }
            } else {
                buffer[idx++] = byte;

                if (idx == 3) {
                    // After 3 bytes, we have full length
                    expected_length = (buffer[1] << 8) | buffer[2];
                    expected_length += 4;  // Include start, length bytes, and checksum
                }

                if (idx >= expected_length && expected_length > 0) {
                    pc.write("Zigbee message received\n", 25);
                    process_api_frame(buffer, expected_length);
                    receiving = false;
                    idx = 0;
                }

                if (idx >= 100) {  // overflow guard
                    receiving = false;
                    idx = 0;
                }
            }
        }
    }
}
