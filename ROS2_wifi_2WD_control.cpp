/*
  [최종 코드: micro-ROS 무선 로봇 제어]
  - Wi-Fi로 micro-ROS Agent에 접속
  - "/cmd_vel" 토픽을 구독(Subscribe)함
  - ROS 2 명령을 받아 모터를 제어함 (튜닝된 핀맵/속도 적용)
*/

// ===========================================
// ★★★★★ (TO-DO 1) Wi-Fi 및 Agent IP 수정 ★★★★★
// ===========================================
#include <WiFi.h>
char ssid[] = "";      // 1. const char* -> char[]
char password[] = ""; // 2. const char* -> char[]
char agent_ip[] = "";    // 3. IPAddress -> char[]
// ===========================================

// ===========================================
// ★★★★★ (TO-DO 2) micro-ROS 라이브러리 ★★★★★
// ===========================================
#include <micro_ros_arduino.h>
#include <stdio.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>

// "Twist" 메시지 타입을 사용 (선속도 linear.x, 각속도 angular.z)
#include <geometry_msgs/msg/twist.h>
// ===========================================

// ===========================================
// ★★★★★ (TO-DO 3) 6시간의 산물: 핀맵 & 튜닝 ★★★★★
// ===========================================
// 최종 핀맵
#define LEFT_MOTOR_ENA_PIN  23  // ENA
#define LEFT_MOTOR_IN1_PIN  18  // IN1
#define LEFT_MOTOR_IN2_PIN  19  // IN2
#define RIGHT_MOTOR_ENB_PIN 26  // ENB
#define RIGHT_MOTOR_IN3_PIN 33  // IN3
#define RIGHT_MOTOR_IN4_PIN 25  // IN4

// PWM 설정 (2.0.14 버전 기준)
#define PWM_FREQ 1000
#define PWM_RESOLUTION 8
#define LEFT_PWM_CHANNEL 0
#define RIGHT_PWM_CHANNEL 1

// ★ (중요!) 튜닝한 속도 값을 여기에 넣으세요!
int leftSpeed = 220;  // 예: 느린 왼쪽 튜닝 값
int rightSpeed = 180; // 예: 빠른 오른쪽 튜닝 값
// ===========================================

// --- micro-ROS 객체들 ---
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;
rcl_subscription_t subscriber;
geometry_msgs__msg__Twist msg; // /cmd_vel 메시지를 담을 변수

// ================================================================
// ★★★★★ (핵심!) /cmd_vel 명령을 받았을 때 실행되는 함수 ★★★★★
// ================================================================
void subscription_callback(const void * mssgin) {
  // 메시지를 Twist 타입으로 변환
  const geometry_msgs__msg__Twist * msg = (const geometry_msgs__msg__Twist *)mssgin;

  // ROS 2 명령 해석:
  // msg->linear.x > 0  => "앞으로 가!"
  // msg->linear.x < 0  => "뒤로 가!"
  // msg->angular.z > 0 => "왼쪽으로 돌아!"
  // msg->angular.z < 0 => "오른쪽으로 돌아!"

  if (msg->linear.x > 0) {
    moveForward();
  } else if (msg->linear.x < 0) {
    moveBackward();
  } else if (msg->angular.z > 0) {
    turnLeft();
  } else if (msg->angular.z < 0) {
    turnRight();
  } else {
    stopMotors(); // 아무 명령도 없으면 정지
  }
}

