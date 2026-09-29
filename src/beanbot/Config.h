#ifndef BEANBOT_CONFIG_H
#define BEANBOT_CONFIG_H

#include "Types.h"

namespace Config {

namespace Communication {
constexpr bool useWifi = true;
constexpr unsigned long baud = 115200;
constexpr int shieldPin = 25;
constexpr char commandStart[] = "CMDS/";
constexpr char commandEnd[] = "/CMDEND/";
constexpr char setupStart[] = "SETUPB/";
constexpr char setupEnd[] = "/SETUPE/";
}

// The startup defaults and the local/demo request were different in the sketch.
constexpr beanbot::Order initialOrder = {50, 50, 100};
constexpr beanbot::Order demoOrder = {100, 150, 200};
constexpr int reservoirCount = 3;

namespace Servo {
constexpr int carriageConnector = 12;
constexpr int gateConnector = 11;
constexpr int probeConnector = 9;
constexpr int lastChannel = 15; // PCA9685 channel = 15 - shield connector.
constexpr int frequencyHz = 50;
constexpr unsigned long oscillatorHz = 27000000;
constexpr int pulseMin = 80;
constexpr int pulseMax = 409;
constexpr int minimumAngle = 0;
constexpr int standardRange = 180;
constexpr int carriageForward = 0;
constexpr int carriageBack = 180;
constexpr int gateClosed = 145;
constexpr int gateOpen = 45; // Preserve the implementation, not the old "65" comment.
constexpr int probeRange = 270;
constexpr int probeTop = 270;
constexpr int firstProbeStep = 1;
constexpr int lastProbeStep = 120;
constexpr int probeStep = 1;
constexpr unsigned long travelDelayMs = 10000;
// Preserve the existing delayMicroseconds conversion; its units are unresolved.
constexpr double probeDelayUs = 2.5;
}

namespace Scale {
constexpr int dataPin = 3;
constexpr int clockPin = 2;
constexpr long calibrationFactor = -7050.0;
}

namespace ColorSensor {
constexpr int s0 = 45;
constexpr int s1 = 4;
constexpr int s2 = 53;
constexpr int s3 = 46;
constexpr int outputPin = 28;
constexpr int enablePin = 52;
constexpr unsigned long readingDelayMs = 100;

// Retain the original table, including unused ranges. The active classifier
// still uses only redMaxWhite and greenMinBlack, then falls back to red.
constexpr int redMinRed = 56;
constexpr int redMaxRed = 70;
constexpr int greenMinRed = 94;
constexpr int greenMaxRed = 118;
constexpr int blueMinRed = 83;
constexpr int blueMaxRed = 109;
constexpr int redMinWhite = 10;
constexpr int redMaxWhite = 170;
constexpr int greenMinWhite = 10;
constexpr int greenMaxWhite = 93;
constexpr int blueMinWhite = 10;
constexpr int blueMaxWhite = 82;
constexpr int redMinBlack = 71;
constexpr int redMaxBlack = 130;
constexpr int greenMinBlack = 220;
constexpr int greenMaxBlack = 160;
constexpr int blueMinBlack = 110;
constexpr int blueMaxBlack = 160;
}

namespace Conveyor {
constexpr int enablePin = 9;
constexpr int reversePin = 8;
constexpr unsigned long reverseDelayMs = 10000;
}

namespace Frame {
constexpr int stepPin = 12;
constexpr int directionPin = 13;
constexpr int enablePin = 11;
constexpr int smallSteps = 8;
constexpr int largeSteps = 17;
constexpr unsigned long pulseDelayMs = 100;
constexpr unsigned long settleDelayMs = 5000;
}

namespace DistanceSensor {
constexpr int triggerPin = 44;
constexpr int echoPin = 48;
constexpr unsigned int triggerLowUs = 2;
constexpr unsigned int triggerHighUs = 10;
constexpr long microsecondsPerCentimeter = 29;
}

namespace Inventory {
// Original literals AND types are retained here. Fractional truncation and
// the arithmetic that uses these values are explicit stage-3 corrections.
constexpr long maximumHeight = 4.72;
constexpr long base = 18.29;
constexpr long angle = 17;
constexpr long length = 12.98;
constexpr long rpm = 71.43;
constexpr long initialHeight = 10;
constexpr long radius = 2.4;
constexpr long whiteRedBeanWeight = 1 / 3;
constexpr long blackBeanWeight = 1 / 6;
constexpr long whiteRedBeanVolume = 0.75;
constexpr long blackBeanVolume = 0.50;
constexpr double timeIncrement = 2.5;
constexpr double pi = 3.1415;
}

namespace Display {
constexpr int rs = 21; // Preserved; the known Mega SCL conflict is not changed here.
constexpr int enablePin = 14;
constexpr int d4 = 19;
constexpr int d5 = 7;
constexpr int d6 = 6;
constexpr int d7 = 18;
constexpr int columns = 16;
constexpr int rows = 2;
constexpr int redPosition = 2;
constexpr int whitePosition = 3;
constexpr int blackPosition = 1;
}

constexpr unsigned long unloadDelayMs = 10000;

} // namespace Config

#endif
