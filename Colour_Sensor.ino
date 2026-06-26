#include <Arduino.h>

// --- PIN ASSIGNMENTS ---
#define S0 18
#define S1 19
#define S2 21
#define S3 22
#define OUT_PIN 23
const int servoPin = 13;   // Ensure your Servo Signal wire is on GPIO 13

// --- CALIBRATION DATA ---
int redMin = 15;   
int greenMin = 15;
int blueMin = 15;

int redMax = 400;  
int greenMax = 400;
int blueMax = 400;
// ------------------------

// Variables to store 0-255 values
int redValue;
int greenValue;
int blueValue;

// Track the last processed color to avoid slamming the servo repeatedly
String lastServoColor = "UNKNOWN";

// Declaration of functions
int getRawRed();
int getRawGreen();
int getRawBlue();
String identifyColor();
void setServoAngle(int angle);

void setup() {
  Serial.begin(115200);

  // Set up control pins
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  
  pinMode(servoPin, OUTPUT);
  pinMode(OUT_PIN, INPUT);

  // Set Frequency Scaling to 20%
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);

  // Initialize servo to center position (90 degrees) on startup
  Serial.println("Initializing Servo...");
  setServoAngle(90);

  Serial.println("\n=========================================");
  Serial.println("  TCS3200 Sorter - 5 CORE COLORS MOD    ");
  Serial.println("=========================================");
  delay(1000);
}

void loop() {
  // 1. Read raw timings from the sensor
  int rawRed = getRawRed();
  delay(20); 

  int rawGreen = getRawGreen();
  delay(20);

  int rawBlue = getRawBlue();
  delay(20);

  // 2. Map timings directly to standard 0-255 RGB space
  int instantRed   = map(rawRed, redMin, redMax, 255, 0);
  int instantGreen = map(rawGreen, greenMin, greenMax, 255, 0);
  int instantBlue  = map(rawBlue, blueMin, blueMax, 255, 0);

  // 3. ASSIGN VALUES DIRECTLY
  redValue   = instantRed;
  greenValue = instantGreen;
  blueValue  = instantBlue;

  // Apply red sensitivity dampening (10% reduction)
  redValue = redValue * .9;

  // Constrain protection boundaries
  redValue   = constrain(redValue, 0, 255);
  greenValue = constrain(greenValue, 0, 255);
  blueValue  = constrain(blueValue, 0, 255);

  // 4. Identify the color
  String detectedColor = identifyColor();

  // 5. Print out data clearly to help you debug live hardware
  Serial.print("RGB values: [");
  Serial.print(redValue);   Serial.print(", ");
  Serial.print(greenValue); Serial.print(", ");
  Serial.print(blueValue);  Serial.print("]");
  Serial.print(" (Raw R="); Serial.print(rawRed); Serial.print(")");
  Serial.print("  --> COLOR: "); Serial.println(detectedColor);

  // 6. SERVO DRIVE LOGIC (Configured for 5 unique distribution slots)
  if (detectedColor != lastServoColor) {
    if (detectedColor == "RED") {
      Serial.println(">> [SERVO ACTION] Routing RED to 0 Degrees <<");
      setServoAngle(95);
      lastServoColor = detectedColor;
    } 
    else if (detectedColor == "BLACK") {
      Serial.println(">> [SERVO ACTION] Routing BLACK to 45 Degrees <<");
      setServoAngle(135);
      lastServoColor = detectedColor;
    }
    else if (detectedColor == "GREEN") {
      Serial.println(">> [SERVO ACTION] Routing GREEN to 90 Degrees <<");
      setServoAngle(60);
      lastServoColor = detectedColor;
    } 
    else if (detectedColor == "BLUE") {
      Serial.println(">> [SERVO ACTION] Routing BLUE to 135 Degrees <<");
      setServoAngle(30);
      lastServoColor = detectedColor;
    }
    else if (detectedColor == "WHITE") {
      Serial.println(">> [SERVO ACTION] Routing WHITE to 180 Degrees <<");
      setServoAngle(180);
      lastServoColor = detectedColor;
    }
    // UNKNOWN states bypass the motor commands completely
  }

  delay(200); // Loop cycle pace
}

// --- HARDWARE INTERACTION FUNCTIONS ---

int getRawRed() {
  digitalWrite(S2, LOW);
  digitalWrite(S3, LOW);
  return pulseIn(OUT_PIN, LOW, 50000); 
}

int getRawGreen() {
  digitalWrite(S2, HIGH);
  digitalWrite(S3, HIGH);
  return pulseIn(OUT_PIN, LOW, 50000);
}

int getRawBlue() {
  digitalWrite(S2, LOW);
  digitalWrite(S3, HIGH);
  return pulseIn(OUT_PIN, LOW, 50000);
}

// --- FIXED SERVO MOTOR DRIVE ENGINE ---
void setServoAngle(int angle) {
  int pulsewidth = map(angle, 0, 180, 500, 2500);
  
  for (int i = 0; i < 35; i++) {
    digitalWrite(servoPin, HIGH);
    delayMicroseconds(pulsewidth);
    digitalWrite(servoPin, LOW);
    delayMicroseconds(20000 - pulsewidth); 
  }
}

// --- ADJUSTED COLOR DECISION LOGIC ---
String identifyColor() {
  int totalBrightness = redValue + greenValue + blueValue;

  // 1. BLACK: Weak light reflection across all filters
  if (totalBrightness < 450) { 
    return "BLACK";
  }

  // 2. WHITE: Strong, evenly balanced brightness values
  if (totalBrightness > 660) {
    if (abs(redValue - greenValue) < 65 && 
        abs(greenValue - blueValue) < 45 && 
        abs(blueValue - redValue) < 65) {
      return "WHITE";
    }
  }

  // 3. PRIMARY RED
  if (redValue > greenValue && redValue > blueValue) {
    return "RED";
  }

  // 4. PRIMARY GREEN
  if (greenValue > redValue && greenValue > blueValue) {
    return "GREEN";
  }

  // 5. PRIMARY BLUE
  if (blueValue > redValue && blueValue > greenValue) {
    return "BLUE";
  }

  return "UNKNOWN";
}