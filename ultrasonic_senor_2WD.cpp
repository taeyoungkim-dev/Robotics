/*
  [프로젝트: 초음파 센서 장애물 회피 로봇]
  - 평소에는 앞으로 전진합니다.
  - HC-SR04 초음파 센서로 전방 거리를 측정합니다.
  - 약 20cm 이내에 장애물이 감지되면,
  - 멈췄다가 -> 오른쪽으로 회전 -> 다시 전진을 시작합니다.
*/

// ===========================================
// ★★★ 사용자가 제공한 핀맵 ★★★
// ===========================================
// 모터 핀
#define LEFT_MOTOR_ENA_PIN  23  // ENA (PWM)
#define LEFT_MOTOR_IN1_PIN  18  // IN1
#define LEFT_MOTOR_IN2_PIN  19  // IN2
#define RIGHT_MOTOR_ENB_PIN 26  // ENB (PWM)
#define RIGHT_MOTOR_IN3_PIN 33  // IN3
#define RIGHT_MOTOR_IN4_PIN 25  // IN4

// 초음파 센서 핀
#define TRIG_PIN 16 // Trig (출력)
#define ECHO_PIN 17 // Echo (입력)

// ===========================================
// ★★★ 모터 속도 및 PWM 설정 ★★★
// ===========================================
// PWM 설정
#define PWM_FREQ 1000       // 1kHz
#define PWM_RESOLUTION 8    // 8비트 (0-255)
#define LEFT_PWM_CHANNEL 0  // PWM 채널 0
#define RIGHT_PWM_CHANNEL 1 // PWM 채널 1

// 모터 속도 (0~255 사이 값)
// 로봇이 무겁다고 하셨으니 최대로 설정합니다.
int motorSpeed = 255; 

// ===========================================
// ★★★ 장애물 감지 설정 (튜닝 필요!) ★★★
// ===========================================
#define OBSTACLE_DISTANCE 10  // 장애물로 인식할 거리 (cm)
#define TURN_DURATION 500    // 회전할 시간 (1000ms = 1초). 이 값을 조절해 90도에 가깝게 튜닝하세요.

// ================================================================
// === setup() : 시작 시 한 번만 실행 ===
// ================================================================
void setup() {
  Serial.begin(115200); // 디버깅을 위한 시리얼 모니터 시작
  Serial.println("장애물 회피 로봇 시작!");

  // --- 1. 모터 핀 초기화 ---
  pinMode(LEFT_MOTOR_IN1_PIN, OUTPUT);
  pinMode(LEFT_MOTOR_IN2_PIN, OUTPUT);
  pinMode(RIGHT_MOTOR_IN3_PIN, OUTPUT);
  pinMode(RIGHT_MOTOR_IN4_PIN, OUTPUT);

  // ESP32 PWM 설정
  ledcSetup(LEFT_PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(RIGHT_PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);

  // PWM 핀 연결
  ledcAttachPin(LEFT_MOTOR_ENA_PIN, LEFT_PWM_CHANNEL);
  ledcAttachPin(RIGHT_MOTOR_ENB_PIN, RIGHT_PWM_CHANNEL);

  // --- 2. 초음파 센서 핀 초기화 ---
  pinMode(TRIG_PIN, OUTPUT); // Trig 핀은 초음파를 쏘므로 출력
  pinMode(ECHO_PIN, INPUT);  // Echo 핀은 신호를 받으므로 입력

  // --- 3. 모터 정지 ---
  stopMotors();
  delay(1000); // 시작 전 1초 대기
}

// ================================================================
// === loop() : 메인 루프 (계속 반복) ===
// ================================================================
void loop() {
  // 1. 전방 거리 측정
  long distance = getDistance();
  
  Serial.print("측정 거리: ");
  Serial.print(distance);
  Serial.println(" cm");

  // 2. 장애물 감지 로직
  // (거리가 0보다 크고) 설정한 거리(OBSTACLE_DISTANCE)보다 가까우면
  if (distance > 0 && distance < OBSTACLE_DISTANCE) {
    Serial.println("!!! 장애물 감지 !!!");

    // 2-1. 일단 정지
    Serial.println("정지");
    stopMotors();
    delay(500); // 0.5초 대기

    // 2-2. 오른쪽으로 회전 (제자리 회전)
    Serial.println("오른쪽으로 회전");
    turnRight();
    delay(TURN_DURATION); // ★ 설정한 시간만큼 회전 (이 시간을 조절해 90도 회전을 맞추세요)

    // 2-3. 다시 정지 (회전 멈춤)
    stopMotors();
    delay(500); // 0.5초 대기
    
  } else {
    // 3. 장애물이 없으면 -> 전진
    Serial.println("전진");
    moveForward();
  }

  // 4. 너무 자주 측정하지 않도록 잠시 대기
  delay(100); // 0.1초마다 거리 측정 반복
}

// ===========================================
// ★★★ 초음파 거리 측정 함수 ★★★
// ===========================================
long getDistance() {
  // 1. Trig 핀으로 10us 펄스 발사
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10); // 10 마이크로초
  digitalWrite(TRIG_PIN, LOW);

  // 2. Echo 핀이 HIGH가 되는 시간(duration) 측정 (단위: 마이크로초)
  //    (타임아웃 30000us -> 약 5미터)
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); 

  // 3. 거리 계산 (cm)
  //    속도 = 340m/s = 0.034cm/us
  //    거리 = (시간 * 속도) / 2  (왕복이니까 2로 나눔)
  //    거리 = (duration * 0.034) / 2
  //    거리 = duration / 58.8 (이 공식을 더 많이 씀)
  
  long distance_cm = duration / 58.8;

  return distance_cm;
}

