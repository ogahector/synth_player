#pragma once
#include <bitset>

class Knob {
    private:
        int upperLimit;
        int lowerLimit;
        std::bitset<2> prev;
        std::bitset<2> curr;
        int rotation;
        int missed;
    public: 
        Knob(int lower = 0, int upper = 8){
            upperLimit = upper;
            lowerLimit = lower;
            rotation = 0;
            prev = 0b00;
            curr = 0b00;
        }
        int update(std::bitset<32U>::reference input0, std::bitset<32U>::reference input1){
            prev = curr;
            curr[0] = input0;
            curr[1] = input1;
            if ( (prev == 0b00 && curr == 0b01) || (prev == 0b11 && curr == 0b10)){
                if (rotation < upperLimit) rotation += 1;
                missed = 1;
            }
            else if ((prev == 0b01 && curr == 0b00) || (prev == 0b10 && curr == 0b11)){
                if (rotation > lowerLimit) rotation -= 1;
                missed = -1;
            } 
            //Can be added in to catch missed steps for faster rotation, but also may cause overloading with very fast rotation
            // else if ((prev == 0b00 && curr == 0b11) || (prev == 0b11 && curr == 0b00)){
            //     if (rotation > lowerLimit && rotation < upperLimit) rotation += missed;
            // }
            return rotation;
        }

        int getRotation(){
            return rotation;
        }

        void setLimits(int upper, int lower){
            upperLimit = upper;
            lowerLimit = lower;
        }  
};