#!/usr/bin/env python3
"""
OpenMS Gas Chromatograph Controller

A command-line application for controlling and monitoring a gas chromatograph
connected via serial port.

Features:
- Serial connection to Arduino controller
- Configuration file loading and saving
- Data logging to CSV files
- Interactive and script-based operation
- Time-based temperature and pressure profiles
"""

import argparse
import csv
import json
import os
import sys
import time
from datetime import datetime
import serial
import serial.tools.list_ports

# Default configuration
DEFAULT_CONFIG = {
    "serial_port": "",
    "baud_rate": 115200,
    "log_directory": "logs",
    "temperature_profile": [],
    "pressure_profile": [],
    "ms_gain": 1.0,
    "ms_offset": 0.0,
    "sample_rate": 1000,
    "logging_enabled": True
}

class OpenMSController:
    def __init__(self, config=None):
        """Initialize the controller with optional configuration"""
        self.config = config or DEFAULT_CONFIG.copy()
        self.serial_conn = None
        self.data_file = None
        self.csv_writer = None
        self.running = False
        self.logger_active = False
        
    def connect(self, port=None, baud_rate=None):
        """Connect to the Arduino controller"""
        port = port or self.config.get("serial_port", "")
        baud_rate = baud_rate or self.config.get("baud_rate", 115200)
        
        # Auto-detect port if not specified
        if not port:
            ports = list(serial.tools.list_ports.comports())
            if not ports:
                print("Error: No serial ports found. Please connect the Arduino and try again.")
                return False
            
            # Look for Arduino-like devices
            for p in ports:
                if "Arduino" in p.description or "CH340" in p.description or "USB Serial" in p.description:
                    port = p.device
                    print(f"Auto-detected Arduino on port {port}")
                    break
            
            # If no Arduino-like device found, use the first available port
            if not port and ports:
                port = ports[0].device
                print(f"No Arduino detected. Using first available port: {port}")
        
        try:
            self.serial_conn = serial.Serial(port, baud_rate, timeout=1)
            # Wait for Arduino to reset
            time.sleep(2)
            
            # Flush any pending data
            self.serial_conn.reset_input_buffer()
            self.serial_conn.reset_output_buffer()
            
            # Read welcome message
            response = self.read_line()
            if response:
                print(f"Connected: {response}")
            
            self.running = True
            return True
        except serial.SerialException as e:
            print(f"Error connecting to {port}: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from the Arduino controller"""
        if self.serial_conn and self.serial_conn.is_open:
            self.serial_conn.close()
            self.running = False
            print("Disconnected from controller")
        
        if self.data_file:
            self.data_file.close()
            self.data_file = None
            self.csv_writer = None
            
    def send_command(self, command):
        """Send a command to the Arduino controller"""
        if not self.serial_conn or not self.serial_conn.is_open:
            print("Error: Not connected to controller")
            return False
            
        try:
            # Add newline if not present
            if not command.endswith('\n'):
                command += '\n'
                
            self.serial_conn.write(command.encode('utf-8'))
            self.serial_conn.flush()
            
            # Wait for response
            response = self.read_line()
            if response:
                print(f"Response: {response}")
                
            return True
        except serial.SerialException as e:
            print(f"Error sending command: {e}")
            return False
    
    def read_line(self):
        """Read a line from the serial connection"""
        if not self.serial_conn or not self.serial_conn.is_open:
            return None
            
        try:
            line = self.serial_conn.readline().decode('utf-8').strip()
            return line if line else None
        except serial.SerialException as e:
            print(f"Error reading from serial: {e}")
            return None
        except UnicodeDecodeError:
            # Handle garbage data
            return None
    
    def start_data_logging(self):
        """Start logging data to a CSV file"""
        if self.data_file:
            # Already logging
            return True
            
        log_dir = self.config.get("log_directory", "logs")
        os.makedirs(log_dir, exist_ok=True)
        
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = os.path.join(log_dir, f"gc_data_{timestamp}.csv")
        
        try:
            self.data_file = open(filename, 'w', newline='')
            self.csv_writer = csv.writer(self.data_file)
            
            # Write header
            self.csv_writer.writerow(["timestamp", "time_ms", "temperature", "pressure", "ms_value"])
            
            print(f"Logging data to {filename}")
            self.logger_active = True
            return True
        except IOError as e:
            print(f"Error creating log file: {e}")
            return False
    
    def process_data_line(self, line):
        """Process a data line from the controller and log it if needed"""
        if not line.startswith("DATA:"):
            return
            
        parts = line.split(':')
        if len(parts) != 5:
            return
            
        _, time_ms, temp, press, ms = parts
        
        if self.csv_writer:
            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
            self.csv_writer.writerow([timestamp, time_ms, temp, press, ms])
            self.data_file.flush()
    
    def load_config(self, filename):
        """Load configuration from a JSON file"""
        try:
            with open(filename, 'r') as f:
                config = json.load(f)
                
            # Validate config (basic validation)
            required_fields = ["serial_port", "baud_rate", "log_directory"]
            for field in required_fields:
                if field not in config:
                    config[field] = DEFAULT_CONFIG[field]
                    
            self.config = config
            print(f"Loaded configuration from {filename}")
            return True
        except (IOError, json.JSONDecodeError) as e:
            print(f"Error loading configuration: {e}")
            return False
    
    def save_config(self, filename):
        """Save current configuration to a JSON file"""
        try:
            with open(filename, 'w') as f:
                json.dump(self.config, f, indent=2)
                
            print(f"Saved configuration to {filename}")
            return True
        except IOError as e:
            print(f"Error saving configuration: {e}")
            return False
    
    def load_profile(self, filename):
        """Load temperature and pressure profiles from a JSON file"""
        try:
            with open(filename, 'r') as f:
                profile = json.load(f)
                
            # Check for required fields
            if "temperature_profile" not in profile and "pressure_profile" not in profile:
                print("Error: Profile file must contain temperature_profile and/or pressure_profile")
                return False
                
            # Update profiles in config
            if "temperature_profile" in profile:
                self.config["temperature_profile"] = profile["temperature_profile"]
                
            if "pressure_profile" in profile:
                self.config["pressure_profile"] = profile["pressure_profile"]
                
            print(f"Loaded profile from {filename}")
            
            # Apply profiles to controller
            self.apply_profiles()
            return True
        except (IOError, json.JSONDecodeError) as e:
            print(f"Error loading profile: {e}")
            return False
    
    def apply_profiles(self):
        """Apply temperature and pressure profiles to the controller"""
        # Clear existing profiles
        self.send_command("TEMP_PROFILE:CLEAR")
        self.send_command("PRESS_PROFILE:CLEAR")
        
        # Add temperature profile points
        temp_profile = self.config.get("temperature_profile", [])
        for point in temp_profile:
            if "time" in point and "value" in point:
                cmd = f"TEMP_PROFILE:ADD:{point['time']}:{point['value']}"
                self.send_command(cmd)
        
        # Add pressure profile points
        press_profile = self.config.get("pressure_profile", [])
        for point in press_profile:
            if "time" in point and "value" in point:
                cmd = f"PRESS_PROFILE:ADD:{point['time']}:{point['value']}"
                self.send_command(cmd)
    
    def start_profile(self, profile_type="ALL"):
        """Start temperature and/or pressure profiles"""
        if profile_type not in ["TEMP", "PRESS", "ALL"]:
            print("Error: Profile type must be TEMP, PRESS, or ALL")
            return False
            
        self.send_command(f"START_PROFILE:{profile_type}")
        print(f"Started {profile_type} profile(s)")
        return True
    
    def stop_profile(self, profile_type="ALL"):
        """Stop temperature and/or pressure profiles"""
        if profile_type not in ["TEMP", "PRESS", "ALL"]:
            print("Error: Profile type must be TEMP, PRESS, or ALL")
            return False
            
        self.send_command(f"STOP_PROFILE:{profile_type}")
        print(f"Stopped {profile_type} profile(s)")
        return True
    
    def set_ms_params(self, gain, offset):
        """Set Mass Spectrometer gain and offset"""
        cmd = f"SET_MS:{gain}:{offset}"
        self.send_command(cmd)
        
        # Update config
        self.config["ms_gain"] = gain
        self.config["ms_offset"] = offset
    
    def set_sample_rate(self, rate):
        """Set sample rate in milliseconds"""
        cmd = f"SAMPLE_RATE:{rate}"
        self.send_command(cmd)
        
        # Update config
        self.config["sample_rate"] = rate
    
    def set_logging(self, enabled):
        """Enable or disable data logging"""
        cmd = f"LOGGING:{'ON' if enabled else 'OFF'}"
        self.send_command(cmd)
        
        # Update config
        self.config["logging_enabled"] = enabled
    
    def run_interactive(self):
        """Run in interactive mode, reading from serial and processing commands"""
        if not self.running:
            print("Error: Not connected to controller")
            return
            
        print("Interactive mode (Ctrl+C to exit)")
        print("Enter commands directly, or use these special commands:")
        print("  :help         - Show this help")
        print("  :quit         - Exit the program")
        print("  :start        - Start logging data")
        print("  :stop         - Stop logging data")
        print("  :run temp     - Start temperature profile")
        print("  :run press    - Start pressure profile")
        print("  :run all      - Start all profiles")
        print("  :stop temp    - Stop temperature profile")
        print("  :stop press   - Stop pressure profile")
        print("  :stop all     - Stop all profiles")
        print("  :status       - Request status from controller")
        
        try:
            # Start data logger
            if self.config.get("logging_enabled", True):
                self.start_data_logging()
            
            while self.running:
                # Check for serial data
                while self.serial_conn and self.serial_conn.in_waiting > 0:
                    line = self.read_line()
                    if line:
                        if line.startswith("DATA:"):
                            self.process_data_line(line)
                        else:
                            print(f"< {line}")
                
                # Check for user input (non-blocking)
                if sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
                    cmd = input().strip()
                    
                    if cmd == ":quit":
                        break
                    elif cmd == ":help":
                        print("Interactive mode commands:")
                        print("  :help         - Show this help")
                        print("  :quit         - Exit the program")
                        print("  :start        - Start logging data")
                        print("  :stop         - Stop logging data")
                        print("  :run temp     - Start temperature profile")
                        print("  :run press    - Start pressure profile")
                        print("  :run all      - Start all profiles")
                        print("  :stop temp    - Stop temperature profile")
                        print("  :stop press   - Stop pressure profile")
                        print("  :stop all     - Stop all profiles")
                        print("  :status       - Request status from controller")
                    elif cmd == ":start":
                        self.start_data_logging()
                    elif cmd == ":stop":
                        if self.data_file:
                            self.data_file.close()
                            self.data_file = None
                            self.csv_writer = None
                            self.logger_active = False
                            print("Stopped data logging")
                    elif cmd == ":run temp":
                        self.start_profile("TEMP")
                    elif cmd == ":run press":
                        self.start_profile("PRESS")
                    elif cmd == ":run all":
                        self.start_profile("ALL")
                    elif cmd == ":stop temp":
                        self.stop_profile("TEMP")
                    elif cmd == ":stop press":
                        self.stop_profile("PRESS")
                    elif cmd == ":stop all":
                        self.stop_profile("ALL")
                    elif cmd == ":status":
                        self.send_command("STATUS")
                    else:
                        # Send command directly to controller
                        self.send_command(cmd)
                
                # Sleep to prevent CPU hogging
                time.sleep(0.01)
                
        except KeyboardInterrupt:
            print("\nExiting interactive mode")
            
        finally:
            if self.data_file:
                self.data_file.close()
                self.data_file = None
                self.csv_writer = None
    
def main():
    """Main entry point for the program"""
    parser = argparse.ArgumentParser(description="OpenMS Gas Chromatograph Controller")
    
    # Connection options
    parser.add_argument('-p', '--port', dest='port', help="Serial port to connect to")
    parser.add_argument('-b', '--baud', dest='baud', type=int, default=115200, help="Baud rate for serial connection")
    
    # Configuration options
    parser.add_argument('-c', '--config', dest='config', help="Load configuration from file")
    parser.add_argument('-s', '--save-config', dest='save_config', help="Save configuration to file")
    
    # Profile options
    parser.add_argument('--profile', dest='profile', help="Load profile from file")
    parser.add_argument('--run', dest='run_profile', choices=['temp', 'press', 'all'], help="Run profile(s)")
    
    # Command options
    parser.add_argument('--command', dest='command', help="Send a command to the controller")
    
    # Data logging options
    parser.add_argument('--log-dir', dest='log_dir', help="Directory for data logs")
    parser.add_argument('--no-log', dest='no_log', action='store_true', help="Disable data logging")
    
    # MS options
    parser.add_argument('--ms-gain', dest='ms_gain', type=float, help="MS gain value")
    parser.add_argument('--ms-offset', dest='ms_offset', type=float, help="MS offset value")
    
    # Sample rate
    parser.add_argument('--sample-rate', dest='sample_rate', type=int, help="Sample rate in milliseconds")
    
    # Interactive mode
    parser.add_argument('-i', '--interactive', dest='interactive', action='store_true', help="Run in interactive mode")
    
    args = parser.parse_args()
    
    # Create controller
    controller = OpenMSController()
    
    # Load configuration if specified
    if args.config:
        controller.load_config(args.config)
    
    # Override config with command-line options
    if args.port:
        controller.config["serial_port"] = args.port
    
    if args.baud:
        controller.config["baud_rate"] = args.baud
    
    if args.log_dir:
        controller.config["log_directory"] = args.log_dir
    
    if args.no_log:
        controller.config["logging_enabled"] = False
    
    if args.ms_gain is not None:
        controller.config["ms_gain"] = args.ms_gain
    
    if args.ms_offset is not None:
        controller.config["ms_offset"] = args.ms_offset
    
    if args.sample_rate is not None:
        controller.config["sample_rate"] = args.sample_rate
    
    # Connect to the controller
    if not controller.connect():
        sys.exit(1)
    
    try:
        # Apply configuration
        if args.ms_gain is not None or args.ms_offset is not None:
            controller.set_ms_params(
                controller.config.get("ms_gain", 1.0),
                controller.config.get("ms_offset", 0.0)
            )
        
        if args.sample_rate is not None:
            controller.set_sample_rate(controller.config.get("sample_rate", 1000))
        
        if args.no_log:
            controller.set_logging(False)
        
        # Load and apply profile if specified
        if args.profile:
            controller.load_profile(args.profile)
        
        # Run profile if specified
        if args.run_profile:
            controller.start_profile(args.run_profile.upper())
        
        # Send command if specified
        if args.command:
            controller.send_command(args.command)
        
        # Save configuration if specified
        if args.save_config:
            controller.save_config(args.save_config)
        
        # Run in interactive mode if specified
        if args.interactive:
            # Need to import select module for interactive mode
            import select
            controller.run_interactive()
        # If no interactive mode and no commands, run interactively anyway
        elif not (args.command or args.run_profile):
            print("No command specified, running in interactive mode")
            import select
            controller.run_interactive()
    
    finally:
        controller.disconnect()

if __name__ == "__main__":
    main()