// ===========================================
// ★★★ 모터 제어 함수들 ★★★
// (이전 코드와 동일, 속도 변수만 적용)
// ===========================================

void moveForward() {
  digitalWrite(LEFT_MOTOR_IN1_PIN, HIGH);
  digitalWrite(LEFT_MOTOR_IN2_PIN, LOW);
  ledcWrite(LEFT_PWM_CHANNEL, motorSpeed);
  
  digitalWrite(RIGHT_MOTOR_IN3_PIN, HIGH);
  digitalWrite(RIGHT_MOTOR_IN4_PIN, LOW);
  ledcWrite(RIGHT_PWM_CHANNEL, motorSpeed);
}

void moveBackward() {
  digitalWrite(LEFT_MOTOR_IN1_PIN, LOW);
  digitalWrite(LEFT_MOTOR_IN2_PIN, HIGH);
  ledcWrite(LEFT_PWM_CHANNEL, motorSpeed);

  digitalWrite(RIGHT_MOTOR_IN3_PIN, LOW);
  digitalWrite(RIGHT_MOTOR_IN4_PIN, HIGH);
  ledcWrite(RIGHT_PWM_CHANNEL, motorSpeed);
}

void turnLeft() {
  // 왼쪽 뒤로, 오른쪽 앞으로 (제자리 좌회전)
  digitalWrite(LEFT_MOTOR_IN1_PIN, LOW);
  digitalWrite(LEFT_MOTOR_IN2_PIN, HIGH);
  ledcWrite(LEFT_PWM_CHANNEL, motorSpeed);

  digitalWrite(RIGHT_MOTOR_IN3_PIN, HIGH);
  digitalWrite(RIGHT_MOTOR_IN4_PIN, LOW);
  ledcWrite(RIGHT_PWM_CHANNEL, motorSpeed);
}

void turnRight() {
  // 왼쪽 앞으로, 오른쪽 뒤로 (제자리 우회전)
  digitalWrite(LEFT_MOTOR_IN1_PIN, HIGH);
  digitalWrite(LEFT_MOTOR_IN2_PIN, LOW);
  ledcWrite(LEFT_PWM_CHANNEL, motorSpeed);

  digitalWrite(RIGHT_MOTOR_IN3_PIN, LOW);
  digitalWrite(RIGHT_MOTOR_IN4_PIN, HIGH);
  ledcWrite(RIGHT_PWM_CHANNEL, motorSpeed);
}

void stopMotors() {
  ledcWrite(LEFT_PWM_CHANNEL, 0);
  ledcWrite(RIGHT_PWM_CHANNEL, 0);
}