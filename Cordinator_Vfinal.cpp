#include "mbed.h"

// Zigbee UART (TX, RX)
BufferedSerial zigbee(PA_11, PA_12, 9600);

// USB debug
BufferedSerial pc(USBTX, USBRX, 9600);

// Potentiometer analog input
AnalogIn pot(A0);

// Destination Zigbee 64-bit address
uint8_t dest_addr[8] = { 0x00, 0x13, 0xA2, 0x00, 0x42, 0x2D, 0xB5, 0xC4 };

const uint8_t FRAME_TYPE = 0x10;

// Maps a float (0.0–1.0) to an integer range
int map_pot_value(float val, int out_min, int out_max) {
    return static_cast<int>(val * (out_max - out_min) + out_min);
}

void send_api_packet(const char* data, uint8_t* dest64, uint8_t frame_id = 0x01) {
    uint8_t len = 14 + strlen(data);  // Frame data length

    uint8_t frame[100];
    int idx = 0;

    frame[idx++] = 0x7E;  // Start delimiter
    frame[idx++] = 0x00;  // Length MSB
    frame[idx++] = len;   // Length LSB

    frame[idx++] = FRAME_TYPE;  // Frame Type: Transmit Request
    frame[idx++] = frame_id;

    // 64-bit destination address
    for (int i = 0; i < 8; i++) frame[idx++] = dest64[i];

    // 16-bit dest address = 0xFFFE
    frame[idx++] = 0xFF;
    frame[idx++] = 0xFE;

    frame[idx++] = 0x00;  // Broadcast radius
    frame[idx++] = 0x00;  // Options

    // RF Data (payload)
    for (int i = 0; data[i] != '\0'; i++) {
        frame[idx++] = data[i];
    }

    // Checksum
    uint8_t checksum = 0;
    for (int i = 3; i < idx; i++) {
        checksum += frame[i];
    }
    frame[idx++] = 0xFF - checksum;

    // Send via Zigbee UART
    zigbee.write(frame, idx);

    // Debug output
    pc.write("Sent payload: ", 14);
    pc.write(data, strlen(data));
    pc.write(" -> Frame: ", 11);
    for (int i = 0; i < idx; i++) {
        char buf[4];
        sprintf(buf, "%02X ", frame[i]);
        pc.write(buf, strlen(buf));
    }
    pc.write("\n", 1);
}

int main() {
    pc.write("Zigbee Potentiometer Sender Starting...\n", 38);
    ThisThread::sleep_for(2s);

    int last_sent_angle = -1;

    while (true) {
        float pot_value = pot.read();  // 0.0 to 1.0
        int angle = map_pot_value(pot_value, 1, 180);

        if (angle != last_sent_angle) {
            char payload[5];
            sprintf(payload, "%d", angle);

            send_api_packet(payload, dest_addr);

            last_sent_angle = angle;
        }

        ThisThread::sleep_for(200ms);  // Check every 200ms for responsiveness
    }
}
