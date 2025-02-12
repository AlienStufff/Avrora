import serial
import time


class MotorController:
    def __init__(self, port, baudrate=9600, timeout=1):
        """
        Инициализация подключения к Arduino.
        :param port: COM-порт, к которому подключено устройство.
        :param baudrate: Скорость передачи данных.
        :param timeout: Таймаут для чтения данных.
        """
        self.ser = serial.Serial(port, baudrate, timeout=timeout)
        time.sleep(2)  # Задержка для установки соединения
        self.ser.flush()  # Очистка буфера

    def send_command(self, command):
        """
        Отправить команду в Arduino.
        :param command: Строка команды.
        """
        self.ser.write((command + '\n').encode('utf-8'))
        time.sleep(0.1)  # Небольшая задержка

    def read_response(self):
        """
        Прочитать ответ из Arduino.
        :return: Строка с ответом.
        """
        if self.ser.in_waiting > 0:
            return self.ser.readline().decode('utf-8').strip()
        return None

    def set_speed(self, speed):
        """
        Установить скорость заднего мотора.
        :param speed: Скорость в диапазоне [-255, 255].
        """
        if -255 <= speed <= 255:
            command = f"SPEED {speed}"
            self.send_command(command)
        else:
            raise ValueError("Speed out of range! Must be between -255 and 255.")

    def set_angle(self, angle):
        """
        Установить угол переднего мотора.
        :param angle: Угол в диапазоне [-30, 30].
        """
        if -30 <= angle <= 30:
            command = f"ANGLE {angle}"
            self.send_command(command)
        else:
            raise ValueError("Angle out of range! Must be between -30 and 30.")

    def read_telemetry(self):
        """
        Считать телеметрию от Arduino (угол и скорость).
        :return: Словарь с данными об угле и скорости.
        """
        self.send_command("TELEMETRY")
        response = self.read_response()
        if response:
            if "Front Angle" in response and "Rear Speed" in response:
                try:
                    angle = float(response.split("Front Angle: ")[1].split(" deg")[0])
                    speed = float(response.split("Rear Speed: ")[1].split(" ticks/sec")[0])
                    return {"angle": angle, "speed": speed}
                except (ValueError, IndexError):
                    raise ValueError("Error parsing telemetry response.")
        return None

    def close(self):
        """
        Закрыть соединение.
        """
        self.ser.close()