// --- setup() ---
void setup() {
  Serial.begin(115200);

  // --- 1. 모터 핀 초기화 (지식 A) ---
  pinMode(LEFT_MOTOR_IN1_PIN, OUTPUT);
  pinMode(LEFT_MOTOR_IN2_PIN, OUTPUT);
  pinMode(RIGHT_MOTOR_IN3_PIN, OUTPUT);
  pinMode(RIGHT_MOTOR_IN4_PIN, OUTPUT);
  ledcSetup(LEFT_PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(RIGHT_PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(LEFT_MOTOR_ENA_PIN, LEFT_PWM_CHANNEL);
  ledcAttachPin(RIGHT_MOTOR_ENB_PIN, RIGHT_PWM_CHANNEL);
  stopMotors(); // 일단 정지

  // --- 2. Wi-Fi 및 micro-ROS Agent 연결 (지식 B) ---
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Wi-Fi connected!");

  // micro-ROS 설정 (UDP, Agent IP, 8888 포트)
  set_microros_wifi_transports(ssid, password, agent_ip, 8888);
  
  allocator = rcl_get_default_allocator();

  // micro-ROS 초기화
  rclc_support_init(&support, 0, NULL, &allocator);

  // 노드(Node) 생성: "esp32_robot_node"
  rclc_node_init_default(&node, "esp32_robot_node", "", &support);

  // 구독자(Subscriber) 생성:
  // "/cmd_vel" 토픽을 구독하고, 메시지가 오면 "subscription_callback" 함수를 실행
  rclc_subscription_init_default(
    &subscriber,
    &node,
    ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
    "/cmd_vel"); // <-- 바로 이 토픽!

  // 실행기(Executor) 생성:
  // 구독자가 메시지를 받았는지 1초에 100번 확인
  rclc_executor_init(&executor, &support.context, 1, &allocator);
  rclc_executor_add_subscription(&executor, &subscriber, &msg, &subscription_callback, ON_NEW_DATA);

  Serial.println("micro-ROS setup complete. Waiting for /cmd_vel commands...");
}

// --- loop() ---
void loop() {
  // micro-ROS 실행기가 10ms 동안 "듣도록" 함
  rclc_executor_spin_some(&executor, RCL_MS_TO_NS(10));
}

// ===========================================
// ★★★★★ (지식 A) 모터 제어 함수들 ★★★★★
// ===========================================

void moveForward() {
  digitalWrite(LEFT_MOTOR_IN1_PIN, HIGH);
  digitalWrite(LEFT_MOTOR_IN2_PIN, LOW);
  ledcWrite(LEFT_PWM_CHANNEL, leftSpeed);
  
  digitalWrite(RIGHT_MOTOR_IN3_PIN, HIGH);
  digitalWrite(RIGHT_MOTOR_IN4_PIN, LOW);
  ledcWrite(RIGHT_PWM_CHANNEL, rightSpeed);
}

void moveBackward() {
  digitalWrite(LEFT_MOTOR_IN1_PIN, LOW);
  digitalWrite(LEFT_MOTOR_IN2_PIN, HIGH);
  ledcWrite(LEFT_PWM_CHANNEL, leftSpeed);

  digitalWrite(RIGHT_MOTOR_IN3_PIN, LOW);
  digitalWrite(RIGHT_MOTOR_IN4_PIN, HIGH);
  ledcWrite(RIGHT_PWM_CHANNEL, rightSpeed);
}

void turnLeft() {
  // 왼쪽 뒤로, 오른쪽 앞으로
  digitalWrite(LEFT_MOTOR_IN1_PIN, LOW);
  digitalWrite(LEFT_MOTOR_IN2_PIN, HIGH);
  ledcWrite(LEFT_PWM_CHANNEL, leftSpeed);

  digitalWrite(RIGHT_MOTOR_IN3_PIN, HIGH);
  digitalWrite(RIGHT_MOTOR_IN4_PIN, LOW);
  ledcWrite(RIGHT_PWM_CHANNEL, rightSpeed);
}

void turnRight() {
  // 왼쪽 앞으로, 오른쪽 뒤로
  digitalWrite(LEFT_MOTOR_IN1_PIN, HIGH);
  digitalWrite(LEFT_MOTOR_IN2_PIN, LOW);
  ledcWrite(LEFT_PWM_CHANNEL, leftSpeed);

  digitalWrite(RIGHT_MOTOR_IN3_PIN, LOW);
  digitalWrite(RIGHT_MOTOR_IN4_PIN, HIGH);
  ledcWrite(RIGHT_PWM_CHANNEL, rightSpeed);
}

void stopMotors() {
  ledcWrite(LEFT_PWM_CHANNEL, 0);
  ledcWrite(RIGHT_PWM_CHANNEL, 0);
}
