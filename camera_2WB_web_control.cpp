//좌우 바뀌어 있음
#include <WiFi.h>
#include <WebServer.h>

// ================= [ 사용자 설정 구간 ] =================
const char* ssid     = "";     // ★와이파이 이름
const char* password = "";   // ★와이파이 비밀번호

// 카메라 주소 (아까 확인한 주소!)
String camera_url = "[여기주소]/stream"; 
// ======================================================

// [핀 번호 설정] (보내주신 정답 코드 기준)
#define LEFT_MOTOR_ENA_PIN  23  // PWM
#define LEFT_MOTOR_IN1_PIN  18
#define LEFT_MOTOR_IN2_PIN  19

#define RIGHT_MOTOR_ENB_PIN 26  // PWM
#define RIGHT_MOTOR_IN3_PIN 33
#define RIGHT_MOTOR_IN4_PIN 25

// [PWM 설정] (정답 코드의 설정을 그대로 가져옴)
#define PWM_FREQ 1000       // 1kHz
#define PWM_RESOLUTION 8    // 8비트
#define LEFT_PWM_CHANNEL 0  // 채널 0
#define RIGHT_PWM_CHANNEL 1 // 채널 1

// 속도 (0 ~ 255)
int motorSpeed = 255; // 풀파워

WebServer server(8080); // 포트 8080 유지

String getHTML() {
  String html = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<style>";
  html += "body { text-align: center; background-color: #121212; color: white; margin: 0; user-select: none; }";
  html += "h2 { margin-top: 10px; }";
  
  // ★ [수정 1] 카메라 회전 0도로 변경 (뒤집힘 해결)
  html += "img { width: 100%; max-width: 400px; transform: rotate(0deg); border-radius: 10px; }"; 
  
  html += ".btn-group { margin-top: 20px; display: grid; grid-template-columns: 1fr 1fr 1fr; max-width: 300px; margin-left: auto; margin-right: auto; gap: 10px; }";
  html += "button { height: 80px; font-size: 30px; border: none; border-radius: 15px; background-color: #333; color: white; touch-action: manipulation; }";
  html += "button:active { background-color: #4CAF50; }"; 
  html += "</style>";
  
  html += "<script>";
  html += "function move(dir) { fetch('/' + dir); }"; 
  html += "function stop() { fetch('/stop'); }";      
  html += "</script>";
  
  html += "</head><body>";
  
  html += "<h2>ESP32 ROVER</h2>";
  html += "<img src='" + camera_url + "'><br>";
  
  html += "<div class='btn-group'>";
  html += "<div></div>"; 
  html += "<button onmousedown=\"move('forward')\" onmouseup=\"stop()\" ontouchstart=\"move('forward')\" ontouchend=\"stop()\">▲</button>";
  html += "<div></div>"; 
  
  html += "<button onmousedown=\"move('left')\" onmouseup=\"stop()\" ontouchstart=\"move('left')\" ontouchend=\"stop()\">◀</button>";
  html += "<button onclick=\"stop()\">■</button>";
  html += "<button onmousedown=\"move('right')\" onmouseup=\"stop()\" ontouchstart=\"move('right')\" ontouchend=\"stop()\">▶</button>";
  
  html += "<div></div>"; 
  html += "<button onmousedown=\"move('backward')\" onmouseup=\"stop()\" ontouchstart=\"move('backward')\" ontouchend=\"stop()\">▼</button>";
  html += "<div></div>"; 
  html += "</div>";
  
  html += "</body></html>";
  return html;
}

// ===========================================
// ★ [수정 2] 모터 제어 함수 (정답 코드의 로직 이식)
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
  // 제자리 좌회전 (왼쪽 뒤로, 오른쪽 앞으로)
  digitalWrite(LEFT_MOTOR_IN1_PIN, LOW);
  digitalWrite(LEFT_MOTOR_IN2_PIN, HIGH);
  ledcWrite(LEFT_PWM_CHANNEL, motorSpeed);

  digitalWrite(RIGHT_MOTOR_IN3_PIN, HIGH);
  digitalWrite(RIGHT_MOTOR_IN4_PIN, LOW);
  ledcWrite(RIGHT_PWM_CHANNEL, motorSpeed);
}

void turnRight() {
  // 제자리 우회전 (왼쪽 앞으로, 오른쪽 뒤로)
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

void handleRoot() { server.send(200, "text/html", getHTML()); }

void setup() {
  Serial.begin(115200);
  
  // 1. 핀 모드 설정
  pinMode(LEFT_MOTOR_IN1_PIN, OUTPUT);
  pinMode(LEFT_MOTOR_IN2_PIN, OUTPUT);
  pinMode(RIGHT_MOTOR_IN3_PIN, OUTPUT);
  pinMode(RIGHT_MOTOR_IN4_PIN, OUTPUT);

  // 2. PWM 설정 (이게 없어서 왼쪽이 안 돌았을 수 있습니다!)
  ledcSetup(LEFT_PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(RIGHT_PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(LEFT_MOTOR_ENA_PIN, LEFT_PWM_CHANNEL);
  ledcAttachPin(RIGHT_MOTOR_ENB_PIN, RIGHT_PWM_CHANNEL);

  // 3. 와이파이 연결
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  
  Serial.println("\nRobot Controller Ready!");
  Serial.print("Controller IP: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":8080");

  server.on("/", handleRoot);
  
  // 웹 요청을 정답 모터 함수에 매핑
  server.on("/forward",  [](){ moveForward(); server.send(200, "text/plain", "OK"); });
  server.on("/backward", [](){ moveBackward(); server.send(200, "text/plain", "OK"); });
  server.on("/left",     [](){ turnLeft(); server.send(200, "text/plain", "OK"); });
  server.on("/right",    [](){ turnRight(); server.send(200, "text/plain", "OK"); });
  server.on("/stop",     [](){ stopMotors(); server.send(200, "text/plain", "OK"); });
  
  server.begin();
}

void loop() {
  server.handleClient();
}
