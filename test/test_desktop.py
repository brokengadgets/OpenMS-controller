import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "desktop")))
import openms_controller as cli  # noqa: E402


class FakeSerial:
    def __init__(self, rx_lines=None):
        self.is_open = True
        self.rx_lines = [line.encode("utf-8") for line in (rx_lines or [])]
        self.tx_writes = []

    @property
    def in_waiting(self):
        return len(self.rx_lines)

    def write(self, data):
        self.tx_writes.append(data.decode("utf-8"))

    def flush(self):
        pass

    def readline(self):
        if self.rx_lines:
            return self.rx_lines.pop(0)
        return b""

    def close(self):
        self.is_open = False


class TestOpenMSControllerCLI(unittest.TestCase):
    def test_send_command_handles_interleaved_data_and_echo(self):
        ctrl = cli.OpenMSController()
        fake = FakeSerial([
            "DATA:1000:45.20:15.10:1.2345\n",
            "CMD: STATUS\n",
            "STATUS:1:45.00:0:0:1.00:0.00:1000:1\n",
            "RESP:0:Command executed successfully\n",
        ])
        ctrl.serial_conn = fake
        ctrl.running = True

        with tempfile.TemporaryDirectory() as tmpdir:
            ctrl.config["log_directory"] = tmpdir
            ctrl.start_data_logging()
            ok = ctrl.send_command("STATUS")
            self.assertTrue(ok)
            self.assertEqual(fake.tx_writes, ["STATUS\n"])
            ctrl.disconnect()

            files = os.listdir(tmpdir)
            self.assertEqual(len(files), 1)
            with open(os.path.join(tmpdir, files[0]), "r") as f:
                rows = f.read().strip().splitlines()
            self.assertEqual(len(rows), 2)
            self.assertIn("45.20,15.10,1.2345", rows[1])

    def test_monitor_run_completes_when_status_reports_inactive(self):
        ctrl = cli.OpenMSController()
        ctrl.config["temperature_profile"] = [{"time": 0, "value": 40.0}, {"time": 10, "value": 100.0}]
        ctrl.config["pressure_profile"] = []
        fake = FakeSerial([
            "DATA:100:40.10:14.90:0.5000\n",
            "STATUS:1:45.00:0:0:1.00:0.00:1000:1\n",
            "DATA:200:99.90:15.00:0.6000\n",
            "Temperature profile completed\n",
            "STATUS:0:0:0:0:1.00:0.00:1000:1\n",
        ])
        ctrl.serial_conn = fake
        ctrl.running = True

        with tempfile.TemporaryDirectory() as tmpdir:
            ctrl.config["log_directory"] = tmpdir
            completed = ctrl.monitor_run("TEMP")
            self.assertTrue(completed)
            files = os.listdir(tmpdir)
            self.assertEqual(len(files), 1)
            with open(os.path.join(tmpdir, files[0]), "r") as f:
                rows = f.read().strip().splitlines()
            self.assertEqual(len(rows), 3)  # header + 2 DATA rows

    def test_offline_load_profile_and_save_config(self):
        ctrl = cli.OpenMSController()
        sample_profile = os.path.abspath(
            os.path.join(os.path.dirname(__file__), "..", "desktop", "sample_profile.json")
        )
        self.assertTrue(ctrl.load_profile(sample_profile))
        self.assertGreater(len(ctrl.config["temperature_profile"]), 0)

        with tempfile.TemporaryDirectory() as tmpdir:
            out_cfg = os.path.join(tmpdir, "saved.json")
            self.assertTrue(ctrl.save_config(out_cfg))
            ctrl2 = cli.OpenMSController()
            self.assertTrue(ctrl2.load_config(out_cfg))
            self.assertEqual(ctrl2.config["temperature_profile"], ctrl.config["temperature_profile"])


if __name__ == "__main__":
    unittest.main()
