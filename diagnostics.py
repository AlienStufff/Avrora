import threading
import subprocess
from motor_controller import MotorController
from parking_sensor import ParkingSensor


def diagnostic_thread(controller, sensor, proc):
    """
    Фоновый поток для диагностики.
    Отправляет данные от энкодеров и парктроников в диагностическую консоль.
    """
    print("Диагностический поток запущен.")
    try:
        while True:
            # Чтение данных с энкодеров
            encoder_data = controller.ser.readline()
            if encoder_data:
                proc.stdin.write("ENCODER: " + encoder_data.decode('utf-8').strip() + '\n')
                proc.stdin.flush()

            # Чтение данных с парктроников
            sensor_data = sensor.read_data()
            if sensor_data:
                sensors = sensor_data["sensors"]
                mode = "Front" if sensor_data["mode"] == 0 else "Rear"
                proc.stdin.write(f"PARKING: Sensors = {sensors}, Mode = {mode}\n")
                proc.stdin.flush()
    except Exception as e:
        print(f"Диагностический поток завершён: {e}")


def main():
    """
    Главный интерфейс управления.
    """
    command = ['python', 'diagnostics_consol.py']

    proc = subprocess.Popen(
        command,
        stdin=subprocess.PIPE,
        text=True,
        creationflags=subprocess.CREATE_NEW_CONSOLE
    )

    print("=== Car Control Interface ===")
    print("Commands:")
    print("  speed <value>   - Set rear motor speed (-255 to 255)")
    print("  angle <value>   - Set steering angle (-30 to 30)")
    print("  telemetry       - Read telemetry data")
    print("  exit            - Exit the program")

    motor_port = input("Enter motor COM port (e.g., COM3): ")
    sensor_port = input("Enter parking sensor COM port (e.g., COM4): ")

    try:
        controller = MotorController(motor_port)
        sensor = ParkingSensor(sensor_port)
    except Exception as e:
        print(f"Failed to connect: {e}")
        return

    thread = threading.Thread(target=diagnostic_thread, args=(controller, sensor, proc))
    thread.daemon = True
    thread.start()

    try:
        while True:
            command = input("Enter command: ").strip().lower()
            if command.startswith("speed"):
                try:
                    value = int(command.split()[1])
                    controller.set_speed(value)
                    print(f"Speed set to: {value}")
                except (IndexError, ValueError):
                    print("Invalid speed value. Usage: speed <value>")
            elif command.startswith("angle"):
                try:
                    value = int(command.split()[1])
                    controller.set_angle(value)
                    print(f"Angle set to: {value}")
                except (IndexError, ValueError):
                    print("Invalid angle value. Usage: angle <value>")
            elif command == "telemetry":
                try:
                    telemetry = controller.read_telemetry()
                    if telemetry:
                        print(f"Telemetry: Angle = {telemetry['angle']}°, Speed = {telemetry['speed']} ticks/sec")
                    else:
                        print("No telemetry data received.")
                except Exception as e:
                    print(f"Telemetry error: {e}")
            elif command == "exit":
                print("Exiting program.")
                break
            else:
                print("Unknown command. Type 'help' for a list of commands.")
    except KeyboardInterrupt:
        print("Program interrupted.")
    finally:
        controller.close()
        sensor.close()
        print("Connections closed.")


if __name__ == "__main__":
    main()
