#include <SPI.h>

// --- Настройки пинов ---
// Задний мотор
#define REAR_MOTOR_PWM 6   // ШИМ заднего мотора
#define REAR_MOTOR_DIR 7   // Направление заднего мотора

// Пины для энкодера
#define ENCODER_CS_PIN 10 // CS энкодера
#define ENCODER_CLK_PIN 13 // CLK энкодера (SCK SPI)
#define ENCODER_DATA_PIN 12 // MISO энкодера (данные)

// Пины для квадратурного энкодера
#define ENCODER_A_PIN 8     // Канал A
#define ENCODER_B_PIN 9     // Канал B

// Пины для мотора
#define FRONT_MOTOR_LEFT 5 // ШИМ переднего мотора
#define FRONT_MOTOR_RIGHT 4 // Направление переднего мотора

// Диапазон углов колес
#define WHEEL_ANGLE_MIN -30 // Минимальный угол поворота колес
#define WHEEL_ANGLE_MAX 30  // Максимальный угол поворота колес

// Переменные энкодера
volatile long encoderPosition = 0; // Положение энкодера
long lastEncoderPosition = 0;      // Положение энкодера в предыдущую секунду
unsigned long lastReportTime = 0;  // Время последнего отчета

// Диапазон углов энкодера (градусы)
int ENCODER_ANGLE_RIGHT=0;
int ENCODER_ANGLE_LEFT=0;

// Параметр П-контроллера
float Kp = 0.5;

// Целевой угол поворота колес
int targetWheelAngle = 0;

int readEncoder(int* position = NULL);
void handleEncoder();

void setup() {
	
  // Настройка пинов квадратурного энкодера
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), handleEncoder, CHANGE);
  
  // Настройка пинов энкодера
  pinMode(ENCODER_CS_PIN, OUTPUT);
  digitalWrite(ENCODER_CS_PIN, HIGH); // CS по умолчанию высокий

  // Настройка SPI
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);          // Работаем в режиме SPI MODE0
  SPI.setClockDivider(SPI_CLOCK_DIV16); // Делитель частоты (подходит для большинства энкодеров)

  // Настройка пинов моторов
  pinMode(REAR_MOTOR_PWM, OUTPUT);
  pinMode(REAR_MOTOR_DIR, OUTPUT);
  pinMode(FRONT_MOTOR_RIGHT, OUTPUT);
  pinMode(FRONT_MOTOR_LEFT, OUTPUT);


  //test_encoder_max(&ENCODER_ANGLE_RIGHT, &ENCODER_ANGLE_LEFT);

  // Отладка
  Serial.begin(9600);
  Serial.println("Driver ready");
}

void loop() {
  delay(50);
  digitalWrite(FRONT_MOTOR_LEFT, HIGH); // Вправо
  digitalWrite(FRONT_MOTOR_RIGHT, LOW); // Вправо
  

  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.startsWith("SPEED")) { // Установка скорости заднего мотора
      int speed = command.substring(6).toInt();
      setRearMotorSpeed(speed);
    } else if (command.startsWith("ANGLE")) { // Установка угла переднего мотора
      int angle = command.substring(6).toInt();
      // Ограничиваем целевой угол в пределах допустимого диапазона
      targetWheelAngle = constrain(angle, WHEEL_ANGLE_MIN, WHEEL_ANGLE_MAX);
      controlSteering(angle);

    } else if (command.startsWith("TELEMETRY")){
      Serial.println("Front Angle: " + String(targetWheelAngle) +" deg | " +"Rear Speed: " + String(lastEncoderPosition) +" ticks/sec");     
    } else {
      Serial.println("Invalid Command");
    }
  }
  // Отладка
  Serial.print(targetWheelAngle);
  Serial.println();
}

