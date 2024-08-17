#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <string>
#include <iostream>

#include "candy.h"

Candy::Candy() {
    std::cout << "[*] Starting candy" << std::endl;
    this->error = "";
    this->_connected = false;

};

Candy::~Candy() {
    this->closeCanLink();
};

void Candy::setup() {
    std::cout << "[*] Starting can link" << std::endl;
    int status = this->setupCanLink();
    if(status != 0){
        std::cout << this->error << std::endl;
    };
};

void Candy::shutdown() {
    std::cout << "[*] Closing can link" << std::endl;
    this->closeCanLink();
};

std::string Candy::getError() {
    return this->error;
};

int Candy::setupCanLink() {
    struct sockaddr_can addr;
    struct ifreq ifr;
    int ret;

    std::cout << "Setting up can link" << std::endl;
    system("sudo ip link set can0 type can bitrate 10400");
    system("sudo ifconfig can0 up");

    this->s = socket(PF_CAN, SOCK_RAW, CAN_RAW);

    if (this->s < 0) {
        this->error = "Socket failure";
        return 1;
    };

    strcpy(ifr.ifr_name, "can0");
    ret = ioctl(this->s, SIOCGIFINDEX, &ifr);
    if (ret < 0) {
        this->error = "Device failure";
        return 1;
    };

    addr.can_family = PF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    ret = bind(this->s, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        this->error = "Bind failure";
        return 1;
    };

    this->_connected = true;
    return 0;
}

void Candy::closeCanLink() {
    close(this->s);
    system("sudo ifconfig can0 down");
};

bool Candy::isConnected(){
    return this->_connected;
};

can_frame Candy::recieve() {
    std::cout << "Listening for can data" << std::endl;
    int nbytes;
    struct can_frame frame;
    
    memset(&frame, 0, sizeof(struct can_frame));

    //4.Define receive rules
    //struct can_filter rfilter[1];
    //rfilter[0].can_id = 0x123;
    //rfilter[0].can_mask = CAN_SFF_MASK;
    //setsockopt(this->s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

    while(1) {
        nbytes = read(this->s, &frame, sizeof(frame));
        if(nbytes > 0) {
            printf("can_id = 0x%X\r\ncan_dlc = %d \r\n", frame.can_id, frame.can_dlc);
            int i = 0;
            for(i = 0; i < 8; i++)
                printf("data[%d] = %d\r\n", i, frame.data[i]);
            return frame;
        };
    };
};

int Candy::send() {
    int nbytes;
    struct can_frame frame;
    memset(&frame, 0, sizeof(struct can_frame));

    //4.Disable filtering rules, do not receive packets, only send
    setsockopt(this->s, SOL_CAN_RAW, CAN_RAW_FILTER, NULL, 0);

    //5.Set send data
    frame.can_id = 0x123;
    frame.can_dlc = 8;
    frame.data[0] = 1;
    frame.data[1] = 2;
    frame.data[2] = 3;
    frame.data[3] = 4;
    frame.data[4] = 5;
    frame.data[5] = 6;
    frame.data[6] = 7;
    frame.data[7] = 8;

    printf("can_id  = 0x%X\r\n", frame.can_id);
    printf("can_dlc = %d\r\n", frame.can_dlc);
    int i = 0;
    for(i = 0; i < 8; i++)
        printf("data[%d] = %d\r\n", i, frame.data[i]);

    //6.Send message
    nbytes = write(this->s, &frame, sizeof(frame));
    if(nbytes != sizeof(frame)) {
        printf("Send Error frame[0]!\r\n");
        system("sudo ifconfig can0 down");
    }

    return 0;
}