void test_encoder_max(int* right, int* left){
  digitalWrite(FRONT_MOTOR_LEFT, HIGH); // Вправо
  analogWrite(FRONT_MOTOR_RIGHT, 0);
  digitalWrite(ENCODER_CS_PIN, LOW); // Активируем энкодер
  delay(1000);

  // Считываем данные энкодера (например, 16-битное значение)
  byte highByte = SPI.transfer(0x00); // Высший байт
  byte lowByte = SPI.transfer(0x00);  // Низший байт

  digitalWrite(ENCODER_CS_PIN, HIGH); // Деактивируем энкодер
  // Объединяем байты
  *right = (highByte << 8) | lowByte;
  
  digitalWrite(FRONT_MOTOR_RIGHT, HIGH); // Активируем энкодер
  analogWrite(FRONT_MOTOR_LEFT, 0);

  delay(1000);

  // Считываем данные энкодера (например, 16-битное значение)
  highByte = SPI.transfer(0x00); // Высший байт
  lowByte = SPI.transfer(0x00);  // Низший байт

  digitalWrite(ENCODER_CS_PIN, HIGH); // Деактивируем энкодер
  // Объединяем байты
  *left = (highByte << 8) | lowByte;

  analogWrite(FRONT_MOTOR_RIGHT, 255);
  analogWrite(FRONT_MOTOR_LEFT, 255);

}

// Функция управления поворотом колес
void controlSteering(int targetAngle) {
  int currentAngle;


  do {
    // Считываем текущий угол с энкодера
    currentAngle = readEncoder();

    // Вычисляем ошибку
    int error = targetAngle - currentAngle;

    // Пропорциональное управление
    int motorSpeed = Kp * abs(error);

    // Ограничиваем скорость
    motorSpeed = constrain(motorSpeed, 0, 255);

    // Определяем направление вращения
    if (error < 0) {
      digitalWrite(FRONT_MOTOR_RIGHT, HIGH); // Вправо
      analogWrite(FRONT_MOTOR_LEFT, 0);

    } else {
      digitalWrite(FRONT_MOTOR_LEFT, HIGH); // Влево
      analogWrite(FRONT_MOTOR_RIGHT, 0);

    }


    delay(50); // Задержка для стабильности
  } while (abs(targetAngle - currentAngle) > 1); // Условие выхода, если ошибка мала
  analogWrite(FRONT_MOTOR_RIGHT, 255);
  analogWrite(FRONT_MOTOR_LEFT, 255);

}
// Обработчик прерывания для квадратурного энкодера
void handleEncoder() {
  int stateA = digitalRead(ENCODER_A_PIN);
  int stateB = digitalRead(ENCODER_B_PIN);

  // Определяем направление вращения
  if (stateA == stateB) {
    encoderPosition++;
  } else {
    encoderPosition--;
  }
}


// Функция для чтения данных с энкодера
int readEncoder() {
  digitalWrite(ENCODER_CS_PIN, LOW); // Активируем энкодер
  delayMicroseconds(1);

  // Считываем данные энкодера (например, 16-битное значение)
  byte highByte = SPI.transfer(0x00); // Высший байт
  byte lowByte = SPI.transfer(0x00);  // Низший байт

  digitalWrite(ENCODER_CS_PIN, HIGH); // Деактивируем энкодер
  // Объединяем байты
  int position = (highByte << 8) | lowByte;
  Serial.print(position);
  // Преобразуем в угол (если энкодер выдает значения от 0 до 4096)
  int angle = double(double(position - ENCODER_ANGLE_LEFT) / double(ENCODER_ANGLE_RIGHT - ENCODER_ANGLE_LEFT)) * 60;
  Serial.print(" ");
  Serial.println(angle);
  return angle;
}

// --- Управление скоростью заднего мотора ---
void setRearMotorSpeed(int speed) {
  if (speed > 255) speed = 255;
  if (speed < -255) speed = -255;
  
  speed=255+speed;
  if (speed >= 0) {
    digitalWrite(REAR_MOTOR_DIR, HIGH);
    analogWrite(REAR_MOTOR_PWM, speed);
  } else {
    digitalWrite(REAR_MOTOR_DIR, LOW);
    analogWrite(REAR_MOTOR_PWM, speed);
  }

  Serial.print("Rear motor speed set to: ");
  Serial.println(speed);
}